/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project showcases how we can create an application in RA6W2 using the AWS IoT platform.
It implements a cloud-connected door lock system.
The RA6W2 device connects to a Wi-Fi network and establishes a secure MQTT connection to AWS IoT Core.
The actual door lock hardware is expected to be controlled by RA6W2.
The mobile application communicates with AWS IoT Core through the internet to remotely control the door lock via RA6W2.
This example also demonstrates Wi-Fi provisioning over Bluetooth Low Energy (BLE).
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
 2.  Download jsmn.h file from https://raw.githubusercontent.com/zserge/jsmn/refs/heads/master/jsmn.h and copy it to the "e2studio/thirdparty" folder in the application if it is not present. If the sysytem has wget installed the file will be downloaded automatically during build process.
 3.  Generate, build the Example project.
 4.  Connect the RA6W2 MCU motherboard debug port to the host PC via a type C USB cable.
 5.  Debug or flash the EP project to the RA6W2 board.
 6.  After flashing, press reset
 7.  Use any terminal application for logging.
 8.  Modify the default AWS Thingname if needed, using the cli command, nvram setenv appcfg thingname <THINGNAME>.
 9.  Do the wifi provisioning using the Renesas Wifi Provisioning application from google play store or ios appstore.
 10.  Select "WiFi+BLE combo chip" tab for provisioning. Select the device name, "RRQ-DEVICE" from the BLE scan list.
 11. After provisioning the deivce will reboot itself and will try for connection with the provisioned access point. 
 12. After successfull connection on wifi, the device will connect to the AWS IoT platform.
 13. Select the AWS IoT tab from the hamburger menu icon in the top left side of the homescreen.
 14. Now the AWS IoT window will show the interface to perform the lock and unlock operations.
 15. It will also show the status of connection with the device and AWS cloud. Also the subscribe and publish topic
     and messages.
 16. Press on the lock image to toggle between lock and unlock after the connection with device is successfull. 
 17. After one lock/unlock action is completed, the AWS application is ready to go to sleep and ble svc_stop command is
     set by AWS application to BLE to stop BLE. Now the device will go to sleep. 
 18. Only after RA6W2 goes to sleep, do the next lock/unlock toggle action.
 19. When the lock/unlock is done from the mobile application, the terminal logs will show the door lock state changes accordingly.

Note:
Apply the door open or close button only after the device goes to sleep.

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
