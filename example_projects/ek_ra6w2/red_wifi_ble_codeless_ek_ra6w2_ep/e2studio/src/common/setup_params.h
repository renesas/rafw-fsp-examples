/***********************************************************************************************************************

* File Name    : setup_params.h

* Description  : Function declarations

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/


#ifndef __SETUP_PARAMS_H__
#define __SETUP_PARAMS_H__

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** This structure contains parameter information for rm_provision_w in the application. */
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

/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
void execute_WIFI_profile(void);
void execute_factory_default(void);
void execute_AP_profile(void);
void execute_factory_default(void);
void provisioning_factory_reset(void);

#endif
