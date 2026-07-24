/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This example demonstrates how https client request to server and server response corresponding to it.

2. Software Requirements:
Terminal Console Application: Tera Term, RTT viewer or a similar application


3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging. 


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
	
6. Configuration:
All HTTPS configurations are configured in `config.h`:

7. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use segger/RTTViewer for login.
 4. Debug or flash the EP project to the RA6W1 board.
 5. After flashing, press reset and observe the logs.

8. Example Log: 

00> [INFO] 
00> Https Server Running...
00> [INFO] Event: HTTPS_EVENT_SERVER_RECVED
00> [INFO] Client Request Header:
00>  GET /index.html HTTP/1.1
00> User-Agent: lwIP/2.1.3 (http://savannah.nongnu.org/projects/lwip)
00> Accept: */*
00> Host: localhost
00> Connection: Close
00> 
00> 
00> [INFO] Event: HTTPS_EVENT_CLIENT_GET_DONE
00> [INFO] Event: HTTPS_EVENT_CLIENT_RECVED
00> [INFO] received: 1534 byte, err: 0
00> [INFO] Event: HTTPS_EVENT_CLIENT_RECVED
00> [INFO] received: 217 byte, err: 0
00> [INFO] Event: HTTPS_EVENT_CLIENT_RESULT
00> [INFO] httpc_result: 0, received: 1751 byte, err: 0
00> [INFO] Https Server closing...

Note:
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200088b4
   
   
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
