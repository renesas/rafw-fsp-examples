/***********************************************************************************************************************
 * File Name    : app_awsiot_user_config.h
 * Description  : user defines for the application.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if !defined(_APP_AWSIOT_USER_CONFIG_H_)
#define _APP_AWSIOT_USER_CONFIG_H_

/** Define the first default DNS IP address */
#define DEFAULT_AWS_DNS_ADDR					"8.8.8.8" ///< Google DNS 

/** Define the second default DNS IP address */
#define DEFAULT_AWS_2ND_DNS_ADDR				"208.67.222.222" ///< openDNS

/**
 * @brief Response RECV Timeout after device published
 */
#define DEVICE_PUB_RESPONSE_TIMEOUT				300

#define AWS_NVRAM_CONFIG_BROKER_URL				"AWS_BROKER" ///< AWS broker's URL on NVRAM
#define AWS_NVRAM_CONFIG_PORT					"AWS_PORT" ///< AWS server port on NVRAM
#define AWS_NVRAM_CONFIG_THINGNAME				"AWS_THINGNAME" ///< AWS thing name on NVRAM
#define AWS_NVRAM_CONFIG_KEEP_INTERVAL				"AWS_KEEPINTER"	///< AWS keepalive interval on NVRAM
#define AWS_NVRAM_CONFIG_DPM_AUTO				"AWS_DPMAUTO" ///< DPM running flag

#define AWS_NVRAM_CONFIG_OTA_FLAG				"AWS_OTAFLAG" ///< OTA flag
#define AWS_NVRAM_CONFIG_OTA_URL				"AWS_OTAURL" ///< OTA URL
#define AWS_NVRAM_CONFIG_OTA_RESULT				"AWS_OTARC" ///< OTA result value
#define AWS_NVRAM_CONFIG_OTA_STATE				"AWS_OTASTATE" ///< OTA state

#define AWS_CONTROL_CONNECT					"connected" ///< AWS app command for checking connection
#define AWS_CONTROL_OPEN					"doorOpen" ///< AWS app command for opening door
#define AWS_CONTROL_CLOSE					"doorClose" ///< AWS app command for closing door
#define AWS_CONTROL_OTA						"confirmOTA" ///< AWS app command for confirming OTA
#define AWS_CONTROL_SENSOR					"updateSensor" ///< AWS app command for updating sensor

#define AWS_RESPONSE_CONNECT					"yes" ///< AWS app response publishing for 'connected'
#define AWS_RESPONSE_CLOSE					"closed" ///< AWS app response publishing for 'closed'
#define AWS_RESPONSE_OPEN					"opened" ///< AWS app response publishing for 'opened'
#define AWS_RESPONSE_OPEN_APP					"app" ///< open response by APP
#define AWS_RESPONSE_OPEN_MCU					"mcu" ///< open response by MCU
#define AWS_RESPONSE_NOT_OPEN					"none" ///< not open response (checked by sensor)
#define AWS_RESPONSE_CLOSE_TIMER				"timer" ///< close response by timer
#define AWS_RESPONSE_SENSOR_UPDATE				"updated" ///< AWS app response publishing for 'updated'

#define AWS_UPDATE_OTA_OK					"OTA_OK" ///< AWS app update string for OTA OK
#define AWS_UPDATE_OTA_NG					"OTA_NG" ///< AWS app update string for OTA NG
#define AWS_UPDATE_OTA_UNKNOWN					"OTA_UNKNOWN" ///< AWS app update string for not OTA process : default

#define INIT_SENSOR_VAL_4B					(0xFFFFFFFF) ///< 4bytes initial value for sensor variables
#define SENSOR_VAL_NOT_AVAILABLE				"Not available" ///< string for not available sensor

/* add OTA related state */
#define AWS_OTA_RESULT_OK					0 ///< OK for OTA update
#define AWS_OTA_RESULT_NG					1 ///< NG for OTA update
#define AWS_OTA_RESULT_UNKNOWN					0xFF ///< unknown state for OTA update (default)

#define AWS_OTA_STATE_READY					1 ///<
#define AWS_OTA_STATE_UNKOWN					0xFF ///< 

#define AWS_SUBTASK_PRIORITY					1

#define DL_DATA_RTM_NAME					"DOOR_LOCK_RTM_NAME"
/**
 * @brief data used to save persistent information on RTM for platform
 */
typedef struct _DL_dpm_info_rtm
{
	INT32 doorOpenMode; ///< Door open mode auto/manual
	INT32 doorOpen; ///< door state
	float temperature; ///< temperature
	float battery; ///< battery
} DL_dpm_info_rtm;

DL_dpm_info_rtm* DL_dpm_info_get(char *_name);
#endif //_APP_AWSIOT_USER_CONFIG_H_
