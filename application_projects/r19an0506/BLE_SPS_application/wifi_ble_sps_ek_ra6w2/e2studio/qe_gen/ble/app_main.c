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
#include "r_ble_dis.h"
#include "r_ble_sps_services.h"

/******************************************************************************
 * User file includes
 *******************************************************************************/

/* Start user code for file includes. Do not edit comment generated here */
#include <stdio.h>
#include "ble_thread.h"
#ifdef RM_MAP_PERSISTANT_W
 #include "rm_map_persistant_w.h"
#endif
#include "rm_vee_flash_w_rrq_nvram.h"

#include "comms_ble.h"
#include "rm_comms_lock.h"
#include "comms_bridge.h"
#include "ble_msg_queue.h"
#include "common_utils.h"

/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG                      "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE    (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN       (245)
#define BLE_GATTS_QUEUE_NUM              (1)

/******************************************************************************
 * User macro definitions
 *******************************************************************************/

/* Start user code for macro definitions. Do not edit comment generated here */
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
bool                R_BLE_AbsGapCb(uint16_t type, ble_status_t result, st_ble_evt_data_t * data);
static void         ble_set_bd_addr(const char * bd_addr_string);
void                custom_conf_ble(void);
ble_status_t        ble_uninit(void);
static const char * ble_address_to_string(const uint8_t * address);

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
    0x02,                                                                                           /**< Data Size */
    0x01,                                                                                           /**< Data Type */
    (0x06),                                                                                         /**< Data Value */

    /* Complete List of 128-bit Service Class UUIDs */
    0x11,                                                                                           /**< Data Size */
    0x07,                                                                                           /**< Data Type */
    0xb7, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07, /**< Data Value */
};

/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Complete List of 128-bit Service Class UUIDs */
    0x11,                                                                                           /**< Data Size */
    0x07,                                                                                           /**< Data Type */
    0xb7, 0x5c, 0x49, 0xd2, 0x04, 0xa3, 0x40, 0x71, 0xa0, 0xb5, 0x35, 0x85, 0x3e, 0xb0, 0x83, 0x07, /**< Data Value */

    /* Complete Local Name */
    0x0A,                                                                                           /**< Data Size */
    0x09,                                                                                           /**< Data Type */
    0x53, 0x50, 0x53, 0x2d, 0x52, 0x41, 0x36, 0x57, 0x32,                                           /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
    .p_peer_address             = NULL,                                                        ///< Peer address.
    .slow_advertising_interval  = 0x000000A0,                                                  ///< Slow advertising interval. 100.0(ms)
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
volatile bool sps_flow_enabled = true;

typedef struct
{
    uint8_t state;
} st_ble_sps_services_flow_ctrl_t;

typedef enum e_app_state
{
    APP_STATE_STOPPED,
    APP_STATE_STOPPING,
    APP_STATE_RUNNING,
} app_state_t;

volatile app_state_t g_app_state = APP_STATE_RUNNING; /* Application starts with running state */

/* Advertisement handle */
volatile uint8_t g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

uint8_t remote_addr[BLE_BD_ADDR_LEN];
uint8_t remote_addr_type;
char    remote_addr_type_str[16];

static rm_comms_ble_instance_ctrl_t g_comms_ble_ctrl;

#if BSP_CFG_RTOS == 1                  // ThreadX
 #if !defined(g_comms_ble_tx_mutex)
rm_comms_mutex_t g_comms_ble_tx_mutex =
{
    .p_name = "g_comms_ble tx mutex",
};
 #endif

 #if !defined(g_comms_ble_tx_semaphore)
rm_comms_semaphore_t g_comms_ble_tx_semaphore =
{
    .p_name = "g_comms_ble tx semaphore",
};
 #endif

#elif BSP_CFG_RTOS == 2                // FreeRTOS

 #if !defined(g_comms_ble_tx_mutex)
rm_comms_mutex_t g_comms_ble_tx_mutex;
 #endif

 #if !defined(g_comms_ble_tx_semaphore)
rm_comms_semaphore_t g_comms_ble_tx_semaphore;
 #endif
#else

#endif

rm_comms_ble_extended_cfg_t g_comms_ble_extended_cfg =
{
#if BSP_CFG_RTOS
 #if !defined(g_comms_ble_tx_mutex)
    .p_tx_mutex     = &g_comms_ble_tx_mutex,
 #else
    .p_tx_mutex     = NULL,
 #endif

    .p_rx_mutex     = NULL,

 #if !defined(g_comms_ble_tx_semaphore)
    .p_tx_semaphore = &g_comms_ble_tx_semaphore,
 #else
    .p_tx_semaphore = NULL,
 #endif

    .p_rx_semaphore = NULL,
    .mutex_timeout  = 0xFFFFFFFF,
#endif
};

static rm_comms_cfg_t g_comms_ble_cfg =
{
    .semaphore_timeout = 0xFFFFFFFF,
    .p_lower_level_cfg = NULL,
    .p_extend          = (void *) &g_comms_ble_extended_cfg,
    .p_callback        = NULL,
};

static const rm_comms_instance_t g_comms_ble =
{
    .p_ctrl = &g_comms_ble_ctrl,
    .p_cfg  = &g_comms_ble_cfg,
    .p_api  = &g_comms_on_comms_ble,
};

static rm_comms_bridge_instance_ctrl_t g_comms_bridge_ctrl;

static rm_comms_bridge_cfg_t g_comms_bridge_cfg =
{
    .comms_instance_a = &g_comms_ble,
    .comms_instance_b = &g_comms_uart_w0,
};

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
    if (R_BLE_AbsGapCb(type, result, p_data))
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
        case BLE_GAP_EVENT_ADV_ON:
        {
            const st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;

            APP_PRINT("Legacy advertising started\n");
            break;
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

            APP_PRINT("Legacy advertising stopped\n");
            break;
        }

        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
            const st_ble_gap_data_len_chg_evt_t * p_gap_data_len_chg_evt_param =
                (st_ble_gap_data_len_chg_evt_t *) p_data->p_param;
            uint16_t tx_octets = p_gap_data_len_chg_evt_param->tx_octets;
            uint16_t tx_time   = p_gap_data_len_chg_evt_param->tx_time;
            uint16_t rx_octets = p_gap_data_len_chg_evt_param->rx_octets;
            uint16_t rx_time   = p_gap_data_len_chg_evt_param->rx_time;
            APP_PRINT("Data length changed \n\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tMaximum RX data length: %d\n\tMaximum RX time: %d\n", rx_octets, rx_time);
            APP_PRINT("\tMaximum TX data length: %d\n\tMaximum TX time: %d\n\n", tx_octets, tx_time);
            break;
        }

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            const st_ble_gap_pairing_req_evt_t * p_gap_pairing_req_evt_param =
                (st_ble_gap_pairing_req_evt_t *) p_data->p_param;
            st_ble_gap_auth_info_t auth_info = p_gap_pairing_req_evt_param->auth_info;
            uint8_t                bonding   = auth_info.bonding;
            APP_PRINT("Pair request\n");
            APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tBond: %s\n\n", bonding ? "true" : "false");
            break;
        }

        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            const st_ble_gap_pairing_info_evt_t * p_gap_pairing_info_evt_param =
                (st_ble_gap_pairing_info_evt_t *) p_data->p_param;
            st_ble_gap_auth_info_t auth_info = p_gap_pairing_info_evt_param->auth_info;
            uint8_t                bonding   = auth_info.bonding;
            APP_PRINT("Pair completed\n");
            APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tBond: %s\n\n", bonding ? "true" : "false");
            break;
        }

        case BLE_GAP_EVENT_ENC_CHG:
        {
            const st_ble_gap_enc_chg_evt_t * p_gap_enc_chg_evt_param = (st_ble_gap_enc_chg_evt_t *) p_data->p_param;
            uint8_t enc_status = p_gap_enc_chg_evt_param->enc_status;
            APP_PRINT("Security level changed\n");
            APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tSecurity level: %d\n\n", enc_status);
            break;
        }

        default:
        {
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
        default:
        {
            break;
        }

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
        default:
        {
            break;
        }

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
        st_ble_vs_get_bd_addr_comp_evt_t * p_get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
        AUTOTEST_PRINT("BD address from device: %s\n", ble_address_to_string(p_get_address->addr.addr));
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
        default:
        {
            break;
        }

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
        default:
        {
            break;
        }

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
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: dis_cb
 * Description  : Callback function for Device Information Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Device Information Service server feature.
 *              : ble_status_t result -
 *                  Event result of Device Information Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of Device Information Service server feature.
 * Return Value : none
 ******************************************************************************/
static void dis_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Device Information Service Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Device Information Service server events defined in e_ble_dis_event_t */
/* Start user code for Device Information Service Server callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: sps_services_cb
 * Description  : Callback function for SPS Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of SPS Service server feature.
 *              : ble_status_t result -
 *                  Event result of SPS Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of SPS Service server feature.
 * Return Value : none
 ******************************************************************************/
static void sps_services_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for SPS Service Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of SPS Service server events defined in e_ble_sps_services_event_t */
/* Start user code for SPS Service Server callback function event process. Do not edit comment generated here */
        case BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_WRITE_REQ:
        case BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_WRITE_CMD:
        {
            st_ble_sps_services_sps_server_rx_data_t * p_rx_data =
                (st_ble_sps_services_sps_server_rx_data_t *) p_data->p_param;

            rm_comms_ble_callback(RM_COMMS_BLE_EVENT_DATA_AVAILABLE,
                                  p_rx_data->rx_data,
                                  p_rx_data->rx_len,
                                  g_comms_ble.p_ctrl);
            break;
        }

        case BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_HDL_VAL_CNF:
        {
            rm_comms_ble_callback(RM_COMMS_BLE_EVENT_TX_COMPLETE, NULL, 0, g_comms_ble.p_ctrl);
            break;
        }

        case BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_WRITE_REQ:
        {
            st_ble_sps_services_flow_ctrl_t * flow =
                (st_ble_sps_services_flow_ctrl_t *) p_data->p_param;

            if (flow->state == 1)
            {
                sps_flow_enabled = true;
            }
            else
            {
                sps_flow_enabled = false;
            }

            break;
        }

        default:
        {
            /* Nothing to do here */
            break;
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
static void gapc_cb (uint16_t type, ble_status_t result, st_ble_servc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GAP Service client events defined in e_ble_gapc_event_t */
/* Start user code for GAP Service Client callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

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

    /* Initialize Device Information Service server API */
    status = R_BLE_DIS_Init(dis_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize SPS Service server API */
    status = R_BLE_SPS_SERVICES_Init(sps_services_cb);
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
    ble_status_t   ret;
    rm_ble_msg_t * p_input_msg;
    char         * p_bd_addr = NULL;

    ret = RM_BLE_MSG_QUEUE_Init();
    assert(FSP_SUCCESS == ret);

    p_input_msg = malloc(sizeof(rm_ble_msg_t));

    assert(NULL != p_input_msg);

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

        if (RM_BLE_MSG_QUEUE_Dequeue(p_input_msg) == FSP_SUCCESS)
        {
            switch (p_input_msg->type)
            {
                case RM_BLE_MSG_TYPE_TX_NOTIFY:
                {
                    if (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl)
                    {
                        /* No active connection. Skip the transmission. */
                        break;
                    }

                    ret = R_BLE_SPS_SERVICES_NotifySps_server_tx_data(g_conn_hdl, &p_input_msg->data.tx_data);

                    if (BLE_ERR_INVALID_OPERATION != ret)
                    {
                        if (FSP_SUCCESS != ret)
                        {
                            rm_comms_ble_callback(RM_COMMS_BLE_EVENT_TX_ERROR, NULL, 0, g_comms_ble.p_ctrl);
                        }
                    }
                    else
                    {
                        /* Re-submit the message as BLE subsystem isn't ready yet */
                        ret = RM_BLE_MSG_QUEUE_Push(p_input_msg);
                        assert(FSP_SUCCESS == ret);

                        xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);

                        vTaskDelay(10);
                    }

                    break;
                }

                case RM_BLE_MSG_TYPE_FLOW_CTRL_NOTIFY:
                {
                    if (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl)
                    {
                        /* No active connection. Skip the transmission. */
                        break;
                    }

                    ret = R_BLE_SPS_SERVICES_NotifySps_flow_ctrl(g_conn_hdl, &p_input_msg->data.flow_ctrl_data);
                    if (BLE_ERR_INVALID_OPERATION != ret)
                    {
                        if (FSP_SUCCESS != ret)
                        {
                            rm_comms_ble_callback(RM_COMMS_BLE_EVENT_FLOW_CTRL_ERROR, NULL, 0, g_comms_ble.p_ctrl);
                        }
                    }
                    else
                    {
                        /* Re-submit the message as BLE subsystem isn't ready yet */
                        ret = RM_BLE_MSG_QUEUE_Push(p_input_msg);
                        assert(FSP_SUCCESS == ret);

                        xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);

                        vTaskDelay(10);
                    }

                    break;
                }

                case RM_BLE_MSG_TYPE_SVC_START:
                {
                    /* Started already */
                    if (APP_STATE_RUNNING == g_app_state)
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
                    if (NULL != p_bd_addr)
                    {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    g_app_state = APP_STATE_RUNNING;
                    break;
                }

                case RM_BLE_MSG_TYPE_SVC_STOP:
                {
                    /* Already not running */
                    if (APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    g_app_state = APP_STATE_STOPPING;

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

                case RM_BLE_MSG_TYPE_ADV_START:
                {
                    /* Not running */
                    if (APP_STATE_RUNNING != g_app_state)
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

                case RM_BLE_MSG_TYPE_ADV_STOP:
                {
                    /* Not running */
                    if (APP_STATE_RUNNING != g_app_state)
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

                case RM_BLE_MSG_TYPE_DISCONNECT:
                {
                    /* Not running */
                    if (APP_STATE_RUNNING != g_app_state)
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

                case RM_BLE_MSG_TYPE_BD_ADDR_GET:
                {
                    /* Not running */
                    if (APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    ret = R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_PUBLIC);
                    assert(BLE_SUCCESS == ret);
                    break;
                }

                default:
                {
                    APP_PRINT("Received BLE msg of type (%d) which has not been handled.\n", p_input_msg->type);
                    break;
                }
            }
        }

        if ((APP_STATE_STOPPING == g_app_state) &&
            (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl) &&
            (BLE_GAP_INVALID_ADV_HDL == g_adv_hdl))
        {
            ret = ble_uninit();
            assert(BLE_SUCCESS == ret);

            g_app_state = APP_STATE_STOPPED;
        }

        while (APP_STATE_STOPPED == g_app_state)
        {
            if (RM_BLE_MSG_QUEUE_Dequeue(p_input_msg) == FSP_SUCCESS)
            {
                if (RM_BLE_MSG_TYPE_SVC_START == p_input_msg->type)
                {
#ifndef RM_MAP_PERSISTANT_W
                    p_bd_addr = (uint8_t *) read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                    ENV_GROUP_BLECFG,
                                                    "PUBLIC_BD_ADDR",
                                                    (char *) &p_bd_addr);
#endif
                    if (NULL != p_bd_addr)
                    {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    g_app_state = APP_STATE_RUNNING;
                }
                else
                {
                    vTaskDelay(10);
                }
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
    free(p_input_msg);

    ret = RM_BLE_MSG_QUEUE_Uninit();
    assert(FSP_SUCCESS == ret);

/* End user code. Do not edit comment generated here */

    /* Terminate BLE */
    RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 * User function definitions
 *******************************************************************************/

/* Start user code for function definitions. Do not edit comment generated here */
bool R_BLE_AbsGapCb (uint16_t type, ble_status_t result, st_ble_evt_data_t * data)
{
    bool event_handled = false;

    switch (type)
    {
        case BLE_GAP_EVENT_CONN_IND:
        {
            const st_ble_gap_conn_evt_t * evt = (st_ble_gap_conn_evt_t *) data->p_param;

            if (BLE_SUCCESS == result)
            {
                g_conn_hdl       = evt->conn_hdl;
                remote_addr_type = evt->remote_addr_type;

                st_ble_gap_conn_param_t conn_updt_param_new =
                {
                    .conn_intv_min = 0x0006,

                    .conn_intv_max = 0x0006,

                    .conn_latency  = 0x0000,

                    .sup_to        = 0x000D,
                };
                memcpy(&remote_addr[0], &evt->remote_addr[0], BLE_BD_ADDR_LEN);
                switch (remote_addr_type)
                {
                    case 0x00:
                    {
                        memcpy(&remote_addr_type_str[0], "public", 7);
                        break;
                    }

                    case 0x01:
                    {
                        memcpy(&remote_addr_type_str[0], "random", 7);
                        break;
                    }

                    case 0x02:
                    {
                        memcpy(&remote_addr_type_str[0], "public identity", 16);
                        break;
                    }

                    case 0x03:
                    {
                        memcpy(&remote_addr_type_str[0], "random identity", 16);
                        break;
                    }

                    default:
                    {
                        memcpy(&remote_addr_type_str[0], " ", 2);
                        break;
                    }
                }

                APP_PRINT("Device connected\n");
                APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
                APP_PRINT("\tAddress: %s %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                          remote_addr_type_str,
                          remote_addr[5],
                          remote_addr[4],
                          remote_addr[3],
                          remote_addr[2],
                          remote_addr[1],
                          remote_addr[0]);

                RM_COMMS_BRIDGE_Open(&g_comms_bridge_ctrl, &g_comms_bridge_cfg);

                /* Issue a data length exchange */

                R_BLE_GAP_SetDataLen(g_conn_hdl, 0x00FB, 0x4290);

                R_BLE_GAP_UpdConn(g_conn_hdl, BLE_GAP_CONN_UPD_MODE_REQ, BLE_GAP_CONN_UPD_ACCEPT, &conn_updt_param_new);
            }

            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            RM_COMMS_BRIDGE_Close(&g_comms_bridge_ctrl);

            /* Restart advertising when disconnected */
            const st_ble_gap_disconn_evt_t * p_gap_disconn_evt_param = (st_ble_gap_disconn_evt_t *) data->p_param;
            uint8_t disconn_reason = p_gap_disconn_evt_param->reason;
            APP_PRINT("Device disconnected\n");
            APP_PRINT("\tConnection index: %X\n", g_conn_hdl);
            APP_PRINT("\tBD address of disconnected device: %s, %02X:%02X:%02X:%02X:%02X:%02X\n",
                      remote_addr_type_str,
                      remote_addr[5],
                      remote_addr[4],
                      remote_addr[3],
                      remote_addr[2],
                      remote_addr[1],
                      remote_addr[0]);
            APP_PRINT("\tReason of disconnection: 0x%X\n\n", disconn_reason);
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

            if (APP_STATE_RUNNING == g_app_state)
            {
                /* We aren't disconnecting due to closing of the service - restore the advertisement. */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }

            event_handled = true;
            break;
        }

        default:
        {
            break;
        }
    }

    return event_handled;
}

ble_status_t ble_uninit (void)
{

    /* Terminate BLE */
    return RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

static void ble_set_bd_addr (const char * bd_addr_string)
{
    if (NULL == bd_addr_string)
    {
        return;
    }

    size_t bd_addr_len = strlen(bd_addr_string);
    char   tmp_bd_string[bd_addr_len + 1];

    memset(tmp_bd_string, 0, bd_addr_len + 1);
    memcpy(tmp_bd_string, bd_addr_string, bd_addr_len);

    st_ble_dev_addr_t bd_addr = {.type = BLE_GAP_ADDR_PUBLIC, .addr = {0}};

    char       * saveptr;
    const char * p_tp = strtok_r(tmp_bd_string, ":", &saveptr);
    uint32_t     p    = 5;

    while ((NULL != p_tp) && (p >= 0))
    {
        bd_addr.addr[p] = (char) strtol(p_tp, NULL, 16);
        p--;

        p_tp = strtok_r(NULL, ":", &saveptr);
    }

    ble_status_t ret = R_BLE_VS_SetBdAddr(BLE_VS_ADDR_AREA_REG, &bd_addr);
    assert(BLE_SUCCESS == ret);
}

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

/******************************************************************************
 * Function Name: ble_address_to_string
 * Description  : Converts a BLE address into a colon-separated string format
 * Arguments    : const uint8_t *address - Pointer to BLE address array
 * Return Value : const char* - String representation of BLE address
 ******************************************************************************/
static const char * ble_address_to_string (const uint8_t * address)
{
    static char buf[18];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", address[5], address[4], address[3], address[2], address[1],
            address[0]);

    return buf;
}

/* End user code. Do not edit comment generated here */
