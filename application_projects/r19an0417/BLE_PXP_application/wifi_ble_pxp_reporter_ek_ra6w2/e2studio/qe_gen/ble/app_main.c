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
#include "r_ble_gatc.h"
#include "r_ble_gats.h"
#include "r_ble_dis.h"
#include "r_ble_ias.h"
#include "r_ble_lls.h"
#include "r_ble_tps.h"
#include "r_ble_bas.h"

/******************************************************************************
 * User file includes
 *******************************************************************************/

/* Start user code for file includes. Do not edit comment generated here */
#include <stdio.h>
#include "r_ble_gtl.h"
#include "r_typedefs.h"

#include "ble_thread.h"
#include "cli_ble_queue.h"
#include "common_utils.h"
#include "qe_ble_profile.h"

#ifdef RM_MAP_PERSISTANT_W
 #include "rm_map_persistant_w.h"
#else
 #include "rm_vee_flash_w_rrq_nvram.h"
#endif

#include <app_lib/r_list.h>
#include "led.h"
#include "r_ble_profile_cmn.h"

/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG                           "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE         (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN            (245)
#define BLE_GATTS_QUEUE_NUM                   (1)

/******************************************************************************
 * User macro definitions
 *******************************************************************************/

/* Start user code for macro definitions. Do not edit comment generated here */
#define PX_REPORTER_INCLUDE_BAS               (1)

/* Interval (in ms) for checking battery level (can be configured externally as well) */
#define PX_REPORTER_BATTERY_CHECK_INTERVAL    (60000)

/* Interval (in ms) for checking battery level (can be configured externally as well) */
#define ALERT_TMO_NOTIF                       (1 << 4)
#define BAS_TMO_NOTIF                         (1 << 5)
#define BLINK_TMO_NOTIF                       (1 << 6)
#define PXP_UPDATE_CONN_PARAM_NOTIF           (1 << 7)

#define OS_TASK_NOTIFY_APP_BITS               (ALERT_TMO_NOTIF | BAS_TMO_NOTIF | BLINK_TMO_NOTIF | \
                                               PXP_UPDATE_CONN_PARAM_NOTIF)

/* LED blinking periods in ms */
#define SLOW_BLINKING                         (750)
#define FAST_BLINKING                         (250)

#define NO_MITM                               (0x01)
#define WITH_MITM                             (0x02)

/* default Fast advertising period. 30,000(ms) */
#define FAST_ADVERTISING_PERIOD               (0x0BB8)
#define ABS_LEGACY_HDL                        (0x00)

/** Convert time in milliseconds to connection interval value */
#define BLE_CONN_INTERVAL_FROM_MS(MS)      ((MS) * 100 / 125)

/** Convert time in milliseconds to supervision timeout value */
#define BLE_SUPERVISION_TMO_FROM_MS(MS)    ((MS) / 10)

#define DEFAULT_BLE_ATT_DB_CONFIGURATION    (0x10)                              // Peripheral Pref. Conn. Param. attribute
#define DEFAULT_BLE_PPCP_INTERVAL_MIN       (BLE_CONN_INTERVAL_FROM_MS(500))    // 500 ms
#define DEFAULT_BLE_PPCP_INTERVAL_MAX       (BLE_CONN_INTERVAL_FROM_MS(750))    // 750 ms
#define DEFAULT_BLE_PPCP_SLAVE_LATENCY      (0)                                 // 0 events
#define DEFAULT_BLE_PPCP_SUP_TIMEOUT        (BLE_SUPERVISION_TMO_FROM_MS(6000)) // 6000 ms

#define TICK_PERIOD_MS                      ((TickType_t) ((1000 + (configTICK_RATE_HZ / 2)) / configTICK_RATE_HZ))

#define ATT_ERROR_APPLICATION_ERROR         (0x80)

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
static void  ias_cb(uint16_t type, ble_status_t result, st_ble_ias_evt_data_t * p_data);
ble_status_t ble_init(void);
void         app_main(void);

/******************************************************************************
 * User function prototype declarations
 *******************************************************************************/

/* Start user code for function prototype declarations. Do not edit comment generated here */
static void         bas_update(void);
static void         read_battery_level(uint8_t * const batt_level);
static void         lls_alert_timeout_cb(TimerHandle_t xTimer);
static const char * ble_address_to_string(const uint8_t * address);
static void         handle_encryption_change(st_ble_gap_enc_chg_evt_t * evt);
static void         handle_security_level(uint16_t conn_hdl, st_ble_gap_auth_info_t * sec_info);
static void         conn_params_timer_cb(TimerHandle_t xTimer);
static void         handle_gap_conn_ind(st_ble_gap_conn_evt_t * evt);
static void         handle_gap_disconn_ind(st_ble_gap_disconn_evt_t * evt);
static bool         device_match_addr(const void * elem, const void * addr);
static bool         conn_params_match(const void * elem, const void * ud);
static void         do_alert(uint8_t level);
static void         lls_alert_update(uint16_t conn_hdl, const uint8_t * address, uint8_t level);
static void         conn_param_update(uint16_t conn_hdl);

#if (PX_REPORTER_INCLUDE_BAS == 1)
static void bas_timeout_cb(TimerHandle_t xTimer);

#endif
static void    led_blink_tim_cb(TimerHandle_t xTimer);
static void    ias_alert(uint16_t conn_hdl, uint8_t level);
static void    led_toggle(void);
static void    ble_set_bd_addr(const char * bd_addr_string);
static bool    gap_cb_user(uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data);
static void    dis_send_read_cfm(uint16_t conn_hdl, uint16_t val_hdl);
static uint8_t lls_level_check(uint16_t conn_hdl, uint16_t handle, void * p_app_value);
static uint8_t ias_level_check(uint16_t conn_hdl, uint16_t handle, void * p_app_value);

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated global variables
 *******************************************************************************/

/* GATT Service UUID */
static uint8_t GATC_UUID[] = {0x01, 0x18};

/* Service discovery parameters */
static st_ble_disc_entry_t gs_disc_entries[] =
{
    {
        .p_uuid    = GATC_UUID,
        .uuid_type = BLE_GATT_16_BIT_UUID_FORMAT,
        .serv_cb   = R_BLE_GATC_ServDiscCb,
    },
};

/* Advertising Data */
static uint8_t gs_advertising_data[] =
{
    /* Flags */
    0x02,                                                                                                             /**< Data Size */
    0x01,                                                                                                             /**< Data Type */
    (0x06),                                                                                                           /**< Data Value */

    /* Complete Local Name */
    0x14,                                                                                                             /**< Data Size */
    0x09,                                                                                                             /**< Data Type */
    0x52, 0x65, 0x6e, 0x65, 0x73, 0x61, 0x73, 0x20, 0x50, 0x58, 0x20, 0x52, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x72, /**< Data Value */
};

/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Complete Local Name */
    0x14,                                                                                                             /**< Data Size */
    0x09,                                                                                                             /**< Data Type */
    0x52, 0x65, 0x6e, 0x65, 0x73, 0x61, 0x73, 0x20, 0x50, 0x58, 0x20, 0x52, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x65, 0x72, /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
    .p_peer_address             = NULL,                                                        ///< Peer address.
    .fast_advertising_interval  = 0x00000030,                                                  ///< Fast advertising interval. 30.0(ms)
    .fast_advertising_period    = 0x0BB8,                                                      ///< Fast advertising period. 30,000(ms)
    .slow_advertising_interval  = 0x00000640,                                                  ///< Slow advertising interval. 1,000.0(ms)
    .slow_advertising_period    = 0xFFFF,                                                      ///< Slow advertising period. 655,350(ms)
    .p_advertising_data         = gs_advertising_data,                                         ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
    .advertising_data_length    = ARRAY_SIZE(gs_advertising_data),                             ///< Advertising data length (in bytes).
    .p_scan_response_data       = gs_scan_response_data,                                       ///< Scan response data. If p_scan_response_data is specified as NULL, scan response data is not set.
    .scan_response_data_length  = ARRAY_SIZE(gs_scan_response_data),                           ///< Scan response data length (in bytes).
    .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY,                        ///< Advertising Filter Policy.
    .advertising_channel_map    = (BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39), ///< Channel Map.
    .own_bluetooth_address_type = BLE_GAP_ADDR_PUBLIC,                                         ///< Own Bluetooth address type.
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

/* Immediate Alert Service initialize parameter */
static st_ble_ias_init_param_t gs_ias_init_param =
{
    .cb = ias_cb,
};

/* Immediate Alert Service connect parameter */
static st_ble_ias_connect_param_t gs_ias_conn_param;

/* Immediate Alert Service disconnect parameter */
static st_ble_ias_disconnect_param_t gs_ias_disconn_param;

/******************************************************************************
 * User global variables
 *******************************************************************************/

/* Start user code for global variables. Do not edit comment generated here */

typedef struct __attribute__((packed)) r_ble_gtl_gattc_write_req_ind
{
    uint16_t handle;
    uint16_t offset;
    uint16_t length;
    uint8_t  value[0];
}
r_ble_gtl_gattc_write_req_ind_t;

typedef enum e_mfr_app_state
{
    MFR_APP_STATE_STOPPED,
    MFR_APP_STATE_STOPPING,
    MFR_APP_STATE_RUNNING,
} mfr_app_state_t;

/* Advertisement handle */
volatile uint8_t g_adv_hdl   = BLE_GAP_INVALID_ADV_HDL;
mfr_app_state_t  g_app_state = MFR_APP_STATE_RUNNING; /* Application starts with running state */

uint8_t remote_addr[BLE_BD_ADDR_LEN];
uint8_t remote_addr_type;
char_t  remote_addr_type_str[16];
int8_t  curr_tx_pwr;
uint8_t battery_level;

struct device
{
    struct device * next;
    uint8_t         addr[BLE_BD_ADDR_LEN];
};

typedef struct
{
    void        * next;
    bool          expired;
    uint16_t      conn_hdl;
    TimerHandle_t param_timer;
    TaskHandle_t  current_task;
    uint8_t       addr[BLE_BD_ADDR_LEN];
} conn_dev_t;

/* List of devices pending reconnection */
static void * gp_reconnection_list;

/* List of devices waiting for connection parameters update */
static void * gp_param_connections;

/* LED state */
static bool g_led_active;

typedef struct
{
    st_ble_dev_addr_t addr;
    uint8_t           remote_addr[BLE_BD_ADDR_LEN];
    uint8_t           level;
    bool              level_update;
    bool              connected;
} st_device_info_t;
#define CONN_SUPPORTED_MAX_NUM    (3)
static st_device_info_t gs_connected_device_info[CONN_SUPPORTED_MAX_NUM];
static bool             g_security_level_printed;

/* LLS timer handle*/
static TimerHandle_t g_lls_alert_timer;
#if (PX_REPORTER_INCLUDE_BAS == 1)

/* Timer used for battery monitoring */
static TimerHandle_t g_bas_timer;
#endif

static TimerHandle_t g_led_blink_timer;

/* Timer handle declaration */
static TimerHandle_t g_bas_timer;

#define PX_REPORTER_BATTERY_CHECK_INTERVAL    (60000)

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
            /* Start Advertising when BLE protocol stack is ready */
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t * p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *) p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
                R_BLE_IAS_Connect(g_conn_hdl, &gs_ias_conn_param);
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
            R_BLE_IAS_Disconnect(g_conn_hdl, &gs_ias_disconn_param);
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
        case BLE_GAP_EVENT_LOC_VER_INFO:
        {
            /* fix to local identity address not being set (resulting in GAP resolved providing
             * incorrect private address */
            R_BLE_GAP_SetLocIdInfo((st_ble_dev_addr_t *) (&g_ble_abs0_ctrl.loc_bd_addr), g_ble_abs0_ctrl.local_irk);
            break;
        }

        case BLE_GAP_EVENT_ADV_ON:
        {
            st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;
            if (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl)
            {
                break;
            }

            if (R_BLE_GAP_ReadRssi(g_conn_hdl) != BLE_SUCCESS)
            {
                /* Message will be printed the value of RSSI reading when no connection has established */
                APP_PRINT("\n'No Connection has Established' R_BLE_GAP_ReadRssi(g_conn_hdl): %d", g_conn_hdl);
            }

            break;
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            st_ble_gap_adv_off_evt_t * p_gap_adv_evt_param = (st_ble_gap_adv_off_evt_t *) p_data->p_param;
#if AUTOTEST_ENABLE == 0
            APP_PRINT("\n\tAdvertise stopped with reason : %d\n", p_gap_adv_evt_param->reason);
#endif
            if (1 == p_gap_adv_evt_param->reason)
            {
                g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
            }
            else if (3 <= p_gap_adv_evt_param->reason)
            {
                /* restart ADV only if not max connected devices */
                for (uint8_t i = 0; i < CONN_SUPPORTED_MAX_NUM; i++)
                {
                    if (false == gs_connected_device_info[i].connected)
                    {
                        RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
                        break;
                    }
                }
            }

            break;
        }

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            st_ble_gap_pairing_req_evt_t * p_pair_req_param = (st_ble_gap_pairing_req_evt_t *) p_data->p_param;
#if AUTOTEST_ENABLE
            st_ble_gap_auth_info_t p_auth_param = p_pair_req_param->auth_info;
            AUTOTEST_PRINT("Pair request\r\n");
            AUTOTEST_PRINT("\tConnection index: %d\r\n", p_pair_req_param->conn_hdl);
            AUTOTEST_PRINT("\tBond: %s\r\n", p_auth_param.bonding ? "true" : "false");
#endif

            /* Just accept the pairing request, set bond flag to what peer requested */
            R_BLE_GAP_ReplyPairing(p_pair_req_param->conn_hdl, BLE_GAP_PAIRING_ACCEPT);
            break;
        }

#if AUTOTEST_ENABLE
        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            st_ble_gap_pairing_info_evt_t * p_pair_req_param = (st_ble_gap_pairing_info_evt_t *) p_data->p_param;
            st_ble_gap_auth_info_t          p_auth_param     = p_pair_req_param->auth_info;

            if (!g_security_level_printed)
            {
                /* Security level needs to be handled here as bonding information is present in the stack here */
                handle_security_level(p_pair_req_param->conn_hdl, &(p_pair_req_param->auth_info));
                g_security_level_printed = true;
            }

            AUTOTEST_PRINT("Pair completed\r\n");
            AUTOTEST_PRINT("\tConnection index: %d\r\n", p_pair_req_param->conn_hdl);
            AUTOTEST_PRINT("\tStatus: 0x%02X\r\n", result);
            if (BLE_SUCCESS == result)
            {
                AUTOTEST_PRINT("\tBond: %s\r\n", p_auth_param.bonding ? "true" : "false");
                AUTOTEST_PRINT("\tMITM: %s\r\n", (p_auth_param.security == WITH_MITM) ? "true" : "false");
            }

            break;
        }

        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
            st_ble_gap_data_len_chg_evt_t * p_data_len_chg_param = (st_ble_gap_data_len_chg_evt_t *) p_data->p_param;
            AUTOTEST_PRINT("Data length changed\r\n");
            AUTOTEST_PRINT("\tConnection index: %d\r\n", p_data_len_chg_param->conn_hdl);
            AUTOTEST_PRINT("\tMaximum RX data length: %d\r\n", p_data_len_chg_param->rx_octets);
            AUTOTEST_PRINT("\tMaximum RX time: %d\r\n", p_data_len_chg_param->rx_time);
            AUTOTEST_PRINT("\tMaximum TX data length: %d\r\n", p_data_len_chg_param->tx_octets);
            AUTOTEST_PRINT("\tMaximum TX time: %d\r\n", p_data_len_chg_param->tx_time);
            break;
        }

        case BLE_GAP_EVENT_ENC_CHG:
        {
            st_ble_gap_enc_chg_evt_t * p_evt = (st_ble_gap_enc_chg_evt_t *) p_data->p_param;

            handle_encryption_change(p_evt);
            break;
        }
#endif
        default:
        {
            /* Handle code reach this line? */
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
        case BLE_GATTS_EVENT_DB_ACCESS_IND:
        {
            st_ble_gatts_db_access_evt_t * p_db_access_evt_param = (st_ble_gatts_db_access_evt_t *) p_data->p_param;

            switch (p_db_access_evt_param->p_params->db_op)
            {
                case BLE_GATTS_OP_CHAR_PEER_WRITE_REQ:
                case BLE_GATTS_OP_CHAR_PEER_WRITE_CMD:
                {
                    if (BLE_IAS_ALERT_LEVEL_VAL_HDL == p_db_access_evt_param->p_params->attr_hdl)
                    {
                        uint8_t app_value;
                        app_value = *(p_db_access_evt_param->p_params->value.p_value);
                        st_ble_ias_evt_data_t evt_data =
                        {
                            .conn_hdl  = p_db_access_evt_param->p_handle->conn_hdl,
                            .param_len = sizeof(app_value),
                            .p_param   = &app_value,
                        };

                        ias_cb(BLE_IAS_EVENT_ALERT_LEVEL_WRITE_CMD, BLE_SUCCESS, &evt_data);
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }

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
            /* Handle code reach this line? */
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
/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_VsCb(type, result, p_data);
    switch (type)
    {
/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
            AUTOTEST_PRINT("BD address from device: %s\n", ble_address_to_string(get_address->addr.addr));
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

        case BLE_VS_EVENT_SET_TX_POWER:
        {
            /* Set current Tx_Power */
            st_ble_vs_set_tx_pwr_comp_evt_t * p_tx_pwr_set_param = (st_ble_vs_set_tx_pwr_comp_evt_t *) p_data->p_param;
            curr_tx_pwr = p_tx_pwr_set_param->curr_tx_pwr;

            /* tx_power Transmission power set BLE_VS_TX_POWER_MID(0x01) */
            curr_tx_pwr = BLE_VS_TX_POWER_MID;
            printf("\nBLE_VS_SET_TX_PWR_COMP %d\n", curr_tx_pwr);
            APP_PRINT("\nBLE_VS_SET_TX_PWR_COMP %d\n", curr_tx_pwr);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
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
    FSP_PARAMETER_NOT_USED(*p_data);
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GATT Service server events defined in e_ble_gats_event_t */
/* Start user code for GATT Service Server callback function event process. Do not edit comment generated here */
        default:
        {
            /* Handle code reach this line? */
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
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Device Information Service server events defined in e_ble_dis_event_t */
/* Start user code for Device Information Service Server callback function event process. Do not edit comment generated here */
        case BLE_DIS_EVENT_MFR_NAME_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_MFR_NAME_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_MODEL_NUM_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_MODEL_NUM_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_SER_NUM_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_SER_NUM_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_HW_REV_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_HW_REV_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_FIRM_REV_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_FIRM_REV_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_SW_REV_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_SW_REV_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_SYS_ID_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_SYS_ID_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_REG_CER_DATA_LIST_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_REG_CER_DATA_LIST_VAL_HDL);
            break;
        }

        case BLE_DIS_EVENT_PNP_ID_READ_REQ:
        {
            dis_send_read_cfm(p_data->conn_hdl, BLE_DIS_PNP_ID_VAL_HDL);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: ias_cb
 * Description  : Callback function for Immediate Alert Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Immediate Alert Service server feature.
 *              : ble_status_t result -
 *                  Event result of Immediate Alert Service server feature.
 *              : st_ble_ias_evt_data_t *p_data -
 *                  Event parameters of Immediate Alert Service server feature.
 * Return Value : none
 ******************************************************************************/
static void ias_cb (uint16_t type, ble_status_t result, st_ble_ias_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Immediate Alert Service Server callback function common process. Do not edit comment generated here */
    /*Get input value */
    FSP_PARAMETER_NOT_USED(result);
    uint8_t * p_data_enter;
    p_data_enter = (uint8_t *) p_data->p_param;

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Immediate Alert Service server events defined in e_ble_ias_event_t */
/* Start user code for Immediate Alert Service Server callback function event process. Do not edit comment generated here */
        case BLE_IAS_EVENT_ALERT_LEVEL_WRITE_CMD:
        {
            /* Figure out the level of the alert */
            e_ble_ias_alert_level_t alert_level = *p_data_enter;
            bool correct_value = true;

            switch (alert_level)
            {
                case BLE_IAS_ALERT_LEVEL_ALERT_LEVEL_NO_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_LEVEL_ALERT_LEVEL_NO_ALERT \n");
                    break;
                }

                case BLE_IAS_ALERT_LEVEL_ALERT_LEVEL_MILD_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_ALERT_LEVEL_MILD_ALERT \n");
                    break;
                }

                case BLE_IAS_ALERT_LEVEL_ALERT_LEVEL_HIGH_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_ALERT_LEVEL_HIGH_ALERT \n");
                    break;
                }

                default:
                {
                    correct_value = false;
                    APP_PRINT("\nBLE_ALERT_LEVEL incorrect value, ignored\n");
                    break;
                }
            }

            if (correct_value)
            {
                ias_alert(p_data->conn_hdl, alert_level);
            }

            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: lls_cb
 * Description  : Callback function for Link Loss Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Link Loss Service server feature.
 *              : ble_status_t result -
 *                  Event result of Link Loss Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of Link Loss Service server feature.
 * Return Value : none
 ******************************************************************************/
static void lls_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Link Loss Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);
    uint8_t * p_data_enter;
    p_data_enter = (uint8_t *) p_data->p_param;

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Link Loss Service server events defined in e_ble_lls_event_t */
/* Start user code for Link Loss Service Server callback function event process. Do not edit comment generated here */
        case BLE_LLS_EVENT_ALERT_LEVEL_WRITE_COMP:
        {
            /* Message will be printed on RTT Viewer */
            APP_PRINT("\nBLE_LLS_EVENT_ALERT_LEVEL_WRITE_COMP %d\n", *p_data_enter);
            break;
        }

        case BLE_LLS_EVENT_ALERT_LEVEL_WRITE_REQ:
        {
            APP_PRINT("\nBLE_LLS_EVENT_ALERT_LEVEL_WRITE_REQ is: %d", *p_data_enter);
            e_ble_lls_alert_level_level_t lls_alert_level = *p_data_enter;
            bool correct_value = true;

            switch (lls_alert_level)
            {
                case BLE_LLS_ALERT_LEVEL_LEVEL_NO_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_LLS_ALERT_LEVEL_LEVEL_NO_ALERT");
                    APP_PRINT("\nLED0_ALERT WILL REMAIN ON\n\n");
                    break;
                }

                case BLE_LLS_ALERT_LEVEL_LEVEL_MILD_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_LLS_ALERT_LEVEL_LEVEL_MILD_ALERT");
                    APP_PRINT("\nLED0_ALERT WILL BE BLINKING\n\n");
                    break;
                }

                case BLE_LLS_ALERT_LEVEL_LEVEL_HIGH_ALERT:
                {
                    /* Messages will be printed on RTT Viewer */
                    APP_PRINT("\nBLE_LLS_ALERT_LEVEL_LEVEL_HIGH_ALERT");
                    APP_PRINT("\nLED0_ALERT WILL BE BLINKING FASTER\n\n");
                    break;
                }

                default:
                {
                    correct_value = false;
                    APP_PRINT("\nBLE_LLS_ALERT_LEVEL incorrect value, ignored");
                    break;
                }
            }

            if (correct_value)
            {
                gs_connected_device_info[p_data->conn_hdl].level        = lls_alert_level;
                gs_connected_device_info[p_data->conn_hdl].level_update = true;
            }

            break;
        }

        case BLE_LLS_EVENT_ALERT_LEVEL_READ_REQ:
        {
            uint8_t             lls_alert_level;
            st_ble_gatt_value_t p_value;

            lls_alert_level = gs_connected_device_info[p_data->conn_hdl].level;
            APP_PRINT("\nREAD BACK BLE_LLS_ALERT_LEVEL is: %d\n\n", lls_alert_level);
            p_value.p_value   = &lls_alert_level;
            p_value.value_len = 1;
            R_BLE_GTL_GATTS_ReadCfm(p_data->conn_hdl, BLE_LLS_ALERT_LEVEL_VAL_HDL, &p_value);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: tps_cb
 * Description  : Callback function for Tx Power Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Tx Power Service server feature.
 *              : ble_status_t result -
 *                  Event result of Tx Power Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of Tx Power Service server feature.
 * Return Value : none
 ******************************************************************************/
static void tps_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Tx Power Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Tx Power Service server events defined in e_ble_tps_event_t */
/* Start user code for Tx Power Service Server callback function event process. Do not edit comment generated here */
        case BLE_TPS_EVENT_TX_POWER_LEVEL_READ_REQ:
        {
            st_ble_gatt_value_t p_value;
            p_value.p_value   = (uint8_t *) &curr_tx_pwr;
            p_value.value_len = 1;
            R_BLE_GTL_GATTS_ReadCfm(p_data->conn_hdl, BLE_TPS_TX_POWER_LEVEL_VAL_HDL, &p_value);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: bas_cb
 * Description  : Callback function for Battery Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Battery Service server feature.
 *              : ble_status_t result -
 *                  Event result of Battery Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of Battery Service server feature.
 * Return Value : none
 ******************************************************************************/
static void bas_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Battery Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Battery Service server events defined in e_ble_bas_event_t */
/* Start user code for Battery Service Server callback function event process. Do not edit comment generated here */
        case BLE_BAS_EVENT_BATTERY_LEVEL_READ_REQ:
        {
            st_ble_gatt_value_t p_value;
            p_value.p_value   = &battery_level;
            p_value.value_len = 1;
            R_BLE_GTL_GATTS_ReadCfm(p_data->conn_hdl, BLE_BAS_BATTERY_LEVEL_VAL_HDL, &p_value);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gatc_cb
 * Description  : Callback function for GATT Service client feature.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Service client feature.
 *              : ble_status_t result -
 *                  Event result of GATT Service client feature.
 *              : st_ble_servc_evt_data_t *p_data -
 *                  Event parameters of GATT Service client feature.
 * Return Value : none
 ******************************************************************************/
static void gatc_cb (uint16_t type, ble_status_t result, st_ble_servc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Service Client callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(*p_data);
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GATT Service client events defined in e_ble_gatc_event_t */
/* Start user code for GATT Service Client callback function event process. Do not edit comment generated here */
        default:
        {
            /* Handle code reach this line? */
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
    FSP_PARAMETER_NOT_USED(conn_hdl);

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

    /* Initialize Immediate Alert Service server API */
    status = R_BLE_IAS_Init(&gs_ias_init_param);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize Link Loss Service server API */
    status = R_BLE_LLS_Init(lls_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize Tx Power Service server API */
    status = R_BLE_TPS_Init(tps_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize Battery Service server API */
    status = R_BLE_BAS_Init(bas_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT Service client API */
    status = R_BLE_GATC_Init(gatc_cb);
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
    R_BLE_GATTS_DeregisterCb(NULL);
    R_BLE_GATTS_RegisterCb(gatts_cb, 2);
    *(qe_ble_profile[QE_ATTRIBUTE_HANDLE_CHARACTERISTIC_VALUE_LL_ALERT_LEVEL].p_write_cmd_cb) = lls_level_check;
    *(qe_ble_profile[QE_ATTRIBUTE_HANDLE_CHARACTERISTIC_VALUE_IA_ALERT_LEVEL].p_write_cmd_cb) = ias_level_check;
    ble_status_t  ret;
    cli_ble_msg_t msg;
    char        * p_bd_addr = NULL;

    cli_ble_queue_init();

    // this is workaround to QE-tool not being able to set slow advertising to zero (i.e. continue forever slow ADV))
    g_ble_advertising_parameter.slow_advertising_period = 0;
    uint32_t notif;
    g_lls_alert_timer =
        xTimerCreate("lls timer", 15 * 1000 / TICK_PERIOD_MS, pdFALSE, xTaskGetCurrentTaskHandle(),
                     (TimerCallbackFunction_t) lls_alert_timeout_cb);
#if (PX_REPORTER_INCLUDE_BAS == 1)
    g_bas_timer = xTimerCreate("bas timer",
                               PX_REPORTER_BATTERY_CHECK_INTERVAL / TICK_PERIOD_MS,
                               pdTRUE,
                               xTaskGetCurrentTaskHandle(),
                               (TimerCallbackFunction_t) bas_timeout_cb);
#endif

    /* Create timer to implement led blinking */
    g_led_blink_timer =
        xTimerCreate("led", 500 / TICK_PERIOD_MS, pdFALSE, xTaskGetCurrentTaskHandle(), led_blink_tim_cb);
#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("PXP Reporter application started\r\n");
#endif

#if (PX_REPORTER_INCLUDE_BAS == 1)

    /* Update battery level exposed in BAS */
    bas_update();
#endif                                 /* (PX_REPORTER_INCLUDE_BAS == 1) */
    EventBits_t uxBits = 0;
#if 0

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
#endif

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
            uxBits = xEventGroupWaitBits(g_ble_event_group_handle,
                                         (EventBits_t) BLE_EVENT_PATTERN | OS_TASK_NOTIFY_APP_BITS,
                                         pdTRUE,
                                         pdFALSE,
                                         portMAX_DELAY);
        }
#endif
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
                    APP_PRINT("Received CLI msg of type (%d) which was not handled.\n", msg.type);
                }
            }
        }

        if ((MFR_APP_STATE_STOPPING == g_app_state) &&
            (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl) &&
            (BLE_GAP_INVALID_ADV_HDL == g_adv_hdl))
        {
            ret = RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
            assert(BLE_SUCCESS == ret);
            g_app_state = MFR_APP_STATE_STOPPED;
        }

        while (MFR_APP_STATE_STOPPED == g_app_state)
        {
            if (cli_ble_queue_dequeue(&msg) &&
                (CLI_BLE_MSG_TYPE_SVC_START == msg.type))
            {
#ifndef RM_MAP_PERSISTANT_W
                p_bd_addr = (uint8_t *) read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, "PUBLIC_BD_ADDR",
                                                (char *) &p_bd_addr);
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

        notif = uxBits;
#if (PX_REPORTER_INCLUDE_BAS == 1)
        if (notif & BAS_TMO_NOTIF)
        {
            bas_update();
        }
#endif

        /* Notified from blink timer? */
        if (notif & BLINK_TMO_NOTIF)
        {
            /* Toggle LED state */
            led_toggle();
        }

        if (notif & ALERT_TMO_NOTIF)
        {
            do_alert(0);
            R_List_free(&gp_reconnection_list, NULL, NULL);
        }

        if (notif & PXP_UPDATE_CONN_PARAM_NOTIF)
        {
            conn_dev_t * p_conn_dev = gp_param_connections;

            if (p_conn_dev && p_conn_dev->expired)
            {
                gp_param_connections = p_conn_dev->next;

                conn_param_update(p_conn_dev->conn_hdl);

                xTimerDelete(p_conn_dev->param_timer, 0);
                free(p_conn_dev);

                /*
                 * If the queue is not empty, reset bit and check if timer expired
                 * next time.
                 */
                if (gp_param_connections)
                {
                    BaseType_t pxHigherPriorityTaskWoken = false;
                    xEventGroupSetBitsFromISR((EventGroupHandle_t) g_ble_event_group_handle,
                                              (EventBits_t) PXP_UPDATE_CONN_PARAM_NOTIF,
                                              &pxHigherPriorityTaskWoken);
                }
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
 * User function definitions
 *******************************************************************************/

/* Start user code for function definitions. Do not edit comment generated here */

/******************************************************************************
 * Function Name: read_battery_level
 * Description  : Provides battery level value
 * Arguments    : uint8_t* const batt_level - Pointer to store battery level
 * Return Value : none
 ******************************************************************************/
static void read_battery_level (uint8_t * const batt_level)
{
/* HW battery voltage reading implementation code will be added here later.
 * At the moment a fixed value is used for test flow purpose */
    *batt_level = 0x35;
}

/******************************************************************************
 * Function Name: dummy_read_battery_level
 * Description  : Provides a decrementing dummy battery level
 * Arguments    : uint8_t* const level - Pointer to store dummy battery level
 * Return Value : none
 ******************************************************************************/
#ifdef DECLARE_DUMMY_READ_BATTERY_LEVEL
static void dummy_read_battery_level (uint8_t * const level)
{
    static uint8_t dummy_level = 100;

    *level = dummy_level--;

    /* Handle wrap-around */
    if (dummy_level > 100)
    {
        dummy_level = 100;
    }
}

#endif                                 /* DECLARE_BATTERY_LEVEL_DUMMY_READ */

/******************************************************************************
 * Function Name: bas_update
 * Description  : Updates battery level in GATT database and notifies connected
 *                BLE clients.
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
#if (PX_REPORTER_INCLUDE_BAS == 1)
static void bas_update (void)
{
    ble_status_t status;
 #if AUTOTEST_ENABLE
    bsp_io_level_t ctrl_pin_status;

    /*
     * Low state on P009 means we use dummy battery level.
     * High state (default) means we use actual hardware to read battery level (SoC or GPADC).
     */
    status = R_GPIO_W_PinRead(&g_gpio_w_ctrl, BSP_IO_PORT_00_PIN_09, &ctrl_pin_status);

    if (ctrl_pin_status == BSP_IO_LEVEL_LOW)
    {
        dummy_read_battery_level((uint8_t * const) &battery_level);
    }
    else
    {
        read_battery_level((uint8_t * const) &battery_level);
    }

    AUTOTEST_PRINT("Battery level: %d\r\n", battery_level);
 #else
    read_battery_level((uint8_t * const) &battery_level);
 #endif

    /* Update battery level on GATT database and notify client */
    R_BLE_BAS_SetBatteryLevel(&battery_level);
    uint8_t conn_hdl;

    for (conn_hdl = 0; conn_hdl < CONN_SUPPORTED_MAX_NUM; conn_hdl++)
    {
        if (gs_connected_device_info[conn_hdl].connected)
        {
            status = R_BLE_BAS_NotifyBatteryLevel(conn_hdl, &battery_level);
            if (BLE_SUCCESS != status)
            {
                if (BLE_ERR_INVALID_OPERATION == status)
                {
                    APP_PRINT("Error in BATT Notify. Enable notifications for BAS.\n");
                }
                else
                {
                    APP_PRINT("Error in BATT Notify - %x\n", status);
                }
            }

            break;
        }
    }
}

#endif

/******************************************************************************
 * Function Name: lls_alert_timeout_cb
 * Description  : Callback to set alert timeout notif event bits from ISR
 * Arguments    : TimerHandle_t xTimer - Timer handle
 * Return Value : none
 ******************************************************************************/
static void lls_alert_timeout_cb (TimerHandle_t xTimer)
{
    FSP_PARAMETER_NOT_USED(xTimer);
    BaseType_t pxHigherPriorityTaskWoken = false;
    xEventGroupSetBitsFromISR((EventGroupHandle_t) g_ble_event_group_handle,
                              (EventBits_t) ALERT_TMO_NOTIF,
                              &pxHigherPriorityTaskWoken);
}

/******************************************************************************
 * Function Name: bas_timeout_cb
 * Description  : Callback to set BAS timeout notification event bits from ISR
 * Arguments    : TimerHandle_t xTimer - Timer handle
 * Return Value : none
 ******************************************************************************/
#if (PX_REPORTER_INCLUDE_BAS == 1)
static void bas_timeout_cb (TimerHandle_t xTimer)
{
    FSP_PARAMETER_NOT_USED(xTimer);
    BaseType_t pxHigherPriorityTaskWoken = false;
    xEventGroupSetBitsFromISR((EventGroupHandle_t) g_ble_event_group_handle,
                              (EventBits_t) BAS_TMO_NOTIF,
                              &pxHigherPriorityTaskWoken);
}

#endif

/******************************************************************************
 * Function Name: led_blink_tim_cb
 * Description  : Callback to set LED blink timeout notif event bits from ISR
 * Arguments    : TimerHandle_t xTimer - Timer handle
 * Return Value : none
 ******************************************************************************/
static void led_blink_tim_cb (TimerHandle_t xTimer)
{
    FSP_PARAMETER_NOT_USED(xTimer);
    BaseType_t pxHigherPriorityTaskWoken = false;
    xEventGroupSetBitsFromISR((EventGroupHandle_t) g_ble_event_group_handle,
                              (EventBits_t) BLINK_TMO_NOTIF,
                              &pxHigherPriorityTaskWoken);
}

/******************************************************************************
 * Function Name: ble_address_to_string
 * Description  : Converts a BLE address into a colon-separated string format
 * Arguments    : const uint8_t *address - Pointer to BLE address array
 * Return Value : const char* - String representation of BLE address
 ******************************************************************************/
static const char * ble_address_to_string (const uint8_t * address)
{
    static char_t buf[18];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", address[5], address[4], address[3], address[2], address[1],
            address[0]);

    return buf;
}

/******************************************************************************
 * Function Name: handle_encryption_change
 * Description  : Handles BLE encryption change event and updates security
 *                level status
 * Arguments    : st_ble_gap_enc_chg_evt_t *evt -
 *                           Encryption change event parameters
 * Return Value : none
 ******************************************************************************/
static void handle_encryption_change (st_ble_gap_enc_chg_evt_t * evt)
{
    st_ble_gap_auth_info_t sec_info;

    if (R_BLE_GAP_GetDevSecInfo(evt->conn_hdl, &sec_info) == BLE_SUCCESS)
    {
        handle_security_level(evt->conn_hdl, &sec_info);
        g_security_level_printed = true;
    }
    else
    {
        g_security_level_printed = false;
    }
}

/******************************************************************************
 * Function Name: handle_security_level
 * Description  : Determines and prints BLE security level based on auth info
 * Arguments    : uint16_t conn_hdl - Connection handle
 *                st_ble_gap_auth_info_t *sec_info - Pointer to security info
 * Return Value : none
 ******************************************************************************/
static void handle_security_level (uint16_t conn_hdl, st_ble_gap_auth_info_t * sec_info)
{
    uint8_t level = 0;

    if (NO_MITM == sec_info->security)
    {
        level = 1;
    }
    else
    {
        if (16 == sec_info->ekey_size)
        {
            level = 3;
        }
        else
        {
            level = 2;
        }
    }

    AUTOTEST_PRINT("Security level changed\r\n");
    AUTOTEST_PRINT("\tConnection index: %d\r\n", conn_hdl);
    AUTOTEST_PRINT("\tSecurity level: %u\r\n", level + 1);
}

/******************************************************************************
 * Function Name: conn_params_timer_cb
 * Description  : Callback to mark connection parameters as expired and notify
 *                update event from ISR
 * Arguments    : TimerHandle_t xTimer - Timer handle
 * Return Value : none
 ******************************************************************************/
static void conn_params_timer_cb (TimerHandle_t xTimer)
{
    conn_dev_t * p_conn_dev = (conn_dev_t *) pvTimerGetTimerID(xTimer);;

    p_conn_dev = R_List_find(gp_param_connections, conn_params_match, (const void *) (uint32_t) p_conn_dev->conn_hdl);
    if (p_conn_dev)
    {
        p_conn_dev->expired = true;
        BaseType_t pxHigherPriorityTaskWoken = false;
        xEventGroupSetBitsFromISR((EventGroupHandle_t) g_ble_event_group_handle,
                                  (EventBits_t) PXP_UPDATE_CONN_PARAM_NOTIF,
                                  &pxHigherPriorityTaskWoken);
    }
}

/******************************************************************************
 * Function Name: handle_gap_conn_ind
 * Description  : Handles BLE connection indication, updates device info,
 *                starts timers, and manages reconnections
 * Arguments    : st_ble_gap_conn_evt_t *evt - Connection event parameters
 * Return Value : none
 ******************************************************************************/
static void handle_gap_conn_ind (st_ble_gap_conn_evt_t * evt)
{
    conn_dev_t    * p_conn_dev;
    struct device * dev;

    gs_connected_device_info[evt->conn_hdl].level        = 0;
    gs_connected_device_info[evt->conn_hdl].level_update = false;
    memcpy(gs_connected_device_info[evt->conn_hdl].remote_addr, evt->remote_addr, BLE_BD_ADDR_LEN);
    gs_connected_device_info[evt->conn_hdl].connected = true;
#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("Device connected\r\n");
    AUTOTEST_PRINT("\tConnection index: %d\r\n", evt->conn_hdl);
    AUTOTEST_PRINT("\tAddress: %s\r\n", ble_address_to_string(evt->remote_addr));
#endif

#if (PX_REPORTER_INCLUDE_BAS == 1)

    /* Start battery monitoring if not yet started, but first update current battery level */
    if (!xTimerIsTimerActive(g_bas_timer))
    {
        bas_update();
        xTimerStart(g_bas_timer, 0);
    }
#endif                                 /* (PX_REPORTER_INCLUDE_BAS == 1) */

    /* Add timer that when expired will re-negotiate connection parameters */
    p_conn_dev = malloc(sizeof(*p_conn_dev));
    if (p_conn_dev)
    {
        p_conn_dev->conn_hdl     = evt->conn_hdl;
        p_conn_dev->expired      = false;
        p_conn_dev->current_task = xTaskGetCurrentTaskHandle();
        p_conn_dev->param_timer  =
            xTimerCreate("conn timer",
                         5 * 1000 / TICK_PERIOD_MS,
                         pdFALSE,
                         (void *) p_conn_dev,
                         (TimerCallbackFunction_t) conn_params_timer_cb);
        R_List_append(&gp_param_connections, p_conn_dev);
        xTimerStart(p_conn_dev->param_timer, 0);
    }

    /*
     * Try to unlink the device with the same address from the reconnection list - if found,
     * this is a reconnection and we should stop alerting and also clear the reconnection list
     * and disable the timer.
     */
    dev = R_List_unlink(&gp_reconnection_list, device_match_addr, &evt->remote_addr);
    if (dev)
    {
        do_alert(0);

        R_List_free(&gp_reconnection_list, NULL, NULL);

        xTimerStop(g_lls_alert_timer, 0);

        /*
         * The device is reconnected so set interval values immediately to "reduced power".
         * Stop advertising so it can be restarted with the new interval values.
         */
        R_BLE_GAP_StopAdv(ABS_LEGACY_HDL);
        g_ble_advertising_parameter.fast_advertising_period = 0;
    }
}

/******************************************************************************
 * Function Name: device_match_addr
 * Description  : Compares a device addr with a given addr to check for a match
 * Arguments    : const void *elem - Pointer to device structure
 *                const void *addr - Pointer to address to compare
 * Return Value : bool - true if addresses match, false otherwise
 ******************************************************************************/
static bool device_match_addr (const void * elem, const void * addr)
{
    const struct device * dev = elem;

    return !memcmp(&dev->addr, addr, BLE_BD_ADDR_LEN);
}

/******************************************************************************
 * Function Name: do_alert
 * Description  : Controls LED behavior based on alert level
 * Arguments    : uint8_t level - Alert level value
 * Return Value : none
 ******************************************************************************/
static void do_alert (uint8_t level)
{
    switch (level)
    {
        case 0:
        {
            xTimerStop(g_led_blink_timer, 0);
            LED_LEDOnOff(BSP_LED_LED1, BSP_IO_LEVEL_LOW);
            g_led_active = false;
#if AUTOTEST_ENABLE
            AUTOTEST_PRINT("Set alert level: No Alert\r\n");
#endif
            break;
        }

        case 1:
        case 2:
        {
            xTimerStop(g_led_blink_timer, 0);
            if (1 == level)
            {
                xTimerChangePeriod(g_led_blink_timer, SLOW_BLINKING / TICK_PERIOD_MS, 0);
#if AUTOTEST_ENABLE
                AUTOTEST_PRINT("Set alert level: Mild\r\n");
#endif
            }
            else
            {
                xTimerChangePeriod(g_led_blink_timer, FAST_BLINKING / TICK_PERIOD_MS, 0);
#if AUTOTEST_ENABLE
                AUTOTEST_PRINT("Set alert level: High\r\n");
#endif
            }

            LED_LEDOnOff(BSP_LED_LED1, BSP_IO_LEVEL_HIGH);
            g_led_active = true;
            xTimerStart(g_led_blink_timer, 0);
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }
    }
}

/******************************************************************************
 * Function Name: handle_gap_disconn_ind
 * Description  : Handles BLE disconnection event, updates device info,
 *                manages alerts, timers, and advertising
 * Arguments    : st_ble_gap_disconn_evt_t *evt - Disconnection event params
 * Return Value : none
 ******************************************************************************/
static void handle_gap_disconn_ind (st_ble_gap_disconn_evt_t * evt)
{
    conn_dev_t * p_conn_dev = R_List_unlink(&gp_param_connections,
                                            conn_params_match,
                                            (const void *) (uint32_t) evt->conn_hdl);

    if (gs_connected_device_info[evt->conn_hdl].level_update)
    {
        /* do not fire alert if disconnection was triggered by either side */
        if ((0x00 != evt->reason) && (0x13 != evt->reason) && (0x16 != evt->reason))
        {
            lls_alert_update(evt->conn_hdl,
                             gs_connected_device_info[evt->conn_hdl].remote_addr,
                             gs_connected_device_info[evt->conn_hdl].level);

            return;
        }
    }
    else
    {
        do_alert(0);
    }

#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("Device disconnected\r\n");
    AUTOTEST_PRINT("\tConnection index: %d\r\n", evt->conn_hdl);
    AUTOTEST_PRINT("\tBD address of disconnected device: %s\r\n",
                   ble_address_to_string(gs_connected_device_info[evt->conn_hdl].remote_addr));
    AUTOTEST_PRINT("\tReason of disconnection: 0x%02x\r\n", evt->reason);
#endif
    memset(&gs_connected_device_info[evt->conn_hdl], 0, sizeof(st_device_info_t));

    /*
     * The device is still in the list if the disconnection happened before the timer expiration.
     * In this case stop the timer and free the associated memory.
     */
    if (p_conn_dev)
    {
        xTimerDelete(p_conn_dev->param_timer, 0);
        free(p_conn_dev);
    }

    /* Switch back to fast advertising interval */
    g_ble_advertising_parameter.fast_advertising_period = FAST_ADVERTISING_PERIOD;
    R_BLE_GAP_StopAdv(ABS_LEGACY_HDL);

#if (PX_REPORTER_INCLUDE_BAS == 1)

    /* Stop monitoring battery level if no one is connected */
    if (R_List_size(gp_reconnection_list) == 0)
    {
        xTimerStop(g_bas_timer, 0);
    }
#endif                                 /* (PX_REPORTER_INCLUDE_BAS == 1) */
}

/******************************************************************************
 * Function Name: conn_params_match
 * Description  : Checks if a connection device matches the given conn_hdl
 * Arguments    : const void *elem - Pointer to connection device structure
 *                const void *ud   - Pointer to connection handle
 * Return Value : bool - true if connection handle matches, false otherwise
 ******************************************************************************/
static bool conn_params_match (const void * elem, const void * ud)
{
    conn_dev_t * p_conn_dev = (conn_dev_t *) elem;
    uint16_t     conn_hdl   = (uint16_t) (uint32_t) ud;

    return p_conn_dev->conn_hdl == conn_hdl;
}

/******************************************************************************
 * Function Name: lls_alert_update
 * Description  : Updates LLS alert level, adds device to reconnection list,
 *                triggers alert, and restarts timer
 * Arguments    : uint16_t conn_hdl       - Connection handle
 *                const uint8_t *address  - Pointer to device address
 *                uint8_t level           - Alert level value
 * Return Value : none
 ******************************************************************************/
static void lls_alert_update (uint16_t conn_hdl, const uint8_t * address, uint8_t level)
{
    struct device * dev;

#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("LLS alert level update\r\n");
    AUTOTEST_PRINT("\tConnection index: %d\r\n", conn_hdl);
    AUTOTEST_PRINT("\tAlert level: %d\r\n", level);
#endif

    if (0 == level)
    {
        return;
    }

    /* Add alerting device to reconnection list */
    dev = malloc(sizeof(*dev));
    memcpy(&dev->addr, address, sizeof(dev->addr));
    R_List_add(&gp_reconnection_list, dev);

    /* Trigger an alert */
    do_alert(level);

    /* (Re)start alert timeout timer */
    xTimerReset(g_lls_alert_timer, 0);

    /*
     * Set interval values to "fast connect" and stop advertising timer. Stop advertising
     * so it can be restarted with new interval values.
     */
    R_BLE_GAP_StopAdv(ABS_LEGACY_HDL);
    g_ble_advertising_parameter.fast_advertising_period = FAST_ADVERTISING_PERIOD;
}

/******************************************************************************
 * Function Name: conn_param_update
 * Description  : Updates BLE connection parameters with default PPCP values
 * Arguments    : uint16_t conn_hdl - Connection handle
 * Return Value : none
 ******************************************************************************/
static void conn_param_update (uint16_t conn_hdl)
{
    st_ble_gap_conn_param_t conn_updt_param;

    memset(&conn_updt_param, 0, sizeof(conn_updt_param));

    conn_updt_param.conn_intv_max = DEFAULT_BLE_PPCP_INTERVAL_MAX;
    conn_updt_param.conn_intv_min = DEFAULT_BLE_PPCP_INTERVAL_MIN;
    conn_updt_param.conn_latency  = DEFAULT_BLE_PPCP_SLAVE_LATENCY;
    conn_updt_param.sup_to        = DEFAULT_BLE_PPCP_SUP_TIMEOUT;

    R_BLE_GAP_UpdConn(conn_hdl, BLE_GAP_CONN_UPD_MODE_REQ, 0, &conn_updt_param);
}

/******************************************************************************
 * Function Name: ias_alert
 * Description  : Handles IAS alert level and triggers LED behavior.
 * Arguments    : uint16_t conn_hdl - Connection handle
 *                uint8_t level     - Alert level value
 * Return Value : none
 ******************************************************************************/
static void ias_alert (uint16_t conn_hdl, uint8_t level)
{
#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("IAS alert level update\r\n");
    AUTOTEST_PRINT("\tConnection index: %d\r\n", conn_hdl);
    AUTOTEST_PRINT("\tAlert level: %u\r\n", level);
#endif
    do_alert(level);
}

/******************************************************************************
 * Function Name: led_toggle
 * Description  : Toggles LED state if active and restarts the blink timer
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
static void led_toggle (void)
{
    static bool led_state;

    if (!g_led_active)
    {
        return;
    }

    led_state = !led_state;
    LED_LEDOnOff(BSP_LED_LED1, led_state);
    xTimerStart(g_led_blink_timer, 0);
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
    char       * p_tp = strtok_r(tmp_bd_string, ":", &saveptr);
    unsigned int p    = BLE_BD_ADDR_LEN;
    while ((NULL != p_tp) && (p > 0))
    {
        bd_addr.addr[p - 1] = (char) strtol(p_tp, NULL, 16);
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
 * Function Name: gap_cb_user
 * Description  : Handles custom BLE GAP events
 * Arguments    : uint16_t type              - GAP event type
 *                ble_status_t result        - Result status of the event
 *                st_ble_evt_data_t *p_data  - Pointer to event data
 * Return Value : bool - true if event handled, false otherwise
 ******************************************************************************/
static bool gap_cb_user (uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data)
{
    bool event_handled = false;

    switch (type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            memset(gs_connected_device_info, 0, sizeof(gs_connected_device_info));
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);

            event_handled = true;
            break;
        }

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                st_ble_gap_conn_evt_t * p_evt = (st_ble_gap_conn_evt_t *) p_data->p_param;

                // Default code replicated
                g_conn_hdl = p_evt->conn_hdl;

                // Custom code
                handle_gap_conn_ind(p_evt);
            }

            ble_status_t adv_err = BLE_SUCCESS;
            adv_err = RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            if (BLE_SUCCESS != adv_err)
            {
                APP_ERR_PRINT("\nAdvertise Start Error: %d", adv_err);
            }

            event_handled = true;
            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Store connection handle */
            st_ble_gap_disconn_evt_t * p_evt = (st_ble_gap_disconn_evt_t *) p_data->p_param;

            // Custom code
            handle_gap_disconn_ind(p_evt);
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);

            event_handled = true;
            break;
        }

        default:
        {
            /* Handle code reach this line? */
            break;
        }
    }

    return event_handled;
}

/******************************************************************************
 * Function Name: dis_send_read_cfm
 * Description  : Sends read confirmation for a Device Information Service
 *                characteristic.
 * Arguments    : uint16_t conn_hdl - Connection hdl of the client
 *                uint16_t val_hdl  - Attribute hdl of the DIS characteristic
 * Return Value : void
 ******************************************************************************/
static void dis_send_read_cfm (uint16_t conn_hdl, uint16_t val_hdl)
{
    st_ble_gatt_value_t gatt_value;
    gatt_value.p_value   = qe_ble_profile[val_hdl].value;
    gatt_value.value_len = qe_ble_profile[val_hdl].value_length;

    /* Confirm read back to the client */
    R_BLE_GTL_GATTS_ReadCfm(conn_hdl, val_hdl, &gatt_value);
}

static uint8_t ias_level_check (uint16_t conn_hdl, uint16_t handle, void * p_app_value)
{
    FSP_PARAMETER_NOT_USED(conn_hdl);
    FSP_PARAMETER_NOT_USED(handle);
    uint8_t ias_alert_value;
    uint8_t status = 0;
    r_ble_gtl_gattc_write_req_ind_t * p_param = (r_ble_gtl_gattc_write_req_ind_t *) p_app_value;

    if (p_param->length > 1)
    {
        status = ATT_ERROR_APPLICATION_ERROR;
    }
    else
    {
        ias_alert_value = p_param->value[0];
        if (ias_alert_value > BLE_IAS_ALERT_LEVEL_ALERT_LEVEL_HIGH_ALERT)
        {
            status = ATT_ERROR_APPLICATION_ERROR;
        }
    }

    return status;
}

static uint8_t lls_level_check (uint16_t conn_hdl, uint16_t handle, void * p_app_value)
{
    FSP_PARAMETER_NOT_USED(conn_hdl);
    FSP_PARAMETER_NOT_USED(handle);
    uint8_t lls_alert_value;
    uint8_t status = 0;
    r_ble_gtl_gattc_write_req_ind_t * p_param = (r_ble_gtl_gattc_write_req_ind_t *) p_app_value;

    if (p_param->length > 1)
    {
        status = ATT_ERROR_APPLICATION_ERROR;
    }
    else
    {
        lls_alert_value = p_param->value[0];
        if (lls_alert_value > BLE_LLS_ALERT_LEVEL_LEVEL_HIGH_ALERT)
        {
            status = ATT_ERROR_APPLICATION_ERROR;
        }
    }

    return status;
}

/* End user code. Do not edit comment generated here */
