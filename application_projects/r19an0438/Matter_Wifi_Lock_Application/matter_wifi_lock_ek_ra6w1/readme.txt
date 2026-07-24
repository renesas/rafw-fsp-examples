/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This project shows the Matter Door lock application developed on RA6W1.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
Jumper Wires
Google Nest Hub.(Can use Samsung Smarthings)
IPV6 supported Wifi Access Point 

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

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

6. Configure google hub on Home application, refer-"https://youtu.be/_qwh14SaXpQ?si=d6-_FY-UfRaUazsW" and "https://youtu.be/c7X6qXN5Lh4?si=gymJtK84OdWDrIu2"
      
7. Verifying Operation:

	1. Import the Matter example project into your IDE or build environment.
	   * If using a Renesas VID/PID, generate and flash the Matter attestation certificates as described in "Matter Device Attestation", then configure the VID/PID by 
	     following "e² studio Test VID/PID Modification and Certification Regeneration" in the RA6W2 Matter Certification Document.
	   * If using the default test VID/PID, no additional configuration is required. Proceed to the next step after importing the project.      
	2. Generate the necessary build files and build the firmware. Flash the compiled firmware image onto the RA6W1 board.
	3. Download and install the Google Home app from the Play Store or App Store (Home → Devices).
	4. After flashing, press the Reset button to start execution.
	5. Use the setup command in the console to connect the device to your Wi-Fi network.
	6. Enable Device Power Management (DPM) and apply the default configuration.
	7. Reboot the device and wait for the Wi-Fi connection to establish successfully.
	8. Add Device in Google Home App:
   		a. Open the Google Home app and navigate to Home → Devices.
   		b. Tap the "+ Add" button (top-left corner).
   		c. Choose "Set up device" → "New device", then tap "Next".
   		d. Tap "Scan QR code" or "Set up without QR code".
   		e. Enter the setup code displayed on screen, then tap "I'm ready to scan" or "Continue without scanning".
   		f. Once the console displays "App Task started" and "RM_PMGR_W_dpm_is_enabled", the app transfers Matter credentials to the device. Select "Set up anyway" when prompted.
   		g. Tap "Continue" when prompted.
   		h. Select the device type as "Door Lock" and assign a name.
   		i. Tap "Next" → "Setup", then tap "Done".
	9. The Door Lock interface now appears under Home → Devices → [Door Lock Name]. Tap the Lock / Unlock button in the Google Home app to control the door. The device reflects these actions in real-time via logs on the device console.	
	
Note: The console logs reflect the following Matter commissioning events:
   - Provisioning & commissioning: Device is securely added to the hub using QR/setup credentials.
   - Session establishment: A CASE session creates an encrypted channel between device and hub.
   - Certificate exchange: DAC is verified; the hub issues an NOC to join the fabric.
   - Endpoint/cluster discovery: Device reports its endpoints (logical units) and supported clusters (e.g., On/Off, Level Control).
   - Control operations: Hub sends commands to device clusters via the secure session.


The Link for Google home application - https://play.google.com/store/apps/details?id=com.google.android.apps.chromecast.app&pcampaignid=web_share
Link for Samsung Smarthing Application - https://play.google.com/store/apps/details?id=com.samsung.android.oneconnect&pcampaignid=web_share

Flashing Procedure:
	1. Open e²studio and connect your JTAG debugger.
	2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
	3. Start debugging and the image will be flashed automatically to the RA6W1.


