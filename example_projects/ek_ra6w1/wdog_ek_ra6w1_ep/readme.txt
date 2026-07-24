/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
The example project demonstrates the typical use of the WDOG HAL module APIs.
User can give input through JLinkRTTViewer to start the WDOG. WDOG gets refreshed periodically through GPT timer.
The LED 1 turns on when WDOG is started and the it starts blinking while GPT timer is running.
User can press the push button(BTN1) to stop the GPT timer which in turn stops refreshing WDOG.
After 3 seconds, WDOG resets the MCU and turn the blinking the LED 1 OFF.
When Watchdog reset happens, the LED 2 turns ON.   

2. Software Requirements:
Segger J-Link RTT Viewer.

3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 daughter board.
Type C USB cable for programming and debugging
Two Jumper Wires that type is Female to Female.
Two Jumpers

4. Hardware Connections:
Attach RA6W1 daughter board to the motherboard via the extension socket.
Connect the USB debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project:
1.Jumper Configurations
	BTN1 (J201)  <---> P0_13 (J201) [Exnteral interrupt]
	J213                            [Button1 connection]
	P1_12 (J203) <---> LED 1 (J611) [LED1 connection]
	P1_13 (J203) <---> LED 2 (J611) [LED2 connection]

6. Verifying Operation:
 1. Import the example project.
 2. Generate the configuration.xml, build the Example project.
 3. Connect the RA6W1 mother board debug port to the host PC via a type C USB cable.
 4. Flash the project onto the RA6W1 board via the debugger.
 5. After flashing the image,  press reset. 
 6. Start Segger J-Link RTT viewer and connect.
 7. Input 1 on RTT viewer to start this watchdog project. 
    LED 1 starts blinking while WDOG gets refreshed by the timer.
 8. On RA6W1 mother board, press the BTN1 (J609) to stop the GPT for refreshing.
    LED 1 stops blinking and LED 2 turns on after WDOG reset has happened.

Note:
To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg: 0x20000b7c

Flashing Procedure:
1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.