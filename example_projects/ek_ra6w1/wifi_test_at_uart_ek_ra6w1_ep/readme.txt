/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This is a general-purpose Wi-Fi test application for the RA6W1, designed to establish Wi-Fi connectivity using AT commands.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.
Connect the pin P0_10 with any of the in-build buttons on the mother board like BTN1 using a jumper wire.

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

 2. Additional Wire configuration
	Connect P0_04 on J201 to FD0_SCLK_DA on J203
	Connect P0_05 on J201 to FD1_DI_DA on J203
	Connect FD3_CS on J305 to FD3_CS_D on J305
	Connect FD2_DO on J304 to FD2_DO_D on J304
	Connect FD1_DI on J303 to FD1_DI_D on J303
	Connect FD0_SCLK on J302 to FD_SCLK_D on J302
 3. For connecting External Host please use below configurations
   	RXD = P0_04
 	TXD = P0_05
 	RTS = P0_08
 	CTS = P0_09
 	GND = J219-P5 (GND)

6. Verification:
 1.  Import the example project.
 2.  Generate, build the Example project.
 3.  Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
 4.  Debug or flash the EP project to the RA6W1 board.
 5.  After flashing, press reset
 6.  Use any terminal application for logging.
 7.  You can configure any modes [Soft-AP, Station, Station & Soft-AP] using AT Commands.
     For more details regarding ATCMD refer AT Command manual. 

Note:
For Serial terminal application:
1) User need to enable CR to view console logs properly.
2) The configuration parameters of the serial port on the terminal application are as follows:
	Baud rate:    115200 bps
	Data length:  8-bits
	Parity:       none
	Stop bit:     1-bit
	Flow control: none

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
