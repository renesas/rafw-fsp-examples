/***********************************************************************************************************************
* File Name    : provisioning.h
* Description  : Device provisioning function declarations.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef __PROVISIONING_H__
#define __PROVISIONING_H__

#define PROV_SSID_LEN 32
#define PROV_PASS_LEN 64

typedef struct {
} prov_cmd_factory_reset_t;

typedef struct {
} prov_cmd_check_network_t;

typedef struct {
} prov_cmd_reboot_t;

typedef struct {
} prov_cmd_get_azure_connection_t;

typedef struct {
} prov_cmd_get_mode_t;

typedef struct {
} prov_cmd_get_name_t;

typedef struct {
} prov_cmd_scan_t;

typedef struct {
    uint32_t ping_ip;
    uint32_t srvr_ip;
    uint32_t srvr_port;
    char     cutomer_srvr_url[64];
    char     srvr_url[64];
} prov_cmd_network_info_t;

typedef struct {
    char     ssid[PROV_SSID_LEN + 1];
    char     password[PROV_PASS_LEN + 1];
    uint32_t security_type;
    uint32_t is_hidden;

    struct { // WPA Enterprise
        uint32_t eap_auth_mode;
        uint32_t eap_phase2;
    };
} prov_cmd_select_ap_t;

typedef struct {
} prov_cmd_disconnect_t;

typedef struct {
} prov_cmd_wifi_status_t;

typedef enum {
    SEC_TYPE_OPEN,
    SEC_TYPE_WEP,
    SEC_TYPE_WPA,
    SEC_TYPE_WPA2,
    SEC_TYPE_WPA2_ENTERPRISE,
    SEC_TYPE_WPA3,
} security_type_t;

typedef struct {
    char            ssid[PROV_SSID_LEN + 1];
    int32_t         rssi;
    uint32_t        channel;
    security_type_t security_type;
} prov_ap_entry_t;

typedef struct {
    uint32_t num_of_networks;
    prov_ap_entry_t networks[];
} prov_scanned_networks_t;

typedef struct {
  int (*reboot)(const prov_cmd_reboot_t* params);
  int (*factory)(const prov_cmd_factory_reset_t* params);
  int (*wifi_scan)(const prov_cmd_scan_t* params, prov_scanned_networks_t** networks);
  int (*wifi_ext_connect)(const prov_cmd_select_ap_t* params);
  int (*wifi_disconnect)(const prov_cmd_disconnect_t* params);
  int (*wifi_ping)(const prov_cmd_network_info_t* params);
} provisioning_cb_t;

void provisioning_init(const provisioning_cb_t* params);
void provisioning_cmd_dispatch(char* cmd);
void escape_json_string(const char *ssid, char *clean_ssid);

#endif /* __PROVISIONING_H__ */

