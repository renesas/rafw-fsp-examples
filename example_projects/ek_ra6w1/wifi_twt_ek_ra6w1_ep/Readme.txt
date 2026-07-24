/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The wifi_twt_ek_ra6w1_ep is a basic TWT application. After connecting to access point, it initiates a TWT session negotiation with defined wake intervals and durations.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

Renesas ra6w1 Mother board.
Renesas ra6w1 Module.
Micro USB cable for programming and debugging or type C USB cable.

4. Hardware Connections:

Attach ra6w1 module to the motherboard via the extension socket.
Connect the USB Debug port on the ra6w1 mother board to the host PC via a type C USB cable.

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

6. Verifying Operation:

1. Import the example project.
2. Update TWT_WAKE_INT_MANTISSA, TWT_WAKE_INT_EXPONENT ,TWT_WAKE_INT_MIN_TWT_WAKE_DUR, TWT_FLOW_TYPE, TWT_NEG_TYPE, TWT_TRIGGER in twt_config.h.
3. Generate, build the Example project.
4. Connect the ra6w1 MCU motherboard debug port to the host PC via a type C USB cable.
5. Debug or flash the EP project to the ra6w1 board.
6. After flashing, press reset;
7. TWT negotiation will be initiated once Wi-Fi connection is complete.

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
3. Start debugging and the image will be flashed automatically to the ra6w1.