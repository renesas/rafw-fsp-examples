/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about the OTA Example Project and detailed instructions
**********************************************************************************************************************/
 
1. Project Overview:
 
The OTA (Over-The-Air) example project demonstrates firmware update functionality over a Wi-Fi network on the Renesas RA6W2 platform. The application connects to a Wi-Fi network, downloads a new firmware binary image from a configured HTTP server, writes it to the secondary flash bank, and triggers a reboot to execute the updated firmware.
 
2. Software Requirements:

Terminal Console Application: RTT viewer or a similar application
XAMPP (for hosting the OTA firmware image via Apache HTTP server)
 
3. Hardware Requirements:

Renesas RA6W2 Motherboard.
Renesas RA6W2 Wi-Fi Module.
Micro USB cable or USB Type-C cable for programming and debugging.
 
4. Hardware Connections:

Attach the RA6W2 Wi-Fi module to the motherboard using the extension connector.
Connect the USB debug port on the RA6W2 motherboard to the host PC using a USB cable.
 
5. Hardware Settings for the Project:
 
Ensure the board is powered through the USB debug port.
Ensure the Wi-Fi network used for testing is available and the SSID and password are correctly configured in the application source code.
Ensure the host PC running XAMPP and the RA6W2 board are connected to the same Wi-Fi network.
 
6. Application Configuration:
 
Before building the project, update the following parameters in the configuration header file (config.h):

Wi-Fi SSID      : Name of the wireless access point
Wi-Fi Password  : Password of the wireless access point
OTA Image URL   : Full HTTP URL pointing to the firmware binary hosted on the XAMPP Apache server (http://<host-PC-IP>/ota_image.bin)
 
These parameters are user configurable and determine the OTA server target and Wi-Fi credentials used during runtime.
 
7. Verifying Operation:
 
Setting Up the OTA HTTP Server (XAMPP):
 
The OTA firmware image must be hosted on an HTTP server accessible by the RA6W2 board.
Follow the steps below to set up a local server using XAMPP:

1. Download and install XAMPP on the host PC from https://www.apachefriends.org.
2. Copy the OTA firmware binary image (e.g., ota_image.bin) into the XAMPP htdocs directory (typically located at C:\xampp\htdocs\ on Windows).
3. Open the XAMPP Control Panel.
4. Click the Start button next to the Apache module to launch the Apache HTTP server.
5. Verify that the Apache server status shows as Running (green indicator) in the XAMPP Control Panel.
 
Verifying the Server is Running:
 
Open a web browser on the host PC and navigate to the OTA image URL configured in config.h (http://<host-PC-IP>/ota_image.bin).
- If the browser prompts a file download  -> server is running correctly and the image is accessible.
- If the page returns an error            -> verify Apache is started and the binary is placed in htdocs.
NOTE: Ensure the XAMPP Apache server is running and the OTA image is accessible via the configured URL BEFORE flashing the firmware onto the board.
 
Flashing and Running the Project:
 
1. Import the OTA example project into e2 studio.
2. Open the project configuration.xml using the FSP Configurator.
3. Ensure the required stacks (Wi-Fi, BSP, networking, RTOS, OTA middleware, and other dependencies)are properly configured.
4. Generate the project content using "Generate Project Content".
5. Build the project.
6. Flash the project onto the board using Renesas GDB Hardware Debug.
7. Reset the board.
8. Open SEGGER J-Link RTT Viewer on the host PC to view runtime logs.
 
To view console output in RTT Viewer:
 
1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
e.g.: 0x2005ec04
 
After reset, the device will:
 
Connect to the configured Wi-Fi network.
Send an HTTP request to the configured OTA image URL on the XAMPP server.
Download the firmware binary image.
Write the downloaded image to the secondary flash bank.
Verify the integrity of the downloaded image.
Reboot and execute the updated firmware.
 
Typical RTT output includes:
 
Wi-Fi connection status.
OTA image download progress.
Flash write status.
Image verification result.
Reboot notification and new firmware version confirmation.
