/***********************************************************************************************************************

* File Name    : new_thread0_entry.c

* Description  : Contains Wi-Fi provisioning logic including loading, storing, and executing Wi-Fi profiles 

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/
#include "new_thread0.h"
#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "rm_map_persistant_w.h"
#include "bsp_common.h"
#include "sys_feature.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "rm_cli_w.h"
#include dg_configADNVPARAM_PROJ_FILE

#define EASY_SETUP_TASK_EVENT_FINISH BIT1

extern TaskHandle_t cli_handle_task;

static void load_wifi_profile(map_persistant_w_instance_ctrl_t *, WIFINetworkParamsExt_t *);
static void load_default_wifi_profile(WIFINetworkParamsExt_t *);
static void store_wifi_profile(map_persistant_w_instance_ctrl_t *, WIFINetworkParamsExt_t *);
static void execute_wifi_profile(void);

static void load_wifi_profile(map_persistant_w_instance_ctrl_t * p_ctrl,WIFINetworkParamsExt_t * params)
{
    char *result_ptr;
    int result = 0;

    if (RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_SYS_MODE, &result) != FSP_SUCCESS)
    {
        rm_wifi_set_mode(WIFI_DEVICE_MODE_EXT_STATION);
    }
    else
    {
        rm_wifi_set_mode(result);
    }

    result = 0;

    if (RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_BAND, &result) != FSP_SUCCESS)
    {
        params->ucBand = (WIFIBand_t)WPA_SETBAND_2G;
    }
    else
    {
        params->ucBand =  result;
    }

    result = 0;

    if (RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_CHANNEL,&result) != FSP_SUCCESS)
    {
        params->xNetworkParams.ucChannel = CHANNEL_DEFAULT;
    }
    else
    {
        params->xNetworkParams.ucChannel = result;
    }

    result_ptr = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, &result_ptr);

    if (result_ptr)
    {
        strcpy((char *)params->xNetworkParams.ucSSID, result_ptr);
        params->xNetworkParams.ucSSIDLength = strlen((char *)params->xNetworkParams.ucSSID);
    }

    result_ptr = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, &result_ptr);

    if (result_ptr)
    {
        strcpy((char *)params->xNetworkParams.xPassword.xWPA.cPassphrase, result_ptr);
        params->xNetworkParams.xPassword.xWPA.ucLength = strlen((char *)params->xNetworkParams.xPassword.xWPA.cPassphrase);
    }

    result = 0;

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, &result);

    params->xNetworkParams.xSecurity = result;

    result = 0;

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_PMF_0, &result);

    params->pmf = result;
}

static void load_default_wifi_profile(WIFINetworkParamsExt_t *params)
{
    rm_wifi_set_mode(WIFI_DEVICE_MODE_EXT_STATION);
    params->ucBand = (WIFIBand_t)WPA_SETBAND_2G;
    params->xNetworkParams.ucChannel = CHANNEL_DEFAULT;
    strcpy((char *)params->xNetworkParams.ucSSID, "RenesasMatter");
    params->xNetworkParams.ucSSIDLength = strlen((char *)params->xNetworkParams.ucSSID);
    strcpy((char *)params->xNetworkParams.xPassword.xWPA.cPassphrase, "Matter2023");
    params->xNetworkParams.xPassword.xWPA.ucLength = strlen((char *)params->xNetworkParams.xPassword.xWPA.cPassphrase);
    params->xNetworkParams.xSecurity = eWiFiSecurityWPA2;
}

static void store_wifi_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParamsExt_t *params)
{
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_BAND, params->ucBand);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_CHANNEL, params->xNetworkParams.ucChannel);
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, (char *)params->xNetworkParams.ucSSID);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, params->xNetworkParams.xSecurity);
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, (char *)params->xNetworkParams.xPassword.xWPA.cPassphrase);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_PMF_0, params->pmf);
}

static void execute_wifi_profile()
{
    WIFINetworkParamsExt_t *xNetworkParams = NULL;
    int is_profile_present = 0;
    map_persistant_w_instance_ctrl_t *p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();
    WIFIReturnCode_t wifi_err;

    if(MAP_PERSISTANT_W_OPEN != p_ctrl->map_persistant_w_open)
    {
        printf("MAP_PERSISTANT_W is not open\n");
        return;
    }

    xNetworkParams = pvPortMalloc(sizeof(WIFINetworkParamsExt_t));

    if (!xNetworkParams)
    {
        return;
    }

    memset(xNetworkParams, 0, sizeof(WIFINetworkParamsExt_t));

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, &is_profile_present);

    if(is_profile_present == 1)
    {
        printf("Loading wifi profile..\n");
        load_wifi_profile(p_ctrl, xNetworkParams);
    }
    else
    {
        printf("Loading default wifi profile..\n");
        load_default_wifi_profile(xNetworkParams);
    }

    printf("SSID: %s\n", (char *)xNetworkParams->xNetworkParams.ucSSID);
    printf("Passphrase: %s\n", (char *)xNetworkParams->xNetworkParams.xPassword.xWPA.cPassphrase);
    printf("Encryption: %d\n", xNetworkParams->xNetworkParams.xSecurity);

    if(is_profile_present != 1)
    {
        printf("Storing default wifi profile..");
        store_wifi_profile(p_ctrl, xNetworkParams);
    }

    wifi_err = WIFI_ConnectAPExt(xNetworkParams);

    vPortFree(xNetworkParams);

    if (wifi_err)
    {
        printf("%s: WIFI_ConnectAP failed with wifi_err=%d\n", __func__, wifi_err);
    }
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
#if CFG_WIFI

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                               g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
                                                             g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);

    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);

    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);

    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif //WIFI_CFG_WATCHDOG_SERVICE_ENABLE

    /* Init CC312 HW engine and psa crypto */
    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif //SUPPORT_FSP_RM_OTA_W

#if CFG_CLI
    cli_open();
#endif //CFG_CLI

    xTaskNotify(cli_handle_task, EASY_SETUP_TASK_EVENT_FINISH, eSetBits);

    WIFI_On();
#if defined(__SUPPORT_WIFI_USER_GPIO__)
    rm_wifi_app_gpio_wakeup_set(BTN_WAKEUP_PIN, BSP_WAKEUP_EDGE_LOW, NULL);
#endif

#if CFG_PMGR
    if (!RM_PMGR_W_dpm_is_wakeup())
    {
        execute_wifi_profile();
    }
#endif /* CFG_PMGR */

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();
#if defined(__SUPPORT_WIFI_USER_GPIO__)
    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif//__SUPPORT_WIFI_USER_GPIO__
#endif //__SUPPORT_FACTORY_RESET_BTN__
#endif //CFG_WIFI

    MATTER_On();

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
