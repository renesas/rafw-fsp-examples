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
#include "net_sntp_client.h"
#include "mqtt_construct.h"
#include "mqtt_ws_client.h"

#if defined(__SUPPORT_WEBSOCKET_CLIENT__)

#define WS_APP_TAG                      "WS_APP"

/* ---- User Configuration ---- */
#define WS_APP_SERVER_URI               "wss://10.54.153.175:8765"  /* WebSocket server address (secure) */
#define WS_APP_SUBPROTOCOL              "mqtt"                      /* MQTT over WebSocket subprotocol */
#define WS_APP_SEND_INTERVAL_MS         (5000)                      /* Send every 5 seconds */
#define WS_APP_TASK_STACK               (1024)                      /* 1K words = 4KB */
#define WS_APP_TASK_PRIORITY            (6)
#define WS_APP_MAX_RETRIES              (5)

/* ---- Reconnection Configuration ---- */
#define WS_RECONNECT_INITIAL_DELAY_MS  (1000)   /* Initial reconnect delay: 1 second */
#define WS_RECONNECT_MAX_DELAY_MS       (60000)  /* Maximum reconnect delay: 60 seconds */
#define WS_RECONNECT_BACKOFF_MULTIPLIER (2.0)    /* Exponential backoff multiplier */
#define WS_RECONNECT_MAX_ATTEMPTS       (20)     /* Max reconnection attempts before giving up */
#define WS_RECONNECT_RESET_TIME_MS      (300000) /* Reset retry counter after 5 minutes of stable connection */



/* ---- Root CA certificate for server verification (CN=IP of server) ---- */
#define WS_APP_ROOT_CA \
"-----BEGIN CERTIFICATE-----\n" \
"MIIBWzCCAQKgAwIBAgIUQai7wFktzIp869JxLI11+XwqV/4wCgYIKoZIzj0EAwIw\n" \
"FDESMBAGA1UEAwwJTXlNUVRUX0NBMB4XDTI2MDQyODEyNTE1NFoXDTM2MDQyNTEy\n" \
"NTE1NFowFDESMBAGA1UEAwwJTXlNUVRUX0NBMFkwEwYHKoZIzj0CAQYIKoZIzj0D\n" \
"AQcDQgAESqzC8ZkZHYl/wUeHCzLKnWF6iShILwlw05FTUHSDL5MFVaIIUa4Z0hE6\n" \
"pYiE4iyQBPRLRuCu68evbwLy/OWL+KMyMDAwHQYDVR0OBBYEFP8EjwaIzTWCzyae\n" \
"M1TRMsfq9w52MA8GA1UdEwEB/wQFMAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgdpqX\n" \
"2jN5djuohhPRYsYOhLApdJ+KN7KMJj2D7XzMUdsCIFHcmwMk3VSCcaXJWQmK00/X\n" \
"56Cb7r88xVaIATL/RUNC\n" \
"-----END CERTIFICATE-----\n"


static volatile bool g_pingresp_received = false;

/* ---- WebSocket client state ---- */
static websocket_client_handle_t g_ws_client = NULL;
static volatile bool g_ws_connected = false;

/* ---- Reconnection state ---- */
static volatile uint32_t g_reconnect_attempts = 0;
static volatile uint32_t g_reconnect_delay_ms = WS_RECONNECT_INITIAL_DELAY_MS;
static volatile TickType_t g_last_connected_time = 0;
static volatile bool g_reconnect_in_progress = false;

/* ---- MQTT over WS State ---- */
static mqtt_ws_client_handle_t g_mqtt_ws = NULL;
static volatile bool g_mqtt_connected = false;
static volatile bool g_mqtt_subscribed = false;

/* ---- Forward Declarations ---- */
static void ws_app_start(void);

/**
 * Calculate the next reconnect delay with exponential backoff.
 * Returns the delay in milliseconds.
 */
static uint32_t ws_reconnect_calculate_delay(void)
{
    uint32_t delay = g_reconnect_delay_ms;

    /* Apply exponential backoff: delay = delay * multiplier */
    uint32_t new_delay = (uint32_t)((float)delay * WS_RECONNECT_BACKOFF_MULTIPLIER);

    /* Cap at maximum delay */
    if (new_delay > WS_RECONNECT_MAX_DELAY_MS)
    {
        new_delay = WS_RECONNECT_MAX_DELAY_MS;
    }

    g_reconnect_delay_ms = new_delay;
    return delay;
}

/**
 * Reset the reconnection state after successful connection.
 */
static void ws_reconnect_reset(void)
{
    g_reconnect_attempts = 0;
    g_reconnect_delay_ms = WS_RECONNECT_INITIAL_DELAY_MS;
    APP_PRINT_INFO("[%s] Reconnection state reset\n", WS_APP_TAG);
}

/**
 * Attempt to reconnect to the WebSocket server.
 * Returns true if reconnection was successful, false otherwise.
 */
static bool ws_reconnect_attempt(void)
{
    if (g_reconnect_in_progress)
    {
        APP_PRINT_INFO("[%s] Reconnection already in progress, skipping...\n", WS_APP_TAG);
        return false;
    }

    g_reconnect_in_progress = true;
    g_reconnect_attempts++;

    if (g_reconnect_attempts > WS_RECONNECT_MAX_ATTEMPTS)
    {
        APP_PRINT_INFO("[%s] Max reconnection attempts (%lu) reached. Giving up.\n",
                       WS_APP_TAG, (unsigned long)g_reconnect_attempts);
        g_reconnect_in_progress = false;
        return false;
    }

    uint32_t delay = ws_reconnect_calculate_delay();
    APP_PRINT_INFO("[%s] Reconnecting in %lu ms (attempt %lu/%lu)...\n",
                   WS_APP_TAG, (unsigned long)delay,
                   (unsigned long)g_reconnect_attempts,
                   (unsigned long)WS_RECONNECT_MAX_ATTEMPTS);

    /* Clean up old client if exists */
    if (g_ws_client != NULL)
    {
        APP_PRINT_INFO("[%s] Destroying old WebSocket client...\n", WS_APP_TAG);
        websocket_client_destroy(g_ws_client);
        g_ws_client = NULL;
    }

    /* Wait before reconnecting */
    vTaskDelay(pdMS_TO_TICKS(delay));

    /* Re-establish WebSocket connection */
    ws_app_start();

    g_reconnect_in_progress = false;

    return (g_ws_client != NULL);
}

/**
 * Check if reconnection should be attempted based on time since last successful connection.
 * Resets the retry counter if enough time has passed.
 */
static void ws_reconnect_check_reset(void)
{
    if (g_last_connected_time > 0)
    {
        TickType_t current_time = xTaskGetTickCount();
        uint32_t elapsed_ms = (current_time - g_last_connected_time) * portTICK_PERIOD_MS;

        if (elapsed_ms > WS_RECONNECT_RESET_TIME_MS)
        {
            /* Enough time has passed with stable connection, reset the retry counter */
            ws_reconnect_reset();
        }
    }
}

/**
 * MQTT Event Callback - Called from the MQTT-over-WS layer.
 */
static void mqtt_event_callback(mqtt_ws_event_id_t event_id, mqtt_ws_event_data_t *event_data)
{
    switch (event_id)
    {
        case MQTT_WS_EVENT_CONNECTED:
            APP_PRINT_INFO("[%s] MQTT Connected\n", WS_APP_TAG);
            g_mqtt_connected = true;
            break;

        case MQTT_WS_EVENT_DISCONNECTED:
            APP_PRINT_INFO("[%s] MQTT Disconnected\n", WS_APP_TAG);
            g_mqtt_connected = false;
            g_mqtt_subscribed = false;
            break;

        case MQTT_WS_EVENT_DATA:
        {
            char topic[64] = {0};
            char payload[128] = {0};
            memcpy(topic, event_data->topic, (event_data->topic_len < 63) ? event_data->topic_len : 63);
            memcpy(payload, event_data->payload, (event_data->payload_len < 127) ? event_data->payload_len : 127);
            
            APP_PRINT_INFO("[%s] MQTT Data: Topic=%s, Payload=%s\n", WS_APP_TAG, topic, payload);
            break;
        }

        case MQTT_WS_EVENT_PUBACK:
            APP_PRINT_INFO("[%s] MQTT PUBACK received (ID: %d)\n", WS_APP_TAG, event_data->packet_id);
            break;

        case MQTT_WS_EVENT_SUBACK:
            APP_PRINT_INFO("[%s] MQTT SUBACK received\n", WS_APP_TAG);
            g_mqtt_subscribed = true;
            break;

        case MQTT_WS_EVENT_TIMEOUT:
            APP_PRINT_INFO("[%s] MQTT QoS Timeout (ID: %d)\n", WS_APP_TAG, event_data->packet_id);
            break;

        default:
            break;
    }
}

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
            g_last_connected_time = xTaskGetTickCount();
            /* Reset reconnection state on successful connection */
            ws_reconnect_reset();
            break;

        case WEBSOCKET_CLIENT_EVENT_DISCONNECTED:
            APP_PRINT_INFO("[%s] Disconnected from server\n", WS_APP_TAG);
            g_ws_connected = false;
            if (g_mqtt_ws) mqtt_ws_reset(g_mqtt_ws);
            break;

        case WEBSOCKET_CLIENT_EVENT_DATA:
        {
            if (event_data->op_code == 0x02 && event_data->data_len > 0)
            {
                if (g_mqtt_ws)
                {
                    mqtt_ws_process_data(g_mqtt_ws, (const uint8_t *)event_data->data_ptr, event_data->data_len);
                }
            }
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
    ws_cfg.subprotocol            = WS_APP_SUBPROTOCOL;

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

    /* Initialize MQTT over WS helper */
    mqtt_ws_config_t mqtt_cfg = {
        .ws_handle = g_ws_client,
        .client_id = MQTT_CLIENT_ID,
        .callback  = mqtt_event_callback
    };
    g_mqtt_ws = mqtt_ws_client_init(&mqtt_cfg);

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

    /* Start SNTP to sync time for TLS certificate validation */
    if (!is_sntp_sync())
    {
        APP_PRINT_INFO("[%s] Starting SNTP to sync time...\n", WS_APP_TAG);

        /* Bypass DNS entirely by using an IP for the NTP server */
        set_sntp_server((unsigned char *)"216.239.35.0", 0);
        
        start_sntp();

        int sntp_wait = 0;
        while (!is_sntp_sync() && (sntp_wait < 30))
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            sntp_wait++;
            if (sntp_wait % 5 == 0)
            {
                APP_PRINT_INFO("[%s] Waiting for SNTP sync... (%d s)\n", WS_APP_TAG, sntp_wait);
            }
        }

        if (is_sntp_sync())
        {
            APP_PRINT_INFO("[%s] Time successfully synced via SNTP!\n", WS_APP_TAG);
        }
        else
        {
            APP_PRINT_INFO("[%s] WARNING: SNTP sync timed out. TLS connection might fail due to certificate validity.\n", WS_APP_TAG);
        }
    }
    else
    {
        APP_PRINT_INFO("[%s] Time already synced via SNTP.\n", WS_APP_TAG);
    }

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

   // bool mqtt_handshake_done = false;

    /* Periodically send messages */
    while (1)
    {
        if (g_ws_connected)
        {
        	if (!g_mqtt_connected)
        	{
        	    static TickType_t last_connect_attempt = 0;
        	    if ((xTaskGetTickCount() - last_connect_attempt) > pdMS_TO_TICKS(5000))
        	    {
        	        APP_PRINT_INFO("[%s] Sending MQTT CONNECT...\n", WS_APP_TAG);
        	        mqtt_ws_connect(g_mqtt_ws);
        	        last_connect_attempt = xTaskGetTickCount();
        	    }
        	}
        	else
        	{
        	    if (!g_mqtt_subscribed)
        	    {
        	        APP_PRINT_INFO("[%s] Sending MQTT SUBSCRIBE...\n", WS_APP_TAG);
        	        mqtt_ws_subscribe(g_mqtt_ws, MQTT_SUB_TOPIC);
        	    }
        	    else
        	    {
        	        snprintf(msg_buf, sizeof(msg_buf), "MQTT Message #%lu", (unsigned long)msg_count++);
        	        mqtt_ws_publish(g_mqtt_ws, MQTT_PUB_TOPIC, msg_buf, MQTT_QOS_1);
        	    }
        	}

            /* Maintain MQTT state (check timeouts) */
            mqtt_ws_maintain(g_mqtt_ws);
        }
        else
        {
            APP_PRINT_INFO("[%s] Waiting for connection...\n", WS_APP_TAG);

            g_mqtt_connected = false;
            g_mqtt_subscribed = false;

            /* Check if we should reset the retry counter after stable connection */
            ws_reconnect_check_reset();

            /* Attempt automatic reconnection if not connected */
            if (!g_ws_connected && g_ws_client == NULL)
            {
                APP_PRINT_INFO("[%s] Connection lost, attempting automatic reconnection...\n", WS_APP_TAG);
                if (ws_reconnect_attempt())
                {
                    APP_PRINT_INFO("[%s] Reconnection successful!\n", WS_APP_TAG);
                }
                else
                {
                    APP_PRINT_INFO("[%s] Reconnection failed, will retry in next cycle\n", WS_APP_TAG);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WS_APP_SEND_INTERVAL_MS));
    }
}

#endif /* __SUPPORT_WEBSOCKET_CLIENT__ */
