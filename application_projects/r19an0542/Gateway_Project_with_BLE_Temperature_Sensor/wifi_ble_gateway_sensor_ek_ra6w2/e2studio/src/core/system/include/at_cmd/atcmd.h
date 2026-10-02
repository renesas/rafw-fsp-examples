/**
 ****************************************************************************************
 *
 * @file atcmd.h
 *
 * @brief Header file for DA16200/DA16600 AT-CMD
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

#ifndef __ATCMD_H__
#define __ATCMD_H__


#include "da16x_types.h"
#include "da16x_system.h"
#include "task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"


#include "clib.h"
#include "uart.h"

#include "command.h"
#include "command_sys.h"
#include "command_net.h"
#include "command_host.h"
#include "common_uart.h"

#include "iface_defs.h"
#include "da16x_network_common.h"
#include "environ.h"
#include "da16x_time.h"
#include "util_api.h"
#include "sys_feature.h"

#include "atcmd_peripheral.h"
#include "user_dpm.h"
#include "user_dpm_api.h"

#include "common_utils.h"
#include "common_config.h"
#include "user_nvram_cmd_table.h"

#include "da16x_dpm_sockets.h"
#include "da16x_ping.h"
#include "dhcpserver.h"
#include "dhcpserver_options.h"
#include "user_http_server.h"
#include "user_http_client.h"
#include "da16x_dhcp_server.h"
#include "da16x_network_main.h"
#include "mqtt_client.h"

#if defined (__SUPPORT_OTA__)
#include "ota_update.h"
#include "ota_update_common.h"

#if defined (__OTA_UPDATE_MCU_FW__)
#include "ota_update_mcu_fw.h"
#endif // __OTA_UPDATE_MCU_FW__
#endif // __SUPPORT_OTA__

#if defined (__SUPPORT_ZERO_CONFIG__)
#include "zero_config.h"
#endif // __SUPPORT_ZERO_CONFIG__

#if defined (__SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__)
#include "ws_err.h"
#include "atcmd_websocket_client.h"
#endif // __SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__

#if defined (__SUPPORT_ATCMD_TLS__)
#include "app_errno.h"
#include "atcmd_tls_client.h"
#endif // __SUPPORT_ATCMD_TLS__

#include "pwm.h"
#include "sys_i2s.h"
#include "sys_i2c.h"
#include "sys_image.h"
#include "adc.h"

#if defined (__SUPPORT_PERI_CTRL__)
#include "atcmd_peri_ctrl.h"
#endif // __SUPPORT_PERI_CTRL__

#if defined (__BLE_COMBO_REF__)
#include "atcmd_ble.h"
#endif // __BLE_COMBO_REF__



/**************************************************************************
 * AT-CMD defines
 **************************************************************************/

#define DELIMIT                          ((char*) " ")
#define DELIMIT_FIRST                    ((char*) "=")
#define DELIMIT_COMMA                    ((char*) ",")

#define TX_PAYLOAD_MAX_SIZE              HTTPC_REQ_DATA_MAX_SIZE
#define USER_ATCMD_BUF                   (TX_PAYLOAD_MAX_SIZE + 32)
#define ATCMD_STACK_SIZE                 (1024 * 10)
#define MAX_PARAMS                       20
#define ESC_CMD_MAX_PARAMS               3
#define ESCAPE_CODE                      0x1B

#define ATCMD_ON                         1
#define ATCMD_OFF                        0

/// AT-CMD Debug message on UART0
#undef    DEBUG_ATCMD

#if defined (DEBUG_ATCMD)
  #define ATCMD_DBG                      PRINTF
#else
  #define ATCMD_DBG(...)                 do { } while (0);
#endif // (DEBUG_ATCMD)

/// defined - Use APP_POOL / Undefined - Use Heap memory
#define ATCMD_MALLOC(...)                pvPortMalloc( __VA_ARGS__ )
#define ATCMD_FREE(...)                  vPortFree( __VA_ARGS__ )

/// TCP Server Connection Prefix
#define ATCMD_TCPS_CONN_RX_PREFIX        "+TRCTS"
/// TCP Server Disconnection Prefix
#define ATCMD_TCPS_DISCONN_RX_PREFIX     "+TRXTS"
/// TCP Client Disconnection Prefix
#define ATCMD_TCPC_DISCONN_RX_PREFIX     "+TRXTC"

/// Rx TCP Server message Prefix
#define ATCMD_TCPS_DATA_RX_PREFIX        "+TRDTS"
/// Rx TCP Client message Prefix
#define ATCMD_TCPC_DATA_RX_PREFIX        "+TRDTC"
/// Rx UDP message Prefix
#define ATCMD_UDP_DATA_RX_PREFIX         "+TRDUS"
/// UDP Session Fail Prefix
#define ATCMD_UDP_SESS_FAIL_PREFIX       "+TRXUS"

/// NVRAM name of TCP server local port
#define ATC_NVRAM_TCPS_PORT              "ATC_TS_PORT"

/// NVRAM name of TCP client remote IP address
#define ATC_NVRAM_TCPC_IP                "ATC_TC_IP"

/// NVRAM name of TCP client remote port
#define ATC_NVRAM_TCPC_RPORT             "ATC_TC_RPORT"

/// NVRAM name of TCP client local port
#define ATC_NVRAM_TCPC_LPORT             "ATC_TC_LPORT"

/// NVRAM name of UDP session local port
#define ATC_NVRAM_UDPS_PORT              "ATC_US_PORT"

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
#define ATCMD_NW_TR_MAX_NVR_CNT           10
#define ATCMD_NW_TR_MAX_SESSION_CNT       ATCMD_NW_TR_MAX_NVR_CNT    // 10. Must be less than ATCMD_NW_TR_MAX_NVR_CNT
#define ATCMD_NW_TR_MAX_SESSION_CNT_DPM   DPM_SOCK_MAX_TCP_SESS      // 8. Must be less than ATCMD_NW_TR_MAX_NVR_CNT.

typedef enum _atcmd_sess_type {
    ATCMD_SESS_NONE                  = -1,
    ATCMD_SESS_TCP_SERVER            = 0,
    ATCMD_SESS_TCP_CLIENT            = 1,
    ATCMD_SESS_UDP_SESSION           = 2,
} atcmd_sess_type;

typedef enum _atcmd_sess_nvr_type {
    ATCMD_SESS_NVR_CID               = 0,
    ATCMD_SESS_NVR_LOCAL_PORT        = 1,
    ATCMD_SESS_NVR_PEER_PORT         = 2,
    ATCMD_SESS_NVR_PEER_IP_ADDRESS   = 3,
    ATCMD_SESS_NVR_MAX_ALLOWED_PEER  = 4,
} atcmd_sess_nvr_type;

#define ATCMD_RTM_NW_TR_NAME                  "ATC_NW_TR_NAME"

// Name of NVRAM to support multi session
#define ATCMD_NVR_NW_TR_CID_0                 "0:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_1                 "1:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_2                 "2:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_3                 "3:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_4                 "4:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_5                 "5:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_6                 "6:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_7                 "7:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_8                 "8:ATC_NW_TR_CID"
#define ATCMD_NVR_NW_TR_CID_9                 "9:ATC_NW_TR_CID"

#define ATCMD_NVR_NW_TR_LOCAL_PORT_0          "0:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_1          "1:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_2          "2:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_3          "3:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_4          "4:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_5          "5:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_6          "6:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_7          "7:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_8          "8:ATC_NW_TR_LPORT"
#define ATCMD_NVR_NW_TR_LOCAL_PORT_9          "9:ATC_NW_TR_LPORT"

#define ATCMD_NVR_NW_TR_PEER_PORT_0           "0:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_1           "1:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_2           "2:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_3           "3:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_4           "4:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_5           "5:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_6           "6:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_7           "7:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_8           "8:ATC_NW_TR_PPORT"
#define ATCMD_NVR_NW_TR_PEER_PORT_9           "9:ATC_NW_TR_PPORT"

#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_0    "0:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_1    "1:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_2    "2:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_3    "3:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_4    "4:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_5    "5:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_6    "6:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_7    "7:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_8    "8:ATC_NW_TR_MAX_PEER"
#define ATCMD_NVR_NW_TR_MAX_ALLOWED_PEER_9    "9:ATC_NW_TR_MAX_PEER"

#define ATCMD_NVR_NW_TR_PEER_IPADDR_0         "0:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_1         "1:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_2         "2:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_3         "3:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_4         "4:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_5         "5:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_6         "6:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_7         "7:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_8         "8:ATC_NW_TR_PIPADDR"
#define ATCMD_NVR_NW_TR_PEER_IPADDR_9         "9:ATC_NW_TR_PIPADDR"

#define ATCMD_NVR_NW_TR_PEER_IPADDR_LEN        16    // IPv4

#endif // __SUPPORT_ATCMD_MULTI_SESSION__

/// The number of AT-CMD TCP/UDP Client number (TCP server/client, UDP)
#define ATCMD_MAX_CID                          3

/// TCP Server CID
#define ID_TS                                  0
/// TCP Client CID
#define ID_TC                                  1
/// UDP Session CID
#define ID_US                                  2

#if !defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
/// TCP/UDP Tx/Rx data buffer size
#define    ATCMD_DATA_BUF_SIZE                 2048
#define ATCMD_HDR_BUF_SIZE                     128

#if defined (__ATCMD_IF_SDIO__) || defined (__ATCMD_IF_SPI__)
  #define ATCMD_TCP_WIN_SZ                     ( 16 * 1024 )

  #define ATCMD_TCPS_WIN_SZ                    ( 8 * 1024 )
  #define ATCMD_TCPC_WIN_SZ                    ( 8 * 1024 )
#else
  #define ATCMD_TCP_WIN_SZ                     ( 8 * 1024 )

  #define ATCMD_TCPS_WIN_SZ                    ATCMD_TCP_WIN_SZ
  #define ATCMD_TCPC_WIN_SZ                    ATCMD_TCP_WIN_SZ
#endif // __ATCMD_IF_SDIO__ || __ATCMD_IF_SPI__
#endif // !__SUPPORT_ATCMD_MULTI_SESSION__

/// TCP client reconnection timeout (sec.)
#define ATCMD_TCPC_RECONNECT_TIMEOUT           3

 /// Wi-Fi Scan buffer size (need to sync supp_eloop.h)
#define    CLI_SCAN_RSP_BUF_SIZE               3584

#if defined ( __SUPPORT_ATCMD_TLS__ )
  #define ATCMD_TLS_MAX_ALLOW_CNT              2

  // For NVRAM
  #define ATCMD_TLS_NVR_CID_0                  "0:ATC_TLS_CID"
  #define ATCMD_TLS_NVR_CID_1                  "1:ATC_TLS_CID"
  #define ATCMD_TLS_NVR_ROLE_0                 "0:ATC_TLS_ROLE"
  #define ATCMD_TLS_NVR_ROLE_1                 "1:ATC_TLS_ROLE"
  #define ATCMD_TLS_NVR_PROFILE_0              "0:ATC_TLS_PROFILE"
  #define ATCMD_TLS_NVR_PROFILE_1              "1:ATC_TLS_PROFILE"

  #define ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE     2

  #define ATCMD_TLSC_NVR_CA_CERT_NAME_0        "0:ATC_TLSC_CA_NAME"
  #define ATCMD_TLSC_NVR_CA_CERT_NAME_1        "1:ATC_TLSC_CA_NAME"
  #define ATCMD_TLSC_NVR_CA_CERT_NAME_LEN      32                      // ATCMD_CM_MAX_NAME

  #define ATCMD_TLSC_NVR_CERT_NAME_0           "0:ATC_TLSC_CERT_NAME"
  #define ATCMD_TLSC_NVR_CERT_NAME_1           "1:ATC_TLSC_CERT_NAME"
  #define ATCMD_TLSC_NVR_CERT_NAME_LEN         32                      // ATCMD_CM_MAX_NAME

  #define ATCMD_TLSC_NVR_HOST_NAME_0           "0:ATC_TLSC_HOST_NAME"
  #define ATCMD_TLSC_NVR_HOST_NAME_1           "1:ATC_TLSC_HOST_NAME"
  #define ATCMD_TLSC_NVR_HOST_NAME_LEN         32                      // ATCMD_TLSC_MAX_HOSTNAME

  #define ATCMD_TLSC_NVR_AUTH_MODE_0           "0:ATC_TLSC_AUTH_MODE"
  #define ATCMD_TLSC_NVR_AUTH_MODE_1           "1:ATC_TLSC_AUTH_MODE"

  #define ATCMD_TLSC_NVR_INCOMING_LEN_0        "0:ATC_TLSC_INCOMING"
  #define ATCMD_TLSC_NVR_INCOMING_LEN_1        "1:ATC_TLSC_INCOMING"

  #define ATCMD_TLSC_NVR_OUTGOING_LEN_0        "0:ATC_TLSC_OUTGOING"
  #define ATCMD_TLSC_NVR_OUTGOING_LEN_1        "1:ATC_TLSC_OUTGOING"

  #define ATCMD_TLSC_NVR_LOCAL_PORT_0          "0:ATC_TLSC_LPORT"
  #define ATCMD_TLSC_NVR_LOCAL_PORT_1          "1:ATC_TLSC_LPORT"

  #define ATCMD_TLSC_NVR_PEER_PORT_0           "0:ATC_TLSC_PPORT"
  #define ATCMD_TLSC_NVR_PEER_PORT_1           "1:ATC_TLSC_PPORT"

  #define ATCMD_TLSC_NVR_PEER_IPADDR_0         "0:ATC_TLSC_PIP"
  #define ATCMD_TLSC_NVR_PEER_IPADDR_1         "1:ATC_TLSC_PIP"
  #define ATCMD_TLSC_NVR_PEER_IPADDR_LEN       16                      // IPv4

  #define ATCMD_TLS_DPM_CONTEXT_NAME           "atcmd_tls_ctx"

  #define ATCMD_TLS_CERTSTORE_CMD              "AT+TRSSLCERTSTORE"
  #define ATCMD_TLS_WR_CMD                     "AT+TRSSLWR"

#endif // __SUPPORT_ATCMD_TLS__

#if defined ( __SUPPORT_PASSIVE_SCAN__ )
  #define ATCMD_PASSIVE_SCAN_PRINT_CHECK       0
  #define ATCMD_PASSIVE_SCAN_PRINT_TRUE        1                       // PRINTF_ATCMD
  #define ATCMD_PASSIVE_SCAN_PRINT_FALSE       2                       // PRINTF
#endif // __SUPPORT_PASSIVE_SCAN__ 


#if defined ( __ATCMD_IF_SDIO__ ) || defined ( __ATCMD_IF_SPI__ )
#define SLAVE_CMD_SIZE              (8192 + 64)
#else
#define SLAVE_CMD_SIZE              (256)
#endif // __ATCMD_IF_SDIO__ || __ATCMD_IF_SPI__


void *_zalloc(size_t size);

/// Error Response Codes
typedef enum _atcmd_error_code {
    /// No error (without Debugging messages)
    AT_CMD_ERR_CMD_OK_WO_PRINT                      =  1,

    /// No error
    AT_CMD_ERR_CMD_OK                               =  0,

    /// Unknown command
    AT_CMD_ERR_UNKNOWN_CMD                          = -1,

    /// Insufficient parameter
    AT_CMD_ERR_INSUFFICENT_ARGS                     = -2,

    /// Too many parameters
    AT_CMD_ERR_TOO_MANY_ARGS                        = -3,

    /// Wrong parameter value
    AT_CMD_ERR_WRONG_ARGUMENTS                      = -4,

    /// Unsupported function
    AT_CMD_ERR_NOT_SUPPORTED                        = -5,

    /// Not connected to an AP or a socket
    AT_CMD_ERR_NOT_CONNECTED                        = -6,

    /// No result
    AT_CMD_ERR_NO_RESULT                            = -7,

    /// Response buffer overflow
    AT_CMD_ERR_TOO_LONG_RESULT                      = -8,

    /// Function is not configured
    AT_CMD_ERR_INSUFFICENT_CONFIG                   = -9,

    /// Command timeout
    AT_CMD_ERR_TIMEOUT                              = -10,

    /// NVRAM write failure
    AT_CMD_ERR_NVR_WRITE                            = -11,

    /// Retention memory write failure
    AT_CMD_ERR_RTM_WRITE                            = -12,

    /// System busy so cannot handle the request
    AT_CMD_ERR_SYS_BUSY                             = -13,

    /// Memory allocation failure
    AT_CMD_ERR_MEM_ALLOC                            = -14,

    /// Data Tx failure
    AT_CMD_ERR_DATA_TX                              = -20,

    /// IP address get failure
    AT_CMD_ERR_IP_ADDRESS                           = -22,

    /// AT-CMD Common function
    AT_CMD_ERR_COMMON_SYS_MODE                      = -100,

    AT_CMD_ERR_COMMON_ARG_TYPE                      = -110,
    AT_CMD_ERR_COMMON_ARG_RANGE                     = -111,
    AT_CMD_ERR_COMMON_ARG_LEN                       = -112,
    AT_CMD_ERR_COMMON_WRONG_CC                      = -113,
    AT_CMD_ERR_COMMON_WRONG_MAC_ADDR                = -114,


    /// AT-CMD FLASH operation related
    AT_CMD_ERR_SFLASH_READ                          = -170,
    AT_CMD_ERR_SFLASH_WRITE                         = -171,
    AT_CMD_ERR_SFLASH_ERASE                         = -172,
    AT_CMD_ERR_SFLASH_ACCESS                        = -173,


    /// AT-CMD NVRAM operation related
    AT_CMD_ERR_NVRAM_READ                           = -180,
    AT_CMD_ERR_NVRAM_WRITE                          = -181,
    AT_CMD_ERR_NVRAM_ERASE                          = -182,
    AT_CMD_ERR_NVRAM_DIGIT                          = -183,
    AT_CMD_ERR_NVRAM_SAME_MAC                       = -184,
    AT_CMD_ERR_NVRAM_CANCELED                       = -185,
    AT_CMD_ERR_NVRAM_INVALID                        = -186,
    AT_CMD_ERR_NVRAM_UNKNOWN                        = -187,
    AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE                = -188,


    /// AT-CMD Basic function
    AT_CMD_ERR_BASIC_ARG_NULL_PTR                   = -200,
    AT_CMD_ERR_BASIC_ARG_DATE                       = -201,
    AT_CMD_ERR_BASIC_ARG_TIME                       = -202,
    AT_CMD_ERR_BASIC_ARG_TIME_ETC                   = -203,


    /// AT-CMD UART function
    AT_CMD_ERR_UART_INTERFACE                       = -220,
    AT_CMD_ERR_UART_BAUDRATE                        = -221,
    AT_CMD_ERR_UART_DATABITS                        = -222,
    AT_CMD_ERR_UART_PARITY                          = -223,
    AT_CMD_ERR_UART_STOPBIT                         = -224,
    AT_CMD_ERR_UART_FLOWCTRL                        = -225,
    AT_CMD_ERR_UART_BAUDRATE_NV_WR                  = -226,
    AT_CMD_ERR_UART_DATABITS_NV_WR                  = -227,
    AT_CMD_ERR_UART_PARITY_NV_WR                    = -228,
    AT_CMD_ERR_UART_STOPBIT_NV_WR                   = -229,
    AT_CMD_ERR_UART_FLOWCTRL_NV_WR                  = -230,


    /// AT-CMD DPM function
    AT_CMD_ERR_DPM_MODE_DISABLED                    = -300,
    AT_CMD_ERR_DPM_SLEEP_STARTED                    = -301,
    AT_CMD_ERR_DPM_FAST_CONN_EN                     = -302,
    AT_CMD_ERR_DPM_USER_RTM_ALLOC                   = -303,
    AT_CMD_ERR_DPM_USER_RTM_DUP                     = -304,
    AT_CMD_ERR_DPM_MODE_ARG                         = -305,
    AT_CMD_ERR_DPM_NVRAM_FLAG_ARG                   = -306,
    AT_CMD_ERR_DPM_SLP2_PERIOD_TYPE                 = -309,
    AT_CMD_ERR_DPM_SLP2_PERIOD_RANGE                = -310,
    AT_CMD_ERR_DPM_SLP2_RTM_FLAG_ARG                = -311,
    AT_CMD_ERR_DPM_SLP2_RTM_FLAG_RANGE              = -312,
    AT_CMD_ERR_DPM_SLP1_RTM_FLAG_ARG                = -313,
    AT_CMD_ERR_DPM_SLP1_RTM_FLAG_RANGE              = -314,
#if !defined ( __DISABLE_DPM_ABNORM__ )
    AT_CMD_ERR_DPM_ABN_ARG                          = -315,
#endif // !__DISABLE_DPM_ABNORM__
    AT_CMD_ERR_DPM_SLP2_DPM_MODE_ENABLED            = -316,
    AT_CMD_ERR_DPM_SLP3_PERIOD_TYPE                 = -317,
    AT_CMD_ERR_DPM_SLP3_PERIOD_RANGE                = -318,


    /// AT-CMD WIFI function
    AT_CMD_ERR_WIFI_NOT_CONNECTED                   = -400,
    AT_CMD_ERR_WIFI_RUN_MODE_TYPE                   = -401,
    AT_CMD_ERR_WIFI_RUN_MODE_RANGE                  = -402,
    AT_CMD_ERR_WIFI_MAC_ADDR                        = -403,
    AT_CMD_ERR_WIFI_WPS_PIN_NUM                     = -404,
    AT_CMD_ERR_WIFI_CC_WRONG_FREQ                   = -405,
    AT_CMD_ERR_WIFI_SCAN_UNSUPPORTED                = -406,
    AT_CMD_ERR_WIFI_PSCAN_FREQ_RANGE                = -407,
    AT_CMD_ERR_WIFI_PSCAN_CMAX_RANGE                = -408,
    AT_CMD_ERR_WIFI_PSCAN_CMIN_RANGE                = -409,
    AT_CMD_ERR_WIFI_JAP_SSID_NO_VALUE               = -410,
    AT_CMD_ERR_WIFI_JAP_SSID_LEN                    = -411,
    AT_CMD_ERR_WIFI_JAP_SECU_ARG_TYPE               = -412,
    AT_CMD_ERR_WIFI_JAP_SECU_ARG_RANGE              = -413,
    AT_CMD_ERR_WIFI_JAP_OPEN_TOO_MANY_ARG           = -414,
    AT_CMD_ERR_WIFI_JAP_OPEN_HIDDEN_TYPE            = -415,
    AT_CMD_ERR_WIFI_JAP_OPEN_HIDDEN_RANGE           = -416,
    AT_CMD_ERR_WIFI_JAP_SECU_HIDDEN_TYPE            = -417,
    AT_CMD_ERR_WIFI_JAP_SECU_HIDDEN_RANGE           = -418,
    AT_CMD_ERR_WIFI_JAP_WEP_IDX_TYPE                = -419,
    AT_CMD_ERR_WIFI_JAP_WEP_IDX_RANGE               = -420,
    AT_CMD_ERR_WIFI_JAP_WEP_KEY_LEN                 = -421,
    AT_CMD_ERR_WIFI_JAP_WPA_MODE_TYPE               = -422,
    AT_CMD_ERR_WIFI_JAP_WPA_MODE_RANGE              = -423,
    AT_CMD_ERR_WIFI_JAP_WPA_KEY_LEN                 = -424,
    AT_CMD_ERR_WIFI_JAPA_SSID_NO_VALUE              = -425,
    AT_CMD_ERR_WIFI_JAPA_SSID_LEN                   = -426,
    AT_CMD_ERR_WIFI_JAPA_PSK_LEN                    = -427,
    AT_CMD_ERR_WIFI_JAPA_WEP_NOT_SUPPORT            = -428,
    AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE                = -429,
    AT_CMD_ERR_WIFI_JAPA_HIDDEN_RANGE               = -430,
    AT_CMD_ERR_WIFI_JAPA_WPA3_MODE_TYPE             = -431,
    AT_CMD_ERR_WIFI_JAPA_WPA3_MODE_RANGE            = -432,
    AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_TYPE           = -433,
    AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_RANGE          = -434,
    AT_CMD_ERR_WIFI_ROAP_ROAM_TYPE                  = -435,
    AT_CMD_ERR_WIFI_ROAP_ROAM_RANGE                 = -436,
    AT_CMD_ERR_WIFI_ENTAP_SSID_NO_VALUE             = -437,
    AT_CMD_ERR_WIFI_ENTAP_SSID_LEN                  = -438,
    AT_CMD_ERR_WIFI_ENTAP_AUTH0_UNSUPPORT           = -439,
    AT_CMD_ERR_WIFI_ENTAP_ENC0_UNSUPPORT            = -440,
    AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE1                = -441,
    AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE2                = -442,
    AT_CMD_ERR_WIFI_ENTAP_SECU_MODE                 = -443,
    AT_CMD_ERR_WIFI_ENTAP_ENC_MODE                  = -444,
    AT_CMD_ERR_WIFI_ENTAP_EAP_MODE                  = -445,
    AT_CMD_ERR_WIFI_ENTAP_EAP_ID_NO_VALUE           = -446,
    AT_CMD_ERR_WIFI_ENTAP_EAP_ID_LEN                = -447,
    AT_CMD_ERR_WIFI_ENTAP_EAP_PWD_LEN               = -448,
    AT_CMD_ERR_WIFI_SOFTAP_SSID_NO_VALUE            = -449,
    AT_CMD_ERR_WIFI_SOFTAP_SECU_MODE                = -450,
    AT_CMD_ERR_WIFI_SOFTAP_ENC_MODE                 = -451,
    AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_TYPE            = -452,
    AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_RANGE           = -453,
    AT_CMD_ERR_WIFI_SOFTAP_OPEN_TOO_MANY_ARG        = -454,
    AT_CMD_ERR_WIFI_SOFTAP_CH_TX_PWR_VALUE          = -455,
    AT_CMD_ERR_WIFI_SOFTAP_WEP_NOT_SUPPORT          = -456,
    AT_CMD_ERR_WIFI_SOFTAP_ENC_MODE_TYPE            = -457,
    AT_CMD_ERR_WIFI_SOFTAP_ENC_MODE_RANGE           = -458,
    AT_CMD_ERR_WIFI_SOFTAP_PASSKEY_LEN              = -459,

    // ... Add here additional WIFI error code ...
    AT_CMD_ERR_WIFI_ALREADY_CONNECTED               = -460,
    AT_CMD_ERR_WIFI_CONCURRENT_NO_PROFILE           = -461,
    AT_CMD_ERR_WIFI_PSCAN_DURATION                  = -462,
    AT_CMD_ERR_WIFI_JAPA_WPA3_PSK_LEN               = -463,
    AT_CMD_ERR_WIFI_SOFTAP_OWE_TOO_MANY_ARG         = -464,
    AT_CMD_ERR_WIFI_SOFTAP_SSID_LEN                 = -465,
    AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_TYPE           = -466,
    AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_RANGE          = -467,
    AT_CMD_ERR_WIFI_SOFTAP_CC_VALUE                 = -468,

#if defined ( __SUPPORT_P2P__ )
    // AT-CMD WiFI - Wi-Fi Direct
    AT_CMD_ERR_WIFI_P2P_CONN_WPS_TYPE               = -481,        // p2p_connect <MAC>
    AT_CMD_ERR_WIFI_P2P_UNSUPPORT_WPS_TYPE          = -482,
    AT_CMD_ERR_WIFI_P2P_CONN_JOIN_TYPE              = -483,
    AT_CMD_ERR_WIFI_P2P_CONN_JOIN_RANGE             = -484,
    AT_CMD_ERR_WIFI_P2P_LISTEN_CH_TYPE              = -485,
    AT_CMD_ERR_WIFI_P2P_LISTEN_CH_RANGE             = -486,
    AT_CMD_ERR_WIFI_P2P_OPERATION_CH_TYPE           = -487,
    AT_CMD_ERR_WIFI_P2P_OPERATION_CH_RANGE          = -488,
    AT_CMD_ERR_WIFI_P2P_GO_INTENT_TYPE              = -489,
    AT_CMD_ERR_WIFI_P2P_GO_INTENT_RANGE             = -490,
    AT_CMD_ERR_WIFI_P2P_FIND_TIMEOUT_TYPE           = -491,
    AT_CMD_ERR_WIFI_P2P_FIND_TIMEOUT_RANGE          = -492,
#endif    // __SUPPORT_P2P__


    /// AT-CMD WIFI CLI operation related - common
    AT_CMD_ERR_WIFI_CLI_STATUS                      = -500,
    AT_CMD_ERR_WIFI_CLI_SET_NETWORK                 = -501,
    AT_CMD_ERR_WIFI_CLI_SET_NETWORK_HIDDEN          = -502,
    AT_CMD_ERR_WIFI_CLI_SELECT_NETWORK              = -503,
    AT_CMD_ERR_WIFI_CLI_SAVE_CONF                   = -504,
    AT_CMD_ERR_WIFI_CLI_SAVE_CONF_HIDDEN            = -505,
    AT_CMD_ERR_WIFI_CLI_DISCONNECT                  = -506,        
    AT_CMD_ERR_WIFI_CLI_DEAUTHENTICATE              = -507,        // cli deauthenticate <MAC>
    AT_CMD_ERR_WIFI_CLI_DISASSOCIATE                = -508,        // cli disassociate <MAC>

    /// AT-CMD WIFI CLI operation related
    AT_CMD_ERR_WIFI_CLI_WPS_PBC_ANY                 = -510,
    AT_CMD_ERR_WIFI_CLI_WPS_PIN_GET                 = -511,
    AT_CMD_ERR_WIFI_CLI_WPS_PIN_ANY                 = -512,
    AT_CMD_ERR_WIFI_CLI_WPS_PIN_NUM                 = -513,
    AT_CMD_ERR_WIFI_CLI_WPS_CANCEL                  = -514,
    AT_CMD_ERR_WIFI_CLI_COUNTRY                     = -515,
    AT_CMD_ERR_WIFI_CLI_PSCAN_CH_TL                 = -516,
    AT_CMD_ERR_WIFI_CLI_PSCAN_STOP                  = -517,
    AT_CMD_ERR_WIFI_CLI_PSCAN_CMAX_GET              = -518,
    AT_CMD_ERR_WIFI_CLI_PSCAN_CMAX_SET              = -519,
    AT_CMD_ERR_WIFI_CLI_PSCAN_CMIN_GET              = -520,
    AT_CMD_ERR_WIFI_CLI_PSCAN_CMIN_SET              = -521,

    AT_CMD_ERR_WIFI_CLI_SOFTAP_START                = -522,        // cli ap start
    AT_CMD_ERR_WIFI_CLI_SOFTAP_STOP                 = -523,        // cli ap stop
    AT_CMD_ERR_WIFI_CLI_SOFTAP_RESTART              = -524,        // cli ap restart

#if defined ( __SUPPORT_P2P__ )
    AT_CMD_ERR_WIFI_CLI_P2P_FIND                    = -530,        // cli p2p_find
    AT_CMD_ERR_WIFI_CLI_P2P_FIND_STOP               = -531,        // cli p2p_stop_find
    AT_CMD_ERR_WIFI_CLI_P2P_JOIN                    = -532,        // cli p2p_connect pbc <join>
    AT_CMD_ERR_WIFI_CLI_P2P_GRP_REMOVE              = -533,        // cli p2p_group_remove
    AT_CMD_ERR_WIFI_CLI_P2P_PEER_INFO               = -534,        // cli p2p_peers
    AT_CMD_ERR_WIFI_CLI_P2P_LISTEN_CH_TYPE          = -535,
    AT_CMD_ERR_WIFI_CLI_P2P_LISTEN_CH_RANGE         = -536,
#endif    // __SUPPORT_P2P__


    /// AT-CMD Network function
    AT_CMD_ERR_NW_NET_IF_NOT_INITIALIZE             = -600,
    AT_CMD_ERR_NW_NET_IF_IS_DOWN                    = -601,
    AT_CMD_ERR_NW_IP_IFACE_TYPE                     = -602,
    AT_CMD_ERR_NW_IP_IFACE_RANGE                    = -603,
    AT_CMD_ERR_NW_IP_ADDR_CLASS                     = -604,
    AT_CMD_ERR_NW_IP_INVALID_ADDR                   = -605,
    AT_CMD_ERR_NW_IP_NETMASK                        = -606,
    AT_CMD_ERR_NW_IP_GATEWAY                        = -607,
    AT_CMD_ERR_NW_DNS_A_QUERY_FAIL                  = -608,
    AT_CMD_ERR_NW_PING_IFACE_ARG_TYPE               = -609,
    AT_CMD_ERR_NW_PING_IFACE_ARG_RANGE              = -610,
    AT_CMD_ERR_NW_PING_DST_ADDR                     = -611,
    AT_CMD_ERR_NW_PING_TX_COUNT                     = -612,

    /// For DHCP-Client
    AT_CMD_ERR_NW_DHCPC_START_FAIL                  = -613,
    AT_CMD_ERR_NW_DHCPC_HOSTNAME_LEN                = -614,
    AT_CMD_ERR_NW_DHCPC_HOSTNAME_TYPE               = -615,

    /// For DHCP-Server
    AT_CMD_ERR_NW_DHCPS_START_ADDR_NOT_EXIST        = -616,
    AT_CMD_ERR_NW_DHCPS_END_ADDR_NOT_EXIST          = -617,
    AT_CMD_ERR_NW_DHCPS_WRONG_START_IP_CLASS        = -618,
    AT_CMD_ERR_NW_DHCPS_WRONG_END_IP_CLASS          = -619,
    AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH       = -620,
    AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_OVERFLOW       = -621,
    AT_CMD_ERR_NW_DHCPS_NO_CONNECTED_CLIENT         = -622,
    AT_CMD_ERR_NW_DHCPS_RUN_FLAG_TYPE               = -623,
    AT_CMD_ERR_NW_DHCPS_RUN_FLAG_VAL                = -624,
    AT_CMD_ERR_NW_DHCPS_LEASE_TIME_TYPE             = -625,
    AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE            = -626,
#if defined ( __SUPPORT_MESH__ )
    AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_NOT_EXIST      = -627,
    AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_CLASS          = -628,
#endif    // __SUPPORT_MESH__

    /// For SNTP Client
    AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED                = -629,
    AT_CMD_ERR_NW_SNTP_FLAG_TYPE                    = -630,
    AT_CMD_ERR_NW_SNTP_FLAG_VAL                     = -631,
    AT_CMD_ERR_NW_SNTP_PERIOD_TYPE                  = -632,
    AT_CMD_ERR_NW_SNTP_PERIOD_RANGE                 = -633,

    /// For MQTT-Client
    AT_CMD_ERR_NW_MQTT_NOT_CONNECTED                = -634,
    AT_CMD_ERR_NW_MQTT_NEED_TO_STOP                 = -635,
    AT_CMD_ERR_NW_MQTT_UNKNOWN_OP_ID                = -636,
    AT_CMD_ERR_NW_MQTT_CLIENT_TASK_START            = -637,

    // MQTT Broker setting
    AT_CMD_ERR_NW_MQTT_BROKER_NAME_NOT_FOUND        = -638,
    AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_TYPE         = -639,
    AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_RANGE        = -640,
    AT_CMD_ERR_NW_MQTT_BROKER_NAME_LEN              = -641,

    // MQTT TLS setting
    AT_CMD_ERR_NW_MQTT_TLS_TYPE                     = -642,
    AT_CMD_ERR_NW_MQTT_TLS_RANGE                    = -643,
    AT_CMD_ERR_NW_MQTT_TLS_ALPN_NOT_EXIST           = -644,
    AT_CMD_ERR_NW_MQTT_TLS_ALPN_COUNT_TYPE          = -645,
    AT_CMD_ERR_NW_MQTT_TLS_ALPN_COUNT_RANGE         = -646,
    AT_CMD_ERR_NW_MQTT_TLS_ALPN_NAME_LEN            = -647,
    AT_CMD_ERR_NW_MQTT_TLS_SNI_NOT_EXIST            = -648,
    AT_CMD_ERR_NW_MQTT_TLS_SNI_LEN                  = -649,
    AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NUM_NOT_EXIST     = -650,
    AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NOT_EXIST         = -651,
    AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NUM_NVRAM_WR      = -652,
    AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NVRAM_WR          = -653,

    // MQTT Subscribe-topic
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NOT_EXIST         = -654,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_TYPE          = -655,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_RANGE         = -656,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_LEN               = -657,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_DUP               = -658,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_NVRAM_WR      = -659,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_OVERFLOW      = -660,
    AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_ALREADY_EXIST     = -661,

    // MQTT Publisher-topic
    AT_CMD_ERR_NW_MQTT_PUB_TOPIC_NOT_EXIST          = -662,
    AT_CMD_ERR_NW_MQTT_PUB_TOPIC_LEN                = -663,

    // MQTT Will message
    AT_CMD_ERR_NW_MQTT_WILL_TOPIC_NOT_EXIST         = -664,
    AT_CMD_ERR_NW_MQTT_WILL_MESSAGE_NOT_EXIST       = -665,
    AT_CMD_ERR_NW_MQTT_WILL_TOPIC_LEN               = -666,
    AT_CMD_ERR_NW_MQTT_WILL_MESSAGE_LEN             = -667,
    AT_CMD_ERR_NW_MQTT_WILL_QOS_TYPE                = -668,
    AT_CMD_ERR_NW_MQTT_WILL_QOS_RANGE               = -669,

    // MQTT Common
    AT_CMD_ERR_NW_MQTT_PROTOCOL                     = -670,
    AT_CMD_ERR_NW_MQTT_PING_PERIOD_TYPE             = -671,
    AT_CMD_ERR_NW_MQTT_PING_PERIOD_RANGE            = -672,
    AT_CMD_ERR_NW_MQTT_USERNAME_NOT_EXIST           = -673,
    AT_CMD_ERR_NW_MQTT_USERNAME_LEN                 = -674,
    AT_CMD_ERR_NW_MQTT_PASSWORD_LEN                 = -675,
    AT_CMD_ERR_NW_MQTT_PUB_MESSAGE_LEN              = -676,
    AT_CMD_ERR_NW_MQTT_PUB_TX_IN_PROGRESS           = -677,

    /// For HTTP(s)
    AT_CMD_ERR_NW_HTS_TASK_CREATE_FAIL              = -680,
    AT_CMD_ERR_NW_HTSS_TASK_CREATE_FAIL             = -681,
    AT_CMD_ERR_NW_HTC_TASK_CREATE_FAIL              = -682,
    AT_CMD_ERR_NW_HTC_ALPN_CNT_TYPE                 = -683,
    AT_CMD_ERR_NW_HTC_ALPN_CNT_RANGE                = -684,
    AT_CMD_ERR_NW_HTC_ALPN1_STR_LEN                 = -685,
    AT_CMD_ERR_NW_HTC_ALPN2_STR_LEN                 = -686,
    AT_CMD_ERR_NW_HTC_ALPN3_STR_LEN                 = -687,
    AT_CMD_ERR_NW_HTC_SNI_LEN                       = -688,

    /// For Web-Socket client
    AT_CMD_ERR_NW_WSC_URL_STR_LEN                   = -689,
    AT_CMD_ERR_NW_WSC_INVALID_URL                   = -690,
    AT_CMD_ERR_NW_WSC_TASK_ALREADY_EXIST            = -691,
    AT_CMD_ERR_NW_WSC_CB_FUNC_DOES_NO_EXIST         = -692,
    AT_CMD_ERR_NW_WSC_INVALID_STATE                 = -693,
    AT_CMD_ERR_NW_WSC_TASK_CREATE_FAIL              = -694,
    AT_CMD_ERR_NW_WSC_CLOSE_FAIL                    = -695,
    AT_CMD_ERR_NW_WSC_SESS_NOT_CONNECTED            = -696,
    AT_CMD_ERR_NW_WSC_UNKNOW_CMD                    = -697,

	/// For OTA update
    AT_CMD_ERR_NW_OTA_WRONG_FW_TYPE                 = -700,
    AT_CMD_ERR_NW_OTA_DOWN_OK_AND_WAIT_RENEW        = -701,
    AT_CMD_ERR_NW_OTA_FLASH_READ_SIZE_TYPE          = -702,
    AT_CMD_ERR_NW_OTA_FLASH_COPY_SIZE_TYPE          = -703,
    AT_CMD_ERR_NW_OTA_FLASH_ERASE_SIZE_TYPE         = -704,
    AT_CMD_ERR_NW_OTA_BY_MCU_INIT                   = -705,
    AT_CMD_ERR_NW_OTA_SET_TLS_AUTH_MODE_NVRAM       = -706,
    AT_CMD_ERR_NW_OTA_SET_MCU_FW_NAME               = -707,

    /// For ZeroConfig
    AT_CMD_ERR_NW_MDNS_WRONG_FLAG                   = -710,
    AT_CMD_ERR_NW_MDNS_WRONG_MODE                   = -711,
    AT_CMD_ERR_NW_MDNS_NOT_RUNNING                  = -712,
    AT_CMD_ERR_NW_MDNS_ALREADY_RUN                  = -713,
    AT_CMD_ERR_NW_MDNS_IN_PROCESS                   = -714,
    AT_CMD_ERR_NW_MDNS_UNKNOWN_FAULT                = -715,
    AT_CMD_ERR_NW_MDNS_START_RUN_MODE_VAL           = -716,
    AT_CMD_ERR_NW_MDNS_SOCKET_FAIL                  = -717,
    AT_CMD_ERR_NW_DNS_SD_NOT_RUNNING                = -718,
    AT_CMD_ERR_NW_DNS_SD_ALREADY_RUN                = -719,
    AT_CMD_ERR_NW_DNS_SD_IN_PROCESS                 = -720,
    AT_CMD_ERR_NW_DNS_SD_SVC_CREATE_FAIL            = -721,
    AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS                 = -722,
    AT_CMD_ERR_NW_DNS_SD_SVC_INST_NAME_NVRAM_WR     = -723,
    AT_CMD_ERR_NW_DNS_SD_SVC_PROTOCOL_NVRAM_WR      = -724,
    AT_CMD_ERR_NW_DNS_SD_SVC_PORT_NO_NVRAM_WR       = -725,
    AT_CMD_ERR_NW_DNS_SD_SVC_TEXT_NVRAM_WR          = -726,


    /// AT-CMD transport function
    AT_CMD_ERR_TCP_SERVER_LOCAL_PORT_TYPE           = -730,
    AT_CMD_ERR_TCP_SERVER_MAX_PEER_TYPE             = -731,
    AT_CMD_ERR_TCP_SERVER_TASK_CREATE               = -732,
    AT_CMD_ERR_TCP_CLIENT_SVR_PORT_TYPE             = -733,
    AT_CMD_ERR_TCP_CLIENT_LOCAL_PORT_TYPE           = -734,
    AT_CMD_ERR_TCP_CLIENT_MAX_PEER_TYPE             = -735,
    AT_CMD_ERR_TCP_CLIENT_TASK_CREATE               = -736,
    AT_CMD_ERR_UDP_SESS_LOCAL_PORT_TYPE             = -737,
    AT_CMD_ERR_UDP_SESS_LOCAL_PORT_RANGE            = -738,
    AT_CMD_ERR_UDP_SESS_TASK_CREATE                 = -739,
    AT_CMD_ERR_UDP_CID2_SESS_NOT_EXIST              = -740,
    AT_CMD_ERR_UDP_CID2_SESS_ALREADY_EXIST          = -741,
    AT_CMD_ERR_UDP_CID2_SESS_INFO                   = -742,
    AT_CMD_ERR_UDP_CID2_REMOTE_PORT_TYPE            = -743,
    AT_CMD_ERR_NO_CONNECTED_SESSION_EXIST           = -744,
    AT_CMD_ERR_NO_FOUND_REQ_CID_SESSION             = -745,
    AT_CMD_ERR_CONTEXT_CID_TYPE                     = -746,
    AT_CMD_ERR_CONTEXT_DELETE                       = -747,
    AT_CMD_ERR_CONTEXT_TYPE_IS_NOT_TCP_SVR          = -748,
    AT_CMD_ERR_CONTEXT_INVALID_SESS_TYPE            = -749,
    AT_CMD_ERR_TRTRM_CID_TYPE                       = -750,
    AT_CMD_ERR_TRTRM_REMOTE_PORT_NUM_TYPE           = -751,
    AT_CMD_ERR_TRTRM_TCP_SVR_REMOTE_SESS_DISCON     = -752,
    AT_CMD_ERR_TCP_SERVER_TERMINATE                 = -753,
    AT_CMD_ERR_TCP_CLIENT_TERMINATE                 = -754,
    AT_CMD_ERR_UDP_SESSION_TERMINATE                = -755,
    AT_CMD_ERR_MULTI_SESSION_CID_TERMINATE          = -756,
    AT_CMD_ERR_NO_SESSION_TO_SAVE_NVRAM             = -757,


    /// AT-CMD SSL/TLS function
    AT_CMD_ERR_SSL_ROLE_NOT_SUPPORT                 = -760,
    AT_CMD_ERR_SSL_CONF_CID_TYPE                    = -761,
    AT_CMD_ERR_SSL_CONTEXT_NOT_FOUND                = -762,
    AT_CMD_ERR_SSL_CONTEXT_ALREADY_EXIST            = -763,
    AT_CMD_ERR_SSL_CONF_ID_NOT_SUPPORTED            = -764,
    AT_CMD_ERR_SSL_SAVE_CLR_ALL_NV                  = -765,
    AT_CMD_ERR_SSL_SAVE_FAIL_NV                     = -766,

    /// AT-CMD SSL Configuration
    AT_CMD_ERR_SSL_CONF_ID_TYPE                     = -767,
    AT_CMD_ERR_SSL_CONF_ID_RANGE                    = -768,
    AT_CMD_ERR_SSL_CONF_CID_CA_CERT                 = -769,
    AT_CMD_ERR_SSL_CONF_CID_CERT                    = -770,
    AT_CMD_ERR_SSL_CONF_CID_SNI                     = -771,
    AT_CMD_ERR_SSL_CONF_CID_SVR_VALID_TYPE          = -772,
    AT_CMD_ERR_SSL_CONF_CID_SVR_VALID_RANGE         = -773,
    AT_CMD_ERR_SSL_CONF_CID_RX_BUF_LEN              = -774,
    AT_CMD_ERR_SSL_CONF_CID_TX_BUF_LEN              = -775,
    AT_CMD_ERR_SSL_CONN_CID_TYPE                    = -776,
    AT_CMD_ERR_SSL_CONN_ALREADY_CONNECTED           = -777,
    AT_CMD_ERR_SSL_CONN_PORT_NUM_TYPE               = -778,
    AT_CMD_ERR_SSL_CONN_UNKNOWN_HOSTNAME            = -779,
    AT_CMD_ERR_SSL_CONN_CFG_SETUP_FAIL              = -780,
    AT_CMD_ERR_SSL_CONN_TLS_CLINET_RUN_FAIL         = -781,

    /// AT-CMD SSL Certificate
    AT_CMD_ERR_SSL_CERT_TYPE                        = -782,
    AT_CMD_ERR_SSL_CERT_RANGE                       = -783,
    AT_CMD_ERR_SSL_CERT_STO_SEQ_TYPE                = -784,
    AT_CMD_ERR_SSL_CERT_STO_SEQ_RANGE               = -785,
    AT_CMD_ERR_SSL_CERT_STO_FORMAT_TYPE             = -786,
    AT_CMD_ERR_SSL_CERT_STO_FORMAT_RANGE            = -787,
    AT_CMD_ERR_SSL_CERT_STO_ALREADY_EXIST           = -788,
    AT_CMD_ERR_SSL_CERT_STO_NO_SPACE                = -789,
    AT_CMD_ERR_SSL_CERT_DEL_LIST_NOT_FOUND          = -790,
    AT_CMD_ERR_SSL_CERT_MODULE                      = -791,
    AT_CMD_ERR_SSL_CERT_FORMAT                      = -792,
    AT_CMD_ERR_SSL_CERT_LENGTH                      = -793,
    AT_CMD_ERR_SSL_CERT_FLASH_ADDR                  = -794,
    AT_CMD_ERR_SSL_CERT_EMPTY_CERT                  = -795,
    AT_CMD_ERR_SSL_CERT_INTERNAL                    = -796,


    /// Reserver error-code
    AT_CMD_ERR_RESERVED_XXX                         = -800,

    /// AT-CMD BLE function
    AT_CMD_ERR_BLE_XXX                              = -880,

    /// AT-CMD Provision function
    AT_CMD_ERR_PROV_XXX                             = -890,

    /// AT-CMD RF function
    AT_CMD_ERR_RF_XXX                               = -900,

    /// AT-CMD Peripheral function
    AT_CMD_ERR_PERI_XXX                             = -950,

    /// Unknown error
    AT_CMD_ERR_UNKNOWN                              = -999

} atcmd_error_code;

#if !defined( __SUPPORT_ATCMD_MULTI_SESSION__ )
typedef struct uart2wlan_tx_buffer {
    char    data[ATCMD_DATA_BUF_SIZE];
    int     data_len;
} uart2wlan_tx_buf_t;

typedef struct uart2wlan_tx_msg {
    uart2wlan_tx_buf_t *uart2wlan_tx_buf;
} uart2wlan_tx_msg_t;

typedef struct _atcmd_dpm_info {
    char    sess_name[32];
    char    ip_addr[16];
    unsigned long   local_port;
    unsigned long   remote_port;
} atcmd_dpm_info_t;
#endif // !__SUPPORT_ATCMD_MULTI_SESSION__


/**************************************************************************
 * Internal Functions
 **************************************************************************/
atcmd_error_code command_parser(char *line);
atcmd_error_code esc_cmd_parser(char *line);


/*
 **************************************************************************
 * Global Functions
 **************************************************************************
 */

/**
 ****************************************************************************************
 * @brief  Standard AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - Command list\n
 * - Factory Reset\n
 * - Echo\n
 * - Baud Rate\n
 ****************************************************************************************
 */
int atcmd_standard(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief  Basic AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - Reboot, Booting Index\n
 * - Time\n
 * - Version\n
 ****************************************************************************************
 */
int	atcmd_basic(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief DPM AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - DPM on/off\n
 * - DPM Parameters (DTIM, Keep-alive period, Wake-up count)\n
 ****************************************************************************************
 */
int	atcmd_dpm(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief  UART Bardrate configuration AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 ****************************************************************************************
 */
int atcmd_uartconf(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Wi-Fi AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - STA\n
 * - Soft-AP\n
 * - Wi-Fi Direct (if defined)\n
 * - IEEE802.11i WPA-Enterprise (if defined)\n
 ****************************************************************************************
 */
int	atcmd_wifi(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Network AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - IP address set/get, DNS, Ping\n
 * - DHCP Server/Client\n
 * - SNTP Client\n
 * - MQTT (if defined)
 * - TLS Setting
 ****************************************************************************************
 */
int	atcmd_network(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief TCP/UDP AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - TCP Server socket open/close\n
 * - TCP Client socket open/close\n
 * - UDP socket open/close\n
 * - Sending TCP/UDP messages\n
 ****************************************************************************************
 */
int	atcmd_transport(int argc, char *argv[]);

#if defined (__SUPPORT_ATCMD_TLS__)
/**
 ****************************************************************************************
 * @brief TLS AT Commands
 * @param[in] argc argument count
 * @param[in] argv[] argument vector
 * @return  ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @susection Command types
 *  - Open and close TLS client session\n
 *  - Transfer message\n
 ****************************************************************************************
 */
int atcmd_transport_ssl(int argc, char *argv[]);
#endif // (__SUPPORT_ATCMD_TLS__)

/**
 ****************************************************************************************
 * @brief Watchdog Service AT Commands
 * @param[in] argc argument count
 * @param[in] argv[] argument vector
 *
 * @return AT_CMD_ERR_CMD_OK if succeed to process, ERROR_CODE else
 ****************************************************************************************
 */
int atcmd_wdog_service(int argc, char *argv[]);


/**
 ****************************************************************************************
 * @brief Start AT-CMD task
 ****************************************************************************************
 */
void start_atcmd(void);

/**
 ****************************************************************************************
 * @brief Forward the event regisered in atcmd
 * @param[in] index Event index
 * @param[in] buf more information
 ****************************************************************************************
 */
void atcmd_asynchony_event(int index, char *buf);

/**
 ****************************************************************************************
 * @brief Forward MQTT PUBLISH received to atcmd (for MQTT module)
 * @param[in] index Event index
 * @param[in] buf1 PUBLISH message
 * @param[in] buf2 Topic
 * @param[in] len PUBLISH message length
 ****************************************************************************************
 */
void atcmd_asynchony_event_for_mqttmsgrecv(int index, char *buf1, char *buf2, int len);

/**
 ****************************************************************************************
 * @brief Get DPM wakeup state value for AT-CMD
 * @return Wakeup type character string.
 * @subsection DPM wakeup types
 *     - "UC"         Wakeup by unicast data receiving
 *     - "NOBCN"      Wakeup by no-beacon
 *     - "NOACK"      Wakeup by Wi-Fi no-ack 
 *     - "DEAUTH"     Wakeup by receiving Wi-Fi connection deauthentication frame
 *     - "RTC"        Wakeup by registered RTC Timer
 *     - "EXT"        Wakeup by External-wakeup GPIO triggring
 *     - "UNDEFINED"
 ****************************************************************************************
 */
char *atcmd_dpm_wakeup_status(void);

/**
 ****************************************************************************************
 * @brief  Set status flag whether "+INIT:DONE,1" (Soft-AP) message was sent.
 * @param[in]  flag pdTRUE:"+INIT:DONE,1" message was sent, pdFALSE: Not sent yet.
 * @return     None
 ****************************************************************************************
 */
void set_INIT_DONE_msg_to_MCU_on_SoftAP_mode(int flag);


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__) ////////////////////////

/// atcmd provisionning status get from wifi action result
typedef enum _atcmd_provision_stat {
    /// Provisioning not working or finish
    ATCMD_PROVISION_IDLE                            = 0,

    /// Provisioning start
    ATCMD_PROVISION_START                           = 1,

    /// Received AP info from mobile 
    ATCMD_PROVISION_SELECTED_AP_SUCCESS             = 101,
    ATCMD_PROVISION_SELECTED_AP_FAIL                = 102,

    /// AP connection fail check SSID or PW
    ATCMD_PROVISION_WRONG_PW                        = 103,    // New add
    ATCMD_PROVISION_AP_FAIL                         = 105,

    /// Network checking status 
    ATCMD_PROVISION_DNS_FAIL_SERVER_FAIL            = 106,
    ATCMD_PROVISION_DNS_FAIL_SERVER_OK              = 107,    // in case wrong DNS name
    ATCMD_PROVISION_NO_URL_PING_FAIL                = 108,
    ATCMD_PROVISION_NO_URL_PING_OK                  = 109,
    ATCMD_PROVISION_DNS_OK_PING_FAIL_N_SERVER_OK    = 110,    // in case AP gives wrong IP
    ATCMD_PROVISION_DNS_OK_PING_OK                  = 111,
    ATCMD_PROVISION_REBOOT_ACK                      = 112,
    ATCMD_PROVISION_DNS_OK_PING_N_SERVER_FAIL       = 113     // in case default ping fail
   
} atcmd_provision_stat;

/**
 ****************************************************************************************
 * @brief Provisioning start AT Commands
 * @return ERR_CMD_OK if succeed to process, ERROR_CODE else
 * @subsection Command types
 * - START : Factory Reset and Start Provisioning
 * - STAT : Request Last Provisioning Status
 ****************************************************************************************
 */
int atcmd_app_provision(int argc, char *argv[]);

/**
 ****************************************************************************************
 * @brief Send Provisioning status to MCU
 * @param[in] status - provisioning status values
 ****************************************************************************************
 */
void atcmd_provstat(int status);

#endif     // __PROVISION_ATCMD__    /////////////////////////////////////////////////////////

/**************************************************************************
 * Global external Functions
 **************************************************************************/
extern void PRINTF_ATCMD(const char *fmt,...);
extern void PUTS_ATCMD(char *data, int data_len);
extern void PRINTF_ESCCMD(unsigned int result_code);
extern void da16x_environ_lock(unsigned int flag);
extern void flash_image_close(HANDLE handler);
extern unsigned int get_wlaninit(void);
extern int  ip_change(unsigned int iface, char *ipaddress, char *netmask, char *gateway, unsigned char save);
extern unsigned int flash_image_read(HANDLE handler, unsigned int img_offset, void *load_addr, unsigned int img_secsize);
extern void set_wlaninit(unsigned int flag);
extern unsigned int da16x_ping_client(int iface,
                                    char *domain_str,
                                    unsigned long ipaddr,
                                    unsigned long *ipv6dst,
                                    unsigned long *ipv6src,
                                    int len,
                                    unsigned long count,
                                    int wait,
                                    int interval,
                                    int nodisplay,
                                    char *ping_result);

extern int  cmd_set_time(char *date_format, char *time_format, int daylight);
extern UCHAR set_boot_index(int index);
extern void cmd_certificate(int argc, char *argv[]);

extern void combo_ble_sw_reset(void);
extern void clear_wakeup_src_all(void);
extern void user_interface_init(void);
extern unsigned long da16x_get_wakeup_source(void);
extern void force_dpm_abnormal_sleep_by_wifi_conn_fail(void);
extern void dpm_abnormal_chk_hold(void);
extern void dpm_abnormal_chk_resume(void);
extern UINT factory_reset_default(int reboot_flag);

#if defined ( __BLE_COMBO_REF__ )
extern int combo_set_sleep2(char* context, int rtm, int seconds);
#endif // __BLE_COMBO_REF__

#if defined ( __SUPPORT_MQTT__ )
extern int mqtt_client_get_qos(void);
#endif	// __SUPPORT_MQTT__

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
extern int factory_reset_btn_onetouch(void);
#endif // __SUPPORT_WIFI_CONCURRENT__

extern atcmd_error_code user_atcmd_parser(char *line);
extern void reg_user_atcmd_help_cb(void);
#if defined ( __TRIGGER_DPM_MCU_WAKEUP__ )
extern void trigger_mcu_wakeup_gpio(void );
#endif // __TRIGGER_DPM_MCU_WAKEUP__
extern size_t printf_decode(u8 *buf, size_t maxlen, const char *str);
#if defined (__SUPPORT_RSSI_CMD__)
extern int    get_current_rssi(int iface);
#endif // __SUPPORT_RSSI_CMD__
extern void mqtt_client_save_to_dpm_user_mem(void);
#if defined (__SUPPORT_TCP_RECVDATA_HEX_MODE__)
extern void set_tcpc_data_mode(int mode);
extern void set_tcps_data_mode(int mode);
#endif // __SUPPORT_TCP_RECVDATA_HEX_MODE__
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
extern int is_disconn_by_too_long_msg_rx_in_cs0(void);
extern void set_disconn_by_too_long_msg_rx_in_cs0(int onff);
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__

#if defined (__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
extern void printf_with_run_time(char *string);
#endif // (__RUNTIME_CALCULATION__)

extern atcmd_error_code command_parser(char *line);
extern atcmd_error_code user_atcmd_parser(char *line);
extern atcmd_error_code esc_cmd_parser(char *line);


/**************************************************************************
 * Global External Variables
 **************************************************************************/
extern HANDLE uart1;
#if defined ( __ATCMD_IF_UART2__ )
extern HANDLE uart2;
#endif // __ATCMD_IF_UART2__
extern int wifi_conn_fail_reason_code;
extern int wifi_disconn_reason_code;
extern int wifi_conn_fail_reason_code;

extern UCHAR atcmd_esc_data_echo_flag;
extern UCHAR atcmd_esc_data_dump_flag;
extern int   sntp_name_idx;
extern mqttParamForRtm mqttParams;

#endif    // __ATCMD_H__

/* EOF */
