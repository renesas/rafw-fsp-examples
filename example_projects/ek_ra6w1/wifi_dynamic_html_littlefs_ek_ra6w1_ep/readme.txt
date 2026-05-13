/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This example project demonstrates a secure HTTPS server over Wi-Fi with LittleFS support, enabling users to access a dynamic web-based GUI, monitor real-time temperature and humidity sensor data over I2C, and upload files to the filesystem through a browser interface.

2. Software Requirements:

Renesas RAFW(FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term / J-Link RTT Viewer

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging or type C USB cable.

4. Hardware Connections:

Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

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

	I2C Sensor Connections:
        VIN  -> VIN
        GND  -> GND
        SCL  -> P010 (I2C1_SCL) 
        SDA  -> P007 (I2C1_SDA)
	
6. Verifying Operation:
 
 1.Import the example project.
 2.Generate, Build the Example project.
 3.Edit http_svr.h and update the Wi-Fi SSID and Password and build the Example project.
 3.Use Segger/RTTViewer for logging.
 4.Connect the RA6W1 motherboard debug port to your host PC using a USB Type-C cable.
 5.Debug or Flash the EP project to the RA6W1 board.
 6.Reboot the board and it will get connected in station mode.
 7.Connect our laptop to the same Wi-Fi.
 13.Open browser and browse https://ip/upload.html
 14.Verify that static html page is loaded(Welcome Select a file to upload and store into LittleFS: — this is expected for the demo).
 15.Click Browse.
 16.Add file to upload.(temp_humidity.html will give the real temperature and humidity updation dashboard ) . The default html files are attached in the folder default_html.
 17.Click Upload. The file will be stored in filesystem.
 18.Very using AT cmd AT+FSLST.
 
 
Behaviour:

1. Client will receive server hosting webpage and used to upload the files to filestystem.

Note:

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x000000002000bc80

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
