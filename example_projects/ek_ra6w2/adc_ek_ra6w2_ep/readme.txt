/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the typical use of the ADC HAL module APIs.
The project initializes the ADC in single scan or continuous scan mode based on user selection in RA6W2 configurator.
Once initialized, user can initiate the ADC scan and also stop the scan (in the case of continuous scan mode) using 
JLinkRTTViewer by sending commands. User provide ADC channel input voltage from 0V to 0.9V with a voltage supply unit 
at the ADC channel voltage input pin. Once ADC scan is initialized, Window Compare Mode is enabled and compares the	
ADC channel input voltage with the upper and lower limits. The upper limit and lower limit are configured in RA6W2
configurator. If the ADC channel input voltage is above the upper limit or below the lower limit, it triggers an event 
and notifies the user to act accordingly. Result and ADC status is displayed on the JLinkRTTViewer.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term, RTT viewer or a similar application

3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 motherboard to the host PC via a type C USB cable.

5. Hardware settings:

1.Jumper Configurations
	UART(RX) = P0_00
	UART(TX) = P0_01
	J102 ={2,3} [Current measurement]
	J106 ={2,3} [Volatge selector]
	J105 ={2,3} [Volatge selector]
	J107 ={2,3} [Volatge selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO

6. Verifying Operation:

1. Import the Example project into e2 studio.
2. Build the project.
3. Flash the project onto the RA6W2 board via the debugger.
4. Open a SEGGER RTT Viewer on the host PC to view the console output.
5. Reset the board to start execution.
6. Maximum analog input voltage is 0.9V.
7. Read samples using menu options:
	Test 1: Connect P0_04 → 0.9V power source.
		Press 1 → Start the ADC scan.
		Press 2 → Stop the ADC scan.
		Press any other key → Return to the Main Menu.
		
	Test 2: Connect P0_04 → 0.5V power source.
		Press 1 → Start the ADC scan.
		Press 2 → Stop the ADC scan.
		Press any other key → Return to the Main Menu.

Note:

To view console output in RTT Viewer: 

	1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   	eg: 0x00000000200037e4
   	Flashing Procedure:
 
 
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
	
