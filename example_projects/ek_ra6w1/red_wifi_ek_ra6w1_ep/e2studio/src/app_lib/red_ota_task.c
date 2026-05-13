/***********************************************************************************************************************

* File Name    : red_ota_task.c

* Description  : Red OTA Task

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#include "custom_config_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "r_rtc_w.h"
#include "rm_ota_w.h"
#include "rm_ota_w_api.h"
#include "net_sntp_client.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "lwip/err.h"
#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#endif

#define RED_OTA_DOWNLOAD_MODE_MANUAL    0
#define RED_OTA_DOWNLOAD_MODE_AUTO      1
#define RED_OTA_SDK_VERSION_COUNT       5    /* OTA SDK VERSION PARSING */

#define RED_OTA_UPDATE_TEST (1)

#if (RED_OTA_UPDATE_TEST)
#define RED_OTA_TASK_DELAY  (1000)            /* Task delay 1 seconds */
#define RED_OTA_PERIOD  (1)               /* Each 1 hour for test */
#else
#define RED_OTA_TASK_DELAY  (3600 * 1000)     /* Task delay 1 hour */
#define RED_OTA_PERIOD  (24 * 7)          /* Each week ( 24 hours * 7 days )*/
#endif
#define RED_OTA_TIMEOUT (60)              /* sec */
#define RED_OTA_DONE    (1)
#define RED_OTA_VERSION_LEN       (32)

/// OTA period thread name
#define RED_OTA_PER_TASK_NAME               "RED_OTA"
#define RED_OTA_PER_TASK_STACK_SIZE         (1024 * 6) / 4 //WORD
#define RED_OTA_CONNECT_FAIL_COUNT_LIMIT    2

/// Failed to sflash write
#ifndef OTA_FAILED_WRITE
#define OTA_FAILED_WRITE			0x0e
#endif

/// Failed to create timer
#ifndef OTA_FAILED_TIMEOUT
#define OTA_FAILED_TIMEOUT			0x10
#endif

/// Not initialized
#ifndef OTA_NOT_READY
#define OTA_NOT_READY				0x11
#endif

static TaskHandle_t red_ota_task_handle = NULL;
static QueueHandle_t  red_ota_done = NULL;

int red_ota_connect_fail_count = (0);

enum red_ota_violation
{
    RED_OTA_VIOLATION_SERVER_CONN_ERROR,
    RED_OTA_VIOLATION_SERVER_ABRT_ERROR,
    RED_OTA_VIOLATION_SERVER_AUTH_ERROR,
    RED_OTA_VIOLATION_OTA_JSON_ERROR
};

void red_ota_task(void);
void RED_OTA_VIOLATION(enum red_ota_violation ota_v);
void RED_OTA_FAULT_HANDLER(UINT status);
void red_ota_get_param(uint8_t * mode, uint32_t * expire, char * url);
fsp_err_t red_ota_set_param(uint8_t mode, uint32_t expire, char * url);

extern err_t red_ota_run_http_client(char *url, char *sdk_version, char *image_name,
    char *mcu_fw_update, char *mcu_fw_image_name, char *cert_key_update, char *cert_key_image_name);

static void red_ota_download_notify(ota_update_type update_type, UINT status, UINT progress)
{
    FSP_PARAMETER_NOT_USED(update_type);
    FSP_PARAMETER_NOT_USED(progress);
    if(red_ota_done != NULL)
    {
        xQueueSend(red_ota_done, (void *)&status, 0);
    }
}

static UINT red_ota_handle_new_fw(const char *url, ota_update_type update_type, UINT auto_renew)
{
    UINT status = OTA_SUCCESS;
    OTA_UPDATE_CONFIG *ota_update_conf = NULL;

    if ((url == NULL) || (strlen(url) == 0))
    {
        return OTA_FAILED;
    }

    ota_update_conf = OTA_MALLOC(sizeof(OTA_UPDATE_CONFIG));
    if (ota_update_conf == NULL)
    {
        OTA_ERR("[%s] Failed to alloc memory\n", __func__);
        return OTA_FAILED;
    }

    red_ota_done = xQueueCreate(1, sizeof(UINT));
    if (red_ota_done == NULL) 
    {
        OTA_ERR("- OTA : Failed to create red_ota_done queue\n");
        OTA_FREE(ota_update_conf);
        return OTA_FAILED;
    }

    memset(ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));
    ota_update_conf->update_type = update_type;
    ota_update_conf->auto_renew = auto_renew;
    memcpy(ota_update_conf->url, url, OTA_HTTP_URL_LEN);
    ota_update_conf->download_notify = red_ota_download_notify;

    /* OTA FW download API */
    status = ota_update_start_download(ota_update_conf);

    /* Wait Event */
    if(status == OTA_SUCCESS)
    {
        UINT ota_status;
        BaseType_t ret;
        ret = xQueueReceive(red_ota_done, 
                            &ota_status,
                            portCONVERT_MS_2_TICKS(RED_OTA_TIMEOUT * 1000));
        if(ret == pdPASS)
        {
            status = ota_status;
        }
        else 
        {
            status = OTA_FAILED_TIMEOUT;            
        }
    }

    if(ota_update_conf != NULL)
    {
        OTA_FREE(ota_update_conf);
    }

    if(red_ota_done)
    {
        vQueueDelete(red_ota_done);
        red_ota_done = NULL;
    }

    return status;
}

static void red_ota_fault_handling(UINT status)
{
    RED_OTA_FAULT_HANDLER(status);
}

static void red_ota_parse_skd_version_string (const char *version_string, uint32_t *ver)
{
    sscanf(version_string, "%u.%u.%u.%u.%u", &ver[0], &ver[1], &ver[2], &ver[3], &ver[4]);
}

static UINT red_ota_check_new_sdk_version (rm_ota_w_update_type_t update_type,
                                           char *cur_ver_str, char *new_ver_str)
{
    FSP_PARAMETER_NOT_USED(update_type);

    UINT ret_val = RM_OTA_W_SUCCESS;
    uint32_t cur_version[RED_OTA_SDK_VERSION_COUNT];
    uint32_t new_version[RED_OTA_SDK_VERSION_COUNT];
    UINT64 cur_ver_value;
    UINT64 new_ver_value;

    /* Check SDK version */
    red_ota_parse_skd_version_string(cur_ver_str, cur_version);
    red_ota_parse_skd_version_string(new_ver_str, new_version);

    cur_ver_value = ((UINT64)(cur_version[0]) << 32) + (cur_version[1] << 24) +
                    (cur_version[2] << 16) + (cur_version[3] << 8) + cur_version[4];
    new_ver_value = ((UINT64)(new_version[0]) << 32) + (new_version[1] << 24) +
                    (new_version[2] << 16) + (new_version[3] << 8) + new_version[4];

    if(cur_ver_value >= new_ver_value)
    {
        ret_val = RM_OTA_W_FAILED;
    }

    return ret_val;
}

static bool red_ota_is_new_version(char *ota_sdk_version)
{
    UINT ret_val = 0;
    char cur_version[64];

    /* Get current version */
    sprintf(cur_version, "%d.%d.%d.%d.%d", SDK_VER_PRODUCT_LINE, SDK_VER_MODE, SDK_VER_TARGET, SDK_VER_BRANCH, SDK_VER_R);

    RM_OTA_W_INFO("Check New version current %s, OTA %s\n", cur_version, ota_sdk_version );
    ret_val = red_ota_check_new_sdk_version(RM_OTA_W_TYPE_RTOS, cur_version, ota_sdk_version);
    if(ret_val == RM_OTA_W_SUCCESS)
    {
        RM_OTA_W_INFO("- New Version\n");
        return true;
    }
    else
    {
        RM_OTA_W_INFO("- No New Version\n");
        return false;
    }
}

void red_ota_get_param(uint8_t *mode, uint32_t *expire, char *url)
{
#ifdef RM_MAP_PERSISTANT_W
    int value;
    char * result_ptr = NULL;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_APPCFG, "OTA_MODE", &value) == FSP_SUCCESS ) 
    {
        *mode = value;
    }

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_APPCFG, "OTA_EXPIRE", &value) == FSP_SUCCESS ) 
    {
        *expire = value;                                        
    } 

    if (RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                        ENV_GROUP_APPCFG, "OTA_URL", &result_ptr) == FSP_SUCCESS ) 
    {
        strcpy(url, result_ptr);
    }         
#endif
}

fsp_err_t red_ota_set_param(uint8_t mode, uint32_t expire, char * url)
{
    fsp_err_t err = FSP_SUCCESS;
#ifdef RM_MAP_PERSISTANT_W

    if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_APPCFG, "OTA_MODE", mode) != 0)
    {
        err = FSP_ERR_WRITE_FAILED;
    }

    if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_APPCFG, "OTA_EXPIRE", expire) != 0)
    {
        err = FSP_ERR_WRITE_FAILED;
    }

    if (RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_APPCFG, "OTA_URL", url) != 0)
    {
        err = FSP_ERR_WRITE_FAILED;
    }
#endif
    return err;
}

static fsp_err_t red_ota_set_expire(uint32_t expire)
{
    fsp_err_t err = FSP_SUCCESS;
#ifdef RM_MAP_PERSISTANT_W
    if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_APPCFG, "OTA_EXPIRE", expire) != 0)
    {
        err = FSP_ERR_WRITE_FAILED;
    }
#endif
    return err;
}

static void red_ota_update_expire_hour(void)
{
    struct tm ts;
    uint32_t expire;

    R_RTC_W_CalendarTimeGet(R_RTC_W_GetCtrl(), &ts);
    expire = (ts.tm_year * 365 + ts.tm_yday) * 24 + ts.tm_hour + RED_OTA_PERIOD; /* set default period value */
    red_ota_set_expire(expire);
}

static bool red_ota_check_expire(uint32_t expire_hour, struct tm ts)
{
    uint32_t current_hour = (ts.tm_year * 365 + ts.tm_yday) * 24 + ts.tm_hour;
    return (expire_hour <= current_hour);
}

static void ota_task(void *params)
{
    FSP_PARAMETER_NOT_USED(params);
    struct tm ts;

    /* RED OTA URL and time setting */
    uint8_t red_ota_mode = 0; /* 0: manual, 1: auto */
    uint32_t red_ota_expire = 0;
    char red_ota_url_host[RM_OTA_W_CACHE_LEN] = {0,};

    /* Waiting for SNTP sync up */
    while (is_sntp_sync() != TRUE) 
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    red_ota_get_param(&red_ota_mode, &red_ota_expire, red_ota_url_host);

    if(red_ota_url_host[0] != '\0')
    {
        if(red_ota_mode == RED_OTA_DOWNLOAD_MODE_AUTO)
        {
            err_t http_ret;
            char url_file_ota_json[RM_OTA_W_CACHE_LEN] = {0,};
            char url_file_image[RM_OTA_W_CACHE_LEN] = {0,};
            char sdk_version[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
            char image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
            char mcu_fw_update[RED_OTA_VERSION_LEN] = {0,};
            char mcu_fw_image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
            char cert_key_update[RED_OTA_VERSION_LEN] = {0,};
            char cert_key_image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
            UINT status;

            strcpy(url_file_ota_json, red_ota_url_host);
            strcat(url_file_ota_json, "/ota.json");
        
            /* read ota.json form server */
            http_ret = red_ota_run_http_client(url_file_ota_json, sdk_version, image_name,
                mcu_fw_update, mcu_fw_image_name, cert_key_update, cert_key_image_name);
            if(http_ret == ERR_OK)
            {
                if(!strcmp(mcu_fw_update, "true")) {
                    printf("Update MCU firmware image name %s\n", mcu_fw_image_name);
                    strcpy(url_file_image, red_ota_url_host);
                    strcat(url_file_image, mcu_fw_image_name);                    
                    status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_MCU_FW, false);
                    red_ota_fault_handling(status);
                }
                
                if(!strcmp(cert_key_update, "true")) {
                    printf("Update cert_key image name %s\n", cert_key_image_name);
                    strcpy(url_file_image, red_ota_url_host);
                    strcat(url_file_image, cert_key_image_name);
                    status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_CERT_KEY, false);
                    red_ota_fault_handling(status);
                }

                if(red_ota_is_new_version(sdk_version))
                {
                    strcpy(url_file_image, red_ota_url_host);
                    strcat(url_file_image, image_name);
                    printf("OTA update when INIT.\n");
                    status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_RTOS, true);
                    red_ota_fault_handling(status);
                }

                red_ota_update_expire_hour();
            } 
            else
            {
                if (http_ret == ERR_RST)
                {
                    red_ota_connect_fail_count++;
                    printf("Server connection fail, retry count %d\n", red_ota_connect_fail_count);
                }
                else if (http_ret == ERR_ABRT)
                {
                    RED_OTA_VIOLATION(RED_OTA_VIOLATION_SERVER_ABRT_ERROR);
                    red_ota_update_expire_hour();
                }
                else if (http_ret == ERR_ARG)
                {
                    RED_OTA_VIOLATION(RED_OTA_VIOLATION_SERVER_AUTH_ERROR);
                    red_ota_update_expire_hour();
                }
                else if (http_ret == ERR_VAL)
                {
                    RED_OTA_VIOLATION(RED_OTA_VIOLATION_OTA_JSON_ERROR);
                    red_ota_update_expire_hour();
                }
                else
                {
                    printf("OTA HTTP Fail with ret %d\n", http_ret);
                }
            }
        }

        while(1) 
        {
            /* Wait for a second. */
            R_RTC_W_CalendarTimeGet(R_RTC_W_GetCtrl(), &ts);
            
            if(red_ota_check_expire(red_ota_expire, ts))
            {
                if(red_ota_mode == RED_OTA_DOWNLOAD_MODE_AUTO)
                {
                    err_t http_ret;
                    char url_file_ota_json[RM_OTA_W_CACHE_LEN] = {0,};
                    char url_file_image[RM_OTA_W_CACHE_LEN] = {0,};
                    char sdk_version[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
                    char image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
                    char mcu_fw_update[RED_OTA_VERSION_LEN] = {0,};
                    char mcu_fw_image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
                    char cert_key_update[RED_OTA_VERSION_LEN] = {0,};
                    char cert_key_image_name[RM_OTA_W_IMAGE_NAME_LEN] = {0,};
                    UINT status;

                    strcpy(url_file_ota_json, red_ota_url_host);
                    strcat(url_file_ota_json, "/ota.json");
    
                    /* read ota.json form server */
                    http_ret = red_ota_run_http_client(url_file_ota_json, sdk_version, image_name,
                        mcu_fw_update, mcu_fw_image_name, cert_key_update, cert_key_image_name);
                    if(http_ret == ERR_OK)
                    {
                        if(!strcmp(mcu_fw_update, "true"))
                        {                            
                            printf("Update MCU firmware image name %s\n", mcu_fw_image_name);
                            strcpy(url_file_image, red_ota_url_host);
                            strcat(url_file_image, mcu_fw_image_name);   
                            status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_MCU_FW, false);
                            red_ota_fault_handling(status);
                        }

                        if(!strcmp(cert_key_update, "true"))
                        {
                            printf("UPdate cert_key image name %s\n", cert_key_image_name);
                            strcpy(url_file_image, red_ota_url_host);
                            strcat(url_file_image, cert_key_image_name);                               
                            status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_CERT_KEY, false);
                            red_ota_fault_handling(status);
                        }

                        if(red_ota_is_new_version(sdk_version))
                        {
                            strcpy(url_file_image, red_ota_url_host);
                            strcat(url_file_image, image_name);
                            printf("OTA update when time expired.\n");
                            status = red_ota_handle_new_fw(url_file_image, OTA_TYPE_RTOS, true);
                            red_ota_fault_handling(status);
                        }

                        red_ota_update_expire_hour();
                    }
                    else 
                    {
                        if (http_ret == ERR_RST)
                        {
                            red_ota_connect_fail_count++;
                            printf("Server connection fail, retry count %d\n", red_ota_connect_fail_count);
                            if(red_ota_connect_fail_count > RED_OTA_CONNECT_FAIL_COUNT_LIMIT)
                            {
                                RED_OTA_VIOLATION(RED_OTA_VIOLATION_SERVER_CONN_ERROR);
                                red_ota_connect_fail_count = 0;
                                red_ota_update_expire_hour();
                            }
                        }
                        else if (http_ret == ERR_ABRT)
                        {
                            RED_OTA_VIOLATION(RED_OTA_VIOLATION_SERVER_ABRT_ERROR);
                            red_ota_update_expire_hour();
                        }
                        else if (http_ret == ERR_ARG)
                        {
                            RED_OTA_VIOLATION(RED_OTA_VIOLATION_SERVER_AUTH_ERROR);
                            red_ota_update_expire_hour();
                        }
                        else if (http_ret == ERR_VAL)
                        {
                            RED_OTA_VIOLATION(RED_OTA_VIOLATION_OTA_JSON_ERROR);
                            red_ota_update_expire_hour();
                        }
                        else
                        {
                        	printf("OTA HTTP Fail with ret %d\n", http_ret);
                        }

                    }
                }
            }
            vTaskDelay( pdMS_TO_TICKS( RED_OTA_TASK_DELAY ) );
            red_ota_get_param(&red_ota_mode, &red_ota_expire, red_ota_url_host);
        }
    }

    vTaskDelete(NULL);

    /* Should never reach here! */
    printf("OTA TASK ERROR - Should never reach here.\n");

    return;    
}

void red_ota_task(void)
{
    BaseType_t    xReturned;
    
    xReturned = xTaskCreate(ota_task,
                            RED_OTA_PER_TASK_NAME,
                            RED_OTA_PER_TASK_STACK_SIZE,
                            NULL,
                            OS_TASK_PRIORITY_USER,
                            &red_ota_task_handle);

    if (xReturned != pdPASS) 
    {
        printf("[%s] Failed to create task(%s)\n", __func__, RED_OTA_PER_TASK_NAME);
    } 
    else 
    {
        printf("[%s] RED_OTA Task Start!! \n", __func__);
    }
}

/* This is an example for OTA violation notification to the customer application 
   customized action like reporting to a Cloud may be updated by the customer */
void RED_OTA_VIOLATION(enum red_ota_violation ota_v)
{
    if(ota_v == RED_OTA_VIOLATION_SERVER_CONN_ERROR)
    {
        printf("==> OTA Violation Server Connection Error !!! \n");
    }
    else if(ota_v == RED_OTA_VIOLATION_SERVER_ABRT_ERROR)
    {
        printf("==> OTA Violation Server Abort Error !!! \n");
    }
    else if(ota_v == RED_OTA_VIOLATION_SERVER_AUTH_ERROR)
    {
        printf("==> OTA Violation Server Authentication Error !!! \n");
    }
    else if(ota_v == RED_OTA_VIOLATION_OTA_JSON_ERROR)
    {
        printf("==> OTA Violation OTA JSON Error !!! \n");
    }
}

/* This is Hault handler when OTA FAULT */
void RED_OTA_FAULT_HANDLER(UINT status)
{
    OTA_INFO("RED_OTA_FAULT_HANDLER  status = 0x%x\n", status);
}
