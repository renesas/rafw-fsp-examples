/***********************************************************************************************************************
 * File Name    : tls_server.c
 * Description  : Contains data structure and echo udp server function definitions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "lwip/altcp.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tls.h"
#include "lwip/priv/altcp_priv.h"
#include "lwip/tcp.h"
#include "lwip/err.h"

#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/priv/altcp_priv.h"
#include "lwip/mem.h"

#include "task.h"
#include "ssl.h"
#include "tls_server_client.h"
#include "altcp_tls_mbedtls_structs.h"

typedef struct
{
    struct altcp_pcb *pcb;
    int client_id;
} conn_info;

struct altcp_tls_config *tls_srv_config = NULL;
struct altcp_pcb *p_altcp_pcb = NULL;
static TaskHandle_t server_task_handle = NULL;
static int client_count = 0;

static uint8_t tls_srv_key[] =
    "-----BEGIN EC PARAMETERS-----\n"
    "BggqhkjOPQMBBw==\n"
    "-----END EC PARAMETERS-----\n"
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHcCAQEEIA0ZIG9cCFP78B7flpAmm+xD/EFTO4FSSne3bNMgTnm6oAoGCCqGSM49\n"
    "AwEHoUQDQgAEvGBVjn6z152DH8iBvEXqEUXDxkJ3Y28M5R/OVC+WtQTKa47Zur/l\n"
    "dR3u/nOWuBIwRaQk1T9iP1qpa8fNAf+CpA==\n"
    "-----END EC PRIVATE KEY-----\n";

static size_t tls_srv_key_len = sizeof(tls_srv_key);

static uint8_t  tls_srv_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDtzCCAp+gAwIBAgIUeblhDZyEJUjr+MJ6sADm9nzDuIYwDQYJKoZIhvcNAQEL\n"
    "BQAwZTELMAkGA1UEBhMCanAxDjAMBgNVBAgMBU9zYWthMRIwEAYDVQQHDAlPc2Fr\n"
    "YS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxlIEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoG\n"
    "A1UEAwwDdGVrMB4XDTI0MTAwMjEzMDI0OVoXDTI1MTAwMjEzMDI0OVowgZMxCzAJ\n"
    "BgNVBAYTAkpQMQ4wDAYDVQQIDAVUb2t5bzEQMA4GA1UEBwwHS29kYWlyYTEQMA4G\n"
    "A1UECgwHUmVuZXNhczENMAsGA1UECwwEU1dURDESMBAGA1UEAwwJbG9jYWxob3N0\n"
    "MS0wKwYJKoZIhvcNAQkBFh5ha2loaXRvLm9rdW11cmEudWpAcmVuZXNhcy5jb20w\n"
    "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS8YFWOfrPXnYMfyIG8ReoRRcPGQndj\n"
    "bwzlH85UL5a1BMprjtm6v+V1He7+c5a4EjBFpCTVP2I/Wqlrx80B/4Kko4H6MIH3\n"
    "MEcGA1UdEQRAMD6CCWxvY2FsaG9zdIIVbG9jYWxob3N0LmxvY2FsZG9tYWluhwR/\n"
    "AAABggNhcHCCD2FwcC5sb2NhbGRvbWFpbjAdBgNVHQ4EFgQUIdO+KVtvxHq23zq9\n"
    "c6Y9urtQiGowgYwGA1UdIwSBhDCBgaFppGcwZTELMAkGA1UEBhMCanAxDjAMBgNV\n"
    "BAgMBU9zYWthMRIwEAYDVQQHDAlPc2FrYS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxl\n"
    "IEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoGA1UEAwwDdGVrghROcyfj2vaChNf/sD5A\n"
    "Endq5udyhzANBgkqhkiG9w0BAQsFAAOCAQEAWRaMsAb3F5k0XK1/C64ixNnVNKhC\n"
    "nsFHOqANkTWELBER9xHwsW8JzjiSIBNXzZIIXgRCB1B4Tg+QpvklGBPi85JieD1r\n"
    "Sv6LTMtyMPLOXutJYwPxOYcxH5McWCsin9fHXswFvyCTDReiyQJRJRjc7TLt+rl6\n"
    "O/pp2k2fQ4YHEjOMiYxJ+FhS1ZorsK9l0PMhVHRyakEEmDb28cq4tEvuPE/dGtcG\n"
    "PNTE/0p+I59gGKLPdyzBeMInskuS7dGBy9uaoh6ITliT/v5oDH2VlRipCm93CzRu\n"
    "+v38SXOqHgFEg6GKH6Qz3NYfvfm95VULXXK0JnykSQR8GQgdHvIh+AXrVw==\n"
    "-----END CERTIFICATE-----\n";

static size_t tls_srv_cert_len = sizeof(tls_srv_cert);

/*
 * Close and clean up client connection, free resources
 */
static void close_client_connection(conn_info *conn)
{
    if (conn && conn->pcb)
    {
        altcp_close(conn->pcb);
        altcp_free(conn->pcb);
        conn->pcb = NULL;
    }

    client_count--;
    free(conn);
}

/*
 * Handle data received from client and echo it back.
 */
static err_t client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    conn_info *conn = (conn_info *)arg;
    char buf[128];
    int len;

    if (!p)
    {
        close_client_connection(conn);

        return ERR_OK;
    }

    if (err != ERR_OK || p->tot_len == 0)
    {
        pbuf_free(p);

        return err;
    }

    altcp_recved(pcb, p->tot_len);;
    len = (p->tot_len < sizeof(buf) - 1) ? p->tot_len : sizeof(buf) - 1;
    memcpy(buf, p->payload, len);
    buf[len] = '\0';

    APP_PRINT_INFO("TLS Server: Received data from Client: %s\r\n", buf);

    /*
     * Echo data back to client
     */
    altcp_write(pcb, p->payload, p->tot_len, TCP_WRITE_FLAG_COPY);
    vTaskDelay(portCONVERT_MS_2_TICKS(100));
    pbuf_free(p);

    return ERR_OK;
}

/*
 * Handle client errors and close the connection.
 */
static void client_error(void *arg, err_t err)
{
    (void)err;
    conn_info *conn = (conn_info *)arg;

    close_client_connection(conn);
}

/*
 * Accept a new client and set up callbacks.
 */
static err_t tls_server_client_handle_task(void *arg, struct altcp_pcb *new_pcb, err_t err)
{
    LWIP_UNUSED_ARG(err);
    LWIP_UNUSED_ARG(arg);

    APP_PRINT_INFO("Accepting Connection \n");
    conn_info *new_conn = malloc(sizeof(conn_info));

    if (!new_conn)
    {
        altcp_close(new_pcb);
        return ERR_MEM;
    }

    new_conn->client_id = ++client_count;
    new_conn->pcb = new_pcb;
    altcp_arg(new_pcb, new_conn);

    /*
     * Set up the various callback functions
     */
    altcp_recv(new_pcb, client_recv);
    altcp_err(new_pcb, client_error);
    APP_PRINT_INFO("Intializing client connection!\n");

    return ERR_OK;
}

/*
 * Runs the TLS server and handles incoming connections.
 */
void tls_server_task(void *pvParameters)
{

    struct altcp_pcb *conn;
    conn = (struct altcp_pcb *) pvParameters;

    altcp_accept(conn, tls_server_client_handle_task);
    APP_PRINT_INFO("TLS server running \n");
    while (1)
    {
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
    }
}

/*
 * Initialize the TLS server.
 * Configures TLS settings, creates the server control block, binds the server port,
 * and starts the main server task.
 */
err_t start_tls_server(void)
{
    err_t err = ERR_OK;
    tls_srv_config = altcp_tls_create_config_server_privkey_cert((const uint8_t *)tls_srv_key, tls_srv_key_len, NULL,
                                                                 0, (const uint8_t *)tls_srv_cert, tls_srv_cert_len);


    if (!tls_srv_config)
    {
        APP_PRINT_INFO("Server Initialization Failed\n");

        return ERR_MEM;
    }

    altcp_tls_config_min_tls_version(tls_srv_config, MBEDTLS_SSL_VERSION_TLS1_2);
    altcp_tls_config_max_tls_version(tls_srv_config, MBEDTLS_SSL_VERSION_TLS1_2);
    altcp_tls_config_authmode(tls_srv_config, MBEDTLS_SSL_VERIFY_NONE);
    p_altcp_pcb = altcp_tls_new(tls_srv_config, IPADDR_TYPE_ANY);

    if (p_altcp_pcb == NULL)
    {
        APP_PRINT_INFO("Failed to initialize connection\n");
        altcp_close(p_altcp_pcb);
        altcp_free(p_altcp_pcb);

        return ERR_MEM;
    }

    if ((err = altcp_bind(p_altcp_pcb, IP_ANY_TYPE, TLS_PORT)) != ERR_OK)
    {
        APP_PRINT_INFO("Socket bind failed\n");
        altcp_close(p_altcp_pcb);
        altcp_free(p_altcp_pcb);

        return false;
    }

    altcp_listen(p_altcp_pcb);

    if (p_altcp_pcb->state == NULL)
    {
       APP_PRINT_INFO("Failed to listen on socket\n");
       altcp_close(p_altcp_pcb);
       altcp_free(p_altcp_pcb);

       return false;
    }

    xTaskCreate(tls_server_task, "TLS_Server_task", ((1024 * 8) / 4), (void *)p_altcp_pcb,
                (tskIDLE_PRIORITY + 2), &server_task_handle);

    return err;
}
