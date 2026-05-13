/***********************************************************************************************************************
 * File Name    : mqtt_construct.h
 * Description  : MQTT packet construction and encoding functions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef MQTT_CONSTRUCT_H
#define MQTT_CONSTRUCT_H

#include <stdint.h>
#include <string.h>

/* MQTT Fixed Header Types */
#define MQTT_MSG_TYPE_CONNECT     0x10
#define MQTT_MSG_TYPE_CONNACK     0x20
#define MQTT_MSG_TYPE_PUBLISH     0x30
#define MQTT_MSG_TYPE_PUBACK      0x40
#define MQTT_MSG_TYPE_SUBSCRIBE   0x82
#define MQTT_MSG_TYPE_SUBACK      0x90
#define MQTT_MSG_TYPE_PINGREQ     0xC0
#define MQTT_MSG_TYPE_PINGRESP    0xD0

/* QoS Levels */
#define MQTT_QOS_0                0x00
#define MQTT_QOS_1                0x01
#define MQTT_QOS_2                0x02

/* Configuration Constants */
#define MQTT_CLIENT_ID            "RA6W1_WS"
#define MQTT_PUB_TOPIC            "rrq61x_pub"
#define MQTT_SUB_TOPIC            "rrq61x_sub"
#define MQTT_KEEPALIVE            60
#define MQTT_DEFAULT_QOS          MQTT_QOS_1

/**
 * Encode an MQTT CONNECT packet (v3.1.1).
 * Returns the length of the packet, or -1 on error.
 */
int mqtt_encode_connect(char *buf, int buf_len, const char *client_id);

/**
 * Encode an MQTT PUBLISH packet (v3.1.1).
 * @param qos: QoS level (0, 1, or 2)
 * @param packet_id: Packet identifier (only used for QoS 1/2)
 * Returns the length of the packet, or -1 on error.
 */
int mqtt_encode_publish(char *buf, int buf_len, const char *topic, const char *payload, uint16_t packet_id, uint8_t qos);

/**
 * Encode an MQTT SUBSCRIBE packet (v3.1.1).
 * @param packet_id: Packet identifier for the subscription
 * Returns the length of the packet, or -1 on error.
 */
int mqtt_encode_subscribe(char *buf, int buf_len, const char *topic, uint16_t packet_id);

/**
 * Encode an MQTT PINGREQ packet (v3.1.1).
 * Returns the length of the packet, or -1 on error.
 */
int mqtt_encode_pingreq(char *buf, int buf_len);

#endif /* MQTT_CONSTRUCT_H */
