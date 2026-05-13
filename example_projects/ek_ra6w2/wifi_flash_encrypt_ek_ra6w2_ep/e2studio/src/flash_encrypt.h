/***********************************************************************************************************************
 * File Name    : flash_encrypt.h
 * Description  : CC312-encrypted string store/load targeting
 *                the SFlash User Area (SF_USER_AREA).
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#ifndef FLASH_ENCRYPT_H_
#define FLASH_ENCRYPT_H_

#include "hal_data.h"
#include "cc_util_asset_prov_int.h"

/* ---- Sizing ---- */
#define FLASH_ASSET_MAX_SIZE   (200U) /* Maximum string length incl. null terminator */
#define FLASH_SLOT_SIZE        (256U) /* Flash slot size (page-aligned)              */

/*
 * Flash address for the plain string slot.
 * Set to SF_USER_AREA + 0x100 so it sits immediately after the
 * first encrypted asset slot (UA_ASSET_APP_INFO @ SF_USER_AREA + 0x000).
 */
#define FLASH_ADDR       (SF_USER_AREA + 0x1000U)

/* ---- CC312 encryption configuration ---- */
#define FLASH_PACKAGE_HDR_SIZE (48U)   /* CC312 AES-GCM package overhead           */
#define FLASH_ASSET_ID         (0x1234U) /* Unique CC312 Asset ID for string slot  */

#define FLASH_USER_KEY         (0x1) /* Use User Key in non-secure LCS (CM & DM).                                                             */
#define FLASH_KCP_KEY          (0x2) /* Use KCP Key on OTP in secure LCS after applying secure boot.                                           */
#define FLASH_ADAPTIVE_KEY     (0x3) /* Use User key in non-secure LCS on the development stage and KCP key in secure LCS on the production stage. */
#define FLASH_KEY_TYPE         FLASH_ADAPTIVE_KEY

/* ---- Public API ---- */

/**
 * Encrypt a null-terminated string with CC312 and write it to the
 * SFlash User Area slot at FLASH_ADDR.
 *
 * @param[in] p_str  Null-terminated string to encrypt and store
 *                   (max FLASH_ASSET_MAX_SIZE - 1 chars).
 *
 * @retval FSP_SUCCESS              Encrypted and written successfully.
 * @retval FSP_ERR_WRITE_FAILED     SFlash write error.
 * @retval FSP_ERR_OUT_OF_MEMORY    Heap allocation failure.
 * @retval FSP_ERR_INVALID_ARGUMENT NULL pointer, string too long, or CC312 error.
 */
fsp_err_t flash_write(const char *p_str);

/**
 * Read and CC312-decrypt a string from the SFlash User Area slot at FLASH_ADDR.
 *
 * @param[out] p_buf     Destination buffer for the decrypted string.
 * @param[in]  buf_size  Size of p_buf in bytes (max FLASH_ASSET_MAX_SIZE).
 *
 * @retval FSP_SUCCESS              Decrypted successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT NULL pointer, invalid buf_size, or CC312 error.
 * @retval FSP_ERR_OUT_OF_MEMORY    Heap allocation failure.
 */
fsp_err_t flash_read(char *p_buf, uint32_t buf_size);

/**
 * Run a self-contained write-then-read test.
 * Call this from app_task_entry().
 */
void flash_test_run(void);

#endif /* FLASH_ENCRYPT_H_ */
