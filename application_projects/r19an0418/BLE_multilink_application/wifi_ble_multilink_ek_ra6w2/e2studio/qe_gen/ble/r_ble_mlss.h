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
* Copyright (C) 2019-2020 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
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
