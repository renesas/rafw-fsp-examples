/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates typical use of the I2C slave HAL module APIs.
The project initializes I2C slave and I2C master module with standard rate and is made interfaced with loop-back mechanism.
It performs Slave read and write operation continuously once initialization is successful. On successful I2C transaction
(6 bytes), Data transceived is compared. Led blinks on data match else it is turned ON as sign of failure.
Output message for both corresponding slave operations is displayed on RTT Viewer. Any API/event failure message is also 
displayed.

2. Software Requirements:
Segger J-Link RTT Viewer.

3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware Settings for the project
 1. Hardware Connection
    The SDIO pull-up resistor on the EK-RA6W2 board is used as I2C pull-up resistor
    EK-RA6W2
    --------
    Channel 1 is used for I2C master and channel 2 is used for I2C slave
    1) Master I2C pins
        I2C1 SDA (P0_09)  ----> Jumper J220 Pin 8
        I2C1 SCL (P0_08)  ----> Jumper J220 Pin 7
    
	2) Slave I2C pins
        I2C2 SDA (P0_12)  ----> SDIO1_D1 (on J203)
        I2C2 SCL (P0_11)  ----> SDIO1_D0 (on J203)
	
	3) I2C pull-up resistor on the EK-RA6W2
		Ensure jumper J221 is populated
	
	4) Connect LED pins
		LED output P0_10  ----> Jumper J611 Pin 1
	
6. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use Segger/RTTViewer for logging.
 4. Flash the EP project to the RA6W2 board.
 5. After flashing, press reset.
 6. Verify I2C slave read/write operations work correctly using segger prints.

7. Limitation
 1. The actual I2C clock output frequency may be approximately half of the configured value.


Note:
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20001b7c

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.