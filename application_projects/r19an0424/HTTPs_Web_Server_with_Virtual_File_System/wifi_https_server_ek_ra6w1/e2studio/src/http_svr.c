/***********************************************************************************************************************
* File Name    : http_srv.c
* Description  : http(s) server functions and configurations
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "common_data.h"
#include "common_utils.h"
#include "http_svr.h"

static https_server_sec_t p_sec;

static const uint8_t tls_srv_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB6DCCAY2gAwIBAgIUPUuczXvddLrhm/ckOgE2CUa9bncwCgYIKoZIzj0EAwIw\n"
    "YTELMAkGA1UEBhMCSU4xDzANBgNVBAgMBktlcmFsYTESMBAGA1UEBwwJS296aGlr\n"
    "b2RlMRQwEgYDVQQKDAtFeGFtcGxlIElvVDEXMBUGA1UEAwwObXlkZXZpY2UubG9j\n"
    "YWwwHhcNMjUwNjE2MTAxOTI3WhcNMjYwNjE2MTAxOTI3WjBhMQswCQYDVQQGEwJJ\n"
    "TjEPMA0GA1UECAwGS2VyYWxhMRIwEAYDVQQHDAlLb3poaWtvZGUxFDASBgNVBAoM\n"
    "C0V4YW1wbGUgSW9UMRcwFQYDVQQDDA5teWRldmljZS5sb2NhbDBZMBMGByqGSM49\n"
    "AgEGCCqGSM49AwEHA0IABCZu8oIidrVJjASgssa5oavfCkQUI93zRxKKTuXN8tsW\n"
    "Aq9Upn8jcCSnZAGhWZhlEHCeyQN6cZFttwWNht56FX2jIzAhMB8GA1UdEQQYMBaC\n"
    "Dm15ZGV2aWNlLmxvY2FshwTAqDKcMAoGCCqGSM49BAMCA0kAMEYCIQDq01321W0O\n"
    "5BeDCx7+Ww/8fzQg6bW5bpI9mTbG99tSTwIhANYIrykIdtDU20unlS01j3cHQM+W\n"
    "ZMx/MmdF3yKUgctH\n"
    "-----END CERTIFICATE-----\n";

static size_t tls_srv_cert_len = sizeof(tls_srv_cert);

static const uint8_t tls_srv_key[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEINGUbjFG3GfiVsV29FG/rh8qjc3PkzqCf6B8CA/3310noAoGCCqGSM49\n"
    "AwEHoUQDQgAEJm7ygiJ2tUmMBKCyxrmhq98KRBQj3fNHEopO5c3y2xYCr1SmfyNw\n"
    "JKdkAaFZmGUQcJ7JA3pxkW23BY2G3noVfQ==\n"
    "-----END EC PRIVATE KEY-----\n";

static size_t tls_srv_key_len = sizeof(tls_srv_key);

void g_https0_callback(https_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case HTTPS_EVENT_SERVER_RECVED:
        {
            APP_PRINT_INFO("Event: HTTPS_EVENT_SERVER_RECVED\n");
            break;
        }

        default:
            APP_PRINT_ERR("Error: Unknown event from http Server received\n");
    }
}

static void config_secure_connection(https_server_sec_t *p_sec_cfg)
{
    p_sec_cfg->p_tls_srv_cert = (uint8_t *)tls_srv_cert;
    p_sec_cfg->p_tls_srv_key = (uint8_t *)tls_srv_key;
    p_sec_cfg->tls_srv_cert_len = tls_srv_cert_len;
    p_sec_cfg->tls_srv_key_len = tls_srv_key_len;
    p_sec_cfg->tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
    p_sec_cfg->tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
    p_sec_cfg->p_priv_pass = NULL;
    p_sec_cfg->priv_pass_len = 0;
}

fsp_err_t init_server()
{
    fsp_err_t err = FSP_SUCCESS;

    /* Open the HTTPS module. */
    err = RM_HTTPS_W_Open((https_ctrl_t *)&g_https_w0_ctrl, &g_https_w0_cfg);

    if (err != FSP_SUCCESS)
    {
        APP_PRINT_INFO("Error: Unable to open http module\n");

        return err;
    }

    config_secure_connection(&p_sec);
    err = RM_HTTPS_W_ServerStart((https_ctrl_t *)&g_https_w0_ctrl, &p_sec);

    if (err != FSP_SUCCESS)
    {
        APP_PRINT_INFO("Error: Http Server start failed\n");
    }
    else
    {
        APP_PRINT_INFO("Https Server Running...\n");
    }

    return err;
}

void deinit_server()
{
    APP_PRINT_INFO("Https Server closing...\n");
    RM_HTTPS_W_ServerStop((https_ctrl_t *)&g_https_w0_ctrl);
    RM_HTTPS_W_Close((https_ctrl_t *)&g_https_w0_ctrl);
}
