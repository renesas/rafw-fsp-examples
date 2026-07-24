/***********************************************************************************************************************
* File Name    : websocket_client_app.c
* Description  : websocket threads and functions
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "websocket_client.h"
#include "net_network_main.h"
#include "iface_defs.h"
#include "common_utils.h"

#if defined(__SUPPORT_WEBSOCKET_CLIENT__)

#define WS_APP_TAG                      "WS_APP"

/* ---- User Configuration ---- */
#define WS_APP_SERVER_URI               "wss://<ip address>:8765"  /* WebSocket server address (secure), Ex:"wss://192.168.50.111:8765" */
#define WS_APP_SEND_INTERVAL_MS         (5000)                      /* Send every 5 seconds */
#define WS_APP_TASK_STACK               (1024)                      /* 1K words = 4KB */
#define WS_APP_TASK_PRIORITY            (6)
#define WS_APP_MAX_RETRIES              (5)

/* ---- Root CA certificate for server verification (CN=IP of server) ---- */
/* ---- Below certificate can vary according to your ca certificate ---- */
#define WS_APP_ROOT_CA \
"-----BEGIN CERTIFICATE-----\n" \
"MIIBejCCASGgAwIBAgIUdVAoEFU7PPYkFqVatXfVF8Q1ZgkwCgYIKoZIzj0EAwIw\n" \
"EzERMA8GA1UEAwwITXlSb290Q0EwHhcNMjYwNjIzMDM0OTU5WhcNMzYwNjIwMDM0\n" \
"OTU5WjATMREwDwYDVQQDDAhNeVJvb3RDQTBZMBMGByqGSM49AgEGCCqGSM49AwEH\n" \
"A0IABFu+WVBM6ZnilTzapG3mblwYQl5GxM159wUj6bdW9jpHim83IN0S/97a+TzS\n" \
"jjkvcdux9qe9W4asGRGqT7XvuVijUzBRMB0GA1UdDgQWBBTeuIJq6xjVANRh61ua\n" \
"Fc9Iymk1jzAfBgNVHSMEGDAWgBTeuIJq6xjVANRh61uaFc9Iymk1jzAPBgNVHRMB\n" \
"Af8EBTADAQH/MAoGCCqGSM49BAMCA0cAMEQCIBgG8Nb0JsdqdmMkZ0p2328VrsCa\n" \
"FT+OwzKcBDV6dWx2AiB5/eul6mcj6fLdMT/foPqpklZJ+RRAl9aQ2V9xWYBNDw==\n" \
"-----END CERTIFICATE-----\n"

/* ---- Internal state ---- */
static websocket_client_handle_t g_ws_client = NULL;
static volatile bool g_ws_connected = false;

/**
 * WebSocket event callback — called from the WebSocket internal task.
 */
static void ws_app_event_handler(websocket_client_event_id_t event_id,
                                 websocket_client_event_data_t *event_data)
{
    switch (event_id)
    {
        case WEBSOCKET_CLIENT_EVENT_CONNECTED:
            APP_PRINT_INFO("[%s] Connected to server\n", WS_APP_TAG);
            g_ws_connected = true;
            break;

        case WEBSOCKET_CLIENT_EVENT_DISCONNECTED:
            APP_PRINT_INFO("[%s] Disconnected from server\n", WS_APP_TAG);
            g_ws_connected = false;
            break;

        case WEBSOCKET_CLIENT_EVENT_DATA:
        {
            APP_PRINT_INFO("[%s] Received %d bytes (opcode=0x%02x): ", WS_APP_TAG,
                   event_data->data_len, event_data->op_code);
            /* Print data if it looks like text */
            if (event_data->data_ptr && event_data->data_len > 0)
            {
                char tmp[128];
                int len = (event_data->data_len < (int)sizeof(tmp) - 1) ? event_data->data_len : (int)sizeof(tmp) - 1;
                memcpy(tmp, event_data->data_ptr, len);
                tmp[len] = '\0';
                APP_PRINT_INFO("%s", tmp);
            }
            APP_PRINT_INFO("\n");
            break;
        }

        case WEBSOCKET_CLIENT_EVENT_ERROR:
            APP_PRINT_INFO("[%s] Error occurred\n", WS_APP_TAG);
            g_ws_connected = false;
            break;

        case WEBSOCKET_CLIENT_EVENT_FAIL_TO_CONNECT:
            APP_PRINT_INFO("[%s] Failed to connect to server\n", WS_APP_TAG);
            g_ws_connected = false;
            break;

        case WEBSOCKET_CLIENT_EVENT_CLOSED:
            APP_PRINT_INFO("[%s] Connection closed cleanly\n", WS_APP_TAG);
            g_ws_connected = false;
            break;

        default:
            break;
    }
}

/**
 * Start the WebSocket client.
 * Call this after WiFi is connected (after WIFI_On()).
 */
static void ws_app_start(void)
{
    APP_PRINT_INFO("[%s] Starting WebSocket client app...\n", WS_APP_TAG);

    /* Configure */
    websocket_client_config_t ws_cfg;
    memset(&ws_cfg, 0, sizeof(ws_cfg));

    ws_cfg.uri                    = WS_APP_SERVER_URI;
    ws_cfg.task_prio              = WS_APP_TASK_PRIORITY;
    ws_cfg.task_stack             = WS_APP_TASK_STACK;
    ws_cfg.buffer_size            = WEBSOCKET_BUFFER_SIZE_BYTE;  /* 4KB */
    ws_cfg.disable_auto_reconnect = false;                       /* auto-reconnect enabled */
    ws_cfg.ping_interval_sec      = WEBSOCKET_PING_INTERVAL_SEC; /* 10s */
    ws_cfg.pingpong_timeout_sec   = WEBSOCKET_PINGPONG_TIMEOUT_SEC; /* 120s */
    ws_cfg.keep_alive_enable      = true;
    ws_cfg.keep_alive_idle        = WEBSOCKET_KEEP_ALIVE_IDLE;
    ws_cfg.keep_alive_interval    = WEBSOCKET_KEEP_ALIVE_INTERVAL;
    ws_cfg.keep_alive_count       = WEBSOCKET_KEEP_ALIVE_COUNT;
    ws_cfg.transport              = WEBSOCKET_CLIENT_TRANSPORT_OVER_SSL;
    ws_cfg.cert_pem               = WS_APP_ROOT_CA;

    /* Initialize */
    g_ws_client = websocket_client_init(&ws_cfg);
    if (g_ws_client == NULL)
    {
        APP_PRINT_INFO("[%s] ERROR: websocket_client_init() failed\n", WS_APP_TAG);
        return;
    }

    /* Start (connects and spawns internal task) */
    ws_err_t err = websocket_client_start(g_ws_client, ws_app_event_handler);
    if (err != WS_OK)
    {
        APP_PRINT_INFO("[%s] ERROR: websocket_client_start() failed (%d)\n", WS_APP_TAG, err);
        /* Note: websocket_client_start destroys the client on failure */
        g_ws_client = NULL;
        return;
    }

    APP_PRINT_INFO("[%s] WebSocket client started, connecting to %s\n", WS_APP_TAG, WS_APP_SERVER_URI);
}

/**
 * Send a text message to the server.
 * Returns the number of bytes sent, or -1 on error.
 */
static int ws_app_send_text(const char *message)
{
    if (g_ws_client == NULL || !g_ws_connected)
    {
        APP_PRINT_INFO("[%s] Cannot send: not connected\n", WS_APP_TAG);
        return -1;
    }

    int len = (int)strlen(message);
    int ret = websocket_client_send_text(g_ws_client, message, len, pdMS_TO_TICKS(5000));
    if (ret < 0)
    {
        APP_PRINT_INFO("[%s] Send failed\n", WS_APP_TAG);
    }
    else
    {
        APP_PRINT_INFO("[%s] Sent %d bytes: %s\n", WS_APP_TAG, ret, message);
    }
    return ret;
}

/**
 * Demo task: connects and sends a message every WS_APP_SEND_INTERVAL_MS.
 * Create this task from new_thread0_entry after WIFI_On().
 */
void ws_app_demo_task(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    uint32_t msg_count = 0;
    char msg_buf[64];

    /* Wait until WiFi network is fully connected with an IP address */
    APP_PRINT_INFO("[%s] Waiting for WiFi network to be ready...\n", WS_APP_TAG);
    while (!ra6w1_network_main_check_network_ready(WLAN0_IFACE))
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    /* Extra delay to ensure DHCP and routing are fully established */
    APP_PRINT_INFO("[%s] WiFi interface ready, waiting for network to stabilize...\n", WS_APP_TAG);
    vTaskDelay(pdMS_TO_TICKS(5000));
    APP_PRINT_INFO("[%s] WiFi network is ready!\n", WS_APP_TAG);

    /* Start the WebSocket connection, retry if it fails */
    for (int retry = 0; retry < WS_APP_MAX_RETRIES; retry++)
    {
        ws_app_start(); /* start websocket client */
        if (g_ws_client != NULL)
        {
            break;  /* init+start succeeded */
        }
        APP_PRINT_INFO("[%s] Retry %d/%d in 5 seconds...\n", WS_APP_TAG, retry + 1, WS_APP_MAX_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    if (g_ws_client == NULL)
    {
        APP_PRINT_INFO("[%s] ERROR: Failed to start after %d retries. Task exiting.\n", WS_APP_TAG, WS_APP_MAX_RETRIES);
        vTaskDelete(NULL);
        return;
    }

    /* Wait a bit for the connection to establish */
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Periodically send messages */
    while (1)
    {
        if (g_ws_connected)
        {
            snprintf(msg_buf, sizeof(msg_buf), "Hello from RA6W2 #%lu", (unsigned long)msg_count++);
            ws_app_send_text(msg_buf);
        }
        else
        {
            APP_PRINT_INFO("[%s] Waiting for connection...\n", WS_APP_TAG);
        }

        vTaskDelay(pdMS_TO_TICKS(WS_APP_SEND_INTERVAL_MS));
    }
}

#endif /* __SUPPORT_WEBSOCKET_CLIENT__ */
