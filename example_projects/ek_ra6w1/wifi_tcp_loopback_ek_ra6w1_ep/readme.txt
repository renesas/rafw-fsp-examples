/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project showcases how we can use lwIP networking API for TCP communication.
A TCP server listens on port 5000 and creates a handler task for each connected client.
Two clients connect to the server, send hello messages, and receive the same messages back.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.

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

6. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use Segger/RTTViewer for logging.
 4. Flash the EP project to the RA6W1 board.
 5. After flashing, press reset
 6. Verify client received data correctly back from server using segger prints
 
 
 Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200042d0

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
