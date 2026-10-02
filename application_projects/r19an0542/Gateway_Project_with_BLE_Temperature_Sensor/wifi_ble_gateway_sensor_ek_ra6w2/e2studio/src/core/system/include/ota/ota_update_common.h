/**
 ****************************************************************************************
 *
 * @file ota_update_common.h
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


#if !defined (__OTA_UPDATE_COMMON_H__)
#define	__OTA_UPDATE_COMMON_H__

#if defined (__SUPPORT_OTA__)
#include <stdio.h>
//#include "application.h"

//#include "da16200_map.h"
//#include "da16x_image.h"
#include "ota_update.h"
#if defined (__BLE_COMBO_REF__)
#include "da16_gtl_features.h"
#endif // __BLE_COMBO_REF__
//#include "da16x_sys_watchdog.h"
//#include "da16x_sys_watchdog_ids.h"

#if !defined (BIT)
#define BIT(x) (1 << (x))
#endif // (BIT)

// For dynamic memory allocation ...
#define USE_HEAP_MEM
#if defined (USE_HEAP_MEM)
#define OTA_MALLOC 	pvPortMalloc
#define OTA_FREE		vPortFree
#else
#define OTA_MALLOC 	APP_MALLOC
#define OTA_FREE		APP_FREE
#endif // (USE_HEAP_MEM)

/// Disable version checking for debugging
#undef DISABLE_OTA_VER_CHK

/// OTA update thread name
#define	OTA_TASK_NAME				"OTA_update"
#define	OTA_DPM_REG_NAME			OTA_TASK_NAME

#define	OTA_TASK_STACK_SZ			(1024 * 5) / 4 //WORD
#define OTA_SFLASH_BUF_SZ			(1024 * 4)

#define OTA_SUCCESS_EVT				0xff //OTA_SUCCESS

/// Process is not ready.
#define OTA_STATE_NOT_READY    		0
/// Process is ready.
#define OTA_STATE_READY       		1
/// Process is in progress.
#define OTA_STATE_PROGRESS			2
/// Process completed.
#define OTA_STATE_FINISH			3
/// Process stopped.
#define OTA_STATE_STOP				4

#define OTA_VER_DELIMITER			"-"
#define OTA_VER_START_OFFSET		16  /* address 0x00010 ~ 0x0002F*/
#define OTA_FW_MAGIC_NUM			{0x46, 0x43, 0x39, 0x4B}

/// Firmware version structure.
#define UPDATE_TYPE_MAX	6
#define VENDOR_MAX 		6
#define MAJOR_MAX		3
#define MINOR_MAX		10
#define CUSTOMER_MAX	10

// ex) FRTOS-GEN01-01-1c3618887a-0000000000
typedef struct {
    // Version type
    CHAR	update_type[UPDATE_TYPE_MAX + 1];
    // Vendor name
    CHAR	vendor[VENDOR_MAX + 1];
    // Major version
    CHAR	major[MAJOR_MAX + 1];
    // Minor version
    CHAR	minor[MINOR_MAX + 1];
    // Customer version
    CHAR	customer[CUSTOMER_MAX + 1];
} FW_versionInfo_t;

/// MCU Firmware version structure.
#define OTA_MCU_FW_NAME_LEN 		(8)
typedef struct {
    char 	name[OTA_MCU_FW_NAME_LEN];
    UINT 	size;
    UINT 	crc;
}	ota_mcu_fw_info_t;
#define OTA_MCU_FW_HEADER_SIZE 		sizeof(ota_mcu_fw_info_t)

// Status for version check.
enum ota_version_flags {
    // Init value.
    OTA_HEADER_INIT				= 0,
    // The type of FW to be compared is the same.
    OTA_HEADER_SAME_FW_TYPE		= 1,
    // The vendor of FW to be compared is the same.
    OTA_HEADER_SAME_VENDOR		= 2,
    // The major version of FW to be compared is the same.
    OTA_HEADER_SAME_MAJOR		= 3,
    // The major version of FW to be compared is the different.
    OTA_HEADER_DIFF_MAJOR		= 4,
    // The minor version of FW to be compared is the different.
    OTA_HEADER_DIFF_MINOR		= 5,
    // The customer version of FW to be compared is the different.
    OTA_HEADER_DIFF_CUST		= 6,
    // The type of FW to be compared is the different.
    OTA_HEADER_DIFF_FW_TYPE		= 7,
    // The vendor version of FW to be compared is the different.
    OTA_HEADER_DIFF_VENDOR		= 8,
    // The versions is incompatible.
    OTA_HEADER_INCOMPATI_FLASH	= 9,
    // The magic number of the FW is the same.
    OTA_HEADER_SAME_MAGIC		= 10,
    // The magic number of the FW is not compatible.
    OTA_HEADER_INCOMPATI_MAGIC	= 11,
    // The FW is not involved in version checking. (user FW or cert)
    OTA_HEADER_VER_DONT_CARE	= 12,
    // Error.
    OTA_HEADER_ERROR //13
};

/// Struct for flash writing
typedef struct {
    UINT	sflash_addr;
    UINT	total_length;
    UINT	length;
    UINT	offset;
    UCHAR	*buffer;
} ota_update_sflash_t;

/// Structure to download firmware
typedef struct {
    ota_update_type	update_type;
    ota_update_sflash_t write;
    UINT	download_status;
    UINT	version_check;
    UINT	content_length;
    UINT	received_length;
} ota_update_download_t;

/// Settings structure used for ota update requests.
typedef struct {
    /// FW type being downloaded
    ota_update_type	update_type;
    /// OTA Server address where RTOS firmware exists.
    char	url[OTA_HTTP_URL_LEN];
#if defined (__BLE_COMBO_REF__)
    /// OTA Server address where BLE firmware exists.
    char	url_ble_fw[OTA_HTTP_URL_LEN];
#endif /* __BLE_COMBO_REF__  */

    /// If the value is true, if the new firmware download is successful, it will reboot with the new firmware. Only for RTOS
    UINT	auto_renew;
    /// Flash address where the downloaded FW is written.
    UINT 	download_sflash_addr;
    /// Callback function pointer to check the download status.
    void 	(*download_notify)(ota_update_type update_type, UINT ret_status, UINT progress);
    /// Callback function pointer to check the renew state. Only for RTOS.
    void 	(*renew_notify)(UINT ret_status);
    /// Status of the download process (not_ready, ready, progress, finish, stop).
    UINT	update_state;
    /// Downlaod status. (success or failed)
    UINT	status;
    /// RTOS download progress.
    UINT	progress_rtos;
    /// MCU FW download progress.
    UINT 	progress_mcu_fw;
    /// CERT_KEY download progress.
    UINT 	progress_cert_key;
    /// MCU FW streaming progress.
    UINT 	progress_mcu_stream; //APP_CORE
#if defined (__BLE_COMBO_REF__)
    /// BLE download progress.
    UINT	progress_ble;
#endif /* __BLE_COMBO_REF__ */
} ota_update_proc_t;


UINT ota_update_check_version(ota_update_type update_type, UCHAR *data, UINT data_len);
UINT ota_update_parse_version_string(UCHAR *version, FW_versionInfo_t *fw_ver);
const char *ota_update_type_to_text(ota_update_type update_type);
UINT ota_update_cli_cmd_parse(int argc, char *argv[]);
UINT32 ota_update_sflash_crc(UINT sectorAddr);
UINT ota_update_check_accept_size(ota_update_type update_type, UINT size);
UINT ota_update_get_curr_sflash_addr(ota_update_type update_type);
UINT ota_update_set_user_sflash_addr(UINT sflash_addr);
UINT ota_update_check_all_download(void);
UINT ota_update_current_fw_renew(void);
void ota_update_status_atcmd(UINT atcmd_event, UINT status);


UINT ota_update_check_refuse_flag(void);
UINT ota_update_process_create(OTA_UPDATE_CONFIG *update_conf);
UINT ota_update_check_state(void);
UINT ota_update_process_stop(void);
UINT ota_update_get_download_progress(ota_update_type update_type);
void ota_update_send_status(UINT status);
UINT ota_update_get_proc_state(void);
void ota_update_print_status(ota_update_type update_type, UINT status);
void ota_update_set_download_progress(ota_update_type update_type, UINT progress);
UINT ota_update_write_sflash(ota_update_sflash_t *sflash_ctx,	UCHAR *data, UINT length);
UINT ota_update_get_image_info(UINT sectorAddr, DA16X_IMGHEADER_TYPE *infoImage);
UINT ota_update_toggle_boot_index(void);

#endif	/* (__SUPPORT_OTA__) */
#endif	/* (__OTA_UPDATE_COMMON_H__) */

/* EOF */
