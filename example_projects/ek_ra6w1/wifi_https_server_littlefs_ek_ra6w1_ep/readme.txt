/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This example project demonstrates accessing a webpage through an HTTP server and upload files to filesystem through web gui.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging or type C USB cable.

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

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
	
6. Verifying Operation:
 
 1.Import the example project.
 2.Need to set the following symbols to 1 for use the feature if it is already enabled ingnore this step.
    SUPPORT_FSP_RM_FS_W,LWIP_HTTPD_CGI,LWIP_HTTPD_SSI,LWIP_HTTPD_SSI_BY_FILE_EXTENSION,LWIP_HTTPD_CUSTOM_FILES,LWIP_HTTPD_DYNAMIC_HEADERS
    2.1 Right click template
    2.2 Select C/C++ Project Settings
    2.3 In Tool Settings under GNU Arn Cross Compiler select preprocessor
    2.4 Add each symbols above mentioned by selecting add button.
 3.Perform the Following Steps on the Server Side(Linux)
    3.1 Ensure Server Name Indication (SNI) is enabled for HTTPS(secure server).
    3.2 Copy the CA certificate from the certificate folder into: /usr/local/share/ca-certificates (Linux).
        Server use the certificate signed by this CA certificate.
    3.3 Update certificates on your system: sudo update-ca-certificates (This will add one certificate).
 4.Generate the Example project.
 5.Edit http_svr.h and update the Wi-Fi SSID and Password and build the Example project.
 6.Flash the EP project to the RA6W1 board.
 7.Use Segger/RTTViewer for logging.
 8.Connect the RA6W1 motherboard debug port to your host PC using a USB Type-C cable.
 9.Debug or flash the EP project to the RA6W1 board.
 10.Reboot the board.
 11.Connect the device to Wi-Fi.
 12.Edit /etc/hosts and add 192.168.50.156 mydevice.local (replace 192.168.50.156 with your assigned IP)
 13.Open browser and browse https://mydevice.local/upload.html
 14.Verify that static html page is loaded(Welcome Select a file to upload and store into LittleFS: — this is expected for the demo).
 15.Click Browse.
 16.Add file to upload.
 17.Click Upload. The file will be stored in filesystem.
 18.Very using AT cmd AT+FSLST.
 
 
Behaviour:

1. Client will receive server hosting webpage and used to upload the files to filestystem.

Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x000000002000bc80

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
