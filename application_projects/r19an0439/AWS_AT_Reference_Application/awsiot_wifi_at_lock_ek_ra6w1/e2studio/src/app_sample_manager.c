/***********************************************************************************************************************
 * File Name    : app_sample_manager.c
 * Description  : Make the device thingname for AWS platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdarg.h"
#include "common_def.h"
#include "common_utils.h"
#include "rm_vee_flash_w_rrq_nvram.h"
/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#include "app_sample_manager.h"
#include "app_dpm_thread.h"

#if defined(RM_MAP_PERSISTANT_W)
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#endif

#undef	TIME_CHECK_DBG //enable or disable of awsiot_app_print_elapse_time_ms()

/*! getting current time after booting   */
extern int ra6w1_vsnprintf(char *buf, size_t n, int linefeed,  const char *fmt, va_list args);

/*! heap size at last time   */
uint32_t previous_heap_size = 0;
/*! heap size at first time   */
uint32_t first_heap_size = 0;
#if defined ( __SUPPORT_AWS_IOT_W__ )
#include "iface_defs.h"
#endif // ( __SUPPORT_AWS_IOT_W__ ) 

static char* getMemoryString(const char *string_data)
{
    size_t strLen;
    char *strTemp;

    strLen = strlen(string_data);
    strTemp = (char *)pvPortMalloc(strLen + 1);
    if (strTemp != NULL)
    {
        memset(strTemp, 0x00, (strLen + 1));
        strcpy(strTemp, string_data);
    }

    return strTemp;
}

char* getAppThingName(void)
{
    char *nvramName = NULL;
    char *app_thing_name = NULL;

#if defined(__SUPPORT_ATCMD__)
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_THINGNAME, &nvramName);
#else
#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_CONFIG_THINGNAME, &nvramName);
#else
    nvramName = read_nvram_appcfg_string(APP_CONFIG_THINGNAME);
#endif 
#endif 

    if (nvramName == NULL)
    {
        app_thing_name = getMemoryString((const char*)AWS_IOT_THING_NAME);
    }
    else
    {
        app_thing_name = nvramName;
    }

    return app_thing_name;
}
