/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project showcases how we can create an application in RA6W1 using the AWS IoT platform.
It implements a cloud-connected door lock system.
The RA6W1 device connects to a Wi-Fi network and establishes a secure MQTT connection to AWS IoT Core.
The actual door lock hardware is expected to be controlled by RA6W1.
The mobile application communicates with AWS IoT Core through the internet to remotely control the door lock via RA6W1.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.
Smart phone with Renesas Wi-Fi Provisioning app installed.

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.
Connect the pin P0_10 with any of the in-build buttons on the mother board like BTN1 using a jumper wire.

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
 2.  Generate, build the Example project.
 3.  Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
 4.  Debug or flash the EP project to the RA6W1 board.
 5.  After flashing, press reset
 6.  Use any terminal application for logging.
 7.  Press BTN1 to enter in to wifi provisioning mode.
 8.  Modify the default AWS Thingname if needed, using the cli command, nvram setenv appcfg thingname <THINGNAME>.
 9.  Do the wifi provisioning using the Renesas Wifi Provisioning application from google play store or ios appstore.
 10.  After provisioning the deivce will reboot itself and will try for connection with the provisioned access point. 
 11. After successfull connection on wifi, the device will connect to the AWS IoT platform.
 12. Select the AWS IoT tab from the hamburger menu icon in the top left side of the homescreen.
 13. Now the AWS IoT window will show the interface to perform the lock and unlock operations.
 14. It will also show the status of connection with the device and AWS cloud. Also the subscribe and publish topic
     and messages.
 15. Press on the lock image to toggle between lock and unlock after the connection with device is successfull. 
 16. When the lock/unlock is done from the mobile application, the terminal logs will show the door lock state changes
     accordingly.

Note:

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
3. Start debugging and the image will be flashed automatically to the RA6W1.
