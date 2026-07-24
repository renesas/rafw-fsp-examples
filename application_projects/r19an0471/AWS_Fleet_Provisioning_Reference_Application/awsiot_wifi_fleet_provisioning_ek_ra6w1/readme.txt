/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This project shows how to implement AWS Fleet Provisioning using the Fleet Provisioning by Claim method. 

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application  

3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Micro USB cable for programming and debugging or type C USB cable.

4. Hardware Connections:
Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project

Jumper Configurations
	UART(RX) = P0_00
	UART(TX) = P0_01
	J102 ={2-3} [Current measurement]
	J106 ={2,3} [Volatge selector]
	J105 ={2,3} [Volatge selector]
	J107 ={2,3} [Volatge selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO
	
5. Verification:
 1.  Import the Fleet Provisioning Example project into e2 studio.
 2.  Generate, build the Example project.
 3.  Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
 4.  Debug or flash the EP project to the RA6W1 board.
 5.  After flashing, press reset
 6.  Use any terminal application for logging.
 7.  Now the wifi setup menu will pop up on inital bootup. Setup the wifi details for internet connection.
 8.  Write the RootCA, Claim Certificate and Claim Private Key into the nvram using cli commands:
     net cert write ca6: RootCA
     net cert write initcert6: Claim Certificate
     net cert write initkey6: Claim Private Key
 9.  Write the Device ID and Fleet Provisioning Template Name into the NVRAM using cli commands:
     nvram setenv appcfg fp_dev_id <DeviceID>
     nvram setenv appcfg fp_tmpl_name <TemplateName>
 10.  Open the mqtt test client in AWS IoT console in web browser and subscribe to the topic:
     $aws/things/<Thingname>/shadow/update/delta.
 11. Now reboot the device using cli command, "reboot" or using reset button. 
 12. After reboot RA6W1 will connect to the configured AP.
 13. After connection to AP is successfull, RA6W1 will check for the AWS IoT provisioned credentials. If it is available,
     then RA6W1 will directly go for AWS IoT Core connection using the provisioned credentials. If not, then RA6W1 will
     initiate the Fleet Provisioning by Claim process. Since this is inital bootup, RA6W1 will go for Fleet Provisioning.
 14. After Fleet provisioing is completed, the unique certificate got from AWS Core and unique key generated locally will be 
     saved in the nvram and RA6W1 will disconnect itself from AWS IoT Core and reconnect using the provisioned 
     credentials. 
 15. After the Fleet Provisioning is completed it will publish a message to the topic: $aws/things/<Thingname>/shadow/update/delta.
 16. Now since the fleet provisioning is now completed, if RA6W1 is rebooted, it will directly connects to AWS IoT Core using
     the provisioned credentials and publish to the same topic.

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
