/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The application project demonstrates how we can transfer raw data in a binary mode using the BLE_CODELESS.

2. Software Requirements:

Renesas RAFW(FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Mobile app: Renesas SmartConsole

3. Hardware Requirements:

1 x Renesas EK-RA6Wx Mother board.
1 x Renesas RA6W2 Daughter board.
1 x USB type C cable.
2 x Jumper Wire Female to Female.
4 x Jumpers

4. Hardware Connections:
Attach RA6W2 Daughter board to the motherboard via the extension socket.
Jumpers should be placed on pair of pins 1-2, 5-6, 9-10 and 13-14 of J3.
Connect P0_4(10 of J201) pin to SCLK(30 of J203) for UART1 RX.
Connect P0_5(12 of J201) pin to MOSI(32 of J203) for UART1 TX.

5. Verifying Operation:
1) Import the application project.
2) Generate, build the application project.
3) Use Segger/RTTViewer for logging.
4) Connect the EK-RA6Wx motherboard debug port to the host PC via a type C USB cable.
5) Debug or flash the e2studio project to the RRQ board.
6) After flashing the image, press reset.
7) Open the higher numbered Serial port using a terminal emulator(TeraTerm).
8) Open SmartConsole app on Mobile device.
9) You will see CLv2-CodeLess from the scan list on SmartConsole app.
10) Select CLv2-Codeless to connect.
11) You will see "+CONNECTED" in the terminal emulator.
12) Enter "AT+BINREQ" in the text edit box on SmartConsole app to start binary mode for raw data transfer,
13) Type "AT+BINREQACK" into the terminal emulator. Now it has entered into a binary mode.
14) Enter some string the "Send Console Mode Data" window of the SmartConsole app, this can be viewed in the terminal emulator.
15) Enter string in the terminal emulator, and it can be seen at SmartConaole app in the "Receive Console Mode Data" window.

6. Temporary Workaround
   - Excluded rm_atcmd_w_core_da14xxx_parse.c and rm_atcmd_w_core.c files from build
   - Added rm_atcmd_w_core_da14xxx_parse_temp_workaround.c and rm_atcmd_w_core_temp_workaround.c files in src folder.

 Note:

To view console output in RTT Viewer:

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20052794