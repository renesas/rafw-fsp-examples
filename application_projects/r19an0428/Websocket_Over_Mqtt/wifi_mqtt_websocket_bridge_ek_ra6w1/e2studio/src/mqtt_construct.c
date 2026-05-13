/***********************************************************************************************************************
 * File Name    : mqtt_construct.c
 * Description  : Implementation of MQTT packet construction functions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "mqtt_construct.h"

int mqtt_encode_connect(char *buf, int buf_len, const char *client_id)
{
    int client_id_len = (int)strlen(client_id);
    int remaining_len = 10 + 2 + client_id_len;
    
    if (buf_len < remaining_len + 2) return -1;

    int pos = 0;
    buf[pos++] = MQTT_MSG_TYPE_CONNECT;
    buf[pos++] = (char)remaining_len;
    
    /* Protocol Name */
    buf[pos++] = 0x00; buf[pos++] = 0x04;
    buf[pos++] = 'M'; buf[pos++] = 'Q'; buf[pos++] = 'T'; buf[pos++] = 'T';
    
    /* Protocol Level */
    buf[pos++] = 0x04;
    
    /* Connect Flags */
    buf[pos++] = 0x02; /* Clean Session */
    
    /* Keep Alive */
    buf[pos++] = (MQTT_KEEPALIVE >> 8) & 0xFF;
    buf[pos++] = MQTT_KEEPALIVE & 0xFF;
    
    /* Client ID */
    buf[pos++] = (char)((client_id_len >> 8) & 0xFF);
    buf[pos++] = (char)(client_id_len & 0xFF);
    memcpy(&buf[pos], client_id, (size_t)client_id_len);
    pos += client_id_len;

    return pos;
}

int mqtt_encode_publish(char *buf, int buf_len, const char *topic, const char *payload, uint16_t packet_id, uint8_t qos)
{
    int topic_len = (int)strlen(topic);
    int payload_len = (int)strlen(payload);
    
    /* For QoS 0: no packet ID
     * For QoS 1/2: 2-byte packet ID after topic */
    int packet_id_len = (qos > 0) ? 2 : 0;
    int remaining_len = 2 + topic_len + packet_id_len + payload_len;

    if (buf_len < remaining_len + 2) return -1;

    int pos = 0;
    
    /* Fixed header: PUBLISH with QoS bit */
    buf[pos++] = MQTT_MSG_TYPE_PUBLISH | (qos << 1);
    buf[pos++] = (char)remaining_len;
    
    /* Topic Name */
    buf[pos++] = (char)((topic_len >> 8) & 0xFF);
    buf[pos++] = (char)(topic_len & 0xFF);
    memcpy(&buf[pos], topic, (size_t)topic_len);
    pos += topic_len;
    
    /* Packet Identifier (QoS 1/2 only) */
    if (qos > 0)
    {
        buf[pos++] = (char)((packet_id >> 8) & 0xFF);
        buf[pos++] = (char)(packet_id & 0xFF);
    }
    
    /* Payload */
    memcpy(&buf[pos], payload, (size_t)payload_len);
    pos += payload_len;

    return pos;
}

int mqtt_encode_subscribe(char *buf, int buf_len, const char *topic, uint16_t packet_id)
{
    int topic_len = (int)strlen(topic);
    int remaining_len = 2 + 2 + topic_len + 1;

    if (buf_len < remaining_len + 2) return -1;

    int pos = 0;
    buf[pos++] = MQTT_MSG_TYPE_SUBSCRIBE;
    buf[pos++] = (char)remaining_len;
    
    /* Packet Identifier */
    buf[pos++] = (char)((packet_id >> 8) & 0xFF);
    buf[pos++] = (char)(packet_id & 0xFF);
    
    /* Topic Filter */
    buf[pos++] = (char)((topic_len >> 8) & 0xFF);
    buf[pos++] = (char)(topic_len & 0xFF);
    memcpy(&buf[pos], topic, (size_t)topic_len);
    pos += topic_len;
    
    /* Requested QoS */
    buf[pos++] = MQTT_DEFAULT_QOS;

    return pos;
}

int mqtt_encode_pingreq(char *buf, int buf_len)
{
    if (buf_len < 2) return -1;

    buf[0] = MQTT_MSG_TYPE_PINGREQ;
    buf[1] = 0x00;  /* Remaining length = 0 */

    return 2;
}
