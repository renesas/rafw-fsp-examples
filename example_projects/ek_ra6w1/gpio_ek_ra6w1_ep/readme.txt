/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This example project demonstrates how to use FSP APIs to perform simple GPIO read and write operations.

2. Software Requirements:
Terminal Console Application: Tera Term,RTT viewer or a similar application

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging
Jumper Wire Female to Female.

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
2.Additional Configurations
Connect P1_14 to VDDIO using jumper cable.
Connect P1_15 pin to any LED in J611 LED TEST using jumper wire.

5.Verification:

Import the example project.
Generate, build the Example project.
Connect the RA6W1 motherboard debug port to the host PC via a type C USB cable.
Flash the EP project to the RA6W1 board.
After flashing, press reset.

Note:
	
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200037ec

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
	
	
