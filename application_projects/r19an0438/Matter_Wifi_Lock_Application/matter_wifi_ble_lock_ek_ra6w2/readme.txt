/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This project shows the Matter Door lock application developed on RA6W2 and Provisioning is done through BLE.

2. Software Requirements:


Terminal Console Application: Tera Term or a similar application
Google Home Application: For using the Application Interface

3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.
Jumper Wires
Google Nest Hub.(Can use Samsung Smarthings)
IPV6 supported Wifi Access Point 

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

6. Configure google hub on Home application, refer-"https://youtu.be/_qwh14SaXpQ?si=d6-_FY-UfRaUazsW" and "https://youtu.be/c7X6qXN5Lh4?si=gymJtK84OdWDrIu2"
      
7. Verifying Operation:
	
	1. Import the Matter example project into your IDE or build environment.
	   * If using a Renesas VID/PID, generate and flash the Matter attestation certificates as described in "Matter Device Attestation", then configure the VID/PID by
	     following "e² studio Test VID/PID Modification and Certification Regeneration" in the RA6W2 Matter Certification Document.   
	   * If using the default test VID/PID, no additional configuration is required. Proceed to the next step after importing the project.
	2. Generate the build files and build the firmware image.
	3. Flash the firmware onto the RA6W2 board.
	4. Press the Reset button to start execution.
	5. Download and install the Google Home app (Play Store / App Store).
	6. When "Start BLE Advertising" appears on the console, open the app to begin pairing.
	7. Tap: + Add → Set up device → New device → Next.
	8. Tap "Scan QR code" or "Set up without QR code" and enter the setup code shown on the console.
	9. Once the console shows "App Task started RM_PMGR_W_dpm_is_enabled", the app transfers Matter credentials to the device.
	10. Select "Set up anyway" only after the following logs appear:

		DIS: mDNS service published: _matterc._udp
		IM: No subscriptions to resume
		============================================ 2...3...4...5...
		Starting App Task

	11. Tap "Continue", then select the device type as Door Lock, assign a name, and tap Next → Setup → Done.
	12. The Door Lock interface appears under Home → Devices → [Door Lock Name]. Use the Lock/Unlock button to control the device; actions are reflected in real-time on the console.


Note: The console logs indicate:
- Provisioning/Commissioning — Device securely joins the home network using setup credentials.
- CASE Session — An encrypted channel is established between the device and hub.
- Certificate Transfer — Device DAC is verified; hub issues an operational certificate (NOC).
- Endpoints/Clusters — Logical device parts and their features (e.g., On/Off, Level Control).
- Control Operations — Hub sends commands to device clusters via the secure session.
	

The Link for Google home application - https://play.google.com/store/apps/details?id=com.google.android.apps.chromecast.app&pcampaignid=web_share
Link for Samsung SmartThing Application - https://play.google.com/store/apps/details?id=com.samsung.android.oneconnect&pcampaignid=web_share

Flashing Procedure:
 
	1. Open e²studio and connect your JTAG debugger.
	2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
	3. Start debugging and the image will be flashed automatically to the RA6W2.

