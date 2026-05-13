/***********************************************************************************************************************
 * File Name    : flash_encrypt.c
 * Description  : CC312-encrypted string store/load targeting
 *                the SFlash User Area (SF_USER_AREA).
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include <flash_encrypt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "r_cc312_openable_w_cfg.h"
#include "r_cc312_common.h"
#include "secureboot_stage_defs.h"
#include "util_api.h"
#include "common_utils.h"

/* ==========================================================================
 * Key configuration
 * ========================================================================== */
#if FLASH_KEY_TYPE == FLASH_USER_KEY
 #define FLASH_KEY_TYPE_A    ASSET_USER_KEY
 #define FLASH_KEY_DATA_A    &flash_user_key_data
#elif FLASH_KEY_TYPE == FLASH_KCP_KEY
 #define FLASH_KEY_TYPE_A    ASSET_KCP_KEY
 #define FLASH_KEY_DATA_A    NULL
#elif FLASH_KEY_TYPE == FLASH_ADAPTIVE_KEY
 /* Adaptive: use KCP key on secure LCS, user key on non-secure LCS */
 #define FLASH_SECURE_KEY_TYPE   ASSET_KCP_KEY
 #define FLASH_SECURE_KEY_DATA   NULL
 #define FLASH_USER_KEY_TYPE_A   ASSET_USER_KEY
 #define FLASH_USER_KEY_DATA_A   &flash_user_key_data
#endif

/* 128-bit test key — development / pre-production use only */
static const uint8_t flash_test_key[] =
{
    0xd5, 0xe9, 0xda, 0x41, 0xa6, 0x5b, 0x7f, 0xd2,
    0xe5, 0xad, 0xf4, 0xb8, 0xf8, 0x43, 0x25, 0x3f
};

static AssetUserKeyData_t flash_user_key_data =
{
    .pKey    = (uint8_t *) flash_test_key,
    .keySize = 16,
};

/* ==========================================================================
 * Private context helpers
 * ========================================================================== */
typedef struct
{
    uint32_t  asset_size;
    uint32_t  aligned_size;
    uint32_t  package_size;
    uint8_t  *asset_buffer;
    uint8_t  *package_buffer;
} flash_ctx_t;

static fsp_err_t flash_init_context(flash_ctx_t *p_ctx, uint32_t size)
{
    p_ctx->asset_size   = size;
    p_ctx->aligned_size = (((size + 15U) >> 4U) << 4U);
    p_ctx->package_size = p_ctx->aligned_size + FLASH_PACKAGE_HDR_SIZE;

    p_ctx->asset_buffer   = malloc(p_ctx->aligned_size);
    p_ctx->package_buffer = malloc(p_ctx->package_size);

    if ((p_ctx->asset_buffer == NULL) || (p_ctx->package_buffer == NULL))
    {
        free(p_ctx->asset_buffer);
        free(p_ctx->package_buffer);

        return FSP_ERR_OUT_OF_MEMORY;
    }

    memset(p_ctx->asset_buffer, 0, p_ctx->aligned_size);
    memset(p_ctx->package_buffer, 0, p_ctx->package_size);

    return FSP_SUCCESS;
}

static void flash_free_context(flash_ctx_t *p_ctx)
{
    free(p_ctx->asset_buffer);
    free(p_ctx->package_buffer);
    memset(p_ctx, 0, sizeof(flash_ctx_t));
}

/* ==========================================================================
 * Debug helper
 * ========================================================================== */
static void HEX_PRINTF(uint8_t *data, uint32_t size)
{
    const uint32_t line_width = 32U;
    const uint32_t half_width = 16U;
    uint32_t index = 0;

    while (index < size)
    {
        uint32_t line_len = size - index;
        if (line_len > line_width)
        {
            line_len = line_width;
        }

        char ascii[line_width + 1];
        for (uint32_t i = 0; i < line_len; i++)
        {
            uint8_t b = data[index + i];
            ascii[i] = (b >= 0x20 && b <= 0x7E) ? (char)b : '.';
        }

        ascii[line_len] = '\0';

        for (uint32_t i = 0; i < line_len; i++)
        {
            APP_PRINT("%02X", data[index + i]);

            if (i + 1 < line_len)
            {
                APP_PRINT(" ");
            }

            if (((i + 1) == half_width) && (line_len > half_width))
            {
                APP_PRINT(" ");
            }
        }

        if (line_len < line_width)
        {
            uint32_t missing = line_width - line_len;
            for (uint32_t i = 0; i < missing; i++)
            {
                APP_PRINT("   ");
            }

            if (line_len <= half_width)
            {
                APP_PRINT(" ");
            }
        }

        APP_PRINT("|%s\r\n", ascii);
        index += line_len;
    }
}

/* ==========================================================================
 * Public API
 * ========================================================================== */

/**
 * Encrypt a null-terminated string with CC312 and write it to the
 * SFlash User Area slot at FLASH_ADDR.
 *
 *   1. Copy string into a zero-padded aligned buffer.
 *   2. Encrypt with CC312 R_CC312_Secure_Asset_RuntimePack.
 *   3. Write back with util_sflash_write() to FLASH_ADDR.
 */
fsp_err_t flash_write(const char *p_str)
{
    fsp_err_t  err;
    flash_ctx_t  ctx = {0};
    int32_t  pkg_size;
    uint32_t  lcs;
    uint8_t  *slot_buf;
    uint32_t  str_len;

    /* Parameter validation */
    if (p_str == NULL)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    str_len = (uint32_t)strlen(p_str);
    if ((str_len == 0U) || ((str_len + 1U) > FLASH_ASSET_MAX_SIZE))
    {
        APP_PRINT_ERR("flash_write: string length %lu out of range (1..%u)\r\n",
                      (unsigned long)str_len, FLASH_ASSET_MAX_SIZE - 1U);

        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Allocate working buffers — always FLASH_ASSET_MAX_SIZE for a fixed package size */
    err = flash_init_context(&ctx, FLASH_ASSET_MAX_SIZE);
    if (err != FSP_SUCCESS)
    {
        return err;
    }

    /* Copy payload to aligned buffer */
    memcpy(ctx.asset_buffer, p_str, str_len + 1U);

    /* ---- Encrypt with CC312 ---- */
#if FLASH_KEY_TYPE != FLASH_ADAPTIVE_KEY
 #if FLASH_KEY_TYPE == FLASH_KCP_KEY
    /* FLASH_KCP_KEY cannot be used in non-secure LCS (CM/DM) */
    {
        CCError_t cc_rc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);
        if ((CC_OK != cc_rc) || (lcs < CC_BSV_SECURE_LCS))
        {
            APP_PRINT_ERR("flash_write: FLASH_KCP_KEY cannot be used in non-secure LCS\r\n");
            flash_free_context(&ctx);
            return FSP_ERR_INVALID_ARGUMENT;
        }
    }
 #endif
    pkg_size = R_CC312_Secure_Asset_RuntimePack(FLASH_KEY_TYPE_A,
                                                0,
                                                FLASH_KEY_DATA_A,
                                                FLASH_ASSET_ID,
                                                "RunPack",
                                                ctx.asset_buffer,
                                                ctx.aligned_size,
                                                ctx.package_buffer);
#else
    {
        CCError_t cc_rc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);

        if ((CC_OK == cc_rc) && (CC_BSV_SECURE_LCS <= lcs))
        {
            /* Secure LCS device — use hardware KCP key */
            pkg_size = R_CC312_Secure_Asset_RuntimePack(FLASH_SECURE_KEY_TYPE,
                                                        0,
                                                        FLASH_SECURE_KEY_DATA,
                                                        FLASH_ASSET_ID,
                                                        "RunPack",
                                                        ctx.asset_buffer,
                                                        ctx.aligned_size,
                                                        ctx.package_buffer);
        }
        else
        {
            /* Development device — use software user key */
            pkg_size = R_CC312_Secure_Asset_RuntimePack(FLASH_USER_KEY_TYPE_A,
                                                        0,
                                                        FLASH_USER_KEY_DATA_A,
                                                        FLASH_ASSET_ID,
                                                        "RunPack",
                                                        ctx.asset_buffer,
                                                        ctx.aligned_size,
                                                        ctx.package_buffer);
        }
    }
#endif

    if (pkg_size > 0)
    {
        if ((uint32_t)pkg_size > ctx.package_size)
        {
            err = FSP_ERR_INVALID_ARGUMENT;
        }
        else
        {
            /* ---- Write the encrypted package to flash ---- */
            slot_buf = malloc(FLASH_SLOT_SIZE);
            if (slot_buf == NULL)
            {
                flash_free_context(&ctx);

                return FSP_ERR_OUT_OF_MEMORY;
            }

            memset(slot_buf, 0, FLASH_SLOT_SIZE);
            memcpy(slot_buf, ctx.package_buffer, (uint32_t)pkg_size);

            if (util_sflash_write((int)FLASH_ADDR,
                                  (char *)slot_buf,
                                  (int)FLASH_SLOT_SIZE) == false)
            {
                err = FSP_ERR_WRITE_FAILED;
            }
            free(slot_buf);
        }
    }
    else
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    flash_free_context(&ctx);

    return err;
}

/**
 * Read and CC312-decrypt a string from the SFlash User Area slot at FLASH_ADDR.
 */
fsp_err_t flash_read(char *p_buf, uint32_t buf_size)
{
    fsp_err_t err;
    flash_ctx_t ctx = {0};
    int32_t asset_sz;
    uint32_t lcs;

    /* Parameter validation */
    if ((p_buf == NULL) || (buf_size == 0U) || (buf_size > FLASH_ASSET_MAX_SIZE))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Allocate working buffers — always FLASH_ASSET_MAX_SIZE for a fixed package size */
    err = flash_init_context(&ctx, FLASH_ASSET_MAX_SIZE);
    if (err != FSP_SUCCESS)
    {
        return err;
    }

    /* ---- Read encrypted package via XIP ---- */
    memcpy(ctx.package_buffer,
           (void *)(BSP_FEATURE_OSPI_W_DEVICE_0_START_ADDRESS | FLASH_ADDR),
           ctx.package_size);

    /* ---- Decrypt with CC312 ---- */
#if FLASH_KEY_TYPE != FLASH_ADAPTIVE_KEY
 #if FLASH_KEY_TYPE == FLASH_KCP_KEY
    /* FLASH_KCP_KEY cannot be used in non-secure LCS (CM/DM) */
    {
        CCError_t cc_rc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);
        if ((CC_OK != cc_rc) || (lcs < CC_BSV_SECURE_LCS))
        {
            APP_PRINT_ERR("flash_read: FLASH_KCP_KEY cannot be used in non-secure LCS\r\n");
            flash_free_context(&ctx);
            return FSP_ERR_INVALID_ARGUMENT;
        }
    }
 #endif
    asset_sz = R_CC312_Secure_Asset_RuntimeUnpack(FLASH_KEY_TYPE_A,
                                                  FLASH_KEY_DATA_A,
                                                  FLASH_ASSET_ID,
                                                  ctx.package_buffer,
                                                  ctx.package_size,
                                                  ctx.asset_buffer);
#else
    {
        CCError_t cc_rc = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);

        if ((CC_OK == cc_rc) && (CC_BSV_SECURE_LCS <= lcs))
        {
            /* Secure LCS device — use hardware KCP key */
            asset_sz = R_CC312_Secure_Asset_RuntimeUnpack(FLASH_SECURE_KEY_TYPE,
                                                          FLASH_SECURE_KEY_DATA,
                                                          FLASH_ASSET_ID,
                                                          ctx.package_buffer,
                                                          ctx.package_size,
                                                          ctx.asset_buffer);
        }
        else
        {
            /* Development device — use software user key */
            asset_sz = R_CC312_Secure_Asset_RuntimeUnpack(FLASH_USER_KEY_TYPE_A,
                                                          FLASH_USER_KEY_DATA_A,
                                                          FLASH_ASSET_ID,
                                                          ctx.package_buffer,
                                                          ctx.package_size,
                                                          ctx.asset_buffer);
        }
    }
#endif

    if (asset_sz > 0)
    {
        uint32_t copy_len = (uint32_t)asset_sz;
        if (copy_len >= buf_size)
        {
            copy_len = buf_size - 1U;
        }

        memcpy(p_buf, ctx.asset_buffer, copy_len);
        p_buf[copy_len] = '\0';
        err = FSP_SUCCESS;
    }
    else
    {
        APP_PRINT_ERR("flash_read: CC312 unpack failed (ret=%ld)\r\n", (long)asset_sz);
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    flash_free_context(&ctx);

    return err;
}

/* ==========================================================================
 * Self-contained test
 * ========================================================================== */
void flash_test_run(void)
{
    char read_buf[FLASH_ASSET_MAX_SIZE];
    char key_str[33];
    uint8_t raw_bytes[FLASH_ASSET_MAX_SIZE];
    fsp_err_t err;

    const char *input_str =
           "0123456789ABCDEFGHIJKLMNO"  /* 25 */
           "0123456789ABCDEFGHIJKLMNO"  /* 50 */
           "0123456789ABCDEFGHIJKLMNO"  /* 75 */
           "0123456789ABCDEFGHIJKLMNO"  /* 100 */
           "0123456789ABCDEFGHIJKLMNO"  /* 125 */
           "0123456789ABCDEFGHIJKLMNO"; /* 150 */

    APP_PRINT_INFO("\n========================================\r\n");
    APP_PRINT_INFO(" Flash Encrypted Test - CC312 (FLASH_ADDR = 0x%08lX)\r\n",
                   (unsigned long)(FLASH_ADDR));
    APP_PRINT_INFO("========================================\r\n");

    for (int i = 0; i < 16; i++)
    {
        sprintf(&key_str[i * 2], "%02X", flash_test_key[i]);
    }

    key_str[32] = '\0';

    APP_PRINT_INFO("keyProv: %s\r\n", key_str);

    /* ------------------------------------------------------------------
     * WRITE
     * ------------------------------------------------------------------ */
    APP_PRINT_INFO("\n[WRITE] Encrypting and writing string to SFlash User Area...\r\n");

    err = flash_write(input_str);
    if (FSP_SUCCESS == err)
    {
        APP_PRINT_INFO("[WRITE] SUCCESS : \"%s\"\r\n", input_str);
    }
    else
    {
        APP_PRINT_ERR("[WRITE] FAILED (err=0x%X)\r\n", (unsigned int)err);
    }

    /* ------------------------------------------------------------------
     * READ BACK (plain XIP read — shows raw encrypted bytes in flash)
     * ------------------------------------------------------------------ */
    APP_PRINT_INFO("\n[READ]  Reading raw bytes from SFlash User Area (encrypted)...\r\n");

    memset(raw_bytes, 0, sizeof(raw_bytes));
    if (util_sflash_read((int)FLASH_ADDR, (char *)raw_bytes, sizeof(raw_bytes)) == false)
    {
        APP_PRINT_ERR("[READ]  util_sflash_read failed\r\n");
    }
    else
    {
        APP_PRINT_INFO("[READ]  Raw bytes (first 64 bytes):\r\n");
        HEX_PRINTF(raw_bytes, 64U);
    }

    /* ------------------------------------------------------------------
     * DECRYPT
     * ------------------------------------------------------------------ */
    APP_PRINT_INFO("\n[DECRYPT] Decrypting string from SFlash User Area...\r\n");

    memset(read_buf, 0, sizeof(read_buf));
    err = flash_read(read_buf, sizeof(read_buf));
    if (FSP_SUCCESS == err)
    {
        APP_PRINT_INFO("[DECRYPT]: \"%s\"\r\n", read_buf);

        if (strcmp(input_str, read_buf) == 0)
        {
            APP_PRINT_INFO("[VERIFY] Decrypted string matches original.\r\n");
        }
        else
        {
            APP_PRINT_ERR("[VERIFY] Decrypted string does not match original.\r\n");
        }
    }
    else
    {
        APP_PRINT_ERR("[DECRYPT] FAILED (err=0x%X)\r\n", (unsigned int)err);
    }

    APP_PRINT_INFO("\n[DONE] Flash Encrypted Test complete.\r\n");
}
