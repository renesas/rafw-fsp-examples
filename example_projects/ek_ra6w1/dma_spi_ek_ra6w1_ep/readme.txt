/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the typical use of the SPI HAL module APIs using SPI and DMAC modules. The project configure 
DMA support SPI channels (Channel 0 and Channel 1) in Master and Slave mode. Once the modules are initialized and the SPI 
channels are configured, Master and Slave can transmit and receive data based on commands from user sent through JLinkRTTViewer.
SPI data transmit and receive are done via DMAC module.

2. Hardware Requirements:
Renesas EK-RA6W1 Mother board. (RTK7WBA6W1S03000BK)
Renesas RA6W1 daughter board. (RTKRRQ61000)
Type C USB cable for programming and debugging
Four Jumper Wires that type is Female to Female.

3. Hardware Connections:
Attach RA6W1 daughter board to the motherboard via the extension socket.
Connect the USB debug port on the EK-RA6W1 mother board to the host PC via a type C USB cable.

4. Hardware settings for the project:
1. Jumper Configurations
    Pin Connection for EK-RRQ61xxx
    MISO  ----> P0_06 - P1_11
    MOSI  ----> P0_05 - P1_12
    CLK   ----> P0_04 - P1_10
    CS    ----> P0_07 - P1_13

5. Verifying Operation:
 1. Import the example project.
 2. Generate the configuration.xml, build the Example project.
 3. Connect the EK-RA6W1 motherboard debug port to the host PC via a type C USB cable.
 4. Start a Debug session. This will automatically flash the binary to the EK-RA6W1 board.
 5. After the debug session starts, click Run to execute the program.
 6. Start Segger J-Link RTT viewer and connect.
 7. Input data to transfer on the RTT.
 8. Check the recevied data and result debug message.

NOTE: 
User is expected to enter data of size not exceeding 64 bytes.
As SPI bit width is set to 32 bits, each 4 bytes of user data input will be transferred in single SPI transmission.
The bytes sent for slave should be less than the bytes sent for master when using WriteRead operation.
Operation is not guaranteed for any user input value other than integer, char(i.e. float, special char) through RTT.

1) Segger RTT block address may needed to download and observe EP operation using a hex file with RTT-Viewer.
   RTT Block address for hex file committed in repository are as follows:
   a. e2studio: 0x20003d0c
 
2) If an EP is modified, compiled, and downloaded please find the block address (for the variable in RAM called _SEGGER_RTT) 
   in .map file generated in the build configuration folder (Debug/Release).

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
