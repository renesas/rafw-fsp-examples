/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The example project demonstrate the timers and pwm mode.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: RTT viewer

3. Hardware Requirements:

Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Micro USB cable for programming and debugging or type C USB cable.
Connect P0_04(J201 pin 10) to Any LED in J611 LED TEST using jumper wire, LED usage is specific to PWM mode only for checking PWM signal.

4. Verifying Operation:

1) Import the example project.
2) Generate, build the Example project.
3) Use segger/RTTViewer for test.
   Change input configuration(Menu->Input->Sending) to "Send on Enter".
4) Start a Debug session. This will automatically flash the image to the board.
5) After the debug session starts, click Run to execute the program (no need to press the board's reset button).
6) Enter each mode 1 or 2 or 3 to test periodic timer, pwm mode and one shot timer in Menu option and input the period or desired duty cycle.
   The one shot timer period is preset to 7000ms (in configuration.xml)
7) In case of periodic/one shot timer, the registered callback function will be excuted and show the log (No LED indication).

Note:
1) Segger RTT block address may be needed to download and observe EP operation using a hex file with RTT-Viewer.
   RTT Block address for hex file committed in repository are as follows:
   a. e2studio: 0x20000B74
 
2) If an EP is modified, compiled, and downloaded please find the block address (for the variable in RAM called _SEGGER_RTT) 
   in .map file generated in the build configuration folder (Debug/Release).

3) The clock source is to set Xtal 32KHz by default and doesn't need to be changed.

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.