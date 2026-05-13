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
* Copyright (c) 2026 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
 * File Name: r_ble_WiFiProvisionings.h
 * Version : 1.0
 * Description : The header file for WiFiProvisioning service.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup WiFiProvisionings WiFiProvisioning Service
 * @{
 * @ingroup profile
 * @brief
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_WIFIPROVISIONINGS_H
#define R_BLE_WIFIPROVISIONINGS_H

/*----------------------------------------------------------------------------------------------------------------------
    WiFiCommand Characteristic
        Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
*******************************************************************************/
typedef struct {
    char user_description[10]; /**< User Description */
} st_ble_wifiprovisionings_wificommand_char_user_description_t;

/***************************************************************************//**
 * @brief WiFiCommand value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifiprovisionings_wificommand_t;

/***************************************************************************//**
 * @brief     Set WiFiCommand char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWificommandCharUserDescription(
    const st_ble_wifiprovisionings_wificommand_char_user_description_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiCommand char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWificommandCharUserDescription(
    st_ble_wifiprovisionings_wificommand_char_user_description_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFiStatus Characteristic
        Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
        Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/


/***************************************************************************//**
 * @brief Characteristic User Description value structure.
*******************************************************************************/
typedef struct {
    char user_description[22]; /**< User Description */
} st_ble_wifiprovisionings_wifistatus_char_user_description_t;

/***************************************************************************//**
 * @brief     Set WiFiStatus characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatus(const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatus(uint16_t *p_value);

/***************************************************************************//**
 * @brief     Send notification of  WiFiStatus characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(
    uint16_t conn_hdl, const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Set WiFiStatus cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatusCliCnfg(
    uint16_t conn_hdl, const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatusCliCnfg(
    uint16_t conn_hdl, uint16_t *p_value);

/***************************************************************************//**
 * @brief     Set WiFiStatus char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatusCharUserDescription(
    const st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatusCharUserDescription(
    st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFiOutput Characteristic
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief WiFiOutput value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifiprovisionings_wifioutput_t;

/***************************************************************************//**
 * @brief     Set WiFiOutput characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifioutput(
    const st_ble_wifiprovisionings_wifioutput_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiOutput characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifioutput(
    st_ble_wifiprovisionings_wifioutput_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFiProvisoning Characteristic
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief WiFiProvisoning value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifiprovisionings_wifiprovisoning_t;

/***************************************************************************//**
 * @brief     Set WiFiProvisoning characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(
    const st_ble_wifiprovisionings_wifiprovisoning_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFiProvisoning characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifiprovisoning(
    st_ble_wifiprovisionings_wifiprovisoning_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFiProvisioning Service
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief WiFiProvisioning characteristic Index.
*******************************************************************************/
typedef enum {
    BLE_WIFIPROVISIONINGS_WIFICOMMAND_IDX,
    BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFIPROVISIONINGS_WIFISTATUS_IDX,
    BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
    BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFIPROVISIONINGS_WIFIOUTPUT_IDX,
    BLE_WIFIPROVISIONINGS_WIFIPROVISONING_IDX,
} e_ble_wifiprovisionings_char_idx_t;

/***************************************************************************//**
 * @brief WiFiProvisioning event type.
*******************************************************************************/
typedef enum {
    /* WiFiCommand */
    BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFICOMMAND_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_COMP =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFICOMMAND_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_READ_REQ),

    /* WiFiStatus */
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_IDX, BLE_SERVS_READ_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_HDL_VAL_CNF =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_IDX, BLE_SERVS_HDL_VAL_CNF),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_COMP =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX, BLE_SERVS_READ_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_COMP =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_READ_REQ),

    /* WiFiOutput */
    BLE_WIFIPROVISIONINGS_EVENT_WIFIOUTPUT_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFIOUTPUT_IDX, BLE_SERVS_READ_REQ),

    /* WiFiProvisoning */
    BLE_WIFIPROVISIONINGS_EVENT_WIFIPROVISONING_READ_REQ =
      BLE_SERVS_ATTR_EVENT(BLE_WIFIPROVISIONINGS_WIFIPROVISONING_IDX, BLE_SERVS_READ_REQ),
} e_ble_wifiprovisionings_event_t;

/***************************************************************************//**
 * @brief     Initialize WiFiProvisioning service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFIPROVISIONINGS_Init(ble_servs_app_cb_t cb);

#endif /* R_BLE_WIFIPROVISIONINGS_H */

/** @} */
