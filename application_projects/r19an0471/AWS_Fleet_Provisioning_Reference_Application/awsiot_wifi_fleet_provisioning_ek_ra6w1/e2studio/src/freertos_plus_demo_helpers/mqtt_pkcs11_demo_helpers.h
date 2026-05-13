/*
 * FreeRTOS V202411.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef MQTT_PKCS11_DEMO_HELPERS_H
#define MQTT_PKCS11_DEMO_HELPERS_H

/* MQTT API header. */
#include "core_mqtt.h"

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"

/* Transport interface implementation include header for TLS. */
#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
#include "include/using_mbedtls_pkcs11.h"
#else//awsupgradeport[[::
#include "transport_mbedtls_pkcs11.h"
#endif//]]

/**
 * @brief Establish a MQTT connection.
 *
 * @param[in, out] pxMqttContext The memory for the MQTTContext_t that will be used for the
 * MQTT connection.
 * @param[out] pxNetworkContext The memory for the NetworkContext_t required for the
 * MQTT connection.
 * @param[in] pxNetworkBuffer The buffer space for initializing the @p pxMqttContext MQTT
 * context used in the MQTT connection.
 * @param[in] eventCallback The callback function used to receive incoming
 * publishes and incoming acks from MQTT library.
 * @param[in] pcClientCertLabel The client certificate PKCS #11 label to use.
 * @param[in] pcPrivateKeyLabel The private key PKCS #11 label for the client certificate.
 *
 * @return The status of the final connection attempt.
 */
#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
BaseType_t xEstablishMqttSession(MQTTContext_t * pxMqttContext,
                                 NetworkContext_t * pxNetworkContext,
                                 MQTTFixedBuffer_t * pxNetworkBuffer,
                                 MQTTEventCallback_t eventCallback,
                                 char * pcClientCertLabel,
                                 char * pcPrivateKeyLabel);
#else //awsupgradeport[[::
BaseType_t xEstablishMqttSession_P11(MQTTContext_t * pxMqttContext,
                                     NetworkContext_t * pxNetworkContext,
                                     MQTTFixedBuffer_t * pxNetworkBuffer,
                                     MQTTEventCallback_t eventCallback,
                                     char * pcClientCertLabel,
                                     char * pcPrivateKeyLabel);
BaseType_t xEstablishMqttSession(MQTTContext_t * pxMqttContext,
                                 NetworkContext_t * pxNetworkContext,
                                 MQTTFixedBuffer_t * pxNetworkBuffer,
                                 MQTTEventCallback_t eventCallback);
#endif //]]

/**
 * @brief Handle the incoming packet if it's not related to the device shadow.
 *
 * @param[in] pxPacketInfo Packet Info pointer for the incoming packet.
 * @param[in] usPacketIdentifier Packet identifier of the incoming packet.
 */
void vHandleOtherIncomingPacket(MQTTPacketInfo_t * pxPacketInfo,
                                uint16_t usPacketIdentifier);

/**
 * @brief Close the MQTT connection.
 *
 * @param[in, out] pxMqttContext The MQTT context for the MQTT connection to close.
 * @param[in, out] pxNetworkContext The network context for the TLS session to
 * terminate.
 *
 * @return pdPASS if DISCONNECT was successfully sent;
 * pdFAIL otherwise.
 */
BaseType_t xDisconnectMqttSession(MQTTContext_t * pxMqttContext,
                                  NetworkContext_t * pxNetworkContext);

/**
 * @brief Subscribe to a MQTT topic filter.
 *
 * @param[in] pxMqttContext The MQTT context for the MQTT connection.
 * @param[in] pcTopicFilter Pointer to the shadow topic buffer.
 * @param[in] usTopicFilterLength Indicates the length of the shadow
 * topic buffer.
 *
 * @return pdPASS if SUBSCRIBE was successfully sent;
 * pdFAIL otherwise.
 */
BaseType_t xSubscribeToTopic(MQTTContext_t * pxMqttContext,
                             const char * pcTopicFilter,
                             uint16_t usTopicFilterLength);

/**
 * @brief Sends an MQTT UNSUBSCRIBE to unsubscribe from the shadow
 * topic.
 *
 * @param[in] pxMqttContext The MQTT context for the MQTT connection.
 * @param[in] pcTopicFilter Pointer to the MQTT topic filter.
 * @param[in] usTopicFilterLength Indicates the length of the topic filter.
 *
 * @return pdPASS if UNSUBSCRIBE was successfully sent;
 * pdFAIL otherwise.
 */
BaseType_t xUnsubscribeFromTopic(MQTTContext_t * pxMqttContext,
                                 const char * pcTopicFilter,
                                 uint16_t usTopicFilterLength);

/**
 * @brief Publish a message to a MQTT topic.
 *
 * @param[in] pxMqttContext The MQTT context for the MQTT connection.
 * @param[in] pcTopicFilter Points to the topic.
 * @param[in] topicFilterLength The length of the topic.
 * @param[in] pcPayload Points to the payload.
 * @param[in] payloadLength The length of the payload.
 *
 * @return pdPASS if PUBLISH was successfully sent;
 * pdFAIL otherwise.
 */
BaseType_t xPublishToTopic(MQTTContext_t * pxMqttContext,
                           const char * pcTopicFilter,
                           int32_t topicFilterLength,
                           const char * pcPayload,
                           size_t payloadLength);

/**
 * @brief Invoke the core MQTT library's process loop function.
 *
 * @param[in] pxMqttContext The MQTT context for the MQTT connection.
 * @param[in] ulTimeoutMs Minimum time for the loop to run, if no error occurs.
 *
 * @return pdPASS if process loop was successful;
 * pdFAIL otherwise.
 */
BaseType_t xProcessLoop(MQTTContext_t * pxMqttContext,
                        uint32_t ulTimeoutMs);

/**
 * @brief Call #MQTT_ProcessLoop in a loop for the duration of a timeout or
 * #MQTT_ProcessLoop returns a failure.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] ulTimeoutMs Duration to call #MQTT_ProcessLoop for.
 *
 * @return Returns the return value of the last call to #MQTT_ProcessLoop.
 */
MQTTStatus_t prvProcessLoopWithTimeout(MQTTContext_t * pMqttContext,
                                       uint32_t ulTimeoutMs);

#endif /* ifndef MQTT_PKCS11_DEMO_HELPERS_H */
