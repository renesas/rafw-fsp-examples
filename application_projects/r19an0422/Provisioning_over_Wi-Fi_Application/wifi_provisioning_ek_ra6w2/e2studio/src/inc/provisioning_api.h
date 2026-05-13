/***********************************************************************************************************************
 * File Name    : provisioning_api.h
 * Description  : Header file for provisioning API definitions
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **********************************************************************************************************************/
/* ${REA_DISCLAIMER_PLACEHOLDER} */

/*******************************************************************************************************************//**
 * @ingroup RENESAS_NETWORKING_INTERFACES
 * @defgroup PROVISIONING_API Provisioning Application (provisioning)
 * @brief Interface for Provisioning Application APIs.
 *
 * @{
 **********************************************************************************************************************/

#ifndef PROVISIONING_API_H

#define PROVISIONING_API_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

/* Includes provisioning configuration file */
#include "provisioning_cfg.h"

/* Includes board and MCU related header files. */
#include "bsp_api.h"
#include "rm_watchdog_service_api.h"

#include "rm_wifi_api.h"
#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER
 * macro at the end of this file. */
FSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/** @def maximum length of SSID */
#define PROV_MAX_SSID_LEN       128

/** @def maximum length of WPA style passkey */
#define PROV_MAX_PW_LEN         128

/** @def maximum length of WEP style passkey */
#define PROV_MAX_WEP_KEY_LEN    16

/** @def maximum  scan channel */
#define SCAN_CHANNEL_MAX        10

/** @def scan buffer. need change */
#define PROV_SCAN_BUF_SIZE      3584

/** @def maximum scanning params. need to change */
#define MAX_CLI_PARAMS          SCAN_CHANNEL_MAX + 1

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/** Provisioning SDK type */
typedef enum e_provisioning_type
{
    NOT_SPECIFIED      = 0,            ///< Not specified
    GENERIC_AP_SDK     = 1,            ///< Generic Soft AP SDK
    GENERIC_CONCUR_SDK = 4,            ///< Generic Concurrent SDK
    GENERIC_AWS        = 10,           ///< Generic Soft AP AWS
    ATCMD_AWS          = 11,           ///< AT Command AWS
    GENERIC_AZURE      = 20,           ///< Generic Soft AP Azure
    ATCMD_AZURE        = 21,           ///< AT Command Azure
} provisioning_type_t;

/** Provisioning Socket Type, TCP or TLS is supported */
typedef enum e_provisioning_socket_type
{
    PROV_TCP_SOCKET = 0,               ///< TCP Socket
    PROV_TLS_SOCKET = 1,               ///< TLS Socket
} provisioning_socket_type_t;

/** Provisioning DPM Mode type */
typedef enum e_provisioning_dpm_mode
{
    PROV_DPM_OFF = 0,                  ///< DPM Off
    PROV_DPM_ON  = 1,                  ///< DPM On
} provisioning_dpm_mode_t;

/** Provisioning Feature for AT Command Support */
typedef enum e_provisioning_feature
{
    PROV_FEATURE_NONE = 0,             ///< None
    PROV_FEATURE_ATCMD,                ///< AT Command
    PROV_FEATURE_DOORLOCK,             ///< Door Lock
    PROV_FEATURE_UNKNOWN,              ///< Unknown
} provisioning_feature_t;

/** Provisioning Scan Result for AP's Wi-Fi Security */
typedef enum e_provisioning_ap_security
{
    eAPSecurityOpen = 0,               ///< Open - No Security
    eAPSecurityWEP,                    ///< WEP
    eAPSecurityWPA,                    ///< WPA
    eAPSecurityWPA2,                   ///< WPA2 (RSN)
    eAPSecurityWPA_AUTO,               ///< WPA & WPA2 (RSN)
    eAPSecurityOWE,                    ///< WPA3 OWE
    eAPSecuritySAE,                    ///< WPA3 SAE
    eAPSecurityRSN_SAE,                ///< WPA2 (RSN) & WPA3 SAE
    eAPSecurityWPA_EAP,                ///< WPA Enterprise
    eAPSecurityWPA2_EAP,               ///< WPA2 Enterprise
    eAPSecurityWPA_AUTO_EAP,           ///< WPA & WPA2 Enterprise
    eAPSecurityWPA3_EAP,               ///< WPA3 Enterprise
    eAPSecurityWPA2_AUTO_EAP,          ///< WPA2 & WPA3 Enterprise
    eAPSecurityWPA3_EAP_192B,          ///< WPA3 192B Enterprise
    eAPSecurityNotSupported            ///< Unknown Security
} provisioning_ap_security_t;

/** EAP Type for WPA Enterprise Phase 1 */
typedef enum e_provisioning_eap_type
{
    TYPE_EAP_DEFAULT,                  /// WPA-Enterprise Phase1: Default ( PEAP / TTLS / FAST )
    TYPE_EAP_PEAP,                     /// WPA-Enterprise Phase1: PEAP
    TYPE_EAP_TTLS,                     /// WPA-Enterprise Phase1: EAP-TTLS
    TYPE_EAP_FAST,                     /// WPA-Enterprise Phase1: EAP-FAST
    TYPE_EAP_TLS,                      /// WPA-Enterprise Phase1: EAP-TLS (Not support)
} provisioning_eap_type_t;

/** EAP Protocol for WPA Enterprise Phase 2 */
typedef enum e_provisioning_eap_protocol
{
    PROTO_EAP_PHASE2_MIX,              /// WPA-Enterprise Phase2: MSCHAPV2 GTC
    PROTO_EAP_MSCHAPV2,                /// WPA-Enterprise Phase2: EAP-MSCHAPv2
    PROTO_EAP_GTC,                     /// WPA-Enterprise Phase2: EAP-GTC
} provisioning_eap_protocol_t;

/** Defines a type containing provison parameters to be passed between device andapps to setup profile. */
typedef struct st_provisioning_param
{
    int32_t auto_restart_flag;                    ///< Auto reboot flag - 0: No reboot, 1: Auto reboot

    char ssid[PROV_MAX_SSID_LEN + 1];             ///< SSID
    char psk[PROV_MAX_PW_LEN + 1];                ///< WPA style passkey

    provisioning_ap_security_t auth_type;      ///< 0: OPEN, 1:WEP, 2:WPA-PSK, 3:WPA2-PSK, 4:WPA-AUTO

    int32_t wep_key_index;                        ///< WEP key index
    char wep_key[4][PROV_MAX_WEP_KEY_LEN + 1]; ///< WEP key for each index

    /**
     * Country Code
     *
     * CA  US  USE USL USX FR  LT  LU  LV  NL
     * NO  NZ  PL  PT  SE  SI  SK  AT  HU  IE
     * IS  IT  HK  EE  ES  FI  GB  GR  DE  DK
     * CZ  CY  CH  AU  BR  BE  CN  ID  KR  MY
     * TH  TW  ZA  IL  SG  JP  ILO PH  IN  EU
     */
    char country[4];                              ///< country code string

    int32_t ip_addr_mode;                         ///< 0:DHCP Client, 1:STATIC

    int32_t sntp_flag;                            ///< 0:disable, 1:enable
    char    sntp_server[32];                      ///< name of SNTP server
    int32_t sntp_period;                          ///< update period of SNTP

    provisioning_dpm_mode_t dpm_mode;          ///< 0:No DPM mode, 1:DPM mode
    int32_t dpm_ka;                               ///< keepalive interval for DTIM
    int32_t dpm_user_wu;                          ///< user defined wakeup time for DTIM
    int32_t dpm_tim_wu;                           ///< wakeup interval for DTIM

    bool hidden;                                  ///< use Hidden SSID
    provisioning_type_t         prov_type;     ///< provision Type
    provisioning_eap_type_t     eap_type;      ///< EAP Type
    provisioning_eap_protocol_t eap_protocol;  ///< EAP Protocol
    char eap_identity[PROV_MAX_SSID_LEN + 1];     ///< EAP Identity
    char eap_password[PROV_MAX_PW_LEN + 1];       ///< EAP Password
} provisioning_param_t;

/** Provisioning TLS Server configuration */
typedef struct st_provisioning_tls_server_cfg
{
    mbedtls_net_context sock_ctx;            ///< Socket context
    unsigned int        local_port;          ///< Local port

    // for TLS
    mbedtls_ssl_context      *ssl_ctx;      ///< SSL context
    mbedtls_ssl_config       *ssl_conf;     ///< SSL configuration
    mbedtls_ctr_drbg_context *ctr_drbg_ctx; ///< CTR DRBG context
    mbedtls_entropy_context  *entropy_ctx;  ///< Entropy context

    mbedtls_x509_crt   *ca_cert_crt;        ///< CA certificate
    mbedtls_x509_crt   *cert_crt;           ///< Certificate
    mbedtls_pk_context *pkey_ctx;           ///< Private key context
    mbedtls_pk_context *pkey_alt_ctx;       ///< Alternative private key context
} provisioning_tls_server_cfg_t;

/** Provisioning scan callback arguments */
typedef struct st_provisioning_scan_callback_args
{
    int index;                                  ///< Index of current argument,
    char p_ssid[PROV_MAX_SSID_LEN + 1];          ///< Pointer to the SSID
    provisioning_ap_security_t security_mode; ///< Security mode for AP, 1 if
    int signal_strength;                        ///< Signal strength of AP in db (RSSI)
    int frequency;                              ///< Frequency of AP
    bool is_listed;                              ///< Scanned AP is listed on AP List sent through phone or not, default is true

    void const *p_context;                      ///< User defined context passed into callback function.
} provisioning_scan_callback_args_t;

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER
 * macro at the top of this file. */
FSP_FOOTER

#endif

/*******************************************************************************************************************/ /**
 * @} (end defgroup PROVISIONING_APP)
 **********************************************************************************************************************/
