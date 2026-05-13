/**
 ****************************************************************************************
 *
 * @file app_dpm_thread.c
 *
 * @brief Define the DPM thread running on Apps module
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
#include "bsp_api.h"
#include "projdefs.h"
#include <stdio.h>
#include "oal.h"
#include "net_common.h"
#include "net_sntp_client.h"
#include "net_dns_client.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "iface_defs.h"
#include "nvedit.h"
#include "sys_feature.h"
#include "common_def.h"
#include "common_utils.h"

#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#if CFG_PMGR
#include "rm_pmgr_w_dpm_internal.h"
#endif
#include "common_utils.h"

#include "common_def.h"
#include "user_dpm.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#include "r_pm_if.h"
#endif

#include "app_common_util.h"

#include "iface_defs.h"
#include "nvedit.h"
#include "sys_feature.h"

#include "app_azure_user_conf.h"

#include "upss.h"
#include "app_dpm_thread.h" 
#include "app_dpm_interface.h"

#define DPM_APP_DEFAULT_STACK_SIZE          (1024*8)/sizeof(StackType_t)
#define DPM_APP_DEFAULT_THREAD_PRIORITY     (tskIDLE_PRIORITY + 1)

#define CreateMSGQ_CMD                      4

#define DM_NEED_INIT                        ((UINT8)0x01)
#define DM_NEED_CONNECT                     ((UINT8)0x02)
#define DM_RECV_WAKEUP                      ((UINT8)0x04)
#define DM_SENSOR_WAKEUP                    ((UINT8)0x08)
#define DM_RTC_WAKEUP                       ((UINT8)0x10)
#define DM_BOOT_WAKEUP                      ((UINT8)0x20)
#define DM_NEED_CHECK_DEVICE                ((UINT8)0x80)

// Note: Select after checking each memory region

#define MIN_RTC_WAKEUP_INTERVAL             3 //  (seconds) minimum value
#define MIN_RTC_WAKEUP_SUBSTRACT_FOR_KA     20 // (seconds) early wakeup substraction for KA

typedef INT32 (*appFuncPtr)(UINT32);

QueueHandle_t appMsgQ = NULL;

char *DPMSleepName = NULL;
char wakeupException[16] = { 0, };

INT32 externalEvent = 0;

extern UINT8 user_get_wakeup_pin_num(void);

extern void RTC_CLEAR_EXT_SIGNAL(void);
extern void RTC_READY_POWER_DOWN(int clear);

static UINT32 dpm_keepalive_timer_unregister(dpmAppThreadInfo *_threadInfo);
static UINT32 dpm_keepalive_timer_register(dpmAppThreadInfo *_threadInfo,
        UINT8 _mode);
static INT32 dpm_GotoSleep(dpmAppThreadInfo *_threadInfo);
static void APPPowerOFF(char *_name);
static INT32 recvDPMAppMSGQ(void);

static APPSleepMode appSleepMode = SLEEP_MODE_3;
static UINT32 appUsecTimer = 0;
static UINT8 sleepRTMMem = 0;

static OTA_STATUS jobStartFlag = OTA_STAT_JOB_NONE;

OTA_STATUS getOTAStat(void) 
{
    return (OTA_STATUS) jobStartFlag;
}

void setOTAStat(OTA_STATUS _stat) 
{
    if (jobStartFlag != _stat)
    {
        jobStartFlag = _stat;
    }

    if (jobStartFlag > OTA_STAT_JOB_CONFIRMED)
    {
        jobStartFlag = OTA_STAT_JOB_NONE;
        SYS_ASSERT(0);
    }
}

static void set_sntp_with_sleepMode(dpmAppThreadInfo *appInfo,
        APPSleepMode sleepMode, UINT32 wakeupSrc, INT32 wakeupType) 
{
    /* check time only when no DPM or boot up */
    if (sleepMode == SLEEP_MODE_1)
    {
        if (RM_PMGR_W_dpm_is_enabled())
        {
            RM_PMGR_W_dpm_disable();
        }

        if (SLEEP_1_USE_RTM != 0)
        {
            if (wakeupSrc != WAKEUP_EXT_SIG_WITH_RETENTION)
            {
                app_print_elapse_time_ms(
                        "[%s:%d] sleep mode 1: setting time(rtm used)",
                        __func__, __LINE__);
                app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount,
                        (UINT8) SLEEP_MODE_1, appInfo->wakeUpTime);
            }
        } else
        {
            app_print_elapse_time_ms(
                    "[%s:%d] sleep mode 1: setting time(rtm not used)",
                    __func__, __LINE__);
            app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount,
                    (UINT8) SLEEP_MODE_1, appInfo->wakeUpTime);
        }
    } else if (sleepMode == SLEEP_MODE_2)
    {
        if (RM_PMGR_W_dpm_is_enabled())
        {
            	RM_PMGR_W_dpm_disable();
        }

        if (SLEEP_2_USE_RTM != 0)
        {
            if (wakeupSrc != WAKEUP_COUNTER_WITH_RETENTION)
            {
                printf("[%s:%d] sleep mode 2: setting time(rtm used)",
                        __func__, __LINE__);
                app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount,
                        (UINT8) SLEEP_MODE_2, appInfo->wakeUpTime);
            }
        } else
        {
            printf("[%s:%d] sleep mode 2: setting time(rtm not used)",
                    __func__, __LINE__);
            app_common_set_time_sync_from_fast_sntp_with_nvram(
                    appInfo->sntpCount, (UINT8) SLEEP_MODE_2,
                    appInfo->wakeUpTime, (unsigned long long) getUsecTimer());
        }

#if defined(__SUPPORT_UPSS__)
        APP_Power_Save_Start();
#endif

    } else // default: sleep mode3 (DPM)
    {
        if (wakeupType == DPM_UNKNOWN_WAKEUP)
        {
            app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount,
                    (UINT8) SLEEP_MODE_3, appInfo->wakeUpTime);
        }

#if defined(__SUPPORT_UPSS__)
        APP_Power_Save_Start();
#endif
    }

}

#if defined(__COINCELL_FEATURE__)
#define DC_DC_STR_REG                   (0x50091044)
#define DC_DC_STR_VALUE                 (0x2c031710)
#define DC_DC_STR_READ_SET_VALUE        (0x0c031710)
#endif

UINT8 checkDPMModeInternal(dpmAppThreadInfo *_threadInfo) 
{
    UINT8 flag = 0x00;

    flag |= DM_NEED_INIT;
    flag |= DM_NEED_CONNECT;

    UINT32 wakeupMode = WAKEUP_SOURCE_FN();
    INT32 wakeup_type = WAKEUP_TYPE_FN(1);

    InternalRTM *data = (InternalRTM*) _threadInfo->internalRTM;

    APRINTF_I("\n============================\n");
    APRINTF("Wake mode [0x%02x] , type [%d] \n", wakeupMode, wakeup_type);
    if (wakeupMode == WAKEUP_SOURCE_EXT_SIGNAL
            || wakeupMode == WAKEUP_EXT_SIG_WITH_RETENTION)
            //|| wakeupMode == WAKEUP_SENSOR_EXT_SIGNAL) //RTC_WAKE_UP pin via button1
    {
        APRINTF_I("wake up from sensor or external signal\n");
        flag |= DM_SENSOR_WAKEUP;
        //clear timer mode
        if (data)
        {
            data->mode = DPM_RTC_NORMAL_MODE;
        }

#if defined(__COINCELL_FEATURE__)
        PRINTF("[%s:%d] in-rush readReg(0x%08x) recovery => 0x%08x\n", 
            __func__, __LINE__, DC_DC_STR_REG, APP_Inrush_Reg_Read());
#endif	

        set_sntp_with_sleepMode(_threadInfo, getSleepMode(), wakeupMode,
                wakeup_type);
    }
    /*
     #if defined(__SUPPORT_UPSS__)
     else if (wakeupMode == WAKEUP_EXT_SIG_WITH_RETENTION && (wakeup_type == DPM_RTCTIME_WAKEUP || wakeup_type == DPM_USER_WAKEUP)) //RTC_WAKE_UP pin via button1
     #else
     else if (wakeupMode == WAKEUP_EXT_SIG_WITH_RETENTION && wakeup_type == DPM_RTCTIME_WAKEUP) //RTC_WAKE_UP pin via button1
     #endif
     {
     flag |= DM_SENSOR_WAKEUP;
     //clear timer mode
     if (data)
     {
     data->mode = DPM_RTC_NORMAL_MODE;
     }
     }
     */
    else
    {
        switch (wakeup_type)
        {
        case DPM_UNKNOWN_WAKEUP:
        {
            INT32 readyDNS = 0;
            APRINTF_I("Wake up .... Boot \n");
            flag |= DM_BOOT_WAKEUP;

#if defined(__COINCELL_FEATURE__)				
                PRINTF("[%s:%d] in-rush readReg(0x%08x) recovery => 0x%08x\n", __func__, __LINE__, DC_DC_STR_REG, APP_Inrush_Reg_Read());
#endif	
            if (wakeupMode == WAKEUP_RESET || wakeupMode == WAKEUP_SOURCE_POR
                    || wakeupMode == WAKEUP_WATCHDOG)
            {
                //set & enable DNS
                APRINTF("DNS_Address [%s] [%s]\n", _threadInfo->DNSAddr,
                        _threadInfo->secDNSAddr);
                if (set_dns_addr(0, _threadInfo->DNSAddr) != pdPASS)
                {
                    APRINTF("set_dns_addr() failed\n");

                } else
                {
                    readyDNS = 1;
                    APRINTF("set_dns_addr() OK: \"%s\"\n",
                            _threadInfo->DNSAddr);
                }

                //set & enable 2 DNS
                if (set_dns_addr_2nd(0, _threadInfo->secDNSAddr) != pdPASS)
                {
                    APRINTF("set_dns_addr_2nd() failed\n");
                } else
                {
                    readyDNS = 1;
                    APRINTF("set_dns_addr_2nd() OK: \"%s\"\n",
                            _threadInfo->secDNSAddr);
                }

                while (readyDNS == 0)
                {

                    APRINTF_E("=================================================================\n");
                    APRINTF_E("DNS SET fail...Please check netwotk state or ap cable\n");
                    APRINTF_E("After checking state , please reboot this device and try again\n");
                    APRINTF_E("==================================================================\n");
                    OAL_MSLEEP(1000);
                }
            }

            set_sntp_with_sleepMode(_threadInfo, getSleepMode(), wakeupMode,
                    wakeup_type);

        }
            break;
        case DPM_RTCTIME_WAKEUP:
        {
            APRINTF_I("Wake up .... RTC timer \n");
            flag |= DM_RTC_WAKEUP;
        }
            break;
        case DPM_PACKET_WAKEUP:
        {
            APRINTF_I("Wake up .... Recv Packet \n");
            flag |= DM_RECV_WAKEUP;
        }
            break;
        case DPM_USER_WAKEUP:
        {
#if defined(__SUPPORT_UPSS__)
                APRINTF_I("Wake up .... User Request (PTIM RTC)\n");
                flag |= DM_RTC_WAKEUP;
#else
            APRINTF_I("Wake up .... User Request \n");
            flag |= DM_SENSOR_WAKEUP;
#endif
        }
            break;
        case DPM_NOACK_WAKEUP:
        {
            APRINTF_I("Wake up .... No Ack \n");
            flag |= DM_NEED_CHECK_DEVICE;
            strcpy(wakeupException, "No Ack");

        }
            break;
        case DPM_DEAUTH_WAKEUP:
        {
            APRINTF_I("Wake up .... De Auth \n");
            strcpy(wakeupException, "De Auth");
            flag |= DM_NEED_CHECK_DEVICE;
        }
            break;
        case DPM_TIM_ERR_WAKEUP:
        {
            APRINTF_I("Wake up .... Tim err \n");
            flag |= DM_NEED_CHECK_DEVICE;
            strcpy(wakeupException, "Tim err");
        }
            break;
        default:
        {
            APRINTF_I("Wake up .... Unknown \n");
            strcpy(wakeupException, "Unknown");
            SYS_ASSERT(0);
        }
            break;
        }

#if defined(__SUPPORT_UPSS__)
        if (wakeup_type != DPM_RTCTIME_WAKEUP && wakeup_type != DPM_USER_WAKEUP)
#else
        if (wakeup_type != DPM_RTCTIME_WAKEUP)
#endif
                {
            //clear time mode
            if (data)
            {
                data->mode = DPM_RTC_NORMAL_MODE;
            }
        }

    }

    APRINTF_I("============================\n\n");
    return flag;

}

INT32 dpm_info_release(char *_name)
{
    UINT32 status = ER_SUCCESS;

    status = RM_PMGR_W_user_rtm_free(_name);
    if ((status != ER_NOT_FOUND) && (status != ER_SUCCESS))
    {
        APRINTF_E("[%s] failed to release dpm info in rtm(0x%02x)", __func__, status);
    }

    return status;
}

INT32 dpm_info_get(char *_name, void **_data, UINT32 _dataSize, UINT8 _share) 
{
    UINT32 status = ER_SUCCESS;
    const ULONG wait_option = 100;
    UINT32 len = 0;

    if (!_share) //internal
    {
        len = RM_PMGR_W_user_rtm_get(_name, (unsigned char**)_data);
        if (len == 0)
        {
            status = RM_PMGR_W_user_rtm_pool_alloc(_name, (void**)_data, _dataSize, wait_option);
            if (status)
            {
                APRINTF_E("[%s] failed to allocate dpm info in rtm(0x%02x)\n", __func__, status);
                vTaskDelay(1);
                return pdFAIL;
            }
            memset(*_data, 0x00, _dataSize);
        } else if (len != _dataSize)
        {
            APRINTF("[%s] invalid size(%ld)\n", __func__, len);
            vTaskDelay(1);
            return pdFAIL;
        }
    } else //external
    {
        app_dpm_info_get(_name, _data);
    }

    if (*_data != NULL)
    {
        APRINTF("\"%s\" got DPM's RTM data size (%ld)\n", _name, _dataSize);
    }

    return pdPASS;
}

static void APPPowerOFF(char *_name)
{
    INT32 reg_value = 0;
    char *sleepName = NULL;

    if (_name == NULL)
    {
        if (DPMSleepName != NULL)
        {
            sleepName = DPMSleepName;
        } else
        {
            APRINTF_E("[APPPowerOFF] No name error\n");
            SYS_ASSERT(0);
        }
    } else
    {
        sleepName = _name;
    }

    APRINTF_E("\n... \" goto DPM SLEEP...");

    unsigned long long time_old;
    time_old = RTC_GET_COUNTER();
    RTC_CLEAR_EXT_SIGNAL();

    while (RTC_GET_COUNTER() < (time_old + 2))
        ;

    reg_value = (*(INT32*) (0x50091028));
    if (reg_value != 0)
    {
        APRINTF("\n reg_value : %x\n", reg_value);
        RTC_CLEAR_EXT_SIGNAL();
        vTaskDelay(1);
    }
    dpm_app_sleep_ready_set(sleepName);

}

static INT32 dpm_GotoSleep(dpmAppThreadInfo *_threadInfo)
{

    APPPowerOFF(_threadInfo->DPMRegeditName);
    return ER_SUCCESS;

}

static INT32 dpm_wakeup_init(dpmAppThreadInfo *_threadInfo)
{
    char *regName = _threadInfo->DPMRegeditName;

    RM_PMGR_W_dpm_wakeup_done(regName);

    /*
     if (_threadInfo->DpmWakeUp)
     {
     wakeup_type = RM_PMGR_W_dpm_wakeup_type_get(0);

     //org
     //		if (wakeup_type != DPM_RTCTIME_WAKEUP)
     //azuredpmwork[[: defense for exception (DPM_NOACK_WAKEUP, DPM_DEAUTH_WAKEUP, DPM_TIM_ERR_WAKEUP)
     //if (wakeup_type == DPM_PACKET_WAKEUP)
     //]]
     //upsswork[[:
     if (wakeup_type != DPM_RTCTIME_WAKEUP && wakeup_type != DPM_USER_WAKEUP)
     //]]
     {
     dpm_keepalive_timer_unregister(_threadInfo);
     APRINTF("wake-up timer unregistered\n");
     }
     }
     */

    return ER_SUCCESS;
}


static void initDPMThread(dpmAppThreadInfo *_threadInfo)
{
    UINT32 status = ER_SUCCESS;

    char *regName = _threadInfo->DPMRegeditName;
    UINT32 dpmPort = _threadInfo->dpmPort;

    //save the passed pointer to global
    pAppDpmThread = (dpmAppThreadInfo*) _threadInfo;

    APRINTF_I("[initDPMThread] =============================\n");

    if (_threadInfo->DpmMode) //in case of dpm mode
    {
        app_socket_set_port_n_filter(AZURE_MQTT_PORT);
    }

}

static void CreateDPMAppMSGQ(void)
{
    appMsgQ = xQueueCreate(CreateMSGQ_CMD, sizeof(dpmAPPDataQ));

    if (appMsgQ == NULL)
    {
        APRINTF("[%s] Failed to create queue \"appMsgQ\" \n", __func__);
    }
}

static INT32 recvDPMAppMSGQ(void)
{
    int status;
    dpmAPPDataQ dpmAppData;

    status = xQueueReceive(appMsgQ, &dpmAppData, portMAX_DELAY);
    if (status != pdPASS)
    {
        APRINTF("[%s] xQueueReceive error !!! (%d)\n", __func__, status);
        return pdFAIL;
    }

    return pdPASS;
}

void sendDPMAppMSGQ(dpmAPPDataQ *_damAppData)
{
    int status = 0;

    APRINTF_I("\nsendDPMAppMSGQ... \n\n");

    status = xQueueSend(appMsgQ, _damAppData, portMAX_DELAY);
    if (status != pdPASS)
    {
        APRINTF("[%s] xQueueSend error !!! (%d)\n", __func__, status);
    }
}

static void dpmAPPManager(void *_threadInfo)
{
    UINT8 statusFlag;
    dpmAppThreadInfo *threadInfo = (dpmAppThreadInfo*) _threadInfo;
    UINT32 userData = threadInfo->pAppData;
    UINT32 rtmData = threadInfo->externalRTM;
    dpmManagerFuncPtr callBackb = (dpmManagerFuncPtr) threadInfo->entryInput;

    recvDPMAppMSGQ();

    statusFlag = checkDPMModeInternal(threadInfo);
    APRINTF_I("[dpmAPPManager] statusFlag : %x\n", statusFlag);

    DPMSleepName = threadInfo->DPMRegeditName;

    // check DPM work or not
    if (statusFlag & DM_NEED_INIT)
    {
        APRINTF_I("[dpmAPPManager] DM_NEED_INIT\n");

        callBackb(userData, rtmData, DM_INIT);
    }

    if (statusFlag & DM_NEED_CONNECT)
    {
        if (statusFlag & DM_NEED_CHECK_DEVICE)
        {
            APRINTF_I("[%s] exception: DM_NEED_CONNECTION bypassed\n", __func__);
        } else
        {
            APRINTF_I("[%s] DM_NEED_CONNECTION\n", __func__);
            callBackb(userData, rtmData, DM_NEED_CONNECTION);
        }
    }

    // need DPM init confrrm ;
    if (statusFlag & DM_RECV_WAKEUP)
    {
        APRINTF_I("[%s] DM_RECV_WAKEUP\n", __func__);
        callBackb(userData, rtmData, DM_WAKEUP_RECV);
    }

    if (statusFlag & DM_SENSOR_WAKEUP)
    {
        APRINTF_I("[%s] DM_SENSOR_WAKEUP\n", __func__);
        callBackb(userData, rtmData, DM_WAKEUP_SENSOR);
    }

    if (statusFlag & DM_RTC_WAKEUP)
    {
        APRINTF_I("[%s] DM_RTC_WAKEUP\n", __func__);
        callBackb(userData, rtmData, DM_WAKEUP_TIMER);
    }

    if (statusFlag & DM_BOOT_WAKEUP)
    {
        APRINTF_I("[%s] DM_BOOT_WAKEUP\n", __func__);
        callBackb(userData, rtmData, DM_WAKEUP_BOOT);
    }

    if (statusFlag & DM_NEED_CHECK_DEVICE)
    {
        if (strlen(wakeupException) == 0)
        {
            APRINTF_I("[%s] Unknown\n", __func__);
        } else
        {
            APRINTF_I("[%s] Exception : ", __func__);
            APRINTF_I("%s\n", wakeupException);
        }
        callBackb(userData, rtmData, DM_CHECK_DEVICE);
    }

    if (appSleepMode == SLEEP_MODE_1)
    {

    } else if (appSleepMode == SLEEP_MODE_2)
    {

    } else
    {
        APRINTF_I("\nSleep mode 3: KA timer interval(=%d sec)\n\n", threadInfo->wakeUpTime);

        //while (1)
        {
            if (RM_PMGR_W_dpm_sleep_is_started())
            {
                //OAL_MSLEEP(100);	// 10 ticks : 100 msec
                //continue;
                //break;
            }

            if (externalEvent != 0)
            {
                APRINTF_I("\nHappend external event\n\n");

                /* Clear flag to protect goto DPM sleep mode */
                RM_PMGR_W_dpm_sleep_ready_clear(threadInfo->DPMRegeditName);
                callBackb(userData, rtmData, DM_EXTERNAL_SENSOR);
                //clear flag
                externalEvent = 0;
            }

            if (threadInfo->DpmMode == 1)
            {
                callBackb(userData, rtmData, DM_FINISH_DEVICE);
            } else
            {
                // retry data;
                callBackb(userData, rtmData, DM_NO_DPM_MODE);
            }
        }
    }

    vTaskDelete(NULL);

}

//internal definitions[[:
/*
 #define	DPM_ABNORM_ACT_1	1	// WIFI Connect fail
 #define	DPM_ABNORM_ACT_2	2	// DHCP Renew fail
 #define	DPM_ABNORM_ACT_3	3	// ARP response fail
 #define	DPM_ABNORM_ACT_4	4	// DPM Power down fail (30 Sec)
 #define	DPM_ABNORM_ACT_5	5	// No data for 5 seconds and DPM is ready
 #define	DPM_ABNORM_ACT_6	6	// DPM application fail
 */
//]]
extern UCHAR get_last_abnormal_act(void);
static void app_chk_sleep_monitor_thread(void *_threadInfo)
{
    UINT8 act;
    UINT8 sleepFlag = 0;
    dpmAppThreadInfo *threadInfo = (dpmAppThreadInfo*) _threadInfo;
    InternalRTM *data = NULL;

    PRINTF("==== app_chk_sleep_monitor_thread() started ====\r\n");

    if (threadInfo)
    {
        data = (InternalRTM*) threadInfo->internalRTM;
    }

    while (data)
    {
        vTaskDelay(200); // 2 seconds

        act = get_last_abnormal_act();
        if (act == 6) // DPM application fail
                {
            if (!sleepFlag && getOTAStat() != OTA_STAT_JOB_CONFIRMED)
            {
                UINT8 mode;
                INT32 interval;
                switch (data->mode)
                {
                case DPM_RTC_NORMAL_MODE:
                case DPM_RTC_ABNORMAL_MODE_1:
                case DPM_RTC_ABNORMAL_MODE_2:
                {
                    mode = data->mode + 1;
                    interval = DPM_RTC_ABNORMAL_INTERVAL_1ST;
                }
                    break;
                case DPM_RTC_ABNORMAL_MODE_3:
                case DPM_RTC_ABNORMAL_MODE_4:
                case DPM_RTC_ABNORMAL_MODE_5:
                {
                    mode = data->mode + 1;
                    interval = DPM_RTC_ABNORMAL_INTERVAL_2ND;
                }
                    break;
                case DPM_RTC_ABNORMAL_MODE_6:
                case DPM_RTC_ABNORMAL_MODE_7:
                case DPM_RTC_ABNORMAL_MODE_8:
                {
                    mode = data->mode + 1;
                    interval = threadInfo->wakeUpTime; //normal interval
                }
                    break;
                case DPM_RTC_ABNORMAL_MODE_9:
                default:
                {
                    mode = DPM_RTC_ABNORMAL_MODE_1;
                    interval = DPM_RTC_ABNORMAL_INTERVAL_1ST;
                }
                    break;
                }

                threadInfo->wakeUpTime = interval;

                APRINTF_E("[%s] due to abnormal act(=%d), App can't enter DPM sleep...force to sleep then wakeup after RTC %d secs\n", __func__, act, threadInfo->wakeUpTime);

                //RTC_READY_POWER_DOWN(0);
                dpm_keepalive_timer_register(threadInfo, mode);
                //dpm_GotoSleep(threadInfo);
                //dpm_debug
                RM_PMGR_W_dpm_sleep_ready_set(threadInfo->DPMRegeditName);
                //dpm_debug
                // app_dpm_put_to_sleep();

                sleepFlag = 1;
                continue;
            }

            if (sleepFlag)
            {
                APRINTF_S("[%s] already set dpm sleep for \"%s\", but another App sleep fail (act=%d)...\n", __func__, threadInfo->DPMRegeditName, act);
            }
        } else
        {
            if (act)
            {
                APRINTF_E("[%s] abnormal act = %d\n", __func__, act);
            }
        }
    };

    vTaskDelete(NULL);
}

void dpmAppThreadCreate(char *name_ptr, dpmManagerFuncPtr _entryFunction,
        UINT32 _pUserStruc, UINT32 _RTMdataSize, UINT32 _stackSize,
        UINT32 _priority, UINT32 _dpmPort, UINT32 _wakeUp_time,
        const char *_DNSAddr, const char *_2ndDNSAddr, UINT32 _SNTPCount)
{
    UINT32 status = 0;
    UINT32 stringSize = 0;
    char *stack_start;
    TaskHandle_t thread_ptr;
    char result[64] = { 0, };
    dpmAppThreadInfo *threadInfo = NULL;

    APRINTF_I("\n[New Thread Start for DPM APP]\n\n");

    if (_stackSize == 0)
    {
        _stackSize = DPM_APP_DEFAULT_STACK_SIZE;

    }

    if (_priority == 0)
    {
        _priority = DPM_APP_DEFAULT_THREAD_PRIORITY;
    }

    if (name_ptr == NULL)
    {
        SYS_ASSERT(0);
    }

    if (_stackSize == 0)
    {
        SYS_ASSERT(0);
    }

    threadInfo = (dpmAppThreadInfo*) APP_DPM_MALLOC(sizeof(dpmAppThreadInfo));

    // make thread name 
    //sprintf(result, "DM_T_%s", name_ptr);
    sprintf(result, "RM_%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->threadName = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->threadName, 0, stringSize + 1);
    strcpy(threadInfo->threadName, result);

    memset(result, 0, 64);

    // make Reg name 
    sprintf(result, "%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->DPMRegeditName = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->DPMRegeditName, 0, stringSize + 1);
    strcpy(threadInfo->DPMRegeditName, result);

    memset(result, 0, 64);

    // make internalRTM name 
    //sprintf(result, "DM_IR_%s", name_ptr);
    sprintf(result, "DIR%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->internalRTMName = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->internalRTMName, 0, stringSize + 1);
    strcpy(threadInfo->internalRTMName, result);

    memset(result, 0, 64);

    // make externalRTM name 
    //sprintf(result, "DM_ER_%s", name_ptr);
    sprintf(result, "DER%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->externalRTMName = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->externalRTMName, 0, stringSize + 1);
    strcpy(threadInfo->externalRTMName, result);

    threadInfo->entryInput = (UINT32) _entryFunction;
    threadInfo->pAppData = _pUserStruc;
    //threadInfo->pRTMData = _RTMdata;
    threadInfo->dpmPort = _dpmPort;
    if (appUsecTimer == 0)
    {
        threadInfo->wakeUpTime = _wakeUp_time;
    } else
    {
        threadInfo->wakeUpTime = appUsecTimer;
    }


    threadInfo->DpmMode = RM_PMGR_W_dpm_is_enabled();
    threadInfo->DpmWakeUp = RM_PMGR_W_dpm_is_wakeup();

    threadInfo->sntpCount = _SNTPCount;

    // set DNS addrress
    stringSize = strlen((const char*) _DNSAddr);
    threadInfo->DNSAddr = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->DNSAddr, 0, stringSize + 1);
    strcpy(threadInfo->DNSAddr, _DNSAddr);

    stringSize = strlen((const char*) _2ndDNSAddr);
    threadInfo->secDNSAddr = (char*) APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->secDNSAddr, 0, stringSize + 1);
    strcpy(threadInfo->secDNSAddr, _2ndDNSAddr);

    /*
     APRINTF_I("name_ptr = %s\n", name_ptr);
     APRINTF_I("Thread name  = [%s]  \n",threadInfo->threadName);
     APRINTF_I("Reg name  = [%s]  \n",threadInfo->DPMRegeditName);
     APRINTF_I("Internal RTM name  = [%s]  \n",threadInfo->internalRTMName);
     APRINTF_I("External RTM name = [%s]  \n",threadInfo->externalRTMName);
     APRINTF_I("1st DNS addr = [%s] \n", threadInfo->DNSAddr);
     APRINTF_I("2nd DNS addr = [%s] \n", threadInfo->secDNSAddr);
     */

    APRINTF_I("\n[New Thread(\"%s\") Start for DPM APP]\n\n", threadInfo->threadName);

    APRINTF_I("\n=============================================\n");

    if (threadInfo->DpmMode == 1)
    {
        APRINTF_S("\n[%s] DPM mode checked...(%d)\n", __func__, appSleepMode);
        dpm_info_get(threadInfo->internalRTMName,
            (void**) (&(threadInfo->internalRTM)), sizeof(InternalRTM), 0);
        dpm_info_get(threadInfo->externalRTMName,
            (void**) (&threadInfo->externalRTM), _RTMdataSize, 1);

        if (threadInfo->DpmWakeUp == 1)
        {
            APRINTF_S("Wake up from DPM Sleep \n\n");
        } else
        {
            APRINTF_S("Boot mode \n\n");
        }

    } else
    {
        char *pCRTM = (char*) APP_DPM_MALLOC(_RTMdataSize);
        if (pCRTM)
        {
            memset(pCRTM, 0, _RTMdataSize);
        } else
        {
            SYS_ASSERT(0);
        }
        threadInfo->internalRTM = (InternalRTM*) APP_DPM_MALLOC(
            sizeof(InternalRTM));
        if (threadInfo->internalRTM)
        {
            memset(threadInfo->internalRTM, 0, sizeof(InternalRTM));
        } else
        {
            SYS_ASSERT(0);
        }
        threadInfo->externalRTM = (UINT32) pCRTM;
        APRINTF_S("\n[%s] No DPM mode checked...\n", __func__);
    }
    APRINTF_I("=============================================\n");

    //dpmSetCurWakeupTimeout(threadInfo->wakeUpTime);

    //RTC wakeup for KA: check the validation
    if (threadInfo->wakeUpTime == 0)
        threadInfo->wakeUpTime = AZURE_CONFIG_KEEP_INTERVAL_DEF;

    app_set_ka_value(threadInfo->wakeUpTime);

    initDPMThread(threadInfo);

    if (threadInfo->DpmMode)
    {
        TaskHandle_t chkSleepMonitor = NULL;
        status = xTaskCreate(app_chk_sleep_monitor_thread, "aChkSleep",
                1024 / sizeof(StackType_t), (void*) threadInfo,
                tskIDLE_PRIORITY + 2, &chkSleepMonitor);
        if (status != pdPASS)
        {
            APRINTF("[%s] Failed to create app sleep monitor thread\r\n",
                    __func__);
            SYS_ASSERT(0);
        }
    }

    //queue NULL bug fixed
    CreateDPMAppMSGQ();

    status = xTaskCreate(dpmAPPManager, threadInfo->threadName, _stackSize,
            (void*) threadInfo, _priority, &thread_ptr);
    if (status != pdPASS)
    {
        APRINTF("[%s] Failed to create dpmAPPManager thread\r\n", __func__);
        SYS_ASSERT(0);
    }

    //org code
    //CreateDPMAppMSGQ();

}

void setSleepMode(APPSleepMode _mode, UINT32 _sec, UINT8 _retention)
{

    appSleepMode = _mode;
    appUsecTimer = _sec;
    sleepRTMMem = _retention;

    APRINTF_S("\n===========================================\n\n");
    if (_mode == 0)
    {
        APRINTF("[ Compiled Set Sleep mode 3 with DPM ] \n\n");
    } else
    {
        APRINTF("[ Set Sleep mode : %d , RTC time : %d ,useRTM : %d ] \n\n",
                _mode, appUsecTimer, sleepRTMMem);
    }

    APRINTF_S("============================================\n\n");
}

APPSleepMode getSleepMode(void)
{
    return appSleepMode;
}

UINT32 getUsecTimer(void)
{
    return appUsecTimer;
}

UINT8 getSleepRTMMem(void)
{
    return sleepRTMMem;
}

void informWakeupAppDpmThread(void)
{
    if (RM_PMGR_W_dpm_is_enabled())
    {
        if (pAppDpmThread)
        {
            dpm_wakeup_init(pAppDpmThread);
        } else
        {
            RM_PMGR_W_dpm_wakeup_done(APP_AZURE_TWIN);
        }
    }

}

static UINT32 dpm_keepalive_timer_unregister(dpmAppThreadInfo *_threadInfo)
{
    InternalRTM *dataInternal = NULL;
    app_dpm_info_rtm *dataExternal = NULL;
    int ret = 0;

    if (_threadInfo) //DPM App Thread used
    {
        dataInternal = (InternalRTM*) _threadInfo->internalRTM;
        if (dataInternal && dataInternal->tid)
        {
            ret = RM_PMGR_W_dpm_timer_delete_by_tid(dataInternal->tid);
            dataInternal->tid = APP_DPM_TIMER_ID;
        }
    } else //DPM App Thread not used
    {
        dataExternal = app_dpm_info_get(NULL);
        if (dataExternal && dataExternal->tid)
        {
            ret = RM_PMGR_W_dpm_timer_delete_by_tid(dataExternal->tid);
            dataExternal->tid = APP_DPM_TIMER_ID;
        }
    }

    if (ret < 0 || ret == DPM_TIMER_ERR)
    {
        return ER_NOT_SUCCESSFUL;
    } else
    {
        return ER_SUCCESS;
    }
}

/* _mode: 0 - normal mode, 1 ~ 9 - abnormal mode */
static UINT32 dpm_keepalive_timer_register(dpmAppThreadInfo *_threadInfo,
        APPTimerMode _mode)
{
    UINT32 status = ER_SUCCESS;
    UINT32 interval;
    INT32 tmpTID;
    INT32 tmpInterval;
    APPTimerMode tmpMode;
    InternalRTM *dataInternal = NULL;
    app_dpm_info_rtm *dataExternal = NULL;
    char dpmThreadName[20] = { 0, };
    INT32 lenDpmThreadName = 0;
//azure [[:
    char dpmTimerJobName[20] = {0, };
    char dpmTimerName[20] = {0, };
//azure]]

    if (_threadInfo == NULL)
    {
        interval = app_get_ka_value();
        dataExternal = app_dpm_info_get(NULL);
        lenDpmThreadName = strlen(APP_AZURE_TWIN);
        memcpy(dpmThreadName, APP_AZURE_TWIN, strlen(APP_AZURE_TWIN));
    } else
    {
        interval = _threadInfo->wakeUpTime;
        dataInternal = (InternalRTM*) _threadInfo->internalRTM;
        lenDpmThreadName = strlen(_threadInfo->DPMRegeditName);
        memcpy(dpmThreadName, _threadInfo->DPMRegeditName, lenDpmThreadName);
    }

    configASSERT(lenDpmThreadName < 12);
    configASSERT(lenDpmThreadName != 0);

    if (dataInternal != NULL)
    {
        if (dataInternal->tid != APP_DPM_TIMER_ID)
        {
            //unregister previous timer
            status = dpm_keepalive_timer_unregister(_threadInfo);
            if (status)
            {
                APRINTF_E("[%s] failed to unreigster timer(0x%02x)\n", __func__, status);
                vTaskDelay(1);
                return status;
            }
        }
        tmpTID = APP_DPM_TIMER_ID;
    } else if (dataExternal != NULL)
    {
        if (dataExternal->tid != APP_DPM_TIMER_ID)
        {
            //unregister previous timer
            status = dpm_keepalive_timer_unregister(NULL);
            if (status)
            {
                APRINTF_E("[%s] failed to unreigster timer(0x%02x)\n", __func__, status);
                vTaskDelay(1);
                return status;
            }
        }
        tmpTID = APP_DPM_TIMER_ID;
    } else
    {
        APRINTF_E("[%s] failed to get data info from rtm\n", __func__);
        vTaskDelay(1);
        status = ER_NOT_SUCCESSFUL;
        return status;
    }

    tmpInterval = interval;
    tmpMode = _mode;

    if (interval < 30 && _mode == DPM_RTC_NORMAL_MODE)
    {
        APRINTF_E("[%s] timer interval very short(=%d secs)\n", __func__, interval);
    }

    if (_mode >= DPM_RTC_ABNORMAL_MODE_1 && _mode <= DPM_RTC_ABNORMAL_MODE_2)
    {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_1ST;
    } else if (_mode >= DPM_RTC_ABNORMAL_MODE_3
            && _mode <= DPM_RTC_ABNORMAL_MODE_4)
            {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_2ND;
    } else if (_mode >= DPM_RTC_ABNORMAL_MODE_5
            && _mode <= DPM_RTC_ABNORMAL_MODE_6)
            {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_3RD;
    } else
    {
        if (_mode > DPM_RTC_ABNORMAL_MODE_9)
        {
            tmpMode = DPM_RTC_NORMAL_MODE;
        }
        //check validation
        if (interval >= AZURE_CONFIG_KEEP_INTERVAL_DEF)
        {
            tmpInterval = interval - MIN_RTC_WAKEUP_SUBSTRACT_FOR_KA;
        } else
        {
            tmpInterval = interval;
        }
        if (tmpInterval < MIN_RTC_WAKEUP_INTERVAL)
        {
            tmpInterval = MIN_RTC_WAKEUP_INTERVAL;
        }
    }

    if (app_dpm_get_sleep_flag() == 0)
    {
        APRINTF("[%s] RTC interval (=%d secs), mode (=%d)\n", __func__,
                tmpInterval, tmpMode);
        vTaskDelay(1);
    }
//azure [[:

    sprintf(dpmTimerJobName, "%s", APP_AZURE_TWIN);
    sprintf(dpmTimerName, "%sN", APP_AZURE_TWIN);
    tmpTID = RM_PMGR_W_dpm_timer_create(dpmTimerJobName, dpmTimerName, NULL, tmpInterval*1000, tmpInterval*1000);
//azure]]
    //register timer
    //tmpTID = rtc_register_timer(tmpInterval*1000, dpmThreadName, tmpTID, 1, //0: one-shot, 1: periodically
        //	NULL); // azure CHECK commented

    if (tmpTID < 0 || tmpTID == DPM_TIMER_ERR)
    {
        APRINTF_E("[%s] failed to register timer: interval = %d, tid = %d\n", __func__, tmpInterval, tmpTID);
        vTaskDelay(1);

        if (dataInternal)
        {
            dataInternal->tid = APP_DPM_TIMER_ID;
        }
        if (dataExternal)
        {
            dataExternal->tid = APP_DPM_TIMER_ID;
        }

        status = ER_NOT_SUCCESSFUL;
        return status;
    } else
    {
        if (dataInternal)
        {
            dataInternal->tid = tmpTID;
            dataInternal->interval = tmpInterval;
        } else if (dataExternal)
        {
            dataExternal->tid = tmpTID;
            dataExternal->interval = tmpInterval;
        }
    }

    return status;
}

static int reentranceCount = 1;
void goSleepAppDpmThread(UINT8 _resetTimer, APPTimerMode _mode)
{
    if (RM_PMGR_W_dpm_is_enabled())
    {
        char dpmThreadName[20] = { 0, };
        INT32 lenDpmThreadName = 0;

        if (pAppDpmThread == NULL)
        {
            lenDpmThreadName = strlen(APP_AZURE_TWIN);
            memcpy(dpmThreadName, APP_AZURE_TWIN, strlen(APP_AZURE_TWIN));
        } else
        {
            lenDpmThreadName = strlen(pAppDpmThread->DPMRegeditName);
            memcpy(dpmThreadName, pAppDpmThread->DPMRegeditName,
                    lenDpmThreadName);
        }
        configASSERT(lenDpmThreadName < 12);
        configASSERT(lenDpmThreadName != 0);

        if (reentranceCount++ % 50 == 0)
        {
            APRINTF("[%s] call count=%d, dpm_pdown_started=%d\n", __func__,
                    reentranceCount, RM_PMGR_W_dpm_sleep_is_started());
            //dpm_debug
            // RM_PMGR_W_dpm_dbg_info_get();
        }

        //RTC_READY_POWER_DOWN(0);

        if (_resetTimer)
        {
            dpm_keepalive_timer_register(pAppDpmThread, _mode);
        }

        RM_PMGR_W_dpm_sleep_ready_set(dpmThreadName);
        app_dpm_set_sleep_flag(1);
        //dpm_debug
        // app_dpm_put_to_sleep();

    }
}

void exitSleepAppDpmThread(void)
{
    if (RM_PMGR_W_dpm_is_enabled())
    {
        char dpmThreadName[20] = { 0, };
        INT32 lenDpmThreadName = 0;

        if (pAppDpmThread == NULL) //DPM app thread not used
        {
            lenDpmThreadName = strlen(APP_AZURE_TWIN);
            memcpy(dpmThreadName, APP_AZURE_TWIN, strlen(APP_AZURE_TWIN));
        } else //DPM app thread used
        {
            lenDpmThreadName = strlen(pAppDpmThread->DPMRegeditName);
            memcpy(dpmThreadName, pAppDpmThread->DPMRegeditName,
                    lenDpmThreadName);
        }
        configASSERT(lenDpmThreadName < 12);
        configASSERT(lenDpmThreadName != 0);

        RM_PMGR_W_dpm_sleep_ready_clear(dpmThreadName);

    }
}

unsigned long long getAppSleep2Interval(int _defSeconds)
{
    unsigned long long retVal = 0;
    int val;
    int defaultVal = _defSeconds;

    if (defaultVal >= 0xFFFFFFFF || defaultVal <= 0)
    {
        defaultVal = SLEEP_2_WAKEUP_TIME_INTERVAL_SEC;
    }

    if ((RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
            SLEEP_MODE2_RTC_TIME, &val) != 0) || val <= 0)
            {
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                SLEEP_MODE2_RTC_TIME, defaultVal) != 0)
                {
            APRINTF_E("Failed SLEEP_MODE2_RTC_TIME(\"%s\")", SLEEP_MODE2_RTC_TIME);
        }

        retVal = (unsigned long long) (defaultVal * 1000000);
    } else
    {
        retVal = (unsigned long long) (val * 1000000);
    }

    return retVal;
}
//azure [[::
UINT8 killDPMAppKeepAliveTimer(void)
{
    UINT8 rcode = pdPASS;
#if CFG_PMGR
    if (RM_PMGR_W_dpm_is_enabled())
    {
        if (app_get_thread_info())
        {
            UINT32 tmpCode = dpm_keepalive_timer_unregister(app_get_thread_info());
            if (tmpCode)
            {
                rcode = pdFAIL;
            }
            else
            {
                rcode = pdPASS;
            }
        }
    }
#endif
    return rcode;
}

void goSleepAppOnException(int _retryCount)
{
#if CFG_PMGR
    //UINT8 saveTime = 0;
#if defined(__RUN_APP_SLEEP_2__)
    APRINTF_E("[%s:%d][sleep mode 2] fail count(=%d) abnormal... sleep in %d seconds...",
        __func__, __LINE__, _retryCount-1, getSleepWakeupTimerIntervalSec());
    vTaskDelay(portCONVERT_MS_2_TICKS(30));
    saveTime = 1;
    app_common_goto_sleep_1_or_2(SLEEP_MODE_2, getSleepRTMUsed(), getSleepWakeupTimerIntervalSec(), saveTime);
#else
    if (RM_PMGR_W_dpm_is_enabled())
    {
        //APRINTF_E("[%s:%d] fail count(=%d) abnormal...sleep 2 in %d seconds...\n", __func__, __LINE__, _retryCount-1, SLEEP_2_WAKEUP_TIME_INTERVAL_SEC);
        //vTaskDelay(portCONVERT_MS_2_TICKS(30));
        //saveTime = 0;
        //app_common_goto_sleep_1_or_2(SLEEP_MODE_2, getSleepRTMUsed(), SLEEP_2_WAKEUP_TIME_INTERVAL_SEC, saveTime);

        APRINTF_E("[%s:%d] fail count(=%d) abnormal...reboot...",
            __func__, __LINE__, _retryCount-1);
        vTaskDelay(portCONVERT_MS_2_TICKS(30));
        //reboot scenario on No DPM
        reboot_func(SYS_REBOOT);
    }
    else
    {
        APRINTF_E("[%s:%d] fail count(=%d) abnormal...reboot...",
            __func__, __LINE__, _retryCount-1);
        vTaskDelay(portCONVERT_MS_2_TICKS(30));
        //reboot scenario on No DPM
        reboot_func(SYS_REBOOT);
    }
#endif
#else
    //reboot scenario on No DPM
    reboot_func(SYS_REBOOT);
#endif
}

//azure]]
