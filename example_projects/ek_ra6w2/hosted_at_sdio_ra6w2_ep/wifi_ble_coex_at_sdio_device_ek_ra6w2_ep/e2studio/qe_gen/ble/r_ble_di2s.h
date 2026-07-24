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
 * File Name: r_ble_di2s.h
 * Version : 1.0
 * Description : The header file for Device Information Service2 service.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version Description
 *         : 31.12.2999 1.00 First Release
 ***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @file
 * @defgroup di2s Device Information Service2 Service
 * @{
 * @ingroup profile
 * @brief   The Device Information Service exposes manufacturer and/or vendor information about a device.
 **********************************************************************************************************************/
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

#ifndef R_BLE_DI2S_H
#define R_BLE_DI2S_H

/*----------------------------------------------------------------------------------------------------------------------
    Manufacturer Name String Characteristic : Thise value of this characteristic is a UTF-8 string representing the name of the manufacturer of the device.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Manufacturer Name String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetMfrName(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Manufacturer Name String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetMfrName(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Model Number String Characteristic : The value of this characteristic is a UTF-8 string representing the model number assigned by the device vendor.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Model Number String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetModelNum(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Model Number String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetModelNum(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Serial Number String Characteristic : The value of this characteristic is a variable-length UTF-8 string representing the serial number for a particular instance of the device.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Serial Number String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetSerNum(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Serial Number String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetSerNum(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Hardware Revision String Characteristic : This characteristic represents the hardware revision for the hardware within the device.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Hardware Revision String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetHwRev(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Hardware Revision String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetHwRev(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Firmware Revision String Characteristic : This characteristic represents the firmware revision for the firmware within the device.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Firmware Revision String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetFirmRev(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Firmware Revision String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetFirmRev(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Software Revision String Characteristic : The value of this characteristic is a UTF-8 string representing the software revision for the software within the device.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set Software Revision String characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetSwRev(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get Software Revision String characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetSwRev(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    System ID Characteristic : This characteristic represents a structure containing an Organizationally Unique Identifier (OUI) followed by a manufacturer-defined identifier and is unique for each individual instance of the product.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief System ID value structure.
*******************************************************************************/
typedef struct
{
    uint8_t manufacturer_identifier[5]; /**< Manufacturer Identifier */
    uint8_t organizationally_unique_identifier[3]; /**< Organizationally Unique Identifier */
} st_ble_di2s_sys_id_t;

/***************************************************************************//**
 * @brief     Set System ID characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetSysId(const st_ble_di2s_sys_id_t *p_value);

/***************************************************************************//**
 * @brief     Get System ID characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetSysId(st_ble_di2s_sys_id_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    IEEE 11073-20601 Regulatory Certification Data List Characteristic : The value of the characteristic is an opaque structure listing various regulatory and/or certification compliance items to which the device claims adherence.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief     Set IEEE 11073-20601 Regulatory Certification Data List characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetRegCerDataList(const st_ble_seq_data_t *p_value);

/***************************************************************************//**
 * @brief     Get IEEE 11073-20601 Regulatory Certification Data List characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetRegCerDataList(st_ble_seq_data_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    PnP ID Characteristic : The PnP_ID characteristic returns its value when read using the GATT Characteristic Value Read procedure.
----------------------------------------------------------------------------------------------------------------------*/
/***************************************************************************//**
 * @brief PnP ID Vendor ID Source enumeration.
*******************************************************************************/
typedef enum
{
    BLE_DI2S_PNP_ID_VENDOR_ID_SOURCE_BLUETOOTH_SIG_ASSIGNED_COMPANY_IDENTIFIER_VALUE_FROM_THE_ASSIGNED_NUMBERS_DOCUMENT = 1, /**< Bluetooth SIG assigned Company Identifier value from the Assigned Numbers document */
    BLE_DI2S_PNP_ID_VENDOR_ID_SOURCE_USB_IMPLEMENTER_S_FORUM_ASSIGNED_VENDOR_ID_VALUE = 2, /**< USB Implementer's Forum assigned Vendor ID value */
} e_ble_di2s_pnp_id_vendor_id_source_t;

/***************************************************************************//**
 * @brief PnP ID value structure.
*******************************************************************************/
typedef struct
{
    uint8_t vendor_id_source; /**< Vendor ID Source */
    uint16_t vendor_id; /**< Vendor ID */
    uint16_t product_id; /**< Product ID */
    uint16_t product_version; /**< Product Version */
} st_ble_di2s_pnp_id_t;

/***************************************************************************//**
 * @brief     Set PnP ID characteristic value to the local GATT database.
 * @param[in] p_value  Characteristic value to set.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_SetPnpId(const st_ble_di2s_pnp_id_t *p_value);

/***************************************************************************//**
 * @brief     Get PnP ID characteristic value from the local GATT database.
 * @param[out] p_value  Output location for the acquired descriptor value.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_GetPnpId(st_ble_di2s_pnp_id_t *p_value);

/*----------------------------------------------------------------------------------------------------------------------
    Device Information Service2 Service
----------------------------------------------------------------------------------------------------------------------*/

/***************************************************************************//**
 * @brief Device Information Service2 characteristic Index.
*******************************************************************************/
typedef enum
{
    BLE_DI2S_MFR_NAME_IDX,
    BLE_DI2S_MODEL_NUM_IDX,
    BLE_DI2S_SER_NUM_IDX,
    BLE_DI2S_HW_REV_IDX,
    BLE_DI2S_FIRM_REV_IDX,
    BLE_DI2S_SW_REV_IDX,
    BLE_DI2S_SYS_ID_IDX,
    BLE_DI2S_REG_CER_DATA_LIST_IDX,
    BLE_DI2S_PNP_ID_IDX,
} e_ble_di2s_char_idx_t;

/***************************************************************************//**
 * @brief Device Information Service2 event type.
*******************************************************************************/
typedef enum
{
    /* Manufacturer Name String */
    BLE_DI2S_EVENT_MFR_NAME_WRITE_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_MFR_NAME_IDX, BLE_SERVS_WRITE_REQ),
    BLE_DI2S_EVENT_MFR_NAME_WRITE_COMP = BLE_SERVS_ATTR_EVENT(BLE_DI2S_MFR_NAME_IDX, BLE_SERVS_WRITE_COMP),
    BLE_DI2S_EVENT_MFR_NAME_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_MFR_NAME_IDX, BLE_SERVS_READ_REQ),
    /* Model Number String */
    BLE_DI2S_EVENT_MODEL_NUM_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_MODEL_NUM_IDX, BLE_SERVS_READ_REQ),
    /* Serial Number String */
    BLE_DI2S_EVENT_SER_NUM_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_SER_NUM_IDX, BLE_SERVS_READ_REQ),
    /* Hardware Revision String */
    BLE_DI2S_EVENT_HW_REV_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_HW_REV_IDX, BLE_SERVS_READ_REQ),
    /* Firmware Revision String */
    BLE_DI2S_EVENT_FIRM_REV_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_FIRM_REV_IDX, BLE_SERVS_READ_REQ),
    /* Software Revision String */
    BLE_DI2S_EVENT_SW_REV_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_SW_REV_IDX, BLE_SERVS_READ_REQ),
    /* System ID */
    BLE_DI2S_EVENT_SYS_ID_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_SYS_ID_IDX, BLE_SERVS_READ_REQ),
    /* IEEE 11073-20601 Regulatory Certification Data List */
    BLE_DI2S_EVENT_REG_CER_DATA_LIST_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_REG_CER_DATA_LIST_IDX, BLE_SERVS_READ_REQ),
    /* PnP ID */
    BLE_DI2S_EVENT_PNP_ID_READ_REQ = BLE_SERVS_ATTR_EVENT(BLE_DI2S_PNP_ID_IDX, BLE_SERVS_READ_REQ),
} e_ble_di2s_event_t;

/***************************************************************************//**
 * @brief     Initialize Device Information Service2 service.
 * @param[in] cb Service callback.
 * @return    @ref ble_status_t
*******************************************************************************/
ble_status_t R_BLE_DI2S_Init(ble_servs_app_cb_t cb);

#endif /* R_BLE_DI2S_H */

/** @} */
