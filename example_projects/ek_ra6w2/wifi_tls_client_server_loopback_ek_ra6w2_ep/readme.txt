/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This example demonstrates secure communication between a TLS client and server. The server initializes first, followed by the client sending a "Hello" message. The server then echoes the message back to the client.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
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
	J106 ={2,3} [Voltage selector]
	J105 ={2,3} [Voltage selector]
	J107 ={2,3} [Voltage selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO
	
6. Verifying Operation:
 1.Import the example project.
 2.Connect the RA6W2 board to your host PC using a USB Type-C cable.
 3.Open Segger RTT Viewer for logging.
 4.Build and flash the example project to the RA6W2 board.
 5.After flashing, reset the board.
 6.The TLS server will start – you can see this information in the console as:
   [INFO] TLS server running
   The client will then connect automatically – this will appear in the console as:
   [INFO] Connected to server
 7.Verify that the client receives messages echoed back from the server – you can see this in the console as:
   [INFO] TLS Server: Received data from Client: Hello from TLS client1!
 8.After that, the connection will close – this will appear in the console as:
   [INFO] All messages sent. Closing connection.
 9.If the connection fails, an error message will be displayed.


Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2000890c
   
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.

