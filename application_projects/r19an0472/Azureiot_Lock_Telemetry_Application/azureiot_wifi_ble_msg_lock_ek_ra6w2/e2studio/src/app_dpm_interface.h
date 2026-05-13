/**
 ****************************************************************************************
 *
 * @file app_dpm_interface.h
 *
 * @brief DPM related APIs used to operate device on platform.
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */

#if !defined(_APP_DPM_INTERFACE_H_)
#define _APP_DPM_INTERFACE_H_

#include "azure_iot_error.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "user_dpm.h"

#include <stdbool.h>

#include "mbedtls/ssl.h"
//#include "user_dpm_api.h"
#include "app_common_support.h"

/** Used to indicate receiving timeout on DPM mode */
#define DPM_RCV_NO_CONNECT      0            // < before connection with server
#define DPM_RCV_OK_CONNECT      1            // < connection checked
#define DPM_RCV_OK_SLEEP        2            // < ready to sleep

#define DPM_RCV_READY_HANDSHAKE 3            // < ready to handshake

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Notify wakeup init state to DPM manager of M/W.
 * 
 * This function will be called at early time when application thread start
 * @return An IoT Error Type defining successful/failed initializing dpm wakeup
 ****************************************************************************************
 */
IoT_Error_t app_dpm_wakeup_init(void);

/**
 ****************************************************************************************
 * @brief Notify wakeup ready state to DPM manager of M/W
 * @param[in] _pNetwork  Pointer to a Network struct defining the network interface.
 * @return An IoT Error Type defining successful/failed notifying ready
 ****************************************************************************************
 */

//IoT_Error_t app_dpm_wakeup_ready(void);
/**
 ****************************************************************************************
 * @brief Command to go DPM sleep mode
 * @return An IoT Error Type defining successful/failed notifying ready
 ****************************************************************************************
 */
IoT_Error_t app_dpm_goto_sleep(void);

IoT_Error_t app_dpm_exit_sleep(void);

/**
 ****************************************************************************************
 * @brief Unregister a RTC timer for Server wakeup
 * @note Don't release RTM memory, it's called by app_dpm_keepalive_timer_register func
 * @return Always ER_SUCCESS
 ****************************************************************************************
 */
UINT32 app_dpm_keepalive_timer_unregister(void);

/**
 ****************************************************************************************
 * @brief Register a RTC timer for Server wakeup
 * @param[in] _interval Timer interal by second
 * @return ER_ error type
 ****************************************************************************************
 */
UINT32 app_dpm_keepalive_timer_register(int _interval);

/**
 ****************************************************************************************
 * @brief Release the allocated RTM resource for Server apps
 * @return ER_ error type
 ****************************************************************************************
 */
UINT32 app_dpm_info_release(void);


/**
 ****************************************************************************************
 * @brief Save a current client' binding port to app_dpm_info_rtm struct
 * @param[in] _port Port to save
 * @return ER_ error type
 ****************************************************************************************
 */
UINT32 app_dpm_set_client_socket_port(UINT32 _port);

/**
 ****************************************************************************************
 * @brief Get the flag whether ack of publish received or not
 * @return 0 or 1
 ****************************************************************************************
 */
char app_dpm_is_received_pub_ack(void);

/**
 ****************************************************************************************
 * @brief Set the flag meaning that ack of publish received
 * @return void
 ****************************************************************************************
 */
void app_dpm_set_recv_pub_ack(void);

/**
 ****************************************************************************************
 * @brief Get the flag whether report publish sent or not
 * @return 0 or 1
 ****************************************************************************************
 */
char app_dpm_get_report_send_flag(void);

/**
 ****************************************************************************************
 * @brief Set the flag meaning that report publish sent
 * @return void
 ****************************************************************************************
 */
void app_dpm_set_report_send_flag(void);

/**
 ****************************************************************************************
 * @brief Get the reserved local port number
 * @return port number
 ****************************************************************************************
 */
UINT32 app_get_local_port(void);

/**
 ****************************************************************************************
 * @brief Get the local port number which binded with peer and registered for DPM mode
 * @return port number
 ****************************************************************************************
 */
UINT32 app_socket_get_local_port(void);

/**
 ****************************************************************************************
 * @brief Save the subscription information on RTM for DPM wakeup
 * @param[in] _packetIDSub the packet id which subscription completed
 * @param[in] _qosSubCount the total count which subscripton completed
 * @param[in] _qosSubReturn the QOS types which subscription completed
 * @return ER_ type error
 ****************************************************************************************
 */
UINT32 app_dpm_set_subscribe_info(UINT16 _packetIDSub, UINT8 _qosSubCount,
		UINT32 *_qosSubReturn);

/**
 ****************************************************************************************
 * @brief Restore the subscription information from RTM for DPM wakeup
 * @param[in] _packetIDSub the packet id saved
 * @param[in] _qosSubCount the total count saved
 * @return NULL or the array pointer for QOS types saved
 ****************************************************************************************
 */
UINT32* app_dpm_get_subscribe_info(UINT16 *_packetIDSub, UINT8 *_qosSubCount);

/**
 ****************************************************************************************
 * @brief Get the flag whether subscription completed or not
 * @return 0 or 1
 ****************************************************************************************
 */
UINT8 app_dpm_is_subscribed(void);

/**
 ****************************************************************************************
 * @brief Set flag whether server ip is saved
 * @param[in] skip_flag
 * @return void
 ****************************************************************************************
 */
void app_dpm_set_skip_saving_serverip(bool skip_flag);
/**
 ****************************************************************************************
 * @brief Get the flag whether server ip is saved
 * @return 0 or 1
 ****************************************************************************************
 */
bool app_dpm_get_skip_saving_serverip(void);

/**
 ****************************************************************************************
 * @brief Save the current TLS context on RTM region for TLS session
 * @param[in] _name Session name to save
 * @param[in] _tlsDataParams Handle for TLS parameters
 * @return IoT_Error_t type error
 ****************************************************************************************
 */
IoT_Error_t azure_tls_save(const char *name, mbedtls_ssl_context *sslCtx);

/**
 ****************************************************************************************
 * @brief Load the saved TLS context from RTM region for TLS session
 * @param[in] _name Sesson name to load
 * @param[in] _tlsDataParams Handle for TLS parameters
 * @return IoT_Error_t type error
 ****************************************************************************************
 */
IoT_Error_t azure_tls_restore(const char *name, mbedtls_ssl_context *sslCtx);

/**
 ****************************************************************************************
 * @brief Terminate the previous created TLS context on RTM region
 * @param[in] _name Session name to load
 * @return IoT_Error_t type error
 ****************************************************************************************
 */
IoT_Error_t azure_tls_clear(const char *_name);

#endif /* _APP_DPM_INTERFACE_H_ */

