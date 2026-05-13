/***********************************************************************************************************************
 * File Name: r_ble_ml.h
 * Description : Multi-link Application.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Copyright (c) 2019 - 2024 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Macro definitions
 ***********************************************************************************************************************/
#ifndef R_BLE_ML_H
#define R_BLE_ML_H

extern ble_abs_connection_parameter_t gs_conn_param;
extern ble_device_address_t           gs_conn_bd_addr;

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 ***********************************************************************************************************************/
void R_BLE_ML_Init(void);

/*******************************************************************************************************************//**
 * @fn void R_BLE_ML_AbsGapCb(uint16_t type, ble_status_t result, st_ble_evt_data_t *data)
 * @brief GAP event handler.
 * @details This function shall be called from GAP callback registered by
 *          @ref R_BLE_GAP_Init or @ref R_BLE_ABS_Init function.
 ***********************************************************************************************************************/
bool R_BLE_ML_AbsGapCb(uint16_t type, ble_status_t result, st_ble_evt_data_t * data);

void R_BLE_ML_SetPendingConnection(ble_device_address_t * addr);

void R_BLE_ML_SetConnectingState(bool state);

#endif                                 /* R_BLE_ML_H */
