/***********************************************************************************************************************
 * File Name    : app_sample_manager.c
 * Description  : Make the device thingname for AWS platform also provide fleet provisioning status.
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
uint8_t statusFleetProv = 0;
char fleet_prov_thing_name[128] = {0, };

extern int getMacAddrMswLsw(UINT iface, ULONG *macmsw, ULONG *maclsw);

uint8_t getFleetProvStatus(void)
{
#if defined (__SUPPORT_ATCMD_WORK__)
    int nvramFPSet = 0;
#ifdef RM_MAP_PERSISTANT_W
    if(RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, USE_FLEET_PROVISION, &nvramFPSet) != FSP_SUCCESS);
        statusFleetProv = 0;
    else
#endif // RM_MAP_PERSISTANT_W
        statusFleetProv = (uint8_t)nvramFPSet;
#endif // (__SUPPORT_ATCMD__)    

    return statusFleetProv;
}

static char* getFleetProvAppThingName()
{
    ULONG macmsw = 0, maclsw = 0;

    getMacAddrMswLsw(WLAN0_IFACE, &macmsw, &maclsw);
    sprintf(fleet_prov_thing_name, "%s_%02lX%02lX%02lx", FP_DEMO_ID_SUFFIX, ((maclsw >> 16) & 0x0ff), ((maclsw >> 8) & 0x0ff),
            ((maclsw >> 0) & 0x0ff));

    return (char*)fleet_prov_thing_name;
}
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

#if defined ( __SUPPORT_AWS_IOT_W__ )
    if (nvramName == NULL && getFleetProvStatus() == 1)
    {
        app_thing_name = (char*)getFleetProvAppThingName();
    }
    else if (nvramName == NULL && getFleetProvStatus() == 0)
    {
        app_thing_name = getMemoryString((const char*)AWS_IOT_THING_NAME);
    }
    else
    {
        if (getFleetProvStatus() == 1 && strncmp(nvramName, getFleetProvAppThingName(), strlen(getFleetProvAppThingName())) != 0)
        {
            app_thing_name = (char*)getFleetProvAppThingName();
        }
        else
        {
            app_thing_name = nvramName;
        }
    }
#else
    if (nvramName == NULL)
    {
        app_thing_name = getMemoryString((const char*)APP_USER_MY_THING_NAME);
    }
    else
    {
        app_thing_name = nvramName;
    }
#endif //( __SUPPORT_AWS_IOT_W__ ) 

    return app_thing_name;
}
