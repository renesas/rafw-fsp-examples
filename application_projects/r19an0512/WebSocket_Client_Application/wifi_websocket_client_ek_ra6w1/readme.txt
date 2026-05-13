/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1.Project Overview
This application demonstrates a secure WebSocket (WSS) client running on the EK-RA6W1. It establishes a TLS-secured WebSocket connection to a remote server, sends periodic text messages, and prints received server responses to the console.

3.Software Requirements
Renesas RAFW (FSP): Version 2.0.1​
e² studio Version 2025-12
GCC ARM Embedded Toolchain Version 13.3.1.arm-13-24
Serial terminal application (e.g., Tera Term)

4.Hardware Requirements
Renesas RA6W1 Mother Board
Renesas RA6W1 Module
USB cable for programming and debugging

5.Hardware Connections
Attach the RA6W1 module to the motherboard
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

7. Server-Side Setup:
   1. Set up a WebSocket server and generate the required certificates
      (server_key.pem and server_cert.pem) for TLS communication.
   2. Copy the contents of server_cert.pem and paste it as the value of WS_APP_ROOT_CA
      in src/websocket_client_app.c.
   3. Update WS_APP_SERVER_URI in src/websocket_client_app.c to match the server IP and port
      (e.g., "wss://<server_ip>:8765").

8. Verifying Operation:
    1. Import the example project.
    2. Update WebSocket server URI (WS_APP_SERVER_URI) and Root CA certificate (WS_APP_ROOT_CA)
       in src/websocket_client_app.c to match your WebSocket server (see Section 7 for details).
    3. Generate, build the Example project.
    4. Connect the RA6W1 MCU motherboard debug port to the host PC via a Type-C USB cable.
    5. Debug or flash the EP project to the RA6W1 board.
    6. After flashing, press reset.
    7. Configure Station Mode using Easy Setup (tested with WPA2-PSK security).
       Ensure SNTP is enabled during Easy Setup for TLS certificate validation.
    8. Console prints can be seen in Tera Term and Segger/RTTViewer.
    9. The WebSocket client starts automatically after connecting to the Wi-Fi network.
   10. The RA6W1 device establishes a secure WebSocket (WSS) connection to the configured server,
       sends periodic "Hello from RA6W1" text messages every 5 seconds, and prints any data
       received from the server to the console.

   Note: The WebSocket connection status (connected/disconnected) is visible only in the
   SEGGER RTT Viewer. The RA6W1 console (Tera Term) does not display the connection status.
   Use RTT Viewer to confirm that the WSS connection has been successfully established.

9.For Serial terminal application:

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
3. Start debugging and the image will be flashed automatically to the RA6W1.