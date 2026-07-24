/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the typical use of the I2C master HAL module APIs.
The project initializes I2C master module with standard rate and interfaces with PmodACL™ Board for ADXL345.
On power up after establishing the connection of sensor with RA6W1 board, it displays accelerometer axis data on
RTTviewer. Any API/event failure will be displayed on RTTviewer.

2. Software Requirements:
Segger J-Link RTT Viewer.

3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.

4. Hardware Connections:
Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Hardware Settings for the project
 1. Hardware Connection
    PMOD ACL has two on board connectors. J2 is used for I2C communication.

    Jumper configuration
    --------
    I2C_SCK_0 = P0_08
    I2C_SDA_0 = P0_09

    Additional wire configuration
    --------
    I2C_SCK (on Arduino interface J207)  ----> SCL (on PMOD-ACL)
    I2C_SDA (on Arduino interface J207)  ----> SDA (on PMOD-ACL)
    GND     (on Arduino interface J207)  ----> GND (on PMOD-ACL)
    VDDIO   (on Arduino interface J207, or J221)  ----> VCC (on PMOD-ACL)

6. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use Segger/RTTViewer for logging.
 4. Flash the EP project to the RA6W1 board.
 5. After flashing, press reset.
 6. Verify RA6W1 read i2c data correctly from sensor board using segger prints.

7. Limitation
 1. The actual I2C clock output frequency may be approximately half of the configured value.


Note:
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20001cec

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.