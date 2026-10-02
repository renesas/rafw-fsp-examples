/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This is a general-purpose Wi-Fi test application for the RA6W2 that uses AT commands over SDIO to establish and verify Wi-Fi connectivity.

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Renesas RA6M4 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

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
	Connect FD3_CS on J305 to FD3_CS_D on J305
	Connect FD2_DO on J304 to FD2_DO_D on J304
	Connect FD1_DI on J303 to FD1_DI_D on J303
	Connect FD0_SCLK on J302 to FD_SCLK_D on J302.

3. Connection of RA6W2 EVB for SDIO Interface.

	Connect p0_08 on J201 to SDIO1_CLK on J203.
	Connect p0_09 on J201 to SDIO1_CMD on J203.
	Connect p0_10 on J201 to SDIO1_D0 on J203.
	Connect p0_11 on J201 to SDIO1_D1 on J203.
	Connect p0_12 on J201 to SDIO1_D2 on J203.
	Connect p0_13 on J201 to SDIO1_D3 on J203.
	Connect jumper at J221 before powering the board.

4. SDIO pin connection for interfacing RA6W2 EVB with EK-RA6M4

	Connect SDIO_CLK to p413.
	Connect SDIO_CMD to p412.
	Connect SDIO_D0 to p411.
	Connect SDIO_D1to p410.
	Connect P0_04 on J201 to p414 on host.
 
6. Verification:
 1.  Import the example project.
 2.  Generate and build the example project.
 3.  Connect the RA6W2 MCU motherboard debug port to the host PC via a type C USB cable.
 4.  Debug or flash the EP project to the RA6W2 board.
 5.  Connect the RA6M4 MCU debug port to the host PC via a Micro USB cable.
 6.  After flashing both boards and making the SDIO pin connections to interface the RA6W2 EVB with the EK-RA6M4, reset both the RA6M4 and RA6W2 boards.
 6.  Use any terminal application for logging.
 7.  You can configure any of the following modes - Soft-AP, Station, or Station & Soft-AP - using AT Commands on the RA6M4 debug console 
     (connect RA6M4 USB FS port to the host PC via Micro USB cable). 
     For more details regarding   AT Commands, refer to the AT Command Manual. 

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
2. RA6W2: Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. RA6M4: Go to Debug Configurations and select your .srec file in the Startup tab.
4. Start debugging and the image will be flashed automatically to both the RA6W2 and RA6M4.
