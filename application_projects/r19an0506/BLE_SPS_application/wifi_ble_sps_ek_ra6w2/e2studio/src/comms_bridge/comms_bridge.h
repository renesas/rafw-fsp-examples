/***********************************************************************************************************************
 * File Name    : rm_comms_bridge.h
 * Description  : Comms Bridge implementation
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef RM_COMMS_BRIDGE_H
#define RM_COMMS_BRIDGE_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "rm_comms_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define RM_COMMS_BRIDGE_RX_BUFF_SIZE    10240

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/* BLE serial adapter configuration */
typedef struct st_rm_comms_bridge_cfg
{
    rm_comms_instance_t * comms_instance_a; /* Pointer to corresponding rm_comms_api instance */
    rm_comms_instance_t * comms_instance_b; /* Pointer to corresponding rm_comms_api instance */
} rm_comms_bridge_cfg_t;

/* Communications middleware control structure. */
typedef struct st_rm_comms_bridge_ctrl
{
    uint32_t open;                       // Open flag.
    rm_comms_bridge_cfg_t const * p_cfg; // Middleware configuration.

    uint8_t  buff_a[RM_COMMS_BRIDGE_RX_BUFF_SIZE];
    uint32_t head_a;
    uint32_t tail_a;
    uint32_t pending_bytes_a;
    uint8_t  buff_b[RM_COMMS_BRIDGE_RX_BUFF_SIZE];
    uint32_t head_b;
    uint32_t tail_b;
    uint32_t pending_bytes_b;

    TaskHandle_t      dispatch_task_a;
    SemaphoreHandle_t dispatch_sema_a;

    TaskHandle_t      dispatch_task_b;
    SemaphoreHandle_t dispatch_sema_b;
} rm_comms_bridge_instance_ctrl_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
extern volatile bool sps_flow_enabled;

/**********************************************************************************************************************
 * Public Function Prototypes
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BRIDGE_Open(rm_comms_bridge_instance_ctrl_t * const p_ctrl,
                               rm_comms_bridge_cfg_t const * const     p_cfg);
fsp_err_t RM_COMMS_BRIDGE_Close(rm_comms_bridge_instance_ctrl_t * const p_ctrl);

FSP_FOOTER

#endif                                 /* RM_COMMS_BRIDGE_H */
