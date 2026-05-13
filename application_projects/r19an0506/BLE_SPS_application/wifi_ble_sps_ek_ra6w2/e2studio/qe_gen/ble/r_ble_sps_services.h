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
 * File Name: r_ble_sps_services.h
 * Version : 1.0
 * Description : The header file for SPS Service service.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup sps_services SPS Service Service
 * @{
 * @ingroup profile
 * @brief   SPS Service
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_SPS_SERVICES_H
 #define R_BLE_SPS_SERVICES_H

/*----------------------------------------------------------------------------------------------------------------------
 *  Server TX Data Characteristic : SPS Server TX Data
 *      Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 *      Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t user_description[14];      /**< User Description */
} st_ble_sps_services_sps_server_tx_data_char_user_desc_t;

/***************************************************************************//**
 * @brief Server TX Data value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t tx_data[250];              /**< tx_data */
    uint8_t tx_len;                    /**< tx_len */
} st_ble_sps_services_sps_server_tx_data_t;

/***************************************************************************//**
 * @brief     Send notification of  Server TX Data characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_NotifySps_server_tx_data(uint16_t                                         conn_hdl,
                                                         const st_ble_sps_services_sps_server_tx_data_t * p_value);

/***************************************************************************//**
 * @brief     Send indication of  Server TX Data characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_IndicateSps_server_tx_data(uint16_t                                         conn_hdl,
                                                           const st_ble_sps_services_sps_server_tx_data_t * p_value);

/***************************************************************************//**
 * @brief     Set Server TX Data cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_server_tx_dataCliCnfg(uint16_t conn_hdl, const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Get Server TX Data cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_server_tx_dataCliCnfg(uint16_t conn_hdl, uint16_t * p_value);

/***************************************************************************//**
 * @brief     Set Server TX Data char user desc descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_server_tx_dataCharUserDesc(
    const st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_value);

/***************************************************************************//**
 * @brief     Get Server TX Data char user desc descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_server_tx_dataCharUserDesc(
    st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  Server RX Data Characteristic : SPS Server RX Data
 *      Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t user_description[14];      /**< User Description */
} st_ble_sps_services_sps_server_rx_data_char_user_desc_t;

/***************************************************************************//**
 * @brief Server RX Data value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t rx_data[250];              /**< rx_data */
    uint8_t rx_len;                    /**< rx_len */
} st_ble_sps_services_sps_server_rx_data_t;

/***************************************************************************//**
 * @brief     Set Server RX Data char user desc descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_server_rx_dataCharUserDesc(
    const st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_value);

/***************************************************************************//**
 * @brief     Get Server RX Data char user desc descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_server_rx_dataCharUserDesc(
    st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  Flow Control Characteristic : SPS Flow Control
 *      Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 *      Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
 *******************************************************************************/
typedef struct
{
    uint8_t user_description[12];      /**< User Description */
} st_ble_sps_services_sps_flow_ctrl_char_user_desc_t;

/***************************************************************************//**
 * @brief Flow Control state enumeration.
 *******************************************************************************/
typedef enum
{
    BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_OFF = 1, /**<  */
    BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_ON  = 2, /**<  */
} e_ble_sps_services_sps_flow_ctrl_state_t;

/***************************************************************************//**
 * @brief     Set Flow Control characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrl(const uint8_t * p_value);

/***************************************************************************//**
 * @brief     Get Flow Control characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrl(uint8_t * p_value);

/***************************************************************************//**
 * @brief     Send notification of  Flow Control characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_NotifySps_flow_ctrl(uint16_t conn_hdl, const uint8_t * p_value);

/***************************************************************************//**
 * @brief     Set Flow Control cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrlCliCnfg(uint16_t conn_hdl, const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Get Flow Control cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrlCliCnfg(uint16_t conn_hdl, uint16_t * p_value);

/***************************************************************************//**
 * @brief     Set Flow Control char user desc descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrlCharUserDesc(
    const st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_value);

/***************************************************************************//**
 * @brief     Get Flow Control char user desc descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrlCharUserDesc(
    st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  SPS Service Service
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief SPS Service characteristic Index.
 *******************************************************************************/
typedef enum
{
    BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_IDX,
    BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_IDX,
    BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_IDX,
    BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_IDX,
    BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_IDX,
    BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
    BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_IDX,
    BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_IDX,
} e_ble_sps_services_char_idx_t;

/***************************************************************************//**
 * @brief SPS Service event type.
 *******************************************************************************/
typedef enum
{
    /* Server TX Data */
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_HDL_VAL_CNF = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_IDX,
        BLE_SERVS_HDL_VAL_CNF),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_CLI_CNFG_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_CLI_CNFG_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_CLI_CNFG_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_IDX,
        BLE_SERVS_READ_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_TX_DATA_CHAR_USER_DESC_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_IDX,
        BLE_SERVS_READ_REQ),

    /* Server RX Data */
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_WRITE_CMD = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_IDX,
        BLE_SERVS_WRITE_CMD),
    BLE_SPS_SERVICES_EVENT_SPS_SERVER_RX_DATA_CHAR_USER_DESC_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_IDX,
        BLE_SERVS_READ_REQ),

    /* Flow Control */
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_WRITE_CMD = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
        BLE_SERVS_WRITE_CMD),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
        BLE_SERVS_READ_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_CLI_CNFG_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_CLI_CNFG_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_CLI_CNFG_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_IDX,
        BLE_SERVS_READ_REQ),
    BLE_SPS_SERVICES_EVENT_SPS_FLOW_CTRL_CHAR_USER_DESC_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_IDX,
        BLE_SERVS_READ_REQ),
} e_ble_sps_services_event_t;

/***************************************************************************//**
 * @brief     Initialize SPS Service service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_SPS_SERVICES_Init(ble_servs_app_cb_t cb);

#endif                                 /* R_BLE_SPS_SERVICES_H */

/** @} */
