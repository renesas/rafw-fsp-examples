/***********************************************************************************************************************
 * File Name    : app_dpm_sample.c
 * Description  : File which validate and starts the OTA update functionality.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

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
#include "app_dpm_sample.h"
#include "app_aws_user_conf.h"
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

#if defined(LIBRARY_LOG_LEVEL)
#undef LIBRARY_LOG_LEVEL
#endif
#if defined(LIBRARY_LOG_NAME)
#undef LIBRARY_LOG_NAME
#endif
#define LIBRARY_LOG_LEVEL       LOG_INFO
#define LIBRARY_LOG_NAME        "AWS_S3_OTA_Demo"
#define MAX_URL_LEN             256

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
#include "transport_mbedtls_pkcs11.h"
#include "app_aws_cred_manager.h"
#include "config_s3_http.h"
#undef printf

#include "rm_lwip_w_helper.h"
#include "iface_defs.h"


static char currentOTAResult[20] = {0, };   // "OTA_OK", "OTA_NG", "OTA_UNKNOWN"
UINT8 aws_file_url[MAX_URL_LEN] = {0, };    // justin change to get it from S3DownloadHTTP. It replace 

char* app_get_ota_url(void)
{
    return (char*)aws_file_url;
}

char* app_get_ota_result(void)
{
    return currentOTAResult;
}

static BaseType_t aws_s3_ota_validate_and_start_thread(void)
{
    fsp_err_t rcode;
    INT32 flagOTA;
    char *tmp_url = NULL;
    BaseType_t status = pdPASS;

    /* check OTA */
    rcode = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_OTA_FLAG, (int*)&flagOTA);
    if ((rcode == FSP_SUCCESS) && (flagOTA == AWS_OTA_STATE_READY))
    {
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_OTA_RESULT, AWS_OTA_RESULT_NG))
        {
            IOT_ERROR("write AWS_NVRAM_CONFIG_OTA_RESULT failed\n")
        }
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
        RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                        AWSIOT_CFG_OTA_URL, &tmp_url);
        if (tmp_url)
        {
            flagOTA = 0;
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                              AWSIOT_CFG_OTA_FLAG, AWS_OTA_STATE_UNKOWN))
            {
                IOT_ERROR("write AWS_NVRAM_CONFIG_OTA_FLAG failed")
            }
            vTaskDelay(portCONVERT_MS_2_TICKS(10));
            strncpy((char*)aws_file_url, (const char*)tmp_url, strlen(tmp_url));
#if defined(__SUPPORT_OTA__)
            aws_s3p_http_ota_create();
#endif
            return status;
        }
        else
        {
            status = pdFAIL;
            IOT_ERROR("OTA URL string invalid")
        }
    }
    else if (rcode != FSP_SUCCESS)
    {
        status = pdFAIL;
        IOT_ERROR("OTA Flag read failed")
    }
    else
    {
        status = pdFAIL;
        IOT_ERROR("OTA Flag is not set ready")
    }

    return status;
}

static char* getAppThingName(void)
{
    char *nvramName = NULL;

#ifdef RM_MAP_PERSISTANT_W
    if (RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                        AWSIOT_CFG_THINGNAME, &nvramName))
    {
        nvramName = (char *)AWS_IOT_THING_NAME;
    }
#else
    nvramName = (char *)AWS_IOT_THING_NAME;
#endif

    return nvramName;
}

void aws_s3_ota_app_start(void *arg)
{
    RA6W1_UNUSED_ARG(arg);

    APRINTF_S("\n===========================================\n\n");
    APRINTF("\nAWS_IOT S3 OTA Update Demo for \"%s\" \n\n", getAppThingName());
    APRINTF_S("============================================\n\n");
    aws_s3_ota_validate_and_start_thread();
    vTaskDelete(NULL);
}
