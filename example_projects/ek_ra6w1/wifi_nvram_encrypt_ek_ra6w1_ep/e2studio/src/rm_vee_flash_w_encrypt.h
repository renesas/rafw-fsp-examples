/***********************************************************************************************************************
 * File Name    : nvram_encrypt.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef RM_VEE_FLASH_W_ENCRYPT_H_
#define NVRAM_ENCRYPT_H_

#include "hal_data.h"
#include "cc_util_asset_prov_int.h"
#define SECURE_ASSET_OTP_ENABLE (0)

#define SECURE_ASSET_APP_INFO (0)
#define SECURE_ASSET_AT_KEY (1)
#define SECURE_ASSET_BASE_ID (1901)

#if SECURE_ASSET_OTP_ENABLE
#define SECURE_ASSET_OTP_START 0x1C0
#endif //SECURE_ASSET_OTP_ENABLE
#define SECURE_ASSET_MAX_SIZE 128
#define CC_ASSET_PROV_TOKEN 0x41736574UL
#define CC_PRINTF(...) printf(__VA_ARGS__)
#define ASSET_ID 0x1234

#define USER_KEY_FOR_ASSET 0x1
#define KCP_KEY_FOR_ASSET 0x2
#define ADAPTIVE_KEY_FOR_ASSET 0x3
#define KEY_TYPE ADAPTIVE_KEY_FOR_ASSET

typedef enum
{
    SECURE_ASSET_MODE_USER,
    SECURE_ASSET_MODE_PROD
} secure_asset_mode_t;

typedef enum
{
    STORAGE_TYPE_OTP = 0,
    STORAGE_TYPE_FLASH = 1,
    STORAGE_TYPE_VEE = 2
} secure_storage_type_t;

/* Asset package helper functions and structures */
typedef struct st_secure_asset_ctx
{
    uint32_t asset_size; //Original asset size
    uint32_t aligned_size; //16-byte aligned size
    uint32_t package_size; //Total package size including header
    uint8_t *asset_buffer; //Buffer for asset data
    uint8_t *package_buffer; //Buffer for complete package
} secure_asset_ctx_t;

typedef struct
{
    uint32_t token;
    uint32_t version;
    uint32_t assetSize;
    uint32_t reserved[CC_ASSET_PROV_RESERVED_WORD_SIZE];
    uint8_t nonce[CC_ASSET_PROV_NONCE_SIZE];
    uint8_t enctag[CC_ASSET_PROV_TAG_SIZE];
} CCRunAssetProvPkg_t;

void secure_asset_test_run(void);

#endif //RM_VEE_FLASH_W_ENCRYPT_H_
