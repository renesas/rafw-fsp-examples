/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * File Name    : dtls_client.c
 * Description  : DTLS client using PSK (RA6W1, FSP-safe)
 **********************************************************************************************************************/

#include "dtls_client.h"
#include "config.h"
#include "common_utils.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include <errno.h>
#include <string.h>

/* mbedTLS */
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

/* =====================================================================================================================
 * Macros
 * ===================================================================================================================*/

#define DTLS_CLIENT_STACK_SIZE          (4096)
#define DTLS_CLIENT_PRIORITY            (5)

/* =====================================================================================================================
 * PSK Details
 * ===================================================================================================================*/

/* Pre-Shared Key (PSK)
 * Must match the PSK configured in the OpenSSL DTLS server
 */
static const unsigned char psk_key[] =
{
    0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08
};

/* PSK Identity
 * Must match the identity configured in the OpenSSL DTLS server
 */
static const char *psk_identity = "YOUR_PSK_IDENTITY";

/* =====================================================================================================================
 * Type Definitions
 * ===================================================================================================================*/

typedef struct
{
    int socket;
} dtls_net_ctx_t;

typedef struct
{
    uint32_t   int_ms;
    uint32_t   fin_ms;
    TickType_t start;
} dtls_timer_t;

/* =====================================================================================================================
 * Private Functions
 * ===================================================================================================================*/

/***********************************************************************************************************************
 * Function Name: dtls_set_timer
 * Description  : Sets DTLS retransmission timer values.
 * Argument     : ctx
 *                int_ms
 *                fin_ms
 * Return Value : None
 **********************************************************************************************************************/
static void dtls_set_timer(void *ctx, uint32_t int_ms, uint32_t fin_ms)
{
    dtls_timer_t *t = (dtls_timer_t *) ctx;

    t->int_ms = int_ms;
    t->fin_ms = fin_ms;
    t->start  = xTaskGetTickCount();
}

/***********************************************************************************************************************
 * Function Name: dtls_get_timer
 * Description  : Returns DTLS timer state.
 * Argument     : ctx
 * Return Value : -1 : Timer not running
 *                 0 : No timeout
 *                 1 : Intermediate timeout
 *                 2 : Final timeout
 **********************************************************************************************************************/
static int dtls_get_timer(void *ctx)
{
    dtls_timer_t *t = (dtls_timer_t *) ctx;

    if (t->fin_ms == 0)
    {
        return -1;
    }

    uint32_t elapsed =
        (xTaskGetTickCount() - t->start) * portTICK_PERIOD_MS;

    if (elapsed >= t->fin_ms)
    {
        return 2;
    }

    if (elapsed >= t->int_ms)
    {
        return 1;
    }

    return 0;
}

/***********************************************************************************************************************
 * Function Name: dtls_send
 * Description  : DTLS BIO send callback.
 * Argument     : ctx
 *                buf
 *                len
 * Return Value : Number of bytes sent or mbedTLS error code
 **********************************************************************************************************************/
static int dtls_send(void *ctx, const unsigned char *buf, size_t len)
{
    dtls_net_ctx_t *net = (dtls_net_ctx_t *) ctx;

    int ret = send(net->socket, buf, len, 0);

    if (ret < 0)
    {
        int err = errno;

        if ((err == EAGAIN) || (err == EWOULDBLOCK))
        {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }

        APP_PRINT("[ERROR] dtls_send errno=%d\r\n", err);

        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    return ret;
}

/***********************************************************************************************************************
 * Function Name: dtls_recv
 * Description  : DTLS BIO receive callback.
 * Argument     : ctx
 *                buf
 *                len
 * Return Value : Number of bytes received or mbedTLS error code
 **********************************************************************************************************************/
static int dtls_recv(void *ctx, unsigned char *buf, size_t len)
{
    dtls_net_ctx_t *net = (dtls_net_ctx_t *) ctx;

    int ret = recv(net->socket, buf, len, 0);

    if (ret < 0)
    {
        int err = errno;

        if ((err == EAGAIN) || (err == EWOULDBLOCK))
        {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }

        APP_PRINT("[ERROR] dtls_recv errno=%d\r\n", err);

        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    if (ret == 0)
    {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return ret;
}

/***********************************************************************************************************************
 * Function Name: dtls_client_task
 * Description  : DTLS client task implementation.
 * Argument     : arg
 * Return Value : None
 **********************************************************************************************************************/
static void dtls_client_task(void *arg)
{
    FSP_PARAMETER_NOT_USED(arg);

    int ret;
    int sock = -1;

    struct sockaddr_in server;
    struct timeval     timeout;

    dtls_net_ctx_t net_ctx;
    dtls_timer_t   timer_ctx = {0};

    mbedtls_ssl_context      ssl;
    mbedtls_ssl_config       conf;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0)
    {
        APP_PRINT("[ERROR] Socket creation failed errno=%d\r\n", errno);
        goto cleanup;
    }

    memset(&server, 0, sizeof(server));

    server.sin_family      = AF_INET;
    server.sin_port        = htons(DTLS_SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(DTLS_SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        APP_PRINT("[ERROR] UDP connect failed errno=%d\r\n", errno);
        goto cleanup;
    }

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    setsockopt(sock,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &timeout,
               sizeof(timeout));

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                mbedtls_entropy_func,
                                &entropy,
                                NULL,
                                0);

    if (ret != 0)
    {
        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] ctr_drbg_seed failed: %s\r\n", err_buf);

        goto cleanup;
    }

    ret = mbedtls_ssl_config_defaults(&conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret != 0)
    {
        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] ssl_config_defaults failed: %s\r\n", err_buf);

        goto cleanup;
    }

    ret = mbedtls_ssl_conf_psk(&conf,
                               psk_key,
                               sizeof(psk_key),
                               (const unsigned char *) psk_identity,
                               strlen(psk_identity));

    if (ret != 0)
    {
        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] ssl_conf_psk failed: %s\r\n", err_buf);

        goto cleanup;
    }

    mbedtls_ssl_conf_rng(&conf,
                         mbedtls_ctr_drbg_random,
                         &ctr_drbg);

    ret = mbedtls_ssl_setup(&ssl, &conf);

    if (ret != 0)
    {
        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] ssl_setup failed: %s\r\n", err_buf);

        goto cleanup;
    }

    net_ctx.socket = sock;

    mbedtls_ssl_set_bio(&ssl,
                        &net_ctx,
                        dtls_send,
                        dtls_recv,
                        NULL);

    mbedtls_ssl_set_timer_cb(&ssl,
                             &timer_ctx,
                             dtls_set_timer,
                             dtls_get_timer);

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
        if ((ret == MBEDTLS_ERR_SSL_WANT_READ) ||
            (ret == MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] Handshake failed: %s (0x%08X)\r\n",
                  err_buf,
                  -ret);

        goto cleanup;
    }

    APP_PRINT(" Handshake SUCCESS\r\n");

    const char msg[] =
        "Hello. This is client. Hi from RA6W1 DTLS client";

    ret = mbedtls_ssl_write(&ssl,
                            (const unsigned char *) msg,
                            strlen(msg));

    if (ret > 0)
    {
        APP_PRINT(" Sent %d bytes securely\r\n", ret);
    }
    else
    {
        char err_buf[128];

        mbedtls_strerror(ret, err_buf, sizeof(err_buf));

        APP_PRINT("[ERROR] ssl_write failed: %s (0x%08X)\r\n",
                  err_buf,
                  -ret);
    }

cleanup:

    if (sock >= 0)
    {
        closesocket(sock);
    }

    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    vTaskDelete(NULL);
}

/***********************************************************************************************************************
 * Function Name: dtls_client_start
 * Description  : Creates DTLS client task.
 * Argument     : None
 * Return Value : FreeRTOS task creation status
 **********************************************************************************************************************/
BaseType_t dtls_client_start(void)
{
    return xTaskCreate(dtls_client_task,
                       "DTLS_CLIENT",
                       DTLS_CLIENT_STACK_SIZE,
                       NULL,
                       DTLS_CLIENT_PRIORITY,
                       NULL);
}
