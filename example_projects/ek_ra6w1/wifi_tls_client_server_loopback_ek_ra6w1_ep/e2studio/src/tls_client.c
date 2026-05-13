/***********************************************************************************************************************
 * File Name    : tls_client.c
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
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "task.h"
#include "ssl.h"
#include "tls_server_client.h"

/*
 * Server address and port
 */
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8443
#define MESSAGE "Hello TLS Server!"
#define MAX_COUNT 5

static const ip_addr_t loopback_addr = IPADDR4_INIT(PP_HTONL(LWIP_MAKEU32(127, 0, 0, 1)));
static TaskHandle_t client_task_handle = NULL;
static struct altcp_pcb *client_pcb;

static uint8_t  tls_cli_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDtzCCAp+gAwIBAgIUD3dGLL5Tu80RubCUHtxvPEkXac4wDQYJKoZIhvcNAQEL\n"
    "BQAwZTELMAkGA1UEBhMCanAxDjAMBgNVBAgMBU9zYWthMRIwEAYDVQQHDAlPc2Fr\n"
    "YS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxlIEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoG\n"
    "A1UEAwwDdGVrMB4XDTI0MTAwMjE0MDgxM1oXDTI1MTAwMjE0MDgxM1owgZMxCzAJ\n"
    "BgNVBAYTAkpQMQ4wDAYDVQQIDAVUb2t5bzEQMA4GA1UEBwwHS29kYWlyYTEQMA4G\n"
    "A1UECgwHUmVuZXNhczENMAsGA1UECwwEU1dURDESMBAGA1UEAwwJbG9jYWxob3N0\n"
    "MS0wKwYJKoZIhvcNAQkBFh5ha2loaXRvLm9rdW11cmEudWpAcmVuZXNhcy5jb20w\n"
    "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARJ6bN6el/auhaUxNOJqfijLyhgRvsg\n"
    "B7wuBWDjqZUDquY+dZi0CZjw/olfTF4EUL2krGYsU2JTSaodXV9stcO+o4H6MIH3\n"
    "MEcGA1UdEQRAMD6CCWxvY2FsaG9zdIIVbG9jYWxob3N0LmxvY2FsZG9tYWluhwR/\n"
    "AAABggNhcHCCD2FwcC5sb2NhbGRvbWFpbjAdBgNVHQ4EFgQUnioi3CheQvC156qn\n"
    "EMAIHxpnJ0YwgYwGA1UdIwSBhDCBgaFppGcwZTELMAkGA1UEBhMCanAxDjAMBgNV\n"
    "BAgMBU9zYWthMRIwEAYDVQQHDAlPc2FrYS1zaGkxFjAUBgNVBAoMDSJFeGFtcGxl\n"
    "IEluYyIxDDAKBgNVBAsMA0ZvbzEMMAoGA1UEAwwDdGVrghROcyfj2vaChNf/sD5A\n"
    "Endq5udyhzANBgkqhkiG9w0BAQsFAAOCAQEANem9xyhfeZr3pYTnidEKn6gdbFVZ\n"
    "uRpZrQmgNfSYwAd5nh2hchw65mWt/U4vxDgR8sqhzUMSud1Ua2B8yphdCwdoEGlo\n"
    "P8K0my/5bLMv9GM0vFuTDQXmhDiWU9ekB2bpzzhXUhnQJYQqvKpplv4X0v7KNUku\n"
    "r6ElGm3oOQQVkv8VaAkVaU7eODylFE2oViWB4+Q1MBXmo4jDxpjZliQklQTae39A\n"
    "eeMVAKGtwGdojnwTR4mEgkmOmQ1TAysr2INccRA0PIYTvY808iYl1H98Qtp6VBvu\n"
    "q9O0DIiFo0vjNKRnFsrz5ncm7G85uma1u31KPO+1Uy5f83GiInWyH2vC7Q==\n"
    "-----END CERTIFICATE-----\n";

static size_t tls_cli_cert_len = sizeof(tls_cli_cert);

/*
  Close the client connection and free the associated resources
 */
static void close_client_connection(struct altcp_pcb *pcb)
{
    if (pcb)
    {
        altcp_close(pcb);
        altcp_free(pcb);
    }

    APP_PRINT_INFO("Client connection closed.\n");
}

/*
  Callback function to handle data received from the server
 */
static err_t client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    (void)arg;
    if (!p)
    {
        close_client_connection(pcb);

        return ERR_OK;
    }

    if (err != ERR_OK || p->tot_len == 0)
    {
        pbuf_free(p);

        return err;
    }

    APP_PRINT_INFO("[TLS CLIENT]Echo received from server: %s \n\n",MESSAGE);

    /* Acknowledge received data */
    altcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

/*
 * Send data messages to the server periodically
 */
static void send_data_to_server(void *pvParameters)
{
    struct altcp_pcb *pcb = (struct altcp_pcb *)pvParameters;
    err_t err;
    int count = 0;

    while (count <= MAX_COUNT)
    {
        err = altcp_write(pcb, MESSAGE, strlen(MESSAGE), TCP_WRITE_FLAG_COPY);

        if (err != ERR_OK)
        {
            APP_PRINT_INFO("[TLS-Client]Failed to send message : %d\n", err);
        }

        vTaskDelay(portCONVERT_MS_2_TICKS(1000));
        count++;
    }

    APP_PRINT_INFO("All messages sent.Closing connection.\n");
    altcp_close(pcb);
    vTaskDelete(NULL);
}

/*
 * Callback function when client successfully connects to the server
 */
static err_t client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
    (void)arg;
    if (err != ERR_OK)
    {
        APP_PRINT_INFO("Connection failed: %d\n", err);
        close_client_connection(pcb);

        return err;
    }

    APP_PRINT_INFO("[TLS CLIENT]Connection established with TLS server.\n");
    xTaskCreate(send_data_to_server, "TLS_Server_task", ((1024 * 8) / 4), (void *)pcb,
                (tskIDLE_PRIORITY + 2), &client_task_handle);

    return ERR_OK;
}

/*
 * Callback function to handle unexpected client errors
 */
static void client_error(void *arg, err_t err)
{
    APP_PRINT_INFO("Client encountered an error: %d\n", err);
    close_client_connection((struct altcp_pcb *)arg);
}

/*
 * Initialize the TLS client and connect to the server
 */
void tls_client_init(void)
{
    struct altcp_tls_config *tls_config = altcp_tls_create_config_client((const uint8_t *)tls_cli_cert, tls_cli_cert_len);

    if (!tls_config)
    {
        APP_PRINT_INFO("Failed to create TLS config..\n");

        return;
    }

    altcp_tls_config_min_tls_version(tls_config, MBEDTLS_SSL_VERSION_TLS1_2);
    altcp_tls_config_max_tls_version(tls_config, MBEDTLS_SSL_VERSION_TLS1_2);
    altcp_tls_config_authmode(tls_config, MBEDTLS_SSL_VERIFY_NONE);
    client_pcb = altcp_tls_new(tls_config, IPPROTO_TCP);

    if (!client_pcb)
    {
        APP_PRINT_INFO("Failed to create PCB\n");

        return;
    }

    altcp_arg(client_pcb, client_pcb);
    altcp_recv(client_pcb, client_recv);
    altcp_err(client_pcb, client_error);
    APP_PRINT_INFO("Connecting to server ,port %d\n", SERVER_PORT);
    altcp_connect(client_pcb, (const ip_addr_t *)&loopback_addr, 8443, client_connected);
}
