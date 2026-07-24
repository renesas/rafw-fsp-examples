/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Application Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This Application project demonstrates the BLE proximity reporter, the application and profiles are controlled by RA6W2
through the GTL interface which is connected to the DA14531 BLE chip. It can be connected to BLE host application like
Renesas Smartbond APP in mobile phone APP store and works as BLE proximity reporter.

2. Software Requirements:
Renesas RAFW(FSP)
e2 studio
GCC ARM Embedded Toolchain
Terminal Console Application: Tera Term
Mobile phone and BLE host application like Smart bond APP.

3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C cable for programming and debugging.

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

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

6. Verifying Operation:
1) Import the Application project into e2 studio.
2) Build the project.
3) Flash the image into the RA6W2 board.
4) Use segger/RTTViewer to check for logs or open a UART console to test like Tera term and
   set baud rate to 115200 and proper COM port number (lower port for RA6W2).
5) Reset the board to start execution.
6) Start scan in Smart bond APP on the mobile phone, check the Renesas PX Reporter in the list
7) Connect to the Renesas PX Reporter

7. Known issue: Workaround added in app_main.c file to support QEtool. xEventGroupWaitBits api return variable(uxBits) and OS_TASK_NOTIFY_APP_BITS added.

Note:

  Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
  Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x2005e1cc
