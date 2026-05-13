/**
 ****************************************************************************************
 *
 * @file app_provision.h
 *
 * @brief reboot AP mode to concurrent or soft AP.
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

#if !defined (_APP_PROVISION_H_)
#define _APP_PROVISION_H_

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief DPM Set value for Provisioning.
 */
typedef enum {
    TYPE_SLEEP_MODE = 0,
    TYPE_USE_DPM,
    TYPE_RTC_TIME,
    TYPE_DPM_KEEP_ALIVE,
    TYPE_USER_WAKE_UP,
    TYPE_TIM_WAKE_UP,
} GetValueType;


/// Common Config - Wi-Fi Enterprise EAP Phase#1 values
typedef enum {
	/// WPA-Enterprise Phase1: Default ( PEAP / TTLS / FAST )
	TYPE_EAP_DEFAULT,
	/// WPA-Enterprise Phase1: PEAP
	TYPE_EAP_PEAP,
	/// WPA-Enterprise Phase1: EAP-TTLS
	TYPE_EAP_TTLS,
	/// WPA-Enterprise Phase1: EAP-FAST
	TYPE_EAP_FAST,
	/// WPA-Enterprise Phase1: EAP-TLS (Not support)
	TYPE_EAP_TLS,
} EAP_Type;

/// Common Config - Wi-Fi Enterprise EAP Phase#2 values
typedef enum {
	/// WPA-Enterprise Phase2: MSCHAPV2 GTC
    PROTO_EAP_PHASE2_MIX,
	/// WPA-Enterprise Phase2: EAP-MSCHAPv2
	PROTO_EAP_MSCHAPV2,
	/// WPA-Enterprise Phase2: EAP-GTC
	PROTO_EAP_GTC,
} EAP_Protocol;

/**
 * @brief Definition for the SoftAP Provisioning 
 */
/** @def Predefine SSID for provision */
#define PREDEFINE_SSID			"Dialog_DA16200"

/** @def Predefine Password */
#define PREDEFINE_PW			"1234567890"

#if defined (__SUPPORT_WIFI_PROVISIONING__)
/** @def maximum length of SSID */
#define PROV_MAX_SSID_LEN		128

/** @def maximum length of WPA style passkey */
#define PROV_MAX_PW_LEN			128	

/** @def maximum length of WEP style passkey */
#define PROV_MAX_WEP_KEY_LEN	16		

/** @def maximum  scan channel */
#define SCAN_CHANNEL_MAX		10

/** @def scan buffer. need change */
#define	PROV_SCAN_BUF_SIZE		3584

/** @def maximum scanning params. need to change */
#define MAX_CLI_PARAMS    	 	SCAN_CHANNEL_MAX+1 

/**
 ****************************************************************************************
 * @brief call-back fucntion type for receving AP list on AP mode after calling app_find_home_ap
 * @param[out] _index current index
 * @param[out] _SSID found SSID 
 * @param[out] _secMode security mode status 
 * @param[out] _signal signal level
 * @return  void
 ****************************************************************************************
 */
typedef void (*app_ap_list_from_scan_cb)(int _index, char *_SSID, int _secMode, int _signal);

/**
 * @brief Provisioning Parameters.
 * 
 * Defines a type containing provison parameters to be passed between device and apps to setup profile.
 */
typedef struct _app_prov_config {

    int32_t auto_restart_flag;						///< Auto reboot flag - 0: No reboot, 1: Auto reboot

    char ssid[PROV_MAX_SSID_LEN + 1];			///< SSID
    char psk[PROV_MAX_PW_LEN + 1];				///< WPA style passkey

    int32_t auth_type;								///< 0: OPEN, 1:WEP, 2:WPA-PSK, 3:WPA2-PSK, 4:WPA-AUTO

    int32_t wep_key_index;							///< WEP key index
    char wep_key[4][PROV_MAX_WEP_KEY_LEN + 1];	///< WEP key for each index

    /**
     * Country Code
     *
     * CA  US  USE USL USX FR  LT  LU  LV  NL
     * NO  NZ  PL  PT  SE  SI  SK  AT  HU  IE
     * IS  IT  HK  EE  ES  FI  GB  GR  DE  DK
     * CZ  CY  CH  AU  BR  BE  CN  ID  KR  MY
     * TH  TW  ZA  IL  SG  JP  ILO PH  IN  EU
     */
    char country[4];								///< country code string

    int32_t ip_addr_mode;							///< 0:DHCP Client, 1:STATIC

    int32_t sntp_flag;								///< 0:disable, 1:enable
    char sntp_server[32];						///< name of SNTP server
    int32_t sntp_period;							///< update period of SNTP

    int32_t dpm_mode;								///< 0:No DPM mode, 1:DPM mode
    int32_t dpm_ka;									///< keepalive interval for DTIM
    int32_t dpm_user_wu;							///< user defined wakeup time for DTIM
    int32_t dpm_tim_wu;							///< wakeup interval for DTIM

    int32_t hidden;								///< use Hidden SSID
    int32_t prov_type;							///< provision Type
#if defined (__SUPPORT_WPA_ENTERPRISE__)
    int32_t ent_type;							///< enterprise type 1:PEAP or TTLS or FAST (Recommend), 2: PEAP, 3: TTLS, 4: FAST, 5:TLS(Not support)
    int32_t ent_protocol;						///< enterprise protocol 1: MSCHAPv2 or GTC (Recommend), 2: MSCHAPv2, 3: GTC
    char ent_id[PROV_MAX_PW_LEN + 1];			///< enterprise router id
    char ent_pw[PROV_MAX_PW_LEN + 1];			///< enterprise router pw
#endif
} app_prov_config_t;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief find AP list on AP mode
 * @param[in] _opt  sorting RSSI or not
 * @param[in] _func call-back function for receiving found SSID,index..
 * @return  1 on success
 ****************************************************************************************
 */
int32_t app_find_home_ap(int32_t _opt, app_ap_list_from_scan_cb _func);

/**
 ****************************************************************************************
 * @brief reboot AP mode 
 * @param[in] _apMode  set mode to SYSMODE_AP_ONLY or SYSMODE_STA_N_AP (concurrent mode)
 * @param[in] _isSecurity   exist security mode or not ,0 : AP_OPEN_MODE  1 : AP_SECURITY_MODE 
 * @param[in] _removeNV   NVRAM remove or not, 0 : not run factory_reset , 1 : run factory_reset
 * @return void
 ****************************************************************************************
 */
void app_reboot_ap_mode(int32_t _apMode, int32_t _isSecurity, int32_t _removeNV);

/**
 ****************************************************************************************
 * @brief reboot STA mode 
 * @param[in] _factoryReset   factory reset mode is erased NV memory of app environment (1 is factory reset)
 * @param[in] _config  structure pointer having provisioning information such as SSID, PW, DPM mode, etc
 * @return void
 ****************************************************************************************
 */
void app_reset_to_station_mode(int32_t _factoryReset, app_prov_config_t *_config);

/**
 ****************************************************************************************
 * @brief Get DPM set value 
 * @param[in] _type - get  value type 
 * @return  success : set value, fail : -1
 ****************************************************************************************
 */
int app_get_DPM_set_value(GetValueType _type);

/**
 ****************************************************************************************
 * @brief Entry function for APP provisioning
 * @param[in] _mode - PROVISIONING_MODE_AP(1)  support only this
 * @return  void
 ****************************************************************************************
 */
void app_start_provisioning(int32_t _mode);

/**
 ****************************************************************************************
 * @brief SoftAP Provisioning Sample application thread calling function 
 * @To test softap provisioning change below definitions
 * @ sys_common_feature.h : #define __PROVISION_SAMPLE__
 * @param[in] arg - transfer information
 * @return  void
 ****************************************************************************************
 */
void softap_provisioning(void *arg);

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "cc3120_hw_eng_initialize.h"

#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"

#define	TCP_SERVER_WIN_SIZE			( 1024 * 4 )
#define	TCP_LISTEN_BAGLOG			5
#define	PROV_TLS_SVR_TIMEOUT		1000	//500	   EU FAE TLS Fail issue 500 ==> 1000

/**
 * @struct info
 * @brief TLS Server inforamtion
 * @note !!! Do not change this structure formation !!!
 */
typedef struct prov_tls_svr_config {
    mbedtls_net_context sock_ctx;
    unsigned int local_port;

    // for TLS
    mbedtls_ssl_context *ssl_ctx;
    mbedtls_ssl_config *ssl_conf;
    mbedtls_ctr_drbg_context *ctr_drbg_ctx;
    mbedtls_entropy_context *entropy_ctx;

    mbedtls_x509_crt *ca_cert_crt;
    mbedtls_x509_crt *cert_crt;
    mbedtls_pk_context *pkey_ctx;
    mbedtls_pk_context *pkey_alt_ctx;
} prov_tls_svr_conf_t;

///////////////////////////////////////////////////

/**
 * @brief external functions  
 */
extern int app_prov_tls_svr_init_config(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_init_socket(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_deinit_socket(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_init_ssl(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_setup_ssl(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_deinit_ssl(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_shutdown_ssl(prov_tls_svr_conf_t *config);
extern int app_prov_tls_svr_do_handshake(prov_tls_svr_conf_t *config);

extern void* app_prov_calloc(size_t n, size_t size);
extern void app_prov_free(void *f);

#endif	// __SUPPORT_WIFI_PROVISIONING__

#endif	// _APP_PROVISION_H_

/* EOF */
