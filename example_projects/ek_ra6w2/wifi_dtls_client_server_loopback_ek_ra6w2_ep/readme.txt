/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrates the implementation of a DTLS client using PSK (Pre-Shared Key)
authentication on the Renesas RA6W2 platform.

The project establishes a secure DTLS connection over UDP between the RA6W2 embedded client
and an OpenSSL-based DTLS server running on a host PC.

2. Software Required:

SEGGER RTT Viewer or similar console application
Git Bash / OpenSSL for DTLS server execution

3. Hardware and Software Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
USB cable for programming and debugging.
PC/Laptop with OpenSSL installed.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.

Connect the USB Debug port on the RA6W2 motherboard to the host PC via a Type-C USB cable.

5. Hardware Settings for the Project

1. Jumper Configurations

    UART(RX) = P0_00
    UART(TX) = P0_01
    J102 ={2-3} [Current measurement]
    J106 ={2,3} [Voltage selector
    J105 ={2,3} [Voltage selector]
    J107 ={2,3} [Voltage selector]
    J210 ={1,2} [Reset]
    SWCLK(c) = SWCLK
    SWDIO(c) = SWDIO

6. Project Configuration:
Before building the project, update the following parameters in the source files.

Wi-Fi Configuration:

Update the following values inside config.h

WIFI_SSID
Configure the Wi-Fi Access Point name

WIFI_PASSWORD
Configure the Wi-Fi password

DTLS_SERVER_IP
Configure the IP address of the PC running the OpenSSL DTLS server

DTLS_SERVER_PORT
Configure the DTLS server port number
Example: 44330

PSK Configuration:
Update the following values inside dtls_client.c

psk_key
Ensure the PSK key matches the OpenSSL server PSK

psk_identity
Ensure the PSK identity matches the OpenSSL server identity

7. Importing and Building the Project:

1) Open e² studio.
2) Import the DTLS example project into the workspace.
3) Build the project.
4) Verify that the build completes without errors.

8. Flashing Procedure:

1) Connect the RA6W2 board to the PC using USB.
2) Open Debug Configurations in e² studio.
3) Select the generated image/binary file in the Startup tab if required.
4) Start debugging.
5) The image will be flashed automatically to the RA6W2 board.
6) Reset the board after flashing.

9. Creating DTLS Server Using OpenSSL:

1) Open Git Bash or terminal window on the host PC.
2) Run the following OpenSSL command:

openssl s_server -dtls1_2 -psk <your_psk_key> -psk_identity <your_psk_identity> -accept 44330 -nocert -quiet

Example:

openssl s_server -dtls1_2 -psk 0102030405060708 -psk_identity Client -accept 44330 -nocert -quiet

3) Keep the server running.
4) Wait for the RA6W2 DTLS client to connect.

10. Viewing RTT Logs:

1) Open SEGGER RTT Viewer or RTT window inside e² studio.
2) Select the appropriate J-Link device.
3) Use default RTT buffer settings.
4) Click "Connect".
5) Reset the board if logs do not appear immediately.
6) Observe the runtime logs printed by the application.

11. Verifying Operation:

Successful execution is verified when:
Wi-Fi connection is established
IP address is assigned
DTLS handshake completes successfully
Secure encrypted message is transmitted to the server