/***********************************************************************************************************************
 * File Name    : mbedded_crypto_example.c
 * Description  : Example of mbedded cypto.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "common_utils.h"
#include "mbedded_crypto_example.h"

void print_ep_info()
{
    fsp_pack_version_t version;
    R_FSP_VersionGet(&version);
    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3, EP_VERSION);
    APP_PRINT(BANNER_4, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
}

void *crypto_sample_calloc(size_t n, size_t size)
{
    void *buf = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf)
    {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

void crypto_sample_free(void *f)
{
    if (f == NULL)
    {
        return;
    }

    vPortFree(f);
}

void mbedded_crypto_example(void)
{
    int ret;

    APP_PRINT("\n>>> START Mbedded Crypto Example\n");

#if defined(MBEDTLS_CIPHER_MODE_CCM)
    APP_PRINT("\n>>> Start AES CCM\n");
    ret = crypto_sample_aes_ccm();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES CCM\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES CCM(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_CCM)

#if defined(MBEDTLS_CIPHER_MODE_GCM)
    APP_PRINT("\n>>> Start AES GCM\n");
    ret = crypto_sample_aes_gcm();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES GCM\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES GCM(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_GCM)

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    APP_PRINT("\n>>> Start AES CBC\n");
    ret = crypto_sample_aes_cbc();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES CBC\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES CBC(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_CBC)

#if defined(MBEDTLS_CIPHER_MODE_CFB)
    APP_PRINT("\n>>> Start AES CFB\n");
    ret = crypto_sample_aes_cfb();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES CFB\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES CFB(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_CFB)

#if defined(MBEDTLS_CIPHER_MODE_CTR)
    APP_PRINT("\n>>> Start AES CTR\n");
    ret = crypto_sample_aes_ctr();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES CTR\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES CTR(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_CTR)

#if defined(MBEDTLS_CIPHER_MODE_ECB)
    APP_PRINT("\n>>> Start AES ECB\n");
    ret = crypto_sample_aes_ecb();
    if (ret == pdFALSE)
    {
        APP_PRINT(">>> SUCCESS: AES ECB\n");
    }
    else
    {
        APP_PRINT(">>> FAIL: AES ECB(%d)\n", ret);
    }
#endif // (MBEDTLS_CIPHER_MODE_ECB)

    APP_PRINT("\n>>> END Mbedded Crypto Example\n");
}
