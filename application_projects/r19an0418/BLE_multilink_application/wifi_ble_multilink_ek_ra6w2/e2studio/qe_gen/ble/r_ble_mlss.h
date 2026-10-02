/* 
* Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates 
* 
* SPDX-License-Identifier: BSD-3-Clause 
*/
/***********************************************************************************************************************
 * File Name: r_ble_mlss.h
 * Version : 1.0
 * Description : The header file for Multi-Link service.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup mlss Multi-Link Service
 * @{
 * @ingroup profile
 * @brief   
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_MLSS_H
#define R_BLE_MLSS_H

/*----------------------------------------------------------------------------------------------------------------------
    Peripheral Address Characteristic
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief Peripheral Address value structure.
*******************************************************************************/
typedef struct {
    uint8_t periph_addr[7]; /**< periph_addr */
} st_ble_mlss_periph_addr_t;

/*----------------------------------------------------------------------------------------------------------------------
    Multi-Link Service
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Multi-Link characteristic Index.
*******************************************************************************/
typedef enum {
    BLE_MLSS_PERIPH_ADDR_IDX,
} e_ble_mlss_char_idx_t;

/***************************************************************************//**
 * @brief Multi-Link event type.
*******************************************************************************/
typedef enum {
    /* Peripheral Address */
    BLE_MLSS_EVENT_PERIPH_ADDR_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_MLSS_PERIPH_ADDR_IDX, BLE_SERVS_WRITE_REQ),
    BLE_MLSS_EVENT_PERIPH_ADDR_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_MLSS_PERIPH_ADDR_IDX, BLE_SERVS_WRITE_COMP),
    BLE_MLSS_EVENT_PERIPH_ADDR_WRITE_CMD = BLE_SERVS_ATTR_EVENT(BLE_MLSS_PERIPH_ADDR_IDX, BLE_SERVS_WRITE_CMD),
} e_ble_mlss_event_t;

/***************************************************************************//**
 * @brief     Initialize Multi-Link service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_MLSS_Init(ble_servs_app_cb_t cb);

#endif /* R_BLE_MLSS_H */

/** @} */
