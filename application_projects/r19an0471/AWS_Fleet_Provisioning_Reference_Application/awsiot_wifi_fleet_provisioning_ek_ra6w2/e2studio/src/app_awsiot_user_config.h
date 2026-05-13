/***********************************************************************************************************************
 * File Name    : app_awsiot_user_config.h
 * Description  : user defines for the application.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if !defined(_APP_AWSIOT_USER_CONFIG_H_)
#define _APP_AWSIOT_USER_CONFIG_H_
/** Define the first default DNS IP address */
#define DEFAULT_AWS_DNS_ADDR					"8.8.8.8" ///< Google DNS 
/** Define the second default DNS IP address */
#define DEFAULT_AWS_2ND_DNS_ADDR				"208.67.222.222" ///< openDNS
/**
 * @brief Response RECV Timeout after device published
 */
#define DEVICE_PUB_RESPONSE_TIMEOUT				300
#define INIT_SENSOR_VAL_4B					    (0xFFFFFFFF) ///< 4bytes initial value for sensor variables
#define AWS_SUBTASK_PRIORITY					1

/**
 * Disable definition of logging interface macros when generating doxygen output,
 * to avoid conflict with documentation of macros at the end of the file.
 */
/* Check that LIBRARY_LOG_LEVEL is defined and has a valid value. */
#if !defined( LIBRARY_LOG_LEVEL ) ||       \
    ( ( LIBRARY_LOG_LEVEL != LOG_NONE ) && \
    ( LIBRARY_LOG_LEVEL != LOG_ERROR ) &&  \
    ( LIBRARY_LOG_LEVEL != LOG_WARN ) &&   \
    ( LIBRARY_LOG_LEVEL != LOG_INFO ) &&   \
    ( LIBRARY_LOG_LEVEL != LOG_DEBUG ) )
    #error "Please define LIBRARY_LOG_LEVEL as either LOG_NONE, LOG_ERROR, LOG_WARN, LOG_INFO, or LOG_DEBUG."
#else
    #if LIBRARY_LOG_LEVEL == LOG_DEBUG
        #ifdef LogAlways
            #undef LogAlways
        #endif
        #ifdef LogError
            #undef LogError
        #endif
        #ifdef LogWarn
            #undef LogWarn
        #endif
        #ifdef LogInfo
            #undef LogInfo
        #endif
        #ifdef LogDebug
            #undef LogDebug
        #endif
        /* All log level messages will logged. */
        #define LogAlways( message )    do { printf message; printf("\r\n"); } while(0)
        #define LogError( message )     do { printf message; printf("\r\n"); } while(0)
        #define LogWarn( message )      do { printf message; printf("\r\n"); } while(0)
        #define LogInfo( message )      do { printf message; printf("\r\n"); } while(0)
        #define LogDebug( message )     do { printf message; printf("\r\n"); } while(0)

    #elif LIBRARY_LOG_LEVEL == LOG_INFO
        /* Only INFO, WARNING, ERROR, and ALWAYS messages will be logged. */
        #define LogAlways( message )    do { printf message; printf("\r\n"); } while(0)
        #define LogError( message )     do { printf message; printf("\r\n"); } while(0)
        #define LogWarn( message )      do { printf message; printf("\r\n"); } while(0)
        #define LogInfo( message )      do { printf message; printf("\r\n"); } while(0)
        #define LogDebug( message )

    #elif LIBRARY_LOG_LEVEL == LOG_WARN
        /* Only WARNING, ERROR, and ALWAYS messages will be logged. */
        #define LogAlways( message )    do { printf message; printf("\r\n"); } while(0)
        #define LogError( message )     do { printf message; printf("\r\n"); } while(0)
        #define LogWarn( message )      do { printf message; printf("\r\n"); } while(0)
        #define LogInfo( message )
        #define LogDebug( message )

    #elif LIBRARY_LOG_LEVEL == LOG_ERROR
        /* Only ERROR and ALWAYS messages will be logged. */
        #define LogAlways( message )    do { printf message; printf("\r\n"); } while(0)
        #define LogError( message )     do { printf message; printf("\r\n"); } while(0)
        #define LogWarn( message )
        #define LogInfo( message )
        #define LogDebug( message )

    #else /* if LIBRARY_LOG_LEVEL == LOG_NONE */

        #define LogAlways( message )
        #define LogError( message )
        #define LogWarn( message )
        #define LogInfo( message )
        #define LogDebug( message )

    #endif /* if LIBRARY_LOG_LEVEL == LOG_NONE */
#endif /* if !defined( LIBRARY_LOG_LEVEL ) || ( ( LIBRARY_LOG_LEVEL != LOG_NONE ) && ( LIBRARY_LOG_LEVEL != LOG_ERROR ) && ( LIBRARY_LOG_LEVEL != LOG_WARN ) && ( LIBRARY_LOG_LEVEL != LOG_INFO ) && ( LIBRARY_LOG_LEVEL != LOG_DEBUG ) ) */
#endif //_APP_AWSIOT_USER_CONFIG_H_
