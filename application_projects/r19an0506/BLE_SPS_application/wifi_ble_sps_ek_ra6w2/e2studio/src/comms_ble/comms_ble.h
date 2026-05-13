/***********************************************************************************************************************
 * File Name    : rm_comms_ble.h
 * Description  : BLE Comms implementation
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef RM_COMMS_BLE_H
#define RM_COMMS_BLE_H

#define RM_COMMS_BLE_RX_BUFF_SIZE    5120

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "rm_comms_api.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/* BLE event type. */
typedef enum e_rm_comms_ble_event
{
    RM_COMMS_BLE_EVENT_DATA_AVAILABLE,
    RM_COMMS_BLE_EVENT_TX_COMPLETE,
    RM_COMMS_BLE_EVENT_TX_ERROR,
    RM_COMMS_BLE_EVENT_FLOW_CTRL_ERROR,
} rm_comms_ble_event_t;

/* BLE serial adapter configuration */
typedef struct st_rm_comms_ble_extended_cfg
{
    void * const   p_tx_mutex;         // Lock device for writing.
    void * const   p_rx_mutex;         // Lock device for reading.
    uint32_t const mutex_timeout;      // Timeout for locking device.
    void * const   p_tx_semaphore;     // Block write operations. If this is NULL then operations will be non-blocking and require a callback.
    void * const   p_rx_semaphore;     // Block read operations. If this is NULL then operations will be non-blocking and require a callback.
} rm_comms_ble_extended_cfg_t;

/* Communications middleware control structure. */
typedef struct st_rm_comms_ble_instance_ctrl
{
    uint32_t                            open;               // Open flag.
    rm_comms_cfg_t const              * p_cfg;              // Middleware configuration.
    rm_comms_ble_extended_cfg_t const * p_extend;           // Pointer to extended configuration structure

    void (* p_callback)(rm_comms_callback_args_t * p_args); // Pointer to callback that is called when a uart_event_t occurs.
    void const * p_context;                                 // Pointer to context passed into callback function

    uint8_t   rx_buff[RM_COMMS_BLE_RX_BUFF_SIZE];
    uint32_t  rx_buff_head;
    uint32_t  rx_buff_tail;
    uint8_t * p_dest;
    uint8_t * p_src;
    uint32_t  rx_bytes_pending;
    uint32_t  tx_bytes_pending;
} rm_comms_ble_instance_ctrl_t;

/** Additional rm_comms_ble info. */
typedef struct st_rm_comms_ble_state_info
{
    uint32_t rx_buff_size;             ///< Internal buffer size.
    uint32_t rx_buff_len;              ///< Octets buffered.
} rm_comms_ble_state_info_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

extern rm_comms_api_t const g_comms_on_comms_ble;

/**********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_Open(rm_comms_ctrl_t * const p_api_ctrl, rm_comms_cfg_t const * const p_cfg);
fsp_err_t RM_COMMS_BLE_Close(rm_comms_ctrl_t * const p_api_ctrl);
fsp_err_t RM_COMMS_BLE_Read(rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_dest, uint32_t const bytes);
fsp_err_t RM_COMMS_BLE_Write(rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_src, uint32_t const bytes);
fsp_err_t RM_COMMS_BLE_WriteRead(rm_comms_ctrl_t * const            p_api_ctrl,
                                 rm_comms_write_read_params_t const write_read_params);
fsp_err_t RM_COMMS_BLE_CallbackSet(rm_comms_ctrl_t * const p_api_ctrl,
                                   void (                * p_callback)(rm_comms_callback_args_t *),
                                   void const * const      p_context);

void rm_comms_ble_callback(rm_comms_ble_event_t event, const uint8_t * p_data, uint8_t len, void * p_context);

FSP_FOOTER

#endif                                 /* RM_COMMS_BLE_H */
