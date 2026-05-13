/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about the OpenWeatherMap Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:

The OpenWeatherMap example project demonstrates how to fetch live weather data from the internet using a REST API on the Renesas RA platform.
The application connects to a Wi-Fi network, sends an HTTP request to the OpenWeatherMap server using a valid API key, and retrieves weather information for a specified city. The received weather data is displayed through runtime logs using RTT.

2. Software Requirements:

Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: RTT viewer or a similar application

3. Hardware Requirements:

Renesas RA6W1 Motherboard.
Renesas RA6W1 Wi-Fi Module.
Micro USB cable or USB Type-C cable for programming and debugging.

4. Hardware Connections:

Attach the RA6W1 Wi-Fi module to the motherboard using the extension connector.
Connect the USB debug port on the RA6W1 motherboard to the host PC using a USB cable.

5. Hardware Settings for the Project:

Ensure the board is powered through the USB debug port.
Ensure the Wi-Fi network used for testing is available and the SSID and password are correctly configured in the application source code.

6. Application Configuration:

Before building the project, update the following parameters in the source code:

Wi-Fi SSID      : Name of the wireless access point (weather_app.h)
Wi-Fi Password  : Password of the access point (weather_app.h)
API Key         : OpenWeatherMap API key (weather_app.c)
City Name       : Location for which weather data is required (weather_app.c)

These parameters are user configurable and determine the weather information retrieved during runtime.

7. Verifying Operation:

1. Import the OpenWeatherMap example project into e2 studio.
2. Open the project configuration.xml using the FSP Configurator.
3. Ensure the required stacks (Wi-Fi, BSP, networking, RTOS and other dependencies) are properly configured.
4. Generate the project content using "Generate Project Content".
5. Build the project.
6. Flash the project onto the board using Renesas GDB Hardware Debug.
7. Reset the board.
8. Open SEGGER J-Link RTT Viewer on the host PC to view runtime logs.

To view console output in RTT Viewer: 

1) Find the RTT block address by searching for the _SEGGER_RTT variable in the .map file located in the Debug or Release folder.
   eg:0x20007f74


After reset, the device will:

• Connect to the configured Wi-Fi network.
• Send an HTTP request to the OpenWeatherMap server.
• Retrieve weather data for the configured city.
• Display the weather information in the RTT Viewer console.

Typical output includes:

City name
Temperature
Weather condition (clear, cloudy, rain, etc.)
Other weather parameters received from the API response.

