/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the typical use of the UART HAL module APIs.
The guide of input string values are displayed on the JLinkRTTViewer.
Any failure will also be displayed using JLinkRTTViewer.

2. Hardware and Software Requirements:

LED on board connected to P0_10
UART loopback is enabled, no need for any connections.
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term, RTT viewer or a similar application

3. Hardware settings for the project:

Wiring details:

RA6W2-EK:
1) Connect LED       (Jumper J611 Pin 1) ----> P0_10 (Jumper J201 Pin 22)  

4. Verifying Operation:

1) Import the Example project into e2 studio.
2) Build the project.
3) Start a Debug session. This will automatically flash the image to the board.
4) After the debug session starts, click Run to execute the program (no need to press the board's reset button).
5) Open a SEGGER RTT Viewer on the host PC to view the console output.
6) Check the console log "CRC Operation is successful.". LED will be blinked for a few seconds.

Note:
1) Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
   RTT Block address for hex file committed in repository are as follows:
   a. e2studio: 0x20000b7c
 
2) If an EP is modified, compiled, and downloaded please find the block address (for the variable in RAM called _SEGGER_RTT) 
   in .map file generated in the build configuration folder (Debug/Release).

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.