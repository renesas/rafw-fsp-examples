/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* 
 * This file was derived and modified from
 * "ShadowDemoMainExample.c" and "JobsDemoExample.c"
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "core_mqtt_serializer.h"
#include "portable.h"
#include "projdefs.h"
#include "task.h"
#include "net_ping.h"
#include "ping.h"

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#include "app_sample_manager.h"
#include "app_aws_user_conf.h"
#include "app_awsiot_user_config.h"
#include "app_dpm_thread.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "aws_iot_error.h"
#include "ota_update.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif
#include "logging_levels.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"
#include "core_pkcs11_config_defaults.h"
#include "rm_cert.h"
#include "os.h"

#if defined(LIBRARY_LOG_LEVEL)
#undef LIBRARY_LOG_LEVEL
#endif
#if defined(LIBRARY_LOG_NAME)
#undef LIBRARY_LOG_NAME
#endif
#define LIBRARY_LOG_LEVEL       LOG_INFO
#define LIBRARY_LOG_NAME        "DoorLockDemo"

#include "logging_stack.h"
/* Jobs library header. */
#include "jobs.h"
/* Shadow API header. */
#include "shadow.h"
/* JSON library includes. */
#include "core_json.h"
/* mbedTLS include for configuring threading functions */
#include "mbedtls/threading.h"
#include "threading_alt.h"
/* TinyCBOR library for CBOR encoding and decoding operations. */
#include "cbor.h"
/* corePKCS11 includes. */
#include "core_pkcs11.h"
#include "core_pkcs11_config.h"
/* AWS IoT Fleet Provisioning Library. */
#include "fleet_provisioning.h"
/* Demo includes. */
#include "mqtt_pkcs11_demo_helpers.h"
#include "pkcs11_operations.h"
#include "tinycbor_serializer.h"
#include "transport_mbedtls_pkcs11.h"

#undef printf

#include "rm_lwip_w_helper.h"
#include "iface_defs.h"
#include "app_sample_manager.h"

#define SNTP_TRY_COUNT                      30  //15

/**
 * @brief Each compilation unit that consumes the NetworkContext must define it.
 * It should contain a single pointer to the type of your desired transport.
 * When using multiple transports in the same compilation unit, define this pointer as void *.
 *
 * @note Transport stacks are defined in FreeRTOS-Plus/Source/Application-Protocols/network_transport.
 */
struct NetworkContext
{
    TlsTransportParams_t *pxParams;
};

/*-----------------------------------------------------------*/

static BaseType_t aws_dpm_app_sample_thread(void);

static void aws_dpm_app_boot_or_report(app_dpm_info_rtm *_rtmData, DM_NOTI _status);

/*-----------------------------------------------------------*/
extern UCHAR get_last_abnormal_act(void);

static void aws_dpm_app_boot_or_report(app_dpm_info_rtm *_rtmData, DM_NOTI _status)
{
	FSP_PARAMETER_NOT_USED(_rtmData);

    if (_status == DM_FINISH_DEVICE)
    {
        UINT8 flagRcvTimeout;
        app_dpm_get_recv_timeout_flag(&flagRcvTimeout);
        if (flagRcvTimeout == DPM_RCV_OK_CONNECT)
        {
            app_dpm_set_recv_timeout_flag(DPM_RCV_OK_SLEEP);
        }
    }
}

static bool is_device_fleet_provisioned(void)
{
    if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_UNIQUE_CERT))
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_UNIQUE_PRIV_KEY))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

static void DPM_App_Main(UINT32 _data, UINT32 _rtmData, DM_NOTI _status)
{
    app_dpm_info_rtm *rtmData = (app_dpm_info_rtm*)_rtmData;

    (void)_data;

    switch (_status)
    {
        case DM_INIT:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_INIT", __func__)
#if CFG_PMGR
            if (RM_PMGR_W_dpm_is_enabled())
            {
                /* set wakeup done */
                informWakeupAppDpmThread();
            }
#endif
            load_aws_credentials();
            app_set_registered_thing_name(getAppThingName()); //pre-registered thing used
            LogInfo(("this is the pre-registered device (thing ID: %s)", getAppThingName()));
        }
        break;
        case DM_NEED_CONNECTION:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_NEED_CONNECTION", __func__)

            /* Do the provisioning and connect here. */
            if (is_device_fleet_provisioned())
            {
                printf("ALREADY PROVISIONED. CONNECT TO AWS CLOUD\n\r");
                connect_to_aws_cloud();
            }
            else
            {
                printf("NEED TO DO FLEET PROVISIONING\n\r");
                prvFleetProvisioningTask();
            }
        }
        break;
        case DM_CHECK_RECV:
        {
        }
        break;
        case DM_WAKEUP_RECV:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_WAKEUP_RECV", __func__)
        }
        break;
        case DM_WAKEUP_SENSOR:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_WAKEUP_SENSOR (or external wakeup)", __func__)
        }
        break;
        case DM_WAKEUP_TIMER:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_WAKEUP_TIMER (tid=%d)", __func__, rtmData->tid)

        }
        break;
        case DM_WAKEUP_BOOT:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_WAKEUP_BOOT", __func__)
            aws_dpm_app_boot_or_report(rtmData, DM_WAKEUP_BOOT);
        }
        break;
        case DM_EXTERNAL_SENSOR:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_EXTERNAL_SENSOR", __func__)
        }
        break;
        case DM_FINISH_DEVICE:
        {
            awsiot_app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
            IOT_INFO("[%s] DM_FINISH_DEVICE", __func__)
#if (1 == AWS_IOT_DPM_APP_ENABLE)       
            /* Go to sleep here */
#endif        
            free_aws_credentials();

        }
        break;

        case DM_NO_DPM_MODE:
        {
            IOT_INFO("[%s] DM_NO_DPM_MODE", __func__)
            /* process only reception loop */
        }
        break;

        case DM_CHECK_DEVICE:
        {
            IOT_INFO("[%s] DM_CHECK_DEVICE", __func__)
            /* check network status */
            /* check communication with the peer server */
            /* just sleep without resetting KA timer */
#if CFG_PMGR
            INT32 wakeup_type;
            UINT32 loopCnt = 0;
LOOP_DM_CHECK_DEVICE:
            loopCnt++;
            wakeup_type = (enum DPM_WAKEUP_TYPE) RM_PMGR_W_dpm_wakeup_type_get(0);
            goSleepAppDpmThread(0, DPM_RTC_NORMAL_MODE);

            if (chk_network_ready(WLAN0_IFACE) == pdFALSE)
            {
                APRINTF("[%s:%d] Wi-Fi Disconn -> Wait for Reconn ...\n", __func__, __LINE__);
                while (1)
                {
                    if (chk_network_ready(WLAN0_IFACE) == pdTRUE)
                    {
                        break;
                    }
                    vTaskDelay(portCONVERT_MS_2_TICKS(500));
                }
            }

            if (wakeup_type == DPM_NOACK_WAKEUP)
            {
                APRINTF_I("[%s] waiting process(cnt=%u) .... No Ack (abnormal act type=%d)\n", __func__, loopCnt, get_last_abnormal_act());
            }
            else if (wakeup_type == DPM_DEAUTH_WAKEUP)
            {
                APRINTF_I("[%s] waiting process(cnt=%u) .... De Auth (abnormal act type=%d)\n", __func__, loopCnt, get_last_abnormal_act());
            }
            else if (wakeup_type == DPM_TIM_ERR_WAKEUP)
            {
                APRINTF_I("[%s] waiting process(cnt=%u) .... Tim err (abnormal act type=%d)\n", __func__, loopCnt, get_last_abnormal_act());
            }
            else
            {
                APRINTF_I("[%s] waiting process(cnt=%u) .... unknown(wakeup type=%d) (abnormal act type=%d)\n", __func__, loopCnt, wakeup_type, get_last_abnormal_act());
            }

            vTaskDelay(portCONVERT_MS_2_TICKS(500));
            goto LOOP_DM_CHECK_DEVICE;
#endif        

        }
        break;

        default:
        {
        }
        break;
    }
}

static BaseType_t aws_dpm_app_sample_thread(void)
{
    int sleepMode = 0;
    int rtcTime = 0;
    unsigned char useRTM = FALSE;
    UINT32 my_port = UNDEF_PORT;
    UINT32 uSizeStack = 0;
    BaseType_t status = pdPASS;
    dpmAPPDataQ sampleData;

#if defined(__SUPPORT_ATCMD__)
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                 AWSIOT_CFG_DPM_SLEEP_MODE, (int *)&sleepMode);
#endif
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                 AWSIOT_CFG_SLEEP_MODE2_RTC_TIME, (int *)&rtcTime);

#if defined(__RUN_APP_SLEEP_2__)
    sleepMode = SLEEP_MODE_2;
#endif //__RUN_APP_SLEEP_2__

    awsiot_app_print_elapse_time_ms("[%s:%d] sleepMode=%d, useRTM=%d", __func__, __LINE__, sleepMode, useRTM);
    /* check validation */
    if (sleepMode > SLEEP_MODE_3 || sleepMode <= SLEEP_MODE_NONE)
    {
        sleepMode = SLEEP_MODE_3; //default
        useRTM = TRUE;
    }

    rtcTime = AWS_CONFIG_KEEP_INTERVAL_MAX;
    if (sleepMode == SLEEP_MODE_2)
    {
        rtcTime = (int)(getAppSleep2Interval(SLEEP_2_WAKEUP_TIME_INTERVAL_SEC) / 1000000);
    }
    
    if (rtcTime <= 0)
    {
        awsiot_app_print_elapse_time_ms("[%s:%d] rtcTime=%u", __func__, __LINE__, rtcTime);
        rtcTime = AWS_CONFIG_KEEP_INTERVAL_MAX;
        if (sleepMode == SLEEP_MODE_2)
        {
            rtcTime = (int)(getAppSleep2Interval(SLEEP_2_WAKEUP_TIME_INTERVAL_SEC) / 1000000);
        }
    }

    if (sleepMode == SLEEP_MODE_2)
    {
        if (SLEEP_2_USE_RTM != 0)
        {
            useRTM = TRUE;
        }
        else
        {
            useRTM = FALSE;
        }
    }
    else
    {
        useRTM = TRUE;
    }

    APRINTF("sleepMode: %d, rtcTime: %d \n\n", sleepMode, rtcTime);
    setSleepMode((APPSleepMode)sleepMode, rtcTime, useRTM);

#if (1 == AWS_IOT_DPM_APP_ENABLE)
    app_dpm_get_client_socket_port(&my_port);
#else
    my_port = AWS_MQTT_PORT;
#endif

    uSizeStack = 4096*4;
    status = dpmAppThreadCreate(APP_AWS_SHADOW, DPM_App_Main, (UINT32)NULL, sizeof(app_dpm_info_rtm), uSizeStack,
                                AWS_SUBTASK_PRIORITY + 1, my_port, (UINT32)rtcTime, DEFAULT_AWS_DNS_ADDR,
                                DEFAULT_AWS_2ND_DNS_ADDR, SNTP_TRY_COUNT);

    sendDPMAppMSGQ(&sampleData);

    return status;
}

void aws_shadow_dpm_auto_start(void *arg)
{
    FSP_PARAMETER_NOT_USED(arg);

#if CFG_PMGR //hold on not entering DPM
    awsiot_app_print_elapse_time_ms(""); //for debug::starting elapsed time
#endif

    aws_dpm_app_sample_thread();
    vTaskDelete(NULL);
}

/*-----------------------------------------------------------*/
