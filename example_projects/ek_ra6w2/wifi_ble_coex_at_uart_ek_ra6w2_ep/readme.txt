/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Application Project and detailed instructions
**********************************************************************************************************************/


1. Project Overview:
This application demonstrates BLE coexistence with Wi-Fi in 2.4GHz band.
The device scans for nearby Wi-Fi networks, shares results with a BLE client, and receives credentials to connect to the chosen Wi-Fi network.

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging. 

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the RA6M4.

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
 2. Generate, build the application project.
 3. Use UART and AT-UART for logging.
 4. Download Renesas Smartbond application from Android/iOS store.
 5. Flash the application project to the RA6W2 board.
 6. After flashing, press reset.
 7. Connect/Disconenct BLE with Smartbond application while TCP traffic is running.
 8. Disabled BLE and put device in DPM mode.
 9. Verified Basic AT command.

