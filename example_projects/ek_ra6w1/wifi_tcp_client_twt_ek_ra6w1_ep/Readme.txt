/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

Example: wifi_tcp_client_twt_ek_ra6w1_ep : TCP Client with DPM and TWT Mode Usage

1. Project Overview:
The wifi_tcp_client_twt_ek_ra6w1_ep application acts as a TCP client in DPM mode and 
establishes a connection with a TCP server running on the Wi-Fi network.
It sends a packet every 120000 ms. An RTC timer is registered with DPM, 
which wakes up the ra6w1 every 120000 ms. The TCP client then sends a packet
and goes back to sleep.
It will also receive packets from the TCP server. If a packet is received while
in sleep mode, it will wake up the ra6w1, allowing the application to receive
the packet. The application will then go back to sleep.

With TWT (Target Wake Time) enabled:
If the connected AP supports TWT, wake-up and data transmission intervals follow the TWT parameters negotiated with the AP.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:

1 x Renesas ra6w1 Mother board
1 x Renesas ra6w1 Module.
1 x Micro USB cable for programming and debugging or 1x type C USB cable. 

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

5. Verifying Operation:
	1. Import the example project.
	2. Update TCPC_DEF_LOCAL_PORT, TCPC_DEF_PEER_IP_ADDR, TCPC_DEF_SEND_PERIOD, TCPC_DEF_KA_ENABLE, TCPC_DEF_KA_IDLE_TIME, TCPC_DEF_KA_INTVL_TIME and  TCPC_DEF_KA_MAX_PROBES in tcp_client_dpm.h
	3. Update TWT parameters TWT_WAKE_INT_MANTISSA, TWT_WAKE_INT_EXPONENT, TWT_WAKE_INT_MIN_TWT_WAKE_DUR, TWT_FLOW_TYPE, TWT_NEG_TYPE and TWT_TRIGGER
	4. Generate, build the Example project.
	5. Connect the ra6w1 MCU motherboard debug port to the host PC via a type C USB cable.
	6. Debug or flash the EP project to the ra6w1 board.
	7. After flashing, press reset;
	8. Run TCP server (ex. IONinja)on Host side.
	9. Provision WiFi and enable DPM
	10. tcp_client will get started after connecting to wifi

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