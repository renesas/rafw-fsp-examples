/**
 ****************************************************************************************
 *
 * @file app_dpm_thread.h
 *
 * @brief Thread related APIs used to operate device in DPM mode on platform.
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

#if !defined(_APP_DPM_THREAD_H_)
#define _APP_DPM_THREAD_H_

//#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_common_support.h"
#include "sdk_defs.h"

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

// Note: Select after checking each memory region
#define APP_MALLOC          pvPortMalloc
#define APP_FREE            vPortFree
#define APP_DPM_MALLOC  APP_MALLOC
#define APP_DPM_FREE    APP_FREE

#define APP_NVRAM_CONFIG_THINGNAME                  AWSIOT_CFG_THINGNAME
#define SLEEP_MODE2_RTC_TIME                        "sleepmodertctime"
#define USE_DPM_FOR_NVRAM                           "setuseDPM"

//[azure dpm freertos work]
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
typedef enum {
	DM_INIT = 0,				///< Initialization state
	DM_NEED_CONNECTION,		///< connection state
	DM_WAKEUP_RECV,			///< wake up state by data receiving
	DM_WAKEUP_SENSOR,		///< wake up state by sensor
	DM_WAKEUP_TIMER,		///< wakeup state by RTC timer
	DM_WAKEUP_BOOT,			///< wakeup state by POR or reboot
	DM_EXTERNAL_SENSOR,		///< wakeup state by external sensor
	DM_CHECK_RECV,			///< checking of receiving data before DPM sleep
	DM_CHECK_DEVICE,	///< checking device on exception state like 'No Ack'
	DM_FINISH_DEVICE,		///< finish state
	DM_NO_DPM_MODE			///< no DPM state
} DM_NOTI;

/**
 * @brief DM OTA_STATUS enum
 *
 * Enumeration structure for OTA status
 */
typedef enum {
	/// Status indicating no OTA job
	OTA_STAT_JOB_NONE = 0,
	/// Status indicating that the OTA job exists
	OTA_STAT_JOB_READY = 1,
	/// Status indicating that the user confirmed a prepared OTA job
	OTA_STAT_JOB_CONFIRMED = 2,
} OTA_STATUS;

/**
 * @brief The mode types of KA or RTC wakeup timer
 *
 * Enumeration structure for wakeup timer mode
 */
typedef enum {
	DPM_RTC_NORMAL_MODE = 0,
	DPM_RTC_ABNORMAL_MODE_1,
	DPM_RTC_ABNORMAL_MODE_2,
	DPM_RTC_ABNORMAL_MODE_3,
	DPM_RTC_ABNORMAL_MODE_4,
	DPM_RTC_ABNORMAL_MODE_5,
	DPM_RTC_ABNORMAL_MODE_6,
	DPM_RTC_ABNORMAL_MODE_7,
	DPM_RTC_ABNORMAL_MODE_8,
	DPM_RTC_ABNORMAL_MODE_9,
} APPTimerMode;

/**
 * @brief The wakeup timer interval depending APPTimerMode
 *
 * Wakeup timer interval by seconds
 */
#define DPM_RTC_ABNORMAL_INTERVAL_1ST		10
#define DPM_RTC_ABNORMAL_INTERVAL_2ND		30
#define DPM_RTC_ABNORMAL_INTERVAL_3RD		300

/**
 * @brief Message queue structure for DPM app thread.
 */
typedef struct _dpmAPPDataQ {
	int status;				///< status field
	void *userData;			///< payload field
} dpmAPPDataQ;

/**
 * @brief Callback function pointer for DPM app thread.
 */
typedef void (*dpmManagerFuncPtr)(UINT32 _pUserStruc, UINT32 _RTMdata,
		DM_NOTI _status);

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
 * @return void
 ****************************************************************************************
 */
void dpmAppThreadCreate(char *name_ptr, dpmManagerFuncPtr _entryFunction,
		UINT32 _pUserStruc, UINT32 _RTMdataSize, UINT32 _stackSize,
		UINT32 _priority, UINT32 _dpmPort, UINT32 _wakeUp_time,
		const char *_DNSAddr, const char *_2ndDNSAddr, UINT32 _SNTPCount);

/**
 ****************************************************************************************
 * @brief Send some message to this DPM app thread
 * @param[in] _dpmAppData the payload pointer to send this DPM app thread
 * @return void
 ****************************************************************************************
 */
void sendDPMAppMSGQ(dpmAPPDataQ *_dpmAppData);

/**
 ****************************************************************************************
 * @brief Retrieves the current state of OTA processing
 * @return OTA_STATUS Current OTA status
 ****************************************************************************************
 */
OTA_STATUS getOTAStat(void);

/**
 ****************************************************************************************
 * @brief Sets the current state for OTA processing
 * @param[in] _stat The value of one of the OTA_STATUS enums
 * @return void
 ****************************************************************************************
 */
void setOTAStat(OTA_STATUS _stat);

/**
 * @brief Sets the sleep mode and it's facotors on sleep mode 1/2/3(DPM)
 *
 * @param [in] _mode The value of one of the APPSleepMode enums
 * @param [in] _sec The value of wakeup timer interval by seconds
 * @param [in] _retention 0 or 1 only used in case of sleep mode 1/2
 *
 * @return void
 ****************************************************************************************
 */
void setSleepMode(APPSleepMode _mode, UINT32 _sec, UINT8 _retention);

/**
 ****************************************************************************************
 * @brief Gets the current sleep mode
 * @return APPSleepMode type value
 ****************************************************************************************
 */
APPSleepMode getSleepMode(void);

/**
 ****************************************************************************************
 * @brief Gets the current sleep timer
 * @return APPSleepMode type value
 ****************************************************************************************
 */

UINT32 getUsecTimer(void);

/**
 ****************************************************************************************
 * @brief Gets the current RTM Memory
 * @return APPSleepMode type value
 ****************************************************************************************
 */

UINT8 getSleepRTMMem(void);

/**
 ****************************************************************************************
 * @brief Sets the main thread of App to go sleep mode 3
 * @param[in] _resetTimer 0 or 1 whether resetting or not KA timer
 * @param[in] _mode APPTimerMode type wakeup timer mode
 * @return void
 ****************************************************************************************
 */
void goSleepAppDpmThread(UINT8 _resetTimer, APPTimerMode _mode);

/**
 ****************************************************************************************
 * @brief Sets the main thread of App to exit sleep mode 3
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
 * @brief Notify wakeup of the App's to set sleep mode 2 duration
 * @return long long
 ****************************************************************************************
 */
unsigned long long getAppSleep2Interval(int _defSeconds);

/**
 ****************************************************************************************
 * @brief Unregister DPM KA timer of App
 * @return pdPASS or pdFAIL
 ****************************************************************************************
 */
UINT8 killDPMAppKeepAliveTimer(void);

/**
 ****************************************************************************************
 * @brief Go into sleep 2 or reboot when exception happened such like connection failure
 * @param[in] _retryCount retried count for normal action
 * @return void
 ****************************************************************************************
 */
void goSleepAppOnException(int _retryCount);


#endif /* _APP_DPM_THREAD_H_ */

