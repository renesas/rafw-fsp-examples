/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2019-2025 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/******************************************************************************
* File Name    : app_main.c
* Tool-Chain   : e2Studio
* Description  : This is a application file for peripheral role.
*******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
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
#define BLE_EVENT_PATTERN   (0x0A0A)
EventGroupHandle_t  g_ble_event_group_handle;
#endif
#include "r_ble_gapc.h"
#include "r_ble_gaps.h"
#include "r_ble_gats.h"
#include "r_ble_wifi_provisionings.h"

/******************************************************************************
 User file includes
*******************************************************************************/
/* Start user code for file includes. Do not edit comment generated here */
#include <assert.h>

#include "rm_wifi_api.h"
#include "rm_cert.h"
#include "rm_cli_w_net.h"

#include "r_ble_gtl.h"
#include "r_typedefs.h"

#include "ble_thread.h"

#include "logging.h"
#include "provisioning.h"
#include "common_utils.h"

#include "cli_ble_queue.h"

#include "ra6w1_platform_nvparam.h"
#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#else
#include "rm_vee_flash_w_rrq_nvram.h"
#endif

#include "iface_defs.h"
#include "net_ping.h"
#include "ping.h"
#include "inet.h"
/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE       (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN          (245)
#define BLE_GATTS_QUEUE_NUM                 (1)

/******************************************************************************
 User macro definitions
*******************************************************************************/
/* Start user code for macro definitions. Do not edit comment generated here */
#define BLE_FAST_ADVERTISEMENT              (1)
#define BLE_MAX_PACKET_SIZE                 (220)
#define BLE_RF_FORCE_EN                     (1)
#define BLE_OUTPUT_BUF_LEN                  (64)
#define BLE_LOG_LEVEL                       (LOG_LEVEL_DEBUG)

#define WIFI_EXT_CONNECT_METHOD             (1)
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated function prototype declarations
*******************************************************************************/
/* Internal functions */
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data);
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data);
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data);
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data);
void disc_comp_cb(uint16_t conn_hdl);
ble_status_t ble_init(void);
void app_main(void);

/******************************************************************************
 User function prototype declarations
*******************************************************************************/
/* Start user code for function prototype declarations. Do not edit comment generated here */
static int_t reboot(const prov_cmd_reboot_t* params);
static int_t factory(const prov_cmd_factory_reset_t* params);
static int_t wifi_scan(const prov_cmd_scan_t* params, prov_scanned_networks_t** networks);
static int_t wifi_ext_connect(const prov_cmd_select_ap_t* params);
static int_t wifi_connect(const prov_cmd_select_ap_t* params);
static int_t wifi_disconnect(const prov_cmd_disconnect_t* params);
static int_t wifi_ping(const prov_cmd_network_info_t* params);
static void ble_set_bd_addr(const char *bd_addr_string);
static ble_status_t ble_uninit(void);
static bool_t gap_cb_user(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data);
static int_t ble_reset(void);
void custom_conf_ble(void);
static const char *ble_address_to_string(const uint8_t *address);
static char * unescape_special_characters(int len, char * output, const char * input, size_t dst_size);
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated global variables
*******************************************************************************/
/* GAP Service UUID */
static uint8_t GAPC_UUID[] = { 0x00, 0x18 };
/* Service discovery parameters */
static st_ble_disc_entry_t gs_disc_entries[] =
{
    {
        .p_uuid     = GAPC_UUID,
        .uuid_type  = BLE_GATT_16_BIT_UUID_FORMAT,
        .serv_cb    = R_BLE_GAPC_ServDiscCb,
    },
};
/* Advertising Data */
static uint8_t gs_advertising_data[] =
{
    /* Flags */
    0x02, /**< Data Size */
    0x01, /**< Data Type */
    ( 0x06 ), /**< Data Value */

    /* Complete List of 128-bit Service Class UUIDs */
    0x11, /**< Data Size */
    0x07, /**< Data Type */
    0xc1, 0xf5, 0xdc, 0x5c, 0xb3, 0x47, 0xca, 0xa3, 0x27, 0x47, 0x4b, 0x1b, 0x01, 0xb2, 0x61, 0x91, /**< Data Value */
};

/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Shortened Local Name */
    0x0B, /**< Data Size */
    0x08, /**< Data Type */
    0x50, 0x52, 0x4f, 0x56, 0x2d, 0x52, 0x41, 0x36, 0x57, 0x32, /**< Data Value */

    /* Manufacturer Specific Data */
    0x06, /**< Data Size */
    0xff, /**< Data Type */
    0x55, 0x55, 0x55, 0x55, 0x55, /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
 .p_peer_address             = NULL,       ///< Peer address.
 .fast_advertising_interval  = 0x00000030, ///< Fast advertising interval. 30.0(ms)
 .fast_advertising_period    = 0x0BB8,     ///< Fast advertising period. 30,000(ms)
 .slow_advertising_interval  = 0x00000640, ///< Slow advertising interval. 1,000.0(ms)
 .slow_advertising_period    = 0x0000,     ///< Slow advertising period.
 .p_advertising_data         = gs_advertising_data,             ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
 .advertising_data_length    = ARRAY_SIZE(gs_advertising_data), ///< Advertising data length (in bytes).
 .p_scan_response_data       = gs_scan_response_data,             ///< Scan response data. If p_scan_response_data is specified as NULL, scan response data is not set.
 .scan_response_data_length  = ARRAY_SIZE(gs_scan_response_data), ///< Scan response data length (in bytes).
 .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY, ///< Advertising Filter Policy.
 .advertising_channel_map    = ( BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39 ), ///< Channel Map.
 .own_bluetooth_address_type = BLE_GAP_ADDR_RAND, ///< Own Bluetooth address type.
 .own_bluetooth_address      = { 0 },
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
static st_ble_gatt_queue_elm_t  gs_queue_elms[BLE_GATTS_QUEUE_ELEMENTS_SIZE];
static uint8_t gs_buffer[BLE_GATTS_QUEUE_BUFFER_LEN];
static st_ble_gatt_pre_queue_t gs_queue[BLE_GATTS_QUEUE_NUM] = {
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
 User global variables
*******************************************************************************/
/* Start user code for global variables. Do not edit comment generated here */
/* Advertisement handle */
uint8_t g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
bool g_delete_bond_pending = FALSE;

extern bool reset(void);
extern int factory_reset(int reboot_flag);

extern char_t wifi_networks[];
static char_t output_buf[BLE_OUTPUT_BUF_LEN];

typedef enum e_mfr_app_state
{
    MFR_APP_STATE_STOPPED,
    MFR_APP_STATE_STOPPING,
    MFR_APP_STATE_RUNNING,
} mfr_app_state_t;

mfr_app_state_t g_app_state = MFR_APP_STATE_RUNNING; /* Application starts with running state */

typedef enum {
    eWiFiSecurityOpen_app = 0,          // Open - No Security
    eWiFiSecurityWEP_app,               // WEP
    eWiFiSecurityWPA_app,               // WPA
    eWiFiSecurityWPA2_app,              // WPA2 (RSN)
    eWiFiSecurityWPA_AUTO_app,          // WPA & WPA2 (RSN)
    eWiFiSecurityOWE_app,               // WPA3 OWE
    eWiFiSecuritySAE_app,               // WPA3 SAE
    eWiFiSecurityRSN_SAE_app,           // WPA2 (RSN) & WPA3 SAE
    eWiFiSecurityWPA_EAP_app,           // WPA Enterprise
    eWiFiSecurityWPA2_EAP_app,          // WPA2 Enterprise
    eWiFiSecurityWPA_AUTO_EAP_app,      // WPA & WPA2 Enterprise
    eWiFiSecurityWPA3_EAP_app,          // WPA3 Enterprise
    eWiFiSecurityWPA2_AUTO_EAP_app,     // WPA2 & WPA3 Enterprise
    eWiFiSecurityWPA3_EAP_192B_app,     // WPA3 192B Enterprise DA16200 not support
    eWiFiSecurityNotSupported_app       // Unknown Security
} WIFISecurity_app_t;

typedef struct {
  uint16_t remain;
  uint16_t total;
} prov_header_t;

static bool_t flag_start = FALSE;
static prov_header_t ble_prov_hdr;

static const char* msg2string(uint16_t msg_type)
{
/* Converts msg type value into its string name */
#define MSG_2_STRING(msg) case (msg): return #msg;

    switch (msg_type) {
        /* WiFiCommand */
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_WRITE_REQ                        )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_WRITE_COMP                       )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ  )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_READ_REQ   )

        /* WiFiStatus */
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_READ_REQ                          )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_WRITE_REQ                )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_WRITE_COMP               )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_READ_REQ                 )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_WRITE_REQ   )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_WRITE_COMP  )
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_READ_REQ    )

        /* WiFiOutput */
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_OUTPUT_READ_REQ                          )

        /* WiFiProvisoning */
        MSG_2_STRING( BLE_WIFI_PROVISIONINGS_EVENT_WIFI_PROVISIONING_READ_REQ                     )

        default: return "Unknown";
    }
#undef MSG_2_STRING
}

static const char* wifi_err_code_2_string(WIFIReturnCode_t rc)
{
/* Converts an error code enum value into its string name */
#define ERRCODE_2_STRING(err_code) case (err_code): return #err_code;

    switch (rc) {
        ERRCODE_2_STRING( eWiFiSuccess      )
        ERRCODE_2_STRING( eWiFiFailure      )
        ERRCODE_2_STRING( eWiFiTimeout      )
        ERRCODE_2_STRING( eWiFiNotSupported )

        default: return "Unknown";
    }
#undef ERRCODE_2_STRING
}

static const char* rm_cert_err_code_2_string(rm_cert_err_t rc)
{
/* Converts an error code enum value into its string name */
#define ERRCODE_2_STRING(err_code) case (err_code): return #err_code;

    switch (rc) {
        ERRCODE_2_STRING( RM_CERT_ERR_OK                 )
        ERRCODE_2_STRING( RM_CERT_ERR_NOK                )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_MODULE     )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_TYPE       )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_FORMAT     )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_LENGTH     )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_FLASH_ADDR )
        ERRCODE_2_STRING( RM_CERT_ERR_INVALID_PARAMS     )
        ERRCODE_2_STRING( RM_CERT_ERR_FOPEN_FAILED       )
        ERRCODE_2_STRING( RM_CERT_ERR_MEM_FAILED         )
        ERRCODE_2_STRING( RM_CERT_ERR_EMPTY_CERTIFICATE  )

        default: return "Unknown";
    }
#undef ERRCODE_2_STRING
}

static const char* wifi_sec_type_2_string(WIFISecurity_app_t sec_type)
{
/* Converts sec type enum value into its string name */
#define SECTYPE_2_STRING(type) case (type): return #type;

    switch (sec_type) {
        SECTYPE_2_STRING( eWiFiSecurityOpen_app         )
        SECTYPE_2_STRING( eWiFiSecurityWEP_app          )
        SECTYPE_2_STRING( eWiFiSecurityWPA_app          )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_app         )
        SECTYPE_2_STRING( eWiFiSecurityWPA_AUTO_app     )
        SECTYPE_2_STRING( eWiFiSecurityOWE_app          )
        SECTYPE_2_STRING( eWiFiSecuritySAE_app          )
        SECTYPE_2_STRING( eWiFiSecurityRSN_SAE_app      )
        SECTYPE_2_STRING( eWiFiSecurityWPA_EAP_app      )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_EAP_app     )
        SECTYPE_2_STRING( eWiFiSecurityWPA_AUTO_EAP_app )
        SECTYPE_2_STRING( eWiFiSecurityWPA3_EAP_app     )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_AUTO_EAP_app)
        SECTYPE_2_STRING( eWiFiSecurityWPA3_EAP_192B_app)
        SECTYPE_2_STRING( eWiFiSecurityNotSupported_app )

        default: return "Unknown";
    }
#undef SECTYPE_2_STRING
}

static const char* wifi_sec_ext_type_2_string(WIFISecurityExt_t sec_type)
{
/* Converts sec type enum value into its string name */
#define SECTYPE_2_STRING(type) case (type): return #type;

    switch (sec_type) {
        SECTYPE_2_STRING( eWiFiSecurityOpen_ext          )
        SECTYPE_2_STRING( eWiFiSecurityWEP_ext           )
        SECTYPE_2_STRING( eWiFiSecurityWPA_ext           )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_ext          )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_ent_ext      )
        SECTYPE_2_STRING( eWiFiSecurityWPA3_ext          )
        SECTYPE_2_STRING( eWiFiSecurityNotSupported_ext  )
        SECTYPE_2_STRING( eWiFiSecurityWPA_ent_ext       )
        SECTYPE_2_STRING( eWiFiSecurityWPA_WPA2_ent_ext  )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_WPA3_ent_ext )
        SECTYPE_2_STRING( eWiFiSecurityWPA3_ent_ext      )
        SECTYPE_2_STRING( eWiFiSecurityWPA3_192B_ent_ext )
        SECTYPE_2_STRING( eWiFiSecurityWPA_WPA2_ext      )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_WPA3_ext     )
        SECTYPE_2_STRING( eWiFiSecurityMax_ext           )

        default: return "Unknown";
    }
#undef SECTYPE_2_STRING
}

static const char* wifi_band_type_2_string(WIFIBand_t band_type)
{
/* Converts band type enum value into its string name */
#define BANDTYPE_2_STRING(type) case (type): return #type;

    switch (band_type) {
        BANDTYPE_2_STRING( eWiFiBand2G   )
        BANDTYPE_2_STRING( eWiFiBand5G   )
        BANDTYPE_2_STRING( eWiFiBandDual )

        default: return "Unknown";
    }
#undef BANDTYPE_2_STRING
}

static const char* wifi_ent_auth_ext_type_2_string(WIFIEntAuthTypeExt_t auth_type)
{
/* Converts an authentication type enum value into its string name */
#define AUTHTYPE_2_STRING(type) case (type): return #type;

    switch (auth_type) {
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_NONE           )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_PEAP_TTLS_FAST )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_PEAP           )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_TTLS           )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_FAST           )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_TLS            )
        AUTHTYPE_2_STRING( ENT_AUTH_TYPE_UNSUPPORTED    )

        default: return "Unknown";
    }
#undef AUTHTYPE_2_STRING
}

static const char* wifi_ent_auth_proto_ext_type_2_string(WIFIEntAuthProtoExt_t proto_type)
{
/* Converts proto type enum value into its string name */
#define PROTOTYPE_2_STRING(type) case (type): return #type;

    switch (proto_type) {
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_NONE         )
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_MSCHAPv2_GTC )
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_MSCHAPv2     )
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_GTC          )
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_TLS          )
        PROTOTYPE_2_STRING( ENT_AUTH_PROTO_UNSUPPORTED  )

        default: return "Unknown";
    }
#undef PROTOTYPE_2_STRING
}

static const char* wifi_phy_mode_ext_type_2_string(e_wifi_phy_mode_ext_t phy_type)
{
/* Converts phy type enum value into its string name */
#define PHYTYPE_2_STRING(type) case (type): return #type;

    switch (phy_type) {
        PHYTYPE_2_STRING( WIFI_MODE_AUTO_STA  )
        PHYTYPE_2_STRING( WIFI_MODE_GN        )
        PHYTYPE_2_STRING( WIFI_MODE_BG        )
        PHYTYPE_2_STRING( WIFI_MODE_N_ONLY    )
        PHYTYPE_2_STRING( WIFI_MODE_G_ONLY    )
        PHYTYPE_2_STRING( WIFI_MODE_B_ONLY    )

        PHYTYPE_2_STRING( WIFI_MODE_AN        )
        PHYTYPE_2_STRING( WIFI_MODE_A_ONLY    )
        PHYTYPE_2_STRING( WIFI_MODE_N_ONLY_5G )

        default: return "Unknown";
    }
#undef PHYTYPE_2_STRING
}
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated function definitions
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
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP callback function common process. Do not edit comment generated here */
    if (gap_cb_user(type, result, p_data))
    {
        /* event handled by own application */
        return;
    }
/* End user code. Do not edit comment generated here */

    switch(type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            /* Get BD address for Advertising */
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);
        } break;

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t *p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *)p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
            }
            else
            {
                /* Restart advertising when connection failed */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }
        } break;

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Restart advertising when disconnected */
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
        } break;

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            /* Send connection update response with value received on connection update request */
            st_ble_gap_conn_upd_req_evt_t *p_conn_upd_req_evt_param = (st_ble_gap_conn_upd_req_evt_t *)p_data->p_param;

            st_ble_gap_conn_param_t conn_updt_param = {
                .conn_intv_min = p_conn_upd_req_evt_param->conn_intv_min,
                .conn_intv_max = p_conn_upd_req_evt_param->conn_intv_max,
                .conn_latency  = p_conn_upd_req_evt_param->conn_latency,
                .sup_to        = p_conn_upd_req_evt_param->sup_to,
            };

            R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
                              BLE_GAP_CONN_UPD_MODE_RSP,
                              BLE_GAP_CONN_UPD_ACCEPT,
                              &conn_updt_param);
        } break;

/* Hint: Add cases of GAP event macros defined as BLE_GAP_XXX */
/* Start user code for GAP callback function event process. Do not edit comment generated here */
        case BLE_GAP_EVENT_ADV_ON:
        {
            st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;

            APP_PRINT_INFO("Legacy advertising started\n");
        } break;

        case BLE_GAP_EVENT_ADV_OFF:
        {
            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

            APP_PRINT_INFO("Legacy advertising stopped\n");
        } break;

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            st_ble_gap_pairing_req_evt_t * p_pair_req_param = (st_ble_gap_pairing_req_evt_t *) p_data->p_param;
            R_BLE_GAP_ReplyPairing(p_pair_req_param->conn_hdl, BLE_GAP_PAIRING_ACCEPT);
        } break;

        case BLE_GAP_EVENT_NUM_COMP_REQ:
        {
            st_ble_gap_num_comp_evt_t * p_numcmp_evt_param = (st_ble_gap_num_comp_evt_t *)p_data->p_param;

            APP_PRINT_INFO("Numeric Comparison: %06d \r\n", p_numcmp_evt_param->numeric);
            /* DO THE COMPARISON, APPLICATION JOB */
            R_BLE_GAP_ReplyNumComp(p_numcmp_evt_param->conn_hdl, BLE_GAP_PAIRING_ACCEPT);
        } break;

        default: break;
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
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_GattsCb(type, result, p_data);
    switch(type)
    {
/* Hint: Add cases of GATT Server event macros defined as BLE_GATTS_XXX */
/* Start user code for GATT Server callback function event process. Do not edit comment generated here */
        default: break;
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
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVC_GattcCb(type, result, p_data);
    switch(type)
    {
        case BLE_GATTC_EVENT_CONN_IND:
        {
            /* Start discovery operation after connection established */
            R_BLE_DISC_Start(p_data->conn_hdl, gs_disc_entries, ARRAY_SIZE(gs_disc_entries), disc_comp_cb);
        } break;

/* Hint: Add cases of GATT Client event macros defined as BLE_GATTC_XXX */
/* Start user code for GATT Client callback function event process. Do not edit comment generated here */
        case BLE_GATTC_EVENT_EX_MTU_RSP:
        {
          st_ble_gattc_ex_mtu_rsp_evt_t * p_ex_mtu_rsp;
          p_ex_mtu_rsp = (st_ble_gattc_ex_mtu_rsp_evt_t *)p_data->p_param;

          APP_PRINT_INFO("--> BLE_GATTC_EVENT_EX_MTU_RSP: %d\n", p_ex_mtu_rsp->mtu);
        } break;

        default: break;
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
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for vender specific callback function common process. Do not edit comment generated here */
    if (BLE_VS_EVENT_GET_ADDR_COMP == type)
    {
        st_ble_vs_get_bd_addr_comp_evt_t * p_get_address = (st_ble_vs_get_bd_addr_comp_evt_t *)p_data->p_param;
        AUTOTEST_PRINT("BD address from device: %s\n", ble_address_to_string(p_get_address->addr.addr));
    }
/* End user code. Do not edit comment generated here */
    
    R_BLE_SERVS_VsCb(type, result, p_data);
    switch(type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *)p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
        } break;

/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
        default: break;
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
static void gaps_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);
    FSP_PARAMETER_NOT_USED(*p_data);
/* End user code. Do not edit comment generated here */

    switch(type)
    {
/* Hint: Add cases of GAP Service server events defined in e_ble_gaps_event_t */
/* Start user code for GAP Service Server callback function event process. Do not edit comment generated here */
        default: break;
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
static void gats_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);
    FSP_PARAMETER_NOT_USED(*p_data);
/* End user code. Do not edit comment generated here */

    switch(type)
    {
/* Hint: Add cases of GATT Service server events defined in e_ble_gats_event_t */
/* Start user code for GATT Service Server callback function event process. Do not edit comment generated here */
        default: break;
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
static void wifi_provisionings_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for wifi_provisioning Server callback function common process. Do not edit comment generated here */
    LOG_INFO("Type: 0x%x, Msg: %s\n", type, msg2string(type));

    if (BLE_SUCCESS == result)
    {
/* End user code. Do not edit comment generated here */

    switch(type)
    {
/* Hint: Add cases of wifi_provisioning server events defined in e_ble_wifi_provisionings_event_t */
/* Start user code for wifi_provisioning Server callback function event process. Do not edit comment generated here */
        case BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_WRITE_REQ:
        {
            st_ble_wifi_provisionings_wifi_command_t *p_event_data =
              (st_ble_wifi_provisionings_wifi_command_t *) p_data->p_param;

            provisioning_cmd_dispatch(p_event_data->value);
        } break;

        case BLE_WIFI_PROVISIONINGS_EVENT_WIFI_OUTPUT_READ_REQ:
        {
            st_ble_wifi_provisionings_wifi_output_t *p_event_data =
              (st_ble_wifi_provisionings_wifi_output_t *) p_data->p_param;

            memset(output_buf, 0, sizeof(output_buf));

            if (FALSE == flag_start)
            {
                uint32_t total = strlen(wifi_networks);

                ble_prov_hdr.remain = total;
                ble_prov_hdr.total  = total;

                flag_start = TRUE;

                LOG_INFO("To be sent: %s\n\n", wifi_networks);
            }

            uint32_t to_be_sent = MIN(
                MIN(ble_prov_hdr.remain, ble_prov_hdr.total),
                MIN(sizeof(output_buf) - sizeof(ble_prov_hdr), BLE_MAX_PACKET_SIZE)
            );

            uint32_t offset = ble_prov_hdr.total - ble_prov_hdr.remain;
            ble_prov_hdr.remain = ble_prov_hdr.remain - to_be_sent;

            strncpy(output_buf + sizeof(ble_prov_hdr), wifi_networks + offset, to_be_sent);
            memcpy(output_buf, &ble_prov_hdr, sizeof(ble_prov_hdr));

            LOG_INFO("Sent(%d:%d:%d:%d): %.*s\n", to_be_sent, offset,
                ble_prov_hdr.remain, ble_prov_hdr.total, to_be_sent, output_buf + sizeof(ble_prov_hdr));

            st_ble_gatt_value_t value =
            {
              .value_len = to_be_sent + sizeof(ble_prov_hdr),
              .p_value = output_buf,
            };

            uint16_t attr_hdl = BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_VAL_HDL;
            uint16_t rc = R_BLE_GTL_GATTS_ReadCfm(p_data->conn_hdl, attr_hdl, &value);
            LOG_INFO("ReadCfm RC: 0x%x\n", rc);

            if (0 == ble_prov_hdr.remain)
            {
                /* Disconnect / timeout, the flag should be cleared */
                flag_start = FALSE;
            }

        } break;

        case BLE_WIFI_PROVISIONINGS_EVENT_WIFI_PROVISIONING_READ_REQ:
        {
            st_ble_wifi_provisionings_wifi_provisioning_t *p_event_data =
              (st_ble_wifi_provisionings_wifi_provisioning_t *) p_data->p_param;

        } break;

        default: break;
        }
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
static void gapc_cb(uint16_t type, ble_status_t result, st_ble_servc_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Client callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);
    FSP_PARAMETER_NOT_USED(*p_data);
/* End user code. Do not edit comment generated here */

    switch(type)
    {
/* Hint: Add cases of GAP Service client events defined in e_ble_gapc_event_t */
/* Start user code for GAP Service Client callback function event process. Do not edit comment generated here */
        default: break;
/* End user code. Do not edit comment generated here */
    }
}
/******************************************************************************
 * Function Name: disc_comp_cb
 * Description  : Callback function for Service Discovery.
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void disc_comp_cb(uint16_t conn_hdl)
{
/* Hint: Input process such as GATT operation */
/* Start user code for Discovery Complete callback function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(conn_hdl);
/* End user code. Do not edit comment generated here */
    return;
}
/******************************************************************************
 * Function Name: ble_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : none
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
ble_status_t ble_init(void)
{
    ble_status_t status;
    fsp_err_t err;

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
void app_main(void)
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
      .reboot = reboot,
      .factory = factory,
      .wifi_scan = wifi_scan,
      .wifi_ext_connect = WIFI_EXT_CONNECT_METHOD ? wifi_ext_connect : wifi_connect,
      .wifi_disconnect = wifi_disconnect,
      .wifi_ping = wifi_ping,
    };

    provisioning_init(&prov_params);
    char_t *p_bd_addr = NULL;

    APP_PRINT_INFO("Please run 'provisioning.py' script\n");

    cli_ble_queue_init();
/* End user code. Do not edit comment generated here */

    /* main loop */
    while (1)
    {
        /* Process BLE Event */
        R_BLE_Execute();

/* When this BLE application works on the FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
        if(0 != R_BLE_IsTaskFree())
        {
            /* If the BLE Task has no operation to be processed, it transits block state until the event from RF transciever occurs. */
            xEventGroupWaitBits(g_ble_event_group_handle,
                                (EventBits_t)BLE_EVENT_PATTERN,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);
        }
#endif

/* Hint: Input process that should be done during main loop such as calling processing functions */
/* Start user code for process during main loop. Do not edit comment generated here */
        cli_ble_msg_t msg;

        if (cli_ble_queue_dequeue(&msg))
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
                    p_bd_addr = (uint8_t *)read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, "PUBLIC_BD_ADDR", (char *)&p_bd_addr);
#endif
                    if (p_bd_addr)
                    {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    g_app_state = MFR_APP_STATE_RUNNING;
                } break;

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
                } break;

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
                } break;

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
                } break;

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
                } break;

                case CLI_BLE_MSG_TYPE_BD_ADDR_GET:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    ret = R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_PUBLIC);
                    assert (BLE_SUCCESS == ret);

                } break;

                default:
                {
                    printf("Received CLI msg of type (%d) which was not handled.\n", msg.type);
                }
            }
        }

        if (MFR_APP_STATE_STOPPING == g_app_state &&
            BLE_GAP_INVALID_CONN_HDL == g_conn_hdl &&
            BLE_GAP_INVALID_ADV_HDL == g_adv_hdl)
        {
            ret = ble_uninit();
            assert(BLE_SUCCESS == ret);

            g_app_state = MFR_APP_STATE_STOPPED;
        }
        while(MFR_APP_STATE_STOPPED == g_app_state)
        {
            if (cli_ble_queue_dequeue(&msg) &&
                CLI_BLE_MSG_TYPE_SVC_START == msg.type)
            {
#ifndef RM_MAP_PERSISTANT_W
                p_bd_addr = (uint8_t *)read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                ENV_GROUP_BLECFG,
                                                "PUBLIC_BD_ADDR",
                                                (char *)&p_bd_addr);
#endif
                if (p_bd_addr)
                {
                    ble_set_bd_addr(p_bd_addr);
                }

                ret = ble_init();
                assert(BLE_SUCCESS == ret);

                g_app_state = MFR_APP_STATE_RUNNING;
            }
            else
            {
                vTaskDelay(10);
            }
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
 User function definitions
*******************************************************************************/
/* Start user code for function definitions. Do not edit comment generated here */

/******************************************************************************
 * Function Name: reboot
 * Description  : Handles the provisioning reboot command by invoking reset.
 * Arguments    : const prov_cmd_reboot_t *params -
 *                  Pointer to reboot command parameters (unused).
 * Return Value : int_t -
 *                  Result of the reset operation.
 ******************************************************************************/
static int_t reboot(const prov_cmd_reboot_t* params)
{
    FSP_PARAMETER_NOT_USED(*params);
    return reset();
}

/******************************************************************************
 * Function Name: ble_reset
 * Description  : Deletes all stored BLE bond information and then performs a
 *                factory reset to restore the device state to defaults.
 * Arguments    : None
 * Return Value : int_t -
 *                  Result of the factory reset operation.
 ******************************************************************************/
static int_t ble_reset(void)
{
    /* Clear BLE bond keys */
    ble_abs_bond_information_parameter_t bond_param;
    memset(&bond_param, 0, sizeof(bond_param));

    bond_param.local_bond_information  = BLE_ABS_LOCAL_BOND_INFORMATION_ALL;
    bond_param.remote_bond_information = BLE_ABS_REMOTE_BOND_INFORMATION_ALL;
    bond_param.delete_non_volatile_area = BLE_ABS_DELETE_NON_VOLATILE_AREA_ENABLE;
    bond_param.p_address = NULL;
    bond_param.abs_delete_bond_callback = NULL;

    fsp_err_t ble_status = RM_BLE_ABS_DeleteBondInformation(&g_ble_abs0_ctrl, &bond_param);
    if (FSP_SUCCESS != ble_status)
    {
        APP_PRINT_INFO("Error BLE Bond Delete(%d)\n", ble_status);
    } else
    {
        APP_PRINT_INFO("BLE bond keys cleared\r\n");
    }
    return factory_reset(TRUE);
}

/******************************************************************************
 * Function Name: factory
 * Description  : Handles BLE reset by disconnecting active BLE connections
 * Arguments    : const prov_cmd_factory_reset_t *params -
 *                  Pointer to factory reset command parameters (not used).
 * Return Value : int_t -
 *                   Result of the BLE reset operation if executed,
 *                   otherwise TRUE when disconnect is initiated.
 ******************************************************************************/
static int_t factory(const prov_cmd_factory_reset_t* params)
{
    FSP_PARAMETER_NOT_USED(*params);
    int_t ret = TRUE;
    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
    {
        g_delete_bond_pending = TRUE;
        R_BLE_GAP_Disconnect(g_conn_hdl, 0x13);
    }
    else
    {
        ret = ble_reset();
    }

    return ret;
}

/******************************************************************************
 * Function Name: is_wifi_network_hidden
 * Description  : Determines whether the given WiFi network should be treated
 *                as hidden. A network is considered hidden when the SSID length
 *                is one byte and the single character is a tab ('\t').
 * Arguments    : const char *ssid -
 *                  Pointer to the SSID buffer.
 *                unsigned ssid_len -
 *                  Length of the SSID in bytes.
 * Return Value : bool_t -
 *                  TRUE  : SSID matches the hidden network pattern.
 *                  FALSE : SSID does not match the hidden network pattern.
 ******************************************************************************/
static bool_t is_wifi_network_hidden(const char* ssid, unsigned ssid_len)
{
    if ((1 == ssid_len) && ('\t' == ssid[0]))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
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
static WIFISecurity_app_t wifi_security_type_reduce(WIFISecurityExt_t sec_type)
{
    switch (sec_type)
    {
        case eWiFiSecurityOpen_ext:
            return eWiFiSecurityOpen_app;

        case eWiFiSecurityWEP_ext:
            return eWiFiSecurityWEP_app;

        case eWiFiSecurityWPA_ext:
            return eWiFiSecurityWPA_app;

        case eWiFiSecurityWPA2_ext:
            return eWiFiSecurityWPA2_app;

        case eWiFiSecurityWPA_WPA2_ext:
            return eWiFiSecurityWPA_AUTO_app;

        case eWiFiSecurityWPA2_WPA3_ext:
            return eWiFiSecurityRSN_SAE_app;

        case eWiFiSecurityWPA2_ent_ext:
            return eWiFiSecurityWPA2_EAP_app;

        case eWiFiSecurityWPA_WPA2_ent_ext:
            return eWiFiSecurityWPA_AUTO_EAP_app;

        case eWiFiSecurityWPA2_WPA3_ent_ext:
            return eWiFiSecurityWPA2_AUTO_EAP_app;

        case eWiFiSecurityWPA3_ext:
            return eWiFiSecuritySAE_app;

        case eWiFiSecurityWPA3_OWE_ext:
            return eWiFiSecurityOWE_app;

        case eWiFiSecurityWPA_ent_ext:
            return eWiFiSecurityWPA_EAP_app;

        case eWiFiSecurityWPA3_ent_ext:
            return eWiFiSecurityWPA3_EAP_app;

        case eWiFiSecurityWPA3_192B_ent_ext:
            return eWiFiSecurityWPA3_EAP_192B_app;

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
static WIFISecurityExt_t wifi_security_type_ext(WIFISecurity_app_t sec_type)
{
    switch (sec_type)
    {
        case eWiFiSecurityOpen_app:
            return eWiFiSecurityOpen_ext;

        case eWiFiSecurityWEP_app:
            return eWiFiSecurityWEP_ext;

        case eWiFiSecurityWPA_app:
            return eWiFiSecurityWPA_ext;

        case eWiFiSecurityWPA2_app:
            return eWiFiSecurityWPA2_ext;

        case eWiFiSecurityWPA_AUTO_app:
            return eWiFiSecurityWPA_WPA2_ext;

        case eWiFiSecurityRSN_SAE_app:
            return eWiFiSecurityWPA2_WPA3_ext;

        case eWiFiSecurityWPA2_EAP_app:
            return eWiFiSecurityWPA2_ent_ext;

        case eWiFiSecurityWPA_AUTO_EAP_app:
            return eWiFiSecurityWPA_WPA2_ent_ext;

        case eWiFiSecurityWPA2_AUTO_EAP_app:
            return eWiFiSecurityWPA2_WPA3_ent_ext;

        case eWiFiSecuritySAE_app:
            return eWiFiSecurityWPA3_ext;

        case eWiFiSecurityOWE_app:
            return eWiFiSecurityWPA3_OWE_ext;

        case eWiFiSecurityWPA_EAP_app:
            return eWiFiSecurityWPA_ent_ext;

        case eWiFiSecurityWPA3_EAP_app:
            return eWiFiSecurityWPA3_ent_ext;

        case eWiFiSecurityWPA3_EAP_192B_app:
            return eWiFiSecurityWPA3_192B_ent_ext;

        case eWiFiSecurityNotSupported_app:
        default:
            return eWiFiSecurityNotSupported_ext;
    }
}


/******************************************************************************
 * Function Name: wifi_scan
 * Description  : Scans for available WiFi networks and returns the filtered list.
 * Arguments    : const prov_cmd_scan_t *params - Scan command parameters (unused)
 *                prov_scanned_networks_t **networks - Output list of networks
 * Return Value : int_t - WiFi scan result code
 ******************************************************************************/
static int_t wifi_scan(const prov_cmd_scan_t* params, prov_scanned_networks_t** networks)
{
    FSP_PARAMETER_NOT_USED(*params);
#define MAX_WIFI_SCAN_RESULTS 20

    LOG_INFO("\n");
    uint32_t i;
    uint32_t j;
    static WIFIScanResult_t scan_data[MAX_WIFI_SCAN_RESULTS];
    memset(scan_data, 0, sizeof(scan_data));

    WIFIReturnCode_t rc = WIFI_Scan(&scan_data[0], MAX_WIFI_SCAN_RESULTS);
    if (eWiFiSuccess == rc )
    {
        uint32_t num_of_found_networks = 0;
        for (i = 0; i < MAX_WIFI_SCAN_RESULTS; i++)
        {
            if (0 != scan_data[i].ucSSIDLength)
            {
                num_of_found_networks++;
            }
            else
            {
                break;
            }
        }

        prov_scanned_networks_t* p_networks_db = malloc(
            sizeof(prov_scanned_networks_t) + (num_of_found_networks * sizeof(prov_ap_entry_t)));
        assert(p_networks_db);

        p_networks_db->num_of_networks = 0;

        APP_PRINT_INFO("Found %d networks:\n", num_of_found_networks);
        j = 0;
        for (i = 0; i < num_of_found_networks; i++)
        {
            WIFISecurity_app_t security = wifi_security_type_reduce(scan_data[i].xSecurity);
            if ((eWiFiSecurityNotSupported_app == security) ||
                (is_wifi_network_hidden(scan_data[i].ucSSID, scan_data[i].ucSSIDLength)))
            {
                LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %30s -> %s [SKIP]\n",
                    scan_data[i].ucSSIDLength, scan_data[i].ucSSID, scan_data[i].cRSSI,
                    scan_data[i].ucChannel, wifi_sec_ext_type_2_string(scan_data[i].xSecurity),
                    wifi_sec_type_2_string(security));
                continue;
            }

            prov_ap_entry_t entry =
            {
                .rssi          = scan_data[i].cRSSI,
                .channel       = scan_data[i].ucChannel,
                .security_type = security,
            };

            strncpy(entry.ssid, scan_data[i].ucSSID,
                MAX(sizeof(entry.ssid) - 1, scan_data[i].ucSSIDLength));

            p_networks_db->networks[j++] = entry;
            p_networks_db->num_of_networks++;

            LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %30s -> %s\n",
                scan_data[i].ucSSIDLength, scan_data[i].ucSSID, scan_data[i].cRSSI,
                scan_data[i].ucChannel, wifi_sec_ext_type_2_string(scan_data[i].xSecurity),
                wifi_sec_type_2_string(security));
        }
        APP_PRINT_INFO("----------------------\n\n");
        *networks = p_networks_db;
    }
    else
    {
        APP_PRINT_ERR("Unable to scan WiFi networks: %s (%d)\n", wifi_err_code_2_string(rc), rc);
    }

    return rc;

#undef MAX_WIFI_SCAN_RESULTS
}

/******************************************************************************
 * Function Name: wifi_network_params_print
 * Description  : Prints the WiFi network parameters for debugging.
 * Arguments    : const WIFINetworkParams_t *params - WiFi network parameters
 * Return Value : none
 ******************************************************************************/
static void wifi_network_params_print(const WIFINetworkParams_t* params)
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
             "}\n", sizeof(params->ucSSID), params->ucSSID, params->ucSSIDLength,
                    sizeof(params->xPassword.xWPA.cPassphrase),
                    params->xPassword.xWPA.cPassphrase, params->xPassword.xWPA.ucLength,
                    wifi_sec_type_2_string(security), params->ucDefaultWEPKeyIndex, params->ucChannel
    );
}

/******************************************************************************
 * Function Name: wifi_enterprise_net_params_print
 * Description  : Prints enterprise WiFi network parameters for debugging.
 * Arguments    : const WIFIEnterpriseNetParams_t *params -
 *                  Enterprise network parameters
 * Return Value : none
 ******************************************************************************/
static void wifi_enterprise_net_params_print(const WIFIEnterpriseNetParams_t* params)
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
             "}\n", wifi_ent_auth_ext_type_2_string(params->ucEntAuthType),
                    wifi_ent_auth_proto_ext_type_2_string(params->ucEntAuthProto),
                    sizeof(params->ucID), params->ucID, params->ucIDLength,
                    sizeof(params->ucPassword), params->ucPassword, params->ucPasswordLength,
                    sizeof(params->ucCert), params->ucCert, params->ucCertLength,
                    sizeof(params->ucPrivKey), params->ucPrivKey, params->ucPrivKeyLength
    );
}

/******************************************************************************
 * Function Name: wifi_network_ext_params_print
 * Description  : Prints extended WiFi network parameters for debugging.
 * Arguments    : const WIFINetworkParamsExt_t *params -
 *                  Extended network parameters
 * Return Value : none
 ******************************************************************************/
static void wifi_network_ext_params_print(const WIFINetworkParamsExt_t* params)
{
    LOG_INFO("WIFINetworkParamsExt_t: {\n"
             "  - ucWiFi_mode:      %s\n"
             "  - ucBand:           %s\n"
             "  - xNetworkParams:   ***\n"
             "  - xEntNetParams:    ***\n"
             "}\n", wifi_phy_mode_ext_type_2_string(params->ucWiFi_mode), wifi_band_type_2_string(params->ucBand)
    );

    wifi_network_params_print(&params->xNetworkParams);
    wifi_enterprise_net_params_print(&params->xEntNetParams);
}

/******************************************************************************
 * Function Name: wifi_enterprise_cert_key_set
 * Description  : Loads the enterprise WiFi certificate and private key into
 *                the network parameters structure.
 * Arguments    : WIFINetworkParamsExt_t *net_params - Output network parameters
 * Return Value : bool_t - TRUE on success, FALSE on failure
 ******************************************************************************/
static bool_t wifi_enterprise_cert_key_set(WIFINetworkParamsExt_t *net_params)
{
    LOG_INFO("\n");

    rm_cert_format_t format = RM_CERT_FORMAT_PEM;
    size_t outlen = CERT_MAX_LENGTH;

    memset(net_params->xEntNetParams.ucCert, 0xff, sizeof(net_params->xEntNetParams.ucCert));
    memset(net_params->xEntNetParams.ucPrivKey, 0xff, sizeof(net_params->xEntNetParams.ucPrivKey));

    rm_cert_err_t rc = RM_CERT_Read(
                           RM_CERT_GetModule(SF_TLS_CERT_WPA_ENT_CERTIFICATE_ADDR),
                           RM_CERT_GetType(SF_TLS_CERT_WPA_ENT_CERTIFICATE_ADDR),
                           &format, (unsigned char*) net_params->xEntNetParams.ucCert, &outlen
    );

    net_params->xEntNetParams.ucCertLength = outlen;
    if ((0xff == net_params->xEntNetParams.ucCert[0]) || (RM_CERT_ERR_OK != rc))
    {
        APP_PRINT_ERR("Unable to get a certificate: %s (%d)\n", rm_cert_err_code_2_string(rc), rc);
        return FALSE;
    }

    rc = RM_CERT_Read(
             RM_CERT_GetModule(SF_TLS_CERT_WPA_ENT_PRIVATE_KEY_ADDR),
             RM_CERT_GetType(SF_TLS_CERT_WPA_ENT_PRIVATE_KEY_ADDR),
             &format, (unsigned char *)net_params->xEntNetParams.ucPrivKey, &outlen
    );

    net_params->xEntNetParams.ucPrivKeyLength = outlen;
    if ((0xff == net_params->xEntNetParams.ucPrivKey[0]) || (RM_CERT_ERR_OK != rc))
    {
        APP_PRINT_ERR("Unable to get a private key: %s (%d)\n", rm_cert_err_code_2_string(rc), rc);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * Function Name: store_provisioning_params
 * Description  : Stores WiFi provisioning parameters into persistent storage.
 * Arguments    : const WIFINetworkParams_t *net_params - WiFi network settings
 * Return Value : fsp_err_t - FSP_SUCCESS on success or error code on failure
 ******************************************************************************/
static fsp_err_t store_provisioning_params(const WIFINetworkParams_t *net_params,uint32_t is_hidden)
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
            RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_WEPKEY0_0, net_params->xPassword.xWEP[0].cKey);
        }
    else
    RM_MAP_PERSISTANT_W_Write_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, net_params->xPassword.xWPA.cPassphrase);
#if CFG_PMGR
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENABLE_DPM, 1);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME, DFLT_DPM_KEEPALIVE_TIME);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DPM_USER_WAKEUP_TIME, MIN_DPM_USER_WAKEUP_TIME);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT, DFLT_DPM_TIM_WAKEUP_COUNT);
#endif /* CFG_PMGR */

    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);

    return FSP_SUCCESS;
}

/******************************************************************************
 * Function Name: wifi_ext_connect
 * Description  : Connects to a WiFi network using extended parameters and
 *                stores provisioning data on success.
 * Arguments    : const prov_cmd_select_ap_t *params - Selected AP parameters
 * Return Value : int_t - WiFi connection result code
 ******************************************************************************/
static int_t wifi_ext_connect(const prov_cmd_select_ap_t* params)
{
    APP_PRINT_INFO("\n");
    char unescaped_ssid[PROV_SSID_LEN + 1];
    uint32_t ssid_len = strnlen(params->ssid, sizeof(params->ssid));
    uint32_t pass_len = strnlen(params->password, sizeof(params->password));
    WIFISecurity_app_t security_app = (WIFISecurity_t) params->security_type;
    WIFISecurityExt_t security = wifi_security_type_ext(security_app);
    WIFINetworkParamsExt_t net_ext_params =
    {
        .xNetworkParams.ucChannel = 4,
        .xNetworkParams.xSecurity = security,
        .ucBand = eWiFiBandDual,
    };
    
    map_persistant_w_instance_ctrl_t * p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();
    WIFINetworkParams_t* p_net_params = &net_ext_params.xNetworkParams;
    WIFIEnterpriseNetParams_t* p_net_enterprise_params = &net_ext_params.xEntNetParams;

    memset(&net_ext_params.xEntNetParams, 0, sizeof(WIFIEnterpriseNetParams_t));
    memset(&net_ext_params.xApNetParams, 0, sizeof(WIFIApNetParams_t));

    net_ext_params.ucBand     = eWiFiBandDual;
    net_ext_params.ucWiFi_mode = WIFI_MODE_AUTO_STA;
    net_ext_params.pmf        = PMF_DEFAULT;
    net_ext_params.sae_groups[0] = '\0';


    net_ext_params.hidden_ssid = (params->is_hidden == 1) ? true : false;
    APP_PRINT_INFO("Hidden SSID: %s\n", net_ext_params.hidden_ssid ? "YES" : "NO");

    unescape_special_characters(ssid_len, unescaped_ssid, params->ssid, sizeof(unescaped_ssid));
    strncpy(p_net_params->ucSSID, unescaped_ssid, sizeof(p_net_params->ucSSID));
    p_net_params->ucSSIDLength = strlen(unescaped_ssid);
    switch (security)
    {
        case eWiFiSecurityOpen_ext:
        {
        } break;

        case eWiFiSecurityWPA_ext:
        case eWiFiSecurityWPA2_ext:
        case eWiFiSecurityWPA2_WPA3_ext:
        case eWiFiSecurityWPA_WPA2_ext:
        case eWiFiSecurityWPA3_ext:
        case eWiFiSecurityWPA3_OWE_ext:
        {
            strncpy(p_net_params->xPassword.xWPA.cPassphrase, params->password,
                sizeof(p_net_params->xPassword.xWPA.cPassphrase));
            p_net_params->xPassword.xWPA.ucLength = pass_len;
        } break;

        case eWiFiSecurityWEP_ext:
        {
            strncpy(p_net_params->xPassword.xWEP[0].cKey, params->password,
                sizeof(p_net_params->xPassword.xWEP[0].cKey));
            p_net_params->xPassword.xWEP[0].ucLength = pass_len;
            p_net_params->ucDefaultWEPKeyIndex = 0;
        } break;

        case eWiFiSecurityWPA_ent_ext:
        case eWiFiSecurityWPA_WPA2_ent_ext:
        case eWiFiSecurityWPA2_WPA3_ent_ext:
        case eWiFiSecurityWPA3_ent_ext:
        case eWiFiSecurityWPA3_192B_ent_ext:
        case eWiFiSecurityWPA2_ent_ext:
        {
            p_net_enterprise_params->ucEntAuthType = (WIFIEntAuthTypeExt_t) params->eap_auth_mode;
            p_net_enterprise_params->ucEntAuthProto = (WIFIEntAuthProtoExt_t) params->eap_phase2;

            unescape_special_characters(ssid_len, unescaped_ssid, params->ssid, sizeof(unescaped_ssid));
            strncpy(p_net_enterprise_params->ucID, unescaped_ssid, sizeof(p_net_enterprise_params->ucID));
            p_net_enterprise_params->ucIDLength = strlen(unescaped_ssid);

            strncpy(p_net_enterprise_params->ucPassword, params->password,
                sizeof(p_net_enterprise_params->ucPassword));
            p_net_enterprise_params->ucPasswordLength = pass_len;

            if (FALSE == wifi_enterprise_cert_key_set(&net_ext_params))
            {
                APP_PRINT_ERR("Unable to get / apply a certificate\n");
                return eWiFiFailure;
            }
        } break;

        default:
        {
            APP_PRINT_ERR("Unable to connect to WiFi network due to "
                "unsupported security type: %s (%d)\n", wifi_sec_type_2_string(security_app), security_app);
            return eWiFiNotSupported;
        }
    }

    wifi_network_ext_params_print(&net_ext_params);
    WIFIReturnCode_t rc = WIFI_ConnectAPExt(&net_ext_params);
    RM_MAP_PERSISTANT_W_Write_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_PMF_0, net_ext_params.pmf);
    if (eWiFiSuccess != rc)
    {
        APP_PRINT_ERR("Unable to connect to WiFi network: %s (%d)\n", wifi_err_code_2_string(rc), rc);
    }
    else
    {
        if(FSP_SUCCESS != store_provisioning_params(p_net_params, params->is_hidden))
        {
            APP_PRINT_ERR("Unable to store NVRAM parameters\n");
        }
    }

    return rc;
}

/******************************************************************************
 * Function Name: wifi_connect
 * Description  : Connects to a WiFi network using basic parameters and prints
 *                the connection details.
 * Arguments    : const prov_cmd_select_ap_t *params - Selected AP parameters
 * Return Value : int_t - WiFi connection result code
 ******************************************************************************/
static int_t wifi_connect(const prov_cmd_select_ap_t* params)
{
    APP_PRINT_INFO("\n");
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
    strncpy(&net_params.xPassword.xWPA.cPassphrase[1], params->password,
        sizeof(net_params.xPassword.xWPA.cPassphrase) - 1);
    uint32_t ssid_len_1 = strnlen(net_params.ucSSID, sizeof(net_params.ucSSID) - 2);
    uint32_t pass_len_1 = strnlen(net_params.xPassword.xWPA.cPassphrase,
        sizeof(net_params.xPassword.xWPA.cPassphrase) - 2);
    net_params.ucSSID[ssid_len_1] = '\'';
    net_params.xPassword.xWPA.cPassphrase[pass_len_1] = '\'';
    net_params.ucSSIDLength = ssid_len;
    net_params.xPassword.xWPA.ucLength = pass_len;
    wifi_network_params_print(&net_params);
    WIFIReturnCode_t rc = WIFI_ConnectAP(&net_params);
    if (eWiFiSuccess != rc)
    {
        APP_PRINT_ERR("Unable to connect to WiFi network: %s (%d)\n", wifi_err_code_2_string(rc), rc);
    }

    return rc;
}

/******************************************************************************
 * Function Name: wifi_disconnect
 * Description  : Disconnects from the currently connected WiFi network.
 * Arguments    : const prov_cmd_disconnect_t *params -
 *                  Disconnect command parameters (unused)
 * Return Value : int_t - WiFi disconnect result code
 ******************************************************************************/
static int_t wifi_disconnect(const prov_cmd_disconnect_t* params)
{
    APP_PRINT_INFO("\n");
    FSP_PARAMETER_NOT_USED(*params);
    WIFIReturnCode_t rc = WIFI_Disconnect();
    if (eWiFiSuccess != rc)
    {
        APP_PRINT_ERR("Unable to disconnect from WiFi network: %s (%d)\n", wifi_err_code_2_string(rc), rc);
    }

    return rc;
}

/******************************************************************************
 * Function Name: wifi_ping
 * Description  : Sends ICMP ping requests to the specified IP address.
 * Arguments    : const prov_cmd_network_info_t *params -
 *                  Ping command parameters
 * Return Value : int_t - 0 on success, nonzero on failure
 ******************************************************************************/
static int_t wifi_ping(const prov_cmd_network_info_t* params)
{
    char_t ip_buff[20] = {};
    static struct cmd_ping_param * ping_data = NULL;
    APP_PRINT_INFO("Ping %s...\n", ip_to_string(params->ping_ip, ip_buff, sizeof(ip_buff)));
    uint8_t ip_addr[4] = {0, 0, 0, 0};
    memcpy(ip_addr, &params->ping_ip, sizeof(ip_addr));

    ping_data = pvPortMalloc(sizeof(struct cmd_ping_param));
    if (NULL == ping_data) {
        APP_PRINT_ERR("[%s:%d] mem alloc fail\n", __func__, __LINE__);
        return 1;
    }
    memset(ping_data, 0, sizeof(struct cmd_ping_param));

    // Copy IPv4 address from params into ping_data
#if defined(__SUPPORT_IPV4__)
    ip4_addr_t ip4;
    ip4.addr = PP_HTONL(params->ping_ip);   // assuming ping_ip is uint32_t in network order
    ip_addr_copy_from_ip4(ping_data->ipaddr, ip4);
    ping_data->ipaddr.type = IPADDR_TYPE_V4;
#elif defined(__SUPPORT_IPV6__)
    // If params->ping_ip is IPv6, use ip6addr_aton or memcpy into ping_data->ipaddr
#endif
    // Fill other ping parameters
    ping_data->ping_interface = NONE_IFACE;   // or WLAN1_IFACE depending on your setup
    ping_data->max_count      = DEFAULT_PING_COUNT;        // send 4 echo requests
    ping_data->len            = DEFAULT_PING_SIZE;         // packet size
    ping_data->wait           = DEFAULT_PING_WAIT;         // timeout in ms
    ping_data->interval       = DEFAULT_INTERVAL;          // interval in ms

    if (NONE_IFACE == ping_data->ping_interface)
    {
#if defined (__SUPPORT_IPV4__)
        if (isvalidIPsubnetInterface((PP_HTONL(ip_2_ip4(&ping_data->ipaddr)->addr)), WLAN1_IFACE)
            && lwip_iface_is_up(WLAN1_IFACE)
        ) {
            ping_data->ping_interface = WLAN1_IFACE;
        } else
#endif // __SUPPORT_IPV4__
        {
            ping_data->ping_interface = WLAN0_IFACE;
        }
    }

    /* To give time for connection to wifi AP complete */
    vTaskDelay(portCONVERT_MS_2_TICKS(1500));

    // Call ping_init_for_console directly
    ping_init_for_console(ping_data);

    // Reset global pointer if required
    ping_data = NULL;

    APP_PRINT_INFO("Ping command executed successfully\n");
    return 0;
}

/******************************************************************************
 * Function Name: ble_set_bd_addr
 * Description  : Parses a string Bluetooth address and sets it as the device's
 *                public BLE address.
 * Arguments    : const char *bd_addr_string -
 *                  Bluetooth address in string format
 * Return Value : none
 ******************************************************************************/
static void ble_set_bd_addr(const char *bd_addr_string)
{
    if (!bd_addr_string)
        return;

    size_t bd_addr_len = strlen(bd_addr_string);
    char_t tmp_bd_string[bd_addr_len + 1];
    memset(tmp_bd_string, 0, bd_addr_len + 1);
    memcpy(tmp_bd_string, bd_addr_string, bd_addr_len);
    st_ble_dev_addr_t bd_addr = {.type = BLE_GAP_ADDR_PUBLIC, .addr = {0} };
    char_t *p_saveptr;
    char_t *p_tp = strtok_r(tmp_bd_string, ":", &p_saveptr);
    int32_t p = BLE_BD_ADDR_LEN - 1;
    while ((NULL != p_tp) && (p >= 0))
    {
        bd_addr.addr[p] = (char)strtol(p_tp, NULL, 16);
        p--;
        p_tp = strtok_r(NULL, ":", &p_saveptr);
    }

    ble_status_t ret = R_BLE_VS_SetBdAddr(BLE_VS_ADDR_AREA_REG, &bd_addr);
    assert(BLE_SUCCESS == ret);
}

/******************************************************************************
 * Function Name: ble_uninit
 * Description  : Shuts down the BLE stack and closes the BLE ABS instance.
 * Arguments    : none
 * Return Value : ble_status_t - BLE_SUCCESS on success or error code on failure
 ******************************************************************************/
static ble_status_t ble_uninit(void)
{
    /* Terminate BLE */
    return RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 * Function Name: gap_cb_user
 * Description  : Handles user defined BLE GAP conn and disconn events.
 * Arguments    : uint16_t type                - GAP event type
 *                ble_status_t result          - Event result status
 *                st_ble_evt_data_t *p_data    - Event data
 * Return Value : bool_t - TRUE if event handled, FALSE otherwise
 ******************************************************************************/
static bool_t gap_cb_user(uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data)
{
    bool_t event_handled = FALSE;

    switch (type)
    {
        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t * p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *)p_data->p_param;
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
        } break;

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            st_ble_gap_disconn_evt_t * p_gap_disconn_evt_param = (st_ble_gap_disconn_evt_t *)p_data->p_param;
            uint8_t disconn_reason = p_gap_disconn_evt_param->reason;
            APP_PRINT("Device disconnected\n");
            APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tReason of disconnection: 0x%X\n\n", disconn_reason);

            /* Restart advertising when disconnected */
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

            /* Delete BLE bond keys on device reset */
            if (TRUE == g_delete_bond_pending)
            {
                ble_reset();
                g_delete_bond_pending = FALSE;
            }
            if (MFR_APP_STATE_RUNNING == g_app_state)
            {
                /* We aren't disconnecting due to closing of the service - restore the advertisement. */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }
            event_handled = TRUE;
        } break;

        default: break;
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
void custom_conf_ble(void)
{
    char_t *p_bd_addr = NULL;

#ifndef RM_MAP_PERSISTANT_W
    p_bd_addr = read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
    fsp_err_t err = 0;
    const uint32_t cnt_max = 50; // 5s
    uint32_t cnt = 0;

    /* Wait until RM_MAP_PERSISTANT_Open is called from WiFi thread */
    while (cnt++ < cnt_max)
    {
        err = RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, "PUBLIC_BD_ADDR", &p_bd_addr);
        if (FSP_SUCCESS == err || FSP_ERR_NOT_FOUND == err)
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

/******************************************************************************
 * Function Name: ble_address_to_string
 * Description  : Converts a BLE address into a colon-separated string format
 * Arguments    : const uint8_t *address - Pointer to BLE address array
 * Return Value : const char* - String representation of BLE address
 ******************************************************************************/
static const char *ble_address_to_string(const uint8_t *address)
{
    static char_t buf[18];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", address[5], address[4], address[3], address[2], address[1],
            address[0]);

    return buf;
}

static char * unescape_special_characters(int len, char * output, const char * input, size_t dst_size)
{
    size_t j = 0;

    for (int i = 0; i < len && j + 1 < dst_size; i++)
    {
        char c = input[i];
        if (c == '\\' && i + 1 < len)
        {
            char next = input[i + 1];
            switch (next)
            {
                case '\\': output[j++] = '\\'; i++; break;
                case '"':  output[j++] = '"';  i++; break;
                case '/':  output[j++] = '/';  i++; break;
                case '\'': output[j++] = '\''; i++; break;
                case 't':  output[j++] = '\t'; i++; break;
                default:
                    output[j++] = '\\';
                    break;
            }
            continue;
        }
        output[j++] = c;
    }

    output[j] = '\0';
    return output;
}
/* End user code. Do not edit comment generated here */
