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
 3. Use UART for logging.
 4. Download Renesas Smartbond application from Android/iOS store.
 5. Flash the application project to the RA6W2 board.
 6. After flashing, press reset.
 7. Connect/Disconenct BLE with Smartbond application while TCP traffic is running.
 8. Disabled BLE and put device in DPM mode.
 
7. Known issue:

Issue:
An increase in transition time is observed between DPM Wakeup and Sleep after the recent code refactoring. 

Root Cause:
The ble_init() api is executed at the beginning of app_main() regardless of the sleep state. As a result, the BLE initialization sequence introduces additional latency before the device can return to sleep mode.

Workaround:
To avoid this latency apply following changes.
Change 1:
Comment out the ble_init() call at the beginning of app_main()(around line 712).
Change 2:
Conditionally invoke ble_init() in app_main() only when sleep mode is disabled(around line 725) as shown below:
	if (is_sleep3_wakeup == false)
	{
		ret = ble_init();
		assert(BLE_SUCCESS == ret);

		ret = R_BLE_GTL_VS_SetSleepModeExt(R_BLE_GTL_SLEEP_MODE_EXTENDED, R_BLE_GTL_SLEEP_RAM_MODE_ON, 1 << DA1453x_GTL_CTS_PIN, 0);
		assert(BLE_SUCCESS == ret);
	}
Change 3:
Comment out the following else block(around line 730)
	else
	{
		ret = ble_uninit();
		assert(BLE_SUCCESS == ret);
	}