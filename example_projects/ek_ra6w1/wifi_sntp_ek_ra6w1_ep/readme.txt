/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates how to configure sntp and get current time.

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
1) Import the example project.
2) Generate and build the example project.
3) Flash the EP project to the RA6W1 board with the Flashing Procedure.
4) After flashing, press the Reset button.
5) Target AP should be connected to the internet to connect to the SNTP server.
6) Select the number and input the password of the Target AP in the Segger/RTTViewer.
7) Make sure the target AP have internet connection, since SNTP needs to fetch time from the server. 
8) Check the current time in the Segger/RTTViewer.

Note:
To input and view console output in RTT Viewer:
1) Select Manu -> Input -> Sending -> Send on Enter in J-Link RTT Viewer.
2) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2000bc78

Flashing Procedure:
1) Open e2studio and connect your JTAG debugger.
2) Go to Debug Configurations and select your .img.bin file in the Startup tab.
3) Start debugging and the image will be flashed automatically to the RA6W1.

Changing the SSID count:
If the desired SSID is not printed, please increase the SSID count.
1) Go to src folder in the example project.
2) Change the parameter #define MAX_WIFI_SCAN_RESULTS in sntp_examples.h.
   eg: #define MAX_WIFI_SCAN_RESULTS    50
3) Rebuild the example project and flash the EP project to the RA6W1 board.