/***********************************************************************************************************************
 * File Name    : app_dpm_thread.c
 * Description  : Define the DPM thread running on Apps module.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "bsp_api.h"
#include "projdefs.h"
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
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

#include "user_dpm.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#include "r_pm_if.h"
#endif

#include "rm_awsiot_w_cfg.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_dpm_thread.h"
#include "app_aws_user_conf.h"
#include "app_sample_manager.h"

#define MIN_RTC_WAKEUP_INTERVAL                                 3  //  (seconds) minimum value
#define MIN_RTC_WAKEUP_SUBSTRACT_FOR_KA                         30 // (seconds) early wakeup substraction for KA

#define DPM_APP_DEFAULT_STACK_SIZE                              (1024*8)/sizeof(StackType_t)
#define DPM_APP_DEFAULT_THREAD_PRIORITY                         (tskIDLE_PRIORITY + 1)

#define CreateMSGQ_CMD                                          4

#define DM_NEED_INIT                                            ((UINT8)0x01)
#define DM_NEED_CONNECT                                         ((UINT8)0x02)
#define DM_RECV_WAKEUP                                          ((UINT8)0x04)
#define DM_SENSOR_WAKEUP                                        ((UINT8)0x08)
#define DM_RTC_WAKEUP                                           ((UINT8)0x10)
#define DM_BOOT_WAKEUP                                          ((UINT8)0x20)
#define DM_NEED_CHECK_DEVICE                                    ((UINT8)0x80)

static void CreateDPMAppMSGQ(void);
static INT32 recvDPMAppMSGQ(void);
static UINT8 checkDPMModeInternal(dpmAppThreadInfo *_threadInfo);
#if CFG_PMGR
static void aws_app_timer_cb(char *timer_name);
#endif
static UINT32 dpm_keepalive_timer_unregister(dpmAppThreadInfo *_threadInfo);
static UINT32 dpm_keepalive_timer_register(dpmAppThreadInfo *_threadInfo, APPTimerMode _mode);

static APPSleepMode appSleepMode = SLEEP_MODE_3;
static unsigned char sleepRTMMem = 0;
static int appUsecTimer = 0;
static QueueHandle_t appMsgQ = NULL;
static char wakeupException[16] = {0, };
static INT32 externalEvent = 0;
static OTA_STATUS jobStartFlag = OTA_STAT_JOB_NONE;

OTA_STATUS getOTAStat(void)
{
    return (OTA_STATUS)jobStartFlag;
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

static void set_sntp_with_sleepMode(dpmAppThreadInfo *appInfo, APPSleepMode sleepMode, UINT32 wakeupSrc,
                                    INT32 wakeupType)
{
#if CFG_PMGR
    /* check time only when no DPM or boot up */
    if (sleepMode == SLEEP_MODE_2)
    {
        if (SLEEP_2_USE_RTM != 0)
        {
            if (wakeupSrc != WAKEUP_COUNTER_WITH_RETENTION)
            {
                awsiot_app_print_elapse_time_ms("[%s:%d] sleep mode 2: setting time(rtm used)", __func__, __LINE__);
                app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount, (UINT8)SLEEP_MODE_2, appInfo->wakeUpTime);
            }
        }
        else
        {
            awsiot_app_print_elapse_time_ms("[%s:%d] sleep mode 2: setting time(rtm not used)", __func__, __LINE__);
#if (1 == AWS_IOT_DPM_APP_ENABLE)
            app_common_set_time_sync_from_fast_sntp_with_nvram(appInfo->sntpCount, (UINT8)SLEEP_MODE_2,
                                                               appInfo->wakeUpTime, (int)(getAppSleep2Interval(SLEEP_2_WAKEUP_TIME_INTERVAL_SEC) / 1000000));
#endif                
        }
    }
    else // default: sleep mode3 (DPM)
#endif
    {
        if (wakeupType == DPM_UNKNOWN_WAKEUP)
        {
            app_common_set_time_sync_from_normal_sntp(appInfo->sntpCount, (UINT8)SLEEP_MODE_3, appInfo->wakeUpTime);
        }
    }
}

static UINT8 checkDPMModeInternal(dpmAppThreadInfo *_threadInfo)
{
    UINT8 flag = 0x00;
    InternalRTM *data;
    INT32 readyDNS;
#if CFG_PMGR
    UINT32 wakeupMode = WAKEUP_SOURCE_FN();
    INT32 wakeup_type = (enum DPM_WAKEUP_TYPE) RM_PMGR_W_dpm_wakeup_type_get(0);
#else
    UINT32 wakeupMode = 0;
    INT32 wakeup_type = 0;
#endif

    flag |= DM_NEED_INIT;
    flag |= DM_NEED_CONNECT;

    data = (InternalRTM*)_threadInfo->internalRTM;

    APRINTF_I("\n============================\n");
    APRINTF("Wake mode [0x%02x] , type [%d] \n", wakeupMode, wakeup_type);
    if (wakeupMode == WAKEUP_SOURCE_EXT_SIGNAL || wakeupMode == WAKEUP_EXT_SIG_WITH_RETENTION)
    {
        APRINTF_I("wake up from sensor or external signal\n");
        flag |= DM_SENSOR_WAKEUP;
        /* clear timer mode */
        if (data)
        {
            data->mode = DPM_RTC_NORMAL_MODE;
        }
        set_sntp_with_sleepMode(_threadInfo, getSleepMode(), wakeupMode, wakeup_type);
    }
    else
    {
        switch (wakeup_type)
        {
            case DPM_UNKNOWN_WAKEUP:
            {
                readyDNS = 0;
                APRINTF_I("Wake up .... Boot \n");
                flag |= DM_BOOT_WAKEUP;

                if (wakeupMode == WAKEUP_RESET || wakeupMode == WAKEUP_SOURCE_POR || wakeupMode == WAKEUP_WATCHDOG)
                {
                    /* set & enable DNS */
                    APRINTF("DNS_Address [%s] [%s]\n", _threadInfo->DNSAddr, _threadInfo->secDNSAddr);
                    /* set & enable 2 DNS */
                    if (set_dns_addr_2nd(0, _threadInfo->secDNSAddr) != pdPASS)
                    {
                        APRINTF("set_dns_addr_2nd() failed\n");
                    }
                    else
                    {
                        readyDNS = 1;
                        APRINTF("set_dns_addr_2nd() OK: \"%s\"\n", _threadInfo->secDNSAddr);
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
                set_sntp_with_sleepMode(_threadInfo, getSleepMode(), wakeupMode, wakeup_type);
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
                APRINTF_I("Wake up .... User Request \n");
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

        if (wakeup_type != DPM_RTCTIME_WAKEUP)
        {
            /* clear time mode */
            if (data)
            {
                data->mode = (UINT8)DPM_RTC_NORMAL_MODE;
            }
        }
    }
    APRINTF_I("============================\n\n");

    return flag;
}

#if CFG_PMGR
static INT32 dpm_info_get(char *_name, void **_data, UINT32 _dataSize, UINT8 _share)
{
    UINT32 status = ER_SUCCESS;
    const ULONG wait_option = 0;
    UINT32 len = 0;

    if (!_share) //internal
    {
        len = RM_PMGR_W_user_rtm_get(_name, (unsigned char**)_data);
        if (len == 0)
        {
            status = (int)RM_PMGR_W_user_rtm_pool_alloc(_name, (void**)_data, _dataSize, wait_option);
            if (status)
            {
                APRINTF_E("[%s] failed to allocate dpm info in rtm(0x%02x)\n", __func__, status);
                vTaskDelay(portCONVERT_MS_2_TICKS(10));
                return pdFAIL;
            }
            memset(*_data, 0x00, _dataSize);
        }
        else if (len != _dataSize)
        {
            APRINTF("[%s] invalid size(%u)\n", __func__, len);
            vTaskDelay(portCONVERT_MS_2_TICKS(10));
            return pdFAIL;
        }
    }
    else //external
    {
        app_dpm_info_get(_name, (app_dpm_info_rtm **)_data);
    }

    if (*_data != NULL)
    {
        APRINTF("\"%s\" got DPM's RTM data size (%u)\n", _name, _dataSize);
    }

    return pdPASS;
}

static INT32 dpm_wakeup_init(dpmAppThreadInfo *_threadInfo)
{
    char *regName = _threadInfo->DPMRegeditName;
    char dpmTimerJobName[20] = {0, };
    int rcode;

    if (regName == NULL)
    {
        sprintf(dpmTimerJobName, "%s", APP_AWS_SHADOW);
    }
    else
    {
        sprintf(dpmTimerJobName, "%s", regName);
    }

    configASSERT(strlen(dpmTimerJobName) < REG_NAME_DPM_MAX_LEN);
    configASSERT(strlen(dpmTimerJobName) > 0);

    rcode = RM_PMGR_W_dpm_wakeup_done(dpmTimerJobName);
    APRINTF("[%s:%d] RM_PMGR_W_dpm_wakeup_done(\"%s\")=%d\n", __func__, __LINE__, dpmTimerJobName, rcode);

    return rcode;
}

static void initDPMThread(dpmAppThreadInfo *_threadInfo)
{
    UINT32 dpmPort = _threadInfo->dpmPort;

    /* save the passed pointer to global */
    app_set_thread_info(_threadInfo);
    APRINTF_I("[%s] =============================\n", __func__);
    APRINTF_I("[%s] _threadInfo->DpmMode = %s\n", __func__, _threadInfo->DpmMode ? "enabled" : "disabled");
    APRINTF_I("[%s] =============================\n", __func__);

#if CFG_PMGR
    if (_threadInfo->DpmMode) //in case of dpm mode
    {
        app_socket_set_port_n_filter(dpmPort); //AWS_MQTT_PORT
    }
#endif

}
#endif

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

static void dpmAPPManager(void *_threadInfo)
{
    UINT8 statusFlag;
    dpmAppThreadInfo *threadInfo = (dpmAppThreadInfo*)_threadInfo;
    UINT32 userData = threadInfo->pAppData;
    UINT32 rtmData = threadInfo->externalRTM;
    UINT8 saveTime = 1;
    dpmManagerFuncPtr cbAppMgr = (dpmManagerFuncPtr)threadInfo->entryInput;

    recvDPMAppMSGQ();

    statusFlag = checkDPMModeInternal(threadInfo);
    APRINTF_I("[%s] statusFlag : %x\n", __func__, statusFlag);
    /* check DPM work or not */
    if (statusFlag & DM_NEED_INIT)
    {
        APRINTF_I("[%s] DM_NEED_INIT\n", __func__);
        initDPMThread(threadInfo);
        awsiot_app_print_elapse_time_ms("[%s][%d] initDPMThread()", __func__, __LINE__);
        /* register KA wakeup timer */
#if (1 == AWS_IOT_DPM_APP_ENABLE)
        dpm_keepalive_timer_register(_threadInfo, DPM_RTC_NORMAL_MODE);
#endif        
        cbAppMgr(userData, rtmData, DM_INIT);
    }

    if (statusFlag & DM_NEED_CONNECT)
    {
        if (statusFlag & DM_NEED_CHECK_DEVICE)
        {
            APRINTF_I("[%s] exception: DM_NEED_CONNECTION bypassed\n", __func__);
        }
        else
        {
            APRINTF_I("[%s] DM_NEED_CONNECTION\n", __func__);
            cbAppMgr(userData, rtmData, DM_NEED_CONNECTION);
        }
    }
    /* need DPM init confrrm */
    if (statusFlag & DM_RECV_WAKEUP)
    {
        APRINTF_I("[%s] DM_RECV_WAKEUP\n", __func__);
        cbAppMgr(userData, rtmData, DM_WAKEUP_RECV);
    }

    if (statusFlag & DM_SENSOR_WAKEUP)
    {
        APRINTF_I("[%s] DM_SENSOR_WAKEUP\n", __func__);
        cbAppMgr(userData, rtmData, DM_WAKEUP_SENSOR);
    }

    if (statusFlag & DM_RTC_WAKEUP)
    {
        APRINTF_I("[%s] DM_RTC_WAKEUP\n", __func__);
        cbAppMgr(userData, rtmData, DM_WAKEUP_TIMER);
    }

    if (statusFlag & DM_BOOT_WAKEUP)
    {
        APRINTF_I("[%s] DM_BOOT_WAKEUP\n", __func__);
        cbAppMgr(userData, rtmData, DM_WAKEUP_BOOT);
    }

    if (statusFlag & DM_NEED_CHECK_DEVICE)
    {
        if (strlen(wakeupException) == 0)
        {
            APRINTF_I("[%s] Unknown\n", __func__);
        }
        else
        {
            APRINTF_I("[%s] Exception : ", __func__);
            APRINTF_I("%s\n", wakeupException);
        }
        cbAppMgr(userData, rtmData, DM_CHECK_DEVICE);
    }

#if CFG_PMGR
    if (appSleepMode == SLEEP_MODE_2)
    {
        saveTime = 1;
        APRINTF_I("\nSleep mode 2: rtm used (=%d), timer interval(=%d sec)\n\n", sleepRTMMem, threadInfo->wakeUpTime);
        awsiot_app_print_elapse_time_ms("[%s:%d] app_common_goto_sleep_1_or_2()", __func__, __LINE__);
        app_common_goto_sleep_1_or_2(appSleepMode, sleepRTMMem, threadInfo->wakeUpTime, saveTime);
    }
    else
    {
        APRINTF_I("\nSleep mode DPM: KA timer interval(=%d sec)\n\n", threadInfo->wakeUpTime);
        /* ToDo::awsmpmwork */
        if (externalEvent != 0)
        {
            APRINTF_I("\nHappend external event\n\n");
            /* Clear flag to protect goto DPM sleep mode */
            exitSleepAppDpmThread();
            cbAppMgr(userData, rtmData, DM_EXTERNAL_SENSOR);
            /* Clear flag */
            externalEvent = 0;
        }

        if (threadInfo->DpmMode == 1)
        {
            cbAppMgr(userData, rtmData, DM_FINISH_DEVICE);
        }
        else
        {
            /* retry to receive data */
            cbAppMgr(userData, rtmData, DM_NO_DPM_MODE);
        }
    }
#else
    /* retry to receive data */
    cbAppMgr(userData, rtmData, DM_NO_DPM_MODE);
#endif

    vTaskDelete(NULL);
}

BaseType_t dpmAppThreadCreate(char *name_ptr, dpmManagerFuncPtr _entryFunction, UINT32 _pUserStruc, UINT32 _RTMdataSize,
                              UINT32 _stackSize, UINT32 _priority, UINT32 _dpmPort, UINT32 _wakeUp_time, const char *_DNSAddr,
                              const char *_2ndDNSAddr, UINT32 _SNTPCount)
{
    UINT32 status = 0;
    UINT32 stringSize = 0;
    TaskHandle_t thread_ptr;
    char result[64] = {0, };
    dpmAppThreadInfo *threadInfo = NULL;
    char *pCRTM;

    APRINTF_I("\n[New thread start for AWS-IOT-W app]\n\n");

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

    threadInfo = (dpmAppThreadInfo*)APP_DPM_MALLOC(sizeof(dpmAppThreadInfo));
    /* make thread name */
    sprintf(result, "RM_%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->threadName = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->threadName, 0, stringSize + 1);
    strcpy(threadInfo->threadName, result);
    memset(result, 0, 64);

    /* make Reg name */
    sprintf(result, "%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->DPMRegeditName = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->DPMRegeditName, 0, stringSize + 1);
    strcpy(threadInfo->DPMRegeditName, result);
    memset(result, 0, 64);

    /* make internalRTM name */
    sprintf(result, "DIR%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->internalRTMName = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->internalRTMName, 0, stringSize + 1);
    strcpy(threadInfo->internalRTMName, result);
    memset(result, 0, 64);

    /* make externalRTM name */
    sprintf(result, "DER%s", name_ptr);
    stringSize = strlen(result);
    threadInfo->externalRTMName = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->externalRTMName, 0, stringSize + 1);
    strcpy(threadInfo->externalRTMName, result);
    threadInfo->entryInput = (UINT32)_entryFunction;
    threadInfo->pAppData = _pUserStruc;
    threadInfo->dpmPort = _dpmPort;
    if (appUsecTimer == 0)
    {
        threadInfo->wakeUpTime = _wakeUp_time;
    }
    else
    {
        threadInfo->wakeUpTime = appUsecTimer;
    }

#if CFG_PMGR
    threadInfo->DpmMode = RM_PMGR_W_dpm_is_enabled();
    threadInfo->DpmWakeUp = RM_PMGR_W_dpm_is_wakeup();
#else
    threadInfo->DpmMode = 0;
    threadInfo->DpmWakeUp = 0;
#endif
    threadInfo->sntpCount = _SNTPCount;
    /* set DNS addrress */
    stringSize = strlen((const char*)_DNSAddr);
    threadInfo->DNSAddr = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->DNSAddr, 0, stringSize + 1);
    strcpy(threadInfo->DNSAddr, _DNSAddr);

    stringSize = strlen((const char*)_2ndDNSAddr);
    threadInfo->secDNSAddr = (char*)APP_DPM_MALLOC(stringSize + 1);
    memset(threadInfo->secDNSAddr, 0, stringSize + 1);
    strcpy(threadInfo->secDNSAddr, _2ndDNSAddr);

#if CFG_PMGR
    if (threadInfo->DpmMode)
    {
        APRINTF_I("\n[New thread(\"%s\") start for AWS-IOT-W DPM App]\n\n", threadInfo->threadName);
    }
    else
#endif
    {
        APRINTF_I("\n[New thread(\"%s\") start for AWS-IOT-W App without DPM]\n\n", threadInfo->threadName);
    }

    APRINTF_I("\n=============================================\n");

#if CFG_PMGR
    if (threadInfo->DpmMode == 1)
    {
        APRINTF_S("\n[%s] low power mode checked...(mode=%d)\n", __func__, appSleepMode);
        dpm_info_get(threadInfo->internalRTMName, (void**)(&(threadInfo->internalRTM)), sizeof(InternalRTM), 0);
        dpm_info_get(threadInfo->externalRTMName, (void**)(&threadInfo->externalRTM), _RTMdataSize, 1);
        /* NOTE : timer ID sync-up with internal RTM and external RTM for displaying correct information on callback function */
        if (threadInfo->internalRTM && threadInfo->externalRTM)
        {
            InternalRTM *rtmDataInt = (InternalRTM*)threadInfo->internalRTM;
            app_dpm_info_rtm *rtmDataExt = (app_dpm_info_rtm*)threadInfo->externalRTM;
            if (rtmDataInt && rtmDataExt)
            {
                rtmDataExt->tid = rtmDataInt->tid;
            }
        }

        if (threadInfo->DpmWakeUp == 1)
        {
            APRINTF_S("Wake up from DPM Sleep \n\n");
        }
        else
        {
            APRINTF_S("Boot mode \n\n");
        }
    }
    else
#endif
    {
        pCRTM = (char*)APP_DPM_MALLOC(_RTMdataSize);
        if (pCRTM)
        {
            memset(pCRTM, 0, _RTMdataSize);
        }
        else
        {
            SYS_ASSERT(0);
        }

        threadInfo->internalRTM = (InternalRTM*)APP_DPM_MALLOC(sizeof(InternalRTM));
        if (threadInfo->internalRTM)
        {
            memset(threadInfo->internalRTM, 0, sizeof(InternalRTM));
        }
        else
        {
            SYS_ASSERT(0);
        }
        threadInfo->externalRTM = (UINT32)pCRTM;
        APRINTF_S("\n[%s] No low power mode checked...\n", __func__);
    }
    APRINTF_I("=============================================\n");

    /* RTC wakeup for KA: check the validation */
    if (threadInfo->wakeUpTime == 0)
        threadInfo->wakeUpTime = AWS_CONFIG_KEEP_INTERVAL_MAX;

    app_set_ka_value(threadInfo->wakeUpTime);
    /* set callback dpm timer in socket lwip wrapper*/
    threadInfo->dpm_timer_callback = goSleepAppDpmThread;
    threadInfo->exit_dpm_sleep_cb = exitSleepAppDpmThread;
    /* set persistent storage callback */
    set_persistant_storage_cb();
    /* queue NULL bug fixed */
    CreateDPMAppMSGQ();

    status = xTaskCreate(dpmAPPManager, threadInfo->threadName, _stackSize, (void*)threadInfo, _priority, &thread_ptr);
    if (status != pdPASS)
    {
        APRINTF("[%s] Failed to create dpmAPPManager thread\r\n", __func__);
    }

    return status;
}

void sendDPMAppMSGQ(dpmAPPDataQ *_dpmAppData)
{
    int status = 0;

    APRINTF_I("\nsendDPMAppMSGQ... \n\n");
    status = xQueueSend(appMsgQ, _dpmAppData, portMAX_DELAY);
    if (status != pdPASS)
    {
        APRINTF("[%s] xQueueSend error !!! (%d)\n", __func__, status);
    }
}

void setSleepMode(APPSleepMode _mode, int _sec, unsigned char _retention)
{
    appSleepMode = _mode;
    appUsecTimer = _sec;
    sleepRTMMem = _retention;

    appSetSleepMode(_mode);
    APRINTF_S("\n===========================================\n\n");
    if (_mode == SLEEP_MODE_NONE)
    {
        APRINTF_E("[ Set valid sleep mode !!! ] \n\n");
    }
    else
    {
        APRINTF("[ Set Sleep mode : %d , RTC Wakeup time (sec) : %d ,useRTM : %d ] \n\n", _mode, appUsecTimer, sleepRTMMem);
    }
    APRINTF_S("============================================\n\n");
}

APPSleepMode getSleepMode(void)
{
    return appSleepMode;
}

unsigned char getSleepRTMUsed(void)
{
    return sleepRTMMem;
}

int getSleepWakeupTimerIntervalSec(void)
{
    if (appUsecTimer <= 0)
    {
        APRINTF_E("[%s:%d] invalid timer interval (=%d)\n", __func__, __LINE__, appUsecTimer);
        /* set as default value */
        appUsecTimer = SLEEP_2_WAKEUP_TIME_INTERVAL_SEC;
    }
    return appUsecTimer;
}

void informWakeupAppDpmThread(void)
{
#if CFG_PMGR
    dpmAppThreadInfo * _threadInfo = NULL;

    app_get_thread_info(&_threadInfo);
    if (RM_PMGR_W_dpm_is_enabled())
    {
        if (_threadInfo)
        {
            dpm_wakeup_init(_threadInfo);
        }
    }
#endif
}

UINT8 killDPMAppKeepAliveTimer(void)
{
    UINT8 rcode = pdPASS;
    dpmAppThreadInfo * _threadInfo = NULL;

#if CFG_PMGR
    if (RM_PMGR_W_dpm_is_enabled())
    {
        app_get_thread_info(&_threadInfo);
        if (_threadInfo)
        {
            UINT32 tmpCode = dpm_keepalive_timer_unregister(_threadInfo);
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

static UINT32 dpm_keepalive_timer_unregister(dpmAppThreadInfo *_threadInfo)
{
    char dpmTimerJobName[20] = {0, };
    char dpmTimerName[20] = {0, };
    int ret = 0;

#if CFG_PMGR
    if (RM_PMGR_W_dpm_is_enabled())
    {
        if (_threadInfo == NULL)
        {
            sprintf(dpmTimerJobName, "%s", APP_AWS_SHADOW);
            sprintf(dpmTimerName, "%sN", APP_AWS_SHADOW);
        } 
        else 
        {
            sprintf(dpmTimerJobName, "%s", _threadInfo->DPMRegeditName);
            sprintf(dpmTimerName, "%sN", _threadInfo->DPMRegeditName);
        }

        configASSERT(strlen(dpmTimerJobName) < REG_NAME_DPM_MAX_LEN);
        configASSERT(strlen(dpmTimerJobName) > 0);
        configASSERT(strlen(dpmTimerName) < DPM_TIMER_NAME_MAX_LEN);
        configASSERT(strlen(dpmTimerName) > 0);

        if (RM_PMGR_W_dpm_timer_remaining_msec_get(dpmTimerJobName, dpmTimerName) != -1)
        {
            ret = RM_PMGR_W_dpm_timer_delete(dpmTimerJobName, dpmTimerName);
            if (ret < 0)
            {
                APRINTF_E("[%s] Failed to deregister AWS DPM's timer(%d)\n", __func__, ret);
                return ER_NOT_SUCCESSFUL;
            }
            else 
            {
                APRINTF_I("[%s] OK to deregister AWS DPM's timer(%d)\n", __func__, ret);
                return ER_SUCCESS;
            }
        }
        else
        {
            return ER_SUCCESS;
        }
    }
    else
#endif
    {
        APRINTF_E("[%s] no CFG_PMGR\n", __func__);
        return ER_SUCCESS;
    }
}

#if CFG_PMGR
static void aws_app_timer_cb(char *timer_name)
{
    APRINTF_I("AWS App Timer(%s)\n", timer_name);
    RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_SLEEP_PROHIBITED);

    return ;
}
#endif

/* _mode: 0 - normal mode, 1 ~ 9 - abnormal mode */
static UINT32 dpm_keepalive_timer_register(dpmAppThreadInfo *_threadInfo, APPTimerMode _mode)
{
    UINT32 status = ER_SUCCESS;
    UINT32 interval = 0;
    INT32 tmpTID = APP_DPM_TIMER_ID;
    INT32 tmpInterval;
    APPTimerMode tmpMode;
    InternalRTM *dataInternal = NULL;
    app_dpm_info_rtm *dataExternal = NULL;
    char dpmTimerJobName[20] = {0, };
    char dpmTimerName[20] = {0, };
    UINT16 kaval;
#if CFG_PMGR
    UINT32 wakeupMode;
    INT32 wakeup_type;
    UINT8 flagRcvTimeout;
    int timeRemainedMsec;
#endif

    if (_threadInfo == NULL)
    {
        app_get_ka_value(&kaval);
        app_dpm_info_get(NULL, &dataExternal);
        sprintf(dpmTimerJobName, "%s", APP_AWS_SHADOW);
        sprintf(dpmTimerName, "%sN", APP_AWS_SHADOW);
        if (dataExternal)
        {
            tmpTID = dataExternal->tid;
        }
    }
    else
    {
        interval = _threadInfo->wakeUpTime;
        dataInternal = (InternalRTM*)_threadInfo->internalRTM;
        sprintf(dpmTimerJobName, "%s", _threadInfo->DPMRegeditName);
        sprintf(dpmTimerName, "%sN", _threadInfo->DPMRegeditName);
        if (dataInternal)
        {
            tmpTID = dataInternal->tid;
        }
    }

    configASSERT(strlen(dpmTimerJobName) < REG_NAME_DPM_MAX_LEN);
    configASSERT(strlen(dpmTimerJobName) > 0);
    configASSERT(strlen(dpmTimerName) < DPM_TIMER_NAME_MAX_LEN);
    configASSERT(strlen(dpmTimerName) > 0);

#if CFG_PMGR
    /* timer - INIT */
    wakeupMode = WAKEUP_SOURCE_FN();
    wakeup_type = (enum DPM_WAKEUP_TYPE) RM_PMGR_W_dpm_wakeup_type_get(0);

    app_dpm_get_recv_timeout_flag(&flagRcvTimeout);
    if (RM_PMGR_W_dpm_is_wakeup() && flagRcvTimeout != DPM_RCV_OK_SLEEP)
    {
        printf("[%s:%d] wakeup_mode=0x%02x, wakeup_type=%d\n", __func__, __LINE__, wakeupMode, wakeup_type);
    }

    tmpInterval = interval;
    tmpMode = _mode;
    (void)tmpMode;

    if (interval < 30 && _mode == DPM_RTC_NORMAL_MODE)
    {
        APRINTF_E("[%s] timer interval very short(=%d secs)\n", __func__, interval);
    }

    if (_mode >= DPM_RTC_ABNORMAL_MODE_1 && _mode <= DPM_RTC_ABNORMAL_MODE_2)
    {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_1ST;
    }
    else if (_mode >= DPM_RTC_ABNORMAL_MODE_3 && _mode <= DPM_RTC_ABNORMAL_MODE_4)
    {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_2ND;
    }
    else if (_mode >= DPM_RTC_ABNORMAL_MODE_5 && _mode <= DPM_RTC_ABNORMAL_MODE_6)
    {
        tmpInterval = DPM_RTC_ABNORMAL_INTERVAL_3RD;
    }
    else
    {
        if (_mode > DPM_RTC_ABNORMAL_MODE_9)
        {
            tmpMode = DPM_RTC_NORMAL_MODE;
        }
        /* check validation */
        if (interval >= AWS_CONFIG_KEEP_INTERVAL_DEF)
        {
            tmpInterval = interval - MIN_RTC_WAKEUP_SUBSTRACT_FOR_KA;
        }
        else
        {
            tmpInterval = interval;
        }
        if (tmpInterval < MIN_RTC_WAKEUP_INTERVAL)
        {
            tmpInterval = MIN_RTC_WAKEUP_INTERVAL;
        }
    }

    /* register timer */
    timeRemainedMsec = RM_PMGR_W_dpm_timer_remaining_msec_get(dpmTimerJobName, dpmTimerName);
    if (timeRemainedMsec == -1)
    {
        /* one-shot */
        tmpTID = RM_PMGR_W_dpm_timer_create(dpmTimerJobName, dpmTimerName, aws_app_timer_cb, tmpInterval*1000, 0);
    }
    else //timer exists: reset timer
    {
        awsiot_app_print_elapse_time_ms("[%s:%d] remained time (%u msec)", __func__, __LINE__, timeRemainedMsec);

        if (timeRemainedMsec == 0)
        {
            tmpTID = RM_PMGR_W_dpm_timer_change(dpmTimerJobName, dpmTimerName, tmpInterval*1000);
            if ((tmpTID >= DPM_TIMER_0) && (tmpTID < DPM_TIMER_ERR))
            {
                APRINTF_I("[%s] OK to change AWS DPM's timer(%d)\n", __func__, tmpTID);
            }
            else
            {
                APRINTF_I("[%s] failed to change AWS DPM's timer(%d)\n", __func__, tmpTID);
            }
        }
        else
        {
            if (dpm_keepalive_timer_unregister(_threadInfo) != ER_SUCCESS)
            {
                APRINTF_E("[%s] Failed to deregister AWS DPM's timer(%d)\n", __func__, tmpTID);
                status = ER_NOT_SUCCESSFUL;
                return status;
            }
            else 
            {
                APRINTF_I("[%s] OK to delete AWS DPM's timer(%d)\n", __func__, tmpTID);
                /* one-shot */
                tmpTID = RM_PMGR_W_dpm_timer_create(dpmTimerJobName, dpmTimerName, aws_app_timer_cb, tmpInterval*1000, 0);
            }
        }
    }

    if (tmpTID < 0 && tmpTID != -3)
    {
        APRINTF_E("[%s] failed to register timer: interval = %d, tid = %d\n", __func__, tmpInterval, tmpTID);
        vTaskDelay(portCONVERT_MS_2_TICKS(10));

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
    } 
    else
    {
        if (dataInternal)
        {
            dataInternal->tid = tmpTID;
            dataInternal->interval = tmpInterval;
        }
        else if (dataExternal)
        {
            dataExternal->tid = tmpTID;
            dataExternal->interval = tmpInterval;
        }
        APRINTF_I("[%s] OK to register timer: interval = %d(sec), tid = %d\n", __func__, tmpInterval, tmpTID);
    }
#else
    {
        APRINTF_E("[%s] no CFG_PMGR\n", __func__);
        status = ER_SUCCESS;
    }
#endif

    return status;
}

void goSleepAppDpmThread(UINT8 _resetTimer, APPTimerMode _mode)
{
#if CFG_PMGR
    static int reentranceCount = 1;
    UINT8 flagSleepDPM = 0;
    char dpmMainJobName[20] = {0, };
    dpmAppThreadInfo * _threadInfo = NULL;
    fsp_err_t rcode = FSP_SUCCESS;
    INT32 rc;
    INT32 wakeup_type;

    app_dpm_get_sleep_flag(&flagSleepDPM);
    if (RM_PMGR_W_dpm_is_enabled())
    {
        app_get_thread_info(&_threadInfo);
        if (_threadInfo == NULL)
        {
            sprintf(dpmMainJobName, "%s", APP_AWS_SHADOW);
        }
        else
        {
            sprintf(dpmMainJobName, "%s", _threadInfo->DPMRegeditName);
        }
        configASSERT(strlen(dpmMainJobName) < REG_NAME_DPM_MAX_LEN);
        configASSERT(strlen(dpmMainJobName) != 0);

        if (reentranceCount++ % 50 == 0)
        {
            APRINTF("[%s] call count=%d, RM_PMGR_W_dpm_sleep_is_started()=%d\n", __func__, reentranceCount, RM_PMGR_W_dpm_sleep_is_started());
            RM_PMGR_W_dpm_dbg_info_get();
        }

        if (_resetTimer)
        {
            if (!RM_PMGR_W_dpm_sleep_is_started() && !flagSleepDPM)
            {
                awsiot_app_print_elapse_time_ms("[%s:%d] RM_PMGR_W_dpm_sleep_is_started(): not started -> KA timer reset", __func__, __LINE__);
                dpm_keepalive_timer_register(_threadInfo, _mode);
            }
        }

        if (!flagSleepDPM)
        {
            rc = RM_PMGR_W_dpm_sleep_ready_set(dpmMainJobName);

            if (rc == DPM_SET_OK)
            {
                app_dpm_set_sleep_flag(1);
            }
            else 
            {
                APRINTF_E("[%s:%d] dpm_app_sleep_ready_set(\"%s\") failed (rc=%d)\n", __func__, __LINE__, dpmMainJobName, rc);
                vTaskDelay(portCONVERT_MS_2_TICKS(30)); //for debug
            }

            rcode = RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_SLEEP_PROHIBITED);
            if (rcode != FSP_SUCCESS)
            {
                APRINTF_E("[%s:%d] RM_PMGR_W_remove_sleep_constraint() failed (rc=%d)\n", __func__, __LINE__, rcode);
            }

            wakeup_type = (enum DPM_WAKEUP_TYPE) RM_PMGR_W_dpm_wakeup_type_get(0);
            if (wakeup_type == DPM_RTCTIME_WAKEUP)
            {
                rcode = RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_SLEEP_PROHIBITED);
                if (rcode != FSP_SUCCESS)
                {
                    APRINTF_E("[%s:%d] RM_PMGR_W_remove_sleep_constraint() failed (rc=%d)\n", __func__, __LINE__, rcode);
                }
            }
        }
    }
#endif
}

void exitSleepAppDpmThread(void)
{
#if CFG_PMGR
    char dpmMainJobName[20] = {0, };
    dpmAppThreadInfo * _threadInfo = NULL;

    if (RM_PMGR_W_dpm_is_enabled())
    {
        app_get_thread_info(&_threadInfo);
        if (_threadInfo == NULL)
        {
            sprintf(dpmMainJobName, "%s", APP_AWS_SHADOW);
        }
        else
        {
            sprintf(dpmMainJobName, "%s", _threadInfo->DPMRegeditName);
        }
        configASSERT(strlen(dpmMainJobName) < REG_NAME_DPM_MAX_LEN);
        configASSERT(strlen(dpmMainJobName) != 0);

        RM_PMGR_W_dpm_sleep_ready_clear(dpmMainJobName);
        app_dpm_set_sleep_flag(0);
    }
#endif
}

unsigned long long getAppSleep2Interval(int _defSeconds)
{
    unsigned long long retVal = 0;
    int32_t val;
    int defaultVal = _defSeconds;

    if ((unsigned int)defaultVal >= 0xFFFFFFFF || (unsigned int)defaultVal <= 0)
    {
        defaultVal = SLEEP_2_WAKEUP_TIME_INTERVAL_SEC;
    }

    if ((RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                      AWSIOT_CFG_SLEEP_MODE2_RTC_TIME, (int *)&val) != 0) || val <= 0)
    {
        if ((RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                           AWSIOT_CFG_SLEEP_MODE2_RTC_TIME, defaultVal)) != 0)
        {
            APRINTF_E("Failed SLEEP_MODE2_RTC_TIME(\"%s\")", SLEEP_MODE2_RTC_TIME);
        }
        retVal = (unsigned long long)(defaultVal * 1000000);
    }
    else
    {
        retVal = (unsigned long long)(val * 1000000);
    }

    return retVal;
}

void goSleepAppOnException(UINT32 _retryCount)
{
#if CFG_PMGR
#if defined(__RUN_APP_SLEEP_2__)
    APRINTF_E("[%s:%d][sleep mode 2] fail count(=%d) abnormal... sleep in %d seconds...",
              __func__, __LINE__, _retryCount-1, getSleepWakeupTimerIntervalSec());
    vTaskDelay(portCONVERT_MS_2_TICKS(30));
    saveTime = 1;
    app_common_goto_sleep_1_or_2(SLEEP_MODE_2, getSleepRTMUsed(), getSleepWakeupTimerIntervalSec(), saveTime);
#else
    if (RM_PMGR_W_dpm_is_enabled())
    {
        APRINTF_E("[%s:%d] fail count(=%d) abnormal...reboot...",__func__, __LINE__, _retryCount-1);
        vTaskDelay(portCONVERT_MS_2_TICKS(30));
        /* reboot scenario on No DPM */
        reboot_func(SYS_REBOOT);
    }
    else
    {
        APRINTF_E("[%s:%d] fail count(=%d) abnormal...reboot...",__func__, __LINE__, _retryCount-1);
        vTaskDelay(portCONVERT_MS_2_TICKS(30));
        /* reboot scenario on No DPM */
        reboot_func(SYS_REBOOT);
    }
#endif
#else
    /* reboot scenario on No DPM */
    reboot_func(SYS_REBOOT);
#endif
}

