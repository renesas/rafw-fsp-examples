/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This project demonstrates how to securely store and retrieve data in the flash memory using the CC312 Security Library
on Renesas RA6W2 microcontrollers.

2. Software Requirements:
Terminal Console Application: Tera Term, RTT viewer or a similar application

3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Micro USB cable for programming and debugging or type C USB cable.


4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

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
	
6. Verification:
1. Import the Example project into e2 studio.
2. Build the project.
3. Flash the project onto the RA6W2 board via the debugger.
4. Open a SEGGER RTT Viewer or minicom on the host PC to view the console output.
5. Reset the board to start execution. 
6. Verify that the output shows:
	Correct display of stored and retrieved data. 
	Log message indicating “Flash Encrypted Test complete".
	
Note:
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20064d54
   
   
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
