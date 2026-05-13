/***********************************************************************************************************************

* File Name    : secure_storage.c

* Description  : Functions for secure storage

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "rm_atcmd_w_cfg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "r_cc312_common.h"
#include "rm_vee_flash_w.h"
#include "bsp_sflash_map_ra6w1.h"
#include "bsv_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "mbedtls/platform.h"
#if defined(MBEDTLS_THREADING_C)
#include "mbedtls/threading.h"
#endif
#include "psa/crypto.h"
#include "psa/crypto_extra.h"
#if BSP_FEATURE_CRYPTO_HAS_CC312
#include "r_cc312_crypto.h"
#include "cc_prod_error.h"
#endif

#include "rm_vee_flash_w.h"
#include "bsp_sflash_map_ra6w1.h"
#include "secure_storage.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define SECURE_ASSET_OTP_START      0x1C0
#define SECURE_ASSET_MAX_SIZE       128
#define CC_ASSET_PROV_TOKEN         0x41736574UL
#define CC_PRINTF(...)              printf(__VA_ARGS__)
#define ASSET_ID                    0x1234

#define USER_KEY_FOR_ASSET          0x1
#define KCP_KEY_FOR_ASSET           0x2
#define ADAPTIVE_KEY_FOR_ASSET      0x3
#define KEY_TYPE                    ADAPTIVE_KEY_FOR_ASSET

#if KEY_TYPE == USER_KEY_FOR_ASSET
#define ASSET_KEY_TYPE              ASSET_USER_KEY
#define ASSET_KEY_DATA              &userKeyData
#elif KEY_TYPE == KCP_KEY_FOR_ASSET
#define ASSET_KEY_TYPE              SSET_KCP_KEY
#define ASSET_KEY_DATA              NULL
#elif KEY_TYPE == ADAPTIVE_KEY_FOR_ASSET
#define USER_ASSET_KEY_TYPE         ASSET_USER_KEY
#define USER_ASSET_KEY_DATA         &userKeyData
#define SECURE_ASSET_KEY_TYPE       ASSET_KCP_KEY
#define SECURE_ASSET_KEY_DATA       NULL
#endif
#include "cc_util_asset_prov_int.h"

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef struct
{
    uint32_t token;
    uint32_t version;
    uint32_t assetSize;
    uint32_t reserved[CC_ASSET_PROV_RESERVED_WORD_SIZE];
    uint8_t nonce[CC_ASSET_PROV_NONCE_SIZE];
    uint8_t enctag[CC_ASSET_PROV_TAG_SIZE];
} CCRunAssetProvPkg_t;

/* Asset package helper functions and structures */
typedef struct st_secure_asset_ctx
{
    uint32_t asset_size;      // Original asset size
    uint32_t aligned_size;    // 16-byte aligned size
    uint32_t package_size;    // Total package size including header
    uint8_t *asset_buffer;    // Buffer for asset data
    uint8_t *package_buffer;  // Buffer for complete package
} secure_asset_ctx_t;

/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/
extern rm_vee_flash_w_instance_ctrl_t g_vee0_ctrl;

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
/* Only test key instead of KCP */
const uint8_t test_key[] = {0xd5, 0xe9, 0xda, 0x41, 0xa6, 0x5b, 0x7f, 0xd2, 0xe5, 0xad, 0xf4, 0xb8, 0xf8, 0x43, 0x25, 0x3f};
AssetUserKeyData_t userKeyData =
{
    .pKey    = (uint8_t *) test_key,
    .keySize = 16,
};

#if RM_PSA_CRYPTO_WIFI_UTIL
bool util_sflash_write(int sflash_addr, char *wr_buf, int len);
#endif

/***********************************************************************************************************************
 * Private Functions Prototypes
 **********************************************************************************************************************/
static bool is_otp_words_zero(uint32_t start_offset, uint32_t count);
static fsp_err_t secure_asset_storage_read(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx);
static fsp_err_t secure_asset_storage_write(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx);
void print_CCRunAssetProvPkg(const CCRunAssetProvPkg_t *dest);
static fsp_err_t secure_asset_init_context(secure_asset_ctx_t * p_ctx, uint32_t size);
static void secure_asset_free_context(secure_asset_ctx_t * p_ctx);
static void HEX_PRINTF(uint8_t * data, uint32_t size);
static uint32_t secure_asset_storage_store(uint32_t  storage_type,
                                           uint32_t  assetID, uint8_t * Payload, uint32_t  PayloadSize);
static uint32_t secure_asset_storage_load(uint32_t  storage_type,
                                          uint32_t  assetID, uint8_t * Payload, uint32_t  PayloadSize);
static uint32_t secure_asset_vee_delete(uint32_t assetID);

/***********************************************************************************************************************
 * Functions 
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Store secured asset data to Production(OTP or FLASH)
 *
 * @param[in]  storage_type Storage Type (SECURE_ASSET_FACTORY_STORAGE_OTP or SECURE_ASSET_FACTORY_STORAGE_FLASH)
 * @param[in]  asset_id     Asset ID (SECURE_ASSET_APP_INFO or SECURE_ASSET_AT_KEY)
 * @param[in]  p_data       Pointer to data to store
 * @param[in]  length       Length of data in bytes
 *
 * @retval FSP_SUCCESS          Successfully stored data to OTP
 * @retval FSP_ERR_WRITE_FAILED OTP write operation failed
 * @retval FSP_ERR_INVALID_ARGUMENT Parameter check failed
 **********************************************************************************************************************/
fsp_err_t R_RED_SecureAssetProdStore(uint32_t storage_type, uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;
    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = secure_asset_storage_store(storage_type, asset_id, p_data, length);
    if (FSP_SUCCESS != err)
    {
        return FSP_ERR_WRITE_FAILED;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Load secured asset data from production data
 *
 * @param[in]  storage_type Storage Type (SECURE_ASSET_FACTORY_STORAGE_OTP or SECURE_ASSET_FACTORY_STORAGE_FLASH)
 * @param[in]  asset_id     Asset ID (SECURE_ASSET_APP_INFO or SECURE_ASSET_AT_KEY)
 * @param[out] p_data       Buffer to store restored data
 * @param[in]  length       Length of data to restore in bytes
 *
 * @retval FSP_SUCCESS           Successfully restored data from OTP
 * @retval FSP_ERR_NOT_FOUND     Data not found in OTP
 * @retval FSP_ERR_INVALID_ARGUMENT Parameter check failed
 **********************************************************************************************************************/
fsp_err_t R_RED_SecureAssetProdLoad(uint32_t storage_type, uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;
    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = secure_asset_storage_load(storage_type, asset_id, p_data, length);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_NOT_FOUND;
}

/*******************************************************************************************************************//**
 * Store secured asset data with layered storage approach(RM_VEE_FLASH)
 * Stores data to Flash storage
 *
 * @param[in]  asset_id    (2 ~ 50) : Asset ID
 *                         (0 ~ 1)  : Reserved Asset ID (SECURE_ASSET_APP_INFO or SECURE_ASSET_AT_KEY)
 * @param[in]  p_data      Pointer to data to store
 * @param[in]  length      Length of data in bytes
 *
 * @retval FSP_SUCCESS           Successfully stored data
 * @retval FSP_ERR_WRITE_FAILED  Storage operation failed
 * @retval FSP_ERR_INVALID_ARGUMENT Parameter check failed
 **********************************************************************************************************************/
fsp_err_t R_RED_SecureAssetStore(uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = secure_asset_storage_store(SECURE_ASSET_USER_STORAGE_VEE, asset_id, p_data, length);
    if (FSP_SUCCESS != err)
    {
        return FSP_ERR_WRITE_FAILED;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************/ /**
 * Load secured asset data with layered retrieval (Production -> RM_VEE_FLASH)
 * First tries to read production secured data from OTP or FLASH, if that fails then tries VEE
 *
 * @param[in]  asset_id    (2 ~ 50) : Asset ID
 *                         (0 ~ 1)  : Reserved Asset ID (SECURE_ASSET_APP_INFO or SECURE_ASSET_AT_KEY)
 * @param[out] p_data      Buffer to store restored data
 * @param[in]  length      Length of data to restore in bytes
 *
 * @retval FSP_SUCCESS           Successfully restored data
 * @retval FSP_ERR_NOT_FOUND     Data not found in any storage
 * @retval FSP_ERR_INVALID_ARGUMENT Parameter check failed
 **********************************************************************************************************************/
fsp_err_t R_RED_SecureAssetLoad(uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Try reading Flash first*/
    err = secure_asset_storage_load(SECURE_ASSET_USER_STORAGE_VEE, asset_id, p_data, length);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    /* Try reading from FACTORY FLASH second */
    err = secure_asset_storage_load(SECURE_ASSET_FACTORY_STORAGE_FLASH, asset_id, p_data, length);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    /* Try reading from FACTORY OTP second */
    err = secure_asset_storage_load(SECURE_ASSET_FACTORY_STORAGE_OTP, asset_id, p_data, length);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_NOT_FOUND;
}

fsp_err_t R_RED_SecureAssetDelete(void)
{
    secure_asset_vee_delete(SECURE_ASSET_APP_INFO);
    secure_asset_vee_delete(SECURE_ASSET_AT_KEY);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Delete secure asset data from RM_VEE_FLASH storage
 *
 * @retval FSP_SUCCESS           Successfully deleted data
 * @retval FSP_ERR_NOT_FOUND     Data not found in Flash storage
 * @retval FSP_ERR_INVALID_ARGUMENT Parameter check failed
 **********************************************************************************************************************/
void secure_test(void)
{
    fsp_err_t err;
    uint32_t assetid = 0x1234;
    secure_asset_ctx_t ctx = {0, };
    const uint8_t asset_data[112] =
    {
        0x74, 0x65, 0x73, 0x41, 0x00, 0x00, 0x01, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xab, 0xff, 0x3f, 0xb0,
        0x14, 0xc1, 0x87, 0x49, 0xce, 0x40, 0xc9, 0x23,
        0x45, 0xf7, 0xf4, 0x00, 0x0a, 0xd3, 0x45, 0xd9,
        0x5d, 0x83, 0x37, 0x19, 0x23, 0x79, 0x3a, 0xc7,
        0x3f, 0xe8, 0x95, 0xed, 0x81, 0x16, 0x3c, 0x0d,
        0x8f, 0xc6, 0xe1, 0xf2, 0xac, 0x10, 0xea, 0x6c,
        0xa4, 0xb0, 0x96, 0x65, 0x94, 0xf5, 0x22, 0xef,
        0x79, 0x24, 0x09, 0x53, 0x7a, 0x1f, 0x4b, 0x96,
        0x96, 0xa7, 0xe5, 0xcb, 0x5b, 0xd3, 0x7c, 0x28,
        0xa2, 0x62, 0xa5, 0xa4, 0xbe, 0x3c, 0xc8, 0x48,
        0x5e, 0x5c, 0x24, 0xf9, 0x17, 0xd5, 0x89, 0x87,
        0x8d, 0x55, 0xba, 0xe5, 0x71, 0x1f, 0x78, 0x6a
    };
    uint32_t asset_data_size = sizeof(asset_data);

    /* Initialize context with required buffer sizes */
    err = secure_asset_init_context(&ctx, asset_data_size-48);
    if (err != FSP_SUCCESS)
    {
        return;
    }

    CC_PRINTF("## pay load ##\r\n");
    CC_PRINTF("PayloadSize = %u\n", asset_data_size);
    HEX_PRINTF((uint8_t *) asset_data, asset_data_size);

#if KEY_TYPE != ADAPTIVE_KEY_FOR_ASSET
    /* Create runtime package */
    int32_t pkg_size = R_CC312_Secure_Asset_RuntimeUnpack(ASSET_KEY_TYPE,
                                                          ASSET_KEY_DATA,
                                                          assetid,
                                                          (uint8_t *) asset_data,
                                                          asset_data_size,
                                                          ctx.asset_buffer);
#else
    CCError_t ccRc = CC_OK;
    uint32_t  lcs;
    int32_t   pkg_size;
    ccRc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);

    if (CC_OK == ccRc && CC_BSV_DEVICE_MANUFACTURE_LCS <= lcs)
    {
        pkg_size = R_CC312_Secure_Asset_RuntimeUnpack(SECURE_ASSET_KEY_TYPE,
                                                      SECURE_ASSET_KEY_DATA,
                                                      assetid,
                                                      ctx.package_buffer,
                                                      ctx.package_size,
                                                      ctx.asset_buffer);
    }
    else
    {
        pkg_size = R_CC312_Secure_Asset_RuntimeUnpack(USER_ASSET_KEY_TYPE,
                                                      USER_ASSET_KEY_DATA,
                                                      assetid,
                                                      ctx.package_buffer,
                                                      ctx.package_size,
                                                      ctx.asset_buffer);
    }
#endif
    CC_PRINTF("## assert pkg ##\r\n");
    CC_PRINTF("pkg_size = %d\n", pkg_size);
    HEX_PRINTF((uint8_t *) ctx.asset_buffer, (uint32_t)pkg_size);

    secure_asset_free_context(&ctx);
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Check if OTP words are all zero for given range
 *
 * @param[in] start_offset Starting word offset in OTP
 * @param[in] count        Number of words to check
 *
 * @retval true           All words are zero
 * @retval false          At least one word is non-zero
 **********************************************************************************************************************/
static bool is_otp_words_zero(uint32_t start_offset, uint32_t count)
{
    bool result = true;
    uint32_t i;

    /* Initialize OTP controller in read mode */
    bsp_otp_init();
    bsp_otp_mode_set(BSP_OTP_MODE_READ);

    /* Read words and check if all zero */
    for (i = 0; i < count; i++)
    {
        uint32_t word = bsp_otp_word_read(start_offset + i);
        if (word != 0)
        {
            result = false;
            break;
        }
    }

    /* Close OTP controller */
    bsp_otp_close();

    return result;
}

/*******************************************************************************************************************/ /**
 * Read default secure asset from OTP or flash memory
 *
 * @param[in] storage_type  Storage type (OTP or Flash)
 * @param[in] asset_id      Asset ID to read
 * @param[in] ctx           Context containing buffer and size
 **********************************************************************************************************************/
static fsp_err_t secure_asset_storage_read(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx)
{
    uint32_t  read_addr;
    uint32_t  otp_offset;
    uint32_t  word_length;

    /* Validate inputs */
    if (ctx == NULL || ctx->package_buffer == NULL)
    {
        return FSP_ERR_INVALID_POINTER;  // NULL pointer check
    }

    if (storage_type == SECURE_ASSET_FACTORY_STORAGE_OTP)
    {
        /* OTP Read */
        switch (asset_id)
        {
            case SECURE_ASSET_APP_INFO:
                otp_offset = SECURE_ASSET_OTP_START;
                break;
            case SECURE_ASSET_AT_KEY:
                otp_offset = SECURE_ASSET_OTP_START + (SECURE_ASSET_MAX_SIZE / 4);
                break;
            default:
                return FSP_ERR_INVALID_ARGUMENT;
        }

        /* Initialize OTP controller */
        bsp_otp_init();

        /* Calculate number of 32-bit words */
        word_length = (ctx->package_size) / 4;

        /* Read data from OTP */
        bsp_otp_read((uint32_t *) ctx->package_buffer, otp_offset, word_length);

        /* Close OTP controller */
        bsp_otp_close();
    }
    else if (storage_type == SECURE_ASSET_FACTORY_STORAGE_FLASH)
    {
        /* Flash Read - Direct memory access using XIP */
        switch (asset_id)
        {
            case SECURE_ASSET_APP_INFO:
                read_addr = SF_SECURE_ASSET_PROD;
                break;
            case SECURE_ASSET_AT_KEY:
                read_addr = SF_SECURE_ASSET_PROD + (SECURE_ASSET_MAX_SIZE);
                break;
            default:
                return FSP_ERR_INVALID_ARGUMENT;
        }

        /* Direct memory copy from flash */
        memcpy(
            ctx->package_buffer, (void *) (BSP_FEATURE_OSPI_W_DEVICE_0_START_ADDRESS | read_addr), ctx->package_size);
    }
#if RM_PSA_CRYPTO_VEE_FLASH
    else if (storage_type == SECURE_ASSET_USER_STORAGE_VEE)
    {
        fsp_err_t err;
        uint32_t length;
        uint8_t *rec_data_ptr;

        /* Get a pointer to the most recent data for ID 0. */
        err = RM_VEE_FLASH_W_RecordPtrGet(&g_vee0_ctrl, SECURE_ASSET_BASE_ID + asset_id, &rec_data_ptr, &length);
        if (err != FSP_SUCCESS)
        {
            return FSP_ERR_NOT_FOUND;  // Record not found
        }
        if (length != ctx->package_size)
        {
            return FSP_ERR_INVALID_SIZE;  // Size mismatch
        }

        /* Read package from flash*/
        memcpy(ctx->package_buffer, rec_data_ptr, ctx->package_size);
    }
#endif
    else
    {
        return FSP_ERR_INVALID_MODE;  // Invalid storage type
    }

    return FSP_SUCCESS;
}

static fsp_err_t secure_asset_storage_write(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx)
{
    uint32_t otp_offset;
    uint32_t word_length;

    /* Validate inputs */
    if (ctx == NULL || ctx->package_buffer == NULL)
    {
        return FSP_ERR_INVALID_POINTER;
    }

    if (storage_type == SECURE_ASSET_FACTORY_STORAGE_OTP)
    {
        bool result;

        switch (asset_id)
        {
            case SECURE_ASSET_APP_INFO:
                otp_offset = SECURE_ASSET_OTP_START;
                break;

            case SECURE_ASSET_AT_KEY:
                otp_offset = SECURE_ASSET_OTP_START + (SECURE_ASSET_MAX_SIZE / 4);
                break;

            default:
                return FSP_ERR_INVALID_ARGUMENT;
        }
        /* Initialize OTP controller */
        bsp_otp_init();
        word_length = ctx->package_size / 4;

        bsp_otp_mode_set(BSP_OTP_MODE_READ);
        if (is_otp_words_zero(otp_offset, word_length))
        {
            /* OTP words are all zero, proceed with programming */
            bsp_otp_mode_set(BSP_OTP_MODE_PROG);
        }
        else
        {
            /* OTP words are not zero, cannot program */
            return FSP_ERR_WRITE_FAILED;
        }

        /* Program data to OTP with verification */
        result = bsp_otp_prog_and_verify((uint32_t *) ctx->package_buffer, otp_offset, word_length);
        bsp_otp_mode_set(BSP_OTP_MODE_READ);
        if (!result)
        {
            return FSP_ERR_WRITE_FAILED;
        }
    }
    else if (storage_type == SECURE_ASSET_FACTORY_STORAGE_FLASH)
    {
#if RM_PSA_CRYPTO_WIFI_UTIL
        uint32_t offset = 0;
        char *data;
        switch (asset_id)
        {
            case SECURE_ASSET_APP_INFO:
                offset = SECURE_ASSET_APP_INFO * SECURE_ASSET_MAX_SIZE;
                break;

            case SECURE_ASSET_AT_KEY:
                offset = SECURE_ASSET_AT_KEY * SECURE_ASSET_MAX_SIZE;
                break;

            default:
                return FSP_ERR_INVALID_ARGUMENT;
        }

        data = malloc(SECURE_ASSET_MAX_SIZE * 2);
        memcpy(data,
               (void *) (BSP_FEATURE_OSPI_W_DEVICE_0_START_ADDRESS | SF_SECURE_ASSET_PROD),
               SECURE_ASSET_MAX_SIZE * 2);
        memcpy(data + offset, ctx->package_buffer, ctx->package_size);

        if (!util_sflash_write((SF_SECURE_ASSET_PROD), data, SECURE_ASSET_MAX_SIZE * 2))
        {
            free(data);
            return FSP_ERR_WRITE_FAILED;
        }
        else
        {
            free(data);
        }
#else
        return FSP_ERR_INVALID_MODE;  // Invalid storage type
#endif
    }
    else if (storage_type == SECURE_ASSET_USER_STORAGE_VEE)
    {
#if RM_PSA_CRYPTO_VEE_FLASH
        fsp_err_t err;
        /* Write package to flash */
        err = RM_VEE_FLASH_W_RecordWrite(
            &g_vee0_ctrl, SECURE_ASSET_BASE_ID + asset_id, ctx->package_buffer, ctx->package_size);
        if (err != FSP_SUCCESS)
        {
            return err;  // VEE write failed
        }
#else
        return FSP_ERR_INVALID_MODE;  // Invalid storage type
#endif
    }
    else
    {
        return FSP_ERR_INVALID_DATA;
    }

    return FSP_SUCCESS;
}

void print_CCRunAssetProvPkg(const CCRunAssetProvPkg_t * dest)
{
    if (dest == NULL)
    {
        CC_PRINTF("Error: Package pointer is NULL\n");
        return;
    }

    CC_PRINTF("Asset Package Details:\n");
    CC_PRINTF("--------------------\n");
    CC_PRINTF("Token: 0x%08X\n", dest->token);
    CC_PRINTF("Version: %u\n", dest->version);
    CC_PRINTF("Asset Size: %u bytes\n", dest->assetSize);

    CC_PRINTF("\nReserved Words:\n");
    for (int i = 0; i < CC_ASSET_PROV_RESERVED_WORD_SIZE; i++)
    {
        CC_PRINTF("Reserved[%d]: 0x%08X\n", i, dest->reserved[i]);
    }

    CC_PRINTF("\nNonce:\n");
    for (int i = 0; i < CC_ASSET_PROV_NONCE_SIZE; i++)
    {
        CC_PRINTF("%02X", dest->nonce[i]);
        if ((i + 1) % 16 == 0)
        {
            CC_PRINTF("\n");
        }
        else if ((i + 1) % 8 == 0)
        {
            CC_PRINTF("  ");
        }
        else
        {
            CC_PRINTF(" ");
        }
    }
    CC_PRINTF("\n");

    CC_PRINTF("\nEncryption Tag:\n");
    for (int i = 0; i < CC_ASSET_PROV_TAG_SIZE; i++)
    {
        CC_PRINTF("%02X", dest->enctag[i]);
        if ((i + 1) % 16 == 0)
        {
            CC_PRINTF("\n");
        }
        else if ((i + 1) % 8 == 0)
        {
            CC_PRINTF("  ");
        }
        else
        {
            CC_PRINTF(" ");
        }
    }
    CC_PRINTF("\n");

    /* Package validation check */
    if (dest->token != CC_ASSET_PROV_TOKEN)
    {
        CC_PRINTF(
            "\nWarning: Invalid package token (Expected: 0x%08X, Got: 0x%08X)\n",
            CC_ASSET_PROV_TOKEN, dest->token);
    }

    if (dest->assetSize == 0 || (dest->assetSize % CC_ASSET_PROV_BLOCK_SIZE) != 0)
    {
        CC_PRINTF("\nWarning: Asset size (%u) is not valid - must be non-zero and multiple of %d\n",
                    dest->assetSize,
                    CC_ASSET_PROV_BLOCK_SIZE);
    }
    else
    {
        CC_PRINTF("\nAsset size is valid.\n");
    }
    CC_PRINTF("--------------------\n");
    CC_PRINTF("End of Asset Package Details\n");
}

static fsp_err_t secure_asset_init_context(secure_asset_ctx_t * p_ctx, uint32_t size)
{
    /* Calculate sizes */
    p_ctx->asset_size = size;
    p_ctx->aligned_size = (((size + 15) >> 4) << 4);  // 16B aligned
    p_ctx->package_size = p_ctx->aligned_size + 48;   // Add header size

    /* Allocate buffers */
    p_ctx->asset_buffer = malloc(p_ctx->aligned_size);
    p_ctx->package_buffer = malloc(p_ctx->package_size);

    if ((p_ctx->asset_buffer == NULL) || (p_ctx->package_buffer == NULL))
    {
        free(p_ctx->asset_buffer);
        free(p_ctx->package_buffer);
        return FSP_ERR_OUT_OF_MEMORY;
    }

    return FSP_SUCCESS;
}

static void secure_asset_free_context(secure_asset_ctx_t * p_ctx)
{
    free(p_ctx->asset_buffer);
    free(p_ctx->package_buffer);

    memset(p_ctx, 0, sizeof(secure_asset_ctx_t));
}

static void HEX_PRINTF(uint8_t * data, uint32_t size)
{
    uint32_t i = 0;
    for (i = 0; i < size; i++)
    {
        char ascii = data[i] > 0x30 && data[i] < 0x7A ? data[i] : '.';
        CC_PRINTF("%02X(%c) ", data[i], ascii);
        if ((i + 1) % 16 == 0)
        {
            CC_PRINTF("\n");
        }
        else if ((i + 1) % 8 == 0)
        {
            CC_PRINTF(" ");
        }
        else
        {
            CC_PRINTF(" ");
        }
    }
    CC_PRINTF("\n");
}

static uint32_t secure_asset_storage_store(uint32_t  storage_type,
                                           uint32_t  assetID,
                                           uint8_t * Payload,
                                           uint32_t  PayloadSize)
{
    fsp_err_t err;
    secure_asset_ctx_t ctx = {0};

    if (Payload == NULL || PayloadSize == 0)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Initialize context with required buffer sizes */
    err = secure_asset_init_context(&ctx, PayloadSize);
    if (err != FSP_SUCCESS)
    {
        return err;
    }

    CC_PRINTF("## Payload ##\r\n");
    CC_PRINTF("## PayloadSize = %u\n", PayloadSize);
    HEX_PRINTF(Payload, PayloadSize);

    /* Copy payload to aligned buffer */
    memcpy(ctx.asset_buffer, Payload, PayloadSize);

#if KEY_TYPE != ADAPTIVE_KEY_FOR_ASSET
    /* Create runtime package */
    int32_t pkg_size = R_CC312_Secure_Asset_RuntimePack(ASSET_KEY_TYPE,
                                                        0,
                                                        ASSET_KEY_DATA,
                                                        ASSET_ID,
                                                        "RunPack",
                                                        ctx.asset_buffer,
                                                        ctx.aligned_size,
                                                        ctx.package_buffer);
#else
    CCError_t ccRc = CC_OK;
    uint32_t  lcs;
    int32_t   pkg_size;
    ccRc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);

    if (CC_OK == ccRc && CC_BSV_DEVICE_MANUFACTURE_LCS <= lcs)
    {
        pkg_size = R_CC312_Secure_Asset_RuntimePack(SECURE_ASSET_KEY_TYPE,
                                                    0,
                                                    SECURE_ASSET_KEY_DATA,
                                                    ASSET_ID,
                                                    "RunPack",
                                                    ctx.asset_buffer,
                                                    ctx.aligned_size,
                                                    ctx.package_buffer);
    }
    else
    {
        pkg_size = R_CC312_Secure_Asset_RuntimePack(USER_ASSET_KEY_TYPE,
                                                    0,
                                                    USER_ASSET_KEY_DATA,
                                                    ASSET_ID,
                                                    "RunPack",
                                                    ctx.asset_buffer,
                                                    ctx.aligned_size,
                                                    ctx.package_buffer);
    }
#endif

    CC_PRINTF("## assert pkg ##\r\n");
    CC_PRINTF("## pkg_size = %d\n", pkg_size);
    HEX_PRINTF(ctx.package_buffer, pkg_size);

    if (pkg_size > 0)
    {
        err = secure_asset_storage_write(storage_type, assetID, &ctx);
    }
    else
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    secure_asset_free_context(&ctx);
    return err;
}

static uint32_t secure_asset_storage_load(uint32_t  storage_type,
                                          uint32_t  assetID,
                                          uint8_t * Payload,
                                          uint32_t  PayloadSize)
{
    fsp_err_t err;
    secure_asset_ctx_t ctx = {0};

    if (Payload == NULL || PayloadSize == 0)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Initialize context with required buffer sizes */
    err = secure_asset_init_context(&ctx, PayloadSize);
    if (err != FSP_SUCCESS)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    err = secure_asset_storage_read(storage_type, assetID, &ctx);

    if (err != FSP_SUCCESS)
    {
        secure_asset_free_context(&ctx);
        return err;
    }

    CC_PRINTF("## assert pkg ##\r\n");
    CC_PRINTF("## pkg_size = %u\n", ctx.package_size);
    HEX_PRINTF(ctx.package_buffer, ctx.package_size);

#if KEY_TYPE != ADAPTIVE_KEY_FOR_ASSET
    /* Unpack runtime package */
    int32_t asset_size = R_CC312_Secure_Asset_RuntimeUnpack(ASSET_KEY_TYPE,
                                                            ASSET_KEY_DATA,
                                                            ASSET_ID,
                                                            ctx.package_buffer,
                                                            ctx.package_size,
                                                            ctx.asset_buffer);
#else
    CCError_t ccRc = CC_OK;
    uint32_t lcs;
    int32_t asset_size;
    ccRc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);

    if (CC_OK == ccRc && CC_BSV_DEVICE_MANUFACTURE_LCS <= lcs)
    {
        asset_size = R_CC312_Secure_Asset_RuntimeUnpack(SECURE_ASSET_KEY_TYPE,
                                                        SECURE_ASSET_KEY_DATA,
                                                        ASSET_ID,
                                                        ctx.package_buffer,
                                                        ctx.package_size,
                                                        ctx.asset_buffer);
    }
    else
    {
        asset_size = R_CC312_Secure_Asset_RuntimeUnpack(USER_ASSET_KEY_TYPE,
                                                        USER_ASSET_KEY_DATA,
                                                        ASSET_ID,
                                                        ctx.package_buffer,
                                                        ctx.package_size,
                                                        ctx.asset_buffer);
    }
#endif

    if (asset_size > 0 && asset_size <= PayloadSize)
    {
        /* Copy decrypted asset to output buffer */
        memcpy(Payload, ctx.asset_buffer, (uint32_t) (asset_size));
    }
    else
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    CC_PRINTF("## Payload ##\r\n");
    CC_PRINTF("## PayloadSize = %u\n", PayloadSize);
    HEX_PRINTF(ctx.asset_buffer, PayloadSize);

    secure_asset_free_context(&ctx);
    return err;
}

static uint32_t secure_asset_vee_delete(uint32_t assetID)
{
    uint8_t rec_delete_data[4] = {0x00, 0x00, 0x00, 0x00};

    /* Write package to flash */
    RM_VEE_FLASH_W_RecordWrite(&g_vee0_ctrl, SECURE_ASSET_BASE_ID + assetID, rec_delete_data, 4);

    return FSP_SUCCESS;
}
