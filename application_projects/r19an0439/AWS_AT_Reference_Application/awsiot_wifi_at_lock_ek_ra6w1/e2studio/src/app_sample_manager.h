/***********************************************************************************************************************
 * File Name    : app_sample_manager.h
 * Description  : Network related defines & declarations used to operate device in DPM mode on platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "stdbool.h"
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"

#if !defined(_APP_SAMPLE_Manager_H_)
#define _APP_SAMPLE_Manager_H_

/*! type define app mutex*/
typedef SemaphoreHandle_t app_mutex;

typedef struct DOORLOCK_TAG
{
    int temperature;
    int battery;
    int doorStateChange;
    char *openMethod;
    bool doorState;
    bool doorBell;
    int doorOpenMode;
    int otaUpdate;
    char *otaResult;
    char *otaVersion;
    char *mcuotaVersion;
} Doorlock;

typedef enum _door_lock_command
{
    Door_Idle = 0,
    Door_Open,
    Door_Close,
    Door_Close_Timer,
    Door_Open_MCU,
    Door_Close_MCU,
} door_lock_command;

/*
 *   USER Thing name define
 *   Generic SDK default : CHIPSET_NAME
 *   AWS IOT default : "IOT-SENSOR-46" or "FAE-DOORLOCK-4" or "assigned_thing_name"
 *   AZURE IOT default: "enter Azure iot hub's device id instead of DA16200"
 */

// for Generic SDK
#define APP_USER_MY_THING_NAME                      CHIPSET_NAME

#if defined ( __SUPPORT_AZURE_IOT__ )
#define APP_USER_MY_DEV_PRIMARY_KEY                 "enter Azure iot hub's device key"
#define APP_USER_MY_HOST_NAME                       "enter Azure iot hub name"
#define APP_USER_MY_IOTHUB_CONN_STRING              "enter Azure iot hub connection string"
#endif

#if defined ( __SUPPORT_AZURE_IOT__ )
#define APP_NVRAM_CONFIG_DEV_PRIMARY_KEY            "APP_DEV_PRIMARY_KEY"
#define APP_NVRAM_CONFIG_HOST_NAME                  "APP_HOSTNAME"
#define APP_NVRAM_CONFIG_IOTHUB_CONN_STRING         "APP_IOTHUB_CONN_STRING"
#endif
#define APP_NVRAM_CONFIG_DPM_AUTO                   "APP_DPMAUTO"

#define SLEEP_MODE_FOR_NVRAM                        "setsleepMode"

#define SENSOR_REF_FACTORY_LED                      6    /* GPIOC 6 */

/**
 ****************************************************************************************
 * @brief get app thing name for AWS or Azure sever
 * @param[in] void
 * @return thing name string
 ****************************************************************************************
 */
char* getAppThingName(void);
void app_common_set_time_sync_from_normal_sntp(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval);
void app_common_set_time_sync_from_fast_sntp_with_nvram(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval,
                                                        unsigned long long added_sleep_dur);
void set_persistant_storage_cb(void);
int32_t app_common_goto_sleep_1_or_2(APPSleepMode _sleepMode, UINT8 _useRTM, INT32 _wakeupIntervalSec, UINT8 _timeSave);
void aws_shadow_dpm_auto_start(void *arg);

/*! memory malloc API for APP  */
#define app_common_malloc(x) pvPortMalloc(x)
/*! memory free API for APP  */
#define app_common_free(x) vPortFree(x)

#endif /* _APP_SAMPLE_Manager_H_ */

/* EOF */
