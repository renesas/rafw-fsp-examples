/\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*

\* File Name    : readme.txt

\* Description  : Contains general information about the BLE Gateway Example Project and detailed instructions

\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*/



1\. Project Overview:



The BLE Gateway example project demonstrates a Bluetooth Low Energy (BLE) to MQTT gateway on the Renesas EK-RA6W2

platform. The application connects to a Wi-Fi network, continuously scans for BLE advertising packets from a target

sensor device (MOKO temperature/humidity/battery sensor), parses the sensor payload, and publishes the data as a

JSON message to a configured MQTT broker over Wi-Fi.



The application runs two concurrent FreeRTOS tasks:

&#x20; - App Task   : Initialises Wi-Fi, acquires an IP address, and configures the MQTT client.

&#x20; - BLE Thread : Initialises the BLE stack, performs continuous passive scanning, and publishes sensor data on detection.





2\. Software Requirements:



&#x20; - SEGGER J-Link RTT Viewer (for runtime log output)

&#x20; - Mosquitto MQTT Broker v2.x (https://mosquitto.org/download/) installed on the host PC





3\. Hardware Requirements:



&#x20; - Renesas EK-RA6W2 Evaluation Kit

&#x20; - Micro USB cable for programming and debugging

&#x20; - MOKO H4 Pro BLE temperature/humidity/battery sensor (or compatible)

&#x20; - Host PC connected to the same Wi-Fi network as the board





4\. Hardware Connections:



&#x20; - Connect the USB debug port on the EK-RA6W2 board to the host PC using a Micro USB cable.

&#x20; - Ensure the MOKO BLE sensor is powered on and within BLE range (\~10 m) of the board.





5\. Hardware Settings for the Project:



&#x20; - Power the board through the USB debug port.

&#x20; - Ensure the target Wi-Fi access point is active and the board is within range.

&#x20; - Ensure the host PC running Mosquitto and the EK-RA6W2 board are connected to the same Wi-Fi network.





6\. Application Configuration:



Before building the project, update the following parameters in the source files:



&#x20; In src/app\_task\_entry.c, update:

&#x20; 	WIFI\_HARD\_CODED\_SSID to the SSID

&#x20;	WIFI\_HARD\_CODED\_PASSWORD to the PASSWORD of the target Wi-Fi access point. 



&#x20; In src/custom\_app.c,update:

&#x20; 	MQTT\_BROKER\_IP to the IPv4 address of the host PC's Wi-Fi adapter that is on the same subnet as the EK-RA6W2 board. Use ipconfig to find this address.

&#x20; 	MQTT\_BROKER\_PORT to the broker port (default 1883). 

&#x20; 	MQTT\_PUB\_TOPIC and MQTT\_SUB\_TOPIC to the desired MQTT topics for publishing sensor data and receiving commands respectively.



&#x20; If using a different BLE sensor, update the g\_moko\_mac array in src/custom\_app.c with the

&#x20; target device's MAC address.





7\. Verifying Operation:



&#x20; Setting Up the MQTT Broker (Mosquitto):



&#x20; 1. Install Mosquitto from https://mosquitto.org/download/.



&#x20; 2. Open C:\\Program Files\\Mosquitto\\mosquitto.conf and ensure these lines are present:

&#x20;      listener 1883 0.0.0.0

&#x20;      allow\_anonymous true



&#x20; 3. Allow port 1883 through Windows Firewall (run PowerShell as Administrator, one-time):

&#x20;      New-NetFirewallRule -DisplayName "Mosquitto MQTT 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow



&#x20; 4. Verify the broker is running:

&#x20;      netstat -ano | Select-String ":1883"

&#x20;    A LISTENING entry confirms the broker is active.



&#x20; 5. Subscribe to incoming sensor data (PowerShell):

&#x09;\& "C:\\Program Files\\Mosquitto\\mosquitto\_sub.exe" -h 127.0.0.1 -p 1883 -t "YOUR\_PUB\_NAME" -v



&#x20;       eg :\& "C:\\Program Files\\Mosquitto\\mosquitto\_sub.exe" -h 127.0.0.1 -p 1883 -t "ble\_gateway\_pub" -v





&#x20; Flashing and Running the Project:



&#x20; 1. Import the BLE Gateway project into e2 studio.

&#x20; 2. Open configuration.xml using the FSP Configurator.

&#x20; 3. Verify all required stacks (Wi-Fi, BLE ABS, lwIP, MQTT, RTOS, and BSP) are configured.

&#x20; 4. Click "Generate Project Content".

&#x20; 5. Update WIFI\_HARD\_CODED\_SSID, WIFI\_HARD\_CODED\_PASSWORD, and MQTT\_BROKER\_IP as described in Section 6.

&#x20; 6. Build the project.

&#x20; 7. Flash the project onto the board using Renesas GDB Hardware Debug.

&#x20; 8. Reset the board.

&#x20; 9. Open SEGGER J-Link RTT Viewer on the host PC to view runtime logs.



&#x20; To view console output in RTT Viewer:



&#x20; 1) Find the RTT block address by searching for the \_SEGGER\_RTT variable in the .map file located in the Debug folder.

&#x20;    e.g.: 0x2005792c

&#x20; 2) Enter this address in the RTT Viewer "RTT Control Block" field and connect.





8\. Expected RTT Output:



&#x20; After reset, the device will:



&#x20;   - Print the application banner.

&#x20;   - Connect to the configured Wi-Fi access point.

&#x20;   - Obtain an IP address via DHCP.

&#x20;   - Initialise the MQTT client.

&#x20;   - Start continuous BLE passive scanning.

&#x20;   - On detection of the target MOKO sensor: parse payload and publish JSON to the MQTT broker.

