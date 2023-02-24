# CS350-Embedded-Systems
<b> About the Project </b>

The provided artifacts are a compressed folder of all of the project files, a single C file of the heart of the code for easy viewing, and the task scheduler documentation.

My hypothetical employer, SysTec, would like enter the smart thermostat market. The goal is to develop a thermostat that sends data to the server over Wi-Fi, but first a prototype needs to be created using the TI SimpleLink CC3220S LaunchPad. The TMP006 temperature sensor will be used to read the room temperature (via I2C), the LED will indicate the output to the thermostat where LED on means the heater is on (via GPIO), two buttons will be used to increase or decrease the temperature set point (via GPIO interrupt), and the UART will be used to simulate sending data to the server.

<b> Project Functionality </b>

The code checks the status of the buttons every 200 ms. If a button has been pressed, the temperature set point is either increased or decreased by one degree, depending on which button was pressed. The room's ambient temperature is checked every 500 ms, and if the ambient temperature is below the temperature set point, the LED is turned on to indicate the heater being turned on. Once the ambient temperature is equal to or above the set point, the LED turns off (heater is off). Every second, the data is reported to the UART in the form of <AA, BB, C, DDDD> where AA is the ambient temperature, BB is the temperature set point, C is the heater status (0 for off, 1 for on), and DDDD is the number of seconds since system reset.

<b> Tools Used </b>

TI SimpleLink CC3220S LaunchPad Development Kit
<br>
&emsp;•	The LaunchPad was used to test code functionality and create the prototype. It comes with all the necessary functionality: I2C, UART, GPIO, &emsp;temperature sensor, LED lights, and buttons.
<br>
<br>
Code Composer Studio
<br>
&emsp;•	Code Composer Studio was used to write the code and was used beacuse it is an IDE made specifically for developing applications for Texas &emsp;Instruments embedded processors.

<br>
<br>

<b> Reproducing the Project </b>

•	Import the project into code Composer Studio
<br>
<br>
•	Plug in the LaunchPad via USB.
<br>
<br>
•	Open a serial console to communicate with the board.
<br>
&emsp;- On Windows, determine which com port the board is assigned to by opening Windows Device Manager > Ports(COM & LPT). The USB connection to the TI &emsp;board is called XDS110, and for this project look for XDS110 Class Application/USer UART. Take note of the COMx port. 
<br>
&emsp;- On Mac, determine which com port the board is assigned to by opening Apple menu > About this Mac > System Report > Hardware > USB. In the Device Tree look for the XDS110 entry and take note of the serial number listed.
<br>
&emsp;- In Code Composer Studio, select "View" then "Terminal" to open the Terminal window. Select the blue monitor icon (open a new terminal) to open the Launch Terminal window. Choose "Serial Terminal" and select the COMx port (Windows) or serial number (Mac) from earlier.
<br>
<br>
•	Build the code, select debug, and run the code. Be sure to have the terminal open to view the UART reports. The set point can be changed with the left and right side buttons (decrease or increase, respectively), and ambient temperature readings can be manipulated by placing a finger over the temperature sensor, a small silver square at the bottom left side of the board next to the Wi-Fi icon.





