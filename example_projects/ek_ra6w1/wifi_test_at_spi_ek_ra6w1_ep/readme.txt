/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This is a general-purpose Wi-Fi test application for the RA6W1 that uses AT commands over SPI to establish and verify Wi-Fi connectivity.

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
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


2. RA6W1 EVB jumper configuration for SPI over PMOD
	P1_00 - FD0_SCLK_DA on J203
	P1_01 - FD1_DI_DA on J203
	P1_02 - FD2_DO_DA on J203
	P1_03 - DF3_CS_DA on J203
	P0_04 - INT_0 on J201

 3. For connecting External Host please use below configurations
	MISOB - P410
	MOSIB - P411
	RSPCKB -P412
	SSLBO - P413
	IRQ09 - P414

 4. Additional wire configuration
	1. The PMOD connector of the RA6W1 should be connected to the PMOD2 port of the host for this program to work.
	2. Connect P0_04 to INT_0 in J201 OR connect P0_04 of RA6W1 to P414 of host.
	3. Connect jumpers between P1_00 and FD0_SCLK_DA ,P1_01 and FD1_DI_DA,P1_02 and FD2_D0_DA,p1_03 and FD3_CS_DA.
	4. Connect GND of RA6W1 to GND of host.

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
