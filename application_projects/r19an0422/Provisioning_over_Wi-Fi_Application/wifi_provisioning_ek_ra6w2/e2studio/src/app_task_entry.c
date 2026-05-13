/***********************************************************************************************************************
* File Name    : app_task_entry.c
* Description  : app main and wifi init
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "app_task.h"
#include "lwip/prot/dhcp.h"
#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "rm_dhcp.h"
#include "rm_map_persistant_w.h"
#include "bsp_common.h"
#include "net_common.h"
#include "net_dhcp_client.h"
#include "net_sntp_client.h"
#include "inc/provisioning.h"
#include "common_data.h"
#include "common_utils.h"
#include "inc/provisioning_api.h"
#include dg_configADNVPARAM_PROJ_FILE

#ifndef SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL
#define SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    3600*36 /*131072*/
#endif
#ifndef DHCPCLIENT
#define DHCPCLIENT 1 // iface_defs.h
#endif
#define WEP_KEY_INDEX 0

static unsigned char sntp_client_param;
static int sntp_client_period_time;
static char sntp_gmt_timezone[8];
static int sntp_timezone_int;
static char sntp_svr_addr[256];
static char sntp_svr_addr1[256];
static char sntp_svr_addr2[256];

static char * unquote(uint8_t *, char *, const char *);
static void execute_WIFI_profile(void);
static void load_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
static void load_provisioning_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
static void store_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
static void execute_SNTP(unsigned char sntp_client_param, int client_period_time, int timezone_int,
                         char *p_svr_addr, char *p_svr_addr1, char *p_svr_addr2);
static unsigned char get_ready_DHCP(void);

#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "rm_prov_w"
#define EP_APP_DESCRIPTION \
            "This example demonstrates the provisioning process for connecting to a Wi-Fi network."

static char * unquote(uint8_t *len, char *output, const char *input)
{
    const char qt = '\"';

    if (*len > 2 && input[0] == qt && input[*len - 1] == qt)
    {
        memcpy(output, input + 1, *len - 2);
        *len -= 2;
    }
    else
    {
        memcpy(output, input, *len);
    }

    return output;
}

static void load_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params)
{
    char *result_ptr = NULL;
    int encryption = 0;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
    {
        strncpy((char*)params->ucSSID, result_ptr, strlen(result_ptr));
        params->ucSSIDLength = strlen((char*)params->ucSSID);
    }

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, &encryption);
    params->xSecurity = encryption;

    if(params->xSecurity == eWiFiSecurityWEP)
    {
        result_ptr = NULL;
        params->ucDefaultWEPKeyIndex = WEP_KEY_INDEX;

        RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPKEY0_0, &result_ptr);
        if (result_ptr && strlen(result_ptr) > 0)
        {
            uint8_t len = strlen(result_ptr);

            unquote(&len, (char *)params->xPassword.xWEP[WEP_KEY_INDEX].cKey, result_ptr);
            params->xPassword.xWEP[WEP_KEY_INDEX].ucLength = len;
        }
    }
    else
    {
       result_ptr = NULL;
       RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, &result_ptr);
       if (result_ptr && strlen(result_ptr) > 0)
       {
           strncpy((char*)params->xPassword.xWPA.cPassphrase, result_ptr, strlen(result_ptr));
           params->xPassword.xWPA.ucLength = strlen(params->xPassword.xWPA.cPassphrase);
       }
    }
}

static void load_provisioning_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl,
                                           WIFINetworkParams_t *params)
{
    WIFINetworkParams_t net_params = {0};
    char *result_ptr = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFICFG, NVR_KEY_SSID_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
    {
        net_params.ucSSIDLength = strlen(result_ptr);
        unquote(&net_params.ucSSIDLength, (char *) net_params.ucSSID, result_ptr);
    }

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFICFG, NVR_KEY_AUTH_TYPE_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
    {
        if (strcmp(result_ptr, MODE_AUTH_WPA_PSK_STR) == 0)
        {
            net_params.xSecurity = eWiFiSecurityWPA2;
            result_ptr = NULL;
            RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFICFG, NVR_KEY_ENCKEY_0, &result_ptr);
            if (result_ptr && strlen(result_ptr) > 0)
            {
                net_params.xPassword.xWPA.ucLength = strlen(result_ptr);
                unquote(&net_params.xPassword.xWPA.ucLength,
                        (char *) net_params.xPassword.xWPA.cPassphrase, result_ptr);
            }
        }
        else if (strcmp(result_ptr, MODE_AUTH_WEP_STR) == 0)
        {
            net_params.xSecurity = eWiFiSecurityWEP;
            net_params.ucDefaultWEPKeyIndex = WEP_KEY_INDEX;
            result_ptr = NULL;

            RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFICFG, NVR_KEY_WEPKEY0_0, &result_ptr);
            if (result_ptr && strlen(result_ptr) > 0)
            {
                uint8_t len = strlen(result_ptr);
                if (!(len == 5 || len == 13 || len == 10 || len == 26))
                {
                    APP_PRINT_ERR("Invalid WEP key length: %d\n", len);
                    net_params.xSecurity = eWiFiSecurityOpen; // fallback to open network
                }
                else
                {
                    net_params.xPassword.xWEP[WEP_KEY_INDEX].ucLength = strlen(result_ptr);
                    unquote(&net_params.xPassword.xWEP[WEP_KEY_INDEX].ucLength,
                            (char *) net_params.xPassword.xWEP[WEP_KEY_INDEX].cKey, result_ptr);
                }
            }
        }
        else
        {
            net_params.xSecurity = eWiFiSecurityOpen;
        }
    }
    else
    {
        net_params.xSecurity = eWiFiSecurityOpen;
    }

    memcpy(params, &net_params, sizeof(WIFINetworkParams_t));
}

static void store_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params)
{
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, (char*)params->ucSSID);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, params->xSecurity);
    if (params->xSecurity == eWiFiSecurityWEP)
    {
        params->ucDefaultWEPKeyIndex = WEP_KEY_INDEX;
        RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPKEY0_0, params->xPassword.xWEP[WEP_KEY_INDEX].cKey);
    }
    else
        RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0,
                                         (char*)params->xPassword.xWPA.cPassphrase);

    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);
}

static unsigned char get_ready_DHCP(void)
{
    int iface = 0;    //iface_select
    struct netif *netif = NULL;

    netif = netif_get_by_index(iface+2);

    return (dhcp_get_state(netif) == DHCP_STATE_BOUND);
}

static void execute_SNTP(unsigned char sntp_client_param, int client_period_time,
                         int timezone_int, char *p_svr_addr,
                         char *p_svr_addr1, char *p_svr_addr2)
{

    if (sntp_client_param)
    {
        long temp;
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                      NVR_KEY_SNTP_SYNC_PERIOD, client_period_time);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                         NVR_KEY_SNTP_SERVER_DOMAIN, p_svr_addr);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                         NVR_KEY_SNTP_SERVER_DOMAIN_1, p_svr_addr1);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                         NVR_KEY_SNTP_SERVER_DOMAIN_2, p_svr_addr2);
#else
        write_nvram_syscfg_int(NVR_KEY_SNTP_SYNC_PERIOD, client_period_time);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN, p_svr_addr);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_1, p_svr_addr1);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_2, p_svr_addr2);
#endif


        temp = ((long)timezone_int / 60) * 60;
        if (timezone_int != 0)
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                        NVR_KEY_TIMEZONE, timezone_int);
#else
            write_nvram_syscfg_int(NVR_KEY_TIMEZONE, timezone_int);
#endif
            R_RTC_W_CalendarTimeZoneSet(R_RTC_W_GetCtrl(), &temp);
        }
        else
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_TIMEZONE);
#else
            delete_nvram_syscfg_env(NVR_KEY_TIMEZONE);
#endif
        }

        /* Set run flag */
        set_sntp_use(1);
    }
}

static void execute_WIFI_profile()
{
    static WIFINetworkParamsExt_t pxNetworkParamsExt __attribute__((aligned(4)));
    memset(&pxNetworkParamsExt, 0, sizeof(pxNetworkParamsExt));

    int is_profile_present = 0;
    int hidden_ssid = 0;
    bool is_ssid_hidden = false;

    map_persistant_w_instance_ctrl_t *p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, &is_profile_present);
    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_HIDDEN_SSID, &hidden_ssid);
    is_ssid_hidden = (hidden_ssid == 1);

    if (is_profile_present == 1)
    {
        APP_PRINT_INFO("Loading wifi profile..\n");
        load_WIFI_profile(p_ctrl, &(pxNetworkParamsExt.xNetworkParams));
    }
    else
    {
        APP_PRINT_INFO("Loading provisioning wifi profile..\n");
        load_provisioning_WIFI_profile(p_ctrl, &(pxNetworkParamsExt.xNetworkParams));
    }

    if (pxNetworkParamsExt.xNetworkParams.ucSSIDLength == 0)
    {
        APP_PRINT_INFO("No wifi profile found\n");

        return;
    }

    pxNetworkParamsExt.hidden_ssid = is_ssid_hidden;
    memset(&pxNetworkParamsExt.xEntNetParams, 0, sizeof(WIFIEnterpriseNetParams_t));
    memset(&pxNetworkParamsExt.xApNetParams, 0,sizeof(WIFIApNetParams_t));
    pxNetworkParamsExt.ucBand = eWiFiBandDual;
    pxNetworkParamsExt.ucWiFi_mode = WIFI_MODE_AUTO_STA;
    pxNetworkParamsExt.pmf = PMF_DEFAULT;
    pxNetworkParamsExt.sae_groups[0] = '\0';

    //check SNTP params
    sntp_client_param = 1;
    sntp_client_period_time = 3600 * (SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600);
    strcpy(sntp_gmt_timezone, "00:00");
    sntp_timezone_int = 0;
    memset(sntp_svr_addr, 0, 256);
    strcpy(sntp_svr_addr, DFLT_SNTP_SERVER_DOMAIN);
    memset(sntp_svr_addr1, 0, 256);
    strcpy(sntp_svr_addr1, DFLT_SNTP_SERVER_DOMAIN_1);
    memset(sntp_svr_addr2, 0, 256);
    strcpy(sntp_svr_addr2, DFLT_SNTP_SERVER_DOMAIN_2);

    //apply SNTP params
    execute_SNTP(sntp_client_param, sntp_client_period_time, sntp_timezone_int, sntp_svr_addr, sntp_svr_addr1, sntp_svr_addr2);

    APP_PRINT_INFO("SSID: %s\n", (char*)pxNetworkParamsExt.xNetworkParams.ucSSID);
    APP_PRINT_INFO("Encryption: %d\n", pxNetworkParamsExt.xNetworkParams.xSecurity);
    if(pxNetworkParamsExt.xNetworkParams.xSecurity == eWiFiSecurityWEP)
        {
            APP_PRINT_INFO("WEP key length: %d\n",
                           pxNetworkParamsExt.xNetworkParams
                           .xPassword.xWEP[pxNetworkParamsExt.xNetworkParams.ucDefaultWEPKeyIndex].ucLength);
        }
        else if(pxNetworkParamsExt.xNetworkParams.xSecurity == eWiFiSecurityWPA2)
        {
            APP_PRINT_INFO("Passphrase: %s\n",
                            (char*)pxNetworkParamsExt.xNetworkParams.xPassword.xWPA.cPassphrase);
        }

    WIFI_ConnectAPExt(&(pxNetworkParamsExt));

    if (is_profile_present != 1)
    {
        APP_PRINT_INFO("Storing wifi profile..");
        store_WIFI_profile(p_ctrl, &(pxNetworkParamsExt.xNetworkParams));
    }

    if (!RM_PMGR_W_dpm_is_wakeup())
    {
        if (get_netmode(0) == DHCPCLIENT)
        {
            while (!get_ready_DHCP())
            {
                APP_PRINT_INFO("Waiting DHCP bound...\n");
                vTaskDelay( pdMS_TO_TICKS( 1000 ) );
            }
            /* apply DNS params */
            set_dhcpCientIP_to_staticIP();
        }
    }
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);

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

#if (1 == AWS_IOT_DPM_APP_ENABLE)
    /*
        Open PMGR (PMGR should be opened once globally)
        internally call add_sleep_constraint(..., PMGR_CONSTRAINT_POWER_RAM) to block sleep
    */
    g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);

    /* Set job name for PMGR */
    RM_PMGR_W_dpm_job_name_set("my_main", 0);

    /* Add wake source PMGR */
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    /* Add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RETENTION);
#endif //(1 == AWS_IOT_DPM_APP_ENABLE)
#endif //CFG_WIFI

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

    WIFI_On();
    g_external_irq3.p_api->open(&g_external_irq3_ctrl, g_external_irq3.p_cfg);
    g_external_irq3.p_api->enable(&g_external_irq3_ctrl);
#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    provision_app_gpio_handle_create_event();

    /* Start GPIO event task */
    provision_app_gpio_handle_task_start();
#endif //defined(__SUPPORT_FACTORY_RESET_BTN__)

    WIFIDeviceMode_t mode = eWiFiModeStation;
    WIFI_GetMode(&mode);
    if (eWiFiModeAP == mode || eWiFiModeAPStation == mode)
    {
        init_wifi_provisioning();
    }
    else
    {
        if (!RM_PMGR_W_dpm_is_wakeup())
        {
            execute_WIFI_profile();
        }
    }

    while (1)
    {
        vTaskDelay (10);
    }
}
