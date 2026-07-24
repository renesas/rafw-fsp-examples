/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the typical use of the PMGR HAL module APIs using PMGR and GPIO modules.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application
    
3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Micro USB cable for programming and debugging or type C USB cable.
Jumper Wire Female to Female.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project

Pin Connection for EK-RA6W2
P0_13  -  BTN1
P0_10  -  Any LED in J611 LED TEST

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

6. Verifying Operation:

1. Import the Example project into e2 studio.
2. Build the project.
3. Flash the project onto the RA6W1 board via the debugger.
4. Reset the board.
4. Open a SEGGER RTT Viewer on the host PC to view the console output.
﻿5. Press BTN1 to wake device from sleep.
   To use BTN1, connecting the J213 PIN is mandatory in addition to P0_13.
   J213 ={1,2} = [BTN1]

Note: 

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder. eg: 0x20003894

Flashing Procedure:
 
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.