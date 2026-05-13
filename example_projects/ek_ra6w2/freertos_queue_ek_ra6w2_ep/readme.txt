/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the typical use of FreeRTOS.
Messages are shared between Sender_Task, ISR and Receiver_Task for the first few seconds.
Later, Sender and Receiver Tasks will be suspended timer will be stopped. Semaphore is acquired and released between Semaphore Task and ISR for the next few seconds and Semaphore Task is suspended.
To restart the application, power cycle the board.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e²studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application

3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.

4. Hardware Connections:
Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware Settings for the project
 1. Hardware Connection
    RA6Wx EVB

6. Verification:
 1. Import the example project.
 2. Generate, build the Example project.
 3. Use Segger/RTTViewer for logging.
 4. Start a Debug session. This will automatically flash the binary to the EK-RA6W1 board.
 5. After the debug session starts, click Run to execute the program.
 6. Verify RA6W2 send and receive the message queue data correctly using segger prints.


NOTE: 

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20003784

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.
