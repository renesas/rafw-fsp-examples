/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This example project demonstrates accessing a webpage through an HTTP server, tested against a Linux server.

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging or type C USB cable.

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
	
6. Verifying Operation:
 
 1.Import the example project.
 2.Perform the Following Steps on the Server Side(Linux)
    2.1 Ensure Server Name Indication (SNI) is enabled for HTTPS(secure server).
    2.2 Copy the CA certificate from the certificate folder into: /usr/local/share/ca-certificates (Linux).
        Server use the certificate signed by this CA certificate.
    2.3 Update certificates on your system: sudo update-ca-certificates (This will add one certificate).
 3.Edit http_svr.h and update the Wi-Fi SSID and Password and build the Example project.
 4.Connect the RA6W2 motherboard debug port to your host PC using a USB Type-C cable.
 5.Debug or flash the EP project to the RA6W2 board.
 6.Reboot the board.
 7.Connect the device to Wi-Fi.
 8.Edit /etc/hosts and add 192.168.50.156 mydevice.local (replace with your assigned IP)
 9.Open browser and browse https://mydevice.local.
 10.Verify that static html page is loaded(A browser security warning will appear because the certificate is self-signed — this is expected for the demo).
 
 
Behaviour:

1. Client will receive server hosting webpage[default page is index.html which has brief information about LWIP

Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x000000002000c480
   
   
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
