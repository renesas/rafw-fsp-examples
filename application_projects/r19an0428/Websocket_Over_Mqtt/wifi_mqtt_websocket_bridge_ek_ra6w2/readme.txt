/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1.Project Overview
This application demonstrates a secure MQTT-over-WebSocket bridge running on the EK-RA6W2. It establishes a TLS-secured WebSocket connection to a remote server, initializes an MQTT session over that connection, and handles periodic publishing and subscribing.

3.Software Requirements
Renesas RAFW (FSP): Version 2.0.1
e² studio Version 2025-12
GCC ARM Embedded Toolchain Version 13.3.1.arm-13-24
Serial terminal application (e.g., Tera Term)

4.Hardware Requirements
Renesas RA6W2 Mother Board
Renesas RA6W2 Module
USB cable for programming and debugging

5.Hardware Connections
Attach the RA6W2 module to the motherboard
Connect the USB debug port to the host PC

6. Hardware settings for the project
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

7. Verifying Operation:
    1. Import the example project.
    2. Update WebSocket server URI (WS_APP_SERVER_URI) and Root CA certificate (WS_APP_ROOT_CA) in src/websocket_client_app.c to match your WebSocket server.
    3. Edit the mosquitto.conf file to include the websocket protocol and listener port. Also provide paths to the cafile, certfile and keyfile generated for the websocket.
    4. Generate, build the Example project.
    5. Connect the RA6W2 MCU motherboard debug port to the host PC via a Type-C USB cable.
    6. Debug or flash the EP project to the RA6W2 board.
    7. After flashing, press reset.
    8. Configure Station Mode using Easy Setup.
    9. Console prints can be seen in Tera Term and Segger/RTTViewer.
    10. The WebSocket client starts automatically after connecting to the Wi-Fi network.
    11. The RA6W2 device establishes a secure WebSocket (WSS) connection to the configured server.
       Once the WebSocket is up, it connects to the MQTT broker (Client ID: RA6W2_WS) and:
       - Publishes "MQTT Message #X" to the topic: rrq61x_pub
       - Subscribes to the topic: rrq61x_sub
       - Prints any data received from the server to the console.

8.For Serial terminal application:

1) User need to enable CR to view console logs properly.	
2) The configuration parameters of the serial port on the terminal application are as follows:
	Baud rate:    115200 bps
	Data length:  8-bits
	Parity:       none
	Stop bit:     1-bit
	Flow control: none
Note:

To view console output in RTT Viewer: 
1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2005f824

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
