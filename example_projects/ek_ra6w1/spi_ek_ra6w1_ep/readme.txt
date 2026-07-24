/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the typical use of the SPI HAL module APIs using SPI and DTC modules. The project configure 
SPI channels (Channel 0 and Channel 1) in Master and Slave mode. Once the modules are initialized and the SPI 
channels are configured, Master and Slave can transmit and receive data based on commands from user sent through JLinkRTTViewer.
SPI data transmit and receive is done via DTC module.

2. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.

3. Hardware Connections:
Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

4. Hardware Settings for the project
 1. Hardware Connection
    Pin Connection for EK-RRQ61xxx
    MISO  ----> P0_06 - P1_11
    MOSI  ----> P0_05 - P1_12
    CLK   ----> P0_04 - P1_10
    CS	  ----> P0_07 - P1_13	

5. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use Segger/RTTViewer for logging.
 4. Start a Debug session. This will automatically flash the binary to the EK-RA6W1 board.
 5. After the debug session starts, click Run to execute the program.
 6. Verify RA6W1 receive data correctly from SPI master or slave using segger prints.


NOTE: 

User is expected to enter data of size not exceeding 64 bytes.
As SPI bit width is set to 32 bits, each 4 bytes of user data input will be transferred in single SPI transmission.
The bytes sent for slave should be lesser than the bytes sent for master when using WriteRead operation.
Operation is not guaranteed for any user input value other than integer,char(i.e. float, special char) through RTT.

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20003ccc
   
Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.

Known Issue
SPI Slave write data limited to 36 bytes.
