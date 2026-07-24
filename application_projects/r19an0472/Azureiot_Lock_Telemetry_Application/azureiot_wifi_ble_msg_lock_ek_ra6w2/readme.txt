/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project showcases how we can create an application in RA6W2 using the Azure IoT platform in DPM mode.
It implements a cloud-connected door lock and telemetry message publishing system.
The RA6W2 device connects to a Wi-Fi network and establishes a secure MQTT connection to Azure IoT Hub.
The actual door lock hardware is expected to be controlled by RA6W2.
The mobile application or the Azure IoT explorer communicates with Azure IoT Hub through the internet to remotely control the door lock via RA6W2.
This example also demonstrates Wi-Fi provisioning over Bluetooth Low Energy (BLE) using Renesas Wi-Fi Provisioning app.
The device scans for nearby Wi-Fi networks, shares results with a BLE client, and receives credentials to connect to
the chosen Wi-Fi network.

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.
Smart phone with Renesas Wi-Fi Provisioning app installed.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project
1.Jumper Configurations
	UART(RX) = P0_00
	UART(TX) = P0_01
	J102 ={2-3} [Current measurement]
	J106 ={2,3} [Volatge selector]
	J105 ={2,3} [Volatge selector]
	J107 ={2,3} [Volatge selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO

6. Verification:
 1.  Import the example project.
 2.  Enter device ID for APP_USER_MY_THING_NAME, device primary key for APP_USER_MY_DEV_PRIMARY_KEY, host name for APP_USER_MY_HOST_NAME and iothub connection string for APP_USER_MY_IOTHUB_CONN_STRING in app_thing_manager.h
 3.  Download jsmn.h file from https://raw.githubusercontent.com/zserge/jsmn/refs/heads/master/jsmn.h and copy it to the "e2studio/thirdparty" folder in the application if it is not present.
 4.  Generate and build the Example project.
 5.  Connect the RA6W2 MCU motherboard debug port to the host PC via a type C USB cable.
 6.  Debug or flash the EP project to the RA6W2 board.
 7.  After flashing, press reset.
 8.  Use any terminal application for logging.
 9.  Modify the default Azure device ID if needed, eg:- nvram setenv appcfg thingname <THINGNAME>.
 10.  Do the Wi-Fi provisioning using the Renesas Wi-Fi Provisioning application which is available in Google play store or iOS appstore.
 11. After provisioning, the device will reboot itself and will try for connection with the provisioned access point.
 12. After successful connection on Wi-Fi, the device will connect to the Azure IoT Hub.
 13. Select the Azure IoT tab from the hamburger menu icon in the top left side of the homescreen.
 14. Now the Azure IoT window will show the interface to perform the lock and unlock operations.
 15. It will also show the status of connection with the device and the Azure cloud.
 16. Press on the lock image to toggle between lock and unlock states, after the connection with device is successful.
 17. When the lock/unlock is done from the mobile application, the terminal logs will show the door lock state changes
     accordingly.
 18. The published values if any, can be seen in the Device Twin of the Azure IotHub.
 19. The Azure IoT Explorer can also be used to test the application instead of the Mobile Application.
 20. The door can be Opened or Closed by giving the Method name as "AppControl" and Payload as "doorOpen" or "doorClose"
 21. The published telemetry messaged from the device can be viewed in the Telemetry section of the web application.
 

For Serial terminal application:
1) User need to enable CR to view console logs properly.
2) The configuration parameters of the serial port on the terminal application are as follows:
	Baud rate:    115200 bps
	Data length:  8-bits
	Parity:       none
	Stop bit:     1-bit
	Flow control: none

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
