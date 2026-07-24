/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates how to configure wpa enterprise and ping test with geteway.

2. Software Requirements:

Terminal Console Application: Tera Term or a similar application

3. Hardware settings for the project:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Verifying Operation:
1) Import the example project.
2) Generate and build the example project.
3) Flash the EP project to the RA6W1 board.
4) After flashing, press the Reset button.
5) Use Segger/RTTViewer for verifying the example.
6) Input the wanted WPA enterprise AP number in the Segger/RTTViewer.
7) Input the WPA enterprise ID and the password in the Segger/RTTViewer.
8) Check the Wi-Fi connection status and the ping test result in the Segger/RTTViewer.

Note:
To input and view console output in RTT Viewer:
1) Select Manu -> Input -> Sending -> Send on Enter in J-Link RTT Viewer.
2) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2000bc78

Flashing Procedure:
1) Open e2studio and connect your JTAG debugger.
2) Go to Debug Configurations and select your .img.bin file in the Startup tab.
3) Start debugging and the image will be flashed automatically to the RA6W2.

Set up WPA enterprise network:
Band: 2.4G, Security: WPA2_ENT Auth type: PEAP, Auth Protocol: MSCHAPv2_GTC


Changing the SSID count:
If the desired SSID is not printed, please increase the SSID count.
1) Go to src folder in the example project.
2) Change the parameter #define MAX_WIFI_SCAN_RESULTS in wpa_ent_example.h.
   eg: #define MAX_WIFI_SCAN_RESULTS    50
3) Rebuild the example project and flash the EP project to the RA6W2 board.