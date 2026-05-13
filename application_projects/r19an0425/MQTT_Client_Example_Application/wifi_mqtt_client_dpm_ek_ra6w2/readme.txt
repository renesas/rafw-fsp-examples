/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1.Project Overview
This MQTT example application demonstrates an MQTT client implementation on the RA6W2 device with DPM support . 
The application connects to an MQTT broker over a Wi-Fi network and supports MQTT publish and subscribe operations.
Messages are exchanged using configured MQTT topics and handled through registered callback and enabling periodic wakeup and low-power operation.

3.Software Requirements
Renesas RAFW (FSP): Version 2.0.1
e² studio version 2025-12
GCC ARM Embedded Toolchain Version 13.3.1.arm-13-24
Serial terminal application (e.g., Tera Term)

4.Hardware Requirements
Renesas RA6W2 Mother Board
Renesas RA6W2 Module
USB cable for programming and debugging

5.Hardware Connections
Attach the RA6W2 module to the motherboard
Connect the USB debug port to the host PC

6. Hardware settings for the project
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

7.Basic Configuration
MQTT broker address, MQTT topic name, MQTT port can be modified in mqtt.h

a) Modify mqtt.h

File Path:- wifi_mqtt_client_dpm_ek_ra6w2\e2studio\src\mqtt.h

Update the required MQTT configuration parameters in this file:

MQTT Broker Address
MQTT Topic Name
MQTT Port Number

Non-TLS port :1883
TLS port : 8883
Default USER_TOPIC: rrq61x_sub

8. Verifying Operation:
	1. Import the example project.
	2. Update MQTT broker address, MQTT topic name, MQTT port in mqtt.h
	3. (If TLS enabled) Update certificates (CA certificate, client certificate and private key) in mqtt_fsp_conf.h 
	4. Generate, build the Example project.
	5. Connect the ra6w2 MCU motherboard debug port to the host PC via a type C USB cable.
	6. Debug or flash the EP project to the ra6w2 board.
	7. After flashing, press reset.
	8. Configure Station Mode using Easy Setup, enable Deep Power Management (DPM), and enable SNTP time synchronization.
	9. Console prints can be seen in Tera Term.
       10. The MQTT client starts automatically after connecting to the network.
       11. Configure the MQTT broker and subscribe to the configured topic. The RA6W2 device wakes up at specified intervals from Deep Power Management (DPM) mode, publishes an MQTT message, and returns to DPM mode. The device also receives subscribed messages while operating in DPM mode

9.For Serial terminal application:

1) User need to enable CR to view console logs properly.	
2) The configuration parameters of the serial port on the terminal application are as follows:
	Baud rate:    115200 bps
	Data length:  8-bits
	Parity:       none
	Stop bit:     1-bit
	Flow control: none

10: Configure mqtt.conf on the Broker Side   

For easier setup, add the following parameters to the mqtt.conf file on the broker side:

     allow_anonymous true
     protocol mqtt
     listener 8883

     #Certificate
     cafile cas.crt
     certfile server.crt
     keyfile server.key  

     #TLS Version
     tls_version tlsv1.2 

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W2.