/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Application Project and detailed instructions
**********************************************************************************************************************/


1. Project Overview:
This application demonstrates Wi-Fi provisioning over Bluetooth Low Energy (BLE). 
The device scans for nearby Wi-Fi networks, shares results with a BLE client, and receives credentials to connect to the chosen Wi-Fi network.

2. Software Requirements:
Renesas RAFW(FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging. 

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
 1. Import the application project.
 2. Download jsmn.h file from https://raw.githubusercontent.com/zserge/jsmn/refs/heads/master/jsmn.h and copy it to the "e2studio/thirdparty" folder in the application if not present.
 3. Generate, build the application project.
 4. Use Segger/RTTViewer and UART for logging.
 5. Download Renesas WiFi Provisioning application from Android/iOS store using https://play.google.com/store/apps/details?id=com.renesas.wifi&hl=en.
 6. Flash the application project to the RA6W2 board.
 7. After flashing, press reset.
 8. Open the Wi-Fi Provisioning App on your phone and select the PROV-RA6W2 device from the BLE scan list.
 9. Connect to the device; the app will trigger a Wi-Fi scan and display the available SSIDs on your phone.
 10.Choose an SSID from the list, enter the Wi-Fi password, and send it to the device through the app.
 11.The device validates the credentials, pings the test server (e.g., 8.8.8.8), and logs the connection result in RTTViewer.
 12.Finally, the device reboots and connects to the selected Wi-Fi network automatically.
 
 
 Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200042d0

