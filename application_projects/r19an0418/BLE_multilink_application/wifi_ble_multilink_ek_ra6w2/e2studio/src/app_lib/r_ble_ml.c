/***********************************************************************************************************************
 * File Name: r_ble_ml.c
 * Version : 1.0
 * Description : BLE multi-link application.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Copyright (c) 2019 - 2024 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/

/******************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "rm_ble_abs_api.h"
#include "rm_ble_abs.h"
#include "r_ble_ml.h"
#include "r_queue.h"
#include "r_ble_gtl_security.h"
#include "ble_thread.h"
#include "common_utils.h"

/******************************************************************************
 * Macro definitions
 *******************************************************************************/
#define NO_MITM            (0x01)
#define WITH_MITM          (0x02)
#define CONN_WORKAROUND    (1)

/******************************************************************************
 * Function prototype declarations
 *******************************************************************************/
static const char * ble_address_to_string(const uint8_t * address);

/******************************************************************************
 * Global variables
 *******************************************************************************/
typedef struct
{
    void * next;

    ble_device_address_t addr;
    uint16_t             conn_hdl;
} st_conn_dev_t;

typedef enum
{
    CONN_CANCEL_NO_REASON,
    CONN_CANCEL_REASON_DEV_NOT_RESPONDING,
} e_conn_canceled_reason_t;

#if CONN_WORKAROUND
typedef struct
{
    uint16_t          conn_hdl;
    st_ble_dev_addr_t addr;
#if AUTOTEST_ENABLE
    bool security_level_printed;
#endif
} st_device_info_t;
#endif

/** BLE error code */
typedef enum
{
    BLE_STATUS_OK                   = 0x00, /**< Success */
    BLE_ERROR_FAILED                = 0x01, /**< Generic failure */
    BLE_ERROR_ALREADY_DONE          = 0x02, /**< Already done */
    BLE_ERROR_IN_PROGRESS           = 0x03, /**< Operation already in progress */
    BLE_ERROR_INVALID_PARAM         = 0x04, /**< Invalid parameter */
    BLE_ERROR_NOT_ALLOWED           = 0x05, /**< Not allowed */
    BLE_ERROR_NOT_CONNECTED         = 0x06, /**< Not connected */
    BLE_ERROR_NOT_SUPPORTED         = 0x07, /**< Not supported */
    BLE_ERROR_NOT_ACCEPTED          = 0x08, /**< Not accepted */
    BLE_ERROR_BUSY                  = 0x09, /**< Busy */
    BLE_ERROR_TIMEOUT               = 0x0A, /**< Request timed out */
    BLE_ERROR_NOT_SUPPORTED_BY_PEER = 0x0B, /**< Not supported by peer */
    BLE_ERROR_CANCELED              = 0x0C, /**< Canceled by user */
    BLE_ERROR_ENC_KEY_MISSING       = 0x0D, /**< encryption key missing */
    BLE_ERROR_INS_RESOURCES         = 0x0E, /**< insufficient resources */
    BLE_ERROR_NOT_FOUND             = 0x0F, /**< not found */
    BLE_ERROR_L2CAP_NO_CREDITS      = 0x10, /**< no credits available on L2CAP CoC */
    BLE_ERROR_L2CAP_MTU_EXCEEDED    = 0x11, /**< MTU exceeded on L2CAP CoC */
    BLE_ERROR_INS_BANDWIDTH         = 0x12, /**< Insufficient bandwidth */
    BLE_ERROR_LMP_COLLISION         = 0x13, /**< LMP collision */
    BLE_ERROR_DIFF_TRANS_COLLISION  = 0x14, /**< Different transaction collision */
} ble_error_t;

static ble_abs_connection_phy_parameter_t gs_conn_phy_1m =
{
    .connection_interval      = 0x00A0,
    .connection_slave_latency = 0x0000,
    .supervision_timeout      = 0x03E8,
};

ble_device_address_t gs_conn_bd_addr;

ble_abs_connection_parameter_t gs_conn_param =
{
    .filter_parameter                 = BLE_GAP_INIT_FILT_USE_ADDR,
    .connection_timeout               = 0,
    .p_connection_phy_parameter_1M    = &gs_conn_phy_1m,
    .p_connection_phy_parameter_2M    = NULL,
    .p_connection_phy_parameter_coded = NULL,
    .p_device_address                 = &gs_conn_bd_addr,
};

static queue_t                  connections;
static uint16_t                 master_dev_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
static e_conn_canceled_reason_t conn_canceled_reason;
#if CONN_WORKAROUND
static st_device_info_t connected_device_info[BLE_ABS_CFG_RF_CONNECTION_MAXIMUM];
#endif

/*********************************************************************************************************************
 * Function Name: matched_strlen
 * Description  : Returns number of characters matched from the top.
 *                e.g. p_str1=gatts, p_str2=gattc, then return 4.
 * Arguments    : p_str1: string 1
 *                p_str2: string 2
 * Return Value : Number of characters matched from the top.
 ********************************************************************************************************************/

static const char * format_bd_address (const ble_device_address_t * addr)
{
    static char  buf[19];
    unsigned int i;

    for (i = 0; i < sizeof(addr->addr); i++)
    {
        unsigned int idx;

        // for printout, address should be reversed
        idx = sizeof(addr->addr) - i - 1;
        sprintf(&buf[i * 3], "%02X:", addr->addr[idx]);
    }

    buf[sizeof(buf) - 2] = '\0';

    return buf;
}

static void print_connection_func (void * data, void * user_data)
{
    const st_conn_dev_t * conn_dev = data;
    int                 * num      = user_data;

    (*num)++;

    printf("%2d | %5d | %s\r\n", *num, conn_dev->conn_hdl, format_bd_address(&conn_dev->addr));
}

static const char * ble_address_to_string (const uint8_t * address)
{
    static char buf[18];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", address[5], address[4], address[3], address[2], address[1],
            address[0]);

    return buf;
}

static bool list_elem_match (const void * elem, const void * ud)
{
    st_conn_dev_t * conn_dev = (st_conn_dev_t *) elem;
    uint16_t      * conn_hdl = (uint16_t *) ud;

    return conn_dev->conn_hdl == *conn_hdl;
}

static void print_connections (void)
{
    int num = 0;

    printf("\r\n");
    printf("Nr | Index | Address\r\n");
    R_Queue_Foreach(&connections, print_connection_func, &num);

    if (!num)
    {
        printf("(no active connections)\r\n");
    }

#if AUTOTEST_ENABLE
    AUTOTEST_PRINT("End of active connections list\r\n");
#endif
    printf("\r\n");
}

#if CONN_WORKAROUND
static uint8_t get_index_for_handle (uint16_t hdl)
{
    uint8_t i;

    for (i = 0; i < BLE_ABS_CFG_RF_CONNECTION_MAXIMUM; i++)
    {
        if (connected_device_info[i].conn_hdl == hdl)
        {
            break;
        }
    }

    return i;
}

#endif

static void handle_connection_indication (st_ble_gap_conn_evt_t * evt)
{
    st_conn_dev_t * conn_dev;
#if CONN_WORKAROUND
    uint8_t idx;

    idx = get_index_for_handle(BLE_GAP_INVALID_CONN_HDL);
    connected_device_info[idx].conn_hdl = evt->conn_hdl;
    connected_device_info[idx].security_level_printed = false;
#endif

    printf("Device connected\r\n");
#if CONN_WORKAROUND
    printf("\tConnection index: %u\r\n", idx);
#else
    printf("\tConnection index: %u\r\n", evt->conn_hdl);
#endif
    printf("\tAddress: %s %s\r\n",
           evt->remote_addr_type == PUBLIC_ADDRESS ? "public" : "private",
           ble_address_to_string(evt->remote_addr));

    conn_dev = malloc(sizeof(st_conn_dev_t));

    conn_dev->addr.type = evt->remote_addr_type;
    memcpy(conn_dev->addr.addr, evt->remote_addr, BLE_BD_ADDR_LEN);
    conn_dev->conn_hdl = evt->conn_hdl;
    R_Queue_PushFront(&connections, (void *) conn_dev);

    if (BLE_GAP_INVALID_CONN_HDL == master_dev_conn_hdl)
    {
        master_dev_conn_hdl = evt->conn_hdl;
    }
    else
    {
        R_BLE_GAP_SetDataLen(evt->conn_hdl, CONN_MAX_PACKET_SIZE, CONN_MAX_TX_TIME);
    }

    print_connections();
}

static bool handle_disconnect (st_ble_gap_disconn_evt_t * evt)
{
    st_conn_dev_t * conn_dev  = R_Queue_Remove(&connections, list_elem_match, &evt->conn_hdl);
    bool            start_adv = false;
#if CONN_WORKAROUND
    uint8_t idx;

    idx = get_index_for_handle(evt->conn_hdl);
    connected_device_info[idx].conn_hdl = BLE_GAP_INVALID_CONN_HDL;
#endif

    if (conn_dev)
    {
        printf("Device disconnected\r\n");
#if CONN_WORKAROUND
        printf("\tConnection index: %u\r\n", idx);
#else
        printf("\tConnection index: %u\r\n", evt->conn_hdl);
#endif
        printf("\tBD address of disconnected device: %s, %s\r\n",
               conn_dev->addr.type == PUBLIC_ADDRESS ? "public" : "private",
               ble_address_to_string(conn_dev->addr.addr));
        printf("\tReason of disconnection: 0x%02x\r\n", evt->reason);

        free(conn_dev);
    }

    print_connections();

    if (master_dev_conn_hdl == evt->conn_hdl)
    {
        master_dev_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
        printf("Advertising is on again\r\n");
        start_adv = true;
    }

    return start_adv;
}

static bool handle_conn_cancel_comp (ble_status_t result)
{
    bool start_adv = false;

    if (BLE_SUCCESS == result)
    {
        if (CONN_CANCEL_REASON_DEV_NOT_RESPONDING == conn_canceled_reason)
        {
            fsp_err_t err;

            printf(
                "handle_evt_gap_conn_completed: Last connection request was canceled. Connecting to new device...\r\n");

            /* gs_conn_param has already the correct address */
            err = RM_BLE_ABS_CreateConnection(&g_ble_abs0_ctrl, &gs_conn_param);
            printf("handle_evt_gap_conn_completed: Status=%d\r\n", err);
            conn_canceled_reason = CONN_CANCEL_NO_REASON;
        }
    }
    else
    {
        if (BLE_GAP_INVALID_CONN_HDL == master_dev_conn_hdl)
        {
            printf("Undefined cancellation reason\r\n");
            start_adv = true;
        }
    }

    return start_adv;
}

static void handle_create_conn_comp (ble_status_t result)
{
    if (BLE_ERR_INVALID_OPERATION == result)
    {
        conn_canceled_reason = CONN_CANCEL_REASON_DEV_NOT_RESPONDING;
        R_BLE_GAP_CancelCreateConn();
    }
}

void R_BLE_ML_Init (void)
{
    R_Queue_Init(&connections);
}

#if AUTOTEST_ENABLE
static void handle_data_length_change (st_ble_gap_data_len_chg_evt_t * evt)
{
    AUTOTEST_PRINT("Data length changed\r\n");
 #if CONN_WORKAROUND
    AUTOTEST_PRINT("\tConnection index: %d\r\n", get_index_for_handle(evt->conn_hdl));
 #else
    AUTOTEST_PRINT("\tConnection index: %d\r\n", evt->conn_hdl);
 #endif
    AUTOTEST_PRINT("\tMaximum RX data length: %d\r\n", evt->rx_octets);
    AUTOTEST_PRINT("\tMaximum RX time: %d\r\n", evt->rx_time);
    AUTOTEST_PRINT("\tMaximum TX data length: %d\r\n", evt->tx_octets);
    AUTOTEST_PRINT("\tMaximum TX time: %d\r\n", evt->tx_time);
}

static void handle_security_level (uint16_t conn_hdl, st_ble_gap_auth_info_t * sec_info)
{
    uint8_t level = 0;

    if (sec_info->security == NO_MITM)
    {
        level = 1;
    }
    else
    {
        if (sec_info->ekey_size == 16)
        {
            level = 3;
        }
        else
        {
            level = 2;
        }
    }

    AUTOTEST_PRINT("Security level changed\r\n");
 #if CONN_WORKAROUND
    AUTOTEST_PRINT("\tConnection index: %d\r\n", get_index_for_handle(conn_hdl));
 #else
    AUTOTEST_PRINT("\tConnection index: %d\r\n", conn_hdl);
 #endif
    AUTOTEST_PRINT("\tSecurity level: %u\r\n", level + 1);
}

static void handle_encryption_change (st_ble_gap_enc_chg_evt_t * evt)
{
    st_ble_gap_auth_info_t sec_info;
    int idx = get_index_for_handle(evt->conn_hdl);

    if (R_BLE_GAP_GetDevSecInfo(evt->conn_hdl, &sec_info) == BLE_SUCCESS)
    {
        handle_security_level(evt->conn_hdl, &sec_info);
        connected_device_info[idx].security_level_printed = true;
    }
    else
    {
        connected_device_info[idx].security_level_printed = false;
    }
}

static void handle_pair_completed (st_ble_gap_pairing_info_evt_t * evt, ble_status_t result)
{
    int idx = get_index_for_handle(evt->conn_hdl);
    if (!connected_device_info[idx].security_level_printed)
    {
        /* Security level needs to be handled here as bonding information is present in the stack here */
        handle_security_level(evt->conn_hdl, &(evt->auth_info));
        connected_device_info[idx].security_level_printed = true;
    }

    AUTOTEST_PRINT("Pair completed\r\n");
 #if CONN_WORKAROUND
    AUTOTEST_PRINT("\tConnection index: %d\r\n", get_index_for_handle(evt->conn_hdl));
 #else
    AUTOTEST_PRINT("\tConnection index: %d\r\n", evt->conn_hdl);
 #endif
    AUTOTEST_PRINT("\tStatus: 0x%02x\r\n", result);
    AUTOTEST_PRINT("\tBond: %s\r\n", evt->auth_info.bonding ? "true" : "false");
    AUTOTEST_PRINT("\tMITM: %s\r\n", (evt->auth_info.security == WITH_MITM) ? "true" : "false");
}

#endif

static ble_error_t translate_advertise_reason2status (uint8_t adv_reason)
{
    ble_status_t status = BLE_STATUS_OK;

    switch (adv_reason)
    {
        case 0x01:                     /* Advertising has been stopped by R_BLE_GAP_StopAdv() */
        {
            status = BLE_ERROR_CANCELED;
            break;
        }

        case 0x02:                     /* Because the duration specified by R_BLE_GAP_StartAdv() was expired */
        {
            status = BLE_ERROR_TIMEOUT;
            break;
        }

        case 0x03:                     /* Because the max_extd_adv_evts parameter specified by R_BLE_GAP_StartAdv() was reached */
        {
            status = BLE_ERROR_TIMEOUT;
            break;
        }

        case 0x04:                     /* Because the connection was established with the remote device, advertising has terminated */
        {
            status = BLE_STATUS_OK;
            break;
        }

        default:
        {
            break;
        }
    }

    return status;
}

static void handle_adv_off (st_ble_gap_adv_off_evt_t * evt)
{
    ble_error_t status = translate_advertise_reason2status(evt->reason);

    if (0x04 == evt->reason)
    {
        /* connection was set up */
        R_BLE_GAP_SetDataLen(evt->conn_hdl, CONN_MAX_PACKET_SIZE, CONN_MAX_TX_TIME);
    }

    printf("Advertising completed\r\n");
    printf("\tAdvertising type: 1\r\n");
    printf("\tCompletion status: 0x%02x\r\n", status);
}

static void handle_pair_req (st_ble_gap_pairing_info_evt_t * evt)
{
    printf("Pair request\r\n");
#if CONN_WORKAROUND
    printf("\tConnection index: %d\r\n", get_index_for_handle(evt->conn_hdl));
#else
    printf("\tConnection index: %d\r\n", evt->conn_hdl);
#endif
    printf("\tBond: %s\r\n", evt->auth_info.bonding ? "true" : "false");
}

bool R_BLE_ML_AbsGapCb (uint16_t type, ble_status_t result, st_ble_evt_data_t * data)
{
    bool event_handled = false;

    FSP_PARAMETER_NOT_USED(result);
    switch (type)
    {
#if CONN_WORKAROUND
        case BLE_GAP_EVENT_STACK_ON:
        {
            printf("Advertising is on\r\n");
            memset(connected_device_info, 0xFF, sizeof(connected_device_info));
            break;
        }
#endif
        case BLE_GAP_EVENT_CONN_IND:
        {
            st_ble_gap_conn_evt_t * evt = (st_ble_gap_conn_evt_t *) data->p_param;

            if (BLE_SUCCESS == result)
            {
                handle_connection_indication(evt);
            }

            break;
        }

        case BLE_GAP_EVENT_CREATE_CONN_COMP:
        {
            APP_PRINT(" %d", result);
            handle_create_conn_comp(result);
            break;
        }

        case BLE_GAP_EVENT_CONN_CANCEL_COMP:
        {
            if (!handle_conn_cancel_comp(result))
            {
                /* ADV should not be started */
                /* no default handling of BLE_GAP_EVENT_DISCONN_IND  */
                event_handled = true;
            }

            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            st_ble_gap_disconn_evt_t * evt = (st_ble_gap_disconn_evt_t *) data->p_param;

            if (!handle_disconnect(evt))
            {
                /* ADV should not be started */
                /* no default handling of BLE_GAP_EVENT_DISCONN_IND  */
                event_handled = true;
            }

            break;
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            st_ble_gap_adv_off_evt_t * evt = (st_ble_gap_adv_off_evt_t *) data->p_param;

            handle_adv_off(evt);
            break;
        }

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            st_ble_gap_pairing_info_evt_t * evt = (st_ble_gap_pairing_info_evt_t *) data->p_param;

            handle_pair_req(evt);
            break;
        }

#if AUTOTEST_ENABLE
        case BLE_GAP_EVENT_ENC_CHG:
        {
            st_ble_gap_enc_chg_evt_t * evt = (st_ble_gap_enc_chg_evt_t *) data->p_param;

            handle_encryption_change(evt);
            break;
        }

        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
            st_ble_gap_data_len_chg_evt_t * evt = (st_ble_gap_data_len_chg_evt_t *) data->p_param;

            handle_data_length_change(evt);
            break;
        }

        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            st_ble_gap_pairing_info_evt_t * evt = (st_ble_gap_pairing_info_evt_t *) data->p_param;

            handle_pair_completed(evt, result);
            break;
        }
#endif
        default:
        {
            break;
        }
    }

    return event_handled;
}
