/**
 ****************************************************************************************
 *
 * @file app_azure_user_conf.h
 *
 * @brief User defines used to operate device on Azure platform.
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#if !defined(_APP_AZURE_USER_CONF_H_)
#define _APP_AZURE_USER_CONF_H_

#include "app_common_util.h"

/** Define whether DPM app thread used or not */
#define ENABLE_AZURE_DPS_EXAMPLE				0

#define GLOBAL_PROV_URI                     "global.azure-devices-provisioning.net"
#define ID_SCOPE                            "0ne005545A6"   // change it by user's iot hub

/** Define the first default DNS IP address */
#define DEFAULT_AZURE_DNS_ADDR              "8.8.8.8"           // google DNS
/** Define the second default DNS IP address */
#define SECOND_AZURE_DNS_ADDR                 "208.67.222.222"  // openDNS

/*! Decide sample whether Dialog DPM sample or Amazaon original sample */
#define APP_AZURE_TWIN                    "azrMaT"      // < main thread name
//#define AZURE_MQTT_PORT                8883          // < IoTHub endpoint port
// #define AZURE_MQTT_PORT                     443         // < IoTHub endpoint port
#define AZURE_MQTT_PORT                     52443 // < Being used as local port here
#define AZURE_CONFIG_PORT_DEF               AZURE_MQTT_PORT    // < Default Azure server port
#define AZURE_CONFIG_KEEP_INTERVAL_MIN      60                 // < Minimux interval for keepalive
#define AZURE_CONFIG_KEEP_INTERVAL_TST      120                // < Short testing interval for keepalive with Azure server
#define AZURE_CONFIG_KEEP_INTERVAL_DEF      180                // < Default interval for keepalive with Azure server: 1177 = 1767 / 1.5

#define AZURE_CONFIG_KEEP_INTERVAL_MAX      1707               // < Maximum interval for keepalive with Azure server, about 28 minutes: 1767 - 60

#define AZURE_NVRAM_CONFIG_BROKER_URL       "AZURE_BROKER"          //< Azure broker's URL on NVRAM
#define AZURE_NVRAM_CONFIG_PORT             "AZURE_PORT"            //< Azure server port on NVRAM
#define AZURE_NVRAM_CONFIG_THINGNAME        "AZURE_THINGNAME"       //< Azure thing name on NVRAM
#define AZURE_NVRAM_CONFIG_KEEP_INTERVAL    "AZURE_KEEPINTER"       //< Azure keepalive interval on NVRAM
// add OTA mem.
#define AZURE_NVRAM_CONFIG_OTA_FLAG         "AZURE_OTAFLAG"                 // < OTA flag
#define AZURE_NVRAM_CONFIG_OTA_VERSION      "AZURE_OTAVER"                  // <OTA version
#define AZURE_NVRAM_CONFIG_CURRENT_OTA_VERSION  "AZURE_CUR_OTAVER"          // <OTA version
#define AZURE_NVRAM_CONFIG_MCUOTA_VERSION       "AZURE_MCU_OTAVER"          // <OTA version
#define AZURE_NVRAM_CONFIG_CURRENT_MCUOTA_VERSION   "AZURE_CUR_MCU_OTAVER"  // <OTA version

#define AZURE_NVRAM_CONFIG_OTA_URL      "AZURE_OTAURL"              // < OTA URL
#define AZURE_NVRAM_CONFIG_OTA_RESULT   "AZURE_OTARC"               // < OTA result value
#define AZURE_NVRAM_CONFIG_OTA_STATE    "AZURE_OTASTATE"            // < OTA state

#define AZURE_CONTROL_CONNECT           "connected"                 // < Azure app command for checking connection
#define AZURE_CONTROL_OPEN              "doorOpen"                  // < Azure app command for opening door
#define AZURE_CONTROL_CLOSE             "doorClose"                 // < Azure app command for closing door
#define AZURE_CONTROL_OTA               "confirmOTA"                // < Azure app command for confirming OTA
#define AZURE_CONTROL_SENSOR            "updateSensor"              // < Azure app command for updating sensor

#define AZURE_RESPONSE_CONNECT          "yes"                       // < Azure app response publishing for 'connected'
#define AZURE_RESPONSE_CLOSE            "closed"                    // < Azure app response publishing for 'closed'
#define AZURE_RESPONSE_OPEN             "opened"                    // < Azure app response publishing for 'opened'
#define AZURE_RESPONSE_OPEN_APP         "app"                       // < open response by APP
#define AZURE_RESPONSE_OPEN_MCU         "mcu"                       // < open response by MCU
#define AZURE_RESPONSE_NOT_OPEN         "none"                      // < not open response (checked by sensor)
#define AZURE_RESPONSE_CLOSE_TIMER      "timer"                     // < close response by timer
#define AZURE_RESPONSE_SENSOR_UPDATE    "updated"                   // < Azure app response publishing for 'updated'
#define AZURE_RESPONSE_LED_OFF          "off"                       // < Azure app response publishing for led 'off'
#define AZURE_RESPONSE_LED_ON           "on"                        // < Azure app response publishing for led 'on'
// add OTA mem
#define AZURE_UPDATE_OTA_OK             "OTA_OK"                    // < Azure app update string for OTA OK
#define AZURE_UPDATE_OTA_NG             "OTA_NG"                    // < Azure app update string for OTA NG
#define AZURE_UPDATE_OTA_UNKNOWN        "OTA_UNKNOWN"               // < Azure app update string for not OTA process : default

#if defined(__BLE_COMBO_REF__)
#define RTOS_NAME           "DA16600_FRTOS-GEN01.img"       // < OTA firmware name for RTOS
#define BLE_NAME            "DA16600_BLE_OTA.img"           // < OTA firmware name for Combo BLE f/w
#else
#define RTOS_NAME           "DA16200_FRTOS-GEN01.img"       // < OTA firmware name for RTOS
#endif

#define SLEEP_MODE_FOR_NARAM        "setsleepMode"
#define SLEEP_MODE2_RTC_TIME        "sleepmodertctime"

// #define _USING_PTIM_SENSOR_
// #define _TEST_SLEEP_

#define _NVRAM_SAVE_THING_FACORY_

#define    INIT_SENSOR_VAL_4B       (0xFFFFFFFF)        // < 4bytes initial value for sensor variables

// add OTA mem.
#define AZURE_OTA_RESULT_OK         0                   // < OK for OTA update
#define AZURE_OTA_RESULT_NG         1                   // < NG for OTA update
#define AZURE_OTA_RESULT_UNKNOWN    0xFF                // < unknown state for OTA update (default)

/* Platform Type */
#define AZURE_MODE_GEN      20 /* AZURE */
#define AZURE_MODE_ATCMD    21 /* AZURE + ATCMD    */

#define AZURE_OTA_STATE_READY       1
#define AZURE_OTA_STATE_UNKOWN      0xFF

#define AZURE_REPORT_VERSION_NONE   "none"

// azurefrtoswork[[
#define AZURE_SAS_EXPIRY_TIME       2000000000          // < over 50 years: 1600000000 = 2000000000 * 0.8
// ]]
// azuredpmwork[[
#define AZURE_TLS_SOCK_NAME         "azure_socka"       // < AZURE TLS session name on DPM mode
#define AZURE_TCP_SOCK_NAME         "azure_sockt"       // < AZURE TCP session name on DPM mode
//]]

#define AZURE_SUBTASK_PRIORITY      1

#define SLEEP_1_USE_RTM             0                   // < define whether RTM used or not on sleep mode 1: used - 1, not used - 0
#define SLEEP_2_USE_RTM             0                   // < define whether RTM used or not on sleep mode 2: used - 1, not used - 0
#define SLEEP_2_WAKEUP_TIME_INTERVAL_SEC    30          // < wakeup timer's default interval: 30 seconds

#define HOST_NAME_HEADER            "HostName="
#define DEVICE_ID_HEADER            "DeviceId="
#define SHARED_KEY_HEADER           "SharedAccessKey="

#define APP_NVRAM_DEVICE_CONNECTION_STRING      "DevConnectionString"

#define AZURE_RTM_NAME              "azure_rtm_data"

#define defaultROOT_CA_PEM "-----BEGIN CERTIFICATE-----\r\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"\
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"\
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"\
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"\
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"\
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"\
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"\
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"\
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"\
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"\
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"\
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"\
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"\
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"\
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"\
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"\
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"\
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"\
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"\
"MrY=\r\n"\
"-----END CERTIFICATE-----\r\n"

#define APP_DPM_TIMER_ID	0

#endif /* _APP_AZURE_USER_CONF_H_ */

/* EOF */
