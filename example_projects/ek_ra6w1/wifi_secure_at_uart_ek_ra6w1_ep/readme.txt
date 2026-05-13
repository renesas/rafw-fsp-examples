/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This is an example for the RA6W1, designed to showcase how to establish a secure AT communication channel using the AT
secure channel key and encrypted AT commands transmitted from the host to the RA6W1. The process begins by generating
a secure asset from the AT Secure Channel Key, using the SBOOT Tool, before programming this secure asset to the
designated AT Secure Channel Key address in the SFLASH.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.
Jumper Wire Female to Female.
Windows Host PC to run host application. 

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the programming PC via a type C USB cable.

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
	Connect P0_04 on J201 to FD0_SCLK_DA on J203
	Connect P0_05 on J201 to FD1_DI_DA on J203
	Connect FD3_CS on J305 to FD3_CS_D on J305
	Connect FD2_DO on J304 to FD2_DO_D on J304
	Connect FD1_DI on J303 to FD1_DI_D on J303
	Connect FD0_SCLK on J302 to FD_SCLK_D on J302
 3. For connecting External Host please use below configurations
   	RXD = P0_04
 	TXD = P0_05
 	RTS = P0_08
 	CTS = P0_09
 	GND = J219-P5 (GND)
        USB debug port with second COM port can be used for connection with Host PC.

6. Verification:
 Follow the steps to generate and flash the AT Secure Channel Key to RA6W as a Secure Asset
 1.  Download and extract the SBOOT tool on the host PC.
 2.  Using a software for ASCII-to-HEX conversion (such as HxD Freeware Hex Editor), create a new file and enter
     your desired secure channel key (also referred to as the ‘known key’ in this document) for the AT Secure
     Channel as decoded/plain text. In this example we have used the known key ‘atkey_on_prod’.
     The length of the key must be 16 bytes. If the key comes up short (such as in our example), you may add
     groupings of 0x00 (NULL) in the Hex editor to compensate.
 3.  Save the file as ‘at_key_to_secure.bin’. This key will be converted into a Secure Asset package by the SBOOT TOOL,
     which will subsequently be provisioned to the SFlash on the RA6W and used as the AT Secure Channel key.
 4.  Run CM.4.secuasset.bat in the SBOOT folder and provide the path to your input file
     (at_key_to_secure.bin) and output file (secure_asset.pkg.bin), before selecting the ‘Generate’ prompt to
     generate the Secure Asset. Once the key is generated, you can see the secure_asset.pkg.bin in the public folder (or your chosen
     location), as well as secure_asset.pkg.txt.
 5.  Using the cli_programmer.exe tool (over J-Link GDB Server), follow the below steps to write the secure key to
     the AT Secure Channel Key address in the RA6Wx’s SFlash:
     cli_programmer.exe gdbserver erase_qspi 0x3fe080 0x400
     cli_programmer.exe gdbserver write_qspi 0x3fe080 secure_asset.pkg.bin
 6.  Verify that the secure asset package has been successful flashed by reading the secure memory using
     cli_programmer.exe:
     cli_programmer.exe gdbserver read_qspi 0x3fe080 -- 0x100

 In order to unpack the Secure Asset on the device, the Kcp that was used to encrypt it must be provisioned to
 the OTP of the RA6W1, and this is done as part of the Secure Boot Image provisioning.
 Follow the below steps to prepare and Flash the Secure Boot image to the device:
 1.  Run the option Secure Boot on the security tool interface. Secure Boot images with the certificate chain are
     generated in the public directory. Secure Boot images in FreeRTOS SDK. RTOS image (XXRTOSXX.img) built from our
     SDK contains RTOS binaries
 2.  Load the RTOS Image into the RRQ610X00 board and in the console perform below commands:
     [/RRQ61000] sbrom
     [/RRQ61000/sbrom] run cmpu
 3.  Power on Reset the board (the Board will be booted with a new lifecycle).
 4.  You can verify the lifecycle state change in the console using the following command:
     [/RRQ61000/sbrom] run socid
 5.  You can verify that the life cycle is changed to DM. In the next step, you can run the dmpu:
     [/RRQ61000/sbrom] run dmpu
 6.  Power on Reset the board. When the board is rebooted, verify the LCS again.
     [/RRQ61000/sbrom] run socid
 7.  Verify that the board LCS is changed to SECURE from the result of above step, also the Soc-ID field is filled
     with values. After the successful execution of the above steps, the board is moved to the Secure State.

 AT Secure Channel requires synchronization between the Host PC and the target device (RA6W). 
 Use the Python GUI Application, secure_channel_v1.py in Host PC.
 Follow the below steps for verifying the AT Secure Channel:
 1.  Import the example project.
 2.  Generate, build the Example project.
 3.  Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
 4.  Debug or flash the EP project to the RA6W1 board.
 5.  After flashing, press reset
 6.  Use any terminal application for logging.
 7.  Open secure_channel_v1.py in a python editor and configure the ‘known_key’ variable:
     In the example, the AT Key in ASCII is ‘atkey_on_prod[NUL] [NUL] [NUL]’,
     the equivalent HEX value of this key is “61746b65795f6f6e5f70726f000000”. The ‘known_key’ variable in the
     ‘secure_channel.py’ program running on the host PC should receive this same hexadecimal value.
     For ease of use, the ‘secure_channel.py’ program is designed to take the ASCII value of the key (i.e.
     ‘atkey_on_prod’) and convert it to HEX, with the appropriate zero-padding, automatically.
 8.  Run secure_channel.py on the host (pyserial and pycryptodome library imports are required).
 9.  From the GUI, select the AT console COM port of the device and click Open Port.
 10. Click "Enable Secure Channel" (do not manually enter an IV value, it is randomly generated).
     After activation, the GUI’s plain data log should display “Secure Channel enabled”, and all transmitted AT
     commands will be encrypted using the AT key.
 11. Enter AT commands in the “Enter full AT Command” field. The GUI will display both the encrypted and the
     plain-text logs or verification.


Note:
The RA6W1 must use a Secure Boot image to enable AT Secure Channel, as the Hardware Key
(Kcp) used to pack the AT Secure Channel Key as a Secure Asset must be provisioned to the device as part of
the Secure Boot image. Secure Storage of credentials in the SFLASH via the <ESC>CERT AT Command can
still operate independently of these requirements.
Also while making the chip secure need to ensure that images and Secure Asset files are created on the same setup exactly with the same SBOOT tool



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
