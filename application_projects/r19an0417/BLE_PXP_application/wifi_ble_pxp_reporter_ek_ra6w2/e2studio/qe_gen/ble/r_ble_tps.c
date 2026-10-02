/*
 * Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * File Name: r_ble_tps.c
 * Version : 1.0
 * Description : The source file for Tx Power Service service.
 **********************************************************************************************************************/

#include "r_ble_tps.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

static st_ble_servs_info_t gs_servs_info;

/*----------------------------------------------------------------------------------------------------------------------
 *  Tx Power Level characteristic : The Transmit Power Level characteristic represents the current transmit power level in dBm,and the level ranges from -100 dBm to +20 dBm to a resolution of 1 dBm.
 * ----------------------------------------------------------------------------------------------------------------------*/

/* Tx Power Level characteristic definition */
static const st_ble_servs_char_info_t gs_tx_power_level_char =
{
    .start_hdl = BLE_TPS_TX_POWER_LEVEL_DECL_HDL,
    .end_hdl   = BLE_TPS_TX_POWER_LEVEL_VAL_HDL,
    .char_idx  = BLE_TPS_TX_POWER_LEVEL_IDX,
    .app_size  = sizeof(int8_t),
    .db_size   = BLE_TPS_TX_POWER_LEVEL_LEN,
    .decode    = (ble_servs_attr_decode_t) decode_int8_t,
    .encode    = (ble_servs_attr_encode_t) encode_int8_t,
};

ble_status_t R_BLE_TPS_SetTxPowerLevel (const int8_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_tx_power_level_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_TPS_GetTxPowerLevel (int8_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_tx_power_level_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Tx Power Service server
 * ----------------------------------------------------------------------------------------------------------------------*/

/* Tx Power Service characteristics definition */
static const st_ble_servs_char_info_t * gspp_chars[] =
{
    &gs_tx_power_level_char,
};

/* Tx Power Service service definition */
static st_ble_servs_info_t gs_servs_info =
{
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_TPS_Init (ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}
