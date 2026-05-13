/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Application Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

	This application uses both Central and Peripheral role. It starts up advertising and another central device may connect to it. After a successful
	connection, this central device is considered the main device. If the main device disconnects, advertising is started again. The next device that
	connects will become the main device. Event and connection information is printed to UART. A connected central can use the Multilink service (see below)
	to use the device's Central role and command it to connect to other peripheral devices that advertise. The main device can write a peer BD address
	to the Peripheral Address characteristic and the MULTILINK-RA6W2 device will initiate a connection procedure to this peer. This way it is possible
	to connect to more than one devices.

2. Software Requirements:
Renesas RAFW(FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24

3. Hardware Requirements:

    Attach RA6W2 module to the motherboard via the extension socket.
    Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.
	A Mobile device or a device with Renesas SmartBond App.

4. Hardware settings for the project
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

5. Execution Steps:
	1. Import the application project.
	2. Generate, build the application project.
	3. Use Segger/RTTViewer and UART for logging.
	4. Download Renesas SmartBond application from Android/iOS store
	5. Flash the application project to the RA6W2 board.
	6. After flashing, press reset.
	7. Setup a terminal in a host device with baudrate 115200 8N1
	8. Use a mobile device or a device with BLE_CLI capability and connect to MULTILINK-RA6W2
	9. After a successful connection, your device will become the main device: the main device controls
	   which peers the MULTILINK-RA6W2 shall connect to.
   10. Find the Peripheral Address characteristic and write one or more BD addresses of peripheral
	   devices which you would like MULTILINK-RA6W2 to connect to.

	   The first octet of data written to Peripheral Address characteristic is the address type, either
	   public (0x00) or private (0x01). The next 6 octets represent the BD address.

	   In order to connect to a device with public address AA:BB:CC:DD:EE:FF, the data written to the
	   characteristic should be:

	   [0x00 0xff 0xee 0xdd 0xcc 0xbb 0xaa]

	   Multilink service UUID:
	   {3292546e-0a42-4348-aa38-33aab6f9af93}

	   Peripheral Address characteristic UUID:
	   {3292546e-0a42-4348-aa38-33aab6f9af94} (properties=Write Without Response)

6. Known Limitations
	1) Up to 2 connection to peripherals is supported


Note:

To view console output in RTT Viewer:

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x200042d0

