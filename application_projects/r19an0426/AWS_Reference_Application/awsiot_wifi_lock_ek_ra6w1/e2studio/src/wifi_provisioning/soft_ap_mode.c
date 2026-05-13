/***********************************************************************************************************************
* File Name    : soft_ap_mode.c
* Description  : config soft ap mode
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "lwip/prot/dhcp.h"
#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "rm_map_persistant_w.h"
#include "bsp_common.h"
#include "common_data.h"
#include "provisioning.h"
#include "provisioning_cfg.h"

#include dg_configADNVPARAM_PROJ_FILE

struct setup_params
{
    char country_code[4];
    unsigned char sysmode;
    unsigned char band;
    unsigned char channel;
    unsigned char wifi_mode;
#if CFG_PMGR
    unsigned char enable_dpm;
    int dpm_keepalive_time;
    int dpm_user_wakeup_time;
    int dpm_TIM_wakeup_count;
#endif //CFG_PMGR
    char ssid[2][wificonfigMAX_SSID_LEN + 1];
    bool hidden_ssid;
    WIFIPmf_t pmf[2];
    WIFISecurityExt_t security[2];
    char password[2][wificonfigMAX_PASSPHRASE_LEN + 1];
    char wep_key[wificonfigMAX_WEPKEY_LEN + 1];
    uint8_t wep_key_idx;
    unsigned char wep_key_type;
    unsigned char wep_bit;
    unsigned char eap_auth_mode;
    unsigned char eap_phase2;
    char eap_id[wificonfigMAX_ENT_IDENTITY_LEN + 1];
    char eap_pw[wificonfigMAX_ENT_PASSWORD_LEN + 1];
    unsigned char p2p_listen_chan;
    unsigned char p2p_go_intent;
    unsigned char netmode[2];
    char ipaddress[2][16];
    char subnetmask[2][16];
    char gateway[2][16];
    char dns[2][16];
#if (defined __SUPPORT_WPA3_SAE__ && defined __SUPPORT_WPA3_PERSONAL__) || defined __SUPPORT_MESH__
    char sae_groups[2][wificonfigMAX_SAE_GROUPS_LEN];
#endif // __SUPPORT_WPA3_SAE__ || __SUPPORT_MESH__
};

#define EASY_SETUP_TASK_EVENT_FINISH     BIT1
#define	COUNTRY_CODE_DEFAULT		     "KR"
#define	CHANNEL_AUTO				     0
#define	CHANNEL_DEFAULT				     CHANNEL_AUTO

extern map_persistant_w_instance_ctrl_t g_map_persistant_w_ctrl;

static inline int RM_MAP_PERSISTANT_W_Read_INT_or_zero(const char *name)
{
    int ret;
    fsp_err_t err = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, name, &ret);
    return FSP_SUCCESS == err ? ret : 0;
}

static void usr_api_load_params(struct setup_params *params)
{
    char * result_ptr;

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

    return;
}

void execute_AP_profile(void)
{
    WIFIReturnCode_t wifi_err;
    WIFINetworkParamsExt_t *net_params = NULL;
    struct setup_params *params = pvPortMalloc(sizeof(struct setup_params));

    if (!params)
        return;

    usr_api_load_params(params);
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
        net_params->ucBand = eWiFiBand2G;
    else if (params->band == WPA_SETBAND_5G)
        net_params->ucBand = eWiFiBand5G;
    else
        net_params->ucBand = eWiFiBandDual;

    net_params->ucWiFi_mode = params->wifi_mode - GAP_USER_CONFIGURE_WIFI_MODE;

    if (params->security[WLAN1_IFACE] == eWiFiSecurityWPA3_ext ||
        params->security[WLAN1_IFACE] == eWiFiSecurityWPA2_WPA3_ext)
        strncpy(net_params->sae_groups,  params->sae_groups[WLAN1_IFACE], wificonfigMAX_SAE_GROUPS_LEN);

    wifi_err = WIFI_ConfigureAPExt(net_params);
    vPortFree(net_params);
    vPortFree(params);

    if (wifi_err)
        printf("%s: WIFI_ConfigureAPExt failed with wifi_err=%d\n", __func__, wifi_err);

    wifi_err = WIFI_StartAP();
    if (wifi_err)
        printf("%s: WIFI_StartAP failed with wifi_err=%d\n", __func__, wifi_err);

    printf("Start AP...\n");

    return;
}
