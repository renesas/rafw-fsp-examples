/*****************************************************************************************************************************************
* File Name    : readme.txt
* Description  : Contains general information about Example Project and detailed instructions
*****************************************************************************************************************************************/

1. Project Overview:
The example project showcases how we can create an application in RA6W2 using the AWS IoT platform.
It implements a cloud-connected door lock system.
The RA6W2 device connects to a Wi-Fi network and establishes a secure MQTT connection to AWS IoT Core.
The actual door lock hardware is expected to be controlled by an external MCU which is linked with RA6W2 usingAT commands.
The mobile application communicates with AWS IoT Core through the internet to remotely control the door lock via RA6W2 and MCU.
The Wi-Fi provisioning is done using BLE.

2. Software Requirements:
Renesas RAFW (FSP): Version 2.0.1
e2 studio: Version 2025-12
GCC ARM Embedded Toolchain: Version 13.3.1.arm-13-24
Terminal Console Application: Tera Term or a similar application


3. Hardware Requirements:
Renesas RA6W2 Mother board.
Renesas RA6W2 Module.
Type C USB cable for programming and debugging.
Jumper Wires.
Smart phone with Renesas Wi-Fi Provisioning app installed.

4. Hardware Connections:

Attach RA6W2 module to the motherboard via the extension socket.
Connect the USB Debug port on the RA6W2 mother board to the host PC via a type C USB cable.

5. Hardware settings for the project
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

 2. Additional Wire configuration
	5th & 6th pin of j201 into  1 & 2 of J203
	J3 pins connection 1,3,5,7

6. Verification:
 1.  Import the example project.
 2. Download jsmn.h file from https://raw.githubusercontent.com/zserge/jsmn/refs/heads/master/jsmn.h and copy it to the "e2studio/thirdparty" folder in the application if it is not present. If the sysytem has wget installed, the file will be downloaded automatically during build process.
 3.  Generate, build the Example project.
 4.  Connect the RA6W2 MCU motherboard debug port to the host PC via a type C USB cable.
 5.  Debug or flash the EP project to the RA6W2 board.
 6.  After flashing, press reset
 7.  Use any terminal application for logging.
 8.  Configure the AWS IOT application using the following AT commands:
     Set thing name: AT+AWS=SET,APP_THINGNAME,<thingname>
     Set host: AT+AWS=SET,AWS_BROKER,a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com
     Set subscribe topic: AT+AWS=SET,APP_SUBTOPIC,AppControl
     Set publish topic: AT+AWS=SET,APP_PUBTOPIC,DeviceControl
     Set attributes 1: AT+AWS=CFG 0 app_door 1 2
     Set attributes 2: AT+AWS=CFG 1 app_shadow 1 2
     Set attributes 3: AT+AWS=CFG 2 doorStat 1 1
     Set attributes 4: AT+AWS=CFG 3 battery 2 1
     Set RootCA: 
     <ESC(0x1B)>C0,-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----<CTRL+C(0x03)>

     Set Client Certificate:
     <ESC(0x1B)>C1,-----BEGIN CERTIFICATE-----
MIIDWjCCAkKgAwIBAgIVAIqSKvd/Qq2E9ZleQWN2Gk/iPw2GMA0GCSqGSIb3DQEB
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xODEyMDYwNjQw
MjZaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDZ/AbN7xxXgAslyB14
ZHV/MPUjrpgPSnrbHcLwhOpKILoHiLO6CTqfXv/pxcXyh0UCHpp1PF63m0vmYYuA
ueRgW23sjKPXRPyGnFPVjGntNhlFuXAWX1+m09GLkqdxWGz2wgKokSa8pMO/otTA
iV5+uh8y/7q5fuASGywZR5WeutH8yjw4ui5Il+66S2yUifsCDrNgsmfTIZdta4cV
umRKG6ZMT7XHEVuFMIrE5N2nfXu62CUWn4GOmmoF4iH0w6FINmV0f0sQcLS73+tv
CR/dFnNzRT3DLm8FJH7RD60jHiYoZQFNHWuSH98cAg3RSyuRnHx9mmD+O8jKyZ6D
r/7NAgMBAAGjYDBeMB8GA1UdIwQYMBaAFL6HYMtZyM54cz3RAAzyR1zF7+1TMB0G
A1UdDgQWBBQeh5c1lEyK80j/TBBMP6Cz/qU3ljAMBgNVHRMBAf8EAjAAMA4GA1Ud
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAp9tbJ4GOFoX11trWKc/HTfdb
TMIVu8KeEnIdFadgGhVcafH6cIrVBcocR5iAQNhV28P5dSFSrsdDOaiYQQ6XyaS9
oOfLJCHosFd0CCAkV+2ZEmxDA0bN+WDdCppQHocYoNt8h6X+Mh0h2hnfB2hPQwDX
TcaCwbJQy2XprqPpBo3ZuWqmSi55uslXj+2B4XgPZutim++8J7DHQbfHAGZwiAFN
90TNlhZBdI87Ga07p0db03KcBQs8dBMaABC0RK39LqJ5ZdQMT/Owx0+iO2Be7w30
7o06zCQB2A0nmfvAR8gSuImIBfKz2I1xQX5+CO4wes8RH5pNIOK2QrKgr9NJkA==
-----END CERTIFICATE-----<CTRL+C(0x03)>

     Set Private Key:
     <ESC(0x1B)>C2,-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA2fwGze8cV4ALJcgdeGR1fzD1I66YD0p62x3C8ITqSiC6B4iz
ugk6n17/6cXF8odFAh6adTxet5tL5mGLgLnkYFtt7Iyj10T8hpxT1Yxp7TYZRblw
Fl9fptPRi5KncVhs9sICqJEmvKTDv6LUwIlefrofMv+6uX7gEhssGUeVnrrR/Mo8
OLouSJfuuktslIn7Ag6zYLJn0yGXbWuHFbpkShumTE+1xxFbhTCKxOTdp317utgl
Fp+BjppqBeIh9MOhSDZldH9LEHC0u9/rbwkf3RZzc0U9wy5vBSR+0Q+tIx4mKGUB
TR1rkh/fHAIN0UsrkZx8fZpg/jvIysmeg6/+zQIDAQABAoIBADfE6fy/4xFj2fZF
l3yYvxLWdLE3VwH6fSoYGCqu5r4mV1HcIJdFCzGA/ZpSlg0xnG8pYz0BP/5bhfSg
Gi/J32rjmWD+rmBB7xWFY1FsRiGBSL/07H9c0Tz+TksWLy6pf981zbZQxIdY5Bfg
UewceQeVGKxUjvIsSql3ODYTgW0FR7h+YGtmtXJ+8SQi3FSRwDdbpyoLokUf1YaH
ksG1RPOxxah7Jr0YFN4waSixMMSb/fAxF5F1/mD0tgUSkUptRXu879mpA5+uYD+Q
YrPzEDhvd8mXPaH1f1e/29Kq+tUNtmBdzY8gcmWr2h859x3R6wpybbYJt5KWK4NT
7auoPKECgYEA/7yOGf8y2QJLfjW08qBAATnYZuZySrV7rNK5kxenGueZl2t52NCa
vRQC8nNqouu33RjiYHSR8NQk9cLdpjnQOVxtSEWTZIctOPhtw53EAdRfw4v2e/7n
oe9kR3VH1OUKfMDhduMUI09UGGyxsyRcKs1uLvvC5DX2XRAGafN1aDkCgYEA2jWD
5SAPPJU+cbkQbkSBmcJph8x0949c/HJ2U6xcMmwR8G4Jnlwe+w3expwKnNlpNXaZ
I+mm2BeXvyPJCgN/BMkDhU1xyDQBscCYrD9q41IU318CTmH6iExoUwv6NNuyOsFd
IJeYnG6ckgI7yGkY0wxQvsI8alleI2mLehHuwzUCgYEA3Ye1xPlXT7r4MH1PoPmG
WEmGlyS7DtKFLuFf1fagT+MeHpgAdfvGf1HNd77ZOgZdQI6k0w9HuMnctnO2U58z
K+1P0VJL6sJaP0actt58g2U4C4m73A+lEZbxVCFZNyetXQIsjTMKJ8g5PesyR8+Q
c5d/Af4fBldkcZtHIxK9uqkCgYAaqpmQwac7Bx4Xdb9NSm/wI3MUFmdg7ZM2gqJ1
PUYTH2Pd1wSz5pweoCZObTlay7LwxqqWWfJ6y/9Oa4ghAiZeplYYz0sNZVWjrF68
BhAA8cH9PjYg8BZW28eQBpGwLf0M8x53Yi9TRq05pq45oqZW/FVNypzpfjxj5X0X
EOP11QKBgQCDnAVbfrXC+4S5UNwxGHw4cZJwAvOkkeApV3WlBSZFbbGzIxrVy79O
7ETTGfSAbksUljV+2HZZVSXtgsCS/fzsFjMWYpeNRX3+9wtFfGCfxoygGW0JvOyY
kg61geirHUDYgog9XzGKATXc3K/m7JdyOcWdbf54nhzcEqjRv1DhCA==
-----END RSA PRIVATE KEY-----<CTRL+C(0x03)>

     NOTE: Please wait after every execution untill we get "OK"

 9.  Do the wifi provisioning using the Renesas Wifi Provisioning application from google play store or ios appstore.
 10. Select the option 'Start RRQ61400-based' for doing the Wi-Fi Provisioning using BLE for RA6W2.
 11. After provisioning the deivce will reboot itself and will try for connection with the provisioned access point. 
 12. After successfull connection on wifi, the device will connect to the AWS IoT platform.
 13. Select the AWS IoT tab from the hamburger menu icon in the top left side of the homescreen.
 14. Now, the AWS IoT window will show the interface to perform the lock and unlock operations.
 15. It will also show the status of connection with the device and AWS cloud. Also the subscribe and publish topic
     and messages.
 16. Press on "open door" or "close door" to toggle between lock and unlock after the connection with device is successfull. 
 17. When the lock/unlock is done from the mobile application, the AT command: +AWSIOT=SERVER_DATA 0 app_door <close/open>
     will sent from RA6W2 to external MCU
 18. Now, the MCU should do the door lock/unlock action and sent back the door status
     (along with other shadow details like battery if needed):
     AT+AWS=CMD MCU_DATA 2 doorStat <opened/closed> 3 battery 89, when the AT command: +AWSIOT=CMD_TO_MCU update is received from RA6W2
 19. Now, when this door status is received, the door lock state will change in the application.

When DPM is enabled follow the steps below:

 20. continue until the step 7 and add the following commands to configure DPM:
     AT command to configure sleep mode (currently only sleep mode 3 is used): AT+AWS=SET,SLEEP_MODE,3
     AT command to enable DPM: AT+AWS=SET,USE_DPM,1
 21. Now do the steps from 8 to 17. After publishing the state of doorlock, the AWS application is ready to sleep.
     Also BLE is stopped using ble svc_stop command. Now RA6W2 will go to sleep and the AT command: +PMGR:1 will be sent by RA6W2.
 22. When open/close door button is clicked in the application, RA6W2 will wakeup from sleep.
 23. Now the AT command: +INIT:WAKEUP,UC will be sent from RA6W2 at wakeup.
 24. When the MCU receives the wakeup AT command, it should sent the following AT commands to RA6W2:
     Add sleep constraint to hold RA6W2 from entering sleep: AT+PMGRCONSTRAINT=1,4
     Notify RA6W2 that MCU is ready to receive AT commands: AT+PMGRMCUWUDONE
 25. Follow steps 15 and 17. And release the RA6W2 to enter in to sleep by sending the AT command: AT+PMGRCONSTRAINT=2,4.
 26. Now, RA6W2 will go to sleep. 

Note:
Apply the door open or close button only after the device goes to sleep.

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
3. Start debugging and the image will be flashed automatically to the RA6W2.
