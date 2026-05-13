/***********************************************************************************************************************
* File Name    : provisioning_helper.c
* Description  : functions and definitions for TLS connection
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "provisioning_helper.h"
#include "provisioning_tls_key.h"

/***********************************************************************************************************************
 * Defines
 **********************************************************************************************************************/
#define SOFTAP_PROV_NAME        "PROV_TLS"
#define SOFTAP_PROV_STACK_SZ    2048

#define PROV_TLS_SOCK_NAME      "TLS_SOCK"
#define PROV_TLS_SVR_PORT       "PROV_PORT"
#define PROV_TLS_SVR_TIMEOUT    1000  // 100   EU FAE TLS Fail issue 100 ==> 1000

#define TCP_SERVER_WIN_SIZE     (1024 * 4)
#define TCP_LISTEN_BAGLOG       5
#define SOFTAP_PROV_PORT_NO     9900

size_t prov_tls_ca_len   = sizeof(prov_tls_ca);
size_t prov_tls_cert_len = sizeof(prov_tls_cert);
size_t prov_tls_key_len  = sizeof(prov_tls_key);

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
int provisioning_tls_svr_rsa_decrypt_func(void *ctx,
                                          size_t *olen,
                                          const unsigned char *input,
                                          unsigned char *output,
                                          size_t output_max_len);
int provisioning_tls_svr_rsa_sign_func(void *ctx,
                                       int(* f_rng)(void *, unsigned char *, size_t),
                                       void *p_rng,
                                       mbedtls_md_type_t md_alg,
                                       unsigned int hashlen,
                                       const unsigned char *hash,
                                       unsigned char *sig);
size_t provisioning_tls_svr_rsa_key_len_func(void *ctx);

/**
 ****************************************************************************************
 * @brief buffer allocation
 * @param[in] n - number
 * @param[in] size - buffer size
 * @return  buffer
 ****************************************************************************************
 */
void * provisioning_calloc(size_t n, size_t size)
{
    void * buf    = NULL;
    size_t buflen = (n * size);

    buf = pvPortMalloc(buflen);
    if (buf)
    {
        memset(buf, 0x00, buflen);
    }

    return buf;
}

/**
 * @brief Local Definition
 */
/**
 ****************************************************************************************
 * @brief buffer free
 * @param[in] f - buffer to free
 * @return  void
 ****************************************************************************************
 */
void provisioning_free(void * f)
{
    if (f == NULL)
    {
        return;
    }

    vPortFree(f);
}

/**
 ****************************************************************************************
 * @brief decrypt function
 * @param ctx - RSA context
 * @param olen - length of the plaintext
 * @param input - the buffer holding the encrypted data
 * @param output - the buffer used to hold the plaintext
 * @param output_max_len - the maximum length of output buffer
 * @return 0: Successful or Error code : Failure
 ****************************************************************************************
 */
int provisioning_tls_svr_rsa_decrypt_func(void *ctx,
                                          size_t *olen,
                                          const unsigned char *input,
                                          unsigned char *output,
                                          size_t output_max_len)
{
    return mbedtls_rsa_pkcs1_decrypt((mbedtls_rsa_context *) ctx, NULL, NULL, olen, input, output, output_max_len);
}

/**
 ****************************************************************************************
 * @brief signing function
 * @param ctx - RSA context
 * @param f_rng - the RNG function
 * @param p_rng - the RNG parameter
 * @param md_alg - the message digest algorithm used to hash the original data
 * @param hashlen - the length of the message digest
 * @param hash - the buffer holding the message digest
 * @param sig - the buffer to holding the message digest
 * @return 0: Successful or Error code : Failure
 ****************************************************************************************
 */
int provisioning_tls_svr_rsa_sign_func(void *ctx,
                                       int(*f_rng)(void *, unsigned char *, size_t),
                                       void *p_rng,
                                       mbedtls_md_type_t md_alg,
                                       unsigned int hashlen,
                                       const unsigned char *hash,
                                       unsigned char *sig)
{
    return mbedtls_rsa_pkcs1_sign((mbedtls_rsa_context *)ctx, f_rng, p_rng, md_alg, hashlen, hash, sig);
}

/**
 ****************************************************************************************
 * @brief function returning key length
 * @param ctx - RSA context
 * @return key length
 ****************************************************************************************
 */
size_t provisioning_tls_svr_rsa_key_len_func(void *ctx)
{
    return ((const mbedtls_rsa_context *)ctx)->MBEDTLS_PRIVATE(len);
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server configure initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_init_config(provisioning_tls_server_cfg_t *config)
{
    config->local_port   = SOFTAP_PROV_PORT_NO;
    config->ssl_ctx      = NULL;
    config->ssl_conf     = NULL;
    config->ctr_drbg_ctx = NULL;
    config->entropy_ctx  = NULL;
    config->ca_cert_crt  = NULL;
    config->cert_crt     = NULL;
    config->pkey_ctx     = NULL;
    config->pkey_alt_ctx = NULL;

    return ERR_OK;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server tcp socket initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_init_socket(provisioning_tls_server_cfg_t *config)
{
    int  ret         = 0;
    char str_port[8] = {0x00, };

    /* Convert type to string */
    snprintf(str_port, sizeof(str_port), "%d", config->local_port);
    mbedtls_net_init(&config->sock_ctx);
    ret = mbedtls_net_bind(&config->sock_ctx, NULL, str_port, MBEDTLS_NET_PROTO_TCP);
    if (ret)
    {
        PRINTF("[%s] Failed to bind socket(0x%x)\r\n", __func__, -ret);
    }

    return ret;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server tcp socket deinitialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_deinit_socket(provisioning_tls_server_cfg_t *config)
{
    mbedtls_net_free(&config->sock_ctx);

    return 0;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server session initialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_init_ssl(provisioning_tls_server_cfg_t *config)
{
    // Init SSL context
    if (!config->ssl_ctx)
    {
        config->ssl_ctx = provisioning_calloc(1, sizeof(mbedtls_ssl_context));
        if (!config->ssl_ctx)
        {
            PRINTF("[%s] Failed to allocate memory for ssl context\r\n", __func__);
            goto error;
        }

        mbedtls_ssl_init(config->ssl_ctx);
    }

    /* Init SSL config */
    if (!config->ssl_conf)
    {
        config->ssl_conf = provisioning_calloc(1, sizeof(mbedtls_ssl_config));
        if (!config->ssl_conf)
        {
            PRINTF("[%s] Failed to allocate memory for ssl config\r\n", __func__);
            goto error;
        }

        mbedtls_ssl_config_init(config->ssl_conf);
    }

    /* Init CTR-DRBG context */
    if (!config->ctr_drbg_ctx)
    {
        config->ctr_drbg_ctx = provisioning_calloc(1, sizeof(mbedtls_ctr_drbg_context));
        if (!config->ctr_drbg_ctx)
        {
            PRINTF("[%s] Failed to allocate memory for ctr-drbg\r\n", __func__);
            goto error;
        }

        mbedtls_ctr_drbg_init(config->ctr_drbg_ctx);
    }

    /* Init Entropy context */
    if (!config->entropy_ctx)
    {
        config->entropy_ctx = provisioning_calloc(1, sizeof(mbedtls_entropy_context));
        if (!config->entropy_ctx)
        {
            PRINTF("[%s] Failed to allocate memory for entropy\r\n", __func__);
            goto error;
        }

        mbedtls_entropy_init(config->entropy_ctx);
    }

    /* Init Certificate context */
    if (!config->ca_cert_crt)
    {
        config->ca_cert_crt = provisioning_calloc(1, sizeof(mbedtls_x509_crt));
        if (!config->ca_cert_crt)
        {
            PRINTF("[%s] Failed to allocate memory for certificate\r\n", __func__);
            goto error;
        }

        mbedtls_x509_crt_init(config->ca_cert_crt);
    }

    /* Init Certificate context */
    if (!config->cert_crt)
    {
        config->cert_crt = provisioning_calloc(1, sizeof(mbedtls_x509_crt));
        if (!config->cert_crt)
        {
            PRINTF("[%s] Failed to allocate memory for certificate\r\n", __func__);
            goto error;
        }

        mbedtls_x509_crt_init(config->cert_crt);
    }

    /* Init Private key context */
    if (!config->pkey_ctx)
    {
        config->pkey_ctx = provisioning_calloc(1, sizeof(mbedtls_pk_context));
        if (!config->pkey_ctx)
        {
            PRINTF("[%s] Failed to allocate memory for private key\r\n", __func__);
            goto error;
        }

        mbedtls_pk_init(config->pkey_ctx);
    }

    /* Init Private key context for alt */
    if (!config->pkey_alt_ctx)
    {
        config->pkey_alt_ctx = provisioning_calloc(1, sizeof(mbedtls_pk_context));
        if (!config->pkey_alt_ctx)
        {
            PRINTF("[%s] Failed to allocate memory for private key - alt\r\n", __func__);
            goto error;
        }

        mbedtls_pk_init(config->pkey_alt_ctx);
    }

    return ERR_OK;

error:
    provisioning_tls_svr_deinit_ssl(config);

    return ERR_MEM;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server sesstion setup
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_setup_ssl(provisioning_tls_server_cfg_t *config)
{
    const char *pers = "tls_server_sample";
    int ret = 0;

    /* Set default configuration */
    ret = mbedtls_ssl_config_defaults(config->ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret)
    {
        PRINTF("[%s] Failed to set default ssl config(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    /* Parse CA */
    ret = mbedtls_x509_crt_parse(config->ca_cert_crt, prov_tls_ca, prov_tls_ca_len);
    if (ret)
    {
        PRINTF("[%s] Failed to parse CA(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    mbedtls_ssl_conf_ca_chain(config->ssl_conf, config->ca_cert_crt, NULL);

    /* Parse certificate */
    ret = mbedtls_x509_crt_parse(config->cert_crt, prov_tls_cert, prov_tls_cert_len);
    if (ret)
    {
        PRINTF("[%s] Failed to parse cert(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    /* Parse private key */
    ret = mbedtls_pk_parse_key(config->pkey_ctx,
                               prov_tls_key,
                               prov_tls_key_len,
                               NULL,
                               0,
                               mbedtls_ctr_drbg_random,
                               config->ctr_drbg_ctx);
    if (ret)
    {
        PRINTF("[%s] Failed to parse private key(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    if (mbedtls_pk_get_type(config->pkey_ctx) == MBEDTLS_PK_RSA)
    {
        ret = mbedtls_pk_setup_rsa_alt(config->pkey_alt_ctx,
                                       (void *) mbedtls_pk_rsa(*config->pkey_ctx),
                                       provisioning_tls_svr_rsa_decrypt_func,
                                       provisioning_tls_svr_rsa_sign_func,
                                       provisioning_tls_svr_rsa_key_len_func);
        if (ret)
        {
            PRINTF("[%s] Failed to set rsa alt(0x%x)\r\n", __func__, -ret);

            return ret;
        }

        /* Import certificate & private key */
        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_alt_ctx);
        if (ret)
        {
            PRINTF("[%s] Failed to set cert & private key(0x%x)\r\n", __func__, -ret);

            return ret;
        }
    }
    else
    {
        /* Import certificate & private key */
        ret = mbedtls_ssl_conf_own_cert(config->ssl_conf, config->cert_crt, config->pkey_ctx);
        if (ret)
        {
            PRINTF("[%s] Failed to set cert & private key(0x%x)\r\n", __func__, -ret);

            return ret;
        }
    }

    ret = mbedtls_ctr_drbg_seed(config->ctr_drbg_ctx,
                                mbedtls_entropy_func,
                                config->entropy_ctx,
                                (const unsigned char *) pers,
                                strlen(pers));
    if (ret)
    {
        PRINTF("[%s] Faield to set ctr drbg seed(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    mbedtls_ssl_conf_rng(config->ssl_conf, mbedtls_ctr_drbg_random, config->ctr_drbg_ctx);
    /* Don't care peer certificate' verification in this sample. */
    mbedtls_ssl_conf_authmode(config->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    /* Set up an SSL context for use. */
    ret = mbedtls_ssl_setup(config->ssl_ctx, config->ssl_conf);
    if (ret)
    {
        PRINTF("[%s] Failed to set ssl config(0x%x)\r\n", __func__, -ret);

        return ret;
    }

    return ERR_OK;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server session deinitialize
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_deinit_ssl(provisioning_tls_server_cfg_t *config)
{
    /* Deinit SSL context */
    if (config->ssl_ctx)
    {
        mbedtls_ssl_free(config->ssl_ctx);
        provisioning_free(config->ssl_ctx);
    }

    /* Deinit SSL config */
    if (config->ssl_conf)
    {
        mbedtls_ssl_config_free(config->ssl_conf);
        provisioning_free(config->ssl_conf);
    }

    /* Deinit CTR-DRBG context */
    if (config->ctr_drbg_ctx)
    {
        mbedtls_ctr_drbg_free(config->ctr_drbg_ctx);
        provisioning_free(config->ctr_drbg_ctx);
    }

    /* Deinit Entropy context */
    if (config->entropy_ctx)
    {
        mbedtls_entropy_free(config->entropy_ctx);
        provisioning_free(config->entropy_ctx);
    }

    /* Deinit Certificate context */
    if (config->cert_crt)
    {
        mbedtls_x509_crt_free(config->ca_cert_crt);
        provisioning_free(config->ca_cert_crt);
    }

    /* Deinit Certificate context */
    if (config->cert_crt)
    {
        mbedtls_x509_crt_free(config->cert_crt);
        provisioning_free(config->cert_crt);
    }

    /* Deinit Private key context */
    if (config->pkey_ctx)
    {
        mbedtls_pk_free(config->pkey_ctx);
        provisioning_free(config->pkey_ctx);
    }

    /* Deinit Private key context for alt */
    if (config->pkey_alt_ctx)
    {
        mbedtls_pk_free(config->pkey_alt_ctx);
        provisioning_free(config->pkey_alt_ctx);
    }

    config->ssl_ctx      = NULL;
    config->ssl_conf     = NULL;
    config->ctr_drbg_ctx = NULL;
    config->entropy_ctx  = NULL;
    config->ca_cert_crt  = NULL;
    config->cert_crt     = NULL;
    config->pkey_ctx     = NULL;
    config->pkey_alt_ctx = NULL;

    return 0;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server session shutdown
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_shutdown_ssl(provisioning_tls_server_cfg_t *config)
{
    int ret = 0;

    ret = mbedtls_ssl_session_reset(config->ssl_ctx);
    if (ret)
    {
        PRINTF("[%s] Failed to reset session(0x%x)\r\n", __func__, -ret);
    }

    return ret;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning server session handshake
 * @param[in] config the TLS server information
 * @return int err_enum_t
 ****************************************************************************************
 */
int provisioning_tls_svr_do_handshake(provisioning_tls_server_cfg_t *config)
{
    int ret = 0;

    while ((ret = mbedtls_ssl_handshake(config->ssl_ctx)) != 0)
    {
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            PRINTF("[%s]Failed to process tls handshake(0x%x)\r\n", __func__, -ret);
            break;
        }
    }

    return ret;
}

/* EOF */
