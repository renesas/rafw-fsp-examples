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
 *  WiFiCommand Characteristic
 *      Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
 *******************************************************************************/
typedef struct
{
    char user_description[10];         /**< User Description */
} st_ble_wifi_provisionings_wificommand_char_user_description_t;

/***************************************************************************//**
 * @brief WiFiCommand value structure.
 *******************************************************************************/
typedef struct
{
    char value[192];                   /**< value */
} st_ble_wifi_provisionings_wificommand_t;

/***************************************************************************//**
 * @brief     Set WiFiCommand char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWificommandCharUserDescription(
    const st_ble_wifi_provisionings_wificommand_char_user_description_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiCommand char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWificommandCharUserDescription(
    st_ble_wifi_provisionings_wificommand_char_user_description_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiStatus Characteristic
 *      Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 *      Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Characteristic User Description value structure.
 *******************************************************************************/
typedef struct
{
    char user_description[22];         /**< User Description */
} st_ble_wifi_provisionings_wifistatus_char_user_description_t;

/***************************************************************************//**
 * @brief     Set WiFiStatus characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatus(const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatus(uint16_t * p_value);

/***************************************************************************//**
 * @brief     Send notification of  WiFiStatus characteristic value to the remote device.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Characteristic value to send.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_NotifyWifistatus(uint16_t conn_hdl, const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Set WiFiStatus cli cnfg descriptor value to the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatusCliCnfg(uint16_t conn_hdl, const uint16_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus cli cnfg descriptor value from the local GATT database.
 * @param[in] conn_hdl Connection handle.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatusCliCnfg(uint16_t conn_hdl, uint16_t * p_value);

/***************************************************************************//**
 * @brief     Set WiFiStatus char user description descriptor value to the local GATT database.
 * @param[in] p_value  Descriptor value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatusCharUserDescription(
    const st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiStatus char user description descriptor value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatusCharUserDescription(
    st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiOutput Characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief WiFiOutput value structure.
 *******************************************************************************/
typedef struct
{
    char value[192];                   /**< value */
} st_ble_wifi_provisionings_wifioutput_t;

/***************************************************************************//**
 * @brief     Set WiFiOutput characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifioutput(const st_ble_wifi_provisionings_wifioutput_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiOutput characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifioutput(st_ble_wifi_provisionings_wifioutput_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiProvisioning Characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief WiFiProvisioning value structure.
 *******************************************************************************/
typedef struct
{
    char value[192];                   /**< value */
} st_ble_wifi_provisionings_wifiprovisioning_t;

/***************************************************************************//**
 * @brief     Set WiFiProvisioning characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiprovisioning(const st_ble_wifi_provisionings_wifiprovisioning_t * p_value);

/***************************************************************************//**
 * @brief     Get WiFiProvisioning characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiprovisioning(st_ble_wifi_provisionings_wifiprovisioning_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  ThingName Characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief ThingName value structure.
 *******************************************************************************/
typedef struct
{
    char value[192];                   /**< value */
} st_ble_wifi_provisionings_thingname_t;

/***************************************************************************//**
 * @brief     Set ThingName characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_SetThingname(const st_ble_wifi_provisionings_thingname_t * p_value);

/***************************************************************************//**
 * @brief     Get ThingName characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_GetThingname(st_ble_wifi_provisionings_thingname_t * p_value);

/*----------------------------------------------------------------------------------------------------------------------
 *  wifi_provisioning Service
 * ----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief wifi_provisioning characteristic Index.
 *******************************************************************************/
typedef enum
{
    BLE_WIFI_PROVISIONINGS_WIFICOMMAND_IDX,
    BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFI_PROVISIONINGS_WIFISTATUS_IDX,
    BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
    BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
    BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_IDX,
    BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_IDX,
    BLE_WIFI_PROVISIONINGS_THINGNAME_IDX,
} e_ble_wifi_provisionings_char_idx_t;

/***************************************************************************//**
 * @brief wifi_provisioning event type.
 *******************************************************************************/
typedef enum
{
    /* WiFiCommand */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFICOMMAND_IDX,
                                                                              BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFICOMMAND_IDX,
                                                                               BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_READ_REQ),

    /* WiFiStatus */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_IDX,
        BLE_SERVS_READ_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
        BLE_SERVS_READ_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_WRITE_REQ),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_COMP = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_WRITE_COMP),
    BLE_WIFI_PROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
        BLE_SERVS_READ_REQ),

    /* WiFiOutput */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFIOUTPUT_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_IDX,
                                                                            BLE_SERVS_READ_REQ),

    /* WiFiProvisioning */
    BLE_WIFI_PROVISIONINGS_EVENT_WIFIPROVISIONING_READ_REQ = BLE_SERVS_ATTR_EVENT(
        BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_IDX,
        BLE_SERVS_READ_REQ),

    /* ThingName */
    BLE_WIFI_PROVISIONINGS_EVENT_THINGNAME_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_WIFI_PROVISIONINGS_THINGNAME_IDX,
                                                                           BLE_SERVS_READ_REQ),
} e_ble_wifi_provisionings_event_t;

/***************************************************************************//**
 * @brief     Initialize wifi_provisioning service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
 *******************************************************************************/
ble_status_t R_BLE_WIFI_PROVISIONINGS_Init(ble_servs_app_cb_t cb);

#endif                                 /* R_BLE_WIFI_PROVISIONINGS_H */

/** @} */
