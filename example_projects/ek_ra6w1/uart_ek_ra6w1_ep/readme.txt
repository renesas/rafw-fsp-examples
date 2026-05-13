/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the typical use of the UART HAL module APIs.
The project initializes the UART with Baud rate of 115200 bps.
Using a Terminal Program (like Tera Term) user can provide a value & press enter key to set the status of the on-board LED.
The guide of input string values are displayed on the JLinkRTTViewer.
Any failure will also be displayed using JLinkRTTViewer.
To see user input values on Serial terminal, enable local echo option.

2. Hardware and Software Requirements:

Three jumper wires are required to connect UART RX/TX lines and LED.
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term, RTT viewer or a similar application

3. Hardware settings for the project:

Wiring details:

RA6W1-EK:
1) Connect UART1_RXD (Jumper J201 Pin 1) ----> P0_00 (Jumper J201 Pin 2)
2) Connect UART1_TXD (Jumper J201 Pin 3) ----> P0_01 (Jumper J201 Pin 4)
3) Connect LED       (Jumper J611 Pin 1 or any LED Pin) ----> P0_10 (Jumper J201 Pin 22)  

4. Verifying Operation:

1) Import the Example project into e2 studio.
2) Build the project.
3) Start a Debug session. This will automatically flash the image to the board.
4) Open a Teraterm UART console on the host PC to test and set correct COM port and baud rate to 115200.
5) After the debug session starts, click Run to execute the program (no need to press the board's reset button).
6) Open a SEGGER RTT Viewer on the host PC to view the console output.
7) Check console log " Type LED status (on or off)
	Input "on" -> Press Enter key and check the LED, it should be turned on.
	Input "off" -> Press Enter key and check the LED, it should be turned off.

Note:
1) Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
   RTT Block address for hex file committed in repository are as follows:
   a. e2studio: 0x20000B7C
 
2) If an EP is modified, compiled, and downloaded please find the block address (for the variable in RAM called _SEGGER_RTT) 
   in .map file generated in the build configuration folder (Debug/Release).

Flashing Procedure:
1. Open e©÷studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.