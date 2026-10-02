/*
 * Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/******************************************************************************
 * File Name    : app_main.c
 * Tool-Chain   : e2Studio
 * Description  : This is a application file for peripheral role.
 *******************************************************************************/

/******************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *******************************************************************************/
#include <string.h>
#include "r_ble_api.h"
#include "rm_ble_abs.h"
#include "rm_ble_abs_api.h"
#include "discovery/r_ble_disc.h"
#include "gatt_db.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "profile_cmn/r_ble_servc_if.h"
#include "hal_data.h"

/* This code is needed for using FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
 #include "FreeRTOS.h"
 #include "task.h"
 #include "event_groups.h"
 #define BLE_EVENT_PATTERN    (0x0A0A)
EventGroupHandle_t g_ble_event_group_handle;
#endif
#include "r_ble_gapc.h"
#include "r_ble_gaps.h"
#include "r_ble_gats.h"
#include "r_ble_wifi_provisionings.h"

/******************************************************************************
 * User file includes
 *******************************************************************************/

/* Start user code for file includes. Do not edit comment generated here */
#include <assert.h>
#include "bsp_api.h"
#include "r_ble_api.h"
#include "r_ble_gtl.h"
#include "rm_ble_abs.h"
#include "rm_wifi_api.h"
#include "rm_cert.h"
#include "r_ble_gtl.h"
#include "rm_ble_abs_api.h"
#include "logging.h"
#include "provisioning.h"
#include "cli_ble_queue.h"
#include "ra6w1_platform_nvparam.h"
#include "ble_thread.h"
#include "common_utils.h"
#ifdef RM_MAP_PERSISTANT_W
 #include "rm_map_persistant_w.h"
#else
 #include "rm_vee_flash_w_rrq_nvram.h"
#endif

/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG                      "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE    (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN       (245)
#define BLE_GATTS_QUEUE_NUM              (1)

/******************************************************************************
 * User macro definitions
 *******************************************************************************/

/* Start user code for macro definitions. Do not edit comment generated here */
#define BLE_FAST_ADVERTISEMENT           (1)
#define BLE_MAX_PACKET_SIZE              (220)
#define BLE_RF_FORCE_EN                  (1)
#define BLE_OUTPUT_BUF_LEN               (64)
#define BLE_LOG_LEVEL                    (LOG_LEVEL_DEBUG)
#define WIFI_EXT_CONNECT_METHOD          (1)

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated function prototype declarations
 *******************************************************************************/

/* Internal functions */
void         gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data);
void         gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t * p_data);
void         gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t * p_data);
void         vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t * p_data);
void         disc_comp_cb(uint16_t conn_hdl);
ble_status_t ble_init(void);
void         app_main(void);

/******************************************************************************
 * User function prototype declarations
 *******************************************************************************/

/* Start user code for function prototype declarations. Do not edit comment generated here */
static int          reboot(const prov_cmd_reboot_t * params);
static int          factory(const prov_cmd_factory_reset_t * params);
static int          wifi_scan(const prov_cmd_scan_t * params, prov_scanned_networks_t ** networks);
static int          wifi_ext_connect(const prov_cmd_select_ap_t * params);
static int          wifi_connect(const prov_cmd_select_ap_t * params);
static int          wifi_disconnect(const prov_cmd_disconnect_t * params);
static int          wifi_ping(const prov_cmd_network_info_t * params);
static void         ble_set_bd_addr(const char * bd_addr_string);
static ble_status_t ble_uninit(void);
static bool         gap_cb_user(uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data);
void                custom_conf_ble(void);
static char       * unescape_special_characters(int len, char * output, const char * input, size_t dst_size);

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated global variables
 *******************************************************************************/

/* GAP Service UUID */
static uint8_t GAPC_UUID[] = {0x00, 0x18};

/* Service discovery parameters */
static st_ble_disc_entry_t gs_disc_entries[] =
{
    {
        .p_uuid    = GAPC_UUID,
        .uuid_type = BLE_GATT_16_BIT_UUID_FORMAT,
        .serv_cb   = R_BLE_GAPC_ServDiscCb,
    },
};

/* Advertising Data */
static uint8_t gs_advertising_data[] =
{
    /* Flags */
    0x02,                              /**< Data Size */
    0x01,                              /**< Data Type */
    (0x06),                            /**< Data Value */
};

/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Complete Local Name */
    0x12,                                                                                                 /**< Data Size */
    0x09,                                                                                                 /**< Data Type */
    0x52, 0x41, 0x46, 0x57, 0x20, 0x54, 0x65, 0x6d, 0x70, 0x20, 0x47, 0x61, 0x74, 0x65, 0x77, 0x61, 0x79, /**< Data Value */

    /* Manufacturer Specific Data */
    0x06,                                                                                                 /**< Data Size */
    0xff,                                                                                                 /**< Data Type */
    0x55, 0x55, 0x55, 0x55, 0x55,                                                                         /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
    .p_peer_address             = NULL,                                                        ///< Peer address.
    .slow_advertising_interval  = 0x00000640,                                                  ///< Slow advertising interval. 1,000.0(ms)
    .slow_advertising_period    = 0x0000,                                                      ///< Slow advertising period.
    .p_advertising_data         = gs_advertising_data,                                         ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
    .advertising_data_length    = ARRAY_SIZE(gs_advertising_data),                             ///< Advertising data length (in bytes).
    .p_scan_response_data       = gs_scan_response_data,                                       ///< Scan response data. If p_scan_response_data is specified as NULL, scan response data is not set.
    .scan_response_data_length  = ARRAY_SIZE(gs_scan_response_data),                           ///< Scan response data length (in bytes).
    .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY,                        ///< Advertising Filter Policy.
    .advertising_channel_map    = (BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39), ///< Channel Map.
    .own_bluetooth_address_type = BLE_GAP_ADDR_RAND,                                           ///< Own Bluetooth address type.
    .own_bluetooth_address      = {0},
};

/* GATT server callback parameters */
ble_abs_gatt_server_callback_set_t gs_abs_gatts_cb_param[] =
{
    {
        .gatt_server_callback_function = gatts_cb,
        .gatt_server_callback_priority = 1,
    },
    {
        .gatt_server_callback_function = NULL,
    }
};

/* GATT client callback parameters */
ble_abs_gatt_client_callback_set_t gs_abs_gattc_cb_param[] =
{
    {
        .gatt_client_callback_function = gattc_cb,
        .gatt_client_callback_priority = 1,
    },
    {
        .gatt_client_callback_function = NULL,
    }
};

/* GATT server Prepare Write Queue parameters */
static st_ble_gatt_queue_elm_t gs_queue_elms[BLE_GATTS_QUEUE_ELEMENTS_SIZE];
static uint8_t                 gs_buffer[BLE_GATTS_QUEUE_BUFFER_LEN];
static st_ble_gatt_pre_queue_t gs_queue[BLE_GATTS_QUEUE_NUM] =
{
    {
        .p_buf_start = gs_buffer,
        .buffer_len  = BLE_GATTS_QUEUE_BUFFER_LEN,
        .p_queue     = gs_queue_elms,
        .queue_size  = BLE_GATTS_QUEUE_ELEMENTS_SIZE,
    }
};

/* Connection handle */
uint16_t g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

/******************************************************************************
 * User global variables
 *******************************************************************************/

/* Start user code for global variables. Do not edit comment generated here */
/* Connection phy parameters */
/* Scan parameters */
/* Scan phy parameters */
static ble_abs_scan_phy_parameter_t gs_scan_phy_parameter =
{
    .fast_scan_interval = 0x0060,      /* 60.0(ms) */
    .fast_scan_window   = 0x0030,      /* 30.0(ms) */
    .slow_scan_interval = 0x00A0,      /* 100.0(ms) */
    .slow_scan_window   = 0x0080,      /* 11.25(ms) */
    .scan_type          = BLE_GAP_SCAN_PASSIVE
};

/*static*/ ble_abs_scan_parameter_t gs_scan_parameter =
{
    .p_phy_parameter_1M        = &gs_scan_phy_parameter,

    // .p_phy_parameter_coded        = &gs_scan_phy_parameter,
    .fast_scan_period          = 0x0000, /* 0(ms) */
    .slow_scan_period          = 0x0000,
    .p_filter_data             = NULL,
    .filter_data_length        = 0,
    .filter_ad_type            = 0x00,
    .device_scan_filter_policy = BLE_GAP_SCAN_ALLOW_ADV_ALL, /* receive ALL devices (no whitelist) */
    .filter_duplicate          = BLE_GAP_SCAN_FILT_DUPLIC_DISABLE,
};

/* Connection phy parameters */
static ble_abs_connection_phy_parameter_t gs_connection_phy_parameter =
{
    .connection_interval      = 0x0028, /* 50.0(ms) */
    .supervision_timeout      = 0x0200, /* 5,120(ms) */
    .connection_slave_latency = 0x0000,
};

/* Connection device address */
static ble_device_address_t gs_connection_device_address;

/* Connection parameters */
static ble_abs_connection_parameter_t gs_connection_parameter =
{
    .p_connection_phy_parameter_1M = &gs_connection_phy_parameter,
    .p_device_address              = &gs_connection_device_address,
    .filter_parameter              = BLE_GAP_INIT_FILT_USE_ADDR,
    .connection_timeout            = 0x05, /* 5(s) */
};
typedef enum e_mfr_app_state
{
    MFR_APP_STATE_STOPPED,
    MFR_APP_STATE_STOPPING,
    MFR_APP_STATE_RUNNING,
} mfr_app_state_t;

/* Connection handle */

/* Advertisement handle */
volatile uint8_t g_adv_hdl   = BLE_GAP_INVALID_ADV_HDL;
mfr_app_state_t  g_app_state = MFR_APP_STATE_RUNNING; /* Application starts with running state */
typedef enum
{
    eWiFiSecurityOpen_app = 0,                        // Open - No Security
    eWiFiSecurityWEP_app,                             // WEP
    eWiFiSecurityWPA_app,                             // WPA
    eWiFiSecurityWPA2_app,                            // WPA2 (RSN)
    eWiFiSecurityWPA_AUTO_app,                        // WPA & WPA2 (RSN)
    eWiFiSecurityOWE_app,                             // WPA3 OWE
    eWiFiSecuritySAE_app,                             // WPA3 SAE
    eWiFiSecurityRSN_SAE_app,                         // WPA2 (RSN) & WPA3 SAE
    eWiFiSecurityWPA_EAP_app,                         // WPA Enterprise
    eWiFiSecurityWPA2_EAP_app,                        // WPA2 Enterprise
    eWiFiSecurityWPA_AUTO_EAP_app,                    // WPA & WPA2 Enterprise
    eWiFiSecurityWPA3_EAP_app,                        // WPA3 Enterprise
    eWiFiSecurityWPA2_AUTO_EAP_app,                   // WPA2 & WPA3 Enterprise
    eWiFiSecurityWPA3_EAP_192B_app,                   // WPA3 192B Enterprise DA16200 not support
    eWiFiSecurityNotSupported_app                     // Unknown Security
} WIFISecurity_app_t;

extern bool reset(void);
extern int  factory_reset(int reboot_flag);

extern char wifi_networks[];
static char output_buf[BLE_OUTPUT_BUF_LEN];

typedef struct
{
    uint16_t remain;
    uint16_t total;
} prov_header_t;

static bool          flag_start = false;
static prov_header_t ble_prov_hdr;

static const char * msg2string (uint16_t msg_type)
{
#define MSG_2_STRING(msg)    case msg: \
        return #msg;

    switch (msg_type)
    {
        /* WiFiCommand */
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_WRITE_COMP)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_READ_REQ)

        /* WiFiStatus */
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_READ_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_COMP)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_READ_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_REQ)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_COMP)
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_READ_REQ)

        /* WiFiOutput */
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFIOUTPUT_READ_REQ)

        /* WiFiProvisoning */
        MSG_2_STRING(BLE_WIFI_PROVISIONINGS_EVENT_WIFIPROVISIONING_READ_REQ)

        default:

            return "Unknown";
    }

#undef MSG_2_STRING
}

static const char * WiFiErrCode2string (WIFIReturnCode_t rc)
{
#define ERRCODE_2_STRING(err_code)    case err_code: \
        return #err_code;

    switch (rc)
    {
        ERRCODE_2_STRING(eWiFiSuccess)
        ERRCODE_2_STRING(eWiFiFailure)
        ERRCODE_2_STRING(eWiFiTimeout)
        ERRCODE_2_STRING(eWiFiNotSupported)

        default:

            return "Unknown";
    }

#undef ERRCODE_2_STRING
}

static const char * RmCertErrCode2string (rm_cert_err_t rc)
{
#define ERRCODE_2_STRING(err_code)    case err_code: \
        return #err_code;

    switch (rc)
    {
        ERRCODE_2_STRING(RM_CERT_ERR_OK)
        ERRCODE_2_STRING(RM_CERT_ERR_NOK)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_MODULE)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_TYPE)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_FORMAT)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_LENGTH)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_FLASH_ADDR)
        ERRCODE_2_STRING(RM_CERT_ERR_INVALID_PARAMS)
        ERRCODE_2_STRING(RM_CERT_ERR_FOPEN_FAILED)
        ERRCODE_2_STRING(RM_CERT_ERR_MEM_FAILED)
        ERRCODE_2_STRING(RM_CERT_ERR_EMPTY_CERTIFICATE)

        default:

            return "Unknown";
    }

#undef ERRCODE_2_STRING
}

static const char * WiFiSecType2string (WIFISecurity_app_t sec_type)
{
#define SECTYPE_2_STRING(type)    case type: \
        return #type;

    switch (sec_type)
    {
        SECTYPE_2_STRING(eWiFiSecurityOpen_app)
        SECTYPE_2_STRING(eWiFiSecurityWEP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA_AUTO_app)
        SECTYPE_2_STRING(eWiFiSecurityOWE_app)
        SECTYPE_2_STRING(eWiFiSecuritySAE_app)
        SECTYPE_2_STRING(eWiFiSecurityRSN_SAE_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA_EAP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_EAP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA_AUTO_EAP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA3_EAP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_AUTO_EAP_app)
        SECTYPE_2_STRING(eWiFiSecurityWPA3_EAP_192B_app)
        SECTYPE_2_STRING(eWiFiSecurityNotSupported_app)

        default:

            return "Unknown";
    }

#undef SECTYPE_2_STRING
}

static const char * WiFiSecExtType2string (WIFISecurityExt_t sec_type)
{
#define SECTYPE_2_STRING(type)    case type: \
        return #type;

    switch (sec_type)
    {
        SECTYPE_2_STRING(eWiFiSecurityOpen_ext)
        SECTYPE_2_STRING(eWiFiSecurityWEP_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA3_ext)
        SECTYPE_2_STRING(eWiFiSecurityNotSupported_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA_WPA2_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_WPA3_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA3_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA3_192B_ent_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA_WPA2_ext)
        SECTYPE_2_STRING(eWiFiSecurityWPA2_WPA3_ext)
        SECTYPE_2_STRING(eWiFiSecurityMax_ext)

        default:

            return "Unknown";
    }

#undef SECTYPE_2_STRING
}

static const char * WiFiBandType2string (WIFIBand_t band_type)
{
#define BANDTYPE_2_STRING(type)    case type: \
        return #type;

    switch (band_type)
    {
        BANDTYPE_2_STRING(eWiFiBand2G)
        BANDTYPE_2_STRING(eWiFiBand5G)
        BANDTYPE_2_STRING(eWiFiBandDual)

        default:

            return "Unknown";
    }

#undef BANDTYPE_2_STRING
}

static const char * WiFiEntAuthExtType2string (WIFIEntAuthTypeExt_t auth_type)
{
#define AUTHTYPE_2_STRING(type)    case type: \
        return #type;

    switch (auth_type)
    {
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_NONE)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_PEAP_TTLS_FAST)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_PEAP)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_TTLS)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_FAST)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_TLS)
        AUTHTYPE_2_STRING(ENT_AUTH_TYPE_UNSUPPORTED)

        default:

            return "Unknown";
    }

#undef AUTHTYPE_2_STRING
}

static const char * WiFiEntAuthProtoExtType2string (WIFIEntAuthProtoExt_t proto_type)
{
#define PROTOTYPE_2_STRING(type)    case type: \
        return #type;

    switch (proto_type)
    {
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_NONE)
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_MSCHAPv2_GTC)
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_MSCHAPv2)
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_GTC)
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_TLS)
        PROTOTYPE_2_STRING(ENT_AUTH_PROTO_UNSUPPORTED)

        default:

            return "Unknown";
    }

#undef PROTOTYPE_2_STRING
}

static const char * WiFiPhyModeExtType2string (e_wifi_phy_mode_ext_t phy_type)
{
#define PHYTYPE_2_STRING(type)    case type: \
        return #type;

    switch (phy_type)
    {
        PHYTYPE_2_STRING(WIFI_MODE_AUTO_STA)
        PHYTYPE_2_STRING(WIFI_MODE_GN)
        PHYTYPE_2_STRING(WIFI_MODE_BG)
        PHYTYPE_2_STRING(WIFI_MODE_N_ONLY)
        PHYTYPE_2_STRING(WIFI_MODE_G_ONLY)
        PHYTYPE_2_STRING(WIFI_MODE_B_ONLY)

        PHYTYPE_2_STRING(WIFI_MODE_AN)
        PHYTYPE_2_STRING(WIFI_MODE_A_ONLY)
        PHYTYPE_2_STRING(WIFI_MODE_N_ONLY_5G)

        default:

            return "Unknown";
    }

#undef PHYTYPE_2_STRING
}

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated function definitions
 *******************************************************************************/

/******************************************************************************
 * Function Name: gap_cb
 * Description  : Callback function for GAP API.
 * Arguments    : uint16_t type -
 *                  Event type of GAP API.
 *              : ble_status_t result -
 *                  Event result of GAP API.
 *              : st_ble_vs_evt_data_t *p_data -
 *                  Event parameters of GAP API.
 * Return Value : none
 ******************************************************************************/
void gap_cb (uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP callback function common process. Do not edit comment generated here */
    if (gap_cb_user(type, result, p_data))
    {

        /* event handled by own application */
        return;
    }

/* End user code. Do not edit comment generated here */

    switch (type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            /* Get BD address for Advertising */
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);
            break;
        }

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t * p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *) p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
            }
            else
            {
                /* Restart advertising when connection failed */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }

            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Restart advertising when disconnected */
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            /* Send connection update response with value received on connection update request */
            st_ble_gap_conn_upd_req_evt_t * p_conn_upd_req_evt_param =
                (st_ble_gap_conn_upd_req_evt_t *) p_data->p_param;

            st_ble_gap_conn_param_t conn_updt_param =
            {
                .conn_intv_min = p_conn_upd_req_evt_param->conn_intv_min,
                .conn_intv_max = p_conn_upd_req_evt_param->conn_intv_max,
                .conn_latency  = p_conn_upd_req_evt_param->conn_latency,
                .sup_to        = p_conn_upd_req_evt_param->sup_to,
            };

            R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
                              BLE_GAP_CONN_UPD_MODE_RSP,
                              BLE_GAP_CONN_UPD_ACCEPT,
                              &conn_updt_param);
            break;
        }

/* Hint: Add cases of GAP event macros defined as BLE_GAP_XXX */
/* Start user code for GAP callback function event process. Do not edit comment generated here */
        case BLE_GAP_EVENT_ADV_REPT_IND:
        {
            /* Stop scan and store BD address when advertising data found */
            st_ble_gap_adv_rept_evt_t * p_adv_rept_param = (st_ble_gap_adv_rept_evt_t *) p_data->p_param;

            /* Correctly extract address and address type based on report type */
            if (p_adv_rept_param->adv_rpt_type == 0x01)
            {
                st_ble_gap_ext_adv_rept_t * p_ext_adv_rept = p_adv_rept_param->param.p_ext_adv_rpt;
                gs_connection_parameter.p_device_address->type = p_ext_adv_rept->addr_type;
                memcpy(gs_connection_parameter.p_device_address->addr, p_ext_adv_rept->p_addr, BLE_BD_ADDR_LEN);
            }
            else
            {
                st_ble_gap_adv_rept_t * p_adv_rpt = p_adv_rept_param->param.p_adv_rpt;
                gs_connection_parameter.p_device_address->type = p_adv_rpt->addr_type;
                memcpy(gs_connection_parameter.p_device_address->addr, p_adv_rpt->p_addr, BLE_BD_ADDR_LEN);
            }

            advRptParse(p_data->p_param);
            break;
        }

        case BLE_GAP_EVENT_SCAN_OFF:
        {
            printf("BLE scan off, restarting...\n");
            ble_status_t scan_ret =
                RM_BLE_ABS_StartScanning(&g_ble_abs0_ctrl, &gs_scan_parameter);
            if (BLE_SUCCESS == scan_ret)
            {
                printf("BLE scan restarted\n");
            }
            else
            {
                printf("BLE scan restart failed: %d\n", scan_ret);
            }

            break;
        }

        case BLE_GAP_EVENT_ADV_ON:
        {
            st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;

            printf("Legacy advertising started\n");
            break;
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

            printf("Legacy advertising stopped\n");
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gatts_cb
 * Description  : Callback function for GATT Server API.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Server API.
 *              : ble_status_t result -
 *                  Event result of GATT Server API.
 *              : st_ble_gatts_evt_data_t *p_data -
 *                  Event parameters of GATT Server API.
 * Return Value : none
 ******************************************************************************/
void gatts_cb (uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_GattsCb(type, result, p_data);
    switch (type)
    {
/* Hint: Add cases of GATT Server event macros defined as BLE_GATTS_XXX */
/* Start user code for GATT Server callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gattc_cb
 * Description  : Callback function for GATT Client API.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Client API.
 *              : ble_status_t result -
 *                  Event result of GATT Client API.
 *              : st_ble_gattc_evt_data_t *p_data -
 *                  Event parameters of GATT Client API.
 * Return Value : none
 ******************************************************************************/
void gattc_cb (uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVC_GattcCb(type, result, p_data);
    switch (type)
    {
        case BLE_GATTC_EVENT_CONN_IND:
        {
            /* Start discovery operation after connection established */
            R_BLE_DISC_Start(p_data->conn_hdl, gs_disc_entries, ARRAY_SIZE(gs_disc_entries), disc_comp_cb);
            break;
        }

/* Hint: Add cases of GATT Client event macros defined as BLE_GATTC_XXX */
/* Start user code for GATT Client callback function event process. Do not edit comment generated here */
        case BLE_GATTC_EVENT_EX_MTU_RSP:
        {
            st_ble_gattc_ex_mtu_rsp_evt_t * p_ex_mtu_rsp;
            p_ex_mtu_rsp = (st_ble_gattc_ex_mtu_rsp_evt_t *) p_data->p_param;

            APP_PRINT_INFO("--> BLE_GATTC_EVENT_EX_MTU_RSP: %d\n", p_ex_mtu_rsp->mtu);
            break;
        }

        default:
        {}
        break;

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: vs_cb
 * Description  : Callback function for Vendor Specific API.
 * Arguments    : uint16_t type -
 *                  Event type of Vendor Specific API.
 *              : ble_status_t result -
 *                  Event result of Vendor Specific API.
 *              : st_ble_vs_evt_data_t *p_data -
 *                  Event parameters of Vendor Specific API.
 * Return Value : none
 ******************************************************************************/
void vs_cb (uint16_t type, ble_status_t result, st_ble_vs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for vender specific callback function common process. Do not edit comment generated here */
    if (BLE_VS_EVENT_GET_ADDR_COMP == type)
    {
        /* Start advertising when BD address is ready */
        st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
        memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
        LOG_INFO("BD address from device: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 get_address->addr.addr[5],
                 get_address->addr.addr[4],
                 get_address->addr.addr[3],
                 get_address->addr.addr[2],
                 get_address->addr.addr[1],
                 get_address->addr.addr[0]);
        ble_status_t scan_ret = RM_BLE_ABS_StartScanning(&g_ble_abs0_ctrl, &gs_scan_parameter);
        if (BLE_SUCCESS == scan_ret)
        {
            printf("BLE scan started\n");
        }
    }

/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_VsCb(type, result, p_data);
    switch (type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gaps_cb
 * Description  : Callback function for GAP Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of GAP Service server feature.
 *              : ble_status_t result -
 *                  Event result of GAP Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of GAP Service server feature.
 * Return Value : none
 ******************************************************************************/
static void gaps_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GAP Service server events defined in e_ble_gaps_event_t */
/* Start user code for GAP Service Server callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gats_cb
 * Description  : Callback function for GATT Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Service server feature.
 *              : ble_status_t result -
 *                  Event result of GATT Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of GATT Service server feature.
 * Return Value : none
 ******************************************************************************/
static void gats_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Service Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GATT Service server events defined in e_ble_gats_event_t */
/* Start user code for GATT Service Server callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: wifi_provisionings_cb
 * Description  : Callback function for wifi_provisioning server feature.
 * Arguments    : uint16_t type -
 *                  Event type of wifi_provisioning server feature.
 *              : ble_status_t result -
 *                  Event result of wifi_provisioning server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of wifi_provisioning server feature.
 * Return Value : none
 ******************************************************************************/
static void wifi_provisionings_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for wifi_provisioning Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of wifi_provisioning server events defined in e_ble_wifi_provisionings_event_t */
/* Start user code for wifi_provisioning Server callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gapc_cb
 * Description  : Callback function for GAP Service client feature.
 * Arguments    : uint16_t type -
 *                  Event type of GAP Service client feature.
 *              : ble_status_t result -
 *                  Event result of GAP Service client feature.
 *              : st_ble_servc_evt_data_t *p_data -
 *                  Event parameters of GAP Service client feature.
 * Return Value : none
 ******************************************************************************/
static void gapc_cb (uint16_t type, ble_status_t result, st_ble_servc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GAP Service client events defined in e_ble_gapc_event_t */
/* Start user code for GAP Service Client callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: disc_comp_cb
 * Description  : Callback function for Service Discovery.
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void disc_comp_cb (uint16_t conn_hdl)
{
/* Hint: Input process such as GATT operation */
/* Start user code for Discovery Complete callback function. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
}

/******************************************************************************
 * Function Name: ble_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : none
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
ble_status_t ble_init (void)
{
    ble_status_t status;
    fsp_err_t    err;

    /* Initialize BLE */
    err = RM_BLE_ABS_Open(&g_ble_abs0_ctrl, &g_ble_abs0_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* Initialize GATT Database */
    status = R_BLE_GATTS_SetDbInst(NULL);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT server */
    status = R_BLE_SERVS_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /*Initialize GATT client */
    status = R_BLE_SERVC_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Set Prepare Write Queue */
    R_BLE_GATTS_SetPrepareQueue(gs_queue, BLE_GATTS_QUEUE_NUM);

    /* Initialize GATT Discovery Library */
    status = R_BLE_DISC_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GAP Service server API */
    status = R_BLE_GAPS_Init(gaps_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT Service server API */
    status = R_BLE_GATS_Init(gats_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize wifi_provisioning server API */
    status = R_BLE_WIFI_PROVISIONINGS_Init(wifi_provisionings_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GAP Service client API */
    status = R_BLE_GAPC_Init(gapc_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    return status;
}

/******************************************************************************
 * Function Name: app_main
 * Description  : Application main function with main loop
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void app_main (void)
{
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)

    /* Create Event Group */
    g_ble_event_group_handle = xEventGroupCreate();
    assert(g_ble_event_group_handle);
#endif

    /* Initialize BLE and profiles */
    ble_init();

/* Hint: Input process that should be done before main loop such as calling initial function or variable definitions */
/* Start user code for process before main loop. Do not edit comment generated here */
    ble_status_t ret;
    log_level_set(BLE_LOG_LEVEL);

#if BLE_RF_FORCE_EN

    /* Set RF switch to BLE */
    CRG_TOP->CLK_AMBA_REG_b.OQSPI_GPIO_MODE = 1;
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_01, BSP_IO_LEVEL_HIGH);
#endif

    APP_PRINT_INFO("Provisioning application is starting...\n");

    provisioning_cb_t prov_params =
    {
        .reboot           = reboot,
        .factory          = factory,
        .wifi_scan        = wifi_scan,
        .wifi_ext_connect = WIFI_EXT_CONNECT_METHOD ? wifi_ext_connect : wifi_connect,
        .wifi_disconnect  = wifi_disconnect,
        .wifi_ping        = wifi_ping,
    };

    provisioning_init(&prov_params);
    char        * p_bd_addr = NULL;
    bool          is_sleep3_wakeup;
    cli_ble_msg_t svc_stop_msg;

    cli_ble_queue_init();

    is_sleep3_wakeup = RM_PMGR_W_dpm_is_wakeup() || RM_PMGR_W_IsSleep3Wakeup();

    if (is_sleep3_wakeup)
    {
        svc_stop_msg.type = CLI_BLE_MSG_TYPE_SVC_STOP;
        cli_ble_queue_enqueue(&svc_stop_msg);
    }

/* End user code. Do not edit comment generated here */

    /* main loop */
    while (1)
    {
        /* Process BLE Event */
        R_BLE_Execute();

/* When this BLE application works on the FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
        if (0 != R_BLE_IsTaskFree())
        {
            /* If the BLE Task has no operation to be processed, it transits block state until the event from RF transciever occurs. */
            xEventGroupWaitBits(g_ble_event_group_handle,
                                (EventBits_t) BLE_EVENT_PATTERN,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);
        }
#endif

/* Hint: Input process that should be done during main loop such as calling processing functions */
/* Start user code for process during main loop. Do not edit comment generated here */
        cli_ble_msg_t msg;
        if (cli_ble_queue_dequeue(&msg, 0))
        {
            switch (msg.type)
            {
                case CLI_BLE_MSG_TYPE_SVC_START:
                {
                    /* Started already */
                    if (MFR_APP_STATE_RUNNING == g_app_state)
                    {
                        break;
                    }

#ifndef RM_MAP_PERSISTANT_W
                    p_bd_addr = (uint8_t *) read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                    ENV_GROUP_BLECFG,
                                                    "PUBLIC_BD_ADDR",
                                                    (char *) &p_bd_addr);
#endif
                    if (p_bd_addr)
                    {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    g_app_state = MFR_APP_STATE_RUNNING;
                    break;
                }

                case CLI_BLE_MSG_TYPE_SVC_STOP:
                {
                    /* Already not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    g_app_state = MFR_APP_STATE_STOPPING;

                    /* Cleaning up the connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        /* 0x13: Remote User Terminated Connection. */
                        ret = R_BLE_GAP_Disconnect(g_conn_hdl, 0x13);
                        if (BLE_SUCCESS != ret)
                        {
                            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
                        }
                    }

                    /* Cleaning up the advertisement */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        ret = R_BLE_GAP_StopAdv(g_adv_hdl);
                        if (BLE_SUCCESS != ret)
                        {
                            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_ADV_START:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cannot advertise while in active connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        break;
                    }

                    /* We are already advertising */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        break;
                    }

                    ret = RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
                    assert(BLE_SUCCESS == ret);
                    break;
                }

                case CLI_BLE_MSG_TYPE_ADV_STOP:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cleaning up the advertisement */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        ret = R_BLE_GAP_StopAdv(g_adv_hdl);
                        if (BLE_SUCCESS != ret)
                        {
                            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_DISCONNECT:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cleaning up the connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        /* 0x13: Remote User Terminated Connection. */
                        ret = R_BLE_GAP_Disconnect(g_conn_hdl, 0x13);
                        if (BLE_SUCCESS != ret)
                        {
                            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_BD_ADDR_GET:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    ret = R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_PUBLIC);
                    assert(BLE_SUCCESS == ret);
                    break;
                }

                default:
                {
                    printf("Received CLI msg of type (%d) which was not handled.\n", msg.type);
                }
            }
        }

        if ((MFR_APP_STATE_STOPPING == g_app_state) &&
            (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl) &&
            (BLE_GAP_INVALID_ADV_HDL == g_adv_hdl))
        {
            ret = ble_uninit();
            assert(BLE_SUCCESS == ret);

            g_app_state = MFR_APP_STATE_STOPPED;
        }

        if (MFR_APP_STATE_STOPPED != g_app_state)
        {
            /* Process BLE Event */
            ret = R_BLE_Execute();
            assert(BLE_SUCCESS == ret);
        }
        else
        {
            vTaskDelay(10);

            continue;
        }

/* End user code. Do not edit comment generated here */
    }

/* Hint: Input process that should be done after main loop such as calling closing functions */
/* Start user code for process after main loop. Do not edit comment generated here */
    cli_ble_queue_uninit();

/* End user code. Do not edit comment generated here */

    /* Terminate BLE */
    RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 * User function definitions
 *******************************************************************************/

/* Start user code for function definitions. Do not edit comment generated here */
static int reboot (const prov_cmd_reboot_t * params)
{
    return reset();
}

static int factory (const prov_cmd_factory_reset_t * params)
{
    return factory_reset(0);
}

static bool is_wifi_network_hidden (const char * ssid, unsigned ssid_len)
{
    if ((ssid_len == 1) && ((ssid[0]) == '\t'))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
 * Function Name: wifi_security_type_reduce
 * Description  : Converts an extended WiFi security type(WIFISecurityExt_t) to
 *                a standard WiFi security type supported by mobile application
 *                (WIFISecurity_app_t). This reduction is required because
 *                mobile provisioning clients (Android/iOS) do not support
 *                extended security enumerations.
 * Arguments    : WIFISecurityExt_t sec_type -
 *                  Extended WiFi security type received from provisioning.
 * Return Value : WIFISecurity_app_t -
 *                  Corresponding reduced WiFi security type. Returns
 *                  eWiFiSecurityNotSupported_app for unsupported or unknowns.
 ******************************************************************************/

static WIFISecurity_app_t wifi_security_type_reduce (WIFISecurityExt_t sec_type)
{
    switch (sec_type)
    {
        case eWiFiSecurityOpen_ext:
        {
            return eWiFiSecurityOpen_app;
        }

        case eWiFiSecurityWEP_ext:
        {
            return eWiFiSecurityWEP_app;
        }

        case eWiFiSecurityWPA_ext:
        {
            return eWiFiSecurityWPA_app;
        }

        case eWiFiSecurityWPA2_ext:
        {
            return eWiFiSecurityWPA2_app;
        }

        case eWiFiSecurityWPA_WPA2_ext:
        {
            return eWiFiSecurityWPA_AUTO_app;
        }

        case eWiFiSecurityWPA2_WPA3_ext:
        {
            return eWiFiSecurityRSN_SAE_app;
        }

        case eWiFiSecurityWPA2_ent_ext:
        {
            return eWiFiSecurityWPA2_EAP_app;
        }

        case eWiFiSecurityWPA_WPA2_ent_ext:
        {
            return eWiFiSecurityWPA_AUTO_EAP_app;
        }

        case eWiFiSecurityWPA2_WPA3_ent_ext:
        {
            return eWiFiSecurityWPA2_AUTO_EAP_app;
        }

        case eWiFiSecurityWPA3_ext:
        {
            return eWiFiSecuritySAE_app;
        }

        case eWiFiSecurityWPA3_OWE_ext:
        {
            return eWiFiSecurityOWE_app;
        }

        case eWiFiSecurityWPA_ent_ext:
        {
            return eWiFiSecurityWPA_EAP_app;
        }

        case eWiFiSecurityWPA3_ent_ext:
        {
            return eWiFiSecurityWPA3_EAP_app;
        }

        case eWiFiSecurityWPA3_192B_ent_ext:
        {
            return eWiFiSecurityWPA3_EAP_192B_app;
        }

        case eWiFiSecurityNotSupported_ext:
        case eWiFiSecurityMax_ext:
        default:

            return eWiFiSecurityNotSupported_app;
    }
}

/******************************************************************************
 * Function Name: wifi_security_type_ext
 * Description  : Converts app WiFi security type(WIFISecurity_app_t) to
 *                a ext WiFi security type (WIFISecurityExt_t).
 * Arguments    : WIFISecurity_app_t sec_type -
 *                  WiFi security type received from provisioning.
 * Return Value : WIFISecurityExt_t -
 *                  Extended WiFi security type. Returns
 *                  eWiFiSecurityNotSupported_ext for unsupported or unknowns.
 ******************************************************************************/
static WIFISecurityExt_t wifi_security_type_ext (WIFISecurity_app_t sec_type)
{
    switch (sec_type)
    {
        case eWiFiSecurityOpen_app:
        {
            return eWiFiSecurityOpen_ext;
        }

        case eWiFiSecurityWEP_app:
        {
            return eWiFiSecurityWEP_ext;
        }

        case eWiFiSecurityWPA_app:
        {
            return eWiFiSecurityWPA_ext;
        }

        case eWiFiSecurityWPA2_app:
        {
            return eWiFiSecurityWPA2_ext;
        }

        case eWiFiSecurityWPA_AUTO_app:
        {
            return eWiFiSecurityWPA_WPA2_ext;
        }

        case eWiFiSecurityRSN_SAE_app:
        {
            return eWiFiSecurityWPA2_WPA3_ext;
        }

        case eWiFiSecurityWPA2_EAP_app:
        {
            return eWiFiSecurityWPA2_ent_ext;
        }

        case eWiFiSecurityWPA_AUTO_EAP_app:
        {
            return eWiFiSecurityWPA_WPA2_ent_ext;
        }

        case eWiFiSecurityWPA2_AUTO_EAP_app:
        {
            return eWiFiSecurityWPA2_WPA3_ent_ext;
        }

        case eWiFiSecuritySAE_app:
        {
            return eWiFiSecurityWPA3_ext;
        }

        case eWiFiSecurityOWE_app:
        {
            return eWiFiSecurityWPA3_OWE_ext;
        }

        case eWiFiSecurityWPA_EAP_app:
        {
            return eWiFiSecurityWPA_ent_ext;
        }

        case eWiFiSecurityWPA3_EAP_app:
        {
            return eWiFiSecurityWPA3_ent_ext;
        }

        case eWiFiSecurityWPA3_EAP_192B_app:
        {
            return eWiFiSecurityWPA3_192B_ent_ext;
        }

        case eWiFiSecurityNotSupported_app:
        default:

            return eWiFiSecurityNotSupported_ext;
    }
}

static int wifi_scan (const prov_cmd_scan_t * params, prov_scanned_networks_t ** networks)
{
#define MAX_WIFI_SCAN_RESULTS    20

    LOG_TRACE("\n");

    static WIFIScanResult_t scan_data[MAX_WIFI_SCAN_RESULTS];
    memset(scan_data, 0, sizeof(scan_data));

    WIFIReturnCode_t rc = WIFI_Scan(&scan_data[0], MAX_WIFI_SCAN_RESULTS);
    if (rc == eWiFiSuccess)
    {
        uint32_t num_of_found_networks = 0;

        for (uint32_t i = 0; i < MAX_WIFI_SCAN_RESULTS; i++)
        {
            if (scan_data[i].ucSSIDLength != 0)
            {
                num_of_found_networks++;
            }
            else
            {
                break;
            }
        }

        prov_scanned_networks_t * networks_db =
            malloc(sizeof(prov_scanned_networks_t) + (num_of_found_networks * sizeof(prov_ap_entry_t)));
        assert(networks_db);

        networks_db->num_of_networks = 0;

        LOG_INFO("Found %d networks:\n", num_of_found_networks);
        for (uint32_t i = 0, j = 0; i < num_of_found_networks; i++)
        {
            WIFISecurity_app_t security = wifi_security_type_reduce(scan_data[i].xSecurity);
            if ((eWiFiSecurityNotSupported_app == security) ||
                (is_wifi_network_hidden(scan_data[i].ucSSID, scan_data[i].ucSSIDLength)))
            {
                LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %30s -> %s [SKIP]\n",
                         scan_data[i].ucSSIDLength,
                         scan_data[i].ucSSID,
                         scan_data[i].cRSSI,
                         scan_data[i].ucChannel,
                         WiFiSecExtType2string(scan_data[i].xSecurity),
                         WiFiSecType2string(security));
                continue;
            }

            prov_ap_entry_t entry =
            {
                .rssi          = scan_data[i].cRSSI,
                .channel       = scan_data[i].ucChannel,
                .security_type = security,
            };

            strncpy(entry.ssid, scan_data[i].ucSSID, MAX(sizeof(entry.ssid) - 1, scan_data[i].ucSSIDLength));

            networks_db->networks[j++] = entry;
            networks_db->num_of_networks++;

            LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %30s -> %s\n",
                     scan_data[i].ucSSIDLength,
                     scan_data[i].ucSSID,
                     scan_data[i].cRSSI,
                     scan_data[i].ucChannel,
                     WiFiSecExtType2string(scan_data[i].xSecurity),
                     WiFiSecType2string(security));
        }

        LOG_INFO("----------------------\n\n");

        *networks = networks_db;
    }
    else
    {
        LOG_ERROR("Unable to scan WiFi networks: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;

#undef MAX_WIFI_SCAN_RESULTS
}

static void wifi_network_params_print (const WIFINetworkParams_t * params)
{
    WIFISecurity_app_t security = wifi_security_type_reduce(params->xSecurity);
    LOG_INFO("WIFINetworkParams_t: {\n"
             "  - ucSSID:           %.*s\n"
             "  - ucSSIDLength:     %d\n"
             "  - cPassphrase:      %.*s\n"
             "  - ucLength:         %d\n"
             "  - xSecurity:        %s\n"
             "  - ucDefWEPKeyIndex: %d\n"
             "  - ucChannel:        %d\n"
             "}\n",
             sizeof(params->ucSSID),
             params->ucSSID,
             params->ucSSIDLength,
             sizeof(params->xPassword.xWPA.cPassphrase),
             params->xPassword.xWPA.cPassphrase,
             params->xPassword.xWPA.ucLength,
             WiFiSecType2string(security),
             params->ucDefaultWEPKeyIndex,
             params->ucChannel);
}

static void wifi_enterprise_net_params_print (const WIFIEnterpriseNetParams_t * params)
{
    LOG_INFO("WIFIEnterpriseNetParams_t: {\n"
             "  - ucEntAuthType:    %s\n"
             "  - ucEntAuthProto:   %s\n"
             "  - ucID:             %.*s\n"
             "  - ucIDLength:       %d\n"
             "  - ucPassword:       %.*s\n"
             "  - ucPasswordLength: %d\n"
             "  - ucCert:           %.*s\n"
             "  - ucCertLength:     %d\n"
             "  - ucPrivKey:        %.*s\n"
             "  - ucPrivKeyLength:  %d\n"
             "}\n",
             WiFiEntAuthExtType2string(params->ucEntAuthType),
             WiFiEntAuthProtoExtType2string(params->ucEntAuthProto),
             sizeof(params->ucID),
             params->ucID,
             params->ucIDLength,
             sizeof(params->ucPassword),
             params->ucPassword,
             params->ucPasswordLength,
             sizeof(params->ucCert),
             params->ucCert,
             params->ucCertLength,
             sizeof(params->ucPrivKey),
             params->ucPrivKey,
             params->ucPrivKeyLength);
}

static void wifi_network_ext_params_print (const WIFINetworkParamsExt_t * params)
{
    LOG_INFO("WIFINetworkParamsExt_t: {\n"
             "  - ucWiFi_mode:      %s\n"
             "  - ucBand:           %s\n"
             "  - xNetworkParams:   ***\n"
             "  - xEntNetParams:    ***\n"
             "}\n",
             WiFiPhyModeExtType2string(params->ucWiFi_mode),
             WiFiBandType2string(params->ucBand));

    wifi_network_params_print(&params->xNetworkParams);
    wifi_enterprise_net_params_print(&params->xEntNetParams);
}

static bool wifi_enterprise_cert_key_set (WIFINetworkParamsExt_t * net_params)
{
    LOG_TRACE("\n");

    rm_cert_format_t format = RM_CERT_FORMAT_PEM;
    size_t           outlen = CERT_MAX_LENGTH;

    memset(net_params->xEntNetParams.ucCert, 0xff, sizeof(net_params->xEntNetParams.ucCert));
    memset(net_params->xEntNetParams.ucPrivKey, 0xff, sizeof(net_params->xEntNetParams.ucPrivKey));

    rm_cert_err_t rc =
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_WPA_ENT_CERTIFICATE_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_WPA_ENT_CERTIFICATE_ADDR),
                     &format,
                     (unsigned char *) net_params->xEntNetParams.ucCert,
                     &outlen);

    net_params->xEntNetParams.ucCertLength = outlen;

    if ((net_params->xEntNetParams.ucCert[0] == 0xff) || (rc != RM_CERT_ERR_OK))
    {
        LOG_ERROR("Unable to get a certificate: %s (%d)\n", RmCertErrCode2string(rc), rc);

        return false;
    }

    rc =
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_WPA_ENT_PRIVATE_KEY_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_WPA_ENT_PRIVATE_KEY_ADDR),
                     &format,
                     (unsigned char *) net_params->xEntNetParams.ucPrivKey,
                     &outlen);

    net_params->xEntNetParams.ucPrivKeyLength = outlen;

    if ((net_params->xEntNetParams.ucPrivKey[0] == 0xff) || (rc != RM_CERT_ERR_OK))
    {
        LOG_ERROR("Unable to get a private key: %s (%d)\n", RmCertErrCode2string(rc), rc);

        return false;
    }

    return true;
}

static char * unescape_special_characters (int len, char * output, const char * input, size_t dst_size)
{
    size_t j = 0;

    for (int i = 0; i < len && j + 1 < dst_size; i++)
    {
        char c = input[i];
        if ((c == '\\') && (i + 1 < len))
        {
            char next = input[i + 1];
            switch (next)
            {
                case '\\':
                {
                    output[j++] = '\\'; i++;
                    break;
                }

                case '"':
                {
                    output[j++] = '"'; i++;
                    break;
                }

                case '/':
                {
                    output[j++] = '/'; i++;
                    break;
                }

                case '\'':
                {
                    output[j++] = '\''; i++;
                    break;
                }

                case 't':
                {
                    output[j++] = '\t'; i++;
                    break;
                }

                default:
                {
                    output[j++] = '\\';
                    break;
                }
            }

            continue;
        }

        output[j++] = c;
    }

    output[j] = '\0';

    return output;
}

static fsp_err_t store_provisioning_params (const WIFINetworkParams_t * net_params, uint32_t is_hidden)
{
    map_persistant_w_instance_ctrl_t * p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();
    FSP_ERROR_RETURN(MAP_PERSISTANT_W_OPEN == p_ctrl->map_persistant_w_open, FSP_ERR_NOT_OPEN);

    /* Write params struct */
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SYS_MODE, WIFI_DEVICE_MODE_EXT_STATION);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_BAND, WPA_SETBAND_AUTO);

    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, net_params->xSecurity);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_HIDDEN_SSID, is_hidden);
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, net_params->ucSSID);
    if (net_params->xSecurity == eWiFiSecurityWEP)
    {
        RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl,
                                         ENV_GROUP_WIFIPROFILE,
                                         WIFI_PROFILE_WEPKEY0_0,
                                         net_params->xPassword.xWEP[0].cKey);
    }
    else
    {
        RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl,
                                         ENV_GROUP_WIFIPROFILE,
                                         WIFI_PROFILE_ENCKEY_0,
                                         net_params->xPassword.xWPA.cPassphrase);
    }

#if CFG_PMGR
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENABLE_DPM, 1);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl,
                                  ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME,
                                  DFLT_DPM_KEEPALIVE_TIME);

    // RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DPM_USER_WAKEUP_TIME, MIN_DPM_USER_WAKEUP_TIME);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DPM_USER_WAKEUP_TIME, 0);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl,
                                  ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT,
                                  DFLT_DPM_TIM_WAKEUP_COUNT);
#endif                                 /* CFG_PMGR */

    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);

    return FSP_SUCCESS;
}

static int wifi_ext_connect (const prov_cmd_select_ap_t * params)
{
    LOG_TRACE("\n");
    char unescaped_ssid[wificonfigMAX_SSID_LEN + 1];
    char unescaped_pass[wificonfigMAX_SSID_LEN + 1];

    uint32_t           ssid_len     = strnlen(params->ssid, sizeof(params->ssid));
    uint32_t           pass_len     = strnlen(params->password, sizeof(params->password));
    WIFISecurity_app_t security_app = (WIFISecurity_t) params->security_type;
    WIFISecurityExt_t  security     = wifi_security_type_ext(security_app);

    WIFINetworkParamsExt_t net_ext_params =
    {
        .xNetworkParams.ucChannel = 4,
        .xNetworkParams.xSecurity = security,
        .ucBand                   = eWiFiBandDual,
    };

    WIFINetworkParams_t       * net_params            = &net_ext_params.xNetworkParams;
    WIFIEnterpriseNetParams_t * net_enterprise_params = &net_ext_params.xEntNetParams;

    memset(&net_ext_params.xEntNetParams, 0, sizeof(WIFIEnterpriseNetParams_t));
    memset(&net_ext_params.xApNetParams, 0, sizeof(WIFIApNetParams_t));

    net_ext_params.ucBand        = eWiFiBandDual;
    net_ext_params.ucWiFi_mode   = WIFI_MODE_AUTO_STA;
    net_ext_params.pmf           = PMF_DEFAULT;
    net_ext_params.sae_groups[0] = '\0';

    net_ext_params.hidden_ssid = (params->is_hidden == 1) ? true : false;
    unescape_special_characters(ssid_len, unescaped_ssid, params->ssid, sizeof(unescaped_ssid));
    strncpy(net_params->ucSSID, unescaped_ssid, sizeof(net_params->ucSSID));
    net_params->ucSSIDLength = strlen(unescaped_ssid);

    switch (security)
    {
        case eWiFiSecurityOpen_ext:
        {
            break;
        }

        case eWiFiSecurityWPA_ext:
        case eWiFiSecurityWPA2_ext:
        case eWiFiSecurityWPA3_ext:
        {
            unescape_special_characters(pass_len, unescaped_pass, params->password, sizeof(unescaped_pass));
            strncpy(net_params->xPassword.xWPA.cPassphrase, unescaped_pass,
                    sizeof(net_params->xPassword.xWPA.cPassphrase));
            net_params->xPassword.xWPA.ucLength = pass_len;
            break;
        }

        case eWiFiSecurityWEP_ext:
        {
            unescape_special_characters(pass_len, unescaped_pass, params->password, sizeof(unescaped_pass));
            strncpy(net_params->xPassword.xWEP[0].cKey, unescaped_pass, sizeof(net_params->xPassword.xWEP[0].cKey));
            net_params->xPassword.xWEP[0].ucLength = pass_len;
            net_params->ucDefaultWEPKeyIndex       = 0;
            break;
        }

        case eWiFiSecurityWPA2_ent:
        {
            net_enterprise_params->ucEntAuthType  = (WIFIEntAuthTypeExt_t) params->eap_auth_mode;
            net_enterprise_params->ucEntAuthProto = (WIFIEntAuthProtoExt_t) params->eap_phase2;

            strncpy(net_enterprise_params->ucID, params->ssid, sizeof(net_enterprise_params->ucID));
            net_enterprise_params->ucIDLength = ssid_len;

            unescape_special_characters(pass_len, unescaped_pass, params->password, sizeof(unescaped_pass));
            strncpy(net_enterprise_params->ucPassword, unescaped_pass, sizeof(net_enterprise_params->ucPassword));
            net_enterprise_params->ucPasswordLength = pass_len;

            if (false == wifi_enterprise_cert_key_set(&net_ext_params))
            {
                LOG_ERROR("Unable to get / apply a certificate\n");

                return eWiFiFailure;
            }

            break;
        }

        default:
        {
            LOG_ERROR("Unable to connect to WiFi network due to "
                      "unsupported security type: %s (%d)\n",
                      WiFiSecType2string(security_app),
                      security_app);

            return eWiFiNotSupported;
        }
    }

    ;

    wifi_network_ext_params_print(&net_ext_params);

    WIFIReturnCode_t rc = WIFI_ConnectAPExt(&net_ext_params);
    if (rc != eWiFiSuccess)
    {
        LOG_ERROR("Unable to connect to WiFi network: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }
    else
    {
        if (FSP_SUCCESS != store_provisioning_params(net_params, params->is_hidden))
        {
            LOG_ERROR("Unable to store NVRAM parameters\n");
        }
    }

    return rc;
}

static int wifi_connect (const prov_cmd_select_ap_t * params)
{
    LOG_TRACE("\n");

    WIFINetworkParams_t net_params =
    {
        .ucChannel = 4,
        .xSecurity = (WIFISecurity_t) params->security_type,
    };

    uint32_t ssid_len = strnlen(params->ssid, sizeof(params->ssid));
    uint32_t pass_len = strnlen(params->password, sizeof(params->password));

    net_params.ucSSID[0] = '\'';
    net_params.xPassword.xWPA.cPassphrase[0] = '\'';

    strncpy(&net_params.ucSSID[1], params->ssid, sizeof(net_params.ucSSID) - 1);
    strncpy(&net_params.xPassword.xWPA.cPassphrase[1],
            params->password,
            sizeof(net_params.xPassword.xWPA.cPassphrase) - 1);

    uint32_t ssid_len_1 = strnlen(net_params.ucSSID, sizeof(net_params.ucSSID) - 2);
    uint32_t pass_len_1 = strnlen(net_params.xPassword.xWPA.cPassphrase,
                                  sizeof(net_params.xPassword.xWPA.cPassphrase) - 2);

    net_params.ucSSID[ssid_len_1] = '\'';
    net_params.xPassword.xWPA.cPassphrase[pass_len_1] = '\'';

    net_params.ucSSIDLength            = ssid_len;
    net_params.xPassword.xWPA.ucLength = pass_len;

    wifi_network_params_print(&net_params);

    WIFIReturnCode_t rc = WIFI_ConnectAP(&net_params);
    if (rc != eWiFiSuccess)
    {
        LOG_ERROR("Unable to connect to WiFi network: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;
}

static int wifi_disconnect (const prov_cmd_disconnect_t * params)
{
    LOG_TRACE("\n");

    WIFIReturnCode_t rc = WIFI_Disconnect();
    if (rc != eWiFiSuccess)
    {
        LOG_ERROR("Unable to disconnect from WiFi network: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;
}

static int wifi_ping (const prov_cmd_network_info_t * params)
{
    char ip_buff[20] = {};

    LOG_TRACE("Ping %s...\n", ip_to_string(params->ping_ip, ip_buff, sizeof(ip_buff)));

    uint8_t ip_addr[4] = {0, 0, 0, 0};
    memcpy(ip_addr, &params->ping_ip, sizeof(ip_addr));

    WIFIReturnCode_t rc = WIFI_Ping(ip_addr, 3, 500);
    if (rc != eWiFiSuccess)
    {
        LOG_ERROR("Unable to ping %s: %s (%d)\n",
                  ip_to_string(params->ping_ip, ip_buff, sizeof(ip_buff)),
                  WiFiErrCode2string(rc),
                  rc);
    }

    return rc;
}

static void ble_set_bd_addr (const char * bd_addr_string)
{
    if (!bd_addr_string)
    {
        return;
    }

    size_t bd_addr_len = strlen(bd_addr_string);
    char   tmp_bd_string[bd_addr_len + 1];

    memset(tmp_bd_string, 0, bd_addr_len + 1);
    memcpy(tmp_bd_string, bd_addr_string, bd_addr_len);

    st_ble_dev_addr_t bd_addr = {.type = BLE_GAP_ADDR_PUBLIC, .addr = {0}};

    char * saveptr;
    char * p_tp = strtok_r(tmp_bd_string, ":", &saveptr);

    unsigned int p = BLE_BD_ADDR_LEN - 1;

    while ((p_tp != NULL) && (p >= 0))
    {
        bd_addr.addr[p] = (char) strtol(p_tp, NULL, 16);
        p--;

        p_tp = strtok_r(NULL, ":", &saveptr);
    }

    ble_status_t ret = R_BLE_VS_SetBdAddr(BLE_VS_ADDR_AREA_REG, &bd_addr);
    assert(BLE_SUCCESS == ret);
}

ble_status_t ble_uninit (void)
{

    /* Terminate BLE */
    return RM_BLE_ABS_Close(&g_ble_abs0_ctrl);;
}

/******************************************************************************
 * Function Name: gap_cb_user
 * Description  : Handles user defined BLE GAP conn and disconn events.
 * Arguments    : uint16_t type                - GAP event type
 *                ble_status_t result          - Event result status
 *                st_ble_evt_data_t *p_data    - Event data
 * Return Value : bool_t - TRUE if event handled, FALSE otherwise
 ******************************************************************************/
static bool gap_cb_user (uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data)
{
    bool event_handled = FALSE;

    switch (type)
    {
        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t * p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *) p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
                APP_PRINT("Device connected\n");
                APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
                R_BLE_GTL_GATTC_ReqExMtu(0, 0xc0);
            }
            else
            {
                /* Restart advertising when connection failed */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }

            event_handled = TRUE;
            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Restart advertising when disconnected */
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

            if (MFR_APP_STATE_RUNNING == g_app_state)
            {
                /* We aren't disconnecting due to closing of the service - restore the advertisement. */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }

            event_handled = TRUE;
            break;
        }

        default:
        {}
        break;
    }

    return event_handled;
}

/******************************************************************************
 * Function Name: custom_conf_ble
 * Description  : Retrieves the stored public BLE address from configuration
 *                and sets it as the device's Bluetooth address.
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void custom_conf_ble (void)
{
    char * p_bd_addr = NULL;

#ifndef RM_MAP_PERSISTANT_W
    p_bd_addr = read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
    fsp_err_t      err     = 0;
    const uint32_t cnt_max = 50;       // 5s
    uint32_t       cnt     = 0;

    /* Wait until RM_MAP_PERSISTANT_Open is called from WiFi thread */
    while (cnt++ < cnt_max)
    {
        err = RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_BLECFG,
                                              "PUBLIC_BD_ADDR",
                                              &p_bd_addr);
        if ((FSP_SUCCESS == err) || (FSP_ERR_NOT_FOUND == err))
        {
            break;
        }

        vTaskDelay(portCONVERT_MS_2_TICKS(100));
    }
    assert((FSP_SUCCESS == err || FSP_ERR_NOT_FOUND == err));
#endif

    if (NULL != p_bd_addr)
    {
        ble_set_bd_addr(p_bd_addr);
    }
}

/* End user code. Do not edit comment generated here */
