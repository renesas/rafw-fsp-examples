/**
 * @file mqtt_ws_client.c
 * @brief Implementation of encapsulated MQTT-over-WebSocket logic.
 */
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "mqtt_ws_client.h"
#include "mqtt_construct.h"
#include "common_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdlib.h>

#define MAX_PENDING_PACKETS    5
#define PENDING_TIMEOUT_MS     5000

typedef struct {
    uint16_t   packet_id;
    TickType_t timestamp;
    bool       in_use;
} pending_packet_t;

struct mqtt_ws_client {
    websocket_client_handle_t ws_handle;
    const char               *client_id;
    mqtt_ws_callback_t        callback;
    uint16_t                  next_packet_id;
    bool                      connected;
    pending_packet_t          pending_list[MAX_PENDING_PACKETS];
};

static mqtt_ws_client_handle_t g_instance = NULL;

mqtt_ws_client_handle_t mqtt_ws_client_init(mqtt_ws_config_t *config)
{
    if (g_instance == NULL)
    {
        g_instance = (mqtt_ws_client_handle_t)malloc(sizeof(struct mqtt_ws_client));
    }
    
    if (g_instance)
    {
        memset(g_instance, 0, sizeof(struct mqtt_ws_client));
        g_instance->ws_handle = config->ws_handle;
        g_instance->client_id = config->client_id;
        g_instance->callback  = config->callback;
        g_instance->next_packet_id = 1;
    }
    
    return g_instance;
}

/**
 * @brief Get the next packet identifier.
 * 
 * @param[in] handle MQTT-over-WS client handle.
 * 
 * @return 16-bit packet identifier.
 */
static uint16_t get_next_id(mqtt_ws_client_handle_t handle)
{
    uint16_t id = handle->next_packet_id++;
    if (handle->next_packet_id == 0) handle->next_packet_id = 1;
    return id;
}

int mqtt_ws_connect(mqtt_ws_client_handle_t handle)
{
    if (handle == NULL) return -1;
    
    char buf[128];
    int len = mqtt_encode_connect(buf, sizeof(buf), handle->client_id);
    if (len < 0) return -1;
    
    return websocket_client_send_bin(handle->ws_handle, buf, (uint32_t)len, pdMS_TO_TICKS(1000));
}

int mqtt_ws_subscribe(mqtt_ws_client_handle_t handle, const char *topic)
{
    if (handle == NULL || topic == NULL) return -1;
    
    char buf[128];
    uint16_t id = get_next_id(handle);
    int len = mqtt_encode_subscribe(buf, sizeof(buf), topic, id);
    if (len < 0) return -1;
    
    return websocket_client_send_bin(handle->ws_handle, buf, (uint32_t)len, pdMS_TO_TICKS(1000));
}

int mqtt_ws_publish(mqtt_ws_client_handle_t handle, const char *topic, const char *payload, uint8_t qos)
{
    if (handle == NULL || topic == NULL) return -1;
    
    char buf[256];
    uint16_t id = (qos > 0) ? get_next_id(handle) : 0;
    int len = mqtt_encode_publish(buf, sizeof(buf), topic, payload, id, qos);
    if (len < 0) return -1;
    
    int ret = websocket_client_send_bin(handle->ws_handle, buf, (uint32_t)len, pdMS_TO_TICKS(1000));
    
    if (ret > 0 && qos > 0)
    {
        /* Add to pending list for non-blocking tracking */
        for (int i = 0; i < MAX_PENDING_PACKETS; i++)
        {
            if (!handle->pending_list[i].in_use)
            {
                handle->pending_list[i].packet_id = id;
                handle->pending_list[i].timestamp = xTaskGetTickCount();
                handle->pending_list[i].in_use    = true;
                break;
            }
        }
    }
    
    return ret;
}

void mqtt_ws_process_data(mqtt_ws_client_handle_t handle, const uint8_t *data, uint32_t len)
{
    if (handle == NULL || data == NULL || len == 0) return;
    
    uint8_t type = data[0] & 0xF0;
    mqtt_ws_event_data_t evt_data = {0};
    
    switch (type)
    {
        case MQTT_MSG_TYPE_CONNACK:
            handle->connected = true;
            if (handle->callback) handle->callback(MQTT_WS_EVENT_CONNECTED, NULL);
            break;
            
        case MQTT_MSG_TYPE_PUBACK:
        {
            uint16_t id = (uint16_t)((data[2] << 8) | data[3]);
            for (int i = 0; i < MAX_PENDING_PACKETS; i++)
            {
                if (handle->pending_list[i].in_use && handle->pending_list[i].packet_id == id)
                {
                    handle->pending_list[i].in_use = false;
                    evt_data.packet_id = id;
                    if (handle->callback) handle->callback(MQTT_WS_EVENT_PUBACK, &evt_data);
                    break;
                }
            }
            break;
        }
            
        case MQTT_MSG_TYPE_SUBACK:
            if (handle->callback) handle->callback(MQTT_WS_EVENT_SUBACK, NULL);
            break;
            
        case MQTT_MSG_TYPE_PUBLISH:
        {
            /* Basic parsing of incoming publish */
            int rem_len = data[1];
            int topic_len = (data[2] << 8) | data[3];
            evt_data.topic = (const char *)&data[4];
            evt_data.topic_len = (uint32_t)topic_len;
            
            int payload_offset = 4 + topic_len;
            evt_data.payload = (const char *)&data[payload_offset];
            evt_data.payload_len = (uint32_t)(rem_len - 2 - topic_len);
            
            if (handle->callback) handle->callback(MQTT_WS_EVENT_DATA, &evt_data);
            break;
        }
            
        default:
            break;
    }
}

void mqtt_ws_maintain(mqtt_ws_client_handle_t handle)
{
    if (handle == NULL) return;
    
    TickType_t now = xTaskGetTickCount();
    
    /* Check for QoS timeouts */
    for (int i = 0; i < MAX_PENDING_PACKETS; i++)
    {
        if (handle->pending_list[i].in_use)
        {
            if ((now - handle->pending_list[i].timestamp) > pdMS_TO_TICKS(PENDING_TIMEOUT_MS))
            {
                handle->pending_list[i].in_use = false;
                mqtt_ws_event_data_t evt_data = {0};
                evt_data.packet_id = handle->pending_list[i].packet_id;
                if (handle->callback) handle->callback(MQTT_WS_EVENT_TIMEOUT, &evt_data);
            }
        }
    }
}

void mqtt_ws_reset(mqtt_ws_client_handle_t handle)
{
    if (handle == NULL) return;
    
    handle->connected = false;
    memset(handle->pending_list, 0, sizeof(handle->pending_list));
    if (handle->callback) handle->callback(MQTT_WS_EVENT_DISCONNECTED, NULL);
}

/* EOF */
