/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This project shows how to provision the device to available Wi-Fi networks and display the connection status on the serial console.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term,RTT viewer or a similar application

3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project
   1.Jumper Configurations
	UART(RX) = P0_00
	UART(TX) = P0_01
	J102 ={2-3} [Current measurement]
	J105 ={2,3} [Volatge selector]
	J106 ={2,3} [Volatge selector]
	J107 ={2,3} [Volatge selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO
    2.Additional Configurations
      For enabling provisioning mode, please use the below config:
 
      1. connect BTN1 and P0_11 [Factory Reset]
      2. Press BTN1 and release to start Provisioning Mode.

6. Verifying Operation:

1.Import the example project.
2.Generate, Build the Example project.
3.Use Segger/RTTViewer for logging.
4.Flash the EP project to the RA6W2 board.
5.Download Renesas WiFi Provisioning application from Android/iOS store using https://play.google.com/store/apps/details?id=com.renesas.wifi&hl=en.
6.After flashing, Reset the board to start execution.
7.Open a SEGGER RTT Viewer on the host PC to view the console output.
8.Press BTN1 [factory reset]
9.Open the Wi-Fi Provisioning App on your phone and select Start provisioning over WiFi : Chipsets RRQ61000, DA16200.
10. In the "Select SoftAP configuration" page, Select "Reneses IoT". (Make sure SSID pattern is 'Renesas_IoT_WiFi' and password is '1234567890')
11.Connect to the device; the app will trigger a Wi-Fi scan and display the available SSIDs on your phone.
12.Choose an SSID from the list, enter the Wi-Fi password, and send it to the device through the app.
13.Finally, the device reboots and connects to the selected Wi-Fi network automatically.

Remarks:
This RA6W2 project supports provisioning over Wi-Fi. When using the Renesas Mobile application with RA6W2, the BLE provisioning example can be used.

Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200658f4

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
