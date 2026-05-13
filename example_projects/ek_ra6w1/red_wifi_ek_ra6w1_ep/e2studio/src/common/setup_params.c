/***********************************************************************************************************************

* File Name    : setup_params.c

* Description  : Setting up parameters.

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/


/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "new_thread0.h"
#include "lwip/prot/dhcp.h"
#include "rm_atcmd_w_core.h"

#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "rm_map_persistant_w.h"
#include "bsp_common.h"
#include "common/setup_params.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include dg_configADNVPARAM_PROJ_FILE
#include "secure_asset/secure_storage.h"
#include "setup_params.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#ifndef SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL
#define SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    (3600 * 36)    // 131072
#endif
#ifndef DHCPCLIENT
#define DHCPCLIENT 1    // iface_defs.h
#endif

#define	COUNTRY_CODE_DEFAULT		     "KR"
#define	CHANNEL_AUTO				     0
#define	CHANNEL_DEFAULT				     CHANNEL_AUTO
#define WLAN1_IFACE			             1
#define ENV_SYS_MODE				     "SYSMODE"
#define STATIC_IP                        2

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/
extern map_persistant_w_instance_ctrl_t g_map_persistant_w_ctrl;
extern atcmd_w_core_instance_t g_at_core_instance;

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
softap_config_t ap_config_param_ex;

/***********************************************************************************************************************
 * Private Functions Prototypes
 **********************************************************************************************************************/
static char* unquote(uint8_t *len, char *output, const char *input);
static int RM_MAP_PERSISTANT_W_Read_INT_or_zero(const char *name);
static void load_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
static void load_provisioning_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
static void store_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params);
extern int get_netmode(int iface);
extern unsigned int set_sntp_use(int use);

static int sntp_client_period_time;
static int sntp_timezone_int;
static char sntp_gmt_timezone[8];
static char sntp_svr_addr[256];
static char sntp_svr_addr1[256];
static char sntp_svr_addr2[256];
static uint8_t sntp_client;

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
void provisioning_factory_reset(void)
{
    ap_info_t ap_info;

    if (R_RED_SecureAssetLoad(SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)) == FSP_SUCCESS)
    {
        factory_reset(0);
        R_RED_SecureAssetStore(SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t));
    }
    else
    {
        factory_reset(0);
    }
}

void execute_AP_profile(void)
{
    char *result_ptr;
    ap_info_t ap_info;
    struct setup_params *params = pvPortMalloc(sizeof(struct setup_params));

    /* Read from nvram and update params struct */
    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                    ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COUNTRY_CODE, &result_ptr);
    if (result_ptr)
        strcpy(params->country_code, result_ptr);
    else
        strcpy(params->country_code, COUNTRY_CODE_DEFAULT);

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_SYS_MODE, (int *)&params->sysmode) != FSP_SUCCESS)
        params->sysmode = WIFI_DEVICE_MODE_EXT_STATION;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_BAND, (int *)&params->band) != FSP_SUCCESS)
        params->band = WPA_SETBAND_2G;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_CHANNEL,(int *)&params->channel) != FSP_SUCCESS)
        params->channel = CHANNEL_DEFAULT;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_WIFI_MODE, (int *)&params->wifi_mode) != FSP_SUCCESS)
        params->wifi_mode = WIFI_MODE_BGN + GAP_USER_CONFIGURE_WIFI_MODE;

#if CFG_PMGR
    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_ENABLE_DPM, (int *)&params->enable_dpm) != FSP_SUCCESS)
        params->enable_dpm = DFLT_DPM;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME, &params->dpm_keepalive_time) != FSP_SUCCESS)
        params->dpm_keepalive_time = DFLT_DPM_KEEPALIVE_TIME;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_DPM_USER_WAKEUP_TIME, &params->dpm_user_wakeup_time) != FSP_SUCCESS)
        params->dpm_user_wakeup_time = DFLT_DPM_USER_WAKEUP_TIME;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                     WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT, &params->dpm_TIM_wakeup_count) != FSP_SUCCESS)
        params->dpm_TIM_wakeup_count = DFLT_DPM_TIM_WAKEUP_COUNT;
#endif /* CFG_PMGR */

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, &result_ptr);
    if (result_ptr)
        strcpy(params->ssid[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_1, &result_ptr);
    if (result_ptr)
        strcpy(params->ssid[1], result_ptr);

    params->hidden_ssid = (bool) RM_MAP_PERSISTANT_W_Read_INT_or_zero(WIFI_PROFILE_HIDDEN_SSID);

    params->pmf[0] = RM_MAP_PERSISTANT_W_Read_INT_or_zero(WIFI_PROFILE_PMF_0);
    params->pmf[1] = RM_MAP_PERSISTANT_W_Read_INT_or_zero(WIFI_PROFILE_PMF_1);

    params->security[0] = RM_MAP_PERSISTANT_W_Read_INT_or_zero(WIFI_PROFILE_SECURITY_0);
    params->security[1] = RM_MAP_PERSISTANT_W_Read_INT_or_zero(WIFI_PROFILE_SECURITY_1);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, &result_ptr);
    if (result_ptr)
        strcpy(params->password[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_1, &result_ptr);
    if (result_ptr)
        strcpy(params->password[1], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SAE_GROUPS_0, &result_ptr);
    if (result_ptr)
        strcpy(params->sae_groups[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SAE_GROUPS_1, &result_ptr);
    if (result_ptr)
        strcpy(params->sae_groups[1], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPKEY0_0, &result_ptr);
    if (result_ptr)
        strcpy(params->wep_key, result_ptr);

    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPINDEX_0, (int *)&params->wep_key_idx);
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPTYPE_0, (int *)&params->wep_key_type);

    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, (int *)&params->eap_auth_mode);
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_PHASE2, (int *)&params->eap_phase2);
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_ID, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
            strncpy(params->eap_id, result_ptr, strlen(result_ptr));

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_PW, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
            strncpy(params->eap_pw, result_ptr, strlen(result_ptr));

    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_P2P_LISTEN_CH, (int *)&params->p2p_listen_chan);
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_P2P_GO_INTENT, (int *)&params->p2p_go_intent);

    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_NETMODE_0, (int *)&params->netmode[0]);
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_NETMODE_1, (int *)&params->netmode[1]);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_IPADDR_0, &result_ptr);
    if (result_ptr)
        strcpy(params->ipaddress[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_IPADDR_1, &result_ptr);
    if (result_ptr)
        strcpy(params->ipaddress[1], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_NETMASK_0, &result_ptr);
    if (result_ptr)
        strcpy(params->subnetmask[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_NETMASK_1, &result_ptr);
    if (result_ptr)
        strcpy(params->subnetmask[1], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_GATEWAY_0, &result_ptr);
    if (result_ptr)
        strcpy(params->gateway[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_GATEWAY_1, &result_ptr);
    if (result_ptr)
        strcpy(params->gateway[1], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DNSSVR_0, &result_ptr);
    if (result_ptr)
        strcpy(params->dns[0], result_ptr);

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DNSSVR_1, &result_ptr);
    if (result_ptr)
        strcpy(params->dns[1], result_ptr);

    WIFIReturnCode_t wifi_err;
    WIFINetworkParamsExt_t * net_params = NULL;
    if (FSP_SUCCESS == R_RED_SecureAssetLoad(SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)))
    {
    	memcpy(params->ssid[1], ap_info.ssid, 32);
    	memcpy(params->password[1], ap_info.password, 32);
    }

    net_params = pvPortMalloc(sizeof(WIFINetworkParamsExt_t));
    if (!net_params)
        return;

    memset(net_params, 0, sizeof(WIFINetworkParamsExt_t));
    net_params->xNetworkParams.ucChannel = params->channel;

    net_params->xNetworkParams.ucSSIDLength = strlen(params->ssid[WLAN1_IFACE]);
    memcpy(net_params->xNetworkParams.ucSSID, params->ssid[WLAN1_IFACE], net_params->xNetworkParams.ucSSIDLength);

    net_params->pmf = params->pmf[WLAN1_IFACE];

    /* Casting only for solve compilation error */
    net_params->xNetworkParams.xSecurity = (WIFISecurity_t)params->security[WLAN1_IFACE];

    if (!(params->security[WLAN1_IFACE] == eWiFiSecurityWPA_ent_ext) &&
        !(params->security[WLAN1_IFACE] == eWiFiSecurityWPA2_ent_ext) &&
        !(params->security[WLAN1_IFACE] == eWiFiSecurityWPA_WPA2_ent_ext) &&
        !(params->security[WLAN1_IFACE] == eWiFiSecurityWPA2_WPA3_ent_ext) &&
        !(params->security[WLAN1_IFACE] == eWiFiSecurityWPA3_ent_ext) &&
        !(params->security[WLAN1_IFACE] == eWiFiSecurityWPA3_192B_ent_ext))
    {
        net_params->xNetworkParams.xPassword.xWPA.ucLength = strlen(params->password[WLAN1_IFACE]);
        memcpy(net_params->xNetworkParams.xPassword.xWPA.cPassphrase, params->password[WLAN1_IFACE], net_params->xNetworkParams.xPassword.xWPA.ucLength);
    }

    if (params->band == WPA_SETBAND_2G)
    {
        net_params->ucBand = eWiFiBand2G;
    }
    else if (params->band == WPA_SETBAND_5G)
    {
        net_params->ucBand = eWiFiBand5G;
    }
    else
    {
        net_params->ucBand = eWiFiBandDual;
    }

    net_params->ucWiFi_mode = params->wifi_mode - GAP_USER_CONFIGURE_WIFI_MODE;

    if (params->security[WLAN1_IFACE] == eWiFiSecurityWPA3_ext ||
        params->security[WLAN1_IFACE] == eWiFiSecurityWPA2_WPA3_ext)
    {
        strncpy(net_params->sae_groups,  params->sae_groups[WLAN1_IFACE], wificonfigMAX_SAE_GROUPS_LEN);
    }

    wifi_err = WIFI_ConfigureAPExt(net_params);
    vPortFree(net_params);
    vPortFree(params);

    if (wifi_err)
    {
        printf("%s: WIFI_ConfigureAPExt failed with wifi_err=%d\n", __func__, wifi_err);
    }

    wifi_err = WIFI_StartAP();
    if (wifi_err)
    {
        printf("%s: WIFI_StartAP failed with wifi_err=%d\n", __func__, wifi_err);
    }
    return;
}

void execute_factory_default(void)
{
    char default_ssid[32] = {0, };
    char default_psk[32] = {0, };

    printf("\nRebootAPMode setting up Customer configuration ...\n");
    sprintf(default_ssid, "Renesas_IoT_WiFi");
    sprintf(default_psk, "1234567890");

    provisioning_factory_reset();

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_SSID_1,
                                     default_ssid);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));
    printf("\nSetting default_ssid = %s ...,\n", default_ssid );

    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, ENV_SYS_MODE, (int) DFLT_SYSMODE);
    set_sys_mode((int)eWiFiModeAP);

    if (strlen(default_psk) > 0)
    {
        char tmp_psk[MAX_PASSKEY_LEN + 3];
        char security = eWiFiSecurityWPA; 

        memset(tmp_psk, 0, MAX_PASSKEY_LEN + 3);
        tmp_psk[0] = 0x22;
        strcpy(&tmp_psk[1], default_psk);
        tmp_psk[strlen(default_psk) + 1] = 0x22;

        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_WIFIPROFILE,
                                         (const char *) WIFI_PROFILE_ENCKEY_1,
                                         tmp_psk);
        printf("\n PW = %s \n", tmp_psk);

        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_WIFIPROFILE,
                                      WIFI_PROFILE_SECURITY_1,
                                      security);
        printf("\n PW = %s  completed\n", tmp_psk);
    }

    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                  ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_NETMODE_1,
                                  (int) STATIC_IP);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                 ENV_GROUP_WIFIPROFILE,
                                 WIFI_PROFILE_BAND,
                                 (int) WPA_SETBAND_2G);
    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                  ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_CHANNEL,
                                  (int) DFLT_AP_CHANNEL);


    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_COUNTRY_CODE,
                                     (const char *) DFLT_AP_COUNTRY_CODE);


    printf("."); vTaskDelay(portCONVERT_MS_2_TICKS(10));

    sprintf(ap_config_param_ex.ip_addr, "%s", "10.0.0.1");
    sprintf(ap_config_param_ex.subnet_mask, "%s", "255.255.255.0");
    sprintf(ap_config_param_ex.default_gw, "%s", "10.0.0.1");
    sprintf(ap_config_param_ex.dns_ip_addr, "%s", "8.8.8.8");	// 10.0.0.1 check

    ap_config_param_ex.dhcpd_lease_time = 3600;
    sprintf(ap_config_param_ex.dhcpd_start_ip, "%s", "10.0.0.2");
    sprintf(ap_config_param_ex.dhcpd_end_ip, "%s", "10.0.0.11");

    printf("\nRebootAPMode setting IPADDR_CUSTOMER...\n");

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_IPADDR_1,
                                     ap_config_param_ex.ip_addr);
    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_NETMASK_1,
                                     ap_config_param_ex.subnet_mask);
    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_GATEWAY_1,
                                     ap_config_param_ex.default_gw);
    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_WIFIPROFILE,
                                     (const char *) WIFI_PROFILE_DNSSVR_1,
                                     ap_config_param_ex.dns_ip_addr);
    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));


    printf("\nRebootAPMode setting DHCPD_CUSTOMER...\n");


    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                  ENV_GROUP_SYSCFG,
                                  (const char *) NVR_KEY_DHCPD,
                                  MODE_ENABLE);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));


    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                  ENV_GROUP_SYSCFG,
                                  (const char *) NVR_KEY_DHCP_TIME,
                                  ap_config_param_ex.dhcpd_lease_time);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));


    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_SYSCFG,
                                     (const char *) NVR_KEY_DHCP_S_IP,
                                     ap_config_param_ex.dhcpd_start_ip);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));


    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                     ENV_GROUP_SYSCFG,
                                     (const char *) NVR_KEY_DHCP_E_IP,
                                     ap_config_param_ex.dhcpd_end_ip);

    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    printf("\nOK\n");
    printf(ANSI_COLOR_DEFULT "\n\n");
    vTaskDelay(portCONVERT_MS_2_TICKS(30));

    reset(0);

    /* Wait for system-reboot */
    while (1)
    {
        vTaskDelay(portCONVERT_MS_2_TICKS(100));
    }
}

static void execute_SNTP(uint8_t sntp_client, int client_period_time, int timezone_int,
                         char *p_svr_addr, char *p_svr_addr1, char *p_svr_addr2)
{
    if (sntp_client)
    {
        int32_t temp;
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

        temp = (timezone_int / 60) * 60;
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

void execute_WIFI_profile(void)
{
    WIFINetworkParams_t xNetworkParams = {0};
    int is_profile_present = 0;
    map_persistant_w_instance_ctrl_t * p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, &is_profile_present);
    if (1 == is_profile_present)
    {
        printf("Loading wifi profile..\n");
        load_WIFI_profile(p_ctrl, &xNetworkParams);
    }
    else
    {
        printf("Loading provisioning wifi profile..\n");
        load_provisioning_WIFI_profile(p_ctrl, &xNetworkParams);
    }

    if (0 == xNetworkParams.ucSSIDLength)
    {
        printf("No wifi profile found\n");
        return;
    }

    /* Check SNTP params */
    sntp_client = 1;
    sntp_client_period_time = 3600 * (SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600);
    strcpy(sntp_gmt_timezone, "00:00");
    sntp_timezone_int = 0;
    memset(sntp_svr_addr, 0, 256);
    strcpy(sntp_svr_addr, DFLT_SNTP_SERVER_DOMAIN);
    memset(sntp_svr_addr1, 0, 256);
    strcpy(sntp_svr_addr1, DFLT_SNTP_SERVER_DOMAIN_1);
    memset(sntp_svr_addr2, 0, 256);
    strcpy(sntp_svr_addr2, DFLT_SNTP_SERVER_DOMAIN_2);

    /* Apply SNTP params */
    execute_SNTP(sntp_client, sntp_client_period_time,
                 sntp_timezone_int, sntp_svr_addr, sntp_svr_addr1, sntp_svr_addr2);

    printf("SSID: %s\n", (char*)xNetworkParams.ucSSID);
    printf("Passphrase: %s\n", (char*)xNetworkParams.xPassword.xWPA.cPassphrase);
    printf("Encryption: %d\n", xNetworkParams.xSecurity);

    WIFI_ConnectAP(&(xNetworkParams));

    if (1 != is_profile_present)
    {
        printf("Storing wifi profile..");
        store_WIFI_profile(p_ctrl, &xNetworkParams);
    }
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
static int RM_MAP_PERSISTANT_W_Read_INT_or_zero(const char *name)
{
    int ret;
    fsp_err_t err = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, name, &ret);
    return FSP_SUCCESS == err ? ret : 0;
}

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
    int encryption = 0;
    char *result_ptr = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
    {
        strncpy((char *) params->ucSSID, result_ptr, strlen(result_ptr));
        params->ucSSIDLength = strlen((char *) params->ucSSID);
    }

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, &encryption);
    params->xSecurity = encryption;

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0)
    {
        strncpy((char *) params->xPassword.xWPA.cPassphrase, result_ptr, strlen(result_ptr));
        params->xPassword.xWPA.ucLength = strlen(params->xPassword.xWPA.cPassphrase);
    }
}

static void load_provisioning_WIFI_profile(map_persistant_w_instance_ctrl_t *p_ctrl, WIFINetworkParams_t *params)
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
        if ( 0 == strcmp(result_ptr, MODE_AUTH_WPA_PSK_STR))
        {
            net_params.xSecurity = eWiFiSecurityWPA2;
            result_ptr = NULL;
            RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFICFG, NVR_KEY_ENCKEY_0, &result_ptr);
            if (result_ptr && strlen(result_ptr) > 0)
            {
                net_params.xPassword.xWPA.ucLength = strlen(result_ptr);
                unquote(&net_params.xPassword.xWPA.ucLength,
                        (char *) net_params.xPassword.xWPA.cPassphrase,
                        result_ptr);
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

static void store_WIFI_profile(map_persistant_w_instance_ctrl_t * p_ctrl, WIFINetworkParams_t * params)
{
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, (char *) params->ucSSID);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, params->xSecurity);
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0,
                                     (char *) params->xPassword.xWPA.cPassphrase);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);
}
