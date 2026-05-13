/***********************************************************************************************************************
 * File Name    : provisioning.c
 * Description  : Handles Wi-Fi provisioning over BLE including scan, connect, network info, and command dispatch.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include "r_ble_wifi_provisionings.h"
#include "provisioning.h"
#include "utility.h"
#include "logging.h"
#include "jsmn.h"
#include "common_utils.h"

#define SCAN_REAL_NETWORK 1

typedef enum {
    WIFI_CMD_SCAN_AP_SUCCESS               = 1,
    WIFI_CMD_SCAN_AP_FAIL                  = 2,

    WIFI_CMD_FW_BLE_DOWNLOAD_SUCCESS       = 3,
    WIFI_CMD_FW_BLE_DOWNLOAD_FAIL          = 4,

    WIFI_CMD_INQ_WIFI_STATUS_CONNECTED     = 5,
    WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED = 6,

    WIFI_PROV_DATA_VALIDITY_CHK_ERR        = 7,
    WIFI_PROV_DATA_SAVE_SUCCESS            = 8,

    WIFI_CMD_ACK                           = 100,
    WIFI_CMD_SELECT_AP_SUCCESS             = 101,
    WIFI_CMD_SELECT_AP_FAIL                = 102,
    WIFI_PROV_WRONG_PW                     = 103,
    WIFI_PROV_NETWORK_INFO                 = 104,
    WIFI_PROV_AP_FAIL                      = 105,
    WIFI_PROV_DNS_FAIL_GOOGLE_FAIL         = 106,
    WIFI_PROV_DNS_FAIL_GOOGLE_OK           = 107,
    WIFI_PROV_NO_URL_PING_FAIL             = 108,
    WIFI_PROV_NO_URL_PING_OK               = 109,
    WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK = 110,
    WIFI_PROV_DNS_OK_PING_OK               = 111,
    WIFI_PROV_REBOOT_ACK                   = 112,
    WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL    = 113,

    WIFI_CMD_UNKNOWN_RCV                   = 114,
} WIFI_STATUS;

typedef uint32_t WIFI_STATUS_t;

typedef enum {
    CMD_FACTORY_RESET,
    CMD_CHECK_NETWORK,
    CMD_REBOOT,
    CMD_GET_AZURE_CONNECTION,
    CMD_GET_MODE,
    CMD_GET_NAME,
    CMD_SCAN,
    CMD_NETWORK_INFO,
    CMD_SELECT_AP,
    CMD_DISCONNECT,
    CMD_WIFI_STATUS,
} cmd_t;

typedef void (*cmd_cb_t) (cmd_t, const char*, jsmntok_t*, uint32_t);

typedef struct {
    const char* cmd;
    cmd_cb_t cb;
    cmd_t param;
} provisioning_cmd_t;

static provisioning_cb_t provisioning_cb;
static prov_cmd_select_ap_t provisioning_data;
static prov_cmd_network_info_t provisioninig_network_info;
char wifi_networks[2048];

static int32_t jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (uint32_t)strlen(s) == (uint32_t)(tok->end - tok->start) &&
        strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0)
    {
      return 0;
    }

    return -1;
}

static char* json_network_add(char* mem, uint32_t mem_len,
                              const char* ssid, security_type_t type, int32_t strength, int32_t isLast)
{
    int32_t len = 0;
    char *p = mem + strlen(mem);
    char clean_ssid[PROV_SSID_LEN + 1] = {0,};

    escape_json_string(ssid, clean_ssid);

    if (isLast == 1)
    {
        len = snprintf(p, mem_len - strlen(mem),
                       "{"
                       "\"SSID\":\"%s\","
                       "\"security_type\":%d,"
                       "\"signal_strength\":%d"
                       "}", clean_ssid, type, strength);
    }
    else
    {
        len = snprintf(p, mem_len - strlen(mem),
                       "{"
                       "\"SSID\":\"%s\","
                       "\"security_type\":%d,"
                       "\"signal_strength\":%d"
                       "},", clean_ssid, type, strength);
    }

    LOG_INFO("network len: %" PRId32 "\n", len);

    return p + len;
}

static char* json_generic_response(WIFI_STATUS_t rc)
{
    static char buf[32];

    snprintf(buf, sizeof(buf),
             "{"
             "\"result\":%u"
             "}", rc);

    return buf;
}

static char* json_check_network_response(WIFI_STATUS_t rc,
                                         const char* ssid, const char* passwd, security_type_t type)
{
    static char buf[256];

    snprintf(buf, sizeof(buf),
             "{"
             "\"result\":%u,"
             "\"ssid\":\"%s\","
             "\"password\":\"%s\","
             "\"security\":%d"
             "}", rc, ssid, passwd, type);

    return buf;
}

static void params_cmd_factory_reset_print(const prov_cmd_factory_reset_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_factory_reset_t: {}\n");
}

static void params_cmd_check_network_print(const prov_cmd_check_network_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_check_network_t: {}\n");
}

static void params_cmd_reboot_print(const prov_cmd_reboot_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_reboot_t: {}\n");
}

static void params_cmd_scan_print(const prov_cmd_scan_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_scan_t: {}\n");
}

static void params_cmd_network_info_print(const prov_cmd_network_info_t* params)
{
    LOG_INFO("prov_cmd_network_info_t: {\n"
             "  - ping_ip: 0x%" PRIx32 "\n"
             "  - srvr_ip: 0x%" PRIx32 "\n"
             "  - srvr_port: %" PRIu32 "\n"
             "  - srvr_url: %s\n"
             "  - customer_srvr_url: %s\n"
             "}\n", params->ping_ip, params->srvr_ip, params->srvr_port,
             params->srvr_url, params->cutomer_srvr_url
            );
}

static void params_cmd_select_ap_print(const prov_cmd_select_ap_t* params)
{
    APP_PRINT_INFO("prov_cmd_select_ap_t: {\n"
                   "  - ssid: %s\n"
                   "  - pass: %s\n"
                   "  - secT: %d\n"
                   "  - hide: %d\n"
                   "}\n", params->ssid, params->password, params->security_type, params->is_hidden
                  );
}

static void params_cmd_disconnect_print(const prov_cmd_disconnect_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_disconnect_t: {}\n");
}

static void params_cmd_wifi_status_print(const prov_cmd_wifi_status_t* params)
{
    UNUSED(params);
    APP_PRINT_INFO("prov_cmd_wifi_status_t: {}\n");
}

static void network_info(prov_cmd_network_info_t* params)
{
    params_cmd_network_info_print(params);
    memcpy(&provisioninig_network_info, params, sizeof(provisioninig_network_info));
    WIFI_STATUS_t status = WIFI_CMD_ACK;
    R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning((st_ble_wifi_provisionings_wifi_provisioning_t *)json_generic_response(WIFI_PROV_NETWORK_INFO));
    R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(0,(uint16_t *)&status);
}

static void select_ap(prov_cmd_select_ap_t* params)
{
    params_cmd_select_ap_print(params);
    WIFI_STATUS_t notify_status = WIFI_CMD_ACK;
    WIFI_STATUS_t status = WIFI_CMD_SELECT_AP_SUCCESS;
    provisioning_cb.wifi_ext_connect(params);
    R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning((st_ble_wifi_provisionings_wifi_provisioning_t *)json_generic_response(status));
    R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(0, (uint16_t *)&notify_status);
    memcpy(&provisioning_data, params, sizeof(provisioning_data));
}

static void scan()
{
    WIFI_STATUS_t status = WIFI_CMD_SCAN_AP_FAIL;
    prov_scanned_networks_t* networks = 0;

    memset(wifi_networks, 0, sizeof(wifi_networks));
    wifi_networks[0] = '[';

#if SCAN_REAL_NETWORK
    int rc = provisioning_cb.wifi_scan(0, &networks);

    if (rc == 0)
    { // Success
        for (uint32_t i = 0; i < networks->num_of_networks; i++)
        {
            prov_ap_entry_t* entry = &networks->networks[i];

            APP_PRINT_INFO("SSID: %25s, RSSI: %4d, Channel: %4u, Security: %d\n",
                           entry->ssid, entry->rssi, entry->channel, entry->security_type);
            json_network_add(wifi_networks, sizeof(wifi_networks) - 1,
                             entry->ssid, entry->security_type,  entry->rssi,
                             i == networks->num_of_networks - 1 ? 1 : 0);
        }

        free(networks);
        status = WIFI_CMD_SCAN_AP_SUCCESS;
    }

#else
    /* Add fake networks */
    json_network_add(wifi_networks, sizeof(wifi_networks) - 1, "myNetwork",  SEC_TYPE_OPEN, -33, 0);
    json_network_add(wifi_networks, sizeof(wifi_networks) - 1, "myNetwork1", SEC_TYPE_WPA , -76, 0);
    json_network_add(wifi_networks, sizeof(wifi_networks) - 1, "myNetwork2", SEC_TYPE_WPA2, -76, 0);
    json_network_add(wifi_networks, sizeof(wifi_networks) - 1, "myNetwork3", SEC_TYPE_OPEN, -76, 0);
    json_network_add(wifi_networks, sizeof(wifi_networks) - 1, "myNetwork4", SEC_TYPE_OPEN, -76, 1);

    status = WIFI_CMD_SCAN_AP_SUCCESS;
#endif

    wifi_networks[strlen(wifi_networks)] = ']';
    R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(0, (uint16_t *)&status);
}

static void reboot()
{
    WIFI_STATUS_t status = WIFI_CMD_ACK;
    R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning((st_ble_wifi_provisionings_wifi_provisioning_t *)json_generic_response(WIFI_PROV_REBOOT_ACK));
    R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(0, (uint16_t *)&status);
    provisioning_cb.reboot(0);
}

static void check_network()
{
    WIFI_STATUS_t notify_status = WIFI_CMD_ACK;
    WIFI_STATUS_t status = WIFI_PROV_DNS_OK_PING_OK;
    int rc = provisioning_cb.wifi_ping(&provisioninig_network_info);
    if (rc != 0)
    {
        /* RC needs to be converted to proper status code */
        status = WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL;
    }

    R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning((st_ble_wifi_provisionings_wifi_provisioning_t *)json_check_network_response(status,
                                               provisioning_data.ssid, provisioning_data.password, provisioning_data.security_type));
    R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(0, (uint16_t *)&notify_status);
}

static void network_info_event_handler(cmd_t param,
                                       const char* cmd, jsmntok_t* token, uint32_t num_of_elements)
{
    UNUSED(param);
    char ip_str[32] = {};
    char* endptr = 0;
    prov_cmd_network_info_t params = {};

    for (uint32_t i = 0; i < num_of_elements; i++)
    {
        if (jsoneq(cmd, &token[i], "ping_addr") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len =(uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(ip_str, cmd_str_start, MIN(sizeof(ip_str), len));
            if (string_to_ip(ip_str, &params.ping_ip) != 0)
            {
                /* bad case */
            }
        }
        else if (jsoneq(cmd, &token[i], "svr_addr") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len =(uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(ip_str, cmd_str_start, MIN(sizeof(ip_str), len));
            if (string_to_ip(ip_str, &params.srvr_ip) != 0)
            {
            	/* bad case */
            }
        }
        else if (jsoneq(cmd, &token[i], "svr_port") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start)
            {
            	/* bad case */
            }
            else
            {
                params.srvr_port = (uint32_t)value;
            }
        }
        else if (jsoneq(cmd, &token[i], "customer_svr_url") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = (uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(params.cutomer_srvr_url, cmd_str_start, MIN(sizeof(params.cutomer_srvr_url), len));
        }
        else if (jsoneq(cmd, &token[i], "svr_url") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = (uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(params.srvr_url, cmd_str_start, MIN(sizeof(params.srvr_url), len));
        }
        else
        {
        }
    }

    network_info(&params);
}

static void select_ap_event_handler(cmd_t param, const char* cmd, jsmntok_t* token, uint32_t num_of_elements)
{
    UNUSED(param);
    char* endptr = 0;
    prov_cmd_select_ap_t params = {};

    for (uint32_t i = 0; i < num_of_elements; i++)
    {
        if (jsoneq(cmd, &token[i], "SSID") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len =(uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(params.ssid, cmd_str_start, MIN(sizeof(params.ssid) - 1, len));
        }
        else if (jsoneq(cmd, &token[i], "security_type") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start)
            {
            	/* bad case */
            }
            else
            {
               params.security_type = (uint32_t)value;
            }

        }
        else if (jsoneq(cmd, &token[i], "password") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = (uint32_t)(token[i+1].end - token[i+1].start);

            strncpy(params.password, cmd_str_start, MIN(sizeof(params.password) - 1, len));
        }
        else if (jsoneq(cmd, &token[i], "isHidden") == 0)
        {
            const char* cmd_str_start = cmd + token[i+1].start;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start)
            {
            	/* bad case */
            }
            else
            {
                params.is_hidden = (uint32_t)value;
            }
        }
        else
        {
        }
    }

    select_ap(&params);
}

static void generic_event_handler(cmd_t param, const char* cmd, jsmntok_t* token, uint32_t num_of_elements)
{
    UNUSED(cmd);
    UNUSED(token);
    UNUSED(num_of_elements);
    switch (param)
    {
        case CMD_FACTORY_RESET:
        {
            params_cmd_factory_reset_print(0);
            provisioning_cb.factory(0);
        } break;

        case CMD_REBOOT:
        {
            params_cmd_reboot_print(0);
            reboot();
        } break;

        case CMD_SCAN:
        {
            params_cmd_scan_print(0);
            scan();
        } break;

        case CMD_DISCONNECT:
        {
            params_cmd_disconnect_print(0);
            provisioning_cb.wifi_disconnect(0);
        } break;

        case CMD_WIFI_STATUS:
        {
            params_cmd_wifi_status_print(0);
        } break;

        case CMD_CHECK_NETWORK:
        {
           params_cmd_check_network_print(0);
           check_network();
        } break;

        default: break;
    }
}

static provisioning_cmd_t prov_list_of_cmds[] =
{
    { "factory_reset"     , &generic_event_handler     , CMD_FACTORY_RESET        },
    { "chk_network"       , &generic_event_handler     , CMD_CHECK_NETWORK        },
    { "reboot"            , &generic_event_handler     , CMD_REBOOT               },
    { "get_azureConString", &generic_event_handler     , CMD_GET_AZURE_CONNECTION },
    { "get_mode"          , &generic_event_handler     , CMD_GET_MODE             },
    { "get_thingName"     , &generic_event_handler     , CMD_GET_NAME             },
    { "scan"              , &generic_event_handler     , CMD_SCAN                 },
    { "network_info"      , &network_info_event_handler, CMD_NETWORK_INFO         },
    { "select_ap"         , &select_ap_event_handler   , CMD_SELECT_AP            },
    { "disconnect"        , &generic_event_handler     , CMD_DISCONNECT           },
};

void escape_json_string(const char *ssid, char *clean_ssid)
{
    while (*ssid)
    {
        if (*ssid == '"')
        {
            *clean_ssid++ = '\\';
            *clean_ssid++ = '"';
        }
        else if (*ssid == '\\')
        {
            *clean_ssid++ = '\\';
            *clean_ssid++ = '\\';
        }
        else
        {
            *clean_ssid++ = *ssid;
        }
        ssid++;
    }
    *clean_ssid = '\0';
}

void provisioning_cmd_dispatch(char* cmd)
{
    jsmn_parser parser;
    jsmntok_t token[64];
    jsmn_init(&parser);
    int32_t num_of_elements = jsmn_parse(&parser, cmd, strlen(cmd),
                              token, sizeof(token) / sizeof(token[0]));

    if (cmd[0] == 0x01)
    {
        cmd += 8; // skip a header
    }

    if (num_of_elements < 0)
    {
        /* No command is found or JSON is corrupted */
        return;
    }

    for (int32_t i = 0; i < num_of_elements; i++)
    {
        if (jsoneq(cmd, &token[i], "dialog_cmd") == 0)
        {
            char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len =(uint32_t)( token[i+1].end - token[i+1].start);

            for (uint32_t j = 0; j < sizeof(prov_list_of_cmds) / sizeof(prov_list_of_cmds[0]); j++)
            {
                if (!strncmp(prov_list_of_cmds[j].cmd, cmd_str_start, len))
                {
                    APP_PRINT_INFO("CMD: %s\n", cmd_str_start);
                    prov_list_of_cmds[j].cb(prov_list_of_cmds[j].param, cmd, token, (uint32_t)num_of_elements);
                    break;
                }
            }
        }
    }
}

void provisioning_init(const provisioning_cb_t* params)
{
    memcpy(&provisioning_cb, params, sizeof(provisioning_cb));
}
