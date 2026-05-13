/***********************************************************************************************************************
 * File Name    : app_awsiot_ota.h
 * Description  : Declarations for helper functions for aws ota feature support.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
 
#ifndef __APP_AWSIOT_OTA_H__
#define __APP_WSIOT_OTA_H__

#include "sdk_defs.h"

/// Operation step of process
typedef enum {
    /// Init value
        AWSIOT_OTA_INIT,
    /// RTOS
        AWSIOT_OTA_RTOS,
    /// BLE firmware, for DA166x
        AWSIOT_OTA_BLE_FW,
    /// RTOS and BLE firmware, for DA166x
        AWSIOT_OTA_BLE_COMBO,
    /// MCU firmware, not DA16x
        AWSIOT_OTA_MCU_FW,
    /// Certificate or Key
        AWSIOT_OTA_CERT_KEY,
    /// APP_CORE
    /// MCU firmware by Stream, not DA16x
        AWSIOT_OTA_MCU_FW_STREAM,
    /// Unknown value
        AWSIOT_OTA_UNKNOWN
} awsiot_ota_update_type;

/**
 ****************************************************************************************
 * @brief The OTA process initialization.
 * @param[in] fw_type OTA_UPDATE_TYPE.
 * @param[in] len OTA image length.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT app_awsiot_ota_init(UINT fw_type, UINT64 len);

/**
 ****************************************************************************************
 * @brief The The OTA process begins. If the firmware download is successful, the boot_idx is changed automatically and rebooted with the new firmware.
 * @param[in] rev_data Pointer of OTA_UPDATE_CONFIG.
 * @param[in] rev_data_len Received data length.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT app_awsiot_ota_download(UCHAR *rev_data, UINT rev_data_len);

/**
 ****************************************************************************************
 * @brief Change boot_idx to the index of the downloaded firmware and reboot. This function is already included in ota_update_start_download.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT app_awsiot_ota_renew(void);

#if defined (__IMG_UPDATE_BY_MCU__)
UINT awsiot_ota_update_by_mcu_init(UINT fw_type, UINT len);
UINT awsiot_ota_update_by_mcu_download(UCHAR *rev_data, UINT rev_data_len);
#endif
#endif  // __APP_AWSIOT_OTA_H__
