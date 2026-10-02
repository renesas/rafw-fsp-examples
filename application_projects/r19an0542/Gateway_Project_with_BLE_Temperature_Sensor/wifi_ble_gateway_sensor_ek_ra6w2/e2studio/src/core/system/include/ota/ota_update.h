/**
 ****************************************************************************************
 *
 * @file ota_update.h
 *
 * @brief Over the air firmware update module
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#if !defined (__OTA_UPDATE_H__)
#define	__OTA_UPDATE_H__

#if defined (__SUPPORT_OTA__)
#include <stdio.h>
//#include "application.h"

//#include "da16200_map.h"
//#include "da16x_image.h"
#if defined (__BLE_COMBO_REF__)
#include "da16_gtl_features.h"
#endif // __BLE_COMBO_REF__

/// Debug feature
#define	ENABLE_OTA_ERR
#define	ENABLE_OTA_INFO
#undef	ENABLE_OTA_DBG

#if defined (ENABLE_OTA_INFO)
#define OTA_INFO	PRINTF
#else
#define OTA_INFO(...)	do { } while (0);
#endif // (ENABLE_OTA_INFO)

#if defined (ENABLE_OTA_ERR)
#define OTA_ERR		PRINTF
#else
#define OTA_ERR(...)	do { } while (0);
#endif // (ENABLE_OTA_ERR)

#if defined (ENABLE_OTA_DBG)
#define OTA_DBG	PRINTF
#else
#define OTA_DBG(...)	do { } while (0);
#endif // (ENABLE_OTA_DBG)

/*******************************************************************/
/* OTA return code */
/*******************************************************************/
/// Return success.
#define OTA_SUCCESS					0x00
/// Return failed.
#define OTA_FAILED					0x01
/// Sflash address is wrong.
#define OTA_ERROR_SFLASH_ADDR		0x02
/// FW type is unknown.
#define OTA_ERROR_TYPE				0x03
/// Server URL is unknown.
#define OTA_ERROR_URL				0x04
/// FW size is wrong or offset address to be downloaded is wrong.
#define OTA_ERROR_SIZE				0x05
/// CRC is not correct.
#define OTA_ERROR_CRC				0x06
/// FW version is unknown.
#define OTA_VERSION_UNKNOWN			0x07
/// FW version is incompatible.
#define OTA_VERSION_INCOMPATI		0x08
/// Fw not found on the server.
#define OTA_NOT_FOUND				0x09
/// Failed to connect to server.
#define OTA_NOT_CONNECTED			0x0a
/// All new FWs have not been downloaded.
#define OTA_NOT_ALL_DOWNLOAD		0x0b
/// Failed to alloc memory.
#define OTA_MEM_ALLOC_FAILED		0x0c

/// BLE FW version is unknown.
#define OTA_BLE_VERSION_UNKNOWN		0xa1

// Chunked size is wrong. (Customer Features)
#define OTA_CHUNKED_SIZE_ERROR		0xf1
/*******************************************************************/

/// SFLASH address definition
#define SFLASH_UNKNOWN_ADDRESS 		0xFFFFFFFF
#define FLASH_ADDR_USER_DW_START	SFLASH_USER_AREA_1_START
#define FLASH_ADDR_USER_DW_END		SFLASH_USER_AREA_1_END


#define OTA_RTOS_NAME				"RTOS"
#define OTA_MCU_FW_NAME				"MCU_FW"
#define OTA_BLE_FW_NAME				"BLE_FW"
#define OTA_BLE_COMBO_NAME			"BLE_COMBO"
#define OTA_CERT_KEY_NAME			"CERT_KEY"
//APP_CORE
#define OTA_STR_MCU_STREAM			"MCU_STRM"

#define OTA_HTTP_URL_LEN			(256)


/// Operation step of process
typedef enum {
    /// Init value
    OTA_TYPE_INIT,
    /// RTOS
    OTA_TYPE_RTOS,
    /// BLE firmware, for DA166x
    OTA_TYPE_BLE_FW,
    /// RTOS and BLE firmware, for DA166x
    OTA_TYPE_BLE_COMBO,
    /// MCU firmware, not DA16x
    OTA_TYPE_MCU_FW,
    /// Certificate or Key
    OTA_TYPE_CERT_KEY,
    //APP_CORE
    /// MCU firmware by Stream, not DA16x
    OTA_TYPE_MCU_FW_STREAM,
    /// Unknown value
    OTA_TYPE_UNKNOWN
} ota_update_type;

/// OTA update configuration structure
typedef struct {
    /// Update type.
    ota_update_type	update_type;
    /// Server address where firmware exists.
    char	url[OTA_HTTP_URL_LEN];
    /// Callback function pointer to check the download status.
    void 	(*download_notify)(ota_update_type update_type, UINT ret_status, UINT progress);
    /// Callback function pointer to check the renew state. Only for RTOS.
    void 	(*renew_notify)(UINT ret_status);
    /// If the value is true, if the new firmware download is successful, it will reboot with the new firmware. Only for RTOS
    UINT 	auto_renew;
    /// Address of sflash where other_fw is stored. Only for MCU_FW and CERT_KEY
    UINT 	download_sflash_addr;
#if defined (__BLE_COMBO_REF__)
    /// Server address where firmware exists.
    char	url_ble_fw[OTA_HTTP_URL_LEN];
#endif /* __BLE_COMBO_REF__  */
} OTA_UPDATE_CONFIG;


/**
 ****************************************************************************************
 * @brief The The OTA process begins. If the firmware download is successful, the boot_idx is changed automatically and rebooted with the new firmware.
 * @param[in] fw_conf Pointer of OTA_UPDATE_CONFIG.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_start_download(OTA_UPDATE_CONFIG *ota_update_conf);

/**
 ****************************************************************************************
 * @brief The firmware download will stop.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_stop_download(void);

/**
 ****************************************************************************************
 * @brief Change boot_idx to the index of the downloaded firmware and reboot. This function is already included in ota_update_start_download.
 * @param[in] fw_conf Pointer of OTA_UPDATE_CONFIG.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_start_renew(OTA_UPDATE_CONFIG *ota_update_conf);

/**
 ****************************************************************************************
 * @brief Download progress is returned as a percentage.
 * @param[in] update_type Input the firmware type. Refer to enum ota_update_type.
 * @return 0~100 (100 is success).
 ****************************************************************************************
 */
UINT ota_update_get_progress(ota_update_type update_type);

/**
 ****************************************************************************************
 * @brief SFLASH address to store downloaded data from the server.
 * @param[in] update_type  Input the firmware type. Refer to enum ota_update_type.
 * @return SFLASH address (hex).
 ****************************************************************************************
 */
UINT ota_update_get_new_sflash_addr(ota_update_type update_type);

/**
 ****************************************************************************************
 * @brief Read SFLASH as much as the input address and length.
 * @param[in] addr  SFLASH address(hex).
 * @param[out] buf  Buffer pointer to store read data.
 * @param[int] len  Length to read.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_read_flash(UINT addr, VOID *buf, UINT len);

/**
 ****************************************************************************************
 * @brief Erase SFLASH as much as the input address and length.
 * @param[in] addr  SFLASH address(hex).
 * @param[int] len  Length to erase.
 * @return return erased length.
 ****************************************************************************************
 */
UINT ota_update_erase_flash(UINT addr, UINT len);

/**
 ****************************************************************************************
 * @brief Copy as much as the length from SFLASH address src_addr to dest_addr.
 * @param[in] dest_addr  Destination SFLASH address(hex).
 * @param[in] src_addr  Source SFLASH address(hex).
 * @param[int] len  Length to copy.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_copy_flash(UINT dest_addr, UINT src_addr, UINT len);

/**
 ****************************************************************************************
 * @brief Set the name(version) of MCU FW to be downloaded to sflash. If not set, it is set as the default string.
 * @param[in] name  Input the firmware name(version). Maximum 8 bytes.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_set_mcu_fw_name(char *name);

/**
 ****************************************************************************************
 * @brief Get name(version) of MCU FW downloaded to sflash.
 * @param[out] name  Pointer to get the name(version) of MCU FW.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_get_mcu_fw_name(char *name);

/**
 ****************************************************************************************
 * @brief Get name(version), size and CRC32 of MCU FW downloaded to sflash.
 * @param[out] name  Pointer to get the name(version) of MCU FW.
 * @param[out] size    Pointer to get the size of MCU FW.
 * @param[out] crc     Pointer to get the CRC32 value of MCU FW.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_get_mcu_fw_info(char *name, UINT *size, UINT *crc);

/**
 ****************************************************************************************
 * @brief Starts transmission of MCU FW stored in flash through UART1 as much as the set size.
 * @param[in] sflash_addr  Start address for reading.
 * @param[in] size     Read size.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_read_mcu_fw(UINT sflash_addr, UINT size);

/**
 ****************************************************************************************
 * @brief Starts transmission of MCU FW stored in flash through UART1.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_trans_mcu_fw(void);

/**
 ****************************************************************************************
 * @brief Delete MCU FW saved in sflash.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_erase_mcu_fw(void);

/**
 ****************************************************************************************
 * @brief Calculate CRC32 of MCU FW stored in sflash.
 * @param[in] sflash_addr  CRC calculation start address.
 * @param[in] size     CRC calculation size.
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_calcu_mcu_fw_crc(int sflash_addr, int size);

/**
 ****************************************************************************************
 * @brief Parsing the cli command.
 * @param[in] argc
 * @param[in] argv[]
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_cmd_parse(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Set mbedtls_ssl_conf_authmode for https server.
 * @param[in] tls_auth_mode
 * @return 0x00 (OTA_SUCCESS) on success.
 ****************************************************************************************
 */
UINT ota_update_set_tls_auth_mode_nvram(int tls_auth_mode);

#endif	/* (__SUPPORT_OTA__) */
#endif	/* (__OTA_UPDATE_H__) */

/* EOF */
