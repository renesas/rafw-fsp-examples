/**
 ****************************************************************************************
 *
 * @file ota_common.c
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
//#include "sdk_type.h"

#if defined (__SUPPORT_OTA__)
#include "FreeRTOSConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "da16x_types.h"
#include "sys_cfg.h"
#include "common_def.h"
#include "common_config.h"
#include "command.h"
#include "command_net.h"
#include "environ.h"
#include "util_api.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "ota_update_http.h"
#if defined (__BLE_COMBO_REF__)
#include "ota_update_ble_fw.h"
#endif //(__BLE_COMBO_REF__)

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"


extern int	get_cur_boot_index(void);
extern UCHAR	set_boot_index(int index);
#if defined (__SUPPORT_ATCMD__)
extern void atcmd_asynchony_event(int index, char *buf);
#endif // (__SUPPORT_ATCMD__)
extern int	interface_select;

static TaskHandle_t ota_proc_xHandle = NULL;
static EventGroupHandle_t ota_event_group;

static ota_update_proc_t _ota_proc = { 0, };
static ota_update_proc_t *ota_proc = &_ota_proc;

static UINT CUSTOM_SFLASH_ADDR = SFLASH_USER_AREA_START;
static UINT ota_refuse_flag = 0;

void ota_update_status_atcmd(UINT atcmd_event, UINT status)
{
#if defined (__SUPPORT_ATCMD__)
    char atc_buf[15] = {0, };
    memset(atc_buf, 0x00, sizeof(atc_buf));
    sprintf(atc_buf, "0x%02x", status);
    /* ATC_EV_OTADWSTART == 6 */
    /* ATC_EV_OTARENEW == 7 */
    atcmd_asynchony_event(atcmd_event, atc_buf);
#else
    DA16X_UNUSED_ARG(atcmd_event);
    DA16X_UNUSED_ARG(status);
#endif // (__SUPPORT_ATCMD__)
}

UINT ota_update_check_refuse_flag(void)
{
    return ota_refuse_flag;
}

static void ota_update_print_fw_header(DA16X_IMGHEADER_TYPE *infoImage)
{
    OTA_INFO(" Version-------- %s \n", infoImage->name);
    OTA_INFO(" Data Size------ %d \n", infoImage->datsiz);
    OTA_INFO(" HCRC----------- 0x%x \n", infoImage->hcrc);
    OTA_INFO(" DCRC----------- 0x%x \n", infoImage->datcrc);
}

UINT ota_update_check_accept_size(ota_update_type update_type, UINT size)
{
    UINT alloc_size = 0;

    if (update_type == OTA_TYPE_INIT) {
        return OTA_SUCCESS;
    }

    if (update_type >= OTA_TYPE_UNKNOWN) {
        OTA_ERR("- OTA: Unknown FW type\n");
        return OTA_ERROR_TYPE;
    }

    if (update_type == OTA_TYPE_RTOS) {
        alloc_size = SFLASH_ALLOC_SIZE_RTOS;

    } else if ((update_type == OTA_TYPE_MCU_FW)
               || (update_type == OTA_TYPE_CERT_KEY)) {
        if (CUSTOM_SFLASH_ADDR != SFLASH_USER_AREA_START) {
            alloc_size = SFLASH_USER_AREA_END - CUSTOM_SFLASH_ADDR;
        } else {
            alloc_size = SFLASH_ALLOC_SIZE_USER;
        }
#if defined (__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_BLE_FW) {
        alloc_size = SFLASH_14531_BLE_AREA_SIZE;

#endif //(__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_MCU_FW_STREAM) { //APP_CORE
        return OTA_SUCCESS;
    } else {
        OTA_ERR("- OTA: Wrong FW type (%d)\n", update_type);
        return OTA_FAILED;
    }

    if (alloc_size <= size) {
        OTA_ERR("- OTA: <%s> FW size error. (Allowable size = %d, Receiving size = %d)\n",
                ota_update_type_to_text(update_type), alloc_size, size);
        return OTA_ERROR_SIZE;
    }

    return OTA_SUCCESS;
}

UINT ota_update_get_curr_sflash_addr(ota_update_type update_type)
{
    if (update_type == OTA_TYPE_INIT) {
        return OTA_SUCCESS;
    }

    if (update_type >= OTA_TYPE_UNKNOWN) {
        OTA_ERR("- OTA: Unknown FW type\n");
        return OTA_ERROR_TYPE;
    }

    if (update_type == OTA_TYPE_RTOS) {
        if (get_cur_boot_index() == 1) {
            return SFLASH_RTOS_1_BASE;
        } else {
            return SFLASH_RTOS_0_BASE;
        }
#if defined (__BLE_COMBO_REF__)
    } else if  (update_type == OTA_TYPE_BLE_FW) {
        return SFLASH_BLE_FW_BASE;
#endif //(__BLE_COMBO_REF__)

    } else if (update_type == OTA_TYPE_MCU_FW) {
        return CUSTOM_SFLASH_ADDR;

    } else if (update_type == OTA_TYPE_CERT_KEY) {
        return SFLASH_ROOT_CA_ADDR1;
    }

    OTA_ERR("- OTA: Wrong FW type (%d)\n", update_type);
    return SFLASH_UNKNOWN_ADDRESS;
}

UINT ota_update_get_new_sflash_addr(ota_update_type update_type)
{
    if (update_type == OTA_TYPE_INIT) {
        return OTA_SUCCESS;
    }

    if (update_type >= OTA_TYPE_UNKNOWN) {
        OTA_ERR("- OTA: Unknown FW type\n");
        return OTA_ERROR_TYPE;
    }

    if (update_type == OTA_TYPE_RTOS) {
        if (get_cur_boot_index() == 1) {
            return SFLASH_RTOS_0_BASE;
        } else {
            return SFLASH_RTOS_1_BASE;
        }
#if defined (__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_BLE_FW) {
        return SFLASH_BLE_FW_BASE;
#endif //(__BLE_COMBO_REF__)

    } else if ((update_type == OTA_TYPE_MCU_FW)
               || (update_type == OTA_TYPE_CERT_KEY)) {
        return CUSTOM_SFLASH_ADDR;
    } else if (update_type == OTA_TYPE_MCU_FW_STREAM) //APP_CORE
        return 0;

    OTA_ERR("- OTA: Wrong FW type (%d)\n", update_type);
    return SFLASH_UNKNOWN_ADDRESS;
}

UINT ota_update_set_user_sflash_addr(UINT sflash_addr)
{
    if (((sflash_addr >= SFLASH_RMA_CERTIFICATE) && (sflash_addr < SFLASH_NVRAM_ADDR))
            || ((sflash_addr >= SFLASH_USER_AREA_START) && (sflash_addr < SFLASH_USER_AREA_END))) {
        OTA_INFO("- OTA : download_sflash_addr = 0x%x \n", sflash_addr);
        CUSTOM_SFLASH_ADDR = sflash_addr;

    } else	{
        OTA_ERR("- OTA : sflash address(0x%x) is incorrect \n", sflash_addr);
        return OTA_ERROR_SFLASH_ADDR;
    }

    return OTA_SUCCESS;
}
const char *ota_update_type_to_text(ota_update_type update_type)
{
    if (update_type == OTA_TYPE_RTOS) {
        return OTA_RTOS_NAME;
    } else if (update_type == OTA_TYPE_MCU_FW) {
        return OTA_MCU_FW_NAME;
#if defined (__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_BLE_FW) {
        return OTA_BLE_FW_NAME;
    } else if (update_type == OTA_TYPE_BLE_COMBO) {
        return OTA_BLE_COMBO_NAME;
#endif //(__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_CERT_KEY) {
        return OTA_CERT_KEY_NAME;
     } else if (update_type == OTA_TYPE_MCU_FW_STREAM) { //APP_CORE
        return OTA_STR_MCU_STREAM;
    }
    return "UNKNOWN";
}

UINT ota_update_check_all_download(void)
{
    UINT progress_rtos = ota_update_get_download_progress(OTA_TYPE_RTOS);
    if (progress_rtos == 100) {
        return OTA_SUCCESS;
    }
    OTA_INFO("- OTA: Download - %s(%s) \n", OTA_RTOS_NAME, (progress_rtos == 100) ? "OK" : "FAIL");
    return OTA_NOT_ALL_DOWNLOAD;
}

UINT ota_update_parse_version_string(UCHAR *version,
        FW_versionInfo_t *fw_ver)
{
    CHAR *pRev_a = NULL;
    CHAR *pRev_b = NULL;
    UINT str_len = 0;
    UINT total_str_len = 0;
    UINT sum_str_len = 0;
    UINT status = OTA_SUCCESS;

    if (version == NULL || fw_ver == NULL) {
        return OTA_FAILED;
    }

    memset(fw_ver, 0x00, sizeof(FW_versionInfo_t));

    total_str_len = strlen((char *)version);

    /* Extract FW_Type */
    pRev_a = (CHAR *)version;
    pRev_b = (CHAR *)strstr((char *)pRev_a, OTA_VER_DELIMITER);
    if (pRev_b == NULL) {
        OTA_DBG("  > Delimiter (-) not found in version string\n");
        return OTA_FAILED;
    }

    str_len = pRev_b - pRev_a;
    if (str_len > UPDATE_TYPE_MAX) {
        OTA_DBG("  > FW_TYPE is too long (max = %d)\n", UPDATE_TYPE_MAX);
        status = OTA_FAILED;
    }

    memcpy(fw_ver->update_type, pRev_a, str_len > UPDATE_TYPE_MAX ? UPDATE_TYPE_MAX : str_len);
    sum_str_len += str_len;

    /* Extract Vendor */
    pRev_b++;
    sum_str_len ++;
    pRev_a = (CHAR *)strstr((char *)pRev_b, OTA_VER_DELIMITER);
    if (pRev_a == NULL) {
        OTA_DBG("  > Delimiter (%s) not found in version string\n", OTA_VER_DELIMITER);
        return OTA_FAILED;
    }

    str_len = pRev_a - pRev_b;
    if (str_len > VENDOR_MAX) {
        OTA_DBG("  > VENDOR is too long (max = %d)\n", VENDOR_MAX);
        status = OTA_FAILED;
    }

    memcpy(fw_ver->vendor, pRev_b, str_len > VENDOR_MAX ? VENDOR_MAX : str_len);
    sum_str_len += str_len;

    /* Extract Major number */
    pRev_a++;
    sum_str_len ++;
    pRev_b = (CHAR *)strstr((char *)pRev_a, OTA_VER_DELIMITER);
    if (pRev_b == NULL) {
        OTA_DBG("  > Delimiter (%s) not found in version string\n", OTA_VER_DELIMITER);
        return OTA_FAILED;
    }

    str_len = pRev_b - pRev_a;
    if (str_len > MAJOR_MAX) {
        OTA_DBG("  > MAJOR is too long (max = %d)\n", MAJOR_MAX);
        status = OTA_FAILED;
    }

    memcpy(fw_ver->major, pRev_a, str_len > MAJOR_MAX ? MAJOR_MAX : str_len);
    sum_str_len += str_len;

    /* Extract Minor number */
    pRev_b++;
    sum_str_len ++;
    pRev_a = (CHAR *)strstr((char *)pRev_b, OTA_VER_DELIMITER);
    if (pRev_a == NULL) {
        OTA_DBG("  > Delimiter (%s) not found in version string\n", OTA_VER_DELIMITER);
        return OTA_FAILED;
    }

    str_len = pRev_a - pRev_b;
    sum_str_len += str_len;
    if (str_len > MINOR_MAX) {
        OTA_DBG("  > MINOR is too long (max = %d)\n", MINOR_MAX);
        status = OTA_FAILED;
    }

    memcpy(fw_ver->minor, pRev_b, str_len > MINOR_MAX ? MINOR_MAX : str_len);

    /* Extract Customer Version */
    pRev_a++;
    sum_str_len ++;
    str_len = total_str_len - sum_str_len;
    if (str_len > CUSTOMER_MAX) {
        OTA_DBG("  > CUSTOMER is too long (max = %d)\n", CUSTOMER_MAX);
        status = OTA_FAILED;
    }

    memcpy(fw_ver->customer, pRev_a, str_len > CUSTOMER_MAX ? CUSTOMER_MAX : str_len);

    if (   !strlen((char *)fw_ver->update_type)
            || !strlen((char *)fw_ver->vendor)
            || !strlen((char *)fw_ver->major)
            || !strlen((char *)fw_ver->minor)
            || !strlen((char *)fw_ver->customer)) {
        memset(fw_ver, 0x00, sizeof(FW_versionInfo_t));
        return OTA_FAILED;
    }

    return status;
}

static UINT ota_update_read_new_fw_version(UCHAR *data,
        FW_versionInfo_t *fw_ver)
{
    UCHAR new_ver[DA16X_IH_NMLEN] = {0x00, };

    if (data == NULL) {
        return OTA_FAILED;
    }

    memset(new_ver, 0x00, DA16X_IH_NMLEN);
    memcpy(new_ver, &data[OTA_VER_START_OFFSET], DA16X_IH_NMLEN);

    /* parse received version */
    if (ota_update_parse_version_string(new_ver, fw_ver)) {
        OTA_ERR("   > Failed to parse Server FW version : %s \n", new_ver);
    } else {
        OTA_INFO("   > Server FW version : %s-%s-%s-%s-%s \n",
                 fw_ver->update_type,
                 fw_ver->vendor,
                 fw_ver->major,
                 fw_ver->minor,
                 fw_ver->customer);
    }

    return OTA_SUCCESS;
}

static UINT ota_update_read_current_fw_version(UINT fw_addr,
        FW_versionInfo_t *fw_ver)
{
    DA16X_IMGHEADER_TYPE infoImage	= { 0, };

    if (fw_ver == NULL) {
        return OTA_FAILED;
    }

    /* READ SFLASH */
    if (ota_update_get_image_info(fw_addr, &infoImage) == 0) {
        return OTA_FAILED;
    }

    /* PARSE VERION */
    if (ota_update_parse_version_string(infoImage.name, fw_ver)) {
        OTA_ERR("   > Failed to parse Current FW version : %s \n", infoImage.name);
    }

    return OTA_SUCCESS;
}
static UINT ota_update_compare_fw_version(ota_update_type update_type,
        FW_versionInfo_t cur_ver, FW_versionInfo_t new_ver)
{
    UINT ver_check_bit = 0x00;

    OTA_INFO("- OTA Update : <%s> Compare Versions\n", ota_update_type_to_text(update_type));

    /* update_type */
    if (memcmp(new_ver.update_type, cur_ver.update_type, strlen((char *)new_ver.update_type))) {
        ver_check_bit = BIT(OTA_HEADER_DIFF_FW_TYPE);
        OTA_INFO("   > Incompatible Image type : %s\n", new_ver.update_type);
        goto chk_finish;
    } else {
        ver_check_bit |= BIT(OTA_HEADER_SAME_FW_TYPE);
    }

    /* vendor */
    if (memcmp(new_ver.vendor, cur_ver.vendor, strlen((char *)new_ver.vendor))) {
        ver_check_bit = BIT(OTA_HEADER_DIFF_VENDOR);
        OTA_INFO("   > Incompatible Vendor : Cur-%s, New-%s\n", cur_ver.vendor, new_ver.vendor);
        goto chk_finish;
    } else {
        ver_check_bit |= BIT(OTA_HEADER_SAME_VENDOR);
    }

    /* major */
    if (memcmp(new_ver.major, cur_ver.major, strlen((char *)new_ver.major))) {
        OTA_INFO("   > Different Major : Cur-%s, New-%s\n", cur_ver.major, new_ver.major);
        ver_check_bit |= BIT(OTA_HEADER_DIFF_MAJOR);
    } else {
        ver_check_bit |= BIT(OTA_HEADER_SAME_MAJOR);
    }

    /* minor */
    if (memcmp(new_ver.minor, cur_ver.minor, strlen((char *)new_ver.minor))) {
        OTA_INFO("   > Different Minor : Cur-%s, New-%s \n", cur_ver.minor, new_ver.minor);
        ver_check_bit |= BIT(OTA_HEADER_DIFF_MINOR);
    }

    /* customer */
    if (memcmp(new_ver.customer, cur_ver.customer, strlen((char *)new_ver.customer))) {
        OTA_INFO("   > Different Customer : Cur-%s, New-%s\n", cur_ver.customer,
                 new_ver.customer);
        ver_check_bit |= BIT(OTA_HEADER_DIFF_CUST);
    }

    if (ver_check_bit & BIT(OTA_HEADER_ERROR)) {
        OTA_INFO("   > Version comparison failed\n");
        goto chk_finish;
    }

    if (   (ver_check_bit & BIT(OTA_HEADER_SAME_FW_TYPE))
            && (ver_check_bit & BIT(OTA_HEADER_SAME_VENDOR))
            && (ver_check_bit & BIT(OTA_HEADER_SAME_MAJOR))
            && ((ver_check_bit & BIT(OTA_HEADER_DIFF_MINOR)) == 0)
            && ((ver_check_bit & BIT(OTA_HEADER_DIFF_CUST)) == 0)) {
        OTA_INFO("   > Same Version : %s-%s-%s-%s-%s \n",
                 new_ver.update_type,
                 new_ver.vendor,
                 new_ver.major,
                 new_ver.minor,
                 new_ver.customer);
    }

chk_finish:
    return ver_check_bit;
}

UINT ota_update_check_version(ota_update_type update_type,
                              UCHAR *data, UINT data_len)
{
    FW_versionInfo_t curr_ver;
    FW_versionInfo_t rev_ver;
    UINT ret_val = 0;
    CHAR magic[4] = OTA_FW_MAGIC_NUM;

    if ((data == NULL) || data_len == 0) {
        OTA_ERR("- OTA: Unknown version\n");
        return OTA_VERSION_UNKNOWN;
    }

    OTA_DBG("[%s]update_type = %d, data_len = %d, data = %s\n", __func__, update_type, data_len, data);
    if (update_type == OTA_TYPE_INIT) {
        return OTA_SUCCESS;

    } else	if (update_type >= OTA_TYPE_UNKNOWN) {
        OTA_ERR("- OTA: Unknown FW type\n");
        return OTA_ERROR_TYPE;

    } else if (update_type == OTA_TYPE_RTOS) {
        if (data_len < sizeof(FW_versionInfo_t)) {
            OTA_ERR("- OTA: Data size is too small to check version \n");
            return OTA_VERSION_UNKNOWN;
        }

        /* GET CURRENT VERSION */
        if (ota_update_read_current_fw_version(
                    ota_update_get_curr_sflash_addr(update_type), &curr_ver) != OTA_SUCCESS) {
            OTA_ERR("- OTA: Failed to read current version \n");
        }

        /* GET RECEIVED VERSION */
        if (ota_update_read_new_fw_version(data, &rev_ver) != OTA_SUCCESS) {
            return OTA_VERSION_UNKNOWN;
        }

        /* CHECK MAGIC NUMBER */
        if (memcmp(&data[0], &magic[0], 4) != 0) {
            OTA_ERR("- OTA: Wrong magic number (0x%02x 0x%02x 0x%02x 0x%02x)\n",
                    data[0], data[1], data[2], data[3]);

            return OTA_VERSION_UNKNOWN;
        }

        /* COMPARE CURRENT AND RECEIVED */
        ret_val = ota_update_compare_fw_version(update_type, curr_ver, rev_ver);

        if (   (ret_val & BIT(OTA_HEADER_DIFF_FW_TYPE))
                || (ret_val & BIT(OTA_HEADER_DIFF_VENDOR))
                || (ret_val & BIT(OTA_HEADER_INCOMPATI_MAGIC))
                || (ret_val & BIT(OTA_HEADER_INIT))
                || (ret_val & BIT(OTA_HEADER_ERROR))
           ) {
            return OTA_VERSION_UNKNOWN;
        }
#if defined (__BLE_COMBO_REF__) && defined (__BLE_FW_VER_CHK_IN_OTA__)
        if (ret_val & BIT(OTA_HEADER_DIFF_MAJOR)) {
            OTA_ERR(" - Mismatch major-number : COMBO %s(%s)\n",
                    OTA_RTOS_NAME, rev_ver.major);
            return OTA_VERSION_UNKNOWN;
        }
#endif //(__BLE_COMBO_REF__) && (__BLE_FW_VER_CHK_IN_OTA__)
    }
#if defined (__BLE_COMBO_REF__) && defined (__BLE_FW_VER_CHK_IN_OTA__)
    else if (update_type == OTA_TYPE_BLE_FW) {
        return ota_update_ble_check_version(data, data_len);
    }
#endif //(__BLE_COMBO_REF__) && (__BLE_FW_VER_CHK_IN_OTA__)

    return OTA_SUCCESS;
}

UINT ota_update_toggle_boot_index(void)
{
    int new_boot_idx;

    new_boot_idx = get_cur_boot_index() ? 0 : 1;

    if (set_boot_index(new_boot_idx)) {
        ota_refuse_flag = 1;
        OTA_INFO(">>> %s is updated and system reboots. (New boot_idx=%d) <<<\n\n"
                 , OTA_RTOS_NAME
                 , new_boot_idx);
        return OTA_SUCCESS;
    }

    OTA_ERR(">>> Failed to set boot index <<<\n");
    return OTA_FAILED;
}

UINT ota_update_current_fw_renew(void)
{
    UCHAR *rd_buf = NULL;
    UINT len = 0;
    UINT status = OTA_FAILED;

    OTA_INFO("\n- OTA Update : Renew - Start\n");
    if (ota_update_check_refuse_flag())	{
        OTA_ERR("- OTA: Try again after reboot\n\n");
        status = OTA_FAILED;
        goto _renew_fail;
    }

#if defined (__BLE_COMBO_REF__)
    status = ota_update_ble_check_all_downloaded();
    if (status != OTA_SUCCESS) {
        OTA_INFO("- OTA: Try to download F/W images\n");
        goto _renew_fail;
    } else {
        /* OTA_SUCCESS */
       extern UINT ota_update_ble_only_download_status(void);
        if (ota_update_ble_only_download_status() == OTA_SUCCESS) {
            return OTA_SUCCESS;
        }
    }
#else
    status = ota_update_check_all_download();
    if (status != OTA_SUCCESS) {
        OTA_INFO("- OTA: Try to download F/W images\n");
        goto _renew_fail;
    }
#endif //(__BLE_COMBO_REF__)

    /* Compare verion - RTOS */
    if (ota_update_sflash_crc(ota_update_get_new_sflash_addr(OTA_TYPE_RTOS)) <= 0) {
        OTA_ERR("- OTA: <%s> CRC Error\n", OTA_RTOS_NAME);
        status = OTA_ERROR_CRC;
        goto _renew_fail;
    }

    if (ota_update_get_download_progress(OTA_TYPE_RTOS) == 100) {
        len = 80; /* byte */
        rd_buf = OTA_MALLOC(len);
        if (rd_buf != NULL) {
            if (ota_update_read_flash(ota_update_get_new_sflash_addr(OTA_TYPE_RTOS), rd_buf, len) == OTA_SUCCESS) {
                if (ota_update_check_version(OTA_TYPE_RTOS,	rd_buf, len) != OTA_SUCCESS) {
                    OTA_ERR("- OTA: <%s> Incompatible new version\n", OTA_RTOS_NAME);
                    status = OTA_VERSION_INCOMPATI;
                    OTA_FREE(rd_buf);
                    goto _renew_fail;
                }
            } else {
                OTA_ERR("- OTA: <%s> Failed to read new version\n", OTA_RTOS_NAME);
                status = OTA_FAILED;
                OTA_FREE(rd_buf);
                goto _renew_fail;
            }
            OTA_FREE(rd_buf);
        } else {
            OTA_ERR("[%s:%d] Failed to allocate read buffer(%d bytes)\n", __func__, __LINE__, len);
            status = OTA_MEM_ALLOC_FAILED;
            goto _renew_fail;
        }

        /* RENEW - Toggle the boot index */
        if (ota_update_toggle_boot_index() == OTA_SUCCESS) {
            return OTA_SUCCESS;
        }
    }

_renew_fail:
    /* RENEW FAIL */
    OTA_ERR("\n>>> OTA FW update failed(0x%02x) <<<\n\n", status);

#if defined (__BLE_COMBO_REF__) && defined (__BLE_FW_VER_CHK_IN_OTA__)
    ota_update_ble_chg_new_fw_valid();
#endif // (__BLE_COMBO_REF__) && (__BLE_FW_VER_CHK_IN_OTA__)

    return status;
}

UINT ota_update_get_image_info(UINT sectorAddr, DA16X_IMGHEADER_TYPE *infoImage)
{
    UINT  status = 0;
#if defined (SUPPORT_SFLASH_DEVICE)
    HANDLE  handler;
    UINT32 sbootmode;

    sbootmode = (DA16X_IMG_BOOT | DA16X_IMG_SKIP | (sizeof(UINT32) * 8));
    handler = flash_image_open(sbootmode, sectorAddr, (VOID *)&da16x_environ_lock);

    if (handler == NULL) {
        OTA_ERR(RED_COLOR "[%s] - Error : Read Flash Image Open \n" CLEAR_COLOR, __func__);
        return status;
    }

    status = flash_image_read(handler,
                              sectorAddr,
                              (VOID *)infoImage,
                              sizeof(DA16X_IMGHEADER_TYPE));
    if ( status <= 0 ) {
        OTA_ERR(RED_COLOR "[%s] - Error: Read Flash Image Header(st:%d) \n" CLEAR_COLOR,
                __func__, status);
    }

    flash_image_close(handler);
#endif	//(SUPPORT_SFLASH_DEVICE)

    return status;	/* 0: error */
}

UINT ota_update_sflash_crc(UINT sectorAddr)
{
    UINT	chk_size = 0;
#if defined (SUPPORT_SFLASH_DEVICE)
    HANDLE	handler;
    UINT	load_addr;
    UINT	jump_addr;

    UINT32 sbootmode;
    sbootmode = (DA16X_IMG_BOOT | DA16X_IMG_SKIP | (sizeof(UINT32) * 8));

    handler = flash_image_open(sbootmode, sectorAddr, (VOID *)&da16x_environ_lock);

    /* CRC Check for Src Image Header */
    chk_size = flash_image_check(handler, 0, sectorAddr);

    if (chk_size > 0) {
        /* CRC Check for Src Image Data */
        chk_size = flash_image_test(handler, sectorAddr, &load_addr, &jump_addr);
        if (chk_size > 0) {
            OTA_DBG(" [%s] check body(size:%d, sec_addr=0x%x load_addr=0x%x jump_addr=0x%x)\n", __func__, chk_size, sectorAddr, load_addr,
                    jump_addr);
        } else {
            OTA_ERR(RED_COLOR
                    " [%s] Failed check body (sec_addr:0x%x load_addr:0x%x jump_addr:0x%x \n"
                    CLEAR_COLOR, __func__, sectorAddr, load_addr, jump_addr);
            chk_size = 0;
        }
    } else {
        OTA_ERR(RED_COLOR " [%s] Failed to check header (sectorAddr:0x%x) \n"
                CLEAR_COLOR, __func__, sectorAddr);
    }

    flash_image_close(handler);
#endif //(SUPPORT_SFLASH_DEVICE)

    return chk_size;
}

static UINT ota_update_set_proc_state(UINT state)
{
    OTA_DBG("[%s]state = %d\n", __func__, state);
    return ota_proc->update_state = state;
}

UINT ota_update_get_proc_state(void)
{
    OTA_DBG("[%s]state = %d\n", __func__, ota_proc->update_state);
    return ota_proc->update_state;
}

void ota_update_print_status(ota_update_type update_type, UINT status)
{
    if (status == OTA_SUCCESS) {
        if (update_type == OTA_TYPE_INIT) {
            OTA_INFO("\n- OTA Update : <%s> Download - Stop\n\n", ota_update_type_to_text(update_type));
        } else {
            OTA_INFO("\n- OTA Update : <%s> Download - Success\n\n", ota_update_type_to_text(update_type));
        }
    } else {
        OTA_ERR("- OTA Update : <%s> Download - Failed (0x%02x)\n", ota_update_type_to_text(update_type), status);
    }
}

UINT ota_update_get_download_progress(ota_update_type update_type)
{
    vTaskDelay( 10 / portTICK_PERIOD_MS );

    if (update_type == OTA_TYPE_RTOS) {
        return ota_proc->progress_rtos;
#if defined (__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_BLE_FW) {
        return ota_proc->progress_ble;
#endif //(__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_MCU_FW) {
        return ota_proc->progress_mcu_fw;
    } else if (update_type == OTA_TYPE_CERT_KEY) {
        return ota_proc->progress_cert_key;
    } else if (update_type == OTA_TYPE_MCU_FW_STREAM) { //APP_CORE
        return ota_proc->progress_mcu_stream;
    }

    return 0;
}

void ota_update_set_download_progress(ota_update_type update_type, UINT progress)
{
    if (update_type == OTA_TYPE_RTOS) {
        ota_proc->progress_rtos = progress;
#if defined (__BLE_COMBO_REF__)
    } else if  (update_type == OTA_TYPE_BLE_FW) {
        ota_proc->progress_ble = progress;
#endif //(__BLE_COMBO_REF__)
    } else if (update_type == OTA_TYPE_MCU_FW) {
        ota_proc->progress_mcu_fw = progress;
    } else if (update_type == OTA_TYPE_CERT_KEY) {
        ota_proc->progress_cert_key = progress;
    } else if (update_type == OTA_TYPE_MCU_FW_STREAM) { //APP_CORE
        ota_proc->progress_mcu_stream = progress;
    }
}

static UINT ota_update_receive_status(void)
{
    ULONG events;

    events = xEventGroupWaitBits(ota_event_group,
                                 0xff,
                                 pdTRUE,
                                 pdFALSE,
                                 portMAX_DELAY);

    OTA_DBG("[%s]status = 0x%02x\n", __func__, events);
    if (events == OTA_SUCCESS_EVT) {
        return OTA_SUCCESS;
    }

    return events;
}

void ota_update_send_status(UINT status)
{
    if (ota_event_group != NULL) {
        OTA_DBG("[%s]status = 0x%02x\n", __func__, status);
        xEventGroupSetBits(ota_event_group, status);
    }
}

UINT ota_update_process_stop(void)
{
    UINT status = OTA_SUCCESS;
    UINT curr_state;

    curr_state = ota_update_get_proc_state();
    if (curr_state == OTA_STATE_PROGRESS) {
        ota_update_set_proc_state(OTA_STATE_STOP);
        status = OTA_SUCCESS;

    } else if ((curr_state == OTA_STATE_FINISH) || (curr_state == OTA_STATE_STOP)) {
        OTA_ERR("- OTA : In progressing... Please wait.\n");
        status = OTA_FAILED;
    } else {
        OTA_DBG("- OTA : No operation\n");
        status = OTA_FAILED;
    }

    ota_update_send_status(status);

    return status;
}

UINT ota_update_check_state(void)
{
    UINT curr_state;

    curr_state = ota_update_get_proc_state();
    if (curr_state == OTA_STATE_PROGRESS) {
        OTA_ERR("- OTA : %s is downloading... Try again after finishing.\n",
                ota_update_type_to_text(ota_proc->update_type));
        return OTA_FAILED;
    } else if ((curr_state == OTA_STATE_FINISH) || (curr_state == OTA_STATE_STOP)) {
        OTA_ERR("- OTA : In progressing(%d)... Please wait.\n", curr_state);
        return OTA_FAILED;
    }

    return OTA_SUCCESS;
}

static void ota_update_process(void *arg)
{
    DA16X_UNUSED_ARG(arg);

#if defined (__BLE_COMBO_REF__)
    UINT status_rtos = OTA_SUCCESS;
    UINT status_ble_fw = OTA_SUCCESS;
    UINT ble_combo_step = 0;
#endif //(__BLE_COMBO_REF__)

    /* DPM mode */
    RM_PMGR_W_dpm_job_name_set(OTA_DPM_REG_NAME, 0);

    ota_event_group = xEventGroupCreate();
    if (ota_event_group == NULL) {
        OTA_ERR("- OTA : Failed to create event_flags\n");
        ota_proc->status = OTA_FAILED;
        goto download_finish;
    }

#if defined (__BLE_COMBO_REF__)
    if (ota_proc->update_type == OTA_TYPE_BLE_COMBO) {
        ble_combo_step = 2;
    } else {
        ble_combo_step = 0;
    }

start_ota_update:

    if (ble_combo_step == 2) {
        ota_proc->update_type = OTA_TYPE_RTOS;
    } else if (ble_combo_step == 1) {
        ota_proc->update_type = OTA_TYPE_BLE_FW;
    }
#endif //(__BLE_COMBO_REF__)

    OTA_DBG("[%s] update_type = %s\n", __func__, ota_update_type_to_text(ota_proc->update_type));
    ota_update_set_proc_state(OTA_STATE_PROGRESS);
    ota_update_set_download_progress(ota_proc->update_type, 0);

    /*******************************/
    /* HTTP Client REQUEST */
    /*******************************/
    ota_proc->status = ota_update_http_client_request(ota_proc);
    if (ota_proc->status != OTA_SUCCESS) {
        goto download_finish;
    }

    /* Wait Event */
    OTA_DBG("[%s] Wait for an event... \n", __func__);
    ota_proc->status = ota_update_receive_status();
#if defined (__BLE_COMBO_REF__)
    if (ota_proc->update_type == OTA_TYPE_RTOS) {
        status_rtos = ota_proc->status;
    } else if (ota_proc->update_type == OTA_TYPE_BLE_FW) {
        status_ble_fw = ota_proc->status;
    }
#endif //(__BLE_COMBO_REF__)

download_finish:

    ota_update_status_atcmd(6, ota_proc->status);
    /* Print status */
    ota_update_print_status(ota_proc->update_type, ota_proc->status);

    if (ota_proc->download_notify != NULL) {
        ota_proc->download_notify(ota_proc->update_type,
                                  ota_proc->status,
                                  ota_update_get_download_progress(ota_proc->update_type));
    }

    ota_update_set_proc_state(OTA_STATE_READY);

#if defined (__BLE_COMBO_REF__)
    /* If any one of BLE_FW and RTOS fails to download, it is terminated. */
    if ((ble_combo_step > 1) && (ota_proc->status == OTA_SUCCESS)) {
        ble_combo_step--;
        goto start_ota_update;
    }
    ble_combo_step = 0;

    OTA_DBG("[%s] RENEW available(status_rtos = 0x%02x, status_ble_fw = 0x%02x, auto_renew = %d, proc_state = %d)\n",
            __func__,
            status_rtos,
            status_ble_fw,
            ota_proc->auto_renew,
            ota_update_get_proc_state());

    if (((status_rtos == OTA_SUCCESS) && (status_ble_fw == OTA_SUCCESS))
            && (ota_update_get_proc_state() != OTA_STATE_STOP)
            && (ota_proc->auto_renew > 0))
#else
    OTA_DBG("[%s] RENEW available(status = 0x%02x, auto_renew = %d, proc_state = %d)\n", __func__,
            ota_proc->status,
            ota_proc->auto_renew,
            ota_update_get_proc_state());

    if ((ota_proc->status == OTA_SUCCESS)
            && (ota_update_get_proc_state() != OTA_STATE_STOP)
            && (ota_proc->auto_renew > 0))
#endif //(__BLE_COMBO_REF__)
    {
        OTA_UPDATE_CONFIG ota_update_conf;
        ota_update_conf.renew_notify = ota_proc->renew_notify;
        ota_update_start_renew(&ota_update_conf);
    }

    /* DPM mode */
    dpm_app_unregister(OTA_DPM_REG_NAME);

    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }

    return;
}

UINT ota_update_process_create(OTA_UPDATE_CONFIG *update_conf)
{
    UINT status = OTA_SUCCESS;
    UINT wlan_init_cnt = 0;
    BaseType_t	xReturned;

    OTA_INFO("\n- OTA Update : <%s> Download - Start\n", ota_update_type_to_text(update_conf->update_type));

    while (chk_network_ready((UCHAR)interface_select) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wlan_init_cnt++;

        if (wlan_init_cnt == 100) {
            OTA_DBG("[%s] Wait to initialize WLAN ...(%d)\n", __func__, wlan_init_cnt / 10);
            OTA_ERR("- OTA : No network connection\n");
            wlan_init_cnt = 0;
            return OTA_FAILED;

        } else if ((wlan_init_cnt % 100) == 0) {
            OTA_DBG("[%s] Wait to initialize WLAN ...(%d)\n", __func__, wlan_init_cnt / 10);
        }
    }

    if (ota_update_check_refuse_flag())	{
        OTA_ERR("- OTA: Try again after rebooting.\n");
        return OTA_FAILED;
    }

    status = ota_update_check_state();
    if (status)	{
        return status;
    }

    if (update_conf->download_sflash_addr > 0) {
        status = ota_update_set_user_sflash_addr(update_conf->download_sflash_addr);
        if (status)	{
            return status;
        }
    }

    ota_proc->status = OTA_SUCCESS;

    if ((update_conf->update_type == OTA_TYPE_RTOS)
        || (update_conf->update_type == OTA_TYPE_BLE_COMBO)) {
        ota_proc->progress_rtos = 0;
    }

    if (update_conf->update_type == OTA_TYPE_MCU_FW) {
        ota_proc->progress_mcu_fw = 0;
    }

    if (update_conf->update_type == OTA_TYPE_CERT_KEY) {
        ota_proc->progress_cert_key = 0;
    }

#if defined (__BLE_COMBO_REF__)
    if ((update_conf->update_type == OTA_TYPE_BLE_FW)
        || (update_conf->update_type == OTA_TYPE_BLE_COMBO)) {
        ota_proc->progress_ble = 0;
    }
#endif // __BLE_COMBO_REF__

    ota_proc->update_type = update_conf->update_type;
    memcpy(ota_proc->url, update_conf->url, OTA_HTTP_URL_LEN);
#if defined (__BLE_COMBO_REF__)
    ota_update_ble_determine_url(update_conf);
    if (update_conf->url_ble_fw != NULL && strlen(update_conf->url_ble_fw)) {
        memcpy(ota_proc->url_ble_fw, update_conf->url_ble_fw, OTA_HTTP_URL_LEN);
    }
#endif //(__BLE_COMBO_REF__)
    ota_proc->auto_renew = update_conf->auto_renew;
    ota_proc->download_sflash_addr = update_conf->download_sflash_addr;
    ota_proc->download_notify = update_conf->download_notify;
    ota_proc->renew_notify = update_conf->renew_notify;

    if (ota_proc_xHandle) {
        vTaskDelete(ota_proc_xHandle);
        ota_proc_xHandle = NULL;
    }

    xReturned = xTaskCreate(ota_update_process,
                            OTA_TASK_NAME,
                            OTA_TASK_STACK_SZ,
                            NULL,
                            OS_TASK_PRIORITY_USER,
                            &ota_proc_xHandle);

    if (xReturned != pdPASS) {
        OTA_ERR(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, OTA_TASK_NAME);
        return OTA_FAILED;
    }

    return OTA_SUCCESS;
}

static void test_cmd_download_notify(ota_update_type update_type, UINT status, UINT progress)
{
#if !defined (ENABLE_OTA_DBG)
    DA16X_UNUSED_ARG(update_type);
    DA16X_UNUSED_ARG(status);
    DA16X_UNUSED_ARG(progress);
#endif //!(ENABLE_OTA_DBG)

    OTA_DBG("[%s] update_type = %d, status = 0x%02x, progress = %d\n", __func__, update_type, status, progress);
}

static void test_cmd_renew_notify(UINT status)
{
#if !defined (ENABLE_OTA_DBG)
    DA16X_UNUSED_ARG(status);
#endif //!(ENABLE_OTA_DBG)

    OTA_DBG("[%s] status = 0x%02x\n", __func__, status);
}

static UINT ota_update_download_test_cmd(ota_update_type update_type, char *url)
{
    UINT status = OTA_SUCCESS;
    OTA_UPDATE_CONFIG *ota_update_conf = NULL;

    if ((url == NULL) || (strlen((char *)url) <= 0)) {
        return OTA_FAILED;
    }

    ota_update_conf = OTA_MALLOC(sizeof(OTA_UPDATE_CONFIG));
    if (ota_update_conf == NULL) {
        OTA_ERR("[%s] Failed to alloc memory\n", __func__);
        return OTA_FAILED;
    }
    memset(ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));
    ota_update_conf->update_type = update_type;
#if defined (__BLE_COMBO_REF__)
    if (update_type == OTA_TYPE_BLE_FW) {
        memcpy(ota_update_conf->url_ble_fw, url, OTA_HTTP_URL_LEN);
    } else {
        memcpy(ota_update_conf->url, url, OTA_HTTP_URL_LEN);
    }
#else
    memcpy(ota_update_conf->url, url, OTA_HTTP_URL_LEN);
#endif //(__BLE_COMBO_REF__)
    ota_update_conf->download_notify = test_cmd_download_notify;

    /* OTA FW download API */
    status = ota_update_start_download(ota_update_conf);

    if (ota_update_conf != NULL) {
        OTA_FREE(ota_update_conf);
    }
    return status;
}

static UINT ota_update_renew_test_cmd(void)
{
    UINT status = OTA_SUCCESS;
    OTA_UPDATE_CONFIG *ota_update_conf = NULL;

    ota_update_conf = OTA_MALLOC(sizeof(OTA_UPDATE_CONFIG));

    if (ota_update_conf == NULL) {
        OTA_ERR("[%s] Failed to alloc memory\n", __func__);
        return OTA_FAILED;
    }

    memset(ota_update_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));

    ota_update_conf->renew_notify = test_cmd_renew_notify;

    status = ota_update_start_renew(ota_update_conf);

    if (ota_update_conf != NULL) {
        OTA_FREE(ota_update_conf);
    }

    return status;
}

UINT ota_update_cli_cmd_parse(int argc, char *argv[])
{
    if ((argc == 1) || (strcasecmp("help", argv[1]) == 0)) {
        goto __ota_cmd_help;
    }

    if ((strcasecmp("rtos", argv[1]) == 0) && (argc == 3)) {
        return ota_update_download_test_cmd(OTA_TYPE_RTOS, argv[2]);

    } else if (strcasecmp("stop", argv[1]) == 0) {
        return ota_update_process_stop();
#if defined (__OTA_UPDATE_MCU_FW__)
    } else if (strcasecmp("mcu_fw", argv[1]) == 0) {
        if ((argv[3] != NULL) && argc == 4) {
            ota_update_set_mcu_fw_name(argv[3]);
        }
        return ota_update_download_test_cmd(OTA_TYPE_MCU_FW, argv[2]);
#endif //(__OTA_UPDATE_MCU_FW__)
#if defined (__BLE_COMBO_REF__)
    } else if (strcasecmp("ble_fw", argv[1]) == 0) {
        return ota_update_download_test_cmd(OTA_TYPE_BLE_FW, argv[2]);
#endif //(__BLE_COMBO_REF__)

    } else if ((strcasecmp("cert_key", argv[1]) == 0) && (argc == 3)) {
        return ota_update_download_test_cmd(OTA_TYPE_CERT_KEY, argv[2]);

    } else if (strcasecmp("renew", argv[1]) == 0) {
        return ota_update_renew_test_cmd();

    } else if ((strcasecmp("addr", argv[1]) == 0) && (argc == 3)) {
        UINT sflash_addr, fw_type;
        if (strcasecmp("rtos", argv[2]) == 0) {
            fw_type = OTA_TYPE_RTOS;
        } else if (strcasecmp("mcu_fw", argv[2]) == 0) {
            fw_type = OTA_TYPE_MCU_FW;
        }
#if defined (__BLE_COMBO_REF__)
        else if (strcasecmp("ble_fw", argv[2]) == 0) {
            fw_type = OTA_TYPE_BLE_FW;
        }
#endif //(__BLE_COMBO_REF__)
        else if (strcasecmp("cert_key", argv[2]) == 0) {
            fw_type = OTA_TYPE_CERT_KEY;
        } else {
            return OTA_FAILED;
        }

        sflash_addr = ota_update_get_new_sflash_addr(fw_type);
        PRINTF("Download area of %s is 0x%x.\n", argv[2], sflash_addr);
        return OTA_SUCCESS;

    } else if (strcasecmp("sflash_addr", argv[1]) == 0) {
        UINT addr;
        char *end = NULL;
        if ((argc == 3) && (argv[2] != NULL)) {
            addr = strtol(argv[2], &end, 16);
            return ota_update_set_user_sflash_addr(addr);
        }

    } else if (strcasecmp("info", argv[1]) == 0) {
#if defined (__OTA_UPDATE_MCU_FW__)
        char name[8];
        UINT size, crc, addr;
#endif // (__OTA_UPDATE_MCU_FW__)
        DA16X_IMGHEADER_TYPE infoImage;

        PRINTF(" * OS(%d %)\n", ota_update_get_progress(OTA_TYPE_RTOS));

        if (ota_update_get_image_info(SFLASH_RTOS_0_BASE, &infoImage)) {
            PRINTF("- [0] RTOS (addr = 0x%x)\n", SFLASH_RTOS_0_BASE);
            ota_update_print_fw_header(&infoImage);
        }

        if (ota_update_get_image_info(SFLASH_RTOS_1_BASE, &infoImage)) {
            PRINTF("- [1] RTOS (addr = 0x%x)\n", SFLASH_RTOS_1_BASE);
            ota_update_print_fw_header(&infoImage);
        }
        PRINTF("\n");
#if defined (__OTA_UPDATE_MCU_FW__)
        //MCU FW info
        addr = ota_update_get_curr_sflash_addr(OTA_TYPE_MCU_FW);
        memset(name, 0, 8);
        ota_update_get_mcu_fw_info(name, &size, &crc);
        PRINTF("- MCU FW (addr = 0x%x)\n", addr);
        PRINTF(" Name-------------%s \n", name);
        PRINTF(" Size-------------%d \n", size);
        PRINTF(" CRC--------------0x%x \n\n", crc);
#endif //(__OTA_UPDATE_MCU_FW__)

#if defined(__BLE_COMBO_REF__)
        {
            ble_img_hdr_t bank1_info, bank2_info;
            int8_t  active_bank_id = -1;

            ota_update_ble_get_fw_info(&bank1_info, &bank2_info, &active_bank_id);

            if (bank1_info.imageid == active_bank_id) {
                PRINTF("- BLE FW INFO - Bank #%d (Active)\n", bank1_info.imageid);
            } else {
                PRINTF("- BLE FW INFO - Bank #%d\n", bank1_info.imageid);
            }
            PRINTF(" Version--------- %s\n", bank1_info.version);
            PRINTF(" Timestamp------- %d \n", bank1_info.timestamp);
            PRINTF(" Size------------ %d \n", bank1_info.code_size);
            PRINTF(" CRC------------- 0x%x \n", bank1_info.CRC);
            PRINTF(" Encryption------ %d \n\n", bank1_info.encryption);

            if (bank2_info.imageid == active_bank_id) {
                PRINTF("- BLE FW INFO - Bank #%d (Active)\n", bank2_info.imageid);
            } else {
                PRINTF("- BLE FW INFO - Bank #%d\n", bank2_info.imageid);
            }
            PRINTF(" Version--------- %s\n", bank2_info.version);
            PRINTF(" Timestamp------- %d \n", bank2_info.timestamp);
            PRINTF(" Size------------ %d \n", bank2_info.code_size);
            PRINTF(" CRC------------- 0x%x \n", bank2_info.CRC);
            PRINTF(" Encryption------ %d \n\n", bank2_info.encryption);
        }
#endif //( __BLE_COMBO_REF__)
        return OTA_SUCCESS;

    } else if (strcasecmp("crc", argv[1]) == 0) {
        UINT addr, size;
        char *end = NULL;
        if (argv[2] != NULL) {
            addr = strtol(argv[2], &end, 16);
            size = ota_update_sflash_crc(addr);

            if (size > 0) {
                PRINTF("HW-CRC: SUCCESS : %dbyte\n", size);
            } else {
                PRINTF("HW-CRC: FAILED\n");
            }
            return OTA_SUCCESS;
        }

    } else if (strcasecmp("get_boot_index", argv[1]) == 0) {
        PRINTF("Current boot index = %d \n", get_cur_boot_index());
        return OTA_SUCCESS;

    } else if (strcasecmp("toggle_boot_index", argv[1]) == 0) {
        /* Toggle the boot index */
        if (ota_update_toggle_boot_index() == OTA_SUCCESS) {
            return OTA_SUCCESS;
        }

    } else if ((strcasecmp("read_sflash", argv[1]) == 0) && (argc == 4)) {
        UCHAR *rd_buf = NULL;
        UINT addr, len;
        char *end = NULL;

        addr = strtol(argv[2], &end, 16);
        len = atoi(argv[3]);
        rd_buf = OTA_MALLOC(len);

        if (rd_buf != NULL) {
            if (ota_update_read_flash(addr, rd_buf, len) == OTA_SUCCESS) {
                hex_dump(rd_buf, len);
            } else {
                PRINTF("Failed to read.\n");
            }
            OTA_FREE(rd_buf);
        }
        return OTA_SUCCESS;

    } else if ((strcasecmp("copy_sflash", argv[1]) == 0) && (argc == 5)) {
        UINT dst_addr, src_addr, len;
        char *end = NULL;

        dst_addr = strtol(argv[2], &end, 16);
        src_addr = strtol(argv[3], &end, 16);
        len = atoi(argv[4]);
        if (ota_update_copy_flash(dst_addr, src_addr, len) != OTA_SUCCESS) {
            PRINTF("Failed to copy.\n");
        }
        return OTA_SUCCESS;

    } else if ((strcasecmp("erase_sflash", argv[1]) == 0) && (argc == 4)) {
        UINT addr, len;
        char *end = NULL;

        addr = strtol(argv[2], &end, 16);
        len = atoi(argv[3]);
        if (ota_update_erase_flash(addr, len) <= 0) {
            PRINTF("Failed to erase.\n");
        }
        return OTA_SUCCESS;
#if defined (__OTA_UPDATE_MCU_FW__)
    } else if ((strcasecmp("set_name_mcu", argv[1]) == 0) && (argc == 3)) {
        if (ota_update_set_mcu_fw_name(argv[2]) != OTA_SUCCESS) {
            PRINTF("Failed to set the name of MCU FW.\n");
        }
        return OTA_SUCCESS;

    } else if (strcasecmp("get_name_mcu", argv[1]) == 0) {
        char name[8];
        if (ota_update_get_mcu_fw_name(name) != OTA_SUCCESS) {
            PRINTF("Failed to get the name of MCU FW\n");
        } else {
            PRINTF("Name = %s(len=%d).\n", name, strlen(name));
        }
        return OTA_SUCCESS;

    } else if ((strcasecmp("read_mcu", argv[1]) == 0) && (argc == 4)) {
        UINT addr, len;
        char *end = NULL;
        addr = strtol(argv[2], &end, 16);
        len = atoi(argv[3]);
        if (ota_update_read_mcu_fw(addr, len) != OTA_SUCCESS) {
            PRINTF("Failed to read.\n");
        }
        return OTA_SUCCESS;

    } else if (strcasecmp("trans_mcu", argv[1]) == 0) {
        if (ota_update_trans_mcu_fw() != OTA_SUCCESS) {
            PRINTF("Failed to trans.\n");
        }
        return OTA_SUCCESS;

    } else if (strcasecmp("erase_mcu", argv[1]) == 0) {
        if (ota_update_erase_mcu_fw() != OTA_SUCCESS) {
            PRINTF("Failed to erase.\n");
        }
        return OTA_SUCCESS;
#endif // (__OTA_UPDATE_MCU_FW__)
    }

__ota_cmd_help:

    PRINTF("\n");
    PRINTF("ota_update [fw_type] [url] \t: Start to FW download. \n");
    PRINTF("\t\t\t\t  * fw_type \n");
    PRINTF("\t\t\t\t    rtos : Fw_type of RTOS \n");
    PRINTF("\t\t\t\t    cert_key : update_type of cert or key.\n");
#if defined (__OTA_UPDATE_MCU_FW__)
    PRINTF("\t\t\t\t    mcu_fw : Fw_type of MCU FW. \n");
#endif // (__OTA_UPDATE_MCU_FW__)
    PRINTF("\t\t\t\t  * url  : Server URL where FW exists \n");
    PRINTF("\t\t\t\t  ex) ota_update rtos http://192.168.0.1/rtos.img \n");
    PRINTF("\n");

    PRINTF("ota_update stop \t\t: Stop to FW download. \n");
    PRINTF("\t\t\t\t  ex) ota_update stop \n");
    PRINTF("\n");

    PRINTF("ota_update renew \t\t: Change current FW to new FW. \n");
    PRINTF("\t\t\t\t  ex) ota_update renew \n");
    PRINTF("\n");

    PRINTF("ota_update info \t\t: Show FW information. \n");
    PRINTF("\t\t\t\t  ex) ota_update info \n");
    PRINTF("\n");

    PRINTF("ota_update crc [addr] \t\t: Check CRC of FW. \n");
    PRINTF("\t\t\t\t  ex) ota_update crc 0x23000 \n");
    PRINTF("\n");

    PRINTF("ota_update read_sflash [addr] [size]\n");
    PRINTF("\t\t\t\t  : Read sflash data.\n");
    PRINTF("\t\t\t\t  ex) ota_update read_sflash 0x3ad000 128\n");
    PRINTF("\n");

    PRINTF("ota_update copy_sflash [dst_addr] [src_addr] [size]\n");
    PRINTF("\t\t\t\t  : Copy from sflash data src_add to dst_add.\n");
    PRINTF("\t\t\t\t  ex) ota_update copy_sflash 0x3ad000 0x3ae000 4096\n");
    PRINTF("\n");

    PRINTF("ota_update erase_sflash [addr] [size]\n");
    PRINTF("\t\t\t\t  : Erase sflash data.\n");
    PRINTF("\t\t\t\t  ex) ota_update erase_sflash 0x3ad000 4096\n");
    PRINTF("\n");
#if defined (__OTA_UPDATE_MCU_FW__)
    PRINTF("ota_update set_name_mcu \t\t: Set the name(version) of MCU FW to be downloaded to sflash. \n");
    PRINTF("\t\t\t\t  ex) ota_update set_name_mcu MCU_FW \n");
    PRINTF("\n");

    PRINTF("ota_update get_name_mcu \t\t: Get name(version) of MCU FW downloaded to sflash. \n");
    PRINTF("\t\t\t\t  ex) ota_update get_name_mcu \n");
    PRINTF("\n");

    PRINTF("ota_update read_mcu [addr] [size]\n");
    PRINTF("\t\t\t\t  : Read the firmware as much as the size from the read_addr and transmit it.\n");
    PRINTF("\t\t\t\t  ex) ota_update read_mcu 0x3ad000 128\n");
    PRINTF("\n");

    PRINTF("ota_update trans_mcu \t\t: Transmit a firmware to MCU through UART. \n");
    PRINTF("\t\t\t\t  ex) ota_update trans_mcu \n");
    PRINTF("\n");

    PRINTF("ota_update erase_mcu \t\t: Erase the firmware stored in serial flash of DA16200. \n");
    PRINTF("\t\t\t\t  ex) ota_update erase_mcu \n");
    PRINTF("\n");
#endif // (__OTA_UPDATE_MCU_FW__)
    PRINTF("ota_update get_boot_index \t: Get current boot index info.\n");
    PRINTF("\t\t\t\t  ex) ota_update get_boot_index \n");
    PRINTF("ota_update toggle_boot_index \t: Toggle boot index.\n");
    PRINTF("\t\t\t\t  ex) ota_update toggle_boot_index \n");
    PRINTF("\n");

    return OTA_FAILED;
}

/* Write the downloaded data directly to sflash. */
UINT ota_update_write_sflash(ota_update_sflash_t *sflash_ctx,
                             UCHAR *data, UINT length)
{
    UINT status = OTA_SUCCESS;
    UINT buff_offset = 0;
    UINT copyToBufLen = 0;
    UINT writeToFlashLen = 0;
    UINT input_len = 0;
    UCHAR *input_data = NULL;

    if (length == 0) {
        OTA_ERR("[%s:%d] Data length error\n", __func__, __LINE__);
        status = OTA_FAILED;
        goto finish;
    }

    if (sflash_ctx->buffer == NULL) {
        sflash_ctx->buffer = (UCHAR *)OTA_MALLOC(OTA_SFLASH_BUF_SZ);
        if (sflash_ctx->buffer == NULL) {
            OTA_ERR("[%s:%d] Failed to allocate receive buffer(%d bytes)\n", __func__, __LINE__,
                    OTA_SFLASH_BUF_SZ);
            status = OTA_MEM_ALLOC_FAILED;
            goto finish;
        }
    }

    input_data = data;
    input_len = length;

    sflash_ctx->length += input_len;

    if (sflash_ctx->offset > 0 ) {
        writeToFlashLen = sflash_ctx->offset;
    }

    writeToFlashLen += input_len;

    while (writeToFlashLen > 0) {
        copyToBufLen = OTA_SFLASH_BUF_SZ - sflash_ctx->offset;

        if (copyToBufLen > (input_len - buff_offset)) {
            copyToBufLen = (input_len - buff_offset);
        }

        if (copyToBufLen > 0) {
            memcpy(&sflash_ctx->buffer[sflash_ctx->offset],
                   (input_data + buff_offset),
                   copyToBufLen);
            buff_offset += copyToBufLen;
            sflash_ctx->offset += copyToBufLen;
        }

        if ((sflash_ctx->offset == OTA_SFLASH_BUF_SZ)
                || (sflash_ctx->offset == sflash_ctx->total_length)
                || ((sflash_ctx->length == sflash_ctx->total_length) && (sflash_ctx->offset > 0))) {
            //APP_CORE
            if (ota_proc->update_type == OTA_TYPE_MCU_FW_STREAM) {
                if (writeDataToMCU(sflash_ctx->sflash_addr, (UINT)sflash_ctx->total_length
                    , sflash_ctx->length, (UINT32 *)&sflash_ctx->buffer[0]
                    , (UINT)OTA_SFLASH_BUF_SZ) != OTA_SFLASH_BUF_SZ) {
                    OTA_ERR("[%s:%d] MCU write failed\n", __func__, __LINE__);
                    status = OTA_FAILED;
                    goto finish;
                }
            } else {
            /* sflash write */
                if (writeDataToFlash(sflash_ctx->sflash_addr,
                                     &sflash_ctx->buffer[0],
                                     (UINT)OTA_SFLASH_BUF_SZ) != TRUE) {
                    OTA_ERR("[%s:%d] SFLASH write failed\n", __func__, __LINE__);
                    status = OTA_FAILED;
                    goto finish;
                }
            }

            sflash_ctx->sflash_addr += OTA_SFLASH_BUF_SZ;

            memset(sflash_ctx->buffer, 0x00, OTA_SFLASH_BUF_SZ);
            sflash_ctx->offset = 0;

            if (writeToFlashLen >= OTA_SFLASH_BUF_SZ) {
                writeToFlashLen = writeToFlashLen - OTA_SFLASH_BUF_SZ;
            }
        } else {
            break;
        }
    }

finish:

    if ((status != OTA_SUCCESS)
            || (sflash_ctx->length == sflash_ctx->total_length)) {
        sflash_ctx->sflash_addr = 0x00;
        sflash_ctx->total_length = 0;
        sflash_ctx->length = 0;
        sflash_ctx->offset = 0;

        if (sflash_ctx->buffer != NULL) {
            OTA_FREE(sflash_ctx->buffer);
            sflash_ctx->buffer = NULL;
        }
    }

    return status;
}

#endif	// (__SUPPORT_OTA__)

/* EOF */
