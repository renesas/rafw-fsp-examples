/***********************************************************************************************************************
 * File Name    : iot_app_cfg.c
 * Description  : Handles SNTP time and server configurations in nvram
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "rm_lwip_w_helper.h"
#include "net_dns_client.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#include "r_pm_if.h"
#endif
/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#include "rm_lwip_w_helper.h"
#include "net_dns_client.h"
#include "net_sntp_client.h"
#include "rm_vee_flash_w_rrq_nvram.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_dpm_thread.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"
#include "app_sample_manager.h"

#define NV_SAVED_IP_ADDRESS         "NV_IP_ADDRESS"
#define NVR_FIRST_SNTP_TIME         "NV_SNTP_TIME"
#define NV_SNTP_SUCCESS_FLAG        "NV_SNTP_SUC_FLAG"
#define MAX_CNT_TO_RESET_SNTP       8 //Maximum checking count to restart SNTP

extern unsigned int start_sntp(void);

static void net_check_prop(void)
{
    while (1)
    {
        if (check_net_init(WLAN0_IFACE) == pdPASS)
        {
            break;
        }
        vTaskDelay(portCONVERT_MS_2_TICKS(50));
    }

    /* Waiting netif status */
    while (chk_network_ready(WLAN0_IFACE) != pdTRUE)
    {
        vTaskDelay(portCONVERT_MS_2_TICKS(50));
    }
}

static char *app_nvram_get_server_ip(void)
{
    char *nvRetIP = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_SAVED_IP_ADDRESS, &nvRetIP);
    return nvRetIP;
}

static UINT32 app_nvram_set_server_ip(char *ip_str)
{
    fsp_err_t res;

    res = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                           AWSIOT_CFG_SAVED_IP_ADDRESS, ip_str);

    if (res == 0)
    {
        return 1;
    }
    else
    {
        printf("write ip address in nvram fail !!\n");
        return 0;
    }
}

static int save_sntp_time_nvram(unsigned long _now)
{
    char str[256];
    fsp_err_t retval;

    sprintf(str, "%lu", _now);
    retval = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                              AWSIOT_CFG_FIRST_SNTP_TIME, str);

    return retval;
}

#if (1 == AWS_IOT_DPM_APP_ENABLE)
static int delete_sntp_time_nvram()
{
    fsp_err_t retval = FSP_ERR_ERASE_FAILED;
    char *sntp_time = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_FIRST_SNTP_TIME, &sntp_time);
    if (sntp_time != NULL)
    {
        retval = RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_FIRST_SNTP_TIME);
    }

    return retval;
}

static unsigned long long read_sntp_time_from_nvram()
{
    char *sntp_time = NULL;
    unsigned long long ret_sntp_time;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_FIRST_SNTP_TIME, &sntp_time);

    if (sntp_time == NULL)
    {
        printf("NO SNTP value in NV\n");

        return 0;
    }
    ret_sntp_time = strtoul(sntp_time, NULL, 10);

    return ret_sntp_time;
}
#endif


void app_common_set_time_sync_from_normal_sntp(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval)
{
    __time64_t now;
    struct tm *ts;
    char buf[80];
    UINT32 loop_cnt = 0;
    char flagSntp = 0;
    int isSntpSuccess = 0;
    int isSntpNeed = 0;
    int use = 0;
    UINT8 saveTime = 0;
#if (1 == AWS_IOT_DPM_APP_ENABLE)
    int wakeupSource=0;
    int wakeupType=0;
    int rcode;
#endif

    /* wait for network initialization */
    net_check_prop();
    printf("\n\nSNTP connection try count = %d\n\n", _sntpCount);

#if (1 == AWS_IOT_DPM_APP_ENABLE)
#if CFG_PMGR
    wakeupSource = WAKEUP_SOURCE_FN();
    wakeupType = (enum DPM_WAKEUP_TYPE) RM_PMGR_W_dpm_wakeup_type_get(0);

    /* get current date/time:: in case of wakeup from sleep mode3, sleep mode2 with RTM or sleep mode1 with RTM */
    if (RM_PMGR_W_dpm_is_wakeup() || (wakeupSource == WAKEUP_COUNTER_WITH_RETENTION && wakeupType == 0)
        || (wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION && wakeupType == 0))
    {
        flagSntp = 1;

        rcode = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                             AWSIOT_CFG_SNTP_SUCCESS_FLAG, (int *)&isSntpSuccess);
        if (rcode != 0 || isSntpSuccess != 1)
        {
            flagSntp = 0;
            isSntpNeed = 1;
        }
        else
        {
            return;
        }
    }
    else
    {
        flagSntp = 0;
        isSntpNeed = 1;
    }
#else
        flagSntp = 0;
        isSntpNeed = 1;
#endif
#else
        flagSntp = 0;
        isSntpNeed = 1;
#endif
    if (isSntpNeed == 1)
    {
#ifdef RM_MAP_PERSISTANT_W
		RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, (const char*)NVR_KEY_SNTP_C, (INT32*)&use);
#else
		read_nvram_syscfg_int((const char*)NVR_KEY_SNTP_C, (INT32*)&use);
#endif

        if (use == -1)
        {
            use = 0;
        }

        if (use == 0)
        {
            printf("SNTP use=%d...call set_sntp_use(1)\n", use);
#ifdef RM_MAP_PERSISTANT_W
			RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SNTP_SYNC_PERIOD, DFLT_SNTP_SYNC_PERIOD);
			RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SNTP_SERVER_DOMAIN, DFLT_SNTP_SERVER_DOMAIN);
			RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SNTP_SERVER_DOMAIN_1, DFLT_SNTP_SERVER_DOMAIN_1);
			RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SNTP_SERVER_DOMAIN_2, DFLT_SNTP_SERVER_DOMAIN_2);
#else
            write_nvram_syscfg_int(NVR_KEY_SNTP_SYNC_PERIOD, DFLT_SNTP_SYNC_PERIOD); //129620);
            write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN, DFLT_SNTP_SERVER_DOMAIN);
            write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_1, DFLT_SNTP_SERVER_DOMAIN_1);
            write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_2, DFLT_SNTP_SERVER_DOMAIN_2);
#endif
            set_sntp_use(0);
            vTaskDelay(portCONVERT_MS_2_TICKS(50));
            set_sntp_use(1);
            start_sntp();
        }
    }

    while (1)
    {
        if (flagSntp || is_sntp_sync())
        {
            ra6w1_time64(NULL, &now);
            ts = (struct tm*)ra6w1_localtime64(&now);
            ra6w1_strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", ts);
            printf("SNTP Sync: %s : checked\n", buf);
            isSntpSuccess = 1;
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                              AWSIOT_CFG_SNTP_SUCCESS_FLAG, isSntpSuccess))
                printf("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

            break;
        }
        else
        {
            saveTime = 0;
            if (loop_cnt++ > _sntpCount)
            {
                printf("SNTP failed (cnt=%d)\n", loop_cnt - 1);
                isSntpSuccess = 0;
                if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                                  AWSIOT_CFG_SNTP_SUCCESS_FLAG, isSntpSuccess))
                    printf("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

                if (_sleepMode == SLEEP_MODE_2)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, saveTime);
                else
                {
                    goSleepAppOnException(loop_cnt);
                }
            }
            vTaskDelay(portCONVERT_MS_2_TICKS(500));
        }
    }
}

#if (1 == AWS_IOT_DPM_APP_ENABLE)
void app_common_set_time_sync_from_fast_sntp_with_nvram(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval,
                                                        unsigned long long added_sleep_dur)
{
    __time64_t now;
    struct tm *ts;
    char buf[80];
    UINT32 loop_cnt = 0;
    __time64_t setTime;
    unsigned long long readSntpTime = 0;
    int wakeupSource;
    int isSntpSuccess = 0;
    int isSntpNeed = 0;
    fsp_err_t rcode;
    UINT8 saveTime = 0;

    /* wait for network initialization */
    net_check_prop();

#if CFG_PMGR
    wakeupSource = WAKEUP_SOURCE_FN();
    if (wakeupSource == WAKEUP_SOURCE_WAKEUP_COUNTER || wakeupSource == WAKEUP_SOURCE_EXT_SIGNAL
        || wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION)
    {
        readSntpTime = read_sntp_time_from_nvram();
        rcode = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                             AWSIOT_CFG_SNTP_SUCCESS_FLAG, (int *)&isSntpSuccess);//[aws work]

        if (readSntpTime != 0 && rcode == FSP_SUCCESS && isSntpSuccess == 1)
        {
            readSntpTime = readSntpTime + added_sleep_dur;
            setTime = (__time64_t )readSntpTime;
            ra6w1_time64(&setTime, &now); /* set UTC Time *///[aws work]
            printf("[Set UTC Time(Saved Time + Sleep Duration)] : %llu \n", (unsigned long long )now);

            return;
        }
        else
        {
            printf("read sntp time fail from nvram!!\n");
            isSntpNeed = 1;
        }
    }
    /* restart sntp client regardless of easy setup */
    else if (wakeupSource == WAKEUP_RESET || wakeupSource == WAKEUP_SOURCE_POR)
    {
        isSntpNeed = 1;
    }
#else
    isSntpNeed = 1;
#endif

    if (isSntpNeed == 1)
    {
        set_sntp_use(0);
        vTaskDelay(portCONVERT_MS_2_TICKS(50));
        set_sntp_use(1);
    }

    while (1)
    {
        if (is_sntp_sync())
        {
            ra6w1_time64(NULL, &now); //[aws work]
            save_sntp_time_nvram((unsigned long)now);
            printf("[Saved SNTP Time in NVRAM] : %llu \n", now);
            ts = (struct tm*)ra6w1_localtime64(&now); //[aws work]

            ra6w1_strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", ts); //[aws work]
            printf("SNTP Sync: %s : checked\n", buf);

            isSntpSuccess = 1;
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                              AWSIOT_CFG_SNTP_SUCCESS_FLAG, isSntpSuccess)) //[aws work]
                printf("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

            set_sntp_use(0);

            break;
        }
        else
        {
            if (loop_cnt++ > _sntpCount)
            {
                printf("SNTP failed (cnt=%d)\n", loop_cnt - 1);
                delete_sntp_time_nvram();
                isSntpSuccess = 0;
                if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                                  AWSIOT_CFG_SNTP_SUCCESS_FLAG, isSntpSuccess))
                    printf("%s (=%d) on NVRAM write failed\n", NV_SNTP_SUCCESS_FLAG, isSntpSuccess);

                if (_sleepMode == SLEEP_MODE_2)
                    app_common_goto_sleep_1_or_2(_sleepMode, 0, _sleepInterval, saveTime);
                else
                {
                    goSleepAppOnException(_sntpCount);
                }
            }
            vTaskDelay(portCONVERT_MS_2_TICKS(500));
        }
    }
}
#endif


static int32_t app_common_save_time_sync_for_fast_sntp_with_nvram(void)
{
    __time64_t now;
    int res;

    ra6w1_time64(NULL, &now);
    res = save_sntp_time_nvram((unsigned long)now);
    if (res == 0)
    {
        printf("saved time before sleep : %llu \n", now);

        return 1;
    }
    else
    {
        printf("save sntp time fail !!\n");

        return 0;
    }
}

int32_t app_common_goto_sleep_1_or_2(APPSleepMode _sleepMode, UINT8 _useRTM, INT32 _wakeupIntervalSec, UINT8 _timeSave)
{
    if (_sleepMode == SLEEP_MODE_NONE || _sleepMode > SLEEP_MODE_2)
    {
        printf("[%s:%d] invalid sleep mode (=%d): failed to sleep\n", __func__, __LINE__, _sleepMode);

        return 1;
    }
    else if (_wakeupIntervalSec <= 0)
    {
        printf("[%s:%d] invalid wakeup timer interval seconds (=%d): failed to sleep\n", __func__, __LINE__, _wakeupIntervalSec);

        return 1;
    }
    else if (_useRTM != 0 && _useRTM != 1)
    {
        printf("[%s:%d] invalid RTM used flag (=%d): failed to sleep\n", __func__, __LINE__, _useRTM);

        return 1;
    }

    /* must unregister KA timer before entering sleep2 */
#if CFG_PMGR
    if (RM_PMGR_W_dpm_is_enabled())
    {
        killDPMAppKeepAliveTimer();
    }
#endif

    if (_sleepMode == SLEEP_MODE_2)
    {
        if (_useRTM == 0 && _timeSave == 1)
        {
            app_common_save_time_sync_for_fast_sntp_with_nvram();
        }

        printf("[%s:%d] go to sleep mode 2...(_useRTM=%d)\n", __func__, __LINE__, _useRTM);
        vTaskDelay(portCONVERT_MS_2_TICKS(30));
#if CFG_PMGR
        if (_useRTM)
        {
            R_PM_LowPowerModeEnter(PMGR_LLD_STATE3, SLEEPSECTOTICK(_wakeupIntervalSec));
        }
        else
        {
            R_PM_LowPowerModeEnter(PMGR_LLD_STATE2, SLEEPSECTOTICK(_wakeupIntervalSec));
        }
#endif
    }

    return 0;
}

static void *persist_storage_read(property_id_t id)
{
    switch (id)
    {
        case PROP_SVR_IP:
            return (void *)app_nvram_get_server_ip();

        case PROP_SNTP:
            return NULL;

        case PROP_DNS_IP:
            return NULL;

        default:
            return NULL;
    }
}

static int persist_storage_write(property_id_t id, const void *value)
{
    switch (id)
    {
        case PROP_SVR_IP:
    	    app_nvram_set_server_ip((char *)value);
        break;

        case PROP_SNTP:
        break;

        case PROP_DNS_IP:
        break;

        default:
            return -1;
    }
    return 0;
}

void set_persistant_storage_cb(void)
{
	persistant_read_callback_set(persist_storage_read);
	persistant_write_callback_set(persist_storage_write);
}
