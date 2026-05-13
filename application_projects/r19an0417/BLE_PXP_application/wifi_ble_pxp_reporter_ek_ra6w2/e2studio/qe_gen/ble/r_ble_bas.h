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
 * Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * File Name: r_ble_bas.h
 * Version : 1.0
 * Description : The header file for Battery Service service.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup bas Battery Service Server
 * @{
 * @ingroup profile
 * @brief   The Battery Service exposes the state of a battery within a device.
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_BAS_H
 #define R_BLE_BAS_H

/*----------------------------------------------------------------------------------------------------------------------
 *  Battery Level Characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic Presentation Format value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t  format;                   /**< Format */
    int8_t   exponent;                 /**< Exponent */
    uint16_t unit;                     /**< Unit */
    uint8_t  namespace;                /**< Namespace */
    uint16_t description;              /**< Description */
} st_ble_bas_battery_level_presn_format_t;

/***************************************************************************//**
 * @brief     Set Battery Level characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_SetBatteryLevel(const uint8_t * p_value);

/***************************************************************************//**
 * @brief     Get Battery Level characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_GetBatteryLevel(uint8_t * p_value);

/***************************************************************************//**
 * @brief     Send notification of  Battery Level characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_NotifyBatteryLevel(uint16_t conn_hdl, const uint8_t * p_value);

/***************************************************************************//**
 * @brief     Set Battery Level cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_SetBatteryLevelCliCnfg(uint16_t conn_hdl, const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Get Battery Level cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_GetBatteryLevelCliCnfg(uint16_t conn_hdl, uint16_t * p_value);

/***************************************************************************//**
 * @brief     Set Battery Level presn format descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_SetBatteryLevelPresnFormat(const st_ble_bas_battery_level_presn_format_t * p_value);

/***************************************************************************//**
 * @brief     Get Battery Level presn format descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_GetBatteryLevelPresnFormat(st_ble_bas_battery_level_presn_format_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  Battery Service Service
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Battery Service characteristic Index.
 *******************************************************************************/
typedef enum
{
    BLE_BAS_BATTERY_LEVEL_IDX,
    BLE_BAS_BATTERY_LEVEL_CLI_CNFG_IDX,
    BLE_BAS_BATTERY_LEVEL_PRESN_FORMAT_IDX,
} e_ble_bas_char_idx_t;

/***************************************************************************//**
 * @brief Battery Service event type.
 *******************************************************************************/
typedef enum
{
    /* Battery Level */
    BLE_BAS_EVENT_BATTERY_LEVEL_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_BAS_BATTERY_LEVEL_IDX,
                                                                BLE_SERVS_READ_REQ),
    BLE_BAS_EVENT_BATTERY_LEVEL_CLI_CNFG_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_BAS_BATTERY_LEVEL_CLI_CNFG_IDX,
                                                                          BLE_SERVS_WRITE_REQ),
    BLE_BAS_EVENT_BATTERY_LEVEL_CLI_CNFG_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_BAS_BATTERY_LEVEL_CLI_CNFG_IDX,
                                                                           BLE_SERVS_WRITE_COMP),
    BLE_BAS_EVENT_BATTERY_LEVEL_CLI_CNFG_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_BAS_BATTERY_LEVEL_CLI_CNFG_IDX,
                                                                         BLE_SERVS_READ_REQ),
    BLE_BAS_EVENT_BATTERY_LEVEL_PRESN_FORMAT_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_BAS_BATTERY_LEVEL_PRESN_FORMAT_IDX,
                                                                             BLE_SERVS_READ_REQ),
} e_ble_bas_event_t;

/***************************************************************************//**
 * @brief     Initialize Battery Service service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_BAS_Init(ble_servs_app_cb_t cb);

#endif                                 /* R_BLE_BAS_H */

/** @} */
