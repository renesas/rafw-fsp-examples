/**
 ****************************************************************************************
 *
 * @file app_common_support.h
 *
 * @brief common supporting for app
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
#include "oal.h"
#include "sdk_defs.h"
#if !defined(_APP_COMMON_SUPPORT_H_)
#define _APP_COMMON_SUPPORT_H_

/** 
 * @brief data used to save persistent information on RTM for  platform 
 */
typedef struct _app_dpm_info_rtm {
    INT32 tid; ///< doorlock close user timer id
    INT32 interval; ///< doorlock close user timer timeout(sec) default 10sec.
    INT32 bindPort; ///< tcp client bind port number
    INT32 doorOpenMode; ///< Door open mode auto/manual
    INT8 doorOpen; ///< door state
    float temperature; ///< temperature
    float battery; ///< battery
    INT32 deltaDir; ///< delta direction
    INT32 FOTAStat; ///< FOTA status
    UINT8 FOTAUrl[256]; ///< Server S3 url for FOTA
    char ServrIp[16]; ///< server ip string of the  server
#if defined(__SUPPORT_AZURE_IOT__)
    UINT16 packetIdSub; ///< subscribe packet ID when connected first
    UINT8 qosSubCount; ///< subscribe item count when connected first
    UINT32 qosSubReturn[8]; ///< subscribe QOS policy when connected first
#endif // (__SUPPORT_AZURE_IOT__)
} app_dpm_info_rtm;

/**
 * @brief Sleep mode enum
 *
 * Enumeration structure for Sleep mode
 */
typedef enum {
    SLEEP_MODE_NONE = 0,
    SLEEP_MODE_1,
    SLEEP_MODE_2,
    SLEEP_MODE_3, /// DPM mode

} APPSleepMode;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ***************************************************************************************
 * @brief Getting IP Address from DNS Server
 * @param[in] Host Name to connect
 * @return IP Address String
 ****************************************************************************************
 */
char* app_common_get_ip_from_noraml_dns(char *hostname);

/**
 ***************************************************************************************
 * @brief Getting IP Address from Retention Memory
 * @param[in] Host Name to connect
 * @param[in] Retention Memory Region Name
 * @param[in] is_real_query - 1:query directly, 0:saved ip on RTM
 * @return IP Address String
 ****************************************************************************************
 */
char* app_common_get_ip_from_fast_dns_with_rtm(char *hostname, char *rtm_data_name, UINT8 is_real_query);

/**
 ***************************************************************************************
 * @brief Getting IP Address from NV RAM
 * @param[in] Host Name to connect
 * @param[in] is_real_query - 1:query directly, 0:saved ip on NVRAM
 * @return IP Address String
 ****************************************************************************************
 */
char* app_common_get_ip_from_fast_dns_with_nvram(char *hostname, UINT8 is_real_query);

/**
 ***************************************************************************************
 * @brief Getting Time from SNTP Server for normal or DPM(sleep3) mode.
 * @param[in] Access Count for SNTP Server
 * @param[in] Sleep Mode
 * @param[in] Wake Up Sleep Interval
 * @return void
 ****************************************************************************************
 */
void app_common_set_time_sync_from_normal_sntp(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval);

/**
 ***************************************************************************************
 * @brief Getting Time from NV Ram after SNTP Server firstly for sleep1 or sleep2
 * @param[in] Access Count for SNTP Server
 * @param[in] Sleep Mode
 * @param[in] Wake Up Sleep Interval
 * @param[in] Sleep duration for adding
 * @return return 1: success, other : error
 ****************************************************************************************
 */
void app_common_set_time_sync_from_fast_sntp_with_nvram(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval,
    unsigned long long added_sleep_dur);

/**
 ***************************************************************************************
 * @brief Saving Time in NR Ram for sleep1 or sleep2
 * @return return 1: success, other : error
 ****************************************************************************************
 */
int32_t app_common_save_time_sync_for_fast_sntp_with_nvram(void);

/**
 * @brief Start sleep mode 1 or 2
 *
 * @param [in] _sleepMode The value of one of the APPSleepMode enums
 * @param [in] _useRTM 0 or 1 only used in case of sleep mode 1/2
 * @param [in] _wakeupIntervalSec 0 or 1 only used in case of sleep mode 1/2
 * @param [in] _timeSave 0 or 1 only used in case of sleep mode 2 with RTM used
 *
 * @return 0 for success, 1 for failure
 *
 */
int32_t app_common_goto_sleep_1_or_2(APPSleepMode _sleepMode, UINT8 _useRTM, INT32 _wakeupIntervalSec, UINT8 _timeSave);

/** 
 * @brief some name of APIs changed depending SDK version 
 */
#if (SDK_MAJOR == 3) && (SDK_MINOR == 2) && (SDK_REVISION >= 5)
extern void fc80211_da16x_pri_pwr_down(int retention);
extern int dpm_mode_get_wakeup_source(void);
extern int dpm_mode_get_wakeup_type(void);
#define WAKEUP_TYPE_FN        dpm_mode_get_wakeup_type
#else
extern void fc80211_da16x_pri_pwr_down(unsigned char retention);
extern int dpm_get_wakeup_source(void);
extern int RM_PMGR_W_dpm_wakeup_type_get(int do_print);
#define WAKEUP_TYPE_FN        RM_PMGR_W_dpm_wakeup_type_get
#endif

#endif // _APP_COMMON_SUPPORT_H_

/* EOF */
