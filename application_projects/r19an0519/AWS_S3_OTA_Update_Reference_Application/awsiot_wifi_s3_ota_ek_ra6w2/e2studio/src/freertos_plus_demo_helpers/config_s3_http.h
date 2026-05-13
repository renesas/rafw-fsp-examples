/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

#ifndef CONFIG_S3_HTTP_H
#define CONFIG_S3_HTTP_H

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the demo. */
#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME "HTTPDemo"
#endif

#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL LOG_INFO
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/

/**
 * @brief HTTP server port number.
 *
 * For this demo, an X.509 certificate is used to verify the client.
 */
#ifndef democonfigHTTPS_PORT
#define democonfigHTTPS_PORT 443
#endif

/**
 * @brief Endpoint for the AWS IoT credential provider.
 *
 * @note Can be found with
 * `aws iot describe-endpoint --endpoint-type iot:CredentialProvider` from
 * the AWS CLI.
 *
 * #define democonfigIOT_CREDENTIAL_PROVIDER_ENDPOINT    "...insert here..."
 */
#define democonfigIOT_CREDENTIAL_PROVIDER_ENDPOINT                   "c2lulxoqidsim9.credentials.iot.ap-northeast-2.amazonaws.com"

/**
 * @brief Role alias name for accessing the credential provider.
 * 
 * @note This is the name of the role alias created in AWS IoT
 * while setting up AWS resources before running the demo.
 * Refer to the demo setup instructions in the README.md file
 * within the same directory as this file in the repository.

 * #define democonfigIOT_CREDENTIAL_PROVIDER_ROLE   "...insert here..."
 */
#define democonfigIOT_CREDENTIAL_PROVIDER_ROLE                       "AWS_S3_OTA_Role_Alias"

/**
 * @brief Name of bucket in AWS S3 from where file needs to be downloaded.
 *
 * #define democonfigS3_BUCKET_NAME   "...insert here..."
 */
#define democonfigS3_BUCKET_NAME                                     "aws-s3-ota-bucket"

/**
 * @brief AWS Region where the bucket resides.
 * #define democonfigS3_BUCKET_REGION   "...insert here..."
 */
#define democonfigS3_BUCKET_REGION                                   "ap-northeast-2"

/**
 * @brief Name of file that needs to be downloaded from AWS S3.
 * #define democonfigS3_OBJECT_NAME   "...insert here..."
 */
#define democonfigS3_OBJECT_NAME                                     "wifi_ble_coex_ek_ra6w2_ep_ota.img"

/**
 * @brief An option to disable Server Name Indication.
 *
 * @note When using a local server setup, SNI needs to be disabled for a server
 * that only has an IP address but no hostname. However, SNI should be enabled
 * whenever possible.
 */
#define democonfigDISABLE_SNI                                        (pdFALSE)
#define democonfigENABLE_SNI                                         (pdTRUE)

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define democonfigTRANSPORT_SEND_RECV_TIMEOUT_MS                     (5000)

/**
 * @brief The length in bytes of the user buffer.
 */
/**
 * original:
 * #define democonfigUSER_BUFFER_LENGTH (4096)
 * modified more bigger size depending on democonfigRANGE_REQUEST_LENGTH (+2048 B)
 */
#define democonfigUSER_BUFFER_LENGTH                                 (1024) * 30

/**
 * @brief The size of the range of the file to download, with each request.
 *
 * @note This should account for the response headers that will also be stored
 * in the user buffer. We don't expect S3 to send more than 1024 bytes of
 * headers.
 */
/**
 * original:
 * #define democonfigRANGE_REQUEST_LENGTH (2048)
 * For downloading file, democonfigRANGE_REQUEST_LENGTH*99 bytes long covered
 */
#define democonfigRANGE_REQUEST_LENGTH                               (1024) * 28

/**
 * @brief Set the stack size of the main demo task.
 *
 * In the Windows port, this stack only holds a structure. The actual
 * stack is created by an operating system thread.
 */
#define democonfigDEMO_STACKSIZE                                     configMINIMAL_STACK_SIZE

#define malloc                                                       pvPortMalloc
#define free                                                         vPortFree

/**
 * @brief Entry point of the s3_http_ota_demo.
 *
 * This demo demonstrates downloading a file from S3 using SigV4 authentication.
 * First the demo establishes a TLS connection with IoT credential provider
 * server to obtain temporary credentials. Then it connects to S3 server and
 * sends HTTP requests to download the file.
 *
 * @note This example is single-threaded.
 *
 */
BaseType_t aws_s3p_http_ota_create(void);

#endif /* ifndef CONFIG_S3_HTTP_H */
