/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This project shows the AT Matter Door lock application developed on RA6W1.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application
Google Home Application: For using the Appliaction Interface

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

   2. Additional Wire configuration
	5th & 6th pin of j201 into  1 & 2 of J203
 	J3 pins connection 1,3,5,7
 

6. Configure google hub on Home application, refer-"https://youtu.be/_qwh14SaXpQ?si=d6-_FY-UfRaUazsW" and "https://youtu.be/c7X6qXN5Lh4?si=gymJtK84OdWDrIu2"
      
7. Verifying Operation:

	1. Import Example Project
	2. Import the Matter example project into your IDE or build environment.
	3. Generate & Build Project
	4. Generate the necessary build files and build the firmware image.
	5. Flash the Firmware
	6. Flash the image onto RA6W1 board.
	7. Install Google Home App
	8. Download and install the Google Home app from the Play Store or App Store.
	   (App Tab: Home → Devices)
	9. Reset the Board
	10. After flashing, press the Reset button to start execution.
	11. Connect Device to Wi-Fi
	12. Use the "setup" command in the console to connect the device to your Wi-Fi network.
	13. Enable DPM and apply the default configurations. You can use the application with DPM turned off. For applications where DPM is disabled, keep DPM turned off and skip step 14.
	14. Enable Device Power Management (DPM) and apply the default configuration.
	15. Reboot Device
	16. Reboot and wait for the Wi-Fi connection to establish successfully.
 	17. Add the below AT commands in console of RA6W1
 	 	
 		AT+MCONFIG=DISC,3840
		AT+MCONFIG=VID,5474
		AT+MCONFIG=PID,5
		AT+MCONFIG=HWVER,1
		AT+MCONFIG=SPKPCNT,1000
		AT+MCONFIG=SPKPSALT,U1BBS0UyUCBLZXkgU2FsdA==
		AT+MCONFIG=SPKPVF,uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw==
		AT+MCONFIG=PINCODE,20202021
		AT+MCONFIG=DEVTYPE,10
        
	18. Reboot and Add Device in Google Home App
	19. Open Google Home App
	20. Tap the “+ Add” button (top-left corner)
	21. Choose “Set up device” → “New device” Tap “Next”
	22. Pair Using QR Code or Setup Code 
	23. Tap “Scan QR code” OR “Set up without QR code”
	24. Enter the setup code seen on console
	25. Tap “I’m ready to scan” or “Continue without scanning”
	26. Transfer Matter Credentials Prints are seen in the console
		Once the console shows:
		"App Task started RM_PMGR_W_dpm_is_enabled"
		 → The app transfers Matter credentials to the device.
		 
                Confirm by selecting ‘Set up anyway’ in the Home application only when the following logs appear on the screen
		
		"DIS: Responding with 0FCEC13AE30DCCFA._matterc._udp.local
		DIS: CHIP minimal mDNS configured as 'Commissionable node device'; instance name : 0FCEC13AE30DCCFA.
		DIS: mDNS service published: _matterc._udp
		IM: No subscriptions to resume

		===========================================2...
		============================================3...
		============================================4...
		============================================5...
		Starting App Task
		=SWU: Stopping the watchdog timer"
		
	26. Tap “Continue” when prompted.
	27. Confirm by selecting “Set up anyway” when prompted.
	28. Select the device type: Door Lock and give the name
	29. Tap “Next” → “Setup” and Finally CLick on Done 
	30. Device Control Interface
		The Door Lock interface now appears under:
		Home → Devices → [Door Lock Name]
			For DPM-enabled applications:
			Lock and unlock actions can be performed through the mobile application.
 
			For DPM-disabled applications: 
			Lock and unlock operations can be performed using AT commands as shown below:
					AT+MATTR=1,257,0,2,32,1-unlock
					AT+MATTR=1,257,0,1,32,1-lock	  
Note:
The console displays messages related to:

	Provisioning and commissioning - Device joins the home network and is securely added to the controller (hub) using setup credentials (QR/Code).
	Transport layer session creation between Device and hub - A secure channel (CASE session) is created between device and hub for encrypted communication.
	Transfer of certificates - Device shares its manufacturer certificate (DAC); hub verifies it and gives an operational certificate (NOC) to join the home fabric.
	Information about clusters and endpoints - Endpoints = logical parts of the device; Clusters = features (like On/Off, Level Control) that define its capabilities
	Control operations - Hub sends commands (On, Off, etc.) to device clusters via the secure session to perform actions.

The Link for Google home application - https://play.google.com/store/apps/details?id=com.google.android.apps.chromecast.app&pcampaignid=web_share
Link for Samsung Smarthing Application - https://play.google.com/store/apps/details?id=com.samsung.android.oneconnect&pcampaignid=web_share

