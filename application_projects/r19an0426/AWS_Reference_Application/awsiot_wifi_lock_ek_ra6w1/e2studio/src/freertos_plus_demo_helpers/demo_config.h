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

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "FreeRTOSConfig.h"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "logging_levels.h"

/* awsupgradeport[[:: */
/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"

#if defined(__SUPPORT_AWS_IOT_W__) //awsupgradeport[[::
#undef LIBRARY_LOG_LEVEL
#endif//]]
/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME    "Demos"
#endif

#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif
#endif//]]

#include "logging_stack.h"


/************ End of logging configuration ****************/

#if defined(__SUPPORT_AWS_IOT_W__) //awsupgradeport[[::
/* awsupgradeport[[::for OTA demo */
#include "sdk_ver.h"
/**
 * @brief The version for the firmware which is running. OTA agent uses this
 * version number to perform anti-rollback validation. The firmware version for the
 * download image should be higher than the current version, otherwise the new image is
 * rejected in self test phase.
 */
#define APP_VERSION_MAJOR                     SDK_MAJOR
#define APP_VERSION_MINOR                     SDK_MINOR
#define APP_VERSION_BUILD                     SDK_REVISION

/**
 * @brief ALPN (Application-Layer Protocol Negotiation) protocol name for AWS IoT MQTT.
 *
 * This will be used if democonfigMQTT_BROKER_PORT is configured as 443 for the AWS IoT MQTT broker.
 * Please see more details about the ALPN protocol for AWS IoT MQTT endpoint
 * in the link below.
 * https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/
 */
/**
 * orig SDK[[::for openssl
 * #define AWS_IOT_MQTT_ALPN           "\x0ex-amzn-mqtt-ca"
 */
#define AWS_IOT_MQTT_ALPN           "x-amzn-mqtt-ca"

/**
 * @brief This is the ALPN (Application-Layer Protocol Negotiation) string
 * required by AWS IoT for password-based authentication using TCP port 443.
 */
/**
 *
 * orig SDK[[::for openssl
 * #define AWS_IOT_CUSTOM_AUTH_ALPN    "\x04mqtt"
 */
#define AWS_IOT_CUSTOM_AUTH_ALPN    "mqtt"
#endif //]]
/**
 * @brief The Thing resource registered on your AWS IoT account to use in the demo.
 * A Thing resource is required to communicate with the AWS IoT Device Shadow service.
 *
 * @note The Things associated with your AWS account can be found in the
 * AWS IoT console under Manage/Things, or using the ListThings REST API (that can
 * be called with the AWS CLI command line tool).
 *
 * #define democonfigTHING_NAME    "...insert here..."
 */
#define democonfigTHING_NAME        "s3_download_thing"

#ifndef democonfigCLIENT_IDENTIFIER

/**
 * @brief The MQTT client identifier used in this example.  Each client identifier
 * must be unique so edit as required to ensure no two clients connecting to the
 * same broker use the same client identifier.
 *
 * @note Appending __TIME__ to the client id string will reduce the possibility of a
 * client id collision in the broker. Note that the appended time is the compilation
 * time. This client id can cause collision, if more than one instance of the same
 * binary is used at the same time to connect to the broker.
 */
#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
    #define democonfigCLIENT_IDENTIFIER    "testClient"__TIME__
#else //awsupgradeport[[::for test
	#define deomconfigFD_DEMO_ID_SUFFIX		"Renesas_DoorLockID"
	#define democonfigFP_DEMO_ID			"Renesas_DoorLockID"__TIME__
	#define democonfigCLIENT_IDENTIFIER     "client"democonfigFP_DEMO_ID
#endif //]]
#endif

/**
 * @brief The AWS IoT broker endpoint to connect to in the demo.
 *
 * @note Your AWS IoT Core endpoint can be found in the AWS IoT console under
 * Settings/Custom Endpoint, or using the DescribeEndpoint REST API (that can
 * be called with AWS CLI command line tool).
 *
 * #define democonfigMQTT_BROKER_ENDPOINT    "...insert here..."
 */
/* awsupgradeport[[:: */
#define democonfigMQTT_BROKER_ENDPOINT	"a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com"

#define democonfigAWS_IOT_ENDPOINT	    "a1kzdt4nun8bnh-ats.iot.ap-northeast-2.amazonaws.com"
//]]

/**
 * @brief The port to use for the demo.
 *
 * In general, port 8883 is for secured MQTT connections.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name. Using ALPN with this demo would require additional changes, including
 * setting the `pAlpnProtos` member of the `NetworkCredentials_t` struct before
 * forming the TLS connection. When using port 8883, ALPN is not required.
 *
 * #define democonfigMQTT_BROKER_PORT    ( insert here. )
 */
/* awsupgradeport[[:: */
#ifndef democonfigMQTT_BROKER_PORT
#define democonfigMQTT_BROKER_PORT	443
#endif

/**
 * @brief AWS IoT Core server port number for HTTPS connections.
 *
 * For this demo, an X.509 certificate is used to verify the client.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name being x-amzn-http-ca. When using port 8443, ALPN is not required.
 */
#ifndef democonfigAWS_HTTP_PORT
    #define democonfigAWS_HTTP_PORT    443
#endif
//]]
/**
 * @brief AWS root CA certificate.
 *
 * This certificate is used to identify the AWS IoT server and is publicly available.
 * Refer to the link below.
 * https://www.amazontrust.com/repository/AmazonRootCA1.pem
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define democonfigROOT_CA_PEM    "...insert here..."
 */

/**
 * @brief Client certificate.
 *
 * Please refer to the AWS documentation below for details
 * regarding client authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This certificate should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 *
 * #define democonfigCLIENT_CERTIFICATE_PEM    "...insert here..."
 */

/**
 * @brief Client's private key.
 *
 * Please refer to the AWS documentation below for details
 * regarding client authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This private key should be PEM-encoded.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 *
 * #define democonfigCLIENT_PRIVATE_KEY_PEM    "...insert here..."
 */

#if defined(__SUPPORT_AWS_IOT_W__) //awsupgradeport[[::
/**
 * @brief Name of the provisioning template to use for the RegisterThing
 * portion of the Fleet Provisioning workflow.
 *
 * For information about provisioning templates, see the following AWS documentation:
 * https://docs.aws.amazon.com/iot/latest/developerguide/provision-template.html#fleet-provision-template
 *
 * The example template used for this demo is available in the
 * example_demo_template.json file in the demo directory. In the example,
 * replace <provisioned-thing-policy> with the policy provisioned devices
 * should have.  The demo template uses Fn::Join to construct the Thing name by
 * concatenating fp_demo_ and the serial number sent by the demo.
 *
 * @note The provisioning template MUST be created in AWS IoT before running the
 * demo.
 *
 * #define democonfigPROVISIONING_TEMPLATE_NAME    "...insert here..."
 */
/* awsupgradeport[[::for fleet provisioning */
#define democonfigPROVISIONING_TEMPLATE_NAME    "FleetProvisioningDemoTemplate"
//]]

/**
 * @brief Role alias name for accessing the credential provider.
 * 
 * @note This is the name of the role alias created in AWS IoT
 * while setting up AWS resources before running the demo.
 * Refer to the demo setup instructions in the README.md file
 * within the same directory as this file in the repository.
 *
 * #define democonfigIOT_CREDENTIAL_PROVIDER_ROLE   "...insert here..."
 */
#define democonfigIOT_CREDENTIAL_PROVIDER_ROLE   "s3_role_alias"

/**
 * @brief Endpoint for the AWS IoT credential provider.
 *
 * @note Can be found with
 * `aws iot describe-endpoint --endpoint-type iot:CredentialProvider` from
 * the AWS CLI.
 *
 * #define democonfigIOT_CREDENTIAL_PROVIDER_ENDPOINT    "...insert here..."
 */

#define democonfigIOT_CREDENTIAL_PROVIDER_ENDPOINT    "c2g5uf3pn8mwl3.credentials.iot.us-east-2.amazonaws.com"


/**
 * @brief Name of bucket in AWS S3 from where file needs to be downloaded.
 *
 * #define democonfigS3_BUCKET_NAME   "...insert here..."
 */
#define democonfigS3_BUCKET_NAME     "s3downloadbucket"

/**
 * @brief AWS Region where the bucket resides.
 * #define democonfigS3_BUCKET_REGION   "...insert here..."
 */
#define democonfigS3_BUCKET_REGION   "us-east-2"

/**
 * @brief Name of file that needs to be downloaded from AWS S3.
 * #define democonfigS3_OBJECT_NAME   "...insert here..."
 */
#define democonfigS3_OBJECT_NAME     "certificate_pem.crt"

 /**
  * @brief Subject name to use when creating the certificate signing request (CSR)
  * for provisioning the demo client with using the Fleet Provisioning
  * CreateCertificateFromCsr APIs.
  *
  * This is passed to MbedTLS; see https://tls.mbed.org/api/x509__csr_8h.html#a954eae166b125cea2115b7db8c896e90
  */
#ifndef democonfigCSR_SUBJECT_NAME
    #define democonfigCSR_SUBJECT_NAME    "CN="democonfigFP_DEMO_ID
#endif
 
/**
 * @brief An option to disable Server Name Indication.
 *
 * @note When using a local server setup, SNI needs to be disabled for a server
 * that only has an IP address but no hostname. However, SNI should be enabled
 * whenever possible.
 */
#define democonfigDISABLE_SNI                       ( pdFALSE )

/**
 * @brief This endpoint can be used to publish a message to a topic named topic
 * on AWS IoT Core.
 *
 * Each client certificate has an associated policy document that must be
 * configured to support the path below for this demo to work correctly.
 *
 * @note QoS=1 implies the message is delivered to all subscribers of the topic
 * at least once.
 */
#define democonfigPOST_PATH                         "/topics/topic?qos=1"

/**
 * @brief Request body to send for POST requests in this demo.
 */
#define democonfigREQUEST_BODY                      "{ \"message\": \"Hello, world\" }"
#endif //]]
/**
 * @brief The username value for authenticating client to the MQTT broker when
 * username/password based client authentication is used.
 *
 * Please refer to the AWS IoT documentation below for
 * details regarding client authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * An authorizer setup needs to be done, as mentioned in the above link, to use
 * username/password based client authentication.
 *
 * #define democonfigCLIENT_USERNAME    "...insert here..."
 */

/**
 * @brief The password value for authenticating client to the MQTT broker when
 * username/password based client authentication is used.
 *
 * Please refer to the AWS IoT documentation below for
 * details regarding client authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * An authorizer setup needs to be done, as mentioned in the above link, to use
 * username/password based client authentication.
 *
 * #define democonfigCLIENT_PASSWORD    "...insert here..."
 */

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define democonfigOS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define democonfigOS_VERSION                tskKERNEL_VERSION_NUMBER

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define democonfigHARDWARE_PLATFORM_NAME    "RRQ61XXX"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#include "core_mqtt.h"     /* Include coreMQTT header for MQTT_LIBRARY_VERSION macro. */
/* awsupgradeport[[:: */
#ifndef democonfigMQTT_LIB
//]]
#define democonfigMQTT_LIB               "core-mqtt@"MQTT_LIBRARY_VERSION
#endif

/**
 * @brief Set the stack size of the main demo task.
 *
 * In the Windows port, this stack only holds a structure. The actual
 * stack is created by an operating system thread.
 */
#define democonfigDEMO_STACKSIZE                         configMINIMAL_STACK_SIZE

/**
 * @brief Size of the network buffer for MQTT packets.
 */
/* awsupgradeport[[::for covering fleet provisioning */
#define democonfigNETWORK_BUFFER_SIZE    ( 4096U )
//]]

#if !defined(__SUPPORT_AWS_IOT_W__) //orig SDK[[::
/**
 * @brief Size of the open TCP ports array.
 *
 * A maximum of these many open TCP ports will be sent in the device defender
 * report.
 */
#define democonfigOPEN_TCP_PORTS_ARRAY_SIZE              10

/**
 * @brief Size of the open UDP ports array.
 *
 * A maximum of these many open UDP ports will be sent in the device defender
 * report.
 */
#define democonfigOPEN_UDP_PORTS_ARRAY_SIZE              10

/**
 * @brief Size of the established connections array.
 *
 * A maximum of these many established connections will be sent in the device
 * defender report.
 */
#define democonfigESTABLISHED_CONNECTIONS_ARRAY_SIZE     10

/**
 * @brief Size of the task numbers array.
 *
 * This must be at least the number of tasks used.
 */
#define democonfigCUSTOM_METRICS_TASKS_ARRAY_SIZE        10

/**
 * @brief Size of the buffer which contains the generated device defender report.
 *
 * If the generated report is larger than this, it is rejected.
 */
#define democonfigDEVICE_METRICS_REPORT_BUFFER_SIZE      1000

/**
 * @brief Major version number of the device defender report.
 */
#define democonfigDEVICE_METRICS_REPORT_MAJOR_VERSION    1

/**
 * @brief Minor version number of the device defender report.
 */
#define democonfigDEVICE_METRICS_REPORT_MINOR_VERSION    0

#else //awsupgradeport[[::
/**
 * @brief Predefined shadow name.
 *
 * Defaults to unnamed "Classic" shadow. Change to a custom string to use a named shadow.
 */
#ifndef democonfigSHADOW_NAME
    #define democonfigSHADOW_NAME    SHADOW_NAME_CLASSIC
#endif

/* awsupgradeport[[::set the maximum value */
/**
 * @brief Keep alive time reported to the broker while establishing an MQTT connection.
 *
 * It is the responsibility of the Client to ensure that the interval between
 * Control Packets being sent does not exceed this Keep Alive value. In the
 * absence of sending any other Control Packets, the Client MUST send a
 * PINGREQ Packet.
 */
#define mqttexampleKEEP_ALIVE_TIMEOUT_SECONDS             ( 1740U )
//]]
#endif //]]
#endif /* DEMO_CONFIG_H */
