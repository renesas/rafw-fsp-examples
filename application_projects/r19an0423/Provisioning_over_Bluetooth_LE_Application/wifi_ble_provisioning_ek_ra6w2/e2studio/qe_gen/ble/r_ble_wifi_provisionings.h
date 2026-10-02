/* 
* Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates 
* 
* SPDX-License-Identifier: BSD-3-Clause 
*/
/***********************************************************************************************************************
 * File Name: r_ble_wifi_provisionings.h
 * Version : 1.0
 * Description : The header file for wifi_provisioning service.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup wifi_provisionings wifi_provisioning Service
 * @{
 * @ingroup profile
 * @brief   
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_WIFI_PROVISIONINGS_H
#define R_BLE_WIFI_PROVISIONINGS_H

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Command Characteristic
        Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
*******************************************************************************/
typedef struct {
    char user_description[10]; /**< User Description */
} st_ble_wifi_provisionings_wifi_command_char_user_description_t;

/***************************************************************************//**
 * @brief WiFi Command value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifi_provisionings_wifi_command_t;

/***************************************************************************//**
 * @brief     Set WiFi Command char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiCommandCharUserDescription(const st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Command char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiCommandCharUserDescription(st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Status Characteristic
        Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
        Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/


/***************************************************************************//**
 * @brief Characteristic User Description value structure.
*******************************************************************************/
typedef struct {
    char user_description[22]; /**< User Description */
} st_ble_wifi_provisionings_wifi_status_char_user_description_t;

/***************************************************************************//**
 * @brief     Set WiFi Status characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatus(const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Status characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatus(uint16_t *p_value);

/***************************************************************************//**
 * @brief     Send notification of  WiFi Status characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(uint16_t conn_hdl, const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Set WiFi Status cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatusCliCnfg(uint16_t conn_hdl, const uint16_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Status cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatusCliCnfg(uint16_t conn_hdl, uint16_t *p_value);

/***************************************************************************//**
 * @brief     Set WiFi Status char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatusCharUserDescription(const st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Status char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatusCharUserDescription(st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Output Characteristic
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief WiFi Output value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifi_provisionings_wifi_output_t;

/***************************************************************************//**
 * @brief     Set WiFi Output characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiOutput(const st_ble_wifi_provisionings_wifi_output_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Output characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiOutput(st_ble_wifi_provisionings_wifi_output_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Provisioning Characteristic
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief WiFi Provisioning value structure.
*******************************************************************************/
typedef struct {
    char value[192]; /**< value */
} st_ble_wifi_provisionings_wifi_provisioning_t;

/***************************************************************************//**
 * @brief     Set WiFi Provisioning characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning(const st_ble_wifi_provisionings_wifi_provisioning_t *p_value);

/***************************************************************************//**
 * @brief     Get WiFi Provisioning characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiProvisioning(st_ble_wifi_provisionings_wifi_provisioning_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    wifi_provisioning Service
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief wifi_provisioning characteristic Index.
*******************************************************************************/
typedef enum {
    BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_STATUS_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_IDX,
    BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_IDX,
} e_ble_wifi_provisionings_char_idx_t;

/***************************************************************************//**
 * @brief wifi_provisioning event type.
*******************************************************************************/
typedef enum {
    /* WiFi Command */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_COMMAND_CHAR_USER_DESCRIPTION_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_READ_REQ),
    /* WiFi Status */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_IDX, BLE_SERVS_READ_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CLI_CNFG_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_IDX, BLE_SERVS_READ_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_STATUS_CHAR_USER_DESCRIPTION_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_IDX, BLE_SERVS_READ_REQ),
    /* WiFi Output */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_OUTPUT_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_IDX, BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_OUTPUT_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_IDX, BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_OUTPUT_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_IDX, BLE_SERVS_READ_REQ),
    /* WiFi Provisioning */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFI_PROVISIONING_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_IDX, BLE_SERVS_READ_REQ),
} e_ble_wifi_provisionings_event_t;

/***************************************************************************//**
 * @brief     Initialize wifi_provisioning service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_Init(ble_servs_app_cb_t cb);

#endif /* R_BLE_WIFI_PROVISIONINGS_H */

/** @} */
