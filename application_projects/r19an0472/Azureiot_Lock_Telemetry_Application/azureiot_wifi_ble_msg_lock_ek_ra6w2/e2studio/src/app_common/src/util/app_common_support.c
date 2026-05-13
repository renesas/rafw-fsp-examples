/**
 ****************************************************************************************
 *
 * @file app_common_support.c
 *
 * @brief Define the common utility for apps module
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
#include "lwip/dns.h"
#include "custom_config_sdk.h"

#include "common_utils.h"
#include "common_def.h"
#include "util_api.h"
#include "app_common_support.h"
#include "app_dpm_thread.h"
#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#endif
#include "rm_aws_lwip_sock_wrap_w_api.h"


#define NV_SAVED_IP_ADDRESS         "NV_SAVED_IP_ADDRESS"
#define NVR_FIRST_SNTP_TIME         "NV_FIRST_SNTP_TIME"
#define NV_SNTP_SUCCESS_FLAG        "NV_SNTP_SUCCESS_FLAG"
#define MAX_CNT_TO_RESET_SNTP       8 //Maximum checking count to restart SNTP


#if CFG_PMGR
//extern int RM_PMGR_W_dpm_is_enabled(void);
extern int RM_PMGR_W_dpm_is_wakeup(void);
#endif

static UINT32 app_nvram_set_server_ip(char *ip_str)
{
fsp_err_t res;

    res = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                           AWSIOT_CFG_SAVED_IP_ADDRESS, ip_str);

    if (res == 0) {
        return 1;
    } else {
        printf("write ip address in nvram fail !!\n");
        return 0;
    }
}

static char* app_nvram_get_server_ip(void)
{
     char *nvRetIP = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_SAVED_IP_ADDRESS, &nvRetIP);
    return nvRetIP;
}

static UINT32 app_rtm_set_server_ip(char *data_name, char *ip_addr)
{
    UINT32 res = ER_SUCCESS;
    app_dpm_info_rtm *data = NULL;

    UINT32 status = 0;
    const ULONG wait_option = 100;
    UINT32 len = 0;

    if (data_name == NULL) {
        printf("[set]rtm data name is null !!\n");
        return ER_NOT_SUCCESSFUL;
    }

#if CFG_PMGR
    /*if (RM_PMGR_W_dpm_is_enabled())*/ {
        len = RM_PMGR_W_user_rtm_get(data_name, (unsigned char**)&data);

        if (len == 0) {
            status = RM_PMGR_W_user_rtm_pool_alloc(data_name, (void**)&data, sizeof(app_dpm_info_rtm), wait_option);

            if (status) {
                printf("failed to allocate app_dpm info in rtm(0x%02x)\n", status);
                data = NULL;
            }
            if (data) {
                memset(data, 0x00, sizeof(app_dpm_info_rtm));
            }
        } else if (len != sizeof(app_dpm_info_rtm)) {
            printf("invalid size(%u)\n", len);
            data = NULL;
        }

        if (data) {
            memset(data->ServrIp, 0, sizeof(data->ServrIp));
            if (isvalidip(ip_addr)) {
                strcpy(data->ServrIp, ip_addr);
            } else {
                printf("[%s:%d] passed ip (=\"%s\") invalid, and cleared \n", __func__,
                    __LINE__, ip_addr);
            }
        } else {
            res = ER_NO_MEMORY;
            printf("[set] rtm data: null");
        }
    }
#endif
    /*else {
        res = ER_NOT_SUCCESSFUL;
    }*/

    return res;
}

static char* app_rtm_get_server_ip(char *data_name)
{
    UINT32 status = 0;
    const ULONG wait_option = 100;

    app_dpm_info_rtm *data = NULL;
    UINT32 len = 0;

    if (data_name == NULL) {
        printf("[get]rtm data name is null !!\n");
        return NULL;
    }

#if CFG_PMGR
    /*if (RM_PMGR_W_dpm_is_enabled())*/ {
        len = RM_PMGR_W_user_rtm_get(data_name, (unsigned char**)&data);

        if (len == 0) {
            status = RM_PMGR_W_user_rtm_pool_alloc(data_name, (void**)&data, sizeof(app_dpm_info_rtm), wait_option);

            if (status) {
                printf("failed to allocate app_dpm info in rtm(0x%02x)\n", status);
                data = NULL;
            }
            if (data) {
                memset(data, 0x00, sizeof(app_dpm_info_rtm));
            }
        } else if (len != sizeof(app_dpm_info_rtm)) {
            printf("invalid size(%u)\n", len);
            data = NULL;
        }

        if (data) {
            return data->ServrIp;
        } else {
            printf("[get] rtm data: null\n");
            return NULL;
        }
    }
#endif

    return NULL;
}

char* app_common_get_ip_from_noraml_dns(char *hostname)
{
    static char ip_str[64];
    bool dns_status;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;
    printf("hostName = \"%s\"\n", hostname);

retryQuery:
    dns_status = dns_A_Query((char*)hostname, ip_str, waitOptQuery);
    if (dns_status != pdFALSE && isvalidip(ip_str)) {
        printf("host IP = \"%s\"\n", ip_str);
    } else {
        if (++retryCount <= 5) {
            waitOptQuery += 400;
            vTaskDelay(10);
            printf("dns_A_Query(time to wait: %lu ms) failed : retry (cnt=%d)...\n", waitOptQuery, retryCount);
            goto retryQuery;
        }
        memset(ip_str, 0, sizeof(ip_str));
		retryCount = 0;
		return NULL;
    }
    retryCount = 0; //re-initialize for reentrance

    return ip_str;
}

char* app_common_get_ip_from_fast_dns_with_rtm(char *hostname, char *rtm_data_name, UINT8 is_real_query)
{
    static char ip_str[64];
    bool dns_status;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;
    printf("hostName = \"%s\", flag to re-query (=%d)\n", hostname, is_real_query);

    if (is_real_query) {
        goto retryQuery;
    }

#if CFG_PMGR
    int wakeupSource = WAKEUP_SOURCE_FN();
    int wakeupType = RM_PMGR_W_dpm_wakeup_type_get(0);
    if (RM_PMGR_W_dpm_is_wakeup() || (wakeupType == 0 && wakeupSource == WAKEUP_EXT_SIG_WITH_RETENTION)
        || (wakeupType == 0 && wakeupSource == WAKEUP_COUNTER_WITH_RETENTION)) {
        strncpy(ip_str, app_rtm_get_server_ip(rtm_data_name), sizeof(ip_str) - 1);
        ip_str[sizeof(ip_str) - 1] = '\0';
        if (isvalidip(ip_str))
            printf("host IP from RTM = \"%s\"\n", ip_str);
        else {
            printf("host IP from RTM: NG\n");
            goto retryQuery;
        }
    }
    else
#endif
    {
retryQuery:
app_print_elapse_time_ms("[%s:%d] call dns_A_Query() directly)", __func__, __LINE__);
        dns_status = dns_A_Query((char*)hostname, ip_str, waitOptQuery);
        if (dns_status != pdFALSE && isvalidip(ip_str)) {
          printf("host IP = \"%s\" \n", ip_str);
          app_rtm_set_server_ip(rtm_data_name, ip_str);
        } else {
            if (++retryCount <= 5) {
                waitOptQuery += 400;
 		vTaskDelay(10);
                printf("dns_A_Query(time to wait: %lu ms) failed : retry (cnt=%d)...\n", waitOptQuery, retryCount);
                goto retryQuery;
            }
			memset(ip_str, 0, sizeof(ip_str));
			retryCount = 0;
            return NULL;
        }
        retryCount = 0; //re-initialize for reentrance
    }

    return ip_str;
}

char* app_common_get_ip_from_fast_dns_with_nvram(char *hostname, UINT8 is_real_query)
{
    static char ip_str[64];
    bool dns_status;
    static char retryCount = 0;
    unsigned long waitOptQuery = 600;

    printf("hostName = \"%s\", flag to re-query (=%d)\n", hostname, is_real_query);

    if (is_real_query) {
        goto retryQuery;
    }

    strncpy(ip_str, app_nvram_get_server_ip(), sizeof(ip_str) - 1);
    ip_str[sizeof(ip_str) - 1] = '\0';
    if (isvalidip(ip_str)) {
        printf("host IP from NVRAM: \"%s\"\n", ip_str);
        return ip_str;
    } else {
        printf("host IP from NVRAM: NG\n");
    }

retryQuery:
    dns_status = dns_A_Query((char*)hostname, ip_str, waitOptQuery);
    if (dns_status != pdFALSE && isvalidip(ip_str)) {
        printf("host IP = \"%s\"\n", ip_str);
        app_nvram_set_server_ip(ip_str);
    } else {
        if (++retryCount <= 5) {
            waitOptQuery += 400;
            vTaskDelay(10);
            printf("dns_A_Query() failed : retry (cnt=%d)...\n", retryCount);
            goto retryQuery;
        }
		memset(ip_str, 0, sizeof(ip_str));
		retryCount = 0;
        return NULL;
    }
    retryCount = 0; //re-initialize for reentrance

    return ip_str;
}
/* EOF */
