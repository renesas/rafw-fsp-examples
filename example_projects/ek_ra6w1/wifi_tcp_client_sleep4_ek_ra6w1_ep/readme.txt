/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The wifi_tcp_client_sleep4_ek_ra6w1_ep application acts as a TCP client in Sleep4 mode and 
establishes a connection with a TCP server running on the Wi-Fi network.
It sends a packet every 10000 ms. When vTaskDelay is called, device enters Sleep4. The TCP client then sends a packet
and goes back to sleep.
It will also receive packets from the TCP server. If a packet is received while
in sleep mode, it will wake up the RA6W1, allowing the application to receive
the packet. The application will then go back to sleep.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Micro USB cable for programming and debugging or type C USB cable.
Jumper Wire Female to Female.

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Hardware Settings : 

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
2. Update TCP_SERVER_IP, TCP_SERVER_PORT in config.h.
2. Update TCPC_DEF_SEND_PERIOD, TCPC_DEF_KA_ENABLE, TCPC_DEF_KA_IDLE_TIME, TCPC_DEF_KA_INTVL_TIME and  TCPC_DEF_KA_MAX_PROBES in tcp_client_dpm.h
3. Generate, build the Example project.
4. Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
5. Debug or flash the EP project to the RA6W1 board.
6. After flashing, press reset;
7. Run TCP server (ex. IONinja)on Host side.
8. Provision WiFi and enable DPM
9. tcp_client will get started after connecting to wifi

Note:

For Serial terminal application:
1) User need to enable CR to view console logs properly.	
2) The configuration parameters of the serial port on the terminal application are as follows:
	Baud rate:    115200 bps
	Data length:  8-bits
	Parity:       none
	Stop bit:     1-bit
	Flow control: none

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg:0x000000002000bea0

