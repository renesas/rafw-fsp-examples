/***********************************************************************************************************************
 * File Name    : https_server_client.c
 * Description  : HTTPS server and client handling
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "mbedtls/ssl.h"
#include "lwip/netif.h"
#include "common_data.h"
#include "config.h"
#include "common_utils.h"
#include "https_server_client.h"
#include "r_rtc_w.h"

extern TaskHandle_t xAppTaskHandle;
static int server_transfer_error = 0;
static int server_transfer_complete = 0;
static int client_transfer_complete = 0;
static int client_transfer_error = 0;

/* HTTPS callback handler for client and server events */
void g_https0_callback(https_callback_args_t *p_args)
{
    switch(p_args->event)
    {
        case HTTPS_EVENT_SERVER_RECVED:
        {
            err_t err = *(int8_t *)p_args->p_param;

            APP_PRINT_INFO("Event: HTTPS_EVENT_SERVER_RECVED\n");
            if (err != ERR_OK || !p_args->len)
            {
                server_transfer_error = 1;
            }

            if(err == ERR_OK)
            {
                server_transfer_complete = 1;
            }

            APP_PRINT_INFO("Client Request Header:\n %s\n",p_args->payload);
            break;
        }

        case HTTPS_EVENT_SERVER_ERR_RESULT:
        {
            err_t err = *(int8_t *)p_args->p_param;

            APP_PRINT_INFO("Event: HTTPS_EVENT_SERVER_ERR_RESULT\n");
            if(err != ERR_OK)
            {
                server_transfer_error = 1;
            }

            break;
        }

        case HTTPS_EVENT_CLIENT_RESULT:
        {
            uint32_t httpc_result = *(uint32_t *)p_args->payload;
            uint32_t rx_content_len = p_args->len;
            err_t err = *(int8_t *)p_args->p_param;

            APP_PRINT_INFO("Event: HTTPS_EVENT_CLIENT_RESULT\n");
            APP_PRINT_INFO("httpc_result: %ld, received: %d byte, err: %d\n", httpc_result, (int)rx_content_len, err);
            if(err == ERR_OK)
            {
                client_transfer_complete = 1;
            }
            else
            {
                client_transfer_error = 1;
            }

            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED:
        {
            err_t err = *(int8_t *)p_args->p_param;
            uint32_t rx_content_len = p_args->len;

            APP_PRINT_INFO("Event: HTTPS_EVENT_CLIENT_RECVED\n");
            APP_PRINT_INFO("received: %d byte, err: %d\n", (int)rx_content_len, err);
            xTaskNotify(xAppTaskHandle, HTTPS_CLIENT_RECV_EVENT, eSetBits);
            break;
        }

        case HTTPS_EVENT_CLIENT_RECVED_DECODED:
        {
            APP_PRINT_INFO("Event: HTTPS_EVENT_CLIENT_RECVED_DECODED\n");
            break;
        }

        case HTTPS_EVENT_CLIENT_GET_DONE:
        {
            APP_PRINT_INFO("Event: HTTPS_EVENT_CLIENT_GET_DONE\n");
            break;
        }

        default:
            APP_PRINT_INFO("Http:UnKnown event received\n");
            break;
    }
}

/* Waits until client transfer is complete or timeout/error occurs */
static fsp_err_t wait_for_client_transfer_complete(bool allow_error)
{
    uint32_t timeout = 1000000;

    while (timeout > 0)
    {
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
        timeout--;
        if (client_transfer_error)
        {
            if (allow_error)
            {
                break;
            }

            client_transfer_error = 0U;

            return FSP_ERR_INVALID_STATE;
        }

        if (client_transfer_complete)
        {
            break;
        }

        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    }

    if (0U == timeout)
    {
        return FSP_ERR_TIMEOUT;
    }

    client_transfer_complete = 0U;
    client_transfer_error    = 0U;

    return FSP_SUCCESS;
}

/* Initializes and starts the HTTPS server */
void server_start()
{
    fsp_err_t err = FSP_SUCCESS;

    /* Server Start */
#ifdef HTTPS
    sever_sec.p_tls_srv_key = tls_srv_key;
    sever_sec.tls_srv_key_len = tls_srv_key_len;
    sever_sec.p_tls_srv_cert = tls_srv_cert;
    sever_sec.tls_srv_cert_len = tls_srv_cert_len;
    sever_sec.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
    sever_sec.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
    sever_sec.p_priv_pass = NULL;
    sever_sec.priv_pass_len = 0;
#endif //HTTPS

    err = RM_HTTPS_W_Open((https_ctrl_t *)&g_https_w0_ctrl, &g_https_w0_cfg);
    err = RM_HTTPS_W_ServerStart ((https_ctrl_t *)&g_https_w0_ctrl,
                                  #ifdef HTTPS
                                      &sever_sec
                                  #else
                                      NULL
                                  #endif
                                 );
    if (err == FSP_SUCCESS)
    {
        APP_PRINT_INFO("\r\nHttps Server Running...\n");
    }

    xTaskNotify(xAppTaskHandle, HTTPS_SERVER_START_EVENT, eSetBits);
}

/* Sends an HTTPS client request and waits for response */
void client_request()
{
    fsp_err_t err = FSP_SUCCESS;

#ifdef HTTPS
    sec_conn.ca = ca_cert;
    sec_conn.ca_len = ca_cert_len;
    sec_conn.tls_ver_min = MBEDTLS_SSL_VERSION_TLS1_3;
    sec_conn.tls_ver_max = MBEDTLS_SSL_VERSION_TLS1_3;
    sec_conn.auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;
    sec_conn.incoming_len = 0;
    sec_conn.outgoing_len = 0;
#endif //HTTPS

    http_client_request_t request =
    {
        .op_code = HTTP_CLIENT_OPCODE_GET,
        .port = SERVER_PORT,
        .hostname = "localhost",
#ifdef HTTPS
        .insecure = pdTRUE,
        .https_conf = sec_conn,
        .path = "https://localhost/index.html",
#else
        .insecure = pdFALSE,
        .path = "http://localhost/index.html",
#endif //HTTPS
    };

    /* Client Send Request */
    err = RM_HTTPS_W_ClientSendRequest((https_ctrl_t *)&g_https_w0_ctrl, &request);

    if (err == FSP_SUCCESS)
    {
        err = wait_for_client_transfer_complete(false);

        if (err != FSP_SUCCESS)
        {
            APP_PRINT_INFO("We are having trouble finding that site\n");
        }
    }
    else
    {
        APP_ERR_PRINT("send failed\n");
    }
}

/* Stops and closes the HTTPS server */
void deinit_server()
{
    APP_PRINT_INFO("Https Server closing...\n");
    RM_HTTPS_W_ServerStop((https_ctrl_t *)&g_https_w0_ctrl);
    RM_HTTPS_W_Close((https_ctrl_t *)&g_https_w0_ctrl);
}

/* Sets system time using RTC */
void set_sys_time()
{
    struct tm correction;
    memset(&correction, 0x00, sizeof(struct tm));

    /* Year */
    correction.tm_year = 2024 - 1900;

    /* Month */
    correction.tm_mon  = 12 - 1;

    /* Day */
    correction.tm_mday = 2;

    /* Season flag, such as daylight saving time */
    correction.tm_isdst = -1;
    R_RTC_W_CalendarTimeSet(R_RTC_W_GetCtrl(), &correction);
}
