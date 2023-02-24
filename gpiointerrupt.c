/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// Driver Header files
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#define DISPLAY(x) UART_write(uart, &output, x);

/***********************
 *
 * Global Declarations
 *
 ***********************/

/*
 * Struct to hold task items
 */
typedef struct task {
    int state; // Task's current state
    unsigned long period; // Task tick rate
    unsigned long elapsedTime; // Time since last tick
    int (*TickFct)(int); // Task's tick function
} task;

/*
 * For use by task scheduler
 */
task tasks[3]; // Array of tasks, three total
const unsigned char tasksNum = 3; // Total number of tasks
const unsigned long tasksPeriodGCD = 100; // GCD of all task periods
const unsigned long periodCheckButtons = 200;
const unsigned long periodCheckTemp = 500;
const unsigned long periodWriteUART = 1000;

/*
 * Data to print to UART
 */
unsigned char setPoint = 18; // Temperature set point
char ambientTemp; // Ambient temperature
unsigned char heat = 0; // Heater indicator. 0 if off (LED is off) or 1 if on (LED is on)
unsigned long seconds = 0; // Seconds since board was reset

/*
 * Flags
 */
volatile unsigned char timerFlag = 0;
volatile unsigned char buttonLeftFlag = 0;
volatile unsigned char buttonRightFlag = 0;

/*
 * Enums for Task State Machines
 */
enum CB_States { CB_SMStart, CB_Check, CB_Increment, CB_Decrement } CB_State; // Check buttons task
enum CT_States { CT_SMStart, CT_On, CT_Off } CT_State; // Check temperature task
enum PU_States { PU_SMStart, PU_Report, PU_Wait } PU_State; // Print to UART task

/*************************
 *
 * Driver Global Variables
 *
 *************************/

/*
 * UART
 */
char output[64];
int bytesToSend;

UART_Handle uart;

/*
 * I2C
 */
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[3] = {
            { 0x48, 0x0000, "11X" },
            { 0x49, 0x0000, "116" },
            { 0x41, 0x0001, "006" }
};

uint8_t txBuffer[1];
uint8_t rxBuffer[2];

I2C_Transaction i2cTransaction;
I2C_Handle i2c;

/***********************************
 *
 * Function Forward Declarations
 *
 ***********************************/

// Driver initializers
void initGPIO(void);
void initUART(void);
void initI2C(void);
void initTimer(void);

// Callbacks
void buttonLeftCallback(uint_least8_t index);
void buttonRightCallback(uint_least8_t index);
void timerCallback(Timer_Handle myHandle, int_fast16_t status);

// Tick functions
int TickFct_CheckButtons(int state);
int TickFct_CheckTemp(int state);
int TickFct_WriteToUART(int state);

void taskScheduler();
int16_t readTemp(void);

/*********************
 *
 * Main thread
 *
 *********************/

void *mainThread(void *arg0)
{
    // Initialize drivers
    initGPIO();
    initUART();
    initI2C();
    initTimer();

    unsigned char i = 0;
    // Task to check buttons and increase/decrease set point
    tasks[i].state = CB_SMStart;
    tasks[i].period = periodCheckButtons;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_CheckButtons;
    i++;
    // Task to check temp and control LED
    tasks[i].state = CT_SMStart;
    tasks[i].period = periodCheckTemp;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_CheckTemp;
    i++;
    // Task to write to the UART
    tasks[i].state = PU_SMStart;
    tasks[i].period = periodWriteUART;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_WriteToUART;

    while(1)
    {
        taskScheduler();
        while(!timerFlag) {}
        timerFlag = 0;
    }
}

/**************************
 *
 * Task Scheduler
 *
 **************************/

void taskScheduler()
{
    unsigned char i;
    // Loop through each task
    for (i = 0; i < tasksNum; i++) {
        // Check if task period has been reached
        if (tasks[i].elapsedTime >= tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += tasksPeriodGCD;
    }
}

/**************************************************
 *
 * Callback functions for the board's side buttons
 * and the Timer.
 *
 **************************************************/

/*
 * Callback for left and right buttons. Use a flag
 * instead of placing the code here directly so
 * that buttons can be checked every 200 ms.
 */
void buttonLeftCallback(uint_least8_t index)
{
    buttonLeftFlag = 1;
}

void buttonRightCallback(uint_least8_t index)
{
    buttonRightFlag = 1;
}

/*
 * Callback for timer. Use a flag instead of placing the code here
 * directly to avoid conflict with UART interrupts.
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    timerFlag = 1;
}

/**************************************************
 *
 * Task Tick Functions
 *
 **************************************************/

/*
 * Check Buttons
 */
int TickFct_CheckButtons(int state)
{
    switch(state)
    {
        case CB_SMStart:
            state = CB_Check;
            break;
        case CB_Check:
            if (buttonRightFlag) {
                state = CB_Increment;
            }
            else if (buttonLeftFlag) {
                state = CB_Decrement;
            }
            break;
        case CB_Increment:
        case CB_Decrement:
            state = CB_Check;
            break;
        default:
            state = CB_SMStart;
            break;
    }
    switch(state)
    {
        case CB_Increment:
            buttonRightFlag = 0;
            if (setPoint < 99) {
                setPoint++;
            }
            break;
        case CB_Decrement:
            buttonLeftFlag = 0;
            if (setPoint > 0) {
                setPoint--;
            }
            break;
        default:
            break;
    }
    return state;
}

/*
 * Check temp and control LED
 */
int TickFct_CheckTemp(int state)
{
    switch(state)
    {
        case CT_SMStart:
            state = CT_Off;
            break;
        case CT_Off:
            if (ambientTemp < setPoint) {
                state = CT_On;
            }
            break;
        case CT_On:
            if (ambientTemp >= setPoint) {
                state = CT_Off;
            }
            break;
        default:
            state = CT_SMStart;
            break;
    }
    switch(state)
    {
        case CT_On:
            heat = 1;
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            ambientTemp = readTemp();
            break;
        case CT_Off:
            heat = 0;
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            ambientTemp = readTemp();
            break;
        default:
            break;
    }
    return state;
}

/*
 * Write to UART
 */
int TickFct_WriteToUART(int state)
{
    switch(state)
    {
        case PU_SMStart:
            state = PU_Report;
            break;
        case PU_Report:
            break;
        default:
            state = PU_SMStart;
            break;
    }
    switch(state)
    {
        case PU_Report:
            // This function is called every second, so increment seconds
            seconds++;
            DISPLAY(snprintf(output, 64, "<%02d, %02d, %d, %04d>\n\r", ambientTemp, setPoint, heat, seconds))
    }
    return state;
}

/**************************************************
 *
 * Initialize GPIO
 *
 **************************************************/

void initGPIO(void)
{
    // Call driver init functions
    GPIO_init();

    // Configure the LED and button pins
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    // Turn on user LED
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

    // Install button callbacks
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, buttonLeftCallback);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1, buttonRightCallback);

    // Enable interrupts
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
}

/**************************************************
 *
 * Initialize UART
 *
 **************************************************/

void initUART(void)
{
    UART_Params uartParams;

    // Init the driver
    UART_init();

    // Configure the driver
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;

    // Open the driver
    uart = UART_open(CONFIG_UART_0, &uartParams);

    if (uart == NULL) {
        // UART_open() failed
        while (1);
    }
}

/***************************************************
 *
 * Initialize I2C and Read Temperature
 *
 ***************************************************/

void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;
    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses

    // Common I2C transaction setup
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.slaveAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;

        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY(snprintf(output, 64, "Found\n\r"))
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.slaveAddress))
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}

int16_t readTemp(void)
{
    int16_t temperature = 0;

    i2cTransaction.readCount = 2;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
        * Extract degrees C from the received data;
        * see TMP sensor datasheet
        */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;

        /*
        * If the MSB is set '1', then we have a 2's complement
        * negative value which needs to be sign extended
        */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }
    return temperature;
}

/**************************************************
 *
 * Initialize Timer
 *
 **************************************************/

void initTimer(void)
{
    Timer_Handle timer0;
    Timer_Params params;

    // Init the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        // Failed to initialized timer
        while (1) {}
    }
    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        // Failed to start timer
        while (1) {}
    }
}
