/**********************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
**********************************************************************************************************************/

1. Project Overview:
This project shows how to implement Over The Air(OTA) firmware upgrade using AWS S3 OTA feature. 

2. Software Requirements:
Terminal Console Application: Tera Term or a similar application  

3. Hardware Requirements:
Renesas RA6W1 Mother board.
Renesas RA6W1 Module.
Micro USB cable for programming and debugging or type C USB cable.

4. Hardware Connections:
Attach RA6W1 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W1 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project

Jumper Configurations
	UART(RX) = P0_00
	UART(TX) = P0_01
	J102 ={2-3} [Current measurement]
	J106 ={2,3} [Volatge selector]
	J105 ={2,3} [Volatge selector]
	J107 ={2,3} [Volatge selector]
	J210 ={1,2} [reset]
	SWCLK(c) = SWCLK
	SWDIO(c) = SWDIO
	
5. Verification:
 1.  Import the AWS S3 OTA Update Example project into e2 studio.
 2.  Configure the following in AWS IoT port layer stack (rm_awsiot_w):
     - AWSIOT_W Thing Name: Device thing name configured in the AWS account
     - AWSIOT_W ROOT CA Certificate: AWS IoT RootCA1, which can be downloaded from AWS account.
     - AWSIOT_W Client Certificate: AWS IoT Certificate in pem, which can be downloaded from AWS account.
     - AWSIOT_W Client Private Key: AWS IoT Private Key in pem, which can be downloaded from AWS account.
     (Use the default configurations to test with demo account).
 3.  Configure the following in the file, config_s3_http.h. Which is located in the path, src/freertos_plus_demo_helpers:
     - democonfigIOT_CREDENTIAL_PROVIDER_ENDPOINT: Endpoint for the AWS IoT credential provider.
     - democonfigIOT_CREDENTIAL_PROVIDER_ROLE: Role alias name for accessing the credential provider, configured in the AWS account.
     - democonfigS3_BUCKET_NAME: Name of bucket in AWS S3 from where file needs to be downloaded.
     - democonfigS3_BUCKET_REGION: AWS Region where the bucket resides.
     - democonfigS3_OBJECT_NAME: Name of file that needs to be downloaded from AWS S3.
     (Use the default configurations to test with demo account).
 4.  Generate, build the Example project.
 5.  Connect the RA6W1 MCU motherboard debug port to the host PC via a type C USB cable.
 6.  Debug or flash the EP project to the RA6W1 board.
 7.  After flashing, press reset
 8.  Use any terminal application for logging.
 9.  Now the wifi setup menu will pop up on inital bootup. Setup the wifi details for internet connection.
 10. Write the S3 Bucket URL (ota_url) and OTA flag (ota_flag) into the NVRAM using cli commands:
     - nvram setenv appcfg ota_flag 1 (1 denotes the OTA update is ready)
     - nvram setenv appcfg ota_url <URL> (The URL for demo is aws-s3-ota-bucket.s3.ap-northeast-2.amazonaws.com)
 11. Now reboot the device using cli command, "reboot" or using reset button. 
 12. After reboot RA6W1 will connect to the configured AP.
 13. After connection to AP is successfull, RA6W1 will check for the OTA flag is set ready and OTA url is available.
     If it is available, then RA6W1 will start the OTA update demo and start to download the image file from the S3 bucket.
     If the configuration is not available, then error message will be shown.
 14. The downloaded image is written to sflash in the address specific for OTA image.
 15. After the whole image is downloaded, the application will call for ota renew API. This will read the image from
     sflash and check the image version from the version header in the OTA image. If the version check passes, then the
     boot index is updated and the device is rebooted. Now the device will bootup in the updated image.   

Note:

For Serial terminal application:
1) User need to enable CR to view console logs properly.
2) The configuration parameters of the serial port on the terminal application are as follows:
        Baud rate:    115200 bps
        Data length:  8-bits
        Parity:       none
        Stop bit:     1-bit
        Flow control: none

Flashing Procedure:

1. Open e²studio and connect your JTAG debugger.
2. Go to Debug Configurations and select your .img.bin file in the Startup tab.
3. Start debugging and the image will be flashed automatically to the RA6W1.
