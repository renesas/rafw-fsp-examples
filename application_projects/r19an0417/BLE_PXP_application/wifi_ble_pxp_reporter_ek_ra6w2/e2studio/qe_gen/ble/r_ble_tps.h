/*
 * Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * File Name: r_ble_tps.h
 * Version : 1.0
 * Description : The header file for Tx Power Service service.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup tps Tx Power Service Service
 * @{
 * @ingroup profile
 * @brief   This service exposes a device's current transmit power level when in a connection.
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_TPS_H
 #define R_BLE_TPS_H

/*----------------------------------------------------------------------------------------------------------------------
 *  Tx Power Level Characteristic : The Transmit Power Level characteristic represents the current transmit power level in dBm,and the level ranges from -100 dBm to +20 dBm to a resolution of 1 dBm.
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief     Set Tx Power Level characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_TPS_SetTxPowerLevel(const int8_t * p_value);

/***************************************************************************//**
 * @brief     Get Tx Power Level characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_TPS_GetTxPowerLevel(int8_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  Tx Power Service Service
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Tx Power Service characteristic Index.
 *******************************************************************************/
typedef enum
{
    BLE_TPS_TX_POWER_LEVEL_IDX,
} e_ble_tps_char_idx_t;

/***************************************************************************//**
 * @brief Tx Power Service event type.
 *******************************************************************************/
typedef enum
{
    /* Tx Power Level */
    BLE_TPS_EVENT_TX_POWER_LEVEL_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_TPS_TX_POWER_LEVEL_IDX, BLE_SERVS_READ_REQ),
} e_ble_tps_event_t;

/***************************************************************************//**
 * @brief     Initialize Tx Power Service service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_TPS_Init(ble_servs_app_cb_t cb);

#endif                                 /* R_BLE_TPS_H */

/** @} */
