/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1.Project Overview
This application demonstrates MQTT communication tunneled over a secure WebSocket (WSS) connection running on the EK-RA6W2. It establishes a TLS-secured WebSocket connection to a remote broker, connects to the MQTT session, publishes periodic messages, and prints received messages to the console.

3.Software Requirements
- Serial terminal application (e.g., Tera Term)
- MQTT Broker supporting WebSockets (e.g., Mosquitto)
- MQTT Client for testing (e.g., MQTT Explorer or mosquitto_sub)

4.Hardware Requirements
Renesas RA6W2 Mother Board
Renesas RA6W2 Module
USB cable for programming and debugging
Host PC

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

    1. Generate certificates for a websocket server with these commands: 
	openssl ecparam -genkey -name prime256v1 -out ca.key
	openssl req -x509 -new -key ca.key -out ca.crt -days 3650 -subj "/CN=MyRootCA"
	openssl req -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -keyout server.key -out server.csr -subj "/CN=<ip_address>"
	openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650

    2. Import the example project.
    3. Update the WebSocket server URI (WS_MQTT_SERVER_URI) and Root CA certificate (WS_APP_ROOT_CA) in src/websocket_client_app.c to match the WebSocket server IP and certificate.
    4. Edit the MQTT broker's mosquitto.conf file to enable secure WebSockets by adding the following at the end of the file (adjust certificate paths as necessary): 
       
       listener 8765
       protocol websockets
       cafile /path/to/ca.crt
       certfile /path/to/server.crt
       keyfile /path/to/server.key
       # Enable detailed logging to see the WebSocket handshake
       log_type all

    5. Run the MQTT broker on the host PC. This will act as both the MQTT broker and the websocket server.
    6. Generate, build the Example project.
    7. Connect the RA6W2 MCU motherboard debug port to the host PC via a Type-C USB cable.
    8. Debug or flash the project to the RA6W2 board.
    9. After flashing, press reset.
    10. Configure Station Mode using Easy Setup.
    11. Console prints can be seen in Tera Term and Segger/RTTViewer.
    12. The WebSocket client starts automatically after connecting to the Wi-Fi network and syncing time via SNTP.
    13. The RA6W2 device establishes a secure WebSocket (WSS) connection to the configured server.
       Once the WebSocket is up, it connects to the MQTT broker and:
       - Publishes "Hello from RA6W2 via MQTT over WebSocket #X" to the topic: rrq61x_pub
       - Subscribes to the topic: rrq61x_sub
       - Prints any MQTT data received from the broker to the console.
    14. The device continuously publishes data to the MQTT broker, which replies with PUBACK packets. This can be verified by observing the message stream on the broker's terminal and the corresponding PUBACK receipts in the SEGGER RTTViewer. 

    15. To interact with the device, open an MQTT client on a separate terminal. Subscribing to the board's outgoing topic enables the device messages to be read on the PC's terminal, while publishing to the board's incoming topic enables MQTT messages to be sent directly to the RA6W2 via the websocket.
To subscribe to the device's messages, run this command on the terminal of the host PC: mosquitto_sub -h <ip_address of PC running the broker> -t "rrq61x_pub" -v
To publish data to the device, use this command: mosquitto_pub -h <ip_address of PC running the broker> -t "rrq61x_sub" -m "<message>"


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
