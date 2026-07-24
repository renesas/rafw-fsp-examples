/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This project shows the AT Matter Door lock application developed on RA6W1.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application
Google Home Application: For using the Application Interface

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
Jumper Wires
Google Nest Hub.(Can use Samsung SmartThing)
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

	1. Import the Matter example project into your IDE or build environment.
	   * If using a Renesas VID/PID, generate and flash the Matter attestation certificates as described in "Matter Device Attestation", then configure the VID/PID by 
             following "e² studio Test VID/PID Modification and Certification Regeneration" in the RA6W2 Matter Certification Document.
           * If using the default test VID/PID, no additional configuration is required. Proceed to the next step after importing the project.
	2. Generate the necessary build files and build the firmware image.
	3. Flash the firmware image onto the RA6W1 board.
	4. Download and install the Google Home app (Play Store / App Store).
	5. Press the Reset button to start execution.
	6. Use the "setup" command in the console to connect the device to your Wi-Fi network.
	7. Enable Device Power Management (DPM) and apply the default configuration.
   	   Note: DPM can be disabled. If disabled, skip step 9.
	8. Reboot the device and wait for the Wi-Fi connection to establish.
	9. Enter the following AT commands in the RA6W1 console (DPM-enabled only):

           AT+MCONFIG=DISC,3840
           AT+MCONFIG=VID,65521
  	   AT+MCONFIG=PID,32774
   	   AT+MCONFIG=HWVER,1
           AT+MCONFIG=SPKPCNT,1000
     	   AT+MCONFIG=SPKPSALT,U1BBS0UyUCBLZXkgU2FsdA==
      AT+MCONFIG=SPKPVF,uWFwqugDNGiEck/po7KHwwMwwqZgN10XuyBajPGuyzUEV/iree4lOrao5GuwnlQ65CJzbeUB49s31EH+NEkg0JVI5MGCQGMMT/SRPFNRODm3wH/MBiehuFc6FJ/NH6Rmzw==
           AT+MCONFIG=PINCODE,20202021
   	   AT+MCONFIG=DEVTYPE,10

	10. Open the Google Home app → tap "+ Add" → "Set up device" → "New device" → "Next".
	11. Tap "Scan QR code" or "Set up without QR code". Enter the setup code displayed in the console.
	12. Wait until the console prints "App Task started RM_PMGR_W_dpm_is_enabled".
    	    The app will then transfer Matter credentials to the device.
	13. Select "Set up anyway" only after the following logs appear in the console:

    	    DIS: Responding with 0FCEC13AE30DCCFA._matterc._udp.local
            DIS: CHIP minimal mDNS configured as 'Commissionable node device'; instance name: 0FCEC13AE30DCCFA.
            DIS: mDNS service published: _matterc._udp
            IM: No subscriptions to resume
		===========================================2...
		============================================3...
		============================================4...
		============================================5...
		Starting App Task
		=SWU: Stopping the watchdog timer"
		
	14. Tap "Continue" → select device type "Door Lock" → assign a name → "Next" → "Setup" → "Done".
        15. The Door Lock interface is now available under: Home → Devices → [Door Lock Name]

            DPM enabled:  Lock/unlock via the mobile app.
            DPM disabled: Lock/unlock via AT commands:
                  AT+MATTR=1,257,0,2,32,1  (unlock)
                  AT+MATTR=1,257,0,1,32,1  (lock)
	  
Note: The console logs reflect the following Matter commissioning events:
   - Provisioning & commissioning: Device is securely added to the hub using QR/setup credentials.
   - Session establishment: A CASE session creates an encrypted channel between device and hub.
   - Certificate exchange: DAC is verified; the hub issues an NOC to join the fabric.
   - Endpoint/cluster discovery: Device reports its endpoints (logical units) and supported clusters (e.g., On/Off, Level Control).
   - Control operations: Hub sends commands to device clusters via the secure session.


The Link for Google home application - https://play.google.com/store/apps/details?id=com.google.android.apps.chromecast.app&pcampaignid=web_share
Link for Samsung Smarthing Application - https://play.google.com/store/apps/details?id=com.samsung.android.oneconnect&pcampaignid=web_share

