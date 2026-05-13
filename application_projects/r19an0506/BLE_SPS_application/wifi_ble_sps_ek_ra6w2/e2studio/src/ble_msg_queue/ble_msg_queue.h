/***********************************************************************************************************************
 * File Name    : rm_ble_msg_queue.h
 * Description  : BLE MSG Queue
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef RM_BLE_MSG_QUEUE_H
#define RM_BLE_MSG_QUEUE_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "fsp_common_api.h"
#include "rm_comms_api.h"
#include "r_ble_sps_services.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

typedef enum e_rm_ble_msg_type
{
    RM_BLE_MSG_TYPE_TX_NOTIFY,
    RM_BLE_MSG_TYPE_FLOW_CTRL_NOTIFY,
    RM_BLE_MSG_TYPE_SVC_START,
    RM_BLE_MSG_TYPE_SVC_STOP,
    RM_BLE_MSG_TYPE_ADV_START,
    RM_BLE_MSG_TYPE_ADV_STOP,
    RM_BLE_MSG_TYPE_DISCONNECT,
    RM_BLE_MSG_TYPE_BD_ADDR_GET,
} rm_ble_msg_type_t;

typedef union u_rm_ble_msg_data
{
    st_ble_sps_services_sps_server_tx_data_t tx_data;
    uint8_t flow_ctrl_data;
} rm_ble_msg_data_t;

typedef struct st_rm_ble_msg
{
    rm_ble_msg_type_t type;
    rm_ble_msg_data_t data;
} rm_ble_msg_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/

fsp_err_t RM_BLE_MSG_QUEUE_Init(void);
fsp_err_t RM_BLE_MSG_QUEUE_Enqueue(const rm_ble_msg_t * p_msg);
fsp_err_t RM_BLE_MSG_QUEUE_Push(const rm_ble_msg_t * p_msg);
fsp_err_t RM_BLE_MSG_QUEUE_Dequeue(rm_ble_msg_t * p_msg);
fsp_err_t RM_BLE_MSG_QUEUE_Uninit(void);

FSP_FOOTER

#endif                                 /* RM_BLE_MSG_QUEUE */
