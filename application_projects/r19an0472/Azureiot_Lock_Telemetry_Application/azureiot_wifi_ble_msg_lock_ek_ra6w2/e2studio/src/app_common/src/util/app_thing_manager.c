/**
 ****************************************************************************************
 *
 * @file app_thing_manager.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_def.h"

#include "common_utils.h"
#include "rm_vee_flash_w_rrq_nvram.h"
#include "app_common_util.h"
#include "app_common_memory.h"

#include "app_thing_manager.h"
#include "app_dpm_thread.h"
char *app_thing_name = NULL;
#if defined(RM_MAP_PERSISTANT_W)
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#endif

#if defined ( __SUPPORT_AZURE_IOT__ )
char *app_dev_primary_key = NULL;
char *app_host_name = NULL;
char *app_iothub_conn_string = NULL;
#endif

int provision_feature = GENERIC;

#if defined ( __SUPPORT_AWS_IOT__ )
#include "iface_defs.h"

char tThingName[128] = {0, };

/* 
 * NOT SUPPORT FLEET PROVISIONING
 *   Fleet Provisioning Usage flag
 *   0: Fleet Provisioning disable
 *   1: Fleet Provisioning enable
 *   uint8_t statUseFleetProvision = 0;
 */

uint8_t getUseFPstatus(void)
{
/*
 * NOT SUPPORT FLEET PROVISIONING
 *   #if defined (__SUPPORT_ATCMD__)
 *       int nvramFPSet = 0;
 *       if (read_nvram_int(USE_FLEET_PROVISION, &nvramFPSet))
 *           statUseFleetProvision = 0;
 *       else
 *           statUseFleetProvision = (uint8_t)nvramFPSet;
 *   #endif // (__SUPPORT_ATCMD__)

 *    return statUseFleetProvision;
 */
    return 0;
}

char* getFPAppThingName()
{
    ULONG macmsw = 0, maclsw = 0;

    getMacAddrMswLsw(WLAN0_IFACE, &macmsw, &maclsw);
    da16x_sprintf(tThingName, "%s_%02lX%02lX%02lx", FP_DEMO_ID_SUFFIX, ((maclsw >> 16) & 0x0ff), ((maclsw >> 8) & 0x0ff),
        ((maclsw >> 0) & 0x0ff));

    return (char*)tThingName;
}
#endif // ( __SUPPORT_AWS_IOT__ ) 

char* getMemString(const char *string_data)
{
    size_t strLen;
    char *strTemp;

    strLen = strlen(string_data);

    strTemp = (char *)pvPortMalloc(strLen + 1);
    memset(strTemp, 0x00, (strLen + 1));
    strncpy(strTemp, string_data, strLen);

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
	RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_NVRAM_CONFIG_THINGNAME, &nvramName);
#else
	nvramName = read_nvram_appcfg_string(APP_NVRAM_CONFIG_THINGNAME);
#endif 
#endif 

#if defined ( __SUPPORT_AWS_IOT__ )
    if (nvramName == NULL && getUseFPstatus() == 1) {
        app_thing_name = (char*)getFPAppThingName();
    } else if (nvramName == NULL && getUseFPstatus() == 0) {
        app_thing_name = getMemString((const char*)APP_USER_MY_THING_NAME);
    } else {
        if (getUseFPstatus() == 1 && strncmp(nvramName, getFPAppThingName(), strlen(getFPAppThingName())) != 0) {
            app_thing_name = (char*)getFPAppThingName();
        } else {
            app_thing_name = nvramName;
        }
    }
#else
    if (nvramName == NULL) {
        app_thing_name = getMemString((const char*)APP_USER_MY_THING_NAME);
    } else {
        app_thing_name = nvramName;
    }
#endif //( __SUPPORT_AWS_IOT__ ) 
    return app_thing_name;
}

#if defined ( __SUPPORT_AZURE_IOT__ )
char* getAppDevPrimaryKey(void)
{
    char *nvramName = NULL;

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_NVRAM_CONFIG_DEV_PRIMARY_KEY, &nvramName);
#endif 
    if (nvramName == NULL) {
        app_dev_primary_key = getMemString((const char*)APP_USER_MY_DEV_PRIMARY_KEY);
    } else {
        app_dev_primary_key = nvramName;
    }

    return app_dev_primary_key;
}

char* getAppHostName(void)
{
    char *nvramName = NULL;

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_NVRAM_CONFIG_HOST_NAME, &nvramName);
#endif 

    if (nvramName == NULL) {
        app_host_name = getMemString((const char*)APP_USER_MY_HOST_NAME);
    } else {
        app_host_name = nvramName;
    }

    return app_host_name;
}

char* getAppIotHubConnString(void)
{
    char *nvramName = NULL;

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_NVRAM_CONFIG_IOTHUB_CONN_STRING, &nvramName);
#endif

    if (nvramName == NULL) {
        app_iothub_conn_string = getMemString((const char*)APP_USER_MY_IOTHUB_CONN_STRING);
    } else {
        app_iothub_conn_string = nvramName;
    }

    return app_iothub_conn_string;
}
#endif

PROV_FEATURES get_prov_feature(void)
{
    return (PROV_FEATURES)provision_feature;
}

void set_prov_feature(PROV_FEATURES flag)
{
    provision_feature = flag;
}

/* EOF */
