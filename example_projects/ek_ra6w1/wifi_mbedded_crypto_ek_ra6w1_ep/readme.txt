/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the Mbedded crypto encryption and decryption.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application

3. Hardware settings for the project:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Type C USB cable for programming and debugging.

4. Hardware Connections:
Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Verifying Operation:
1. Import the example project.
2. Generate and build the example project.
3. Use Segger/RTTViewer for verifying.
4. Flash the EP project to the RA6W1 board.
5. After flashing, press Reset button.
6. Check the encryption and decryption process for each Mbedded crypto mode in the Segger/RTTViewer.

Note:
To view console output in RTT Viewer:
1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2000bbd0

Flashing Procedure:
1) Open e2studio and connect your JTAG debugger.
2) Go to Debug Configurations and select your .img.bin file in the Startup tab.
3) Start debugging and the image will be flashed automatically to the RA6W1.