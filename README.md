# CS350-Embedded-Systems
<b> About the Project </b>

The provided artifacts are a compressed folder of all of the project files, a single C file of the heart of the code for easy viewing, and the task scheduler documentation.

My hypothetical employer, SysTec, would like enter the smart thermostat market. The goal is to develop a thermostat that sends data to the server over Wi-Fi, but first a prototype needs to be created using the TI SimpleLink CC3220S LaunchPad. The TMP006 temperature sensor will be used to read the room temperature (via I2C), the LED will indicate the output to the thermostat where LED on means the heater is on (via GPIO), two buttons will be used to increase or decrease the temperature set point (via GPIO interrupt), and the UART will be used to simulate sending data to the server.

<b> Project Functionality </b>

The code checks the status of the buttons every 200 ms. If a button has been pressed, the temperature set point is either increased or decreased by one degree, depending on which button was pressed. The room's ambient temperature is checked every 500 ms, and if the ambient temperature is below the temperature set point, the LED is turned on to indicate the heater being turned on. Once the ambient temperature is equal to or above the set point, the LED turns off (heater is off). Every second, the data is reported to the UART in the form of <AA, BB, C, DDDD> where AA is the ambient temperature, BB is the temperature set point, C is the heater status (0 for off, 1 for on), and DDDD is the number of seconds since system reset.

Starting state of dashboard: 


![image](https://user-images.githubusercontent.com/95947696/209023782-7d9a3018-9aef-429e-bee5-7689d78f4213.png)

Water Rescue Filter:


![image](https://user-images.githubusercontent.com/95947696/209025649-c283a644-00fa-47d2-9ab7-e7122a7df63c.png)

<br>
<br>

<b> Tools Used </b>

MongoDB
<br>
&emsp;•	MongoDB was used because it is a good way to store and retrieve data and is compatible with many programming languages, including Python which was used for this project. It stores data in BSON format meaning that applications can receive the data in JSON format, which is helpful when using MongoDB with Python because using Python dictionaries make both simple and advanced querying a straightforward task. MongoDB has a driver for Python, PyMongo, which allows for easy connection to the database.
<br>
<br>
Dash Framework
<br>
&emsp;•	Dash was used to build the dashboard and is great for building a customized interface, especially with Python. It provides methods to build many different interactive options, such as the radio items, datatable, geolocation map, and pie chart that were used in this project. Combined with Dash callbacks, a fully interactive and functional dashboard interface can be built. Since it is compatible with Python and Python is compatible with MongoDB, it is a valuable tool for integrating the functionalities of all three together.
<br>
<br>
Jupyter Notebook
<br>
&emsp;•	Jupyter Notebook was used to write the CRUD Python module as a .py file, and to test the module in a .ipynb Python script. The dashboard was also created in a .ipynb Python script.

<br>
<br>

<b> Reproducing the Project </b>

•	Import the AAC database through the terminal, contained in aac_shelter_outcomes.csv.
<br>
•	Enable user authentication for the database and connect to the MongoDB server under that user.
<br>
•	Use a tool like Jupyter Notebook to open the .ipynb dashboard file and the .py CRUD Python module.
<br>
•	In the dashboard code, edit the instantiation of the CRUD Python module to the correct username, password, and port number.
<br>
•	Run the code to view the dashboard. As mentioned before the pie chart will be crowded, so double-click on one breed to isolate and single-click to add more breeds to the chart.
<br>
•	Use the radio buttons next to each row to see that animal’s location on the map. Hover over the marker to see their breed and click the marker to see their name.
<br>
•	Use the radio buttons above the chart to filter for dogs that are best suited to the job specified by the button. Again, breeds on the pie chart may be selected/deselected for a customized view.
<br>
•	If more specific filters are desired, text can be typed in the empty box above any column to perform a specific query.
<br>
•	Click the Grazioso Salvare logo to be redirected to the client homepage.





