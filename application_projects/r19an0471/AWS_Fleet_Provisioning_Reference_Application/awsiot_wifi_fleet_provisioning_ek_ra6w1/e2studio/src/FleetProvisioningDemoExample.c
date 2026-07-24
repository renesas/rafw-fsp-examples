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

/*
 * Demo for showing use of the Fleet Provisioning library to use the Fleet
 * Provisioning feature of AWS IoT Core for provisioning devices with
 * credentials. This demo shows how a device can be provisioned with AWS IoT
 * Core using the Certificate Signing Request workflow of the Fleet
 * Provisioning feature.
 *
 * The Fleet Provisioning library provides macros and helper functions for
 * assembling MQTT topics strings, and for determining whether an incoming MQTT
 * message is related to the Fleet Provisioning API of AWS IoT Core. The Fleet
 * Provisioning library does not depend on any particular MQTT library,
 * therefore the functionality for MQTT operations is placed in another file
 * (mqtt_operations.c). This demo uses the coreMQTT library. If needed,
 * mqtt_operations.c can be modified to replace coreMQTT with another MQTT
 * library. This demo requires using the AWS IoT Core broker as Fleet
 * Provisioning is an AWS IoT Core feature.
 *
 * This demo provisions a device certificate using the provisioning by claim
 * workflow with a Certificate Signing Request (CSR). The demo connects to AWS
 * IoT Core using provided claim credentials (whose certificate needs to be
 * registered with IoT Core before running this demo), subscribes to the
 * CreateCertificateFromCsr topics, and obtains a certificate. It then
 * subscribes to the RegisterThing topics and activates the certificate and
 * obtains a Thing using the provisioning template. Finally, it reconnects to
 * AWS IoT Core using the new credentials.
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* mbedTLS include for configuring threading functions */
#include "mbedtls/threading.h"
#include "threading_alt.h"

/* TinyCBOR library for CBOR encoding and decoding operations. */
#include "cbor.h"

/* corePKCS11 includes. */
#include "core_pkcs11.h"
#include "core_pkcs11_config.h"

/* AWS IoT Fleet Provisioning Library. */
#include "fleet_provisioning.h"

/* Demo includes. */
#include "mqtt_pkcs11_demo_helpers.h"
#include "pkcs11_operations.h"
#include "tinycbor_serializer.h"
#include "core_pkcs11_config_defaults.h"
#include "rm_wifi.h"
#include "defs.h"
#include "app_sample_manager.h"
#include "ra6w1_platform_nvparam.h"
/* Demo Config */
#include "demo_config.h"
#include "app_awsiot_user_config.h"

/**
 * These configurations are required. Throw compilation error if it is not
 * defined.
 */
#ifndef democonfigPROVISIONING_TEMPLATE_NAME
    #error "Please define democonfigPROVISIONING_TEMPLATE_NAME to the template name registered with AWS IoT Core in demo_config.h."
#endif

#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(democonfigROOT_CA_PEM) in demo_config.h."
#endif
#endif

/**
 * @brief The length of #democonfigPROVISIONING_TEMPLATE_NAME.
 */
#define fpdemoPROVISIONING_TEMPLATE_NAME_LENGTH    ( ( uint16_t ) ( sizeof( democonfigPROVISIONING_TEMPLATE_NAME ) - 1 ) )

/**
 * @brief The length of #democonfigFP_DEMO_ID.
 */
#define fpdemoFP_DEMO_ID_LENGTH                    ( ( uint16_t ) ( sizeof( democonfigFP_DEMO_ID ) - 1 ) )

/**
 * @brief Size of AWS IoT Thing name buffer.
 *
 * See https://docs.aws.amazon.com/iot/latest/apireference/API_CreateThing.html#iot-CreateThing-request-thingName
 */
#define fpdemoMAX_THING_NAME_LENGTH                128

/**
 * @brief The maximum number of times to run the loop in this demo.
 *
 * @note The demo loop is attempted to re-run only if it fails in an iteration.
 * Once the demo loop succeeds in an iteration, the demo exits successfully.
 */
#ifndef fpdemoMAX_DEMO_LOOP_COUNT
    #define fpdemoMAX_DEMO_LOOP_COUNT    ( 3 )
#endif

/**
 * @brief Time in seconds to wait between retries of the demo loop if
 * demo loop fails.
 */
#define fpdemoDELAY_BETWEEN_DEMO_RETRY_ITERATIONS_SECONDS    ( 10 )

/**
 * @brief Size of buffer in which to hold the certificate signing request (CSR).
 */
#define fpdemoCSR_BUFFER_LENGTH                              4096

/**
 * @brief Size of buffer in which to hold the certificate.
 */
#define fpdemoCERT_BUFFER_LENGTH                             4096

/**
 * @brief Size of buffer in which to hold the certificate id.
 *
 * @note Has a maximum length of 64 for more information see the following link
 * https://docs.aws.amazon.com/iot/latest/apireference/API_Certificate.html#iot-Type-Certificate-certificateId
 */
#define fpdemoCERT_ID_BUFFER_LENGTH                          64

/**
 * @brief Size of buffer in which to hold the certificate ownership token.
 */
#define fpdemoOWNERSHIP_TOKEN_BUFFER_LENGTH                  1024

/**
 * @brief Milliseconds per second.
 */
#define fpdemoMILLISECONDS_PER_SECOND                        ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define fpdemoMILLISECONDS_PER_TICK                          ( fpdemoMILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

#define AWSIOT_FP_W_TASK_SIZE                                4096*3
#define FP_SHADOW_TOPIC_PREFIX                               "$aws/things/"
#define FP_SHADOW_TOPIC_SUFFIX                               "/shadow/update/delta"
#define FP_SHADOW_TOPIC                                      "$aws/things/FP_Example_demo_" democonfigFP_DEMO_ID "/shadow/update/delta"
#define FP_SHADOW_TOPIC_LEN                                  ((uint16_t)(sizeof(FP_SHADOW_TOPIC) - 1U))
#define SHADOW_MESSAGE                                       "Initial_Connection_Message"
#define SHADOW_MESSAGE_LEN                                   ((uint16_t)(sizeof(SHADOW_MESSAGE) - 1U))
#define CONN_MESSAGE                                         "Connected_With_Provisioned_Credentials_After_Reboot"
#define CONN_MESSAGE_LEN                                     ((uint16_t)(sizeof(CONN_MESSAGE) - 1U))

bool mqtt_packet_type_publish_reveived = false;
/**
 * @brief Status values of the Fleet Provisioning response.
 */
typedef enum
{
    ResponseNotReceived,
    ResponseAccepted,
    ResponseRejected
} ResponseStatus_t;

/**
 * @brief Each compilation unit that consumes the NetworkContext must define it.
 * It should contain a single pointer to the type of your desired transport.
 * When using multiple transports in the same compilation unit, define this pointer as void *.
 *
 * @note Transport stacks are defined in FreeRTOS-Plus/Source/Application-Protocols/network_transport.
 */
struct NetworkContext
{
    TlsTransportParams_t * pxParams;
};

/*-----------------------------------------------------------*/
/**
 * @brief Status reported from the MQTT publish callback.
 */
static ResponseStatus_t xResponseStatus;

/**
 * @brief Buffer to hold the provisioned AWS IoT Thing name.
 */
static char pcThingName[ fpdemoMAX_THING_NAME_LENGTH ];

/**
 * @brief Length of the AWS IoT Thing name.
 */
static size_t xThingNameLength;

/**
 * @brief Buffer to hold responses received from the AWS IoT Fleet Provisioning
 * APIs. When the MQTT publish callback receives an expected Fleet Provisioning
 * accepted payload, it copies it into this buffer.
 */
static uint8_t pucPayloadBuffer[ democonfigNETWORK_BUFFER_SIZE * 2 ];

/**
 * @brief Length of the payload stored in #pucPayloadBuffer. This is set by the
 * MQTT publish callback when it copies a received payload into #pucPayloadBuffer.
 */
static size_t xPayloadLength;

/**
 * @brief The MQTT context used for MQTT operation.
 */
static MQTTContext_t xMqttContext;

/**
 * @brief The network context used for mbedTLS operation.
 */
static NetworkContext_t xNetworkContext;

/**
 * @brief The parameters for the network context using mbedTLS operation.
 */
static TlsTransportParams_t xTlsTransportParams;

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static MQTTFixedBuffer_t xBuffer =
{
    ucSharedBuffer,
    democonfigNETWORK_BUFFER_SIZE
};

/*-----------------------------------------------------------*/
/**
 * @brief Callback to receive the incoming publish messages from the MQTT
 * broker. Sets xResponseStatus if an expected CreateCertificateFromCsr or
 * RegisterThing response is received, and copies the response into
 * responseBuffer if the response is an accepted one.
 *
 * @param[in] pPublishInfo Pointer to publish info of the incoming publish.
 * @param[in] usPacketIdentifier Packet identifier of the incoming publish.
 */
static void prvProvisioningPublishCallback( MQTTContext_t * pxMqttContext,
                                            MQTTPacketInfo_t * pxPacketInfo,
                                            MQTTDeserializedInfo_t * pxDeserializedInfo );

/**
 * @brief Subscribe to the CreateCertificateFromCsr accepted and rejected topics.
 */
static bool prvSubscribeToCsrResponseTopics( void );

/**
 * @brief Unsubscribe from the CreateCertificateFromCsr accepted and rejected topics.
 */
static bool prvUnsubscribeFromCsrResponseTopics( void );

/**
 * @brief Subscribe to the RegisterThing accepted and rejected topics.
 */
static bool prvSubscribeToRegisterThingResponseTopics( void );

/**
 * @brief Unsubscribe from the RegisterThing accepted and rejected topics.
 */
static bool prvUnsubscribeFromRegisterThingResponseTopics( void );

/*-----------------------------------------------------------*/

BaseType_t xPlatformIsNetworkUp( void );

/*-----------------------------------------------------------*/

static void prvProvisioningPublishCallback( MQTTContext_t * pxMqttContext,
                                            MQTTPacketInfo_t * pxPacketInfo,
                                            MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    FleetProvisioningStatus_t xStatus;
    FleetProvisioningTopic_t xApi;
    MQTTPublishInfo_t * pxPublishInfo;

    configASSERT( pxMqttContext != NULL );
    configASSERT( pxPacketInfo != NULL );
    configASSERT( pxDeserializedInfo != NULL );

    /* Suppress the unused parameter warning when asserts are disabled in
     * build. */
    ( void ) pxMqttContext;

    /* Handle an incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        configASSERT( pxDeserializedInfo->pPublishInfo != NULL );
        pxPublishInfo = pxDeserializedInfo->pPublishInfo;

        LogInfo(("MQTT_PACKET_TYPE_PUBLISH Received"));
        mqtt_packet_type_publish_reveived = true;
        xStatus = FleetProvisioning_MatchTopic( pxPublishInfo->pTopicName,
                                                pxPublishInfo->topicNameLength,
                                                &xApi );

        if( xStatus != FleetProvisioningSuccess )
        {
            LogWarn( ( "Unexpected publish message received. Topic: %.*s.",
                       ( int ) pxPublishInfo->topicNameLength,
                       ( const char * ) pxPublishInfo->pTopicName ) );
        }
        else
        {
            if( xApi == FleetProvCborCreateCertFromCsrAccepted )
            {
                LogInfo( ( "Received accepted response from Fleet Provisioning CreateCertificateFromCsr API." ) );
                xResponseStatus = ResponseAccepted;

                /* Copy the payload from the MQTT library's buffer to #pucPayloadBuffer. */
                ( void ) memcpy( ( void * ) pucPayloadBuffer,
                                 ( const void * ) pxPublishInfo->pPayload,
                                 ( size_t ) pxPublishInfo->payloadLength );

                xPayloadLength = pxPublishInfo->payloadLength;
            }
            else if( xApi == FleetProvCborCreateCertFromCsrRejected )
            {
                LogError( ( "Received rejected response from Fleet Provisioning CreateCertificateFromCsr API." ) );
                xResponseStatus = ResponseRejected;
            }
            else if( xApi == FleetProvCborRegisterThingAccepted )
            {
                LogInfo( ( "Received accepted response from Fleet Provisioning RegisterThing API." ) );
                xResponseStatus = ResponseAccepted;

                /* Copy the payload from the MQTT library's buffer to #pucPayloadBuffer. */
                ( void ) memcpy( ( void * ) pucPayloadBuffer,
                                 ( const void * ) pxPublishInfo->pPayload,
                                 ( size_t ) pxPublishInfo->payloadLength );

                xPayloadLength = pxPublishInfo->payloadLength;
            }
            else if( xApi == FleetProvCborRegisterThingRejected )
            {
                LogError( ( "Received rejected response from Fleet Provisioning RegisterThing API." ) );
                xResponseStatus = ResponseRejected;
            }
            else
            {
                LogError( ( "Received message on unexpected Fleet Provisioning topic. Topic: %.*s.",
                            ( int ) pxPublishInfo->topicNameLength,
                            ( const char * ) pxPublishInfo->pTopicName ) );
            }
        }
    }
    else
    {
        vHandleOtherIncomingPacket( pxPacketInfo, pxDeserializedInfo->packetIdentifier );
        xResponseStatus = ResponseAccepted;
    }
}
/*-----------------------------------------------------------*/

static bool prvSubscribeToCsrResponseTopics( void )
{
    bool xStatus;

    xStatus = xSubscribeToTopic( &xMqttContext,
                                 FP_CBOR_CREATE_CERT_ACCEPTED_TOPIC,
                                 FP_CBOR_CREATE_CERT_ACCEPTED_LENGTH );

    if( xStatus == false )
    {
        LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                    FP_CBOR_CREATE_CERT_ACCEPTED_LENGTH,
                    FP_CBOR_CREATE_CERT_ACCEPTED_TOPIC ) );
    }

    if( xStatus == true )
    {
        xStatus = xSubscribeToTopic( &xMqttContext,
                                     FP_CBOR_CREATE_CERT_REJECTED_TOPIC,
                                     FP_CBOR_CREATE_CERT_REJECTED_LENGTH );

        if( xStatus == false )
        {
            LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                        FP_CBOR_CREATE_CERT_REJECTED_LENGTH,
                        FP_CBOR_CREATE_CERT_REJECTED_TOPIC ) );
        }
    }

    return xStatus;
}
/*-----------------------------------------------------------*/

static bool prvUnsubscribeFromCsrResponseTopics( void )
{
    bool xStatus;

    xStatus = xUnsubscribeFromTopic( &xMqttContext,
                                     FP_CBOR_CREATE_CERT_ACCEPTED_TOPIC,
                                     FP_CBOR_CREATE_CERT_ACCEPTED_LENGTH );

    if( xStatus == false )
    {
        LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                    FP_CBOR_CREATE_CERT_ACCEPTED_LENGTH,
                    FP_CBOR_CREATE_CERT_ACCEPTED_TOPIC ) );
    }

    if( xStatus == true )
    {
        xStatus = xUnsubscribeFromTopic( &xMqttContext,
                                         FP_CBOR_CREATE_CERT_REJECTED_TOPIC,
                                         FP_CBOR_CREATE_CERT_REJECTED_LENGTH );

        if( xStatus == false )
        {
            LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                        FP_CBOR_CREATE_CERT_REJECTED_LENGTH,
                        FP_CBOR_CREATE_CERT_REJECTED_TOPIC ) );
        }
    }

    return xStatus;
}
/*-----------------------------------------------------------*/

static bool prvSubscribeToRegisterThingResponseTopics( void )
{
    bool xStatus;
    char *fp_reg_accept_topic = NULL;
    char *fp_reg_reject_topic = NULL;

    fp_reg_accept_topic = get_fleet_provisioning_topics(E_FP_CBOR_REGISTER_ACCEPTED_TOPIC);
    if(fp_reg_accept_topic == NULL)
    {
        LogError( ( "Failed to get fleet provisioning topic" ) );
        xStatus = false;
        goto finish;
    }

    xStatus = xSubscribeToTopic( &xMqttContext, fp_reg_accept_topic,
                                 (uint16_t)strlen(fp_reg_accept_topic) );
    if( xStatus == false )
    {
        LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                    strlen(fp_reg_accept_topic),
                    fp_reg_accept_topic ) );
    }

    if( xStatus == true )
    {
        fp_reg_reject_topic = get_fleet_provisioning_topics(E_FP_CBOR_REGISTER_REJECTED_TOPIC);
        if(fp_reg_reject_topic == NULL)
        {
            LogError( ( "Failed to get fleet provisioning topic" ) );
            xStatus = false;
            goto finish;
        }

        xStatus = xSubscribeToTopic( &xMqttContext, fp_reg_reject_topic,
                                     (uint16_t)strlen(fp_reg_reject_topic) );

        if( xStatus == false )
        {
            LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                        strlen(fp_reg_reject_topic),
                        fp_reg_reject_topic ) );
        }
    }

finish:
    if(fp_reg_accept_topic)
    {
        free(fp_reg_accept_topic);
        fp_reg_accept_topic = NULL;
    }
    if(fp_reg_reject_topic)
    {
        free(fp_reg_reject_topic);
        fp_reg_reject_topic = NULL;
    }

    return xStatus;
}
/*-----------------------------------------------------------*/

static bool prvUnsubscribeFromRegisterThingResponseTopics( void )
{
    bool xStatus;
    char *fp_accept_topic = NULL;
    char *fp_reject_topic = NULL;

    fp_accept_topic = get_fleet_provisioning_topics(E_FP_CBOR_REGISTER_ACCEPTED_TOPIC);
    if(fp_accept_topic == NULL)
    {
        LogError( ( "Failed to get the fleet provisioning topic") );
        xStatus = false;
        goto finish;
    }

    xStatus = xUnsubscribeFromTopic( &xMqttContext, fp_accept_topic,
                                     (uint16_t)strlen(fp_accept_topic));
    if( xStatus == false )
    {
        LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                    strlen(fp_accept_topic),
                    fp_accept_topic ) );
    }

    if( xStatus == true )
    {
        fp_reject_topic = get_fleet_provisioning_topics(E_FP_CBOR_REGISTER_REJECTED_TOPIC);
        if(fp_reject_topic == NULL)
        {
            LogError( ( "Failed to get the fleet provisioning topic") );
            xStatus = false;
            goto finish;
        }

        xStatus = xUnsubscribeFromTopic( &xMqttContext, fp_reject_topic,
                                         (uint16_t)strlen(fp_reject_topic));

        if( xStatus == false )
        {
            LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                        strlen(fp_reject_topic),
                        fp_reject_topic ) );
        }
    }

finish:
    if(fp_accept_topic)
    {
        free(fp_accept_topic);
        fp_accept_topic = NULL;
    }
    if(fp_reject_topic)
    {
        free(fp_reject_topic);
        fp_reject_topic = NULL;
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

/* This example uses a single application task, which shows that how to use
 * the Fleet Provisioning library to generate and validate AWS IoT Fleet
 * Provisioning MQTT topics, and use the coreMQTT library to communicate with
 * the AWS IoT Fleet Provisioning APIs. */
int prvFleetProvisioningTask( void )
{
    bool xStatus = false;
    /* Buffer for holding the CSR. */
    char pcCsr[ fpdemoCSR_BUFFER_LENGTH ] = { 0 };
    size_t xCsrLength = 0;
    /* Buffer for holding received certificate until it is saved. */
    char pcCertificate[ fpdemoCERT_BUFFER_LENGTH ] = {0};
    size_t xCertificateLength;
    /* Buffer for holding the certificate ID. */
    char pcCertificateId[ fpdemoCERT_ID_BUFFER_LENGTH ] = {0};
    size_t xCertificateIdLength;
    /* Buffer for holding the certificate ownership token. */
    char pcOwnershipToken[ fpdemoOWNERSHIP_TOKEN_BUFFER_LENGTH ];
    size_t xOwnershipTokenLength;
    bool xConnectionEstablished = false;
    CK_SESSION_HANDLE xP11Session;
    uint32_t ulDemoRunCount = 0U;
    CK_RV xPkcs11Ret = CKR_OK;
    uint8_t wait_cnt = 0;
    MQTTStatus_t xMQTTStatus;
    char *dev_id;
    size_t shadow_topic_len;
    char *shadow_topic = NULL;
    fsp_err_t err;
    char *reg_thing_pub_topic = NULL;

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pxParams = &xTlsTransportParams;

    do
    {
        LogInfo( ( "---------STARTING FLEET PROVISIONING DEMO---------\r\n" ) );

        /* Initialize the buffer lengths to their max lengths. */
        xCertificateLength = fpdemoCERT_BUFFER_LENGTH;
        xCertificateIdLength = fpdemoCERT_ID_BUFFER_LENGTH;
        xOwnershipTokenLength = fpdemoOWNERSHIP_TOKEN_BUFFER_LENGTH;

        /* Initialize the PKCS #11 module */
        xPkcs11Ret = xInitializePkcs11Session( &xP11Session );

        if( xPkcs11Ret != CKR_OK )
        {
            LogError( ( "Failed to initialize PKCS #11." ) );
            xStatus = false;
        }
        else
        {
            xStatus = validate_fleet_provisioning_nvram_config();
            if( xStatus == false )
            {
                LogError( ( "Failed to read provisioning configurations from nvram." ) );
                goto finish_demo;
            }
            else
            {
                xStatus = xGenerateKeyAndCsr( xP11Session,
                                              pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS,
                                              pkcs11configLABEL_DEVICE_PUBLIC_KEY_FOR_TLS,
                                              pcCsr,
                                              fpdemoCSR_BUFFER_LENGTH,
                                              &xCsrLength );
                if( xStatus == false )
                {
                    LogError( ( "Failed to generate Key and Certificate Signing Request." ) );
                    goto finish_demo;
                }
            }
        }

        /**** Connect to AWS IoT Core with provisioning claim credentials *****/

        /* We first use the claim credentials to connect to the broker. These
         * credentials should allow use of the RegisterThing API and one of the
         * CreateCertificatefromCsr or CreateKeysAndCertificate.
         * In this demo we use CreateCertificatefromCsr. */
        if( xStatus == true )
        {
            /* Attempts to connect to the AWS IoT MQTT broker. If the
             * connection fails, retries after a timeout. Timeout value will
             * exponentially increase until maximum attempts are reached. */
            LogInfo( ( "Establishing MQTT session with claim certificate..." ) );
            xStatus = xEstablishMqttSession(&xMqttContext, &xNetworkContext, &xBuffer, prvProvisioningPublishCallback);

            if( xStatus == false )
            {
                LogError( ( "Failed to establish MQTT session." ) );
            }
            else
            {
                LogAlways( ( "Established connection with claim credentials." ) );
                xConnectionEstablished = true;
            }
        }

        /**** Call the CreateCertificateFromCsr API ***************************/

        /* We use the CreateCertificatefromCsr API to obtain a client certificate
         * for a key on the device by means of sending a certificate signing
         * request (CSR). */
        if( xStatus == true )
        {
            /* Subscribe to the CreateCertificateFromCsr accepted and rejected
             * topics. In this demo we use CBOR encoding for the payloads,
             * so we use the CBOR variants of the topics. */
            xStatus = prvSubscribeToCsrResponseTopics();

            if( xStatus == true )
            {
                /* Subscribe to the RegisterThing response topics. */
                xStatus = prvSubscribeToRegisterThingResponseTopics();
            }
        }


        if( xStatus == true )
        {
            /* Create the request payload containing the CSR to publish to the
             * CreateCertificateFromCsr APIs. */
            xStatus = xGenerateCsrRequest( pucPayloadBuffer,
                                           democonfigNETWORK_BUFFER_SIZE,
                                           pcCsr,
                                           xCsrLength,
                                           &xPayloadLength );
        }

        if( xStatus == true )
        {
            mqtt_packet_type_publish_reveived = false;
            /* Publish the CSR to the CreateCertificatefromCsr API. */
            xPublishToTopic( &xMqttContext,
                             FP_CBOR_CREATE_CERT_PUBLISH_TOPIC,
                             FP_CBOR_CREATE_CERT_PUBLISH_LENGTH,
                             ( char * ) pucPayloadBuffer,
                             xPayloadLength );

            if( xStatus == false )
            {
                LogError( ( "Failed to publish to fleet provisioning topic: %.*s.",
                            FP_CBOR_CREATE_CERT_PUBLISH_LENGTH,
                            FP_CBOR_CREATE_CERT_PUBLISH_TOPIC ) );
            }
        }

        wait_cnt = 0;
        while ((mqtt_packet_type_publish_reveived != true) && (wait_cnt < 10))
        {
            LogDebug(("ProcessLoop"));
            xMQTTStatus = prvProcessLoopWithTimeout(&xMqttContext, 300);
            if(xMQTTStatus != MQTTSuccess)
            {
                LogError(("prvProcessLoopWithTimeout failed"));
            }
            wait_cnt++;
        }

        if( xStatus == true )
        {
            /* From the response, extract the certificate, certificate ID, and
             * certificate ownership token. */
            xStatus = xParseCsrResponse( pucPayloadBuffer,
                                         xPayloadLength,
                                         pcCertificate,
                                         &xCertificateLength,
                                         pcCertificateId,
                                         &xCertificateIdLength,
                                         pcOwnershipToken,
                                         &xOwnershipTokenLength );

            if( xStatus == true )
            {
                LogAlways( ( "Received certificate with Id: %.*s", ( int ) xCertificateIdLength, pcCertificateId ) );
                if(strlen(pcCertificateId) == 0)
                {
                    xStatus = false;
                }
            }
        }

        if( xStatus == true )
        {
            /* Save the certificate into PKCS #11. */
            xStatus = xLoadCertificate( xP11Session,
                                        pcCertificate,
                                        pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                        xCertificateLength );
        }

        if( xStatus == true )
        {
            LogInfo(("xLoadCertificate PASS"));
            /* Unsubscribe from the CreateCertificateFromCsr topics. */
            xStatus = prvUnsubscribeFromCsrResponseTopics();
        }
        else
        {
            LogError(("xLoadCertificate FAILED"));
        }

        /**** Call the RegisterThing API **************************************/

        /* We then use the RegisterThing API to activate the received certificate,
         * provision AWS IoT resources according to the provisioning template, and
         * receive device configuration. */
        if( xStatus == true )
        {
            LogInfo(("prvUnsubscribeFromCsrResponseTopics PASS"));

            RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                            AWSIOT_CFG_FLEET_PROVISIONING_DEVICE_ID, &dev_id);
            /* Create the request payload to publish to the RegisterThing API. */
            xStatus = xGenerateRegisterThingRequest( pucPayloadBuffer,
                                                     democonfigNETWORK_BUFFER_SIZE,
                                                     pcOwnershipToken,
                                                     xOwnershipTokenLength,
                                                     dev_id,
                                                     strlen(dev_id),
                                                     &xPayloadLength );
        }
        else
        {
            LogError(("prvUnsubscribeFromCsrResponseTopics FAILED"));
        }


        if( xStatus == true )
        {
            LogInfo(("xGenerateRegisterThingRequest PASS"));
            mqtt_packet_type_publish_reveived = false;
            reg_thing_pub_topic = get_fleet_provisioning_topics(E_FP_CBOR_REGISTER_PUBLISH_TOPIC);
            if (reg_thing_pub_topic)
            {
                /* Publish the RegisterThing request. */
                xPublishToTopic( &xMqttContext, reg_thing_pub_topic,
                                 (uint16_t)strlen(reg_thing_pub_topic),
                                 ( char * ) pucPayloadBuffer,
                                 xPayloadLength );
            }
            else
            {
                xStatus = false;
                LogError( ( "Failed to get the register thingname topic") );
            }

            if( xStatus == false )
            {
                if (reg_thing_pub_topic)
                {
                    LogError( ( "Failed to publish to fleet provisioning topic: %.*s.",
                                strlen(reg_thing_pub_topic),
                                reg_thing_pub_topic ) );
                }
                else
                {
                    LogError( ( "Register thingname topic received is NULL" ) );
                }
            }
            else
            {
                LogInfo(( "SUCCESS: to publish to fleet provisioning topic: %.*s.",
                        strlen(reg_thing_pub_topic),
                        reg_thing_pub_topic ) );
            }

            if (reg_thing_pub_topic)
            {
                free(reg_thing_pub_topic);
                reg_thing_pub_topic = NULL;
            }
        }
        else
        {
            LogError(("xGenerateRegisterThingRequest FAIL"));
        }

        wait_cnt = 0;
        while ((mqtt_packet_type_publish_reveived != true) && (wait_cnt < 10))
        {
            LogInfo(("ProcessLoop"));
            xMQTTStatus = prvProcessLoopWithTimeout(&xMqttContext, 300);
            if(xMQTTStatus != MQTTSuccess)
            {
                LogError(("prvProcessLoopWithTimeout failed"));
            }
            wait_cnt++;
        }

        if( xStatus == true )
        {
            /* Extract the Thing name from the response. */
            xThingNameLength = fpdemoMAX_THING_NAME_LENGTH;
            xStatus = xParseRegisterThingResponse( pucPayloadBuffer,
                                                   xPayloadLength,
                                                   pcThingName,
                                                   &xThingNameLength );

            if( xStatus == true )
            {
                LogAlways( ( "Received AWS IoT Thing name: %.*s", ( int ) xThingNameLength, pcThingName ) );

                err = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                                       AWSIOT_CFG_THINGNAME, pcThingName);
                if (err)
                {
                    LogError(("Thing name write to NVRAM FAILED"));
                }
            }
            else
            {
                LogError(("xParseRegisterThingResponse FAILED"));
            }
        }

        if( xStatus == true )
        {
            /* Unsubscribe from the RegisterThing topics. */
            LogAlways(("Unsubscribe from the RegisterThing topics"));
            prvUnsubscribeFromRegisterThingResponseTopics();
        }

        /**** Disconnect from AWS IoT Core ************************************/

        /* As we have completed the provisioning workflow, we disconnect from
         * the connection using the provisioning claim credentials. We will
         * establish a new MQTT connection with the newly provisioned
         * credentials. */
        if( xConnectionEstablished == true )
        {
            LogAlways(("Disconnect from AWS IoT Core"));
            xDisconnectMqttSession( &xMqttContext, &xNetworkContext );
            xConnectionEstablished = false;
        }

        /**** Connect to AWS IoT Core with provisioned certificate ************/

        if( xStatus == true )
        {
            LogAlways( ( "Establishing MQTT session with provisioned certificate..." ) );
            xStatus = xEstablishMqttSession_P11( &xMqttContext,
                                                 &xNetworkContext,
                                                 &xBuffer,
                                                 prvProvisioningPublishCallback,
                                                 pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                                 pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS );

            if( xStatus != true )
            {
                LogError( ( "Failed to establish MQTT session with provisioned "
                            "credentials. Verify on your AWS account that the "
                            "new certificate is active and has an attached IoT "
                            "Policy that allows the \"iot:Connect\" action." ) );
            }
            else
            {
                LogAlways( ( "Successfully established connection with provisioned credentials." ) );
                xConnectionEstablished = true;
            }
        }

        if( xConnectionEstablished == true )
        {
            shadow_topic_len = strlen(FP_SHADOW_TOPIC_PREFIX) +
                               xThingNameLength +
                               strlen(FP_SHADOW_TOPIC_SUFFIX) + 1;
            shadow_topic = malloc(shadow_topic_len);
            if (shadow_topic != NULL)
            {
                memset(shadow_topic, 0, shadow_topic_len);
                snprintf(shadow_topic, shadow_topic_len,
                         "%s%s%s",
                         FP_SHADOW_TOPIC_PREFIX, pcThingName, FP_SHADOW_TOPIC_SUFFIX);
                LogInfo( ( "PUBLISH MESSAGE: %s\n\rTo TOPIC: %s", SHADOW_MESSAGE, shadow_topic ) );
                xPublishToTopic( &xMqttContext,
                                 shadow_topic,
                                 (int32_t)strlen(shadow_topic),
                                 ( char * ) SHADOW_MESSAGE,
                                 SHADOW_MESSAGE_LEN);
            }
            else
            {
                LogError( ( "Memory allocation failed for publish shadow topic." ) );
            }

            if (shadow_topic)
            {
                free(shadow_topic);
            }
        }
        /**** Finish **********************************************************/

        if( xConnectionEstablished == true )
        {
            /* Close the connection. */
            xDisconnectMqttSession( &xMqttContext, &xNetworkContext );
            xConnectionEstablished = false;
        }

        /**** Retry in case of failure ****************************************/

        /* Increment the demo run count. */
        ulDemoRunCount++;

        if( xStatus == true )
        {
            LogAlways( ( "Demo iteration %lu is successful.", ulDemoRunCount ) );
        }
        /* Attempt to retry a failed iteration of demo for up to #fpdemoMAX_DEMO_LOOP_COUNT times. */
        else if( ulDemoRunCount < fpdemoMAX_DEMO_LOOP_COUNT )
        {
            LogWarn( ( "Demo iteration %lu failed. Retrying...", ulDemoRunCount ) );
            vTaskDelay( fpdemoDELAY_BETWEEN_DEMO_RETRY_ITERATIONS_SECONDS );
        }
        /* Failed all #fpdemoMAX_DEMO_LOOP_COUNT demo iterations. */
        else
        {
            LogError( ( "All %d demo iterations failed.", fpdemoMAX_DEMO_LOOP_COUNT ) );
            break;
        }
    } while( xStatus != true );

    /* Log demo success. */
    if( xStatus == true )
    {
        LogAlways( ( "Fleet provisioning completed successfully." ) );
    }

finish_demo:
    LogAlways( ( "-------Fleet provisioning DEMO FINISHED-------\r\n" ) );

    return ( xStatus == true ) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void connect_to_aws_cloud(void)
{
    bool xStatus = false;
    CK_RV xPkcs11Ret = CKR_OK;
    CK_SESSION_HANDLE xP11Session;
    uint32_t ulDemoRunCount = 0U;
    char *thingname;
    size_t shadow_topic_len;
    char *shadow_topic = NULL;

    xNetworkContext.pxParams = &xTlsTransportParams;
    /* Initialize the PKCS #11 module */
    xPkcs11Ret = xInitializePkcs11Session( &xP11Session );

    if( xPkcs11Ret != CKR_OK )
    {
        LogError(("Failed to initialize PKCS #11"));
        xStatus = false;
    }
    else
    {
        do
        {
            LogInfo(( "Establishing MQTT session with provisioned certificate..." ));
            xStatus = xEstablishMqttSession_P11( &xMqttContext,
                                                 &xNetworkContext,
                                                 &xBuffer,
                                                 prvProvisioningPublishCallback,
                                                 pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                                 pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS );

            if( xStatus != true )
            {
                LogError(( "Failed to establish MQTT session with provisioned "
                           "credentials. Verify on your AWS account that the "
                           "new certificate is active and has an attached IoT "
                           "Policy that allows the \"iot:Connect\" action." ));

                if( ulDemoRunCount < fpdemoMAX_DEMO_LOOP_COUNT )
                {
                    ulDemoRunCount++;
                    LogError(( "Demo iteration %lu failed. Retrying...", ulDemoRunCount ));
                    vTaskDelay( fpdemoDELAY_BETWEEN_DEMO_RETRY_ITERATIONS_SECONDS );
                }
                else
                {
                    LogError(( "All %d demo iterations failed.", fpdemoMAX_DEMO_LOOP_COUNT ));
                    break;
                }
            }
            else
            {
                LogAlways(( "Successfully established connection with provisioned credentials." ));
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_THINGNAME, &thingname);
                if (thingname != NULL)
                {
                    shadow_topic_len = strlen(FP_SHADOW_TOPIC_PREFIX) +
                                       strlen(thingname) +
                                       strlen(FP_SHADOW_TOPIC_SUFFIX) + 1;
                    shadow_topic = malloc(shadow_topic_len);
                    if (shadow_topic != NULL)
                    {
                        memset(shadow_topic, 0, shadow_topic_len);
                        snprintf(shadow_topic, shadow_topic_len,
                                 "%s%s%s",
                                 FP_SHADOW_TOPIC_PREFIX, thingname, FP_SHADOW_TOPIC_SUFFIX);
                        LogInfo( ( "PUBLISH MESSAGE: %s\n\rTo TOPIC: %s", CONN_MESSAGE, shadow_topic ) );
                        xPublishToTopic( &xMqttContext,
                                         shadow_topic,
                                         (int32_t)strlen(shadow_topic),
                                         ( char * ) CONN_MESSAGE,
                                         CONN_MESSAGE_LEN);
                    }
                    else
                    {
                        LogError( ( "Memory allocation failed for publish shadow topic." ) );
                    }
                }
                else
                {
                    LogError( ( "Could not read Device Thing Name from NVRAM." ) );
                }

                if (shadow_topic)
                {
                    free(shadow_topic);
                }
            }
        }while( xStatus != true );
    }
}
/*-----------------------------------------------------------*/
