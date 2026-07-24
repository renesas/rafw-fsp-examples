/***********************************************************************************************************************
 * File Name    : mqtt_ws_client.h
 * Description  : Encapsulated MQTT logic for use over WebSockets.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef MQTT_WS_CLIENT_H
#define MQTT_WS_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "websocket_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT Event identifiers.
 */
typedef enum {
    MQTT_WS_EVENT_CONNECTED,    /*!< Connection established and CONNACK received */
    MQTT_WS_EVENT_DISCONNECTED, /*!< Connection lost */
    MQTT_WS_EVENT_DATA,         /*!< Incoming PUBLISH data received */
    MQTT_WS_EVENT_PUBACK,       /*!< PUBACK received for a QoS 1 publish */
    MQTT_WS_EVENT_SUBACK,       /*!< SUBACK received for a subscription */
    MQTT_WS_EVENT_TIMEOUT       /*!< Timeout occurred (e.g. QoS 1 ack timeout) */
} mqtt_ws_event_id_t;

/**
 * @brief Structure containing event data.
 */
typedef struct {
    const char *topic;          /*!< Topic string (if applicable) */
    uint32_t    topic_len;      /*!< Length of topic string */
    const char *payload;        /*!< Pointer to payload data */
    uint32_t    payload_len;    /*!< Length of payload data */
    uint16_t    packet_id;      /*!< Packet identifier (for SUBACK/PUBACK/TIMEOUT) */
} mqtt_ws_event_data_t;

/**
 * @brief MQTT-over-WebSocket event callback type.
 * 
 * @param[in] event_id   Identifier of the event.
 * @param[in] event_data Pointer to event-specific data (may be NULL).
 */
typedef void (*mqtt_ws_callback_t)(mqtt_ws_event_id_t event_id, mqtt_ws_event_data_t *event_data);

/**
 * @brief MQTT-over-WebSocket client configuration structure.
 */
typedef struct {
    websocket_client_handle_t ws_handle; /*!< Valid WebSocket client handle */
    const char               *client_id; /*!< MQTT Client ID string */
    mqtt_ws_callback_t        callback;  /*!< User callback function */
} mqtt_ws_config_t;

/**
 * @brief MQTT-over-WebSocket client handle.
 */
typedef struct mqtt_ws_client *mqtt_ws_client_handle_t;

/**
 * @brief Initialize the MQTT-over-WS client.
 * 
 * @param[in] config Pointer to the configuration structure.
 * 
 * @return 
 *     - Handle to the MQTT client instance.
 *     - NULL if initialization fails.
 */
mqtt_ws_client_handle_t mqtt_ws_client_init(mqtt_ws_config_t *config);

/**
 * @brief Send MQTT CONNECT packet.
 * 
 * @param[in] handle MQTT-over-WS client handle.
 * 
 * @return
 *     - 0 on success.
 *     - (-1) on failure.
 */
int mqtt_ws_connect(mqtt_ws_client_handle_t handle);

/**
 * @brief Send MQTT SUBSCRIBE packet.
 * 
 * @param[in] handle MQTT-over-WS client handle.
 * @param[in] topic  Topic to subscribe to.
 * 
 * @return
 *     - 0 on success.
 *     - (-1) on failure.
 */
int mqtt_ws_subscribe(mqtt_ws_client_handle_t handle, const char *topic);

/**
 * @brief Send MQTT PUBLISH packet (QoS 0 or 1).
 * 
 * @param[in] handle  MQTT-over-WS client handle.
 * @param[in] topic   Topic to publish to.
 * @param[in] payload Payload string.
 * @param[in] qos     Quality of Service level (0 or 1).
 * 
 * @return
 *     - 0 on success.
 *     - (-1) on failure.
 */
int mqtt_ws_publish(mqtt_ws_client_handle_t handle, const char *topic, const char *payload, uint8_t qos);

/**
 * @brief Process incoming data from WebSocket.
 *        Call this from the WebSocket DATA event handler.
 * 
 * @param[in] handle MQTT-over-WS client handle.
 * @param[in] data   Pointer to the received binary data.
 * @param[in] len    Length of the received data.
 */
void mqtt_ws_process_data(mqtt_ws_client_handle_t handle, const uint8_t *data, uint32_t len);

/**
 * @brief Perform periodic maintenance (Check keep-alive and QoS timeouts).
 *        Call this periodically from your application task.
 * 
 * @param[in] handle MQTT-over-WS client handle.
 */
void mqtt_ws_maintain(mqtt_ws_client_handle_t handle);

/**
 * @brief Reset MQTT state (e.g. on WS disconnect).
 * 
 * @param[in] handle MQTT-over-WS client handle.
 */
void mqtt_ws_reset(mqtt_ws_client_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_WS_CLIENT_H */

/* EOF */
