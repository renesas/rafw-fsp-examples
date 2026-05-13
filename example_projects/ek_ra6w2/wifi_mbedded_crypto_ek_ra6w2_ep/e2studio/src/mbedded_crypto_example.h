/***********************************************************************************************************************
 * File Name    : mbedded_crypto_example.h
 * Description  : Header of mbedded cypto example.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef MBEDDED_CRYPTO_EXAMPLE_H
    #define MBEDDED_CRYPTO_EXAMPLE_H

    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>

    #include "FreeRTOS.h"
    #include "sdk_defs.h"
    #include "common_utils.h"

typedef unsigned int size_t;

// Definition of Crypto modes
    #define MBEDTLS_CIPHER_MODE_CCM
    #define MBEDTLS_CIPHER_MODE_GCM
    #define MBEDTLS_CIPHER_MODE_CBC
    #define MBEDTLS_CIPHER_MODE_CFB
    #define MBEDTLS_CIPHER_MODE_CTR
    #define MBEDTLS_CIPHER_MODE_ECB

// Function of Crypto samples
    int crypto_sample_aes_ccm(void);
    int crypto_sample_aes_gcm(void);
    int crypto_sample_aes_cbc(void);
    int crypto_sample_aes_cfb(void);
    int crypto_sample_aes_ctr(void);
    int crypto_sample_aes_ecb(void);

    void print_ep_info();
    void  *crypto_sample_calloc(size_t n, size_t size);
    void crypto_sample_free(void *f);

    void mbedded_crypto_example(void);
#endif /* MBEDDED_CRYPTO_EXAMPLE_H */
