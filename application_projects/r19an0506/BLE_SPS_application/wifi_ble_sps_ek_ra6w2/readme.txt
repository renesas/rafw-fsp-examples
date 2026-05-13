/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Application Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

This application project demonstrates the BLE SPS application, the application and profiles are controlled by RA6W2
through the GTL interface which is connected to the DA14531 BLE chip. It can be connected to BLE host like
Renesas Smartconsole APP in mobile phone APP store and deliver the string/data from UART host to BLE host, vice versa

2. Software Requirements:

Renesas RAFW(FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term
Mobile phone and BLE host application like Smart console  APP.

3. Hardware Requirements:

Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C cable for programming and debugging.

4. Hardware Connections:

The GTL interface is connected internally.

Wiring details:
1) UART1 connections on RA6W2-EK
   RX: Jumper J201 Pin 10 (P0_04) -----> FTDI_TX (Jumper J203 Pin 2)
   TX: Jumper J201 Pin 12 (P0_05) -----> FTDI_RX (Jumper J203 Pin 4)
   RTS: Jumper J201 Pin 14 (P0_08) -----> FTDI_CTS (Jumper J203 Pin 6)
   CTS: Jumper J201 Pin 16 (P0_09) -----> FTDI_RTS (Jumper J203 Pin 8)
2) Console program on PC
   Enable flow control with RTS/CTS, Baudrate 115200 bps

5. Verifying Operation:
    Receive inputs on UART1 from MCU/PC uart console, sent it to BLE Host and Receive inputs on BLE Host
1) Import the Application project into e2 studio.
2) Build the project.
3) Flash the image into the RA6W2 board via the debugger(RFP) or UART.
4) Use segger/RTTViewer to check for logs or open a UART console to test like Tera term and
   set baud rate to 115200 and proper COM port number (lower port for RA6W2).
5) Open another UART console to test and set baud rate to 115200 and proper COM port number (higher port for UART host/MCU/PC).
6) Reset the board to start execution.
7) Start scan in Smart console APP on the mobile phone, connect to the SPS-RA6W2 in the list
8) In PC/Teraterm, input "123", it should be shown in Receive console of Mobile APP
9) In send console from the Smart console of Mobile APP, input "abc", it should be shown in PC/Teraterm

Note:
  Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
  RTT Block address for hex file committed in repository are as follows:
  a. e2studio: 0x2000bbb0