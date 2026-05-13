/***********************************************************************************************************************
 * File Name    : app_dpm_thread.h
 * Description  : Thread related APIs used to operate device in DPM mode on platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if !defined(_APP_DPM_THREAD_H_)
#define _APP_DPM_THREAD_H_

#include "rm_aws_lwip_sock_wrap_w_api.h"

#if !defined(SYS_ASSERT)
#define SYS_ASSERT     configASSERT
#endif

#if !defined(OAL_MSLEEP)
#define OAL_MSLEEP(wtime)               {                                                       \
                        portTickType xFlashRate, xLastFlashTime;                                \
                        uint32_t mtime = wtime;                                                 \
                        if (wtime < 10) mtime = 1;                                              \
                        xFlashRate = mtime/portTICK_RATE_MS;                                    \
                        xLastFlashTime = xTaskGetTickCount();                                   \
                        vTaskDelayUntil( &xLastFlashTime, xFlashRate );                         \
}
#endif

/*! print function for APP debugging */
#if !defined(APRINTF)
#define APRINTF(...) printf(__VA_ARGS__)
#endif

#if !defined(APRINTF_Y)
#define APRINTF_Y       APRINTF
#endif
#if !defined(APRINTF_I)
#define APRINTF_I       APRINTF
#endif
#if !defined(APRINTF_S)
#define APRINTF_S       APRINTF
#endif
#if !defined(APRINTF_E)
#define APRINTF_E       APRINTF
#endif

/* Note: Select after checking each memory region */
#define APP_MALLOC                                      pvPortMalloc
#define APP_FREE                                        vPortFree
#define APP_DPM_MALLOC                                  APP_MALLOC
#define APP_DPM_FREE                                    APP_FREE

#define APP_CONFIG_THINGNAME                            AWSIOT_CFG_THINGNAME
#define SLEEP_MODE2_RTC_TIME                            "sleepmodertctime"
#define USE_DPM_FOR_NVRAM                               "setuseDPM"

/* [aws work] */
#define WAKEUP_SOURCE_EXT_SIGNAL                        (BSP_WAKEUP_SOURCE_GPIO )
#define WAKEUP_EXT_SIG_WITH_RETENTION                   (BSP_WAKEUP_SOURCE_GPIO | BSP_WAKEUP_RETENTION )
#define WAKEUP_EXT_SIG_WAKEUP_COUNTER_WITH_RETENTION    (BSP_WAKEUP_SOURCE_GPIO | BSP_WAKEUP_RETENTION |BSP_WAKEUP_SOURCE_WAKEUP_COUNTER) //WAKEUP_SENSOR_GPIO_COUNTER_WITH_RETENTION
#define WAKEUP_COUNTER_WITH_RETENTION                   (BSP_WAKEUP_SOURCE_WAKEUP_COUNTER | BSP_WAKEUP_RETENTION)
#define WAKEUP_RESET                                    (BSP_WAKEUP_RESET)
#define WAKEUP_SOURCE_POR                               (BSP_WAKEUP_SOURCE_POR)
#define WAKEUP_WATCHDOG                                 (BSP_WAKEUP_SOURCE_WATCHDOG)
#define WAKEUP_SOURCE_WAKEUP_COUNTER                    (BSP_WAKEUP_SOURCE_WAKEUP_COUNTER)

/**
 * @brief DM_NOTI enum.
 *
 * Each enum values represent a state of DPM app thread.
 */
typedef enum
{
    DM_INIT = 0,				///< Initialization state
    DM_NEED_CONNECTION,		///< connection state
    DM_WAKEUP_RECV,			///< wake up state by data receiving
    DM_WAKEUP_SENSOR,		///< wake up state by sensor
    DM_WAKEUP_TIMER,		///< wakeup state by RTC timer
    DM_WAKEUP_BOOT,			///< wakeup state by POR or reboot
    DM_EXTERNAL_SENSOR,		///< wakeup state by external sensor
    DM_CHECK_RECV,			///< checking of receiving data before DPM sleep
    DM_CHECK_DEVICE,		///< checking device on exception state like 'No Ack'
    DM_FINISH_DEVICE,		///< finish state
    DM_NO_DPM_MODE			///< no DPM state
} DM_NOTI;

/**
 * @brief The wakeup timer interval depending APPTimerMode
 *
 * Wakeup timer interval by seconds
 */
#define DPM_RTC_ABNORMAL_INTERVAL_1ST	                10
#define DPM_RTC_ABNORMAL_INTERVAL_2ND	                30
#define DPM_RTC_ABNORMAL_INTERVAL_3RD	                300

/**
 * @brief Message queue structure for DPM app thread.
 */
typedef struct _dpmAPPDataQ
{
    int status;				///< status field
    void *userData;			///< payload field
} dpmAPPDataQ;

/**
 * @brief Callback function pointer for DPM app thread.
 */
typedef void (*dpmManagerFuncPtr)(UINT32 _pUserStruc, UINT32 _RTMdata, DM_NOTI _status);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Create DPM app thread to run on platform
 * @param[in] _name_ptr  Unique name for DPM app thread
 * @param[in] _entryFunc Callback fuction pointer
 * @param[in] _pUserStruc The first parameter of callback function
 * @param[in] _RTMdataSize The size(byte unit) used to this DPM app thread
 * @param[in] _stackSize Stack size(byte unit) of this thread to create
 * @param[in] _priority Priority of this thread
 * @param[in] _dpmPort The client' binding port to monitor in DPM mode. Not used, internally defined to 1883 for TCP MQTT
 * @param[in] _wakeUp_time The RTC timer's interval to keep alive with server
 * @param[in] _DNSAddr The first DNS server's address
 * @param[in] _2ndDNSAddr The second DNS server's address
 * @param[in] _SNTPCount The retry count to get current time from SNTP server
 * @return BaseType_t
 ****************************************************************************************
 */
BaseType_t dpmAppThreadCreate(char *name_ptr, dpmManagerFuncPtr _entryFunction, UINT32 _pUserStruc, UINT32 _RTMdataSize,
                              UINT32 _stackSize, UINT32 _priority, UINT32 _dpmPort, UINT32 _wakeUp_time, const char *_DNSAddr,
                              const char *_2ndDNSAddr, UINT32 _SNTPCount);

/**
 ****************************************************************************************
 * @brief Send some message to this DPM app thread
 * @param[in] _dpmAppData the payload pointer to send this DPM app thread
 * @return void
 ****************************************************************************************
 */
void sendDPMAppMSGQ(dpmAPPDataQ *_dpmAppData);

/**
 * @brief Sets the sleep mode and it's facotors on sleep mode 1/2/3(DPM)
 *
 * @param [in] _mode The value of one of the APPSleepMode enums
 * @param [in] _sec The value of wakeup timer interval by seconds
 * @param [in] _retention 0 or 1 only used in case of sleep mode 1/2
 *
 * @return void
 *
 */
void setSleepMode(APPSleepMode _mode, int _sec, unsigned char _retention);

/**
 * @brief Gets the current sleep mode
 *
 * @return APPSleepMode type value
 *
 */
APPSleepMode getSleepMode(void);

/**
 * @brief Get the flag whether RTM used or not
 *
 * @return 0 for not used, 1 for used
 *
 */
unsigned char getSleepRTMUsed(void);

/**
 * @brief Get the current wakeup timer interval by second
 *
 * @return interval by second, invalid if <= 0
 *
 */
int getSleepWakeupTimerIntervalSec(void);

/**
 ****************************************************************************************
 * @brief Gets the current wakeup timer interval by seconds on sleep mode 2 from NVRAM
 * @param[in] _defSeconds default timer interval by seconds when reading from NVRAM failed
 * @return wakeup timer interval by micro seconds
 ****************************************************************************************
 */
unsigned long long getAppSleep2Interval(int _defSeconds);

/**
 ****************************************************************************************
 * @brief Sets the main thread of App to go sleep mode DPM
 * @param[in] _resetTimer 0 or 1 whether resetting or not KA timer
 * @param[in] _mode APPTimerMode type wakeup timer mode
 * @return void
 ****************************************************************************************
 */
void goSleepAppDpmThread(UINT8 _resetTimer, APPTimerMode _mode);

/**
 ****************************************************************************************
 * @brief Sets the main thread of App to exit sleep mode DPM
 * @return void
 ****************************************************************************************
 */
void exitSleepAppDpmThread(void);

/**
 ****************************************************************************************
 * @brief Notify wakeup of the App's main thread to DPM module
 * @return void
 ****************************************************************************************
 */
void informWakeupAppDpmThread(void);

/**
 ****************************************************************************************
 * @brief Go into sleep 2 or reboot when exception happened such like connection failure
 * @param[in] _retryCount retried count for normal action
 * @return void
 ****************************************************************************************
 */
void goSleepAppOnException(UINT32 _retryCount);

/**
 ****************************************************************************************
 * @brief Unregister DPM KA timer of App
 * @return pdPASS or pdFAIL
 ****************************************************************************************
 */
UINT8 killDPMAppKeepAliveTimer(void);

#endif /* _APP_DPM_THREAD_H_ */

