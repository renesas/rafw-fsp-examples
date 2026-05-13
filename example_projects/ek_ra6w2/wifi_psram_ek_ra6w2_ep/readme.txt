/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the typical use of the PSRAM QSPI APIs.
The project initializes the PSRAM QSPI and shows how to clear,read/write memory and string read/write test.
The SEGGER (like Tera Term) can be used to get SEGGER log.
The guide of input string values are displayed on the JLinkRTTViewer.
Any failure will also be displayed using JLinkRTTViewer.

2. Hardware and Software Requirements:

Clock settings change:
In Configuration.xml stack module g_qspi_ram0 QSPI, go to properties change the Bus timings QSPI_CLK Divisor  to numeric value '2'. It is done to match with SYSCLOCK change when WiFi starts.
QSPI interface is already connected to PSRAM inside the board in the EVK.
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: RTT viewer or a similar application

3. Hardware settings for the project:

Wiring details:

RA6W2-EK:
1) QSPI pins are connected already

4. Verifying Operation:

1) Import the Example project into e2 studio.
2) Build the project.
3) Flash the image into the RA6W2 board via the debugger.
4) Open a SEGGER RTT Viewer on the host PC to view the console output.
5) Reset the board to start execution.
7) Check SEGGER log - " PASS [psram_full_range_wr_test], PASS [psram_string_wr_test]" will be printed out in successful case.

Note:
1) Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
   RTT Block address for hex file committed in repository are as follows:
   a. e2studio: 0x2000c770
 
2) If an EP is modified, compiled, and downloaded please find the block address (for the variable in RAM called _SEGGER_RTT) 
   in .map file generated in the build configuration folder (Debug/Release).

Flashing Procedure:

1) Open e²studio and connect your JTAG debugger.
2) Go to Debug Configurations and select your .img.bin file in the Startup tab.
3) Start debugging and the image will be flashed automatically to the RA6W2.
