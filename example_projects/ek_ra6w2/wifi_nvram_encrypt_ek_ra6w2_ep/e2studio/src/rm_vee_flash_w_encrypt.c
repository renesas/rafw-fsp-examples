/***********************************************************************************************************************
#include <rm_vee_flash_w_encrypt.h>
* File Name    : nvram_encypt.c
* Description  : nvram encryption and decryption functions and configurations
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "rm_vee_flash_w_encrypt.h"
#include "r_cc312_openable_w_cfg.h"
#include "r_cc312_common.h"
#include "secureboot_stage_defs.h"
#include "util_api.h"
#include "common_utils.h"

#if KEY_TYPE == USER_KEY_FOR_ASSET
#define ASSET_KEY_TYPE ASSET_USER_KEY
#define ASSET_KEY_DATA &userKeyData
#elif KEY_TYPE == KCP_KEY_FOR_ASSET
#define ASSET_KEY_TYPE SSET_KCP_KEY
#define ASSET_KEY_DATA NULL
#elif KEY_TYPE == ADAPTIVE_KEY_FOR_ASSET
#define USER_ASSET_KEY_TYPE ASSET_USER_KEY
#define USER_ASSET_KEY_DATA &userKeyData
#define SECURE_ASSET_KEY_TYPE ASSET_KCP_KEY
#define SECURE_ASSET_KEY_DATA NULL
#endif //KEY_TYPE == USER_KEY_FOR_ASSET

const uint8_t test_key[] = {0xd5, 0xe9, 0xda, 0x41, 0xa6, 0x5b, 0x7f, 0xd2, 0xe5, 0xad, 0xf4, 0xb8, 0xf8, 0x43, 0x25, 0x3f};
extern rm_vee_flash_w_instance_ctrl_t g_vee0_ctrl;

AssetUserKeyData_t userKeyData =
{
    .pKey = (uint8_t *) test_key,
    .keySize = 16,
};

typedef struct
{
    char ssid[32];
    char password[64];
} ap_info_t;

typedef struct
{
    char key[64];
} key_info_t;

#if RM_PSA_CRYPTO_VEE_FLASH
#if SECURE_ASSET_OTP_ENABLE
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
    uint32_t word;
    uint32_t i;

    /* Initialize OTP controller in read mode */
    bsp_otp_init();
    bsp_otp_mode_set(BSP_OTP_MODE_READ);

    /* Read words and check if all zero */
    for (i = 0; i < count; i++)
    {
        word = bsp_otp_word_read(start_offset + i);

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
#endif //SECURE_ASSET_OTP_ENABLE

/*******************************************************************************************************************/ /**
 * Read default secure asset from OTP or flash memory
 *
 * @param[in] storage_type  Storage type (OTP or Flash)
 * @param[in] asset_id      Asset ID to read
 * @param[in] ctx           Context containing buffer and size
 **********************************************************************************************************************/
static fsp_err_t secure_asset_storage_read(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx)
{
    fsp_err_t err;
    uint32_t read_addr;

#if SECURE_ASSET_OTP_ENABLE
    uint32_t otp_offset;
    uint32_t word_length;
#endif //SECURE_ASSET_OTP_ENABLE

    /* Validate inputs */
    if (ctx == NULL || ctx->package_buffer == NULL)
    {
        return FSP_ERR_INVALID_POINTER; // NULL pointer check
    }

#if SECURE_ASSET_OTP_ENABLE
    if (storage_type == STORAGE_TYPE_OTP)
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
    else
#endif //SECURE_ASSET_OTP_ENABLE
    if (storage_type == STORAGE_TYPE_FLASH)
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
        memcpy(ctx->package_buffer, (void *) (BSP_FEATURE_OSPI_DEVICE_0_START_ADDRESS | read_addr), ctx->package_size);
    }
    else if (storage_type == STORAGE_TYPE_VEE)
    {
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
    else
    {
        return FSP_ERR_INVALID_MODE;  // Invalid storage type
    }

    return FSP_SUCCESS;
}

static fsp_err_t secure_asset_storage_write(uint8_t storage_type, uint32_t asset_id, secure_asset_ctx_t *ctx)
{
    fsp_err_t err;

#if SECURE_ASSET_OTP_ENABLE
    uint32_t otp_offset;
    uint32_t word_length;
#endif //SECURE_ASSET_OTP_ENABLE

    /* Validate inputs */
    if (ctx == NULL || ctx->package_buffer == NULL)
    {
        return FSP_ERR_INVALID_POINTER;
    }

#if SECURE_ASSET_OTP_ENABLE
    if (storage_type == STORAGE_TYPE_OTP)
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
    else
#endif //SECURE_ASSET_OTP_ENABLE
    if (storage_type == STORAGE_TYPE_FLASH)

#if RM_PSA_CRYPTO_WIFI_UTIL
    {
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
               (void *)(BSP_FEATURE_OSPI_DEVICE_0_START_ADDRESS | SF_SECURE_ASSET_PROD),
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
    }
#endif //RM_PSA_CRYPTO_WIFI_UTIL
    else if (storage_type == STORAGE_TYPE_VEE)
    {
        /* Write package to flash */
        err = RM_VEE_FLASH_W_RecordWrite(&g_vee0_ctrl, SECURE_ASSET_BASE_ID + asset_id, ctx->package_buffer, ctx->package_size);

        if (err != FSP_SUCCESS)
        {
            return err;  // VEE write failed
        }
    }
    else
    {
        return FSP_ERR_INVALID_DATA;
    }

    return FSP_SUCCESS;
}

static fsp_err_t secure_asset_init_context(secure_asset_ctx_t *p_ctx, uint32_t size)
{
    /* Calculate sizes */
    p_ctx->asset_size = size;

    /*
     * Align 'size' to the next multiple of 16 bytes:
     * Add 15 to 'size' to ensure rounding up if it's not already a multiple of 16.
     * Right shift by 4 bits (divide by 16) to get the number of 16-byte blocks.
     * Left shift by 4 bits (multiply by 16) to get the aligned size in bytes.
     */
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

static void secure_asset_free_context(secure_asset_ctx_t *p_ctx)
{
    free(p_ctx->asset_buffer);
    free(p_ctx->package_buffer);
    memset(p_ctx, 0, sizeof(secure_asset_ctx_t));
}

static void HEX_PRINTF(uint8_t *data, uint32_t size)
{
    uint32_t i = 0;

    for (i = 0; i < size; i++)
    {
        char ascii = data[i] > 0x30 && data[i] < 0x7A ? data[i] : '.';

        APP_PRINT("%02X(%c) ", data[i], ascii);
        CC_PRINTF("%02X(%c) ", data[i], ascii);
        if ((i + 1) % 16 == 0)
        {
            APP_PRINT("\n");
            CC_PRINTF("\n");
        }
        else if ((i + 1) % 8 == 0)
        {
            APP_PRINT(" ");
            CC_PRINTF(" ");
        }
        else
        {
            APP_PRINT(" ");
            CC_PRINTF(" ");
        }
    }

    APP_PRINT("\n");
    CC_PRINTF("\n");
}

static uint32_t R_CC312_SecureAssetStorageStore(uint32_t storage_type,
                                                uint32_t assetID,
                                                uint8_t *Payload,
                                                uint32_t PayloadSize)
{
    fsp_err_t err;
    secure_asset_ctx_t ctx = {0};
    uint32_t lcs;
    int32_t pkg_size;

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

    APP_PRINT_INFO("## Payload ##\r\n");
    APP_PRINT_INFO("## PayloadSize = %ld\n", PayloadSize);
    CC_PRINTF("## Payload ##\r\n");
    CC_PRINTF("## PayloadSize = %ld\n", PayloadSize);
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
#endif //KEY_TYPE != ADAPTIVE_KEY_FOR_ASSET

    APP_PRINT_INFO("## assert pkg ##\r\n");
    APP_PRINT_INFO("## pkg_size = %ld\n", pkg_size);
    CC_PRINTF("## assert pkg ##\r\n");
    CC_PRINTF("## pkg_size = %ld\n", pkg_size);
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

static uint32_t R_CC312_SecureAssetStorageLoad(uint32_t storage_type,
                                               uint32_t assetID,
                                               uint8_t *Payload,
                                               uint32_t PayloadSize)
{
    fsp_err_t err;
    secure_asset_ctx_t ctx = {0};
    uint32_t lcs;
    int32_t asset_size;

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

    APP_PRINT_INFO("## assert pkg ##\r\n");
    APP_PRINT_INFO("## pkg_size = %ld\n", ctx.package_size);
    CC_PRINTF("## assert pkg ##\r\n");
    CC_PRINTF("## pkg_size = %ld\n", ctx.package_size);
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
#endif //KEY_TYPE != ADAPTIVE_KEY_FOR_ASSET

    if (asset_size > 0 && asset_size >= (int32_t)PayloadSize)
    {
        /* Copy decrypted asset to output buffer */
        memcpy(Payload, ctx.asset_buffer, PayloadSize);
    }
    else
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    APP_PRINT_INFO("## Payload ##\r\n");
    APP_PRINT_INFO("## PayloadSize = %ld\n", PayloadSize);
    CC_PRINTF("## Payload ##\r\n");
    CC_PRINTF("## PayloadSize = %ld\n", PayloadSize);
    HEX_PRINTF(ctx.asset_buffer, PayloadSize);
    secure_asset_free_context(&ctx);

    return err;
}

static uint32_t R_CC312_SecureAssetVeeDelete(uint32_t assetID)
{
    uint8_t rec_delete_data[4] = {0x00, 0x00, 0x00, 0x00};

    /* Write package to flash */
    RM_VEE_FLASH_W_RecordWrite(&g_vee0_ctrl, SECURE_ASSET_BASE_ID + assetID, rec_delete_data, 4);

    return FSP_SUCCESS;
}

static fsp_err_t R_CC312_SecureAssetProdLoad(uint32_t storage_type, uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = R_CC312_SecureAssetStorageLoad(storage_type, asset_id, p_data, length);

    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_NOT_FOUND;
}

static fsp_err_t R_CC312_SecureAssetStore(uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = R_CC312_SecureAssetStorageStore(STORAGE_TYPE_VEE, asset_id, p_data, length);

    if (FSP_SUCCESS != err)
    {
        return FSP_ERR_WRITE_FAILED;
    }

    return FSP_SUCCESS;
}

static fsp_err_t R_CC312_SecureAssetLoad(uint32_t asset_id, uint8_t *p_data, uint32_t length)
{
    fsp_err_t err;

    /* Parameter checking */
    if ((NULL == p_data) || (0 == length))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Try reading Flash first*/
    err = R_CC312_SecureAssetStorageLoad(STORAGE_TYPE_VEE, asset_id, p_data, length);

    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    /* Try reading from FACTORY FLASH second */
    err = R_CC312_SecureAssetStorageLoad(STORAGE_TYPE_FLASH, asset_id, p_data, length);

    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    /* Try reading from FACTORY OTP second */
    err = R_CC312_SecureAssetStorageLoad(STORAGE_TYPE_OTP, asset_id, p_data, length);

    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_NOT_FOUND;
}

static void secure_asset_store(secure_asset_mode_t mode, secure_storage_type_t storage_type)
{
    ap_info_t ap_info;
    key_info_t key_info;
    fsp_err_t err;

    if (mode == SECURE_ASSET_MODE_PROD)
    {
        strcpy(ap_info.ssid, "DemoSSID");
        strcpy(ap_info.password, "DemoPassword");
        strcpy(key_info.key, "DemoATKey");

        err = R_CC312_SecureAssetStorageStore(storage_type,
                                              SECURE_ASSET_APP_INFO,
                                              (uint8_t *)&ap_info,
                                              sizeof(ap_info_t));

        /* R_CC312_SecureAssetStorageStore(storage_type, asset_id, p_data, length); */
        if (err != FSP_SUCCESS)
        {
            APP_PRINT_INFO("PROD: Failed to store AP Info to storage type %u\n", storage_type);
            CC_PRINTF("PROD: Failed to store AP Info to storage type %u\n", storage_type);
        }

        err = R_CC312_SecureAssetStorageStore(storage_type,
                                              SECURE_ASSET_AT_KEY,
                                              (uint8_t *)&key_info,
                                              sizeof(key_info_t));

        if (err != FSP_SUCCESS)
        {
            APP_PRINT_INFO("PROD: Failed to store AT Key to storage type %u\n", storage_type);
            CC_PRINTF("PROD: Failed to store AT Key to storage type %u\n", storage_type);
        }


        APP_PRINT_INFO("Secure asset PROD store complete to storage type %u.\n", storage_type);
        CC_PRINTF("Secure asset PROD store complete to storage type %u.\n", storage_type);
    }
    else
    {
        strcpy(ap_info.ssid, "UserSSID");
        strcpy(ap_info.password, "UserPassword");
        strcpy(key_info.key, "UserATKey");

        err = R_CC312_SecureAssetStore(SECURE_ASSET_APP_INFO, (uint8_t *)&ap_info, sizeof(ap_info_t));

        if (err != FSP_SUCCESS)
        {
            APP_PRINT_INFO("USER: Failed to store AP Info\n");
            CC_PRINTF("USER: Failed to store AP Info\n");
        }

        err = R_CC312_SecureAssetStore(SECURE_ASSET_AT_KEY, (uint8_t *)&key_info, sizeof(key_info_t));

        if (err != FSP_SUCCESS)
        {
            APP_PRINT_INFO("USER: Failed to store AT Key\n");
            CC_PRINTF("USER: Failed to store AT Key\n");
        }

        APP_PRINT_INFO("Secure asset USER store operation complete.\n");
        CC_PRINTF("Secure asset USER store operation complete.\n");
    }
}

static void secure_asset_load(secure_asset_mode_t mode, secure_storage_type_t storage_type)
{
    ap_info_t ap_info = {0};
    key_info_t key_info = {0};
    fsp_err_t err;

    if (mode == SECURE_ASSET_MODE_PROD)
    {
        err = R_CC312_SecureAssetProdLoad(storage_type,
                                          SECURE_ASSET_APP_INFO,
                                          (uint8_t *)&ap_info,
                                          sizeof(ap_info_t));

        if (err == FSP_SUCCESS)
        {
            APP_PRINT_INFO("PROD SSID (type %u): %s\n", storage_type, ap_info.ssid);
            APP_PRINT_INFO("PROD Password: %s\n", ap_info.password);
            CC_PRINTF("PROD SSID (type %u): %s\n", storage_type, ap_info.ssid);
            CC_PRINTF("PROD Password: %s\n", ap_info.password);
        }
        else
        {
            APP_PRINT_INFO("PROD: Failed to load AP Info from storage type %u\n", storage_type);
            CC_PRINTF("PROD: Failed to load AP Info from storage type %u\n", storage_type);
        }

        err = R_CC312_SecureAssetProdLoad(storage_type,
                                          SECURE_ASSET_AT_KEY,
                                          (uint8_t *)&key_info,
                                          sizeof(key_info_t));

        if (err == FSP_SUCCESS)
        {
            APP_PRINT_INFO("PROD AT Key: %s\n", key_info.key);
            CC_PRINTF("PROD AT Key: %s\n", key_info.key);
        }
        else
        {
            APP_PRINT_INFO("PROD: Failed to load AT Key from storage type %u\n", storage_type);
            CC_PRINTF("PROD: Failed to load AT Key from storage type %u\n", storage_type);
        }
    }
    else
    {
        err = R_CC312_SecureAssetLoad(SECURE_ASSET_APP_INFO, (uint8_t *)&ap_info, sizeof(ap_info_t));

        if (err == FSP_SUCCESS)
        {
            APP_PRINT_INFO("USER SSID: %s\n", ap_info.ssid);
            APP_PRINT_INFO("USER Password: %s\n", ap_info.password);
            CC_PRINTF("USER SSID: %s\n", ap_info.ssid);
            CC_PRINTF("USER Password: %s\n", ap_info.password);
        }
        else
        {
            APP_PRINT_INFO("USER: Failed to load AP Info\n");
            CC_PRINTF("USER: Failed to load AP Info\n");
        }

        err = R_CC312_SecureAssetLoad(SECURE_ASSET_AT_KEY, (uint8_t *)&key_info, sizeof(key_info_t));

        if (err == FSP_SUCCESS)
        {
            APP_PRINT_INFO("USER AT Key: %s\n", key_info.key);
            CC_PRINTF("USER AT Key: %s\n", key_info.key);
        }
        else
        {
            APP_PRINT_INFO("USER: Failed to load AT Key\n");
            CC_PRINTF("USER: Failed to load AT Key\n");
        }
    }
}

static void secure_asset_delete_example(void)
{
    R_CC312_SecureAssetVeeDelete(SECURE_ASSET_APP_INFO);
    R_CC312_SecureAssetVeeDelete(SECURE_ASSET_AT_KEY);
    APP_PRINT_INFO("Secure asset VEE delete operation successful.\n");
    CC_PRINTF("Secure asset VEE delete operation successful.\n");
}
#endif //RM_PSA_CRYPTO_VEE_FLASH

/* ---- Integration Entry ---- */
void secure_asset_test_run(void)
{
#if RM_PSA_CRYPTO_VEE_FLASH
    APP_PRINT_INFO("\n--- Running Secure Asset USER Example ---\n");
    CC_PRINTF("\n--- Running Secure Asset USER Example ---\n");
    secure_asset_store(SECURE_ASSET_MODE_USER, 0);  // storage_type ignored for USER
    secure_asset_load(SECURE_ASSET_MODE_USER, 0);

#if SECURE_ASSET_OTP_ENABLE
    APP_PRINT_INFO("\n--- Running Secure Asset PROD (OTP) Example ---\n");
    CC_PRINTF("\n--- Running Secure Asset PROD (OTP) Example ---\n");
    secure_asset_store(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_OTP);
    secure_asset_load(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_OTP);
#endif //SECURE_ASSET_OTP_ENABLE

    vTaskDelay(pdMS_TO_TICKS(5000));
    APP_PRINT_INFO("\n--- Running Secure Asset PROD (FLASH) Example ---\n");
    CC_PRINTF("\n--- Running Secure Asset PROD (FLASH) Example ---\n");
    secure_asset_store(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_FLASH);
    secure_asset_load(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_FLASH);
    vTaskDelay(pdMS_TO_TICKS(5000));
    APP_PRINT_INFO("\n--- Running Secure Asset PROD (VEE) Example ---\n");
    CC_PRINTF("\n--- Running Secure Asset PROD (VEE) Example ---\n");
    secure_asset_store(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_VEE);
    secure_asset_load(SECURE_ASSET_MODE_PROD, STORAGE_TYPE_VEE);
    vTaskDelay(pdMS_TO_TICKS(5000));
    APP_PRINT_INFO("\n--- Running Secure Asset Delete Example ---\n");
    CC_PRINTF("\n--- Running Secure Asset Delete Example ---\n");
    secure_asset_delete_example();
#else
    APP_PRINT_INFO("\n--- secure_asset_test_run RM_PSA_CRYPTO_VEE_FLASH disabled ---\n");
    CC_PRINTF("\n--- secure_asset_test_run RM_PSA_CRYPTO_VEE_FLASH disabled ---\n");
#endif //RM_PSA_CRYPTO_VEE_FLASH
}
