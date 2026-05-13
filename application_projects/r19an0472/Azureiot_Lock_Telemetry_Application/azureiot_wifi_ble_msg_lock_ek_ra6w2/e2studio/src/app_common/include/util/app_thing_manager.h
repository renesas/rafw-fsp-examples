/**
 ****************************************************************************************
 *
 * @file app_thing_manager.h
 *
 * @brief Network related defines & declarations used to operate device in DPM mode
 *        on platform.
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
#include "stdbool.h"

#if !defined(_APP_THING_MANAGER_H_)
#define _APP_THING_MANAGER_H_

typedef enum {
    GENERIC = 0,
    APP_DOORLOCK,
    APP_AT_CMD,
    UNKNOWN
} PROV_FEATURES;

typedef struct DOORLOCK_TAG {
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

typedef enum _door_lock_command {
    Door_Idle = 0,
    Door_Open,
    Door_Close,
    Door_Close_Timer,
    Door_Open_MCU,
    Door_Close_MCU,
} door_lock_command;

/*
 *   USER Thing name define
 */

// for Generic SDK

#define APP_USER_MY_THING_NAME    "enter Azure iot hub device ID"

#if defined ( __SUPPORT_AZURE_IOT__ )
#define APP_USER_MY_DEV_PRIMARY_KEY                 "enter Azure iot hub's device key"
#define APP_USER_MY_HOST_NAME                       "enter Azure iot hub name"
#define APP_USER_MY_IOTHUB_CONN_STRING              "enter Azure iot hub connection string"
#endif

#define APP_NVRAM_CONFIG_THINGNAME                  "APP_THINGNAME"
#if defined ( __SUPPORT_AZURE_IOT__ )
#define APP_NVRAM_CONFIG_DEV_PRIMARY_KEY            "APP_DEV_PRIMARY_KEY"
#define APP_NVRAM_CONFIG_HOST_NAME                  "APP_HOSTNAME"
#define APP_NVRAM_CONFIG_IOTHUB_CONN_STRING         "APP_IOTHUB_CONN_STRING"
#endif
#define APP_NVRAM_CONFIG_DPM_AUTO                   "APP_DPMAUTO"

#define SLEEP_MODE_FOR_NARAM                        "setsleepMode"
#define SLEEP_MODE2_RTC_TIME                        "sleepmodertctime"

#define USE_DPM_FOR_NARAM                           "setuseDPM"

#define SENSOR_REF_FACTORY_LED                      6    /* GPIOC 6 */

#if defined ( __SUPPORT_AWS_IOT__ ) 
/**
 ***************************************************************************************
 * @brief Fleet Provisioning ThingID define
 ****************************************************************************************
 */
#define FP_DEMO_ID_SUFFIX                           "Renesas_DoorLockID"
#define USE_FLEET_PROVISION                         "AWS_USE_FP"

/**
 ****************************************************************************************
 * @brief Fleet Provisioning set status
 * @param[in] void
 * @return 0: disable default, 1: enable
 ****************************************************************************************
 */
uint8_t getUseFPstatus(void);
#endif  // ( __SUPPORT_AWS_IOT__ ) 

/**
 ****************************************************************************************
 * @brief get app thing name for AWS or Azure sever
 * @param[in] void
 * @return thing name string
 ****************************************************************************************
 */
char* getAppThingName(void);

#if defined ( __SUPPORT_AZURE_IOT__ )
/**
 ****************************************************************************************
 * @brief get app device primary key for Azure sever
 * @param[in] void
 * @return device primary key string
 ****************************************************************************************
 */
char* getAppDevPrimaryKey(void);

/**
 ****************************************************************************************
 * @brief get app host name for Azure sever
 * @param[in] void
 * @return host name string
 ****************************************************************************************
 */
char* getAppHostName(void);

/**
 ****************************************************************************************
 * @brief get app IOT HUB connection string for Azure sever
 * @param[in] void
 * @return connection string
 ****************************************************************************************
 */
char* getAppIotHubConnString(void);

#endif

/**
 ****************************************************************************************
 * @brief get provisioning feature
 * @param[in] void
 * @return provisioning value
 ****************************************************************************************
 */
PROV_FEATURES get_prov_feature(void);

/**
 ****************************************************************************************
 * @brief set provisioning feature
 * @param[in] provisioning value
 * @return void
 ****************************************************************************************
 */
void set_prov_feature(PROV_FEATURES flag);

#endif /* _APP_THING_MANAGER_H_ */

/* EOF */
