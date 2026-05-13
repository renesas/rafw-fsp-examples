#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "r_ble_WiFiProvisionings.h"

#include "provisioning.h"
#include "utility.h"
#include "logging.h"
#include "jsmn.h"
#include "app_thing_manager.h"
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#include "qe_ble_profile.h"

#define SCAN_REAL_NETWORK 1
#define APP_THINGNAME 							   APP_NVRAM_CONFIG_THINGNAME
#define APP_IOTHUB_CONN_STRING					   APP_NVRAM_CONFIG_IOTHUB_CONN_STRING
#define AZURE_PROVISIONING_MODE_GENERAL            20
#define AZURE_PROVISIONING_MODE_AT                 21
#define AZURE_THINGNAME_JSON_LEN                   192
#define AZURE_PROVISIONING_MODE_JSON_LEN           21
#define AZURE_CONN_STR_JSON_LEN					   300

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
	WIFI_CMD_AZURE			   			   = 115,
    WIFI_CMD_UNKNOWN_RCV                   = 116,
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

typedef struct
{
   char *thingName;
   bool  is_allocated;   // true if getMemoryString() allocated it
} thing_name_info_t;

typedef struct
{
   char *conn_str;
   bool  is_allocated;   // true if getMemoryString() allocated it
} azure_conn_str_info_t;

typedef void (*cmd_cb_t) (cmd_t, const char*, jsmntok_t*, uint32_t);

typedef struct {
    const char* cmd;
    cmd_cb_t    cb;
    cmd_t       param;
} provisioning_cmd_t;

static int32_t jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (uint32_t)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
      return 0;
    }

    return -1;
}

static provisioning_cb_t provisioning_cb;
static prov_cmd_select_ap_t provisioning_data;
static prov_cmd_network_info_t provisioninig_network_info;
char wifi_networks[2048];

static char* json_network_add(char* mem, uint32_t mem_len,
    const char* ssid, security_type_t type, int32_t strength, int32_t isLast) {

    int32_t len = 0;
    char* p = mem + strlen(mem);
    char clean_ssid[PROV_SSID_LEN + 1] = {0,};

    escape_json_string(ssid, clean_ssid);

    if (isLast == 1) {
        len = snprintf(p, mem_len - strlen(mem),
            "{"
            "\"SSID\":\"%s\","
            "\"security_type\":%u,"
            "\"signal_strength\":%d"
            "}", clean_ssid, type, strength);

    } else {

        len = snprintf(p, mem_len - strlen(mem),
            "{"
            "\"SSID\":\"%s\","
            "\"security_type\":%u,"
            "\"signal_strength\":%d"
            "},", clean_ssid, type, strength);
    }

    LOG_DEBUG("network len: %d\n", len);
    return p + len;
}

static char* json_generic_response(WIFI_STATUS_t rc) {
    static char buf[32];
    snprintf(buf, sizeof(buf),
      "{"
      "\"result\":%d"
      "}", rc);

    return buf;
}

static char* json_check_network_response(WIFI_STATUS_t rc,
    const char* ssid, const char* passwd, security_type_t type) {

    static char buf[256];
    snprintf(buf, sizeof(buf),
      "{"
      "\"result\":%d,"
      "\"ssid\":\"%s\","
      "\"password\":\"%s\","
      "\"security\":%u"
      "}", rc, ssid, passwd, type);

    return buf;
}

static void params_cmd_factory_reset_print(const prov_cmd_factory_reset_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_factory_reset_t: {}\n");
}

static void params_cmd_check_network_print(const prov_cmd_check_network_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_check_network_t: {}\n");
}

static void params_cmd_reboot_print(const prov_cmd_reboot_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_reboot_t: {}\n");
}

static char *getMemoryString(const char *string_data)
{
    size_t strLen;
    char *strTemp;

    strLen = strlen(string_data);
    strTemp = (char *)pvPortMalloc(strLen + 1);
    if (strTemp != NULL)
    {
        memset(strTemp, 0x00, (strLen + 1));
        strcpy(strTemp, string_data);
    }

    return strTemp;
}

static azure_conn_str_info_t get_azure_conn_str(void)
{
	azure_conn_str_info_t info = { .conn_str = NULL, .is_allocated = false };

#ifdef RM_MAP_PERSISTANT_W
   RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                   ENV_GROUP_APPCFG,
								   APP_IOTHUB_CONN_STRING,
                                   &info.conn_str);
#else
   info.conn_str = read_nvram_appcfg_string(APP_CONFIG_CONN_STRG);
#endif
   if (info.conn_str == NULL)
   {
       info.conn_str   = getMemoryString((const char*)APP_USER_MY_IOTHUB_CONN_STRING);
       info.is_allocated = true;   // mark as heap-allocated
   }

   return info;
}

static thing_name_info_t get_thing_name(void)
{
   thing_name_info_t info = { .thingName = NULL, .is_allocated = false };

#ifdef RM_MAP_PERSISTANT_W
   RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                   ENV_GROUP_APPCFG,
				   APP_THINGNAME,
                                   &info.thingName);
#else
   info.thingName = read_nvram_appcfg_string(APP_CONFIG_THINGNAME);
#endif
   if (info.thingName == NULL)
   {
       info.thingName   = getMemoryString((const char*)APP_USER_MY_THING_NAME);
       info.is_allocated = true;   // mark as heap-allocated
   }

   return info;
}

static void params_cmd_get_azure_connection_print(const prov_cmd_get_azure_connection_t* params) {
   WIFI_STATUS_t notify_status = WIFI_CMD_AZURE;
   st_ble_gatt_value_t gatt_value;
   char json_value[AZURE_CONN_STR_JSON_LEN];
   ble_status_t ble_status = (ble_status_t)notify_status;
   UNUSED(params);
   azure_conn_str_info_t name_info = get_azure_conn_str();
   if (name_info.conn_str != NULL)
   {
	   snprintf(json_value, sizeof(json_value),
				"{\"azureConString\":\"%s\"}", name_info.conn_str);
   }
   else
   {
	   snprintf(json_value, sizeof(json_value), "{\"azureConString\":\"\"}");
   }

   LOG_INFO("prov_cmd_get_azure_connection_t: {}\n");
   gatt_value.p_value   = (uint8_t *)json_value;
   gatt_value.value_len = (uint16_t) strlen((char *)json_value);
   R_BLE_GATTS_SetAttr(BLE_GAP_INVALID_CONN_HDL,
		   QE_ATTRIBUTE_HANDLE_CHARACTERISTIC_VALUE_WIFIPROVISIONING_AZURE,
					   &gatt_value);
   R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &ble_status);

   /* Free only if we allocated it ourselves */
   if (name_info.is_allocated && name_info.conn_str != NULL)
   {
	   vPortFree(name_info.conn_str);
   }
}

static void params_cmd_get_name_print(const prov_cmd_get_name_t *params)
{
   WIFI_STATUS_t notify_status = WIFI_CMD_AZURE;
   st_ble_gatt_value_t gatt_value;
   char json_value[AZURE_THINGNAME_JSON_LEN];
   ble_status_t ble_status = (ble_status_t)notify_status;
   UNUSED(params);
   thing_name_info_t name_info = get_thing_name();
   if (name_info.thingName != NULL)
   {
       snprintf(json_value, sizeof(json_value),
                "{\"thingName\":\"%s\"}", name_info.thingName);
   }
   else
   {
       snprintf(json_value, sizeof(json_value), "{\"thingName\":\"\"}");
   }

   LOG_INFO("prov_cmd_get_name_t: {}\n");
   gatt_value.p_value   = (uint8_t *)json_value;
   gatt_value.value_len = (uint16_t) strlen((char *)json_value);

   R_BLE_GATTS_SetAttr(BLE_GAP_INVALID_CONN_HDL,
		   QE_ATTRIBUTE_HANDLE_CHARACTERISTIC_VALUE_WIFIPROVISIONING_AZURE,
                       &gatt_value);
   R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &ble_status);
   /* Free only if we allocated it ourselves */
   if (name_info.is_allocated && name_info.thingName != NULL)
   {
       vPortFree(name_info.thingName);
   }
}

static void params_cmd_get_mode_print(const prov_cmd_get_mode_t *params)
{
    WIFI_STATUS_t notify_status = WIFI_CMD_AZURE;
    st_ble_gatt_value_t gatt_value;
    char json_value[AZURE_PROVISIONING_MODE_JSON_LEN];
    ble_status_t ble_status = (ble_status_t)notify_status;

    UNUSED(params);
    LOG_INFO("prov_cmd_get_mode_t: {}\n");
    snprintf(json_value, sizeof(json_value), "{\"mode\":%d}", AZURE_PROVISIONING_MODE_GENERAL);
    gatt_value.p_value   = (uint8_t *) json_value;
    gatt_value.value_len = (uint16_t) strlen((char *)json_value);

    R_BLE_GATTS_SetAttr(BLE_GAP_INVALID_CONN_HDL,
    		QE_ATTRIBUTE_HANDLE_CHARACTERISTIC_VALUE_WIFIPROVISIONING_AZURE,
                        &gatt_value);
    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &ble_status);
}

static void params_cmd_scan_print(const prov_cmd_scan_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_scan_t: {}\n");
}

static void params_cmd_network_info_print(const prov_cmd_network_info_t* params) {
    LOG_INFO("prov_cmd_network_info_t: {\n"
             "  - ping_ip: 0x%x\n"
             "  - srvr_ip: 0x%x\n"
             "  - srvr_port: %d\n"
             "  - srvr_url: %s\n"
             "  - customer_srvr_url: %s\n",
             "}\n", params->ping_ip, params->srvr_ip,
             params->srvr_port, params->srvr_url, params->cutomer_srvr_url
    );
}

static void params_cmd_select_ap_print(const prov_cmd_select_ap_t* params) {
    LOG_INFO("prov_cmd_select_ap_t: {\n"
             "  - ssid: %s\n"
             "  - pass: %s\n"
             "  - secT: %d\n"
             "  - hide: %d\n"
             "}\n", params->ssid, params->password, params->security_type, params->is_hidden
    );
}

static void params_cmd_disconnect_print(const prov_cmd_disconnect_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_disconnect_t: {}\n");
}

static void params_cmd_wifi_status_print(const prov_cmd_wifi_status_t* params) {
    UNUSED(params);
    LOG_INFO("prov_cmd_wifi_status_t: {}\n");
}

static void network_info(prov_cmd_network_info_t* params) {
    params_cmd_network_info_print(params);
    memcpy(&provisioninig_network_info, params, sizeof(provisioninig_network_info));

    WIFI_STATUS_t status = WIFI_CMD_ACK;

    R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(json_generic_response(WIFI_PROV_NETWORK_INFO));
    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &status);
}

static void select_ap(prov_cmd_select_ap_t* params) {
    params_cmd_select_ap_print(params);

    WIFI_STATUS_t notify_status = WIFI_CMD_ACK;
    WIFI_STATUS_t status = WIFI_CMD_SELECT_AP_SUCCESS;

    provisioning_cb.wifi_ext_connect(params);

    R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(json_generic_response(status));
    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &notify_status);

    memcpy(&provisioning_data, params, sizeof(provisioning_data));
}

static void scan() {
    WIFI_STATUS_t status = WIFI_CMD_SCAN_AP_FAIL;

    memset(wifi_networks, 0, sizeof(wifi_networks));
    wifi_networks[0] = '[';

#if SCAN_REAL_NETWORK
    prov_scanned_networks_t* networks = 0;
    int rc = provisioning_cb.wifi_scan(0, &networks);
    if (rc == 0) { // Success
        for (uint32_t i = 0; i < networks->num_of_networks; i++) {
            prov_ap_entry_t* entry = &networks->networks[i];

            LOG_INFO("SSID: %25s, RSSI: %4d, Channel: %4u, Security: %d\n",
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
    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &status);
}

static void reboot() {
    WIFI_STATUS_t status = WIFI_CMD_ACK;

    R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(json_generic_response(WIFI_PROV_REBOOT_ACK));
    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &status);

    provisioning_cb.reboot(0);
}

static void check_network() {
    WIFI_STATUS_t notify_status = WIFI_CMD_ACK;
    WIFI_STATUS_t status = WIFI_PROV_DNS_OK_PING_OK;

    int rc = provisioning_cb.wifi_ping(&provisioninig_network_info);
    if (rc != 0) {
        // RC needs to be converted to proper status code
        status = WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL;
    }

    R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(
        json_check_network_response(status, provisioning_data.ssid,
          provisioning_data.password, provisioning_data.security_type));

    R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(0, &notify_status);
}

static void network_info_event_handler(cmd_t param,
    const char* cmd, jsmntok_t* token, uint32_t num_of_elements) {

    UNUSED(param);
    prov_cmd_network_info_t params = {};

    for (uint32_t i = 0; i < num_of_elements; i++) {
        if (jsoneq(cmd, &token[i], "ping_addr") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;
            char ip_str[32] = {};

            strncpy(ip_str, cmd_str_start, MIN(sizeof(ip_str), len));

            if (string_to_ip(ip_str, &params.ping_ip) != 0) {
                // bad case
            }
        } else if (jsoneq(cmd, &token[i], "svr_addr") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;
            char ip_str[32] = {};

            strncpy(ip_str, cmd_str_start, MIN(sizeof(ip_str), len));

            if (string_to_ip(ip_str, &params.srvr_ip) != 0) {
                // bad case
            }

        } else if (jsoneq(cmd, &token[i], "svr_port") == 0) {
            /* uint32_t */
            const char* cmd_str_start = cmd + token[i+1].start;

            char* endptr = 0;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start) {
                // bad case
            } else {
                params.srvr_port = value;
            }

        } else if (jsoneq(cmd, &token[i], "customer_svr_url") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;

            strncpy(params.cutomer_srvr_url, cmd_str_start, MIN(sizeof(params.cutomer_srvr_url), len));

        } else if (jsoneq(cmd, &token[i], "svr_url") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;

            strncpy(params.srvr_url, cmd_str_start, MIN(sizeof(params.srvr_url), len));
        }
    }

    network_info(&params);
}

static void select_ap_event_handler(cmd_t param,
    const char* cmd, jsmntok_t* token, uint32_t num_of_elements) {

    UNUSED(param);
    prov_cmd_select_ap_t params = {};

    for (uint32_t i = 0; i < num_of_elements; i++) {
        if (jsoneq(cmd, &token[i], "SSID") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;

            strncpy(params.ssid, cmd_str_start, MIN(sizeof(params.ssid) - 1, len));

        } else if (jsoneq(cmd, &token[i], "security_type") == 0) {
            /* uint32_t */
            const char* cmd_str_start = cmd + token[i+1].start;

            char* endptr = 0;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start) {
                // bad case
            } else {
                params.security_type = value;
            }

        } else if (jsoneq(cmd, &token[i], "password") == 0) {
            /* String */
            const char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;

            strncpy(params.password, cmd_str_start, MIN(sizeof(params.password) - 1, len));

        } else if (jsoneq(cmd, &token[i], "isHidden") == 0) {
            /* uint32_t */
            const char* cmd_str_start = cmd + token[i+1].start;

            char* endptr = 0;
            long value = strtol(cmd_str_start, &endptr, 10);

            if (endptr == cmd_str_start) {
                // bad case
            } else {
                params.is_hidden = value;
            }
        }
    }

    select_ap(&params);
}

static void generic_event_handler(cmd_t param,
    const char* cmd, jsmntok_t* token, uint32_t num_of_elements) {

    UNUSED(cmd);
    UNUSED(token);
    UNUSED(num_of_elements);
    switch (param) {
        case CMD_FACTORY_RESET: {
            params_cmd_factory_reset_print(0);
            provisioning_cb.factory(0);
        } break;

        case CMD_REBOOT: {
            params_cmd_reboot_print(0);
            reboot();
        } break;

        case CMD_SCAN: {
            params_cmd_scan_print(0);
            scan();
        } break;

        case CMD_DISCONNECT: {
            params_cmd_disconnect_print(0);
            provisioning_cb.wifi_disconnect(0);
        } break;

        case CMD_WIFI_STATUS: {
            params_cmd_wifi_status_print(0);
        } break;

        case CMD_CHECK_NETWORK: {
           params_cmd_check_network_print(0);
           check_network();
        } break;

        case CMD_GET_NAME: {
			params_cmd_get_name_print(0);
		} break;

		case CMD_GET_MODE: {
			params_cmd_get_mode_print(0);
		} break;

		case CMD_GET_AZURE_CONNECTION: {
			params_cmd_get_azure_connection_print(0);
		} break;

        default: break;
    }
}

static provisioning_cmd_t prov_list_of_cmds[] = {
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

void escape_json_string(const char *ssid, char *clean_ssid) {
    while (*ssid) {
        if (*ssid == '"') {
            *clean_ssid++ = '\\';
            *clean_ssid++ = '"';
        } else if (*ssid == '\\') {
            *clean_ssid++ = '\\';
            *clean_ssid++ = '\\';
        } else {
            *clean_ssid++ = *ssid;
        }
        ssid++;
    }
}

void provisioning_cmd_dispatch(char* cmd) {
    jsmn_parser parser;
    jsmntok_t   token[64];

    jsmn_init(&parser);

    if (cmd[0] == 0x01) {
        cmd += 8; // skip a header
    }

    int32_t num_of_elements = jsmn_parse(&parser, cmd, strlen(cmd),
        token, sizeof(token) / sizeof(token[0]));

    if (num_of_elements < 0) {
        /* No command is found or JSON is corrupted */
        return;
    }

    for (uint32_t i = 0; i < num_of_elements; i++) {
        if (jsoneq(cmd, &token[i], "dialog_cmd") == 0) {

            char* cmd_str_start = cmd + token[i+1].start;
            uint32_t len = token[i+1].end - token[i+1].start;

            for (uint32_t j = 0; j < sizeof(prov_list_of_cmds) / sizeof(prov_list_of_cmds[0]); j++) {
                if (!strncmp(prov_list_of_cmds[j].cmd, cmd_str_start, len)) {
                    LOG_TRACE("CMD: %s\n", cmd_str_start);

                    prov_list_of_cmds[j].cb(prov_list_of_cmds[j].param, cmd, &token, num_of_elements);
                    break;
                }
            }
        }
    }
}

void provisioning_init(const provisioning_cb_t* params) {
    memcpy(&provisioning_cb, params, sizeof(provisioning_cb));
}

