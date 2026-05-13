/**
 ****************************************************************************************
 *
 * @file atcmd.c
 *
 * @brief AT-Command Handler
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
#if 0 //azure_check[[::
//#include "sdk_type.h"
#include "custom_config_sdk.h"
#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#include "atcmd_tcp_server.h"
#include "atcmd_tcp_client.h"
#include "atcmd_udp_session.h"
#include "atcmd_transfer_mng.h"
#include "da16x_cert.h"
#include "da16x_sys_watchdog.h"

#include "da16x_compile_opt.h"
#pragma GCC diagnostic ignored "-Wrestrict"



#if defined ( __SUPPORT_DPM_EXT_WU_MON__ )
#define DELAY_TICK_TO_RCV_AT_CMD           20    // 200 msec
#endif // __SUPPORT_DPM_EXT_WU_MON__

#define ATCMD_UART_CONF_NAME               "ATCMD_UART_CONF"
#define NVR_KEY_ATCMD_UART_BAUDRATE        "ATCMD_BAUDRATE"
#define NVR_KEY_ATCMD_UART_BITS            "ATCMD_BITS"
#define NVR_KEY_ATCMD_UART_PARITY          "ATCMD_PARITY"
#define NVR_KEY_ATCMD_UART_STOPBIT         "ATCMD_STOPBIT"
#define NVR_KEY_ATCMD_UART_FLOWCTRL        "ATCMD_FLOWCTRL"


/**************************************************************************
 * Internal Variables
 **************************************************************************/
#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static char *atcmd_uart_mon_buf;
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__
static char atcmd_mac_table[6][18] = {0, };
static int  atcmd_uart_echo = pdFALSE;
static int  is_waiting_for_wf_jap_result_in_progress = FALSE;

#if defined ( __SUPPORT_OTA__ )
static OTA_UPDATE_CONFIG _atcmd_ota_conf = { 0, };
static OTA_UPDATE_CONFIG *atcmd_ota_conf = (OTA_UPDATE_CONFIG *) &_atcmd_ota_conf;
#endif // __SUPPORT_OTA__

typedef struct _list_node {
    void *pnode;
    struct _list_node *pnext;
} list_node;



/**************************************************************************
 * Global Variables
 **************************************************************************/
TaskHandle_t      atcmd_task_handler = NULL;
const command_t   *at_command_table;

int  atq_result = 1;
int  atcmd_dpm_ready = pdFALSE;
int  g_is_atcmd_init = 0;

/*
 *  TRUE : 1) at boot (POR/Wakeup), 2) no +WFDAP:0 is sent,
 *         3) no +WFJAP:0 is sent,  4) +WFJAP:1 is sent
 *  FALSE: 5) +WFDAP:0 is sent,     6) +WFJAP:0 is sent
 */
int atcmd_wfdap_condition_resolved = TRUE;


#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
// It's for recovering session
typedef struct _atcmd_sess_info {
    atcmd_sess_type type;
    union {
        atcmd_tcps_sess_info tcps;
        atcmd_tcpc_sess_info tcpc;
        atcmd_udps_sess_info udps;
    } sess;
} atcmd_sess_info;

typedef struct _atcmd_sess_context {
    int cid;
    atcmd_sess_type type;
    union {
        void *ptr;
        atcmd_tcps_context *tcps;
        atcmd_tcpc_context *tcpc;
        atcmd_udps_context *udps;
    } ctx;
    union {
        atcmd_tcps_config tcps;
        atcmd_tcpc_config tcpc;
        atcmd_udps_config udps;
    } conf;
    struct _atcmd_sess_context *next;
} atcmd_sess_context;

unsigned int atcmd_sess_max_session = ATCMD_NW_TR_MAX_SESSION_CNT_DPM;
unsigned int atcmd_sess_max_cid = (ATCMD_NW_TR_MAX_SESSION_CNT_DPM + 1); // + Reserved CID(0,1,2)
atcmd_sess_info *atcmd_sess_info_header = NULL;
atcmd_sess_context *atcmd_sess_ctx_header = NULL;

const TickType_t atcmd_sess_ctx_mutex_timeout = portMAX_DELAY;
SemaphoreHandle_t atcmd_sess_ctx_mutex = NULL;

#else

atcmd_tcps_config *atcmd_tcps_conf = NULL;
atcmd_tcps_context atcmd_tcps_ctx;

atcmd_tcpc_config *atcmd_tcpc_conf = NULL;
atcmd_tcpc_context atcmd_tcpc_ctx;

atcmd_udps_config *atcmd_udps_conf = NULL;
atcmd_udps_context atcmd_udps_ctx;

atcmd_dpm_info_t  *atcmd_dpm_tcpc_info = { NULL, };
atcmd_dpm_info_t  *atcmd_dpm_udp_info = { NULL, };
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

UCHAR atcmd_dpm_tcps_ka_tid = 0;
int  init_done_sent = pdFALSE;

#if defined ( __SUPPORT_NOTIFY_RTC_WAKEUP__ )
int    rtc_wakeup_intr_chk_flag = pdFALSE;
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__

#if defined ( __ENABLE_TRANSFER_MNG__ )
atcmd_tr_mng_context atcmd_tr_mng_ctx;
atcmd_tr_mng_config atcmd_tr_mng_conf;
#endif // __ENABLE_TRANSFER_MNG__

/* For User AT-CMD help */
atcmd_error_code    (* user_atcmd_help_cb_fn)(char *cmd) = NULL;


#if defined ( __SUPPORT_ATCMD_TLS__ )
typedef enum _atcmd_tls_role {
    ATCMD_TLS_NONE   = -1,
    ATCMD_TLS_SERVER = 0,
    ATCMD_TLS_CLIENT = 1,
} atcmd_tls_role;

typedef struct _atcmd_tls_context {
    int cid;
    atcmd_tls_role role;

    union {
        atcmd_tlsc_context *tlsc_ctx;
    } ctx;

    union {
        atcmd_tlsc_config tlsc_conf;
    } conf;

    struct _atcmd_tls_context *next;
} atcmd_tls_context;

atcmd_tls_context *atcmd_tls_ctx_header = NULL;
atcmd_tls_context *atcmd_tls_ctx_rtm_header = NULL;

#endif // __SUPPORT_ATCMD_TLS__

const char esc_cmd_s_upper_case = 'S';
const char esc_cmd_s_lower_case = 's';
const char esc_cmd_m_upper_case = 'M';
const char esc_cmd_m_lower_case = 'm';
const char esc_cmd_h_upper_case = 'H';
const char esc_cmd_h_lower_case = 'h';
const char *esc_cmd_cert_case = "CERT";

/**************************************************************************
 * Global Functions
 **************************************************************************/
char *atcmd_dpm_wakeup_status(void);


/**************************************************************************
 * Staic local Functions
 **************************************************************************/
static int  help_command(int argc, char *argv[]);
#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static void monitor_atcmd(void);
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static int  getchar_atcmd_uart(UINT32 mode);
static int  gets_atcmd_uart(char *buf);
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

static int  atcmd_chk_wifi_conn(void);
static void atcmd_rsp_wifi_conn(void);

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
static int  atcmd_transport_set_max_session(const unsigned int cnt);
static unsigned int atcmd_transport_get_max_session();
static unsigned int atcmd_transport_get_max_cid();
static atcmd_sess_context *atcmd_transport_alloc_context(const atcmd_sess_type type);
static atcmd_sess_context *atcmd_transport_create_context(const atcmd_sess_type type);
static atcmd_sess_context *atcmd_transport_find_context(int cid);
static void atcmd_transport_free_context(atcmd_sess_context **ctx);
static int atcmd_transport_delete_context(int cid);

static int atcmd_network_get_nvr_id(int cid, atcmd_sess_nvr_type type);
static void atcmd_network_display_sess_info(int cid, atcmd_sess_info *sess_info);
static int atcmd_network_save_tcps_context(atcmd_sess_context *ctx, int is_rtm);
static int atcmd_network_save_tcpc_context(atcmd_sess_context *ctx, int is_rtm);
static int atcmd_network_save_udps_context(atcmd_sess_context *ctx, int is_rtm);
static int atcmd_network_save_context_nvram(void);
static int atcmd_network_recover_context_nvram(atcmd_sess_info *sess_info, const int sess_info_cnt);
static int atcmd_network_get_sess_info_cnt();
static atcmd_sess_info *atcmd_network_get_sess_info(const int cid);
static int atcmd_network_clear_sess_info(int cid);
static int atcmd_network_delete_sess_info(void);
static int atcmd_network_recover_session(atcmd_sess_info *sess_info, const int sess_info_cnt);

static int atcmd_network_check_network_ready(const char *module, unsigned int timeout);

// TCP server
static int atcmd_network_connect_tcps(atcmd_sess_context *ctx, int port, int max_peer_cnt);
static int atcmd_network_disconnect_tcps(const int cid);
static int atcmd_network_disconnect_tcps_cli(const int cid, const char *ip_addr, const int port);
static int atcmd_network_display_tcps(const int cid, const char *prefix);

// TCP client
static int atcmd_network_connect_tcpc(atcmd_sess_context *ctx, char *svr_ip, int svr_port, int port);
static int atcmd_network_disconnect_tcpc(const int cid);
static int atcmd_network_display_tcpc(const int cid, const char *prefix);

// UDP session
static int atcmd_network_connect_udps(atcmd_sess_context *ctx, char *peer_ip, int peer_port, int port);
static int atcmd_network_disconnect_udps(const int cid);
static int atcmd_network_set_udps_peer_info(const int cid, char *ip, unsigned int port);
static int atcmd_network_display_udps(const int cid, const char *prefix);

static int atcmd_network_display(const int cid);
static int atcmd_network_terminate_session(const int cid);

#else ////////////////////////////////////

static int atcmd_network_check_network_ready(const char *module, unsigned int timeout);

// TCP server
static atcmd_tcps_config *atcmd_network_new_tcps_config(char *name);
static atcmd_tcps_config *atcmd_network_get_tcps_config(char *name);
static int atcmd_network_set_tcps_config(atcmd_tcps_config *config, int port);
static int atcmd_network_del_tcps_config(char *name, atcmd_tcps_config **config);
static int atcmd_network_connect_tcps(int port);
static int atcmd_network_disconnect_tcps(void);
static int atcmd_network_disconnect_tcps_cli(const char *ip_addr, const int port);
static int atcmd_network_display_tcps(const char *resp);

// TCP client
static atcmd_tcpc_config *atcmd_network_new_tcpc_config(char *name);
static atcmd_tcpc_config *atcmd_network_get_tcpc_config(char *name);
static int atcmd_network_set_tcpc_config(atcmd_tcpc_config *config, char *svr_ip, int svr_port, int port);
static int atcmd_network_del_tcpc_config(char *name, atcmd_tcpc_config **config);
static int atcmd_network_connect_tcpc(char *svr_ip, int svr_port, int port);
static int atcmd_network_disconnect_tcpc(void);
static int atcmd_network_display_tcpc(const char *resp);

// UDP session
static atcmd_udps_config *atcmd_network_new_udps_config(char *name);
static atcmd_udps_config *atcmd_network_get_udps_config(char *name);
static int atcmd_network_set_udps_config(atcmd_udps_config *config, char *peer_ip, int peer_port, int port);
static int atcmd_network_del_udps_config(char *name, atcmd_udps_config **config);
static int atcmd_network_connect_udps(char *peer_ip, int peer_port, int port);
static int atcmd_network_disconnect_udps(void);
static int atcmd_network_set_udps_peer_info(char *ip, unsigned int port);
static int atcmd_network_display_udps(const char *resp);

static int atcmd_network_display(int cid);
static int atcmd_network_save_cid(void);

#endif // __SUPPORT_ATCMD_MULTI_SESSION__

#if defined ( __SUPPORT_TCP_RECVDATA_HEX_MODE__ )
static int atcmd_tcp_data_mode(int argc, char *argv[]);
#endif // __SUPPORT_TCP_RECVDATA_HEX_MODE__

static void atcmd_sdk_version(void);

#if defined ( __SUPPORT_ATCMD_TLS__ )
static void atcmd_transport_ssl_reboot(void);
static atcmd_error_code command_parser_tls_wr(char *line);
static atcmd_error_code command_parser_tls_certstore(char *line);
#endif // __SUPPORT_ATCMD_TLS__


#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )

static void gets_tls_atcmd_uart(char *buf);
static int  atcmd_is_valid_ip4_addr_uart(char *str_ip);

#if defined ( __SUPPORT_ATCMD_TLS__ )
static int atcmd_transport_ssl_do_wr(char *cid_str, char *dst_ip_str, char *dst_port_str, char *datalen_str, char *data);
#endif // __SUPPORT_ATCMD_TLS__
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

static size_t str_decode(u8 *buf, size_t maxlen, const char *str);


/*
 * AT COMMANDs
 */
static const command_t commands[] = {
  /* cmd,                         function,     arg_count, format,                                                brief */
  { (char *)"?",                  help_command,         0, (char *) "",                                           "Commands list with brief and usage"                    },

  { (char *)"HELP",               help_command,         1, (char *) "<command>",                                  "Print help message."                                   },

  /* Standard AT Commands =============================================================== */
  { (char *)"AT",                 atcmd_standard,       0, (char *) "",                                           "Attention command"                                     },
  { (char *)"AT+",                atcmd_standard,       0, (char *) "",                                           "List available commands"                               },
  { (char *)"ATZ",                atcmd_standard,       0, (char *) "",                                           "AT command initialize"                                 },
  { (char *)"ATF",                atcmd_standard,       0, (char *) "",                                           "Restore to Factory mode (NVRAM clean)"                 },
  { (char *)"ATE",                atcmd_standard,       1, (char *) "[<?>]",                                      "Command echo"                                          },
  { (char *)"ATQ",                atcmd_standard,       1, (char *) "[<?>]",                                      "Result Codes On/Off"                                   },
#if !defined ( __USER_UART_CONFIG__ )
#if defined (__UART1_FLOW_CTRL_ON__)
  { (char *)"ATB",                atcmd_uartconf,       5, (char *) "<baudrate>[[,<databits>][,<parity>][,<stopbits>][,<flow_ctrl>]]", "Setting UART parameters"          },
#else  
  { (char *)"ATB",                atcmd_uartconf,       5, (char *) "<baudrate>[[,<databits>][,<parity>][,<stopbits>]]", "Setting UART parameters"                        },
#endif /* __UART1_FLOW_CTRL_ON__ */  
#endif // ! __USER_UART_CONFIG__

  /* Basic AT Commands ================================================================== */
  { (char *)"AT+RESTART",         atcmd_basic,          0, (char *) "",                                           "System Restart"                                        },
  { (char *)"AT+RESET",           atcmd_basic,          0, (char *) "",                                           "System Reset"                                          },
  { (char *)"AT+CHIPNAME",        atcmd_basic,          0, (char *) "",                                           "Chipset Name (DA16X)"                                  },
  { (char *)"AT+VER",             atcmd_basic,          0, (char *) "",                                           "FW Version info"                                       },
  { (char *)"AT+SDKVER",          atcmd_basic,          0, (char *) "",                                           "SDK Version info"                                      },
  { (char *)"AT+TIME",            atcmd_basic,          2, (char *) "<date>,<time>",                              "Set current time"                                      },
  { (char *)"AT+RLT",             atcmd_basic,          0, (char *) "",                                           "System Runtime Inquiry"                                },
  { (char *)"AT+TZONE",           atcmd_basic,          1, (char *) "<sec>",                                      "Set Timezone (GMT; -43200 ~ 43200)"                    },
  { (char *)"AT+DEFAP",           atcmd_basic,          0, (char *) "<boot>",                                     "Factory setting(AP mode) --- SSID:DA16200_XXXXXX, Auth:WPA2/CCMP, IP:10.0.0.1, Enable DHCP" },
  { (char *)"AT+BIDX",            atcmd_basic,          1, (char *) "<idx>",                                      "Booting Index (0|1)"                                   },
  { (char *)"AT+HOSTINITDONE",    atcmd_basic,          0, (char *) "",                                           "Customer MCU Init"                                     },

  /* DPM AT Commands =================================================================== */
  { (char *)"AT+DPM",             atcmd_dpm,            2, (char *) "<dpm>[,<nvm_only>]",                         "DPM On/Off"                                            },
  { (char *)"AT+DPMKA",           atcmd_dpm,            1, (char *) "<period>",                                   "Set DPM Keep-Alive Period (msec : 0 .. 600000)"        },
  { (char *)"AT+DPMTIMWU",        atcmd_dpm,            1, (char *) "<count>",                                    "Set DPM TIM Wakeup Time (DTIM count : 1 .. 6000)"     },
  { (char *)"AT+DPMUSERWU",       atcmd_dpm,            1, (char *) "<time>",                                     "Set DPM User Wakeup Time (msec : 0 .. 86400000)"           },
#if defined (__SUPPORT_DPM_EXT_WU_MON__)
  { (char *)"AT+SETDPMSLPEXT",    atcmd_dpm,            0, (char *) "",                                           "Set DPM Sleep for External Wakeup Resource"            },
  { (char *)"AT+CLRDPMSLPEXT",    atcmd_dpm,            0, (char *) "",                                           "Clear DPM Sleep for External Wakeup Resource"          },
#endif // (__SUPPORT_DPM_EXT_WU_MON__)
  { (char *)"AT+SETSLEEP2EXT",    atcmd_dpm,            2, (char *) "<duration>,<rtm>",                           "Enter Sleep2 (msec : 1000 .. 2,097,151,000)"                   },
  { (char *)"AT+SETSLEEP1EXT",    atcmd_dpm,            1, (char *) "<rtm>",                                      "Enter Sleep1" },
  { (char *)"AT+SETSLEEP3EXT",    atcmd_dpm,            1, (char *) "<duration>",                                 "Enter Sleep3 (msec : 1000 .. 2,097,151,000)" },  
#if defined ( __SUPPORT_FASTCONN__ )
  { (char *)"AT+GETFASTCONN",     atcmd_dpm,            0, (char *) "",                                           "Get Wi-Fi Fast Connection status"                      },
  { (char *)"AT+SETFASTCONN",     atcmd_dpm,            1, (char *) "<flag>",                                     "Set Wi-Fi Fast Connection Mode (mode: 0/1)"            },
#endif // __SUPPORT_FASTCONN__
  { (char *)"AT+MCUWUDONE",       atcmd_dpm,            0, (char *) "",                                           "Notify that MCU is ready."                             },
#if !defined ( __DISABLE_DPM_ABNORM__ )
  { (char *)"AT+DPMABN",          atcmd_dpm,            1, (char *) "<flag>",                                   "DPM Abnormal operation On/Off (0:Off, 1:On)"             },
#endif // !__DISABLE_DPM_ABNORM__
#if defined (__WF_CONN_RETRY_CNT_ABN_DPM__)
  { (char *)"AT+DPMABNWFCCNT",    atcmd_dpm,            1, (char *) "<count>",                                    "Set Wi-Fi Connection retry count in DPM abnormal sleep"},
#endif // __WF_CONN_RETRY_CNT_ABN_DPM__

  /* Wi-Fi Commands ==================================================================== */
  { (char *)"AT+WFMODE",          atcmd_wifi,           1, (char *) "<mode>",                                     "Set Wi-Fi Operation Mode"                              },
  { (char *)"AT+WFMAC",           atcmd_wifi,           1, (char *) "<mac>",                                      "Current MAC_addr Setting/Inquiry"                      },
  { (char *)"AT+WFSPF",           atcmd_wifi,           1, (char *) "<mac>",                                      "MAC Spoofing for Station (set only)"                   },
  { (char *)"AT+WFOTP",           atcmd_wifi,           1, (char *) "<mac>",                                      "OTP MAC (set only)"                                    },
  { (char *)"AT+WFSTAT",          atcmd_wifi,           0, (char *) "",                                           "Get WI-FI profile"                                     },
  { (char *)"AT+WFSTA",           atcmd_wifi,           0, (char *) "",                                           "Get STA status"                                        },
  { (char *)"AT+WFPBC",           atcmd_wifi,           0, (char *) "",                                           "Push WSC Button"                                       },
  { (char *)"AT+WFPIN",           atcmd_wifi,           1, (char *) "<pin>",                                      "Enter WSC PIN/Generate PIN"                            },
  { (char *)"AT+WFCWPS",          atcmd_wifi,           0, (char *) "",                                           "Stop WPS Operation"                                    },
  { (char *)"AT+WFCC",            atcmd_wifi,           1, (char *) "<code>",                                     "Set Country Code"                                      },

  { (char *)"AT+WFSCAN",          atcmd_wifi,           0, (char *) "",                                           "[STA] Get Scan Results"                                },
#if defined ( __SUPPORT_PASSIVE_SCAN__)
  { (char *)"AT+WFPSCAN",         atcmd_wifi,           15, (char *) "<scan_time_limit>,<ch>..<ch>",              "[STA] Get Passive Scan Results (min_time_limit : 30000)" },
  { (char *)"AT+WFPSTOP",         atcmd_wifi,           0, (char *) "",                                           "[STA] Stop Passive Scan"                               },
  { (char *)"AT+WFPCDTMAX",       atcmd_wifi,           2, (char *) "<bssid>,<max_threshold>",                    "[STA] Set Passive Scan Condition - Max RSSI Threshold" },
  { (char *)"AT+WFPCDTMIN",       atcmd_wifi,           2, (char *) "<bssid>,<min_threshold>",                    "[STA] Set Passive Scan Condition - Min RSSI Threshold" },
#endif // __SUPPORT_PASSIVE_SCAN__
#if defined (__SUPPORT_RSSI_CMD__)
  { (char *)"AT+WFRSSI",          atcmd_wifi,           1, (char *) "",                                           "Get current RSSI"                                      },
#endif // __SUPPORT_RSSI_CMD__
  { (char *)"AT+WFJAP",           atcmd_wifi,           5, (char *) "<ssid>,<auth>,<enc>,<key>[,<hidden>]",       "[STA] Connect to AP"                                   },
  { (char *)"AT+WFJAPA",          atcmd_wifi,           3, (char *) "<ssid>[,<key>][,<hidden>]",                  "[STA] Connect to WPA/WPA2-AP with SSID/PSK only"       },
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
  { (char *)"AT+WFJAPA3",         atcmd_wifi,           4, (char *) "<wpa3_flag>,<ssid>[,<key>][,<hidden>]",      "[STA] Connect to WPA/WPA2/WPA3-AP with SSID/PSK only"  },
#endif // __SUPPORT_WPA3_PERSONAL__
  { (char *)"AT+WFCAP",           atcmd_wifi,           0, (char *) "",                                           "[STA] Connect to AP configured currently"              },
  { (char *)"AT+WFQAP",           atcmd_wifi,           0, (char *) "",                                           "[STA] Disconnect from AP"                              },
  { (char *)"AT+WFROAP",          atcmd_wifi,           1, (char *) "<roam>",                                     "[STA] Run/Stop STA Roaming"                            },
  { (char *)"AT+WFROTH",          atcmd_wifi,           1, (char *) "<rssi>",                                     "[STA] Set Roaming threshold RSSI value"                },
  { (char *)"AT+WFDIS",           atcmd_wifi,           1, (char *) "<disabled>",                                 "[STA] Set Wi-Fi Profile Disabled(1) or Enable(0)"      },
#if defined (__SUPPORT_WPA_ENTERPRISE__)
  { (char *)"AT+WFENTAP",         atcmd_wifi,           6, (char *) "<ssid>,<auth>,<enc>,<phase1>[,<phase2>][,<hidden>]",    "[STA] Set Enterprise AP profile"            },
  { (char *)"AT+WFENTLI",         atcmd_wifi,           2, (char *) "<id>,<pw>",                                  "[STA] Set Enterprise Log-in ID/PWD"                    },
#endif // __SUPPORT_WPA_ENTERPRISE__
  { (char *)"AT+WFSAP",           atcmd_wifi,           6, (char *) "<ssid>,<auth>,<enc>,<key>,<ch>,<country>",   "[AP] Operation Soft-AP mode"                           },
  { (char *)"AT+WFOAP",           atcmd_wifi,           0, (char *) "",                                           "[AP] Operate Soft-AP"                                  },
  { (char *)"AT+WFTAP",           atcmd_wifi,           0, (char *) "",                                           "[AP] Stop Soft-AP"                                     },
  { (char *)"AT+WFRAP",           atcmd_wifi,           0, (char *) "",                                           "[AP] Restart Soft-AP"                                  },
  { (char *)"AT+WFLCST",          atcmd_wifi,           0, (char *) "",                                           "[AP] Get Connected STA List"                           },
  { (char *)"AT+WFAPWM",          atcmd_wifi,           1, (char *) "<mode>",                                     "[AP] Set Wi-Fi mode"                                   },
  { (char *)"AT+WFAPCH",          atcmd_wifi,           1, (char *) "<ch>",                                       "[AP] Set Wi-Fi ch (0~13, 0:auto)"                      },
  { (char *)"AT+WFAPBI",          atcmd_wifi,           1, (char *) "<interval>",                                 "[AP] Set Beacon Interval"                              },
  { (char *)"AT+WFAPUI",          atcmd_wifi,           1, (char *) "<timeout>",                                  "[AP] Set User inactivity timeout value (30 ~ 86400)"   },
  { (char *)"AT+WFAPRT",          atcmd_wifi,           1, (char *) "<threshold>",                                "[AP] Set RTS threshold (1 ~ 2347)"                     },
  { (char *)"AT+WFAPDE",          atcmd_wifi,           1, (char *) "<mac>",                                      "[AP] Send Deauth packet to a specific STA"             },
  { (char *)"AT+WFAPDI",          atcmd_wifi,           1, (char *) "<mac>",                                      "[AP] Send Disassoc  packet to a specific STA"          },
  { (char *)"AT+WFWMM",           atcmd_wifi,           1, (char *) "<wmm>",                                      "[AP] WMM on/off"                                       },
  { (char *)"AT+WFWMP",           atcmd_wifi,           1, (char *) "<wmmps>",                                    "[AP] WMM-PS on/off"                                    },

#if defined ( __SUPPORT_P2P__ )
  { (char *)"AT+WFFP2P",          atcmd_wifi,           0, (char *) "",                                           "[P2P] Find P2P devices"                                },
  { (char *)"AT+WFSP2P",          atcmd_wifi,           0, (char *) "",                                           "[P2P] Stop the P2P Find operation"                     },
  { (char *)"AT+WFCP2P",          atcmd_wifi,           3, (char *) "<mac>,<wps>,<join>",                         "[P2P] Connect to the peer P2P device"                  },
  { (char *)"AT+WFDP2P",          atcmd_wifi,           0, (char *) "",                                           "[P2P] Disconnect from P2P device"                      },
  { (char *)"AT+WFPP2P",          atcmd_wifi,           0, (char *) "",                                           "[P2P] Read P2P Peers information"                      },
  { (char *)"AT+WFPLCH",          atcmd_wifi,           1, (char *) "<ch>",                                       "[P2P] Set listen channel value <0|1|6|11>"             },
  { (char *)"AT+WFPOCH",          atcmd_wifi,           1, (char *) "<ch>",                                       "[P2P] Set operation channel value <0|1|6|11>"          },
  { (char *)"AT+WFPGI",           atcmd_wifi,           1, (char *) "<intent>",                                   "[P2P] Set GO Intent (0~15)"                            },
  { (char *)"AT+WFPFT",           atcmd_wifi,           1, (char *) "<timeout>",                                  "[P2P] Set P2P Find timeout value (1 ~ 86400 sec.)"     },
  { (char *)"AT+WFPDN",           atcmd_wifi,           1, (char *) "<name>",                                     "[P2P] Set P2P Device Name"                             },
#endif // ( __SUPPORT_P2P__ )

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  { (char *)"AT+WFSMSAVE",        atcmd_wifi,           1, (char *) "<run_mode>",                                 "[CON] Save Switching system run-mode (0,1)"            },
  { (char *)"AT+WFMODESWTCH",     atcmd_wifi,           0, (char *) "",                                           "[CON] Switch system run-mode"                          },
#endif // __SUPPORT_WIFI_CONCURRENT__


  /* Network Layer Commands ============================================================ */
  { (char *)"AT+NWIP",            atcmd_network,        4, (char *) "<iface>,<ip_addr>,<netmask>,<gw>",           "Setting for IP Address"                                },
  { (char *)"AT+NWDNS",           atcmd_network,        1, (char *) "<dns_ip>",                                   "Setting for DNS"                                       },
  { (char *)"AT+NWDNS2",          atcmd_network,        1, (char *) "<dns_2nd_ip>",                               "Setting for 2nd DNS"                                   },
  { (char *)"AT+NWHOST",          atcmd_network,        1, (char *) "<name>",                                     "Get Host IP Address By Name"                           },
  { (char *)"AT+NWPING",          atcmd_network,        3, (char *) "<iface>,<dst_ip_addr>,<count>",              "Ping test"                                             },
  { (char *)"AT+NWDHC",           atcmd_network,        1, (char *) "<dhcpc>",                                    "Enable/Disable DHCP Client"                            },
#if defined ( __USER_DHCP_HOSTNAME__ )
  { (char *)"AT+NWDHCHN",         atcmd_network,        1, (char *) "<hostname>",                                 "Save User DHCP client hostname"                        },
  { (char *)"AT+NWDHCHNDEL",      atcmd_network,        0, (char *) "",                                           "Remove User DHCP client hostname"                      },
#endif // __USER_DHCP_HOSTNAME__

  { (char *)"AT+NWDHR",           atcmd_network,        2, (char *) "<start_ip>,<end_ip>",                        "Configure IP address range of DHCP Server"             },
  { (char *)"AT+NWDHLT",          atcmd_network,        1, (char *) "<lease_time>",                               "Lease time of DHCP Server"                             },
  { (char *)"AT+NWDHIP",          atcmd_network,        0, (char *) "",                                           "Read Clients IP info"                                  },
#ifdef __SUPPORT_MESH__
  { (char *)"AT+NWDHDNS",         atcmd_network,        1, (char *) "<dns_ip>",                                   "Configure DNS IP address of DHCPS service"             },
  { (char *)"AT+NWDHS",           atcmd_network,        5, (char *) "<dhcpd>(,<start_ip>,<end_ip>,<lease_time>,<dns_ip>)", "Enable/Disable DHCP Server"                   },
#else
  { (char *)"AT+NWDHS",           atcmd_network,        4, (char *) "<dhcpd>(,<start_ip>,<end_ip>,<lease_time>)", "Enable/Disable DHCP Server"                            },
#endif // __SUPPORT_MESH__
  { (char *)"AT+NWSNS",           atcmd_network,        1, (char *) "<server_ip>",                                "Configure IP address for Time Server"                  },
  { (char *)"AT+NWSNS1",          atcmd_network,        1, (char *) "<server_ip>",                                "Configure IP address for Time Server1"                 },
  { (char *)"AT+NWSNS2",          atcmd_network,        1, (char *) "<server_ip>",                                "Configure IP address for Time Server2"                 },
  { (char *)"AT+NWSNUP",          atcmd_network,        1, (char *) "<period>",                                   "Configure update period of SNTP Client"                },
  { (char *)"AT+NWSNTP",          atcmd_network,        3, (char *) "<sntp>(,<server_ip>,<period>)",              "Enable/Disable SNTP Client service"                    },
  { (char *)"AT+NWSNTP1",         atcmd_network,        3, (char *) "<sntp>(,<server_ip>,<period>)",              "Enable/Disable SNTP Client service"                    },
  { (char *)"AT+NWSNTP2",         atcmd_network,        3, (char *) "<sntp>(,<server_ip>,<period>)",              "Enable/Disable SNTP Client service"                    },

#if defined ( __SUPPORT_MQTT__ )
  { (char *)"AT+NWMQBR",          atcmd_network,        2, (char *) "<ip>,<port>",                                "Configure IP address and Port number for MQTT Broker"  },
  { (char *)"AT+NWMQQOS",         atcmd_network,        1, (char *) "<qos>",                                      "Configure MQTT QoS level (0~2)"                        },
  { (char *)"AT+NWMQTLS",         atcmd_network,        1, (char *) "<tls>",                                      "Enable/Disable tls mode for MQTT"                      },
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
  { (char *)"AT+NWMQALPN",        atcmd_network, (2 + MQTT_TLS_MAX_ALPN), (char *) "<#>,<tls_alpn 1>,...",        "Configure TLS ALPN protocol name"                      },
  { (char *)"AT+NWMQSNI",         atcmd_network,        1, (char *) "<sni>",                                      "Configure TLS SNI"                                     },
  { (char *)"AT+NWMQCSUIT",       atcmd_network, (1 + MQTT_TLS_MAX_CSUITS), (char *) "<cipher suit 1>,<cipher suit 2>,...", "Configure cipher suits"                      },
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
  { (char *)"AT+NWMQTS",          atcmd_network, (1 + MQTT_MAX_TOPIC), (char *) "<#>,<topic#n>,...",              "Set topics for MQTT Subscriber"                        },
  { (char *)"AT+NWMQATS",         atcmd_network,        1, (char *) "<topic>",                                    "Add a topic for MQTT Subscriber"                       },
  { (char *)"AT+NWMQDTS",         atcmd_network,        1, (char *) "<topic>",                                    "Delete a topic for MQTT Subscriber"                    },
  { (char *)"AT+NWMQUTS",         atcmd_network,        1, (char *) "<topic>",                                    "Unsubscribe from the specified topic"                  },
  { (char *)"AT+NWMQTP",          atcmd_network,        1, (char *) "<topic>,...",                                "Topics for MQTT Publisher"                             },
  { (char *)"AT+NWMQPING",        atcmd_network,        1, (char *) "<period>",                                   "Configure MQTT ping period"                            },
  { (char *)"AT+NWMQV311",        atcmd_network,        1, (char *) "<use_v311>",                                 "Use MQTT protocol v3.1.1. default is v3.1"             },
  { (char *)"AT+NWMQCID",         atcmd_network,        1, (char *) "<client_id>",                                "MQTT Client ID"                                        },
  { (char *)"AT+NWMQLI",          atcmd_network,        2, (char *) "<name>,<pw>",                                "Login information for MQTT operation"                  },
  { (char *)"AT+NWMQWILL",        atcmd_network,        3, (char *) "<topic>,<msg>,<qos>",                        "Will information for MQTT"                             },
  { (char *)"AT+NWMQDEL",         atcmd_network,        0, (char *) "",                                           "Initialize MQTT Configurations"                        },
  { (char *)"AT+NWMQCL",          atcmd_network,        1, (char *) "<mqtt_client>",                              "Enable/Disable MQTT Client"                            },
  { (char *)"AT+NWMQMSG",         atcmd_network,        2, (char *) "<msg>(,<topic>)",                            "Send message by MQTT Publisher"                        },
  { (char *)"AT+NWMQTT",          atcmd_network,        8, (char *) "<ip>,<port>,<sub_topic>,<pub_topic>,<qos>,<tls>(,<username>,<password>)", "MQTT Client (Subscriber/Publisher) Operation" },
  { (char *)"AT+NWMQAUTO",        atcmd_network,        1, (char *) "<auto>",                                     "Configure MQTT Auto Start"                             },
#if defined ( __MQTT_CLEAN_SESSION_MODE_SUPPORT__ )
  { (char *)"AT+NWMQCS",          atcmd_network,        1, (char *) "<clean_session>",                            "set clean session mode for MQTT"                       },
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#endif // __SUPPORT_MQTT__

#if defined ( __SUPPORT_WPA_ENTERPRISE__ )
  { (char *)"AT+NWTLSV",          atcmd_network,        1, (char *) "<ver>",                                      "Set TLS Version"                                       },
#endif // __SUPPORT_WPA_ENTERPRISE__
  { (char *)"AT+NWCCRT",          atcmd_network,        0, (char *) "",                                           "Check whether Certificate exists"                      },
  { (char *)"AT+NWDCRT",          atcmd_network,        0, (char *) "",                                           "Delete all Certificate info"                           },

#if defined ( __SUPPORT_HTTP_SERVER_FOR_ATCMD__ )
  { (char *)"AT+NWHTS",           atcmd_network,        1, (char *) "<http_server>",                              "Enable/Disable HTTP Server"                            },
  { (char *)"AT+NWHTSS",          atcmd_network,        1, (char *) "<https_server>",                             "Enable/Disable HTTPs Server"                           },
#endif // __SUPPORT_HTTP_SERVER_FOR_ATCMD__

#if defined ( __SUPPORT_HTTP_CLIENT_FOR_ATCMD__ )
  { (char *)"AT+NWHTC",           atcmd_network,        3, (char *) "<uri>,<option>(,<msg>)",                     "HTTP Client Operation (get|post|put)"                  },
  { (char *)"AT+NWHTCH",          atcmd_network,        3, (char *) "<uri>,<option>(,<msg>)",                     "HTTP Client Operation (get|post|put). Insert data size when sending data."},
  { (char *)"AT+NWHTCTLSAUTH",    atcmd_network,        1, (char *) "<tls_auth_mode>",                            "Set TLS auth mode"                                     },
  { (char *)"AT+NWHTCALPN",       atcmd_network, (2 + HTTPC_MAX_ALPN_CNT), (char *) "<tls_alpn 1>,...",           "Configure TLS ALPN protocol name"                      },
  { (char *)"AT+NWHTCSNI",        atcmd_network,        1, (char *) "<sni>",                                      "Configure TLS SNI"                                     },
  { (char *)"AT+NWHTCALPNDEL",    atcmd_network,        0, (char *) "",                                           "Delete ALPN "                                          },
  { (char *)"AT+NWHTCSNIDEL",     atcmd_network,        0, (char *) "",                                           "Delete SNI "                                           },
#endif // __SUPPORT_HTTP_CLIENT_FOR_ATCMD__

#if defined ( __SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__ )
  { (char *)"AT+NWWSC",           atcmd_network,        3, (char *) "<operation>,<uri>(<msg>)",                   "Websocket Client Opearation(connect|disconnect|send)"  },
#endif // __SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__

#if defined ( __SUPPORT_OTA__ )
  { (char *)"AT+NWOTADWSTART",    atcmd_network,        3, (char *) "<fw_type>,<uri>,<mcu_fw_name>",              "Start OTA download "                                   },
  { (char *)"AT+NWOTADWSTOP",     atcmd_network,        0, (char *) "",                                           "Stop OTA download "                                    },
   { (char *)"AT+NWOTADWPROG",    atcmd_network,        1, (char *) "<fw_type>",                                  "Get OTA download progress "                            },
  { (char *)"AT+NWOTARENEW",      atcmd_network,        0, (char *) "",                                           "Replace with downloaded FW "                           },
  { (char *)"AT+NWOTASETADDR",    atcmd_network,        1, (char *) "<sflash_addr>",                              "Set SFLASH address to store downloaded data from the server" },
  { (char *)"AT+NWOTAGETADDR",    atcmd_network,        1, (char *) "<fw_type>",                                  "Get SFLASH address to store downloaded data from the server" },
  { (char *)"AT+NWOTAREADFLASH",  atcmd_network,        2, (char *) "<sflash_addr>,<size>",                       "Read SFLASH as much as the input address and length "  },
  { (char *)"AT+NWOTAERASEFLASH", atcmd_network,        2, (char *) "<sflash_addr>,<size>",                       "Erase SFLASH as much as the input address and length " },
  { (char *)"AT+NWOTACOPYFLASH",  atcmd_network,        3, (char *) "<dest_sflash_addr>,<src_sflash_addr>,<size>","Copy as much as the length from SFLASH address src_addr to dest_addr" },
  { (char *)"AT+NWOTATLSAUTH",    atcmd_network,        1, (char *) "<tls_auth_mode>",                            "Set mbedtls_ssl_conf_authmode for https-server "       },
#if defined ( __OTA_UPDATE_MCU_FW__ )
  { (char *)"AT+NWOTAFWNAME",     atcmd_network,        0, (char *) "",                                           "Get downloaded MCU FW name in User Sflash area"        },
  { (char *)"AT+NWOTAFWSIZE",     atcmd_network,        0, (char *) "",                                           "Get downloaded MCU FW size in User Sflash area"        },
  { (char *)"AT+NWOTAFWCRC",      atcmd_network,        0, (char *) "",                                           "Get downloaded MCU FW CRC in User Sflash area"         },
  { (char *)"AT+NWOTAREADFW",     atcmd_network,        2, (char *) "<sflash_addr>, <size>",                      "Read downloaded MCU FW in User Sflash area"            },
  { (char *)"AT+NWOTATRANSFW",    atcmd_network,        0, (char *) "",                                           "Transfer downloaded MCU FW in User Sflash area"        },
  { (char *)"AT+NWOTAERASEFW",    atcmd_network,        0, (char *) "",                                           "Erase downloaded MCU FW in User Sflash area"           },
  { (char *)"AT+NWOTABYMCU",      atcmd_network,        2, (char *) "<fw_type>, <size>",                          "Receive DA16200 RTOS from MCU"                         },
#endif // __OTA_UPDATE_MCU_FW__
#endif // __SUPPORT_OTA__

#if defined ( __SUPPORT_ZERO_CONFIG__ )
  { (char *)"AT+NWMDNSSTART",     atcmd_network,        1, (char *) "<wlan interface mode>",                      "Start mDNS module with specific wlan interface mode"   },
  { (char *)"AT+NWMDNSHNREG",     atcmd_network,        1, (char *) "<host name>",                                "Register the host name in mDNS module"                 },
#if defined ( __SUPPORT_DNS_SD__ )
  { (char *)"AT+NWMDNSSRVREG",    atcmd_network,       13,(char *) "<Inst name>,<Prot>,<Port>,[<Text record>]",   "Register a service in mDNS module"                     },
  { (char *)"AT+NWMDNSSRVDEREG",  atcmd_network,        0, (char *) "",                                           "De-register a service in mDNS module"                  },
  { (char *)"AT+NWMDNSUPDATETXT", atcmd_network,        2, (char *) "<Service name>,<Text record>",               "Update the text record of a service in mDNS module"    },
#endif // __SUPPORT_DNS_SD__
  { (char *)"AT+NWMDNSSTOP",      atcmd_network,        1, (char *) "",                                           "Stop the mDNS module"                                  },
#endif // __SUPPORT_ZERO_CONFIG__

#if defined ( __SUPPORT_ATCMD_MULTI_SESSION__ )
  { (char *)"AT+TRTS",            atcmd_transport,      2, (char *) "<local_port>[,<max allowed peers>]",         "Configure the local port number for TCP server"        },
#else
  { (char *)"AT+TRTS",            atcmd_transport,      1, (char *) "<local_port>",                               "Configure the local port number for TCP server"        },
#endif // __SUPPORT_ATCMD_MULTI_SESSION__
  { (char *)"AT+TRTC",            atcmd_transport,      3, (char *) "<svr_ip>,<svr_port>(,<local_port>)",         "Configure the information for TCP client"              },
  { (char *)"AT+TRUSE",           atcmd_transport,      1, (char *) "<local_port>",                               "Open UDP socket"                                       },
  { (char *)"AT+TRUR",            atcmd_transport,      2, (char *) "<remote_ip>,<remote_port>",                  "Configure the IP_addr and port number of UDP client"   },

  { (char *)"AT+TRPRT",           atcmd_transport,      1, (char *) "<cid>",                                      "Display Session Info. by CID"                          },
  { (char *)"AT+TRPALL",          atcmd_transport,      0, (char *) "",                                           "Disply all session"                                    },
  { (char *)"AT+TRTRM",           atcmd_transport,      3, (char *) "<cid>[,<remote_ip>,<remote_port>]",          "Close Session by CID."                                 },
  { (char *)"AT+TRTALL",          atcmd_transport,      0, (char *) "",                                           "Close all session"                                     },
  { (char *)"AT+TRSAVE",          atcmd_transport,      0, (char *) "",                                           "Save current status of all session"                    },

#if defined ( __SUPPORT_TCP_RECVDATA_HEX_MODE__ )
  { (char *)"AT+TCPDATAMODE",     atcmd_tcp_data_mode,  1, (char *) "<mode>",                                     "Convert received TCP data to hex string. mode(0|1)"    },
#endif // __SUPPORT_TCP_RECVDATA_HEX_MODE__

#if defined ( __SUPPORT_ATCMD_TLS__ )
  { (char *)"AT+TRSSLINIT",       atcmd_transport_ssl,  1, (char *) "<role>",                                     "Initialize the SSL module."                            },
  { (char *)"AT+TRSSLCFG",        atcmd_transport_ssl,  3, (char *) "<cid>,<conf_id>,<conf_value>",               "Configure SSL connection."                             },
  { (char *)"AT+TRSSLCO",         atcmd_transport_ssl,  3, (char *) "<cid>,<server_ip>,<server_port>",            "Connect to an SSL server."                             },
  { (char *)"AT+TRSSLWR",         atcmd_transport_ssl,  5, (char *) "<cid>,[<dest>,<port>,<data length>,<data>]", "Send the data to SSL connection."                      },
  { (char *)"AT+TRSSLCL",         atcmd_transport_ssl,  1, (char *) "<cid>",                                      "Close SSL connection."                                 },
  { (char *)"AT+TRSSLCERTLIST",   atcmd_transport_ssl,  1, (char *) "<certificate type>",                         "List certificates or list of CA data available."       },
  { (char *)"AT+TRSSLCERTSTORE",  atcmd_transport_ssl,  6, (char *) "<cert_type>,<seq>,<format>,<name>[,<data length>],<data>",   "Store a certificate/CA list data."                     },
  { (char *)"AT+TRSSLCERTDELETE", atcmd_transport_ssl,  2, (char *) "<certificate type>,<name>",                  "Delete a certificate or CA list data."                 },
  { (char *)"AT+TRSSLSAVE",       atcmd_transport_ssl,  0, (char *) "",                                           "Save current status of all TLS session"                },
  { (char *)"AT+TRSSLDELETE",     atcmd_transport_ssl,  0, (char *) "",                                           "Delete saved TLS session"                              },
#if defined ( DEBUG_ATCMD )
  { (char *)"AT+TRSSLLIST",       atcmd_transport_ssl,  0, (char *) "",                                           "Show SSL module"                                       },
#endif // DEBUG_ATCMD
#endif // __SUPPORT_ATCMD_TLS__

  /*---------------------------------------------------------------------------*/

#if defined ( __SUPPORT_LMAC_RF_CMD__ )
  /* TEST MODE ========================================================================= */
  { (char *)"AT+TMRFNOINIT",      atcmd_testmode,       1, (char *) "<flag>",                                     "RF No init : Test_Mode=1,Normal=0" },
  { (char *)"AT+TMLMACINIT",      atcmd_testmode,       0, (char *) "",                                           "Initalize DA16200 LMAC"            },

  /* RF Test cmd ================================================================================================== */
  { (char *)"AT+RFTESTSTART",     atcmd_rftest,         0, (char *) "",                                           "RF Test Mode Start cmd"            },
  { (char *)"AT+RFTESTSTOP",      atcmd_rftest,         0, (char *) "",                                           "RF Test Mode Stop cmd"             },
  { (char *)"AT+RFTX",            atcmd_rftx,          17, (char *) "<Ch>,<BW>,<numFrames>,<frameLen>,<txRate>,<txPower>,<dstAddr>,<bssid>,<htEnable>,<GI>,<greenField>,<preambleType>,<qosEnable>,<ackPolicy>,<scrambler>,<aifsnVal>,<ant>", "RF TX command" },
  { (char *)"AT+RFTXSTOP",        atcmd_rftx,           0, (char *) "",                                           "RF TX Stop cmd"                    },
  { (char *)"AT+RFCWTEST",        atcmd_rfcwttest,      5, (char *) "<Ch>,< BW >,<txPower>,<ant>,<CWCycle>",      "RF CW Test cmd"                    },
  { (char *)"AT+RFCWSTOP",        atcmd_rfcwttest,      0, (char *) "",                                           "RF CW Test Stop cmd"               },
  { (char *)"AT+RFPER",           atcmd_rf_per,         0, (char *) "",                                           "RF Display PER"                    },
  { (char *)"AT+RFPERRESET",      atcmd_rf_per,         0, (char *) "",                                           "RF Reset PER"                      },
  { (char *)"AT+RFCONTSTART",     atcmd_rf_cont,        3, (char *) "<txRate>,<txPower>,<Ch>",                    "RF Continuous TX Start"            },
  { (char *)"AT+RFCONTSTOP",      atcmd_rf_cont,        0, (char *) "",                                           "RF Continuous TX Stop"             },
  { (char *)"AT+RFCHANNEL",       atcmd_rf_chan,        1, (char *) "<Ch>",                                       "RF Channel"                        },
#endif // __SUPPORT_LMAC_RF_CMD__


#if defined ( __SUPPORT_PERI_CMD__ )
  /* LMAC ============================================================================== */
  // LMAC CLI Command to AT

  { (char *)"AT+SPICONF",         atcmd_spiconf,        2, (char *) "<clockpolarity>,<clockphase>",               "Set SPI clock polarity & phase"    },
  { (char *)"AT+UOTPRDASC",       atcmd_uotp,           2, (char *) "<addr>,<cnt>",                               ""                                  },
  { (char *)"AT+UOTPWRASC",       atcmd_uotp,           3, (char *) "<cnt>,<data0~datan>",                        ""                                  },
  { (char *)"AT+XTALWR",          atcmd_xtal,           1, (char *) "<value>",                                    ""                                  },
  { (char *)"AT+XTALRD",          atcmd_xtal,           0, (char *) "",                                           ""                                  },
#if defined (DEBUG_ATCMD)
  { (char *)"AT+UVER",            atcmd_get_uversion,   0, (char *) "get version",                                ""                                  },
#endif // DEBUG_ATCMD
  { (char *)"AT+CALWR",           atcmd_set_TXPower,    3, (char *) "write RF Cal Reg",                           ""                                  },
  { (char *)"AT+FLASHDUMP",       atcmd_flash_dump,     2, (char *) "Dump Flash Image",                           ""                                  },

  { (char *)"AT+GPIOSTART",       atcmd_gpio_test,      3, (char*) "GPIO Test",                                   ""                                  },
  { (char *)"AT+GPIORD",          atcmd_gpio_test,      2, (char*) "GPIO Test",                                   ""                                  },
  { (char *)"AT+GPIOWR",          atcmd_gpio_test,      3, (char*) "GPIO Test",                                   ""                                  },
  { (char *)"AT+SAVE_PININFO",    atcmd_gpio_test,      0, (char*) "GPIO Test",                                   ""                                  },
  { (char *)"AT+RESTORE_PININFO", atcmd_gpio_test,      0, (char*) "GPIO Test",                                   ""                                  },

  { (char *)"AT+SLEEPMS",         atcmd_sleep_ms,       1, (char *) "sleep to wakeup sequence",                   ""                                  },
#endif // __SUPPORT_PERI_CMD__

#if defined ( __SUPPORT_PERI_CTRL__ )
  // ex) 1. AT+LEDINIT
  //       2. AT+LEDCTRL,1,ON  => [1|2|3], [ON|OFF]
  { (char *)"AT+LEDINIT",         atcmd_led_init,       0, (char *) "", ""                                                                            },
  { (char *)"AT+LEDCTRL",         atcmd_led_ctrl,       2, (char *) "<led number>,<on|off>",                      "led control,"                      },

  { (char *)"AT+I2CINIT",         atcmd_i2c,            0, (char *) "", ""                                                                            },
  { (char *)"AT+I2CWRITE",        atcmd_i2c,            4, (char *) "<slave_addr>,<reg_addr>,<len>,<data>,...",   ""                                  },
  { (char *)"AT+I2CREAD",         atcmd_i2c,            3, (char *) "<slave addr>,<reg_addr>,<len>",              ""                                  },

  // ex) 1. AT+PWMINIT
  //       2. AT+PWMSTART=0,40,50,0  => 0 : PWM0, 40 : period, 50 : duty(percent), 0 : mode(optional)
  { (char *)"AT+PWMINIT",         atcmd_pwm,            1, (char *) "",                                           ""                                  },
  { (char *)"AT+PWMSTART",        atcmd_pwm,            4, (char *) "<0>,<period>,<duty>[,<mode>]",               ""                                  },
  { (char *)"AT+PWMSTOP",         atcmd_pwm,            1, (char *) "",                                           ""                                  },

  // ex) 1. AT+ADCINIT
  { (char *)"AT+ADCINIT",         atcmd_adc,            0, (char *) "",                                           ""                                  },
  { (char *)"AT+ADCSTART",        atcmd_adc,            1, (char *) "<div>",                                      ""                                  },
  { (char *)"AT+ADCSTOP",         atcmd_adc,            0, (char *) "", ""                                                                            },
  { (char *)"AT+ADCREAD",         atcmd_adc,            2, (char *) "<channel>,<sample_count>",                   ""                                  },
  { (char *)"AT+ADCCHEN",         atcmd_adc,            2, (char *) "<channel>,<resolution>",                     ""                                  },
#endif // __SUPPORT_PERI_CTRL__

#if defined ( __BLE_COMBO_REF__ )
  { (char *)"AT+BLENAME",         atcmd_ble,            1, (char *) "<device name>",                              "Set the device name of BLE"        },
  { (char *)"AT+ADVSTOP",         atcmd_ble,            1, (char *) "",                                           "Stop BLE ADV"                      },
  { (char *)"AT+ADVSTART",        atcmd_ble,            1, (char *) "",                                           "Start BLE ADV"                     },
#endif // __BLE_COMBO_REF__

#if defined ( __PROVISION_ATCMD__ )
  // Provisioning start
  { (char *)"AT+PROVSTART",       atcmd_app_provision,  0, (char *) "",                                           ""                                  },
  // Provisioning stat
  { (char *)"AT+PROVSTAT",        atcmd_app_provision,  0, (char *) "",                                           ""                                  },
#endif  // __PROVISION_ATCMD__

  // watchdog service
  { (char *)"AT+WDOGRESCALETIME", atcmd_wdog_service,   1, (char *) "<?>|<rescale time>",                         "Set watchdog next expried time."   },

  /*==================================================================================== */

  { NULL, NULL, 0, NULL, NULL }
};


/* .... Funtion body ..................................................................... */

static int softap_init_done_sent_flag = pdFALSE;

static int get_INIT_DONE_sent_flag(void)
{
    return softap_init_done_sent_flag;
}

void set_INIT_DONE_msg_to_MCU_on_SoftAP_mode(int flag)
{
    softap_init_done_sent_flag = flag;
}


void atcmd_wf_jap_dap_print_with_cause(int is_jap)
{
    #define WLAN_REASON_TIMEOUT                         39
    #define WLAN_REASON_PEERKEY_MISMATCH                45
    #define WLAN_REASON_AUTHORIZED_ACCESS_LIMIT_REACHED 46

    #define WLAN_REASON_PREV_AUTH_NOT_VALID             2
    #define WLAN_REASON_DEAUTH_LEAVING                  3
    #define WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY      4
    #define WLAN_REASON_DISASSOC_AP_BUSY                5

    if (is_waiting_for_wf_jap_result_in_progress) {
        return;
    }

    if (is_jap) {
        switch (wifi_conn_fail_reason_code) {
        case WLAN_REASON_TIMEOUT :
            PRINTF_ATCMD("\r\n+WFJAP:0,TIMEOUT\r\n"); break;
        case WLAN_REASON_PEERKEY_MISMATCH :
            PRINTF_ATCMD("\r\n+WFJAP:0,WRONGPWD\r\n"); break;
        case WLAN_REASON_AUTHORIZED_ACCESS_LIMIT_REACHED :
            PRINTF_ATCMD("\r\n+WFJAP:0,ACCESSLIMIT\r\n"); break;
        default :
            PRINTF_ATCMD("\r\n+WFJAP:0,OTHER,%d\r\n", wifi_conn_fail_reason_code); break;
        }

    } else {
        switch (wifi_disconn_reason_code) {
        case WLAN_REASON_PREV_AUTH_NOT_VALID :
            PRINTF_ATCMD("\r\n+WFDAP:0,AUTH_NOT_VALID\r\n"); break;
        case WLAN_REASON_DEAUTH_LEAVING :
            PRINTF_ATCMD("\r\n+WFDAP:0,DEAUTH\r\n"); break;
        case WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY :
            PRINTF_ATCMD("\r\n+WFDAP:0,INACTIVITY\r\n"); break;
        case WLAN_REASON_DISASSOC_AP_BUSY :
            PRINTF_ATCMD("\r\n+WFDAP:0,APBUSY\r\n"); break;
        default :
            PRINTF_ATCMD("\r\n+WFDAP:0,OTHER,%d\r\n", wifi_disconn_reason_code); break;
        }
    }
}


void atcmd_wf_jap_print_with_cause(int cause)
{
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
    extern int wifi_conn_fail_reason_code;

    if (cause == 0 && wifi_conn_fail_reason_code == 0) { // for no selected network case
        PRINTF_ATCMD("\r\n+WFJAP:0,NOT_FOUND\r\n");
        return;
    } else if (cause == 3) { // arp fail
        PRINTF_ATCMD("\r\n+WFJAP:0,ARP_RSP_FAIL\r\n");
        return;
    }
#else
    DA16X_UNUSED_ARG(cause);
#endif // __SUPPORT_DPM_ABNORM_MSG__
}


#if defined (__SUPPORT_MQTT__)
static int atcmd_wfjap_not_send_by_err;
static int atcmd_mqtt_connected_ind_sent;
static int atcmd_mqtt_qos0_msg_send_done_in_dpm;
static int atcmd_mqtt_qos0_msg_send_rc;

void mqtt_client_convert_to_atcmd_err_code(int* err_code, int* atcmd_err_code)
{
    int mosq_err_code = *err_code;

    if (mosq_err_code == MOSQ_ERR_INVAL || mosq_err_code == MOSQ_ERR_PAYLOAD_SIZE) {
        *atcmd_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
    } else if (mosq_err_code == MOSQ_ERR_NO_CONN || mosq_err_code == MOSQ_ERR_CONN_LOST) {
        *atcmd_err_code = AT_CMD_ERR_NOT_CONNECTED;
    } else {
        *atcmd_err_code = AT_CMD_ERR_UNKNOWN;
    }
}

void atcmd_asynchony_event_for_mqttmsgpub(int result, int err_code)
{
    int atcmd_err;

    if (result == 1) {
        PRINTF_ATCMD("\r\n+NWMQMSGSND:1\r\n");
    } else if (result == 0) {
        mqtt_client_convert_to_atcmd_err_code(&err_code, &atcmd_err);
        PRINTF_ATCMD("\r\n+NWMQMSGSND:0,%d\r\n", atcmd_err);
    }
}

static void atcmd_mqtt_msg_qos0_dpm_rsp_handle(void)
{
    if (atcmd_mqtt_qos0_msg_send_done_in_dpm) {
        if (atcmd_mqtt_qos0_msg_send_rc == 0) {
            atcmd_asynchony_event_for_mqttmsgpub(1, 0);
        } else {
            atcmd_asynchony_event_for_mqttmsgpub(0, atcmd_mqtt_qos0_msg_send_rc);
        }

        atcmd_mqtt_qos0_msg_send_done_in_dpm = pdFALSE;
    }
}
#else
static void atcmd_mqtt_msg_qos0_dpm_rsp_handle(void)
{
    ;
}
#endif // __SUPPORT_MQTT__

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static void monitor_atcmd(void)
{
    atcmd_error_code    major_err_code = AT_CMD_ERR_CMD_OK;
    char *user_atcmd_buf = NULL;
    int  cmd_len = 0;

#if defined (__SUPPORT_DPM_ABNORM_MSG__)
    int abn_type = get_last_abnormal_act();
#endif // __SUPPORT_DPM_ABNORM_MSG__

    atcmd_uart_mon_buf = (char *)pvPortMalloc(USER_ATCMD_BUF);
    if (atcmd_uart_mon_buf == NULL) {
        PRINTF("[%s] Failed to allocate memory(atcmd_uart_mon_buf)\n", __func__);
        return;
    }

    user_atcmd_buf = (char *)pvPortMalloc(USER_ATCMD_BUF);
    if (user_atcmd_buf == NULL) {
        PRINTF("[%s] Failed to allocate memory(user_atcmd_buf)\n", __func__);
        return;
    }

    ATCMD_DBG("\nStart AT Command \n");

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
    g_is_atcmd_init = 1;

    if (!dpm_mode_is_wakeup()) {
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
        if (abn_type != 0) {
            PRINTF_ATCMD("\r\n+INIT:WAKEUP,ABNORM_%d\r\n", abn_type);
        } else
#endif // __SUPPORT_DPM_ABNORM_MSG__
        if (!is_in_softap_acs_mode()) {
            PRINTF_ATCMD("\r\n+INIT:DONE,%d\r\n", get_run_mode());

            if (get_run_mode() == SYSMODE_AP_ONLY) {
                set_INIT_DONE_msg_to_MCU_on_SoftAP_mode(pdTRUE);
            }
        }
    } else {
#ifdef    __DPM_WAKEUP_NOTICE_ADDITIONAL__
        PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
#else
        if (   atcmd_dpm_wakeup_status() != NULL
#if defined (__SUPPORT_NOTIFY_RTC_WAKEUP__)
            && strcmp(atcmd_dpm_wakeup_status(), "POR") != 0
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__
           ) {
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
            if (strcmp(atcmd_dpm_wakeup_status(), "UNDEFINED") == 0 && abn_type != 0) {
                PRINTF_ATCMD("\r\n+INIT:WAKEUP,ABNORM_%d\r\n", abn_type);
            } else
#endif // (__SUPPORT_DPM_ABNORM_MSG__)
                PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
        }
#endif // (__DPM_WAKEUP_NOTICE_ADDITIONAL__)
    }

    g_is_atcmd_init = 0;
#endif // __ATCMD_IF_UART1__

#if defined (__DPM_TEST_WITHOUT_MCU__)
    atcmd_dpm_ready = pdTRUE;
#endif // (__DPM_TEST_WITHOUT_MCU__)

    init_done_sent = pdTRUE;

    at_command_table = commands;

    /* For User AT-CMD help */
    reg_user_atcmd_help_cb();

    while (1) {
        memset(atcmd_uart_mon_buf, 0, USER_ATCMD_BUF);

        major_err_code = gets_atcmd_uart(atcmd_uart_mon_buf);

        if (major_err_code == AT_CMD_ERR_CMD_OK) {

#if defined (__OTA_UPDATE_MCU_FW__)
            if (ota_update_atcmd_parser(atcmd_uart_mon_buf) == OTA_SUCCESS) {
                major_err_code = AT_CMD_ERR_CMD_OK;
            } else
#endif //(__OTA_UPDATE_MCU_FW__)
            if (atcmd_uart_mon_buf[0] == ESCAPE_CODE) {
                /* DATA MODE */
                esc_cmd_parser(atcmd_uart_mon_buf);
#if defined (__SUPPORT_ATCMD_TLS__)
            } else if (strncmp(atcmd_uart_mon_buf, ATCMD_TLS_WR_CMD, strlen(ATCMD_TLS_WR_CMD)) == 0) {
                major_err_code = command_parser_tls_wr(atcmd_uart_mon_buf);

                if (major_err_code == AT_CMD_ERR_UNKNOWN_CMD) {
                    goto normal_command;
                }
            } else if (strncmp(atcmd_uart_mon_buf, ATCMD_TLS_CERTSTORE_CMD, strlen(ATCMD_TLS_CERTSTORE_CMD)) == 0) {
                major_err_code = command_parser_tls_certstore(atcmd_uart_mon_buf);

                if (major_err_code == AT_CMD_ERR_UNKNOWN_CMD) {
                    goto normal_command;
                }
#endif // __SUPPORT_ATCMD_TLS__
            } else {
#if defined (__SUPPORT_ATCMD_TLS__)
normal_command:
#endif // __SUPPORT_ATCMD_TLS__
                cmd_len = strlen(atcmd_uart_mon_buf);
                if ((cmd_len > 0) && (USER_ATCMD_BUF > cmd_len)) {
                    major_err_code = command_parser(atcmd_uart_mon_buf);

                    if (major_err_code == AT_CMD_ERR_UNKNOWN_CMD && cmd_len < USER_ATCMD_BUF) {
                        memset(user_atcmd_buf, 0, USER_ATCMD_BUF);
                        strncpy(user_atcmd_buf, atcmd_uart_mon_buf, cmd_len);

                        user_atcmd_parser(user_atcmd_buf);
                    }
                }
            }
        } else {
            PRINTF_ATCMD("\r\nERROR:%d\r\n", major_err_code);
        }
    }
}
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

void atcmd_task(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int sys_wdog_id = -1;
#if defined ( __ENABLE_TRANSFER_MNG__ )
    int ret = 0;
#endif // __ENABLE_TRANSFER_MNG__

    void (*interface_init_fp)(void) = user_interface_init;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    if (atcmd_tr_mng_create_atcmd_mutex()) {
        PRINTF("[%s] Faild to creatre atcmd_mutex !!!\n", __func__);

        da16x_sys_watchdog_unregister(sys_wdog_id);

        vTaskDelete(NULL);
        return ;
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    interface_init_fp();

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

#if defined ( __ENABLE_TRANSFER_MNG__ )
    ret = atcmd_tr_mng_init_context(&atcmd_tr_mng_ctx);
    if (ret) {
        PRINTF("[%s] Failed to init atcmd_tr_mng_ctx !!!\n", __func__);
        goto end;
    }

    ret = atcmd_tr_mng_init_config(&atcmd_tr_mng_conf);
    if (ret) {
        PRINTF("[%s] Failed to init atcmd_tr_mng_conf !!!\n", __func__);
        goto end;
    }

    ret = atcmd_tr_mng_set_config(&atcmd_tr_mng_ctx, &atcmd_tr_mng_conf);
    if (ret) {
        PRINTF("[%s] Failed to set cmd_tr_mng_ctx !!!\n", __func__);
        goto end;
    }

    ret = atcmd_tr_mng_create_task(&atcmd_tr_mng_ctx);
    if (ret) {
        PRINTF("[%s] Failed to create atcmd_tr_mng_ctx !!!\n", __func__);
        goto end;
    }

end :
    if (ret) {
        atcmd_tr_mng_delete_task(&atcmd_tr_mng_ctx);
        atcmd_tr_mng_deinit_context(&atcmd_tr_mng_ctx);
    }
#endif // __ENABLE_TRANSFER_MNG__

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    monitor_atcmd();

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

#if defined ( __ATCMD_IF_UART1__ ) || defined ( __ATCMD_IF_UART2__ )
static int gets_esc_atcmd_uart(char *buf)
{
    typedef enum {
        READ_CID,
        READ_LENGTH,
        READ_CID_LENGTH,
        READ_REMOTE_IP,
        READ_REMOTE_PORT,
        READ_MODE,
        READ_DATA
    } atcmd_esc_cmd_parameter_step;

    atcmd_error_code err = AT_CMD_ERR_CMD_OK;

    HANDLE atcmd_uart;

    char ch = 0;
    int buf_idx = 0;
    int prev_buf_idx = 0;
    int start_msg = 0;

    int tmp_echo_flag = pdFALSE;

    char p_buf[32] = {0x00,};
    unsigned int p_idx = 0;
    atcmd_esc_cmd_parameter_step p_step = READ_CID_LENGTH;

    int cid = 0;
    int data_len = 0;
    int r_mode = pdFALSE;
    int port = 0;
    int mode = 0;

    int dma_read = pdFALSE;
    const int min_dma_data_len = 16;

    //for certificate
    int is_cert_cmd = pdFALSE;
    int is_cert_cmd_done = pdFALSE;
    typedef enum {
        READ_CERT_MODULE,   // 0
        READ_CERT_TYPE,     // 1
        READ_CERT_MODE,     // 2
        READ_CERT_FORMAT,   // 3
        READ_CERT_LENGTH,   // 4
        READ_CERT_DATA      // 5
    } atcmd_esc_cert_cmd_parameter_step;
    atcmd_esc_cert_cmd_parameter_step c_step = READ_CERT_MODULE;

#if defined ( __ATCMD_IF_UART2__ )
    atcmd_uart = uart2;
#else
    atcmd_uart = uart1;
#endif // (__ATCMD_IF_UART2__)

    buf[buf_idx++] = ESCAPE_CODE;

    // Read 1st character
    ch = getchar_atcmd_uart(portMAX_DELAY);
    if ((ch ==  esc_cmd_s_upper_case || ch == esc_cmd_s_lower_case)
            || (ch == esc_cmd_m_upper_case || ch == esc_cmd_m_lower_case)
            || (ch == esc_cmd_h_upper_case || ch == esc_cmd_h_lower_case)) {

        if ((ch == esc_cmd_m_upper_case || ch == esc_cmd_m_lower_case)
                || (ch == esc_cmd_h_upper_case || ch == esc_cmd_h_lower_case)) {
            p_step = READ_CID;
        }

        if (ch == esc_cmd_h_upper_case || ch == esc_cmd_h_lower_case) {
            dma_read = pdTRUE;
        }

        buf[buf_idx++] = ch;
        prev_buf_idx = buf_idx - 1;

        if (atcmd_esc_data_echo_flag == pdTRUE) {
            if (atcmd_uart_echo == pdTRUE) {
                atcmd_uart_echo = pdFALSE;
                tmp_echo_flag = pdTRUE;
            }
        }

        while (1) {
            ch = getchar_atcmd_uart(portMAX_DELAY);

            if (buf_idx >= (USER_ATCMD_BUF - 2)) {
                ATCMD_DBG("[%s:%d]overflow(%ld >= %ld)\n", __func__, __LINE__,
                          buf_idx, (USER_ATCMD_BUF - 2));
                err = AT_CMD_ERR_TOO_LONG_RESULT;
                break;
            }

read_data:
            if (p_step == READ_DATA) {
                if (dma_read) {
                    ATCMD_DBG("[%s:%d]UART_DMA_READ(%ld)\n", __func__, __LINE__, data_len);

                    if (data_len <= 0) {
                        err = AT_CMD_ERR_INSUFFICENT_ARGS;
                        break;
                    }

                    // Clear RX Queue and HW fifo in UART
                    UART_FLUSH(atcmd_uart);

                    // Reponse OK, and then waiting to read data
                    PRINTF_ATCMD("\r\nOK\r\n");

                    // Read data
                    if (data_len < min_dma_data_len) {
                        UART_READ(atcmd_uart, buf + buf_idx, data_len);
                    } else {
                        UART_DMA_READ(atcmd_uart, buf + buf_idx, data_len);
                    }

                    buf_idx += data_len;

                    break;
                } else if (r_mode) {
                    ATCMD_DBG("[%s:%d]R mode(%ld)\n", __func__, __LINE__, data_len);

                    buf[buf_idx++] = ch;

                    if (data_len > 1) {
                        UART_READ(atcmd_uart, buf + buf_idx, data_len - 1);
                    }

                    buf_idx += (data_len - 1);

                    break;
                } else {
                    ATCMD_DBG("[%s:%d]T mode(%ld)\n", __func__, __LINE__, data_len);

                    if (ch == '\b')    {        // backspace
                        buf_idx--;
                        buf[buf_idx] = '\0';
                        PRINTF_ATCMD("\33[0K");
                    } else if (data_len) {
                        buf[buf_idx++] = ch;

                        if (ch == '\n' || ch == '\r') {
                            PRINTF_ATCMD("\r\n");
                        }

                        if (data_len == (buf_idx - start_msg)) {
                            break;
                        }
                    } else {
                        if (ch == '\n' || ch == '\r') {
                            break;
                        }

                        buf[buf_idx++] = ch;
                    }
                }
            } else if (p_step == READ_MODE) {
                start_msg = buf_idx;

                if (p_idx == 0) {
                    //check mode
                    if (ch == 'r' || ch == 'R') {
                        r_mode = pdTRUE;
                    } else if (ch == 't' || ch == 'T') {
                        r_mode = pdFALSE;
                    } else {
                        //default mode
                        r_mode = pdFALSE;
                        p_step = READ_DATA;
                        goto read_data;
                    }

                    p_buf[p_idx++] = ch;
                } else if (p_idx == 1) {
                    p_step = READ_DATA;

                    //check comma
                    if (ch != 0x2C) {
                        buf[buf_idx++] = p_buf[0];
                        start_msg = buf_idx;
                        r_mode = pdFALSE;
                        goto read_data;
                    }
                }
            } else {
                if (p_idx > sizeof(p_buf)) {
                    ATCMD_DBG("[%s:%d]overflow(%ld > %ld)\n", __func__, __LINE__,
                              p_idx, sizeof(p_buf));
                    err = AT_CMD_ERR_TOO_LONG_RESULT;
                    break;
                }

                // allowed only numeric and .
                if ((ch >= 0x30 && ch <= 0x39) || ch == 0x2E) {
                    p_buf[p_idx++] = ch;
                } else if (ch == '\n' || ch == '\r') {
                    if (p_step == READ_REMOTE_PORT) {
                        ATCMD_DBG("[%s:%d]Parse remote port\n", __func__, __LINE__);
                        goto read_remote_port;
                    }

                    //rollback to previous buf idx;
                    buf_idx = prev_buf_idx;

                    break;
                } else if (ch == 0x2C) {    // comma(,)
                    ATCMD_DBG("[%s:%d]p_step(%d,%s)\n",
                              __func__, __LINE__, p_step, p_buf);

                    if (p_step == READ_CID) {    // <cid>
                        if (get_int_val_from_str(p_buf, &cid, POL_1) != 0) {
                            ATCMD_DBG("[%s:%d]invalid cid(%s)\n",
                                      __func__, __LINE__, p_buf);
                            break;
                        }

                        prev_buf_idx = buf_idx;

                        sprintf(buf, "%s%s,", buf, p_buf);
                        buf_idx += (p_idx + 1);

                        p_step = READ_LENGTH;
                    } else if (p_step == READ_LENGTH) {    // <length>
                        if ((get_int_val_from_str(p_buf, &data_len, POL_1) != 0)
                                || (data_len > TX_PAYLOAD_MAX_SIZE)) {
                            ATCMD_DBG("[%s:%d]invalid data length(%s)\n",
                                      __func__, __LINE__, p_buf);
                            break;
                        }

                        prev_buf_idx = buf_idx;

                        sprintf(buf, "%s%s,", buf, p_buf);
                        buf_idx += (p_idx + 1);

                        p_step = READ_REMOTE_IP;
                    } else if (p_step == READ_CID_LENGTH) {    // <cid><length>
                        if ((get_int_val_from_str(p_buf + 1, &data_len, POL_1) != 0)
                                || (data_len > TX_PAYLOAD_MAX_SIZE)) {
                            ATCMD_DBG("[%s:%d]invalid data length(%s)\n",
                                      __func__, __LINE__, p_buf);
                            break;
                        }

                        prev_buf_idx = buf_idx;

                        sprintf(buf, "%s%s,", buf, p_buf);
                        buf_idx += (p_idx + 1);

                        p_step = READ_REMOTE_IP;
                    } else if (p_step == READ_REMOTE_IP) {    // <remote_ip>
                        if ((strcmp(p_buf, "0.0.0.0") != 0)
                            && (strcmp(p_buf, "0") != 0)
                            && !(is_in_valid_ip_class(p_buf))) {
                            ATCMD_DBG("[%s:%d]invalid ip address(%s)\n",
                                      __func__, __LINE__, p_buf);
                            break;
                        }

                        prev_buf_idx = buf_idx;

                        sprintf(buf, "%s%s,", buf, p_buf);
                        buf_idx += (p_idx + 1);

                        p_step = READ_REMOTE_PORT;
                    } else if (p_step == READ_REMOTE_PORT) {    // <remote_port>
read_remote_port:
                        if ((get_int_val_from_str(p_buf, &port, POL_1) != 0)
                                || (port < 0) || (port > 0xFFFF)) {
                            ATCMD_DBG("[%s:%d]invalid port(%s)\n",
                                      __func__, __LINE__, p_buf);
                            break;
                        }

                        prev_buf_idx = buf_idx;

                        sprintf(buf, "%s%s,", buf, p_buf);
                        buf_idx += (p_idx + 1);

                        //skipped mode option in case of <ESC>H
                        if (dma_read) {
                            p_step = READ_DATA;
                            goto read_data;
                        } else {
                            p_step = READ_MODE;
                        }
                    }

                    memset(p_buf, 0x00, sizeof(p_buf));
                    p_idx = 0;

                    start_msg = buf_idx;
                } else {
                    //rollback to previous buf idx;
                    buf_idx = prev_buf_idx;

                    break;
                }
            }
        }

        if (atcmd_esc_data_echo_flag == pdTRUE) {
            if (tmp_echo_flag == pdTRUE) {
                atcmd_uart_echo = pdTRUE;
                tmp_echo_flag = pdFALSE;
            }
        }

        if (atcmd_esc_data_dump_flag == pdTRUE) {
            hex_dump_atcmd((UCHAR *)(buf + start_msg), data_len);
        }
    } else if (ch == 'C' || ch == 'c') {
        buf[buf_idx++] = ch;

        // <ESC>C
        while (1) {
            ch = getchar_atcmd_uart(portMAX_DELAY);

            if (buf_idx >= (USER_ATCMD_BUF - 1)) {
                buf_idx = USER_ATCMD_BUF - 2;
            }

            if (ch == '\b') {        /* backspace */
                buf_idx--;
                buf[buf_idx] = '\0';
            } else if (ch == 0x0d) {
                buf[buf_idx++] = 0x0a;
            } else if (ch == 0x03 || ch == 0x1a) {
                buf[buf_idx++] = '\0';
                break;
            } else {
                buf[buf_idx++] = ch;
            }

            if ((unsigned int)(buf_idx - 1) == strlen(esc_cmd_cert_case)) {
                if (strcasecmp(esc_cmd_cert_case, buf + 1) == 0) {
                    is_cert_cmd = pdTRUE;
                    break;
				}
            }
        }

        // <ESC>CERT
        if (is_cert_cmd == pdTRUE) {
            // ,
            ch = getchar_atcmd_uart(portMAX_DELAY);
            if (ch != 0x2C) {   // comma(,)
                err = AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            buf[buf_idx++] = ch;

            // <module>,<certificate type>,<mode>[,<format>,<length>,<content>]
            while (err == AT_CMD_ERR_CMD_OK && !is_cert_cmd_done) {
                switch (c_step) {
                case READ_CERT_MODULE:
                case READ_CERT_TYPE:
                case READ_CERT_MODE:
                case READ_CERT_FORMAT:
                case READ_CERT_LENGTH:
                    memset(p_buf, 0x00, sizeof(p_buf));
                    p_idx = 0;
                    ch = 0x00;

                    while (ch != 0x2C) {
                        if (p_idx >= sizeof(p_buf)) {
                            err = AT_CMD_ERR_TOO_LONG_RESULT;
                            break;
                        }

                        ch = getchar_atcmd_uart(portMAX_DELAY);

                        p_buf[p_idx++] = ch;

                        if (c_step == READ_CERT_MODE && p_idx == 1) {
                            p_buf[1] = '\0';

                            if (get_int_val_from_str(p_buf, &mode, POL_1) != 0) {
                                err = AT_CMD_ERR_INSUFFICENT_ARGS;
                                break;
                            }

                            if (mode == DA16X_CERT_MODE_DELETION) {
                                is_cert_cmd_done = pdTRUE;
                                break;
                            }
                        }
                    }

                    //hex_dump((unsigned char *)p_buf, p_idx);

                    memcpy(buf + buf_idx, p_buf, p_idx);
                    buf_idx += p_idx;

                    if (c_step == READ_CERT_LENGTH) {
                        p_buf[p_idx - 1] = '\0';
                        if (get_int_val_from_str(p_buf, &data_len, POL_1) != 0) {
                            err = AT_CMD_ERR_INSUFFICENT_ARGS;
                            break;
                        }
                    }

                    //Update step
                    if (c_step == READ_CERT_MODULE) {
                        c_step = READ_CERT_TYPE;
                    } else if (c_step == READ_CERT_TYPE) {
                        c_step = READ_CERT_MODE;
                    } else if (c_step == READ_CERT_MODE) {
                        c_step = READ_CERT_FORMAT;
                    } else if (c_step == READ_CERT_FORMAT) {
                        c_step = READ_CERT_LENGTH;
                    } else if (c_step == READ_CERT_LENGTH) {
                        c_step = READ_CERT_DATA;
                    }
                    break;
                case READ_CERT_DATA:
                    UART_READ(atcmd_uart, buf + buf_idx, data_len);
                    buf_idx += data_len;
                    is_cert_cmd_done = pdTRUE;
                    break;
                default:
                    break;
                }
            }
        }
    }

    buf[buf_idx++] = '\0';

    return err;
}

static int getchar_atcmd_uart(UINT32 mode)
{
    char text;
    int temp ;
    HANDLE    atcmd_uart;

#if defined ( __ATCMD_IF_UART2__ )
    atcmd_uart = uart2;
#else
    atcmd_uart = uart1;
#endif // __ATCMD_IF_UART2__

    if (mode == portNO_DELAY) {
        temp = pdFALSE;
        text = '\0';

        UART_IOCTL(atcmd_uart, UART_SET_RX_SUSPEND, &temp);
        UART_READ(atcmd_uart, &text, sizeof(char));
        temp = pdTRUE;
        UART_IOCTL(atcmd_uart, UART_SET_RX_SUSPEND, &temp);
    } else {
        UART_READ(atcmd_uart, &text, sizeof(char));

        if (atcmd_uart_echo == pdTRUE) {
            if (atcmd_tr_mng_take_atcmd_mutex(portMAX_DELAY)) {
                PRINTF("- [%s] failed to get for atcmd_mutex\n", __func__);
            }

            UART_WRITE(atcmd_uart, &text, sizeof(char));
            if (atcmd_tr_mng_give_atcmd_mutex()) {
                PRINTF("- [%s] Failed to semafree for atcmd_mutex\n", __func__);
            }
        }
    }

    return (int)text;
}

#if defined (__OTA_UPDATE_MCU_FW__)
static int atcmd_hex2dec(char *src, int src_len)
{
	int base = 1;
	int dec_val = 0;

	for (int idx = src_len - 1 ; idx >= 0; idx--) {
		if (src[idx] >= '0' && src[idx] <= '9') {
			dec_val += (src[idx] - '0') * base;
			base = base * 10;
		} else if (src[idx] >= 'A' && src[idx] <= 'F') {
			dec_val += (src[idx] - 'A' + 10) * base;
			base = base * 10;
		} else if (src[idx] >= 'a' && src[idx] <= 'f') {
			dec_val += (src[idx] - 'a' + 10) * base;
			base = base * 10;
		}
	}
	return dec_val;
}
#endif // (__OTA_UPDATE_MCU_FW__)

static int gets_atcmd_uart(char *buf)
{
    char ch = 0;
    int  i = 0;
    char quotes = pdFALSE;
#if defined (__OTA_UPDATE_MCU_FW__)
    char *ota_ptr = buf;
    bool ota_parse_flag = pdFALSE;
    int ota_idx = 0;
    int ota_tx_size = 0;
    int ota_offset_len = 0;
    HANDLE    atcmd_uart;
#if defined ( __ATCMD_IF_UART2__ )
    atcmd_uart = uart2;
#else
    atcmd_uart = uart1;
#endif // (__ATCMD_IF_UART2__)
#endif //(__OTA_UPDATE_MCU_FW__)

    while (1) {

#if defined (__OTA_UPDATE_MCU_FW__)
        if ((ota_tx_size > 0) && (ota_update_by_mcu_get_total_len() > 0)) {
            UART_DMA_READ(atcmd_uart, buf, ota_tx_size);
            return AT_CMD_ERR_CMD_OK;

        } else {
            ch = getchar_atcmd_uart(portMAX_DELAY);
        }

        if (ota_update_by_mcu_get_total_len() > 0) {
            if (ota_tx_size == 0) {
                if (ota_parse_flag == pdTRUE) {
                    if (ch == ',') {
                        ota_parse_flag = pdFALSE;
                        ota_tx_size = atcmd_hex2dec(ota_ptr, (i - ota_idx));
                        ota_update_atcmd_set_tx_size(ota_tx_size);
                    }
                } else {
                    ota_offset_len = (strstr(buf, OTA_BY_MCU) + strlen(OTA_BY_MCU)) - ota_ptr;
                    if (ota_offset_len == i) {
                        ota_parse_flag = pdTRUE;
                        ota_ptr += i;
                        ota_idx = i;
                    }
                }
            } else {
                if ((ota_tx_size + ota_offset_len) == i) {
                    ota_tx_size = 0;
                    buf[i++] = ch;
                    return AT_CMD_ERR_CMD_OK;
                }
            }

            buf[i++] = ch;
            continue;
        }
#else
        ch = getchar_atcmd_uart(portMAX_DELAY);
#endif //(__OTA_UPDATE_MCU_FW__)

        if (ch == 0x00) {
            continue;
        }

        if (ch == ESCAPE_CODE) {
#if defined (__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
            printf_with_run_time("<ESC> Rx ");
#endif // __RUNTIME_CALCULATION_

            if (buf[0] != '\0') {
                return AT_CMD_ERR_WRONG_ARGUMENTS;
            }

            return gets_esc_atcmd_uart(buf);
        }

        // In case of brace role single-quotation or double-quotation
        if ((ch == '\"') || (ch == '\'')) {
            if (buf[i-1] == '=' || buf[i-1] == ',') {
                quotes = pdTRUE;
            }
        }

        // Check at command length
        if (i >= (USER_ATCMD_BUF - 2)) {
            if (ch == '\n' || ch == '\r') {
                ATCMD_DBG("[%s:%d]Input command too long. (input = %d, max = %d)\n", __func__, __LINE__, i, (USER_ATCMD_BUF - 2));
                return AT_CMD_ERR_WRONG_ARGUMENTS;
            }
            // Pass the remaining data larger than the MAX size
            i++;
            continue;
        }

#if defined (__SUPPORT_ATCMD_TLS__)
        if (   (i == (int)strlen(ATCMD_TLS_CERTSTORE_CMD))
            && (strncmp(buf, ATCMD_TLS_CERTSTORE_CMD, strlen(ATCMD_TLS_CERTSTORE_CMD)) == 0)
            && (ch == '=')) {

            buf[i++] = ch;
            gets_tls_atcmd_uart(buf);

            return AT_CMD_ERR_CMD_OK;
        } else if (   (i == (int)strlen(ATCMD_TLS_WR_CMD))
                   && (strncmp(buf, ATCMD_TLS_WR_CMD, strlen(ATCMD_TLS_WR_CMD)) == 0)
                   && (ch == '=')) {
            buf[i++] = ch;
            gets_tls_atcmd_uart(buf);

            return AT_CMD_ERR_CMD_OK;
        } else
#endif // __SUPPORT_ATCMD_TLS__
        {
            if (ch == '\b') {    /* backspace */
                i--;
                if (i < 0) {
                    i = 0;
                }
                buf[i] = '\0';
                PRINTF_ATCMD("\33[0K");
            } else if (ch == '\n' || ch == '\r') {
                if (quotes == pdTRUE) {
                    quotes = pdFALSE;        /* If there is \r\n inside the quotation marks */
                    if (buf[i-1] == '\'') {
                        buf[i++] = '\0';
                        return AT_CMD_ERR_CMD_OK;
                    }
                } else {
                    buf[i++] = '\0';
                    return AT_CMD_ERR_CMD_OK;
                } 
            } else {
                if (quotes == pdTRUE && ch == ',' && buf[i-1] == '\'') {
                    quotes = pdFALSE;
                }
                buf[i++] = ch;
            }
        }
    }

    return AT_CMD_ERR_WRONG_ARGUMENTS;
}

static int atcmd_is_valid_ip4_addr_uart(char *str_ip)
{
    int len = 0;
    char tail[16] = {0x00,};
    unsigned int d[4] = {0x00,};
    int ret = 0;
    int idx = 0;

    //check parameter
    if (str_ip == NULL) {
        return 0;
    }

    //check length
    len = strlen(str_ip);
    if (len < 7 || len > 15) {
        return 0;
    }

    // check dot
    ret = sscanf(str_ip, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);
    if (ret != 4 || tail[0]) {
        return 0;
    }

    // check range
    for (idx  = 0 ; idx < 4; idx++) {
        if (d[idx] > 255) {
            return 0;
        }
    }

    return is_in_valid_ip_class(str_ip);
}

#if defined ( __SUPPORT_ATCMD_TLS__ )
static int gets_tls_atcmd_certstore_uart(HANDLE uart_handler, char *buf, int buf_idx)
{
    typedef enum {
        READ_CERT_TYPE,         // 0
        READ_CERT_SEQUENCE,     // 1
        READ_CERT_FORMAT,       // 2
        READ_CERT_NAME,         // 3
        READ_CERT_DATA_LENGTH,  // 4
        READ_CERT_DATA          // 5
    } atcmd_certstore_cmd_step;

    const char *cert_prefix = "-----";
    int err = AT_CMD_ERR_CMD_OK;
    int is_cert_cmd_done = pdFALSE;
    int cert_format = ATCMD_CM_CERT_FORMAT_DER;

    atcmd_certstore_cmd_step p_step = READ_CERT_TYPE;

    char ch = 0;
    char p_buf[32] = {0x00,};
    unsigned int p_idx = 0;

    int data_len = 0;

    //AT+TRSSLCERTSTORE=<Certificate Type>,<Sequence>,<Format>,<Name>[,<Data length>],<Data>
    while (err == AT_CMD_ERR_CMD_OK && !is_cert_cmd_done) {
        switch (p_step) {
        case READ_CERT_TYPE:
        case READ_CERT_SEQUENCE:
        case READ_CERT_FORMAT:
        case READ_CERT_NAME:
        case READ_CERT_DATA_LENGTH:
            memset(p_buf, 0x00, sizeof(p_buf));
            p_idx = 0;
            ch = 0x00;

            while (ch != 0x2C) {
                if (p_idx >= sizeof(p_buf)) {
                    ATCMD_DBG("[%s:%d]p_idx:%d, sizeof(p_buf):%d\n",
                            __func__, __LINE__, p_idx, sizeof(p_buf));
                    err = AT_CMD_ERR_TOO_LONG_RESULT;
                    break;
                }

                ch = getchar_atcmd_uart(portMAX_DELAY);

                p_buf[p_idx++] = ch;

                if ((p_idx == strlen(cert_prefix))
                    && (p_step == READ_CERT_DATA_LENGTH)
                    && (cert_format == ATCMD_CM_CERT_FOMRAT_PEM)
                    && strncmp(p_buf, cert_prefix, strlen(cert_prefix)) == 0) {
                    //PEM format certificate
                    p_step = READ_CERT_DATA;
                    ATCMD_DBG("[%s:%d]There is no data length and input is pem format\n",
                            __func__, __LINE__);
                    break;
                }
            }

            //hex_dump((unsigned char *)p_buf, p_idx);

            memcpy(buf + buf_idx, p_buf, p_idx);
            buf_idx += p_idx;

            // Update step
            if (p_step == READ_CERT_TYPE) {
                p_step = READ_CERT_SEQUENCE;
            } else if (p_step == READ_CERT_SEQUENCE) {
                p_step = READ_CERT_FORMAT;
            } else if (p_step == READ_CERT_FORMAT) {
                p_buf[p_idx - 1] = '\0';

                if (get_int_val_from_str(p_buf, &cert_format, POL_1) != 0) {
                    err = AT_CMD_ERR_INSUFFICENT_ARGS;
                    ATCMD_DBG("[%s:%d]Failed to get cert format\n", __func__, __LINE__);
                    break;
                }

                p_step = READ_CERT_NAME;
            } else if (p_step == READ_CERT_NAME) {
                p_step = READ_CERT_DATA_LENGTH;
            } else if (p_step == READ_CERT_DATA_LENGTH) {
                p_buf[p_idx - 1] = '\0';

                if (get_int_val_from_str(p_buf, &data_len, POL_1) != 0) {
                    err = AT_CMD_ERR_INSUFFICENT_ARGS;
                    ATCMD_DBG("[%s:%d]Failed to get data_len\n", __func__, __LINE__);
                    break;
                }

                p_step = READ_CERT_DATA;
            }
            break;
        case READ_CERT_DATA:
            is_cert_cmd_done = pdTRUE;

            if (data_len > 0) {
                UART_READ(uart_handler, buf + buf_idx, data_len);
                buf_idx += data_len;
            } else {
                if (cert_format == ATCMD_CM_CERT_FORMAT_DER) {
                    ATCMD_DBG("[%s:%d]Invalid format(%d)\n", __func__, __LINE__, cert_format);
                    err = AT_CMD_ERR_INSUFFICENT_ARGS;
                    break;
                }

                while (1) {
                    ch = getchar_atcmd_uart(portMAX_DELAY);

                    if (buf_idx >= (USER_ATCMD_BUF - 1)) {
                        err = AT_CMD_ERR_TOO_LONG_RESULT;
                        break;
                    }

                    if (ch == '\b') {        /* backspace */
                        buf_idx--;
                        buf[buf_idx] = '\0';
                    } else if (ch == 0x0d) {
                        buf[buf_idx++] = 0x0a;
                    } else if (ch == 0x03 || ch == 0x1a) {
                        buf[buf_idx++] = '\0';
                        break;
                    } else {
                        buf[buf_idx++] = ch;
                    }
                }
            }

            //hex_dump((unsigned char *)buf, buf_idx);

            break;
        }
    }

    ATCMD_DBG("[%s:%d]err(%d)\n", __func__, __LINE__, err);

    return err;
}

static void gets_tls_atcmd_wr_uart(HANDLE uart_handler, char *buf, int buf_idx)
{
    typedef enum {
        READ_CID,
        READ_REMOTE_IP,
        READ_REMOTE_PORT,
        READ_MODE,
        READ_DATA_LENGTH,
        READ_DATA
    } atcmd_wr_cmd_step;

    char ch = 0;
    int start_msg = 0;

    const char *r_mode_str = "r";
    const char *t_mode_str = "t";

    char p_buf[32] = {0x00,};
    unsigned int p_idx = 0;
    atcmd_wr_cmd_step p_step = READ_CID;

    int data_len = 0;
    int r_mode = pdFALSE;
    int port = 0;

    while (1) {
        ch = getchar_atcmd_uart(portMAX_DELAY);

        if (buf_idx >= (USER_ATCMD_BUF - 2)) {
            ATCMD_DBG("[%s:%d]overflow(%d>=%d)\n", __func__, __LINE__,
                    buf_idx, (USER_ATCMD_BUF - 2));
            return ;
        }

        if (p_step == READ_DATA) {
            ATCMD_DBG("[%s:%d]Read data with %s mode(idx:%d, length:%ld)\n", __func__, __LINE__,
                    r_mode ? "R" : "T", buf_idx, data_len);

            if (r_mode) {
                buf[buf_idx++] = ch;

                if (data_len > 1) {
                    UART_READ(uart_handler, buf + buf_idx, data_len - 1);
                }

                buf_idx += (data_len - 1);

                break;
            } else {
                if (ch == '\b') {        /* backspace */
                    buf_idx--;
                    buf[buf_idx] = '\0';
                } else if (data_len == ((buf_idx + 1) - start_msg)) {
                    buf[buf_idx++] = ch;

                    if (ch == '\n' || ch == '\r') {
                        PRINTF_ATCMD("\r\n");
                    }
                    break;
                } else if (ch == 0x0d) {
                    buf[buf_idx++] = 0x0a;
                } else if (ch == 0x03 || ch == 0x1a) {
                    buf[buf_idx++] = '\0';
                    break;
                } else {
                    buf[buf_idx++] = ch;
                }
            }
        } else {
            if (p_idx > sizeof(p_buf)) {
                ATCMD_DBG("[%s:%d]overflow(%ld > %ld)\n", __func__, __LINE__,
                        p_idx, sizeof(p_buf));
                break;
            }

            if (ch != 0x2C) {
                p_buf[p_idx++] = ch;
            } else { // comma
                ATCMD_DBG("[%s:%d]p_step(%d)\n", __func__, __LINE__, p_step);

                if (p_step == READ_CID) {    // <cid>
                    sprintf(buf, "%s%s,", buf, p_buf);
                    buf_idx += (p_idx + 1);

                    p_step = READ_REMOTE_IP;
                } else if (p_step == READ_REMOTE_IP) {    // <remote_ip>
                    if (!atcmd_is_valid_ip4_addr_uart(p_buf)) {
                        p_step = READ_MODE;
                        goto read_mode;
                    }

                    sprintf(buf, "%s%s,", buf, p_buf);
                    buf_idx += (p_idx + 1);

                    p_step = READ_REMOTE_PORT;
                } else if (p_step == READ_REMOTE_PORT) {    // <remote_port>
                    if ((get_int_val_from_str(p_buf, &port, POL_1) != 0)
                            || (port < 0) || (port > 0xFFFF)) {
                        break;
                    }

                    sprintf(buf, "%s%s,", buf, p_buf);
                    buf_idx += (p_idx + 1);

                    p_step = READ_MODE;
                } else if (p_step == READ_MODE) {    // <<mode>>
read_mode:
                    p_step = READ_DATA_LENGTH;

                    if (strcasecmp(p_buf, r_mode_str) == 0) {
                        r_mode = pdTRUE;
                    } else if (strcasecmp(p_buf, t_mode_str) == 0) {
                        r_mode = pdFALSE;
                    } else {
                        goto read_data_length;
                    }
                } else if (p_step == READ_DATA_LENGTH) {    //<data_length>
read_data_length:
                    if ((get_int_val_from_str(p_buf, &data_len, POL_1) != 0)
                            || (data_len <= 0) || (data_len > TX_PAYLOAD_MAX_SIZE)) {
                        break;
                    }

                    sprintf(buf, "%s%s,", buf, p_buf);
                    buf_idx += (p_idx + 1);

                    p_step = READ_DATA;

                    start_msg = buf_idx;
                }

                memset(p_buf, 0x00, sizeof(p_buf));
                p_idx = 0;
            }
        }
    }

    return ;
}

static void gets_tls_atcmd_uart(char *buf)
{
    HANDLE uart_handler = NULL;;

    char *pos = NULL;
    int buf_idx = 0;

#if defined ( __ATCMD_IF_UART2__ )
    uart_handler = uart2;
#else
    uart_handler = uart1;
#endif // (__ATCMD_IF_UART2__)

    //AT+TRSSLCERTSTORE
    pos = strstr(buf, ATCMD_TLS_CERTSTORE_CMD);
    if (pos) {
        buf_idx = strlen(ATCMD_TLS_CERTSTORE_CMD) + 1;

        gets_tls_atcmd_certstore_uart(uart_handler, buf, buf_idx);

        return ;
    }

    // AT+TRSSLWR
    pos = strstr(buf, ATCMD_TLS_WR_CMD);
    if (pos) {
        buf_idx = strlen(ATCMD_TLS_WR_CMD) + 1;

        gets_tls_atcmd_wr_uart(uart_handler, buf, buf_idx);

        return ;
    }

    return ;
}
#endif // __SUPPORT_ATCMD_TLS__
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

char *atcmd_dpm_wakeup_status(void)
{
    unsigned long wakeup_src;
#if defined (__SUPPORT_NOTIFY_RTC_WAKEUP__)
    if (dpm_mode_is_enabled() == pdFALSE) {
        return NULL;
    }

    if (dpm_mode_is_wakeup() == pdFALSE) {
        return "POR";
    }
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__

    // DPM Wakeup ...
    wakeup_src = da16x_get_wakeup_source();

    if (wakeup_src == DPM_UC) {
        return "UC";
    } else if (wakeup_src == DPM_NO_BCN) {
        return "NOBCN";
    } else if (wakeup_src == DPM_KEEP_ALIVE_NO_ACK) {
        return "NOACK";
    } else if (wakeup_src == DPM_DEAUTH) {
        return "DEAUTH";
    } else if (dpm_mode_get_wakeup_source() == WAKEUP_EXT_SIG_WITH_RETENTION) {
        return "EXT";
    }
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
      else if (wakeup_src == DPM_UNDEFINED) {
        return "UNDEFINED";
    }
#endif // __SUPPORT_DPM_ABNORM_MSG__
#ifdef    __DPM_WAKEUP_NOTICE_ADDITIONAL__
       else if (wakeup_src == DPM_FROM_FAST) {
        return "RTC";
    } else {
        return "ETC";
    }
#else
    else {
        return NULL;
    }
#endif /* __DPM_WAKEUP_NOTICE_ADDITIONAL__ */
}

int atcmd_dpm_wait_ready(int timeout)
{
    int wait_count = timeout * 100;

    if (!dpm_mode_is_wakeup()) {
         return 0;
    }

    if (g_is_atcmd_init) {
         return 0;
    }

    while (1) {
        if (atcmd_dpm_ready == pdTRUE) {
            break;
        }

        vTaskDelay(1);

        if (--wait_count == 0) {
            return -1;
        }
    }

    return 0;
}

static int display_help(char *cmd)
{
    const command_t    *cmd_ptr;
    int  status = AT_CMD_ERR_UNKNOWN_CMD;

    for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
        if (strcasecmp(cmd, cmd_ptr->name) == 0) {
            status = AT_CMD_ERR_CMD_OK;

            if (   (cmd_ptr->format == (char *)"" || cmd_ptr->format == (char *)NULL)
                && (cmd_ptr->brief == (char *)"" || cmd_ptr->brief == (char *)NULL)) {
                PRINTF_ATCMD("\r\n   - No example for %s\r\n", cmd);
            } else {
                PRINTF_ATCMD("\r\n %s%s%s\r\n", cmd_ptr->name,
                             (cmd_ptr->arg_count > 0 ? DELIMIT_FIRST : DELIMIT),
                             (cmd_ptr->format != (char *)NULL && cmd_ptr->format != (char *)"") ? cmd_ptr->format : "");

                if ((cmd_ptr->brief != (char *)NULL) && (cmd_ptr->brief != (char *)"")) {
                    PRINTF_ATCMD("   - %s\r\n", cmd_ptr->brief);
                }
            }
        }
    }

    return status;
}

atcmd_error_code command_parser(char *line)
{
    const command_t    *cmd_ptr = NULL;
    char *params[MAX_PARAMS] = {0,};
    uint32_t param_cnt = 0;
    int  length_all = strlen(line);
    char *tmp_line = NULL;

    atcmd_error_code    major_err_code = AT_CMD_ERR_CMD_OK;

#if defined (__ATCMD_IF_SPI__) || defined (__ATCMD_IF_SDIO__)
    at_command_table = commands;
#endif // (__ATCMD_IF_SPI__) || (__ATCMD_IF_SDIO__)

    /* Sanity check */
    if (length_all > USER_ATCMD_BUF) {
        PRINTF("[%s] AT-CMD length is exceeded limits(%d)\n", __func__, length_all);
        return AT_CMD_ERR_UNKNOWN;
    }

    /* To check if first character is ',' or not */
    tmp_line = pvPortMalloc(length_all + 2);
    if (tmp_line == NULL) {
        PRINTF("[%s] Failed to allocate memory !!!\n", __func__);
        return AT_CMD_ERR_MEM_ALLOC;
    }
    memset(tmp_line, 0, length_all + 2);
    strncpy(tmp_line, line, length_all);

    /* First call to strtok. */
    if (strchr(tmp_line, '=') == NULL) {
        params[param_cnt++] = strtok(tmp_line, DELIMIT);    /* ' ' */
    } else {
        params[param_cnt++] = strtok(tmp_line, DELIMIT_FIRST);    /* '=' */
    }

    if (params[0] == NULL) {    /* no command entered */
        major_err_code = AT_CMD_ERR_UNKNOWN_CMD;
    } else {
        /* find the command */
        for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if (strcasecmp(params[0], cmd_ptr->name) == 0) {
                break;
            }
        }

        if (cmd_ptr->name == NULL) {
            line[strlen(params[0])] = '=';
            major_err_code = AT_CMD_ERR_UNKNOWN_CMD;
        } else {
            /* To check if first character is ',' or not */
            /* Command length + '=' */
            if ((tmp_line[strlen(params[0])] == '=' || tmp_line[strlen(params[0])] == ' ') &&
                 tmp_line[strlen(params[0]) + 1] == ',') {
                    major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_result;
            }

            if (strstr(tmp_line, ",,") != NULL) {
                major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_result;
            }

            /* parse arguments */
            while (((params[param_cnt] = strtok(NULL, DELIMIT_COMMA)) != NULL)) {
                if (params[param_cnt][0] == '\'') {
                    char *tmp_ptr;

                    // Restore delimit-character
                    params[param_cnt][strlen(params[param_cnt])] = ',';

                    if (strncmp(params[param_cnt], "',", 2) == 0) {
                        // First argument : AT+XXX=',aaaaa','bbbb'
                        if (param_cnt == 1) {
                            if ((tmp_ptr = strstr(&params[param_cnt][1], "',")) != NULL) {
                                params[param_cnt] = params[param_cnt] + 1;
                                strtok(tmp_ptr, DELIMIT_COMMA);
                                *tmp_ptr = '\0';
                                *(tmp_ptr + 1) = '\0';
                            }
                        } else {
                            if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                                tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                                params[param_cnt] = params[param_cnt] + 1;
                                strtok(tmp_ptr, "'");
                                *tmp_ptr = '\0';
                            } else {
                                major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                                goto atcmd_result;
                            }
                        }
                    } else if ((tmp_ptr = strstr(params[param_cnt], "',")) != NULL) {
                        params[param_cnt] = params[param_cnt] + 1;
                        strtok(tmp_ptr, DELIMIT_COMMA);
                        *tmp_ptr = '\0';
                        *(tmp_ptr + 1) = '\0';
                    } else if (params[param_cnt][strlen(params[param_cnt]) - 1] == '\'') {
                        tmp_ptr = params[param_cnt] + strlen(params[param_cnt]) - 1;
                        params[param_cnt] = params[param_cnt] + 1;
                        strtok(tmp_ptr, "'");
                        *tmp_ptr = '\0';
                    } else {
                        major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_result;
                    }
                } else {
                    if (strstr(params[param_cnt], "'") != NULL) {
                        major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_result;
                    }
                }

                param_cnt++;

                if (param_cnt > (MAX_PARAMS - 1)) {
                    major_err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                    break;
                }
            }

            /* check arguments length */
            if (   (param_cnt > 1)
                && ((length_all - strlen(params[0]) - 1) > TX_PAYLOAD_MAX_SIZE) ) {
                /* Check single quotation mark in payload */
                if (params[1][0] == '\'') {
                    if ((length_all - strlen(params[0]) - 3) > TX_PAYLOAD_MAX_SIZE) {
                        major_err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_result;
                    }
                }
            }

            /* check arguments */
            if (param_cnt - 1 > (unsigned int)cmd_ptr->arg_count) {
                if (strcasecmp(params[1], "?") && strcasecmp(params[1], "HELP")) {
                    /* param cnt exceeding check */
                    major_err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                } else if (cmd_ptr->arg_count == 0 && strcasecmp(params[1], "?") == 0) {
                    /* error on <cmd>=? where <cmd> has no params */
                    major_err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                }
            }

            /* run command */
            if ((major_err_code == AT_CMD_ERR_CMD_OK) && (cmd_ptr->command != NULL)) {
                ATCMD_DBG("\n");

#if defined (DEBUG_ATCMD)
                for (int i = 0; i < (int)param_cnt; i++) {
                    ATCMD_DBG("argv[%d]:%s\n", i, params[i]);
                }
#endif // DEBUG_ATCMD

                if (strcasecmp(params[1], "HELP") == 0) {
                    display_help(params[0]);
                } else {
                    major_err_code = (atcmd_error_code)cmd_ptr->command(param_cnt, params);
                }
            }
        }

atcmd_result:
        if (major_err_code == AT_CMD_ERR_CMD_OK) {
            if (strcasecmp(params[0], "ATB") != 0
#if defined ( __SUPPORT_PASSIVE_SCAN__ )
                && strcasecmp(params[0]+3, "WFPSCAN") != 0
#endif  // __SUPPORT_PASSIVE_SCAN__
            ) {
                if (atq_result == 1) {
                    PRINTF_ATCMD("\r\nOK\r\n");
                }

                atcmd_mqtt_msg_qos0_dpm_rsp_handle();
            }
        } else if (major_err_code == AT_CMD_ERR_CMD_OK_WO_PRINT) {
            /* Not Printed */
        } else if (major_err_code != AT_CMD_ERR_UNKNOWN_CMD) {
            if (atq_result == 1) {
                PRINTF_ATCMD("\r\nERROR:%d\r\n", major_err_code);
            }
        }

        if (major_err_code != AT_CMD_ERR_UNKNOWN_CMD) {
            if (atq_result == 1)
                ATCMD_DBG("\r\nUsage: %s=%s\r\n", cmd_ptr->name, cmd_ptr->format);
        }
    }

    /* To check if first character is ',' or not */
    if (tmp_line != NULL) {
        vPortFree(tmp_line);
    }

    return major_err_code;
}


/******************************************************************************
 * Function Name : help_command
 * Discription : help for at cmd
 * Arguments : argc, argv[]
 * Return Value : result_code
 *****************************************************************************/
static int help_command(int argc, char *argv[])
{
    const command_t *cmd_ptr;
    atcmd_error_code    result_code = AT_CMD_ERR_CMD_OK;

    switch (argc) {
    case 0:
    case 1: { /* help or ?*/
LIST_ALL:
        PRINTF_ATCMD("\r\nAT Commands:\r\n");

        for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if ((cmd_ptr->format != (char *)NULL) && (cmd_ptr->format != (char *)"")) {
                PRINTF_ATCMD("    %s%s%s\r\n", cmd_ptr->name,
                             (cmd_ptr->arg_count > 0 ? DELIMIT_FIRST : DELIMIT), cmd_ptr->format);
            } else {
                PRINTF_ATCMD("    %s\r\n", cmd_ptr->name);
            }

            if ((cmd_ptr->brief != (char *)NULL) && (cmd_ptr->brief != (char *)"")) {
                PRINTF_ATCMD("        - %s\r\n", cmd_ptr->brief);
            } else {
                PRINTF_ATCMD("        - No example for %s\r\n", cmd_ptr->name);
            }
        }
    } break;

    case 2: { /* help <cmd>, ?=? */
        if (strcasecmp(argv[0], "?") == 0) {
            if (strcasecmp(argv[1], "?") == 0) {
                goto LIST_ALL;
            } else {
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                break;
            }
        }

        result_code = AT_CMD_ERR_UNKNOWN_CMD;

        for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
            if (strcasecmp(argv[1], cmd_ptr->name) == 0) {
                result_code = AT_CMD_ERR_CMD_OK;

                if ((cmd_ptr->format == (char *)"" || cmd_ptr->format == (char *)NULL)
                        && (cmd_ptr->brief == (char *)"" || cmd_ptr->brief == (char *)NULL)) {
                    PRINTF_ATCMD("\r\n        - No example for %s\r\n", argv[1]);
                } else {
                    PRINTF_ATCMD("\r\n    %s%s%s\r\n", cmd_ptr->name,
                                 (cmd_ptr->arg_count > 0 ? DELIMIT_FIRST : DELIMIT),
                                 (cmd_ptr->format != (char *)NULL && cmd_ptr->format != (char *)"") ? cmd_ptr->format : "");

                    if ((cmd_ptr->brief != (char *)NULL) && (cmd_ptr->brief != (char *)"")) {
                        PRINTF_ATCMD("          - %s\r\n", cmd_ptr->brief);
                    }
                }
                return result_code;
            }
        }
    } break;
    } // End of Switch

    /* For USER AT-CMD */
    if (user_atcmd_help_cb_fn != NULL) {
        result_code = user_atcmd_help_cb_fn(argv[1]);
    }

    return result_code;
}

int atcmd_standard(int argc, char *argv[])
{
    int    status = AT_CMD_ERR_CMD_OK;
    char result_str[128] = {0, };

    if (strcasecmp(argv[0], "AT") == 0) {
        ;
    } else if (strcasecmp(argv[0], "AT+") == 0) {
        const command_t *cmd_ptr;

        if (argc > 1 && strcasecmp(argv[1], "?") != 0) {
            status = AT_CMD_ERR_WRONG_ARGUMENTS;
        }

        if (status == AT_CMD_ERR_CMD_OK) {
            for (cmd_ptr = at_command_table; cmd_ptr->name != NULL; cmd_ptr++) {
                if (strncasecmp(cmd_ptr->name, "AT", 2) == 0) {
                    PRINTF_ATCMD("\r\n%s", cmd_ptr->name);
                }
            }
        }
    } else if (strcasecmp(argv[0], "ATZ") == 0) {
        atq_result = 1;
        atcmd_uart_echo = pdFALSE;
        sprintf(result_str, "%s", "Display result on\nEcho off");
    } else if (strcasecmp(argv[0], "ATE") == 0) {
        if (argc == 1) {
            atcmd_uart_echo == 0 ? (atcmd_uart_echo = 1) : (atcmd_uart_echo = 0);
        }

        if (argc != 1 && !is_correct_query_arg(argc, argv[1]))
            status = AT_CMD_ERR_WRONG_ARGUMENTS;

        if (atcmd_uart_echo == 1) {
            sprintf(result_str, "%s", "Echo on");
        } else {
            sprintf(result_str, "%s", "Echo off");
        }
    } else if (strcasecmp(argv[0], "ATQ") == 0) {
        if (argc == 1) {
            atq_result == 0 ? (atq_result = 1) : (atq_result = 0);
        }

        if (argc != 1 && !is_correct_query_arg(argc, argv[1]))
            status = AT_CMD_ERR_WRONG_ARGUMENTS;

        if (atq_result == 1) {
            sprintf(result_str, "%s", "Display result on");
        } else {
            sprintf(result_str, "%s", "Display result off");
        }
    } else if (strcasecmp(argv[0], "ATF") == 0) {
#if defined (__SUPPORT_ATCMD_TLS__)
        atcmd_transport_ssl_reboot();
#endif // (__SUPPORT_ATCMD_TLS__)

        status = factory_reset(0);
        reboot_func(SYS_REBOOT);
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (strlen(result_str) > 0 && status == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\n%s", result_str);
    }

    return status;
}

int atcmd_basic(int argc, char *argv[])
{
    int  err_code = AT_CMD_ERR_CMD_OK;
    int  status;
    int  reboot = -1;

    char result_str[128] = {0, };
    int  result_int;
    int  tmp_int1 = 0;

    if (strcasecmp(argv[0] + 3, "RESTART") == 0) {
        reboot = SYS_REBOOT;
    } else if (strcasecmp(argv[0] + 3, "RESET") == 0) {
        reboot = SYS_RESET;
    } else if (strcasecmp(argv[0] + 3, "CHIPNAME") == 0) {
        strcpy(result_str, CHIPSET_NAME);
    } else if (strcasecmp(argv[0] + 3, "VER") == 0) {
        unsigned char rtos_img_ver[32];

        get_firmware_version(2, rtos_img_ver);

        sprintf(result_str, "%s", rtos_img_ver);
    } else if (strcasecmp(argv[0] + 3, "SDKVER") == 0) {
        atcmd_sdk_version();
    } else if (strcasecmp(argv[0] + 3, "TIME") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+TIME=? */
            struct tm *ts;
#if defined (__TIME64__)
            __time64_t now;

            da16x_time64(NULL, &now);
            ts = (struct tm *) da16x_localtime64(&now);
#else
            time_t now;

            now = da16x_time32(NULL);
            ts = da16x_localtime32(&now);
#endif // (__TIME64__)

            da16x_strftime(result_str, sizeof (result_str), "%Y-%m-%d,%H:%M:%S", ts);
        } else if (argc == 3) {
            /* AT+NWTIME=<date>,<time> */
            status = cmd_set_time(argv[1], argv[2], 0);
            if (status != 0) {
                switch (status) {
                case -1 : err_code = AT_CMD_ERR_BASIC_ARG_NULL_PTR;    break;
                case -2 : err_code = AT_CMD_ERR_BASIC_ARG_DATE;        break;
                case -3 : err_code = AT_CMD_ERR_BASIC_ARG_TIME;        break;
                default : err_code = AT_CMD_ERR_BASIC_ARG_TIME_ETC;    break;
                }
            }
        } else {
            // Insufficient arguments
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 3, "RLT") == 0) {
#if defined (__TIME64__)
        __time64_t uptime = 0;
        __uptime(&uptime);
#else
        time_t uptime;
        uptime = __uptime();
#endif // (__TIME64__)
        sprintf(result_str, "%lu,%02lu:%02lu.%02lu\n",
                (unsigned long)uptime / (24 * 3600), (unsigned long)uptime % (24 * 3600) / 3600,
                (unsigned long)uptime % (3600) / 60, (unsigned long)uptime % 60);
    } else if (strcasecmp(argv[0] + 3, "TZONE") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+TZONE=? */
            da16x_get_config_int(DA16X_CONF_INT_GMT, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+TZONE=<sec> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;

                goto atcmd_basic_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_GMT, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    } else if (strcasecmp(argv[0] + 3, "DEFAP") == 0) {
        result_int = da16x_set_default_ap_config();

        if (result_int) {
            err_code = AT_CMD_ERR_UNKNOWN;
        } else {
            reboot = SYS_REBOOT;
        }
    } else if (strcasecmp(argv[0] + 3, "HOSTINITDONE") == 0) {
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
        int abn_type = get_last_abnormal_act();
#endif // __SUPPORT_DPM_ABNORM_MSG__

        if (!dpm_mode_is_wakeup()) {
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
            if (abn_type != 0) {
                PRINTF_ATCMD("\r\n+INIT:WAKEUP,ABNORM_%d\r\n", abn_type);
            } else
#endif // __SUPPORT_DPM_ABNORM_MSG__
                PRINTF_ATCMD("\r\n+INIT:DONE,%d\r\n", get_run_mode());
        } else {
#ifdef    __DPM_WAKEUP_NOTICE_ADDITIONAL__
            PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
#else
            if (atcmd_dpm_wakeup_status() != NULL
#if defined (__SUPPORT_NOTIFY_RTC_WAKEUP__)
            && strcmp(atcmd_dpm_wakeup_status(), "POR") != 0)
#else
            )
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__
            {
#if defined (__SUPPORT_DPM_ABNORM_MSG__)
                if (strcmp(atcmd_dpm_wakeup_status(), "UNDEFINED") == 0 &&
                    abn_type != 0) {
                    PRINTF_ATCMD("\r\n+INIT:WAKEUP,ABNORM_%d\r\n", abn_type);
                } else
#endif // __SUPPORT_DPM_ABNORM_MSG__
                    PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
            }
#endif // (__DPM_WAKEUP_NOTICE_ADDITIONAL__)
        }
    } else if (strcasecmp(argv[0] + 3, "BIDX") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+BIDX=? */
            extern int get_cur_boot_index(void );

            result_int = get_cur_boot_index();
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+BIDX=<idx> */

            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0)) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_basic_end;
            }

            if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_basic_end;
            }

            if (set_boot_index(tmp_int1) != pdTRUE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    } else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (reboot != -1) {
        PRINTF_ATCMD("\r\nOK\r\n");
#if defined (__SUPPORT_ATCMD_TLS__)
        atcmd_transport_ssl_reboot();
#endif // (__SUPPORT_ATCMD_TLS__)
        vTaskDelay(100);
        reboot_func(reboot);
    }

atcmd_basic_end:
    if ((strlen(result_str) > 0 && strncmp(result_str, "OK", 2) != 0) && err_code == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return err_code;
}

int atcmd_dpm(int argc, char *argv[])
{
    int  err_code = AT_CMD_ERR_CMD_OK;
    char result_str[128] = {0, };
    int  result_int;
    int  reboot = -1;
    int  tmp_int1 = 0, tmp_int2 = 0;
    int  status;

    // AT+DPM
    if (strcasecmp(argv[0] + 3, "DPM") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            // Check Fast_reconnect config
            da16x_get_config_int(DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2, &result_int);
            if (result_int != 0) {
                err_code = AT_CMD_ERR_DPM_FAST_CONN_EN;
                goto atcmd_dpm_end;
            }

            /* AT+DPM=? */
            da16x_get_config_int(DA16X_CONF_INT_DPM, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2 || argc == 3) {
            // Check RF-Test mode : Does not support DPM operation
            if (get_wlaninit() == pdFALSE) {
                err_code = AT_CMD_ERR_NOT_SUPPORTED;
                goto atcmd_dpm_end;
            }

            // Check Fast_reconnect config
            da16x_get_config_int(DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2, &result_int);
            if (result_int != 0) {
                err_code = AT_CMD_ERR_DPM_FAST_CONN_EN;
                goto atcmd_dpm_end;
            }

            /* AT+DPM=<dpm>[,<nvm_only>] */
            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) || (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE)) {
                err_code = AT_CMD_ERR_DPM_MODE_ARG;
                goto atcmd_dpm_end;
            }

            // Check running mode : Needs STA only
            if (get_run_mode() != SYSMODE_STA_ONLY) {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                goto atcmd_dpm_end;
            }

            if (argc == 3) {
                if ((get_int_val_from_str(argv[2], &tmp_int2, POL_1) != 0) || (is_in_valid_range(tmp_int2, 0, 1) == pdFALSE)) {
                    err_code = AT_CMD_ERR_DPM_NVRAM_FLAG_ARG;
                    goto atcmd_dpm_end;
                }

                if (tmp_int2 == 1) {
                    write_nvram_int(NVR_KEY_DPM_MODE, tmp_int1);
                    goto atcmd_dpm_end;
                }
            }

            if (da16x_set_config_int(DA16X_CONF_INT_DPM, tmp_int1) != CC_SUCCESS) {
                /*
                 * Since the range of the input variable has been checked
                 * in the previous part, no error occurs.
                 */
                err_code = AT_CMD_ERR_DPM_MODE_ARG;
                goto atcmd_dpm_end;
             }

            reboot = SYS_REBOOT;
        } else {
            /*
             * Since the range of the input variable has been checked in advance
             * in the previous part, no error occurs.
             */
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+DPMKA
    else if (strcasecmp(argv[0] + 3, "DPMKA") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+DPMKA=? */
            read_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, &result_int);

            if (result_int == -1) {
                sprintf(result_str, "%d", DFLT_DPM_KEEPALIVE_TIME);
            } else {
                sprintf(result_str, "%d", result_int);
            }
        } else if (argc == 2) {
            /* AT+DPMKA=<period> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_dpm_end;
            }

            if ((tmp_int1 < MIN_DPM_KEEPALIVE_TIME) || (tmp_int1 > MAX_DPM_KEEPALIVE_TIME)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_dpm_end;
            }

            status = write_nvram_int((const char *)NVR_KEY_DPM_KEEPALIVE_TIME, tmp_int1);
            if (status == -1) {
                err_code = AT_CMD_ERR_DPM_SLEEP_STARTED;
            } else if (status == -2) {
                err_code = AT_CMD_ERR_NVRAM_WRITE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+DPMTIMWU
    else if (strcasecmp(argv[0] + 3, "DPMTIMWU") == 0) {

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+DPMTIMWU=? */
            read_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, &result_int);

            if (result_int == -1) {
                sprintf(result_str, "%d", DFLT_DPM_TIM_WAKEUP_COUNT);
            } else {
                sprintf(result_str, "%d", result_int);
            }
        } else if (argc == 2) {
            /* AT+DPMTIMWU=<time> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_dpm_end;
            }

            if ((tmp_int1 < MIN_DPM_TIM_WAKEUP_COUNT) || (tmp_int1 > MAX_DPM_TIM_WAKEUP_COUNT)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_dpm_end;
            }

            status = write_nvram_int((const char *)NVR_KEY_DPM_TIM_WAKEUP_TIME, tmp_int1);
            if (status == -1) {
                err_code = AT_CMD_ERR_DPM_SLEEP_STARTED;
            } else if (status == -2) {
                err_code = AT_CMD_ERR_NVRAM_WRITE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+DPMUSERWU
    else if (strcasecmp(argv[0] + 3, "DPMUSERWU") == 0) {

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+DPMTIMWU=? */
            read_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, &result_int);
            if (result_int == -1) {
                sprintf(result_str, "%d", DFLT_DPM_USER_WAKEUP_TIME);
            } else {
                sprintf(result_str, "%d", result_int);
            }
        } else if (argc == 2) {
            /* AT+DPMTIMWU=<time> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_dpm_end;
            }

            if ((tmp_int1 < MIN_DPM_USER_WAKEUP_TIME) || (tmp_int1 > MAX_DPM_USER_WAKEUP_TIME)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_dpm_end;
            }

            status = write_nvram_int((const char *)NVR_KEY_DPM_USER_WAKEUP_TIME, tmp_int1);
            if (status == -1) {
                err_code = AT_CMD_ERR_DPM_SLEEP_STARTED;
            } else if (status == -2) {
                err_code = AT_CMD_ERR_NVRAM_WRITE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
#if defined (__SUPPORT_DPM_EXT_WU_MON__)
    // AT+SETDPMSLPEXT
    else if (strcasecmp(argv[0] + 3, "SETDPMSLPEXT") == 0) {
        if (dpm_mode_is_enabled() == pdTRUE) {
            ext_wu_sleep_flag = pdTRUE;
        } else {
            err_code = AT_CMD_ERR_DPM_MODE_DISABLED;
        }
    }
    // AT+CLRDPMSLPEXT
    else if (strcasecmp(argv[0] + 3, "CLRDPMSLPEXT") == 0) {
        if (dpm_mode_is_enabled() == pdTRUE) {
            ext_wu_sleep_flag = pdFALSE;
        } else {
            err_code = AT_CMD_ERR_DPM_MODE_DISABLED;
        }
    }
    // AT+SETSLEEP2EXT
    else if (strcasecmp(argv[0] + 3, "SETSLEEP2EXT") == 0) {
        if (argc == 3) {
            unsigned long long tmp_msec = 0x0ULL;

            /* AT+SETSLEEP2EXT=<period>,<rtm> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_DPM_SLP2_PERIOD_TYPE;
                goto atcmd_dpm_end;
            } else if (is_in_valid_range(tmp_int1, 0x3E8, 0x7CFFFC18) == pdFALSE) {
                err_code = AT_CMD_ERR_DPM_SLP2_PERIOD_RANGE;
                goto atcmd_dpm_end;
            } else {
                tmp_msec = (unsigned long long)tmp_int1;
            }

            if (get_int_val_from_str(argv[2], &tmp_int2, POL_1) != 0) {
                err_code = AT_CMD_ERR_DPM_SLP2_RTM_FLAG_ARG;
                goto atcmd_dpm_end;
            } else if (is_in_valid_range(tmp_int2, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_DPM_SLP2_RTM_FLAG_RANGE;
                goto atcmd_dpm_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_DPM, &result_int);
            if (result_int == 1) {
                err_code = AT_CMD_ERR_DPM_SLP2_DPM_MODE_ENABLED;
                goto atcmd_dpm_end;
            }

#if defined (__BLE_COMBO_REF__)
            int tmp_sec = 0;
            tmp_sec = tmp_int1/1000; 
            result_int = combo_set_sleep2("at", tmp_int2, tmp_sec);
            if (result_int < 0) {
                if (result_int == -1) {
                    err_code = AT_CMD_ERR_INSUFFICENT_CONFIG;
                } else if (result_int == -2) {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                } else if (result_int == -3) {
                    err_code = AT_CMD_ERR_UNKNOWN;
                }
                goto atcmd_dpm_end;
            } else if (result_int == 0) {
                err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;
                return err_code;
            }
#endif // __BLE_COMBO_REF__

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            dpm_sleep_start_mode_2(tmp_msec * 1000ULL, tmp_int2);

            return err_code;
        } else if (argc < 3) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+SETSLEEP3EXT
    else if (strcasecmp(argv[0] + 3, "SETSLEEP3EXT") == 0) {
        if (argc == 2) {
            unsigned long long tmp_msec = 0x0ULL;

            /* AT+SETSLEEP3EXT=<period> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_DPM_SLP3_PERIOD_TYPE;
                goto atcmd_dpm_end;
            } else if (is_in_valid_range(tmp_int1, 0x3E8, 0x7CFFFC18) == pdFALSE) {
                err_code = AT_CMD_ERR_DPM_SLP3_PERIOD_RANGE;
                goto atcmd_dpm_end;
            } else {
                tmp_msec = (unsigned long long)tmp_int1;
            }            
#if defined (__BLE_COMBO_REF__)
            int tmp_sec = 0;
            tmp_sec = tmp_int1/1000; 
            result_int = combo_set_sleep2("at", 1, tmp_sec);
            if (result_int < 0) {
                if (result_int == -1) {
                    err_code = AT_CMD_ERR_INSUFFICENT_CONFIG;
                } else if (result_int == -2) {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                } else if (result_int == -3) {
                    err_code = AT_CMD_ERR_UNKNOWN;
                }
                goto atcmd_dpm_end;
            } else if (result_int == 0) {
                err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;
                return err_code;
            }
#endif // __BLE_COMBO_REF__

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            dpm_sleep_start_mode_3(tmp_msec * 1000ULL);

            return err_code;
        } else if (argc < 3) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    //    Deprecated - AT+SETSLEEP1EXT
    else if (strcasecmp(argv[0] + 3, "SETSLEEP1EXT") == 0) {
        if (argc == 2) {
            /* AT+SETSLEEP1EXT=<rtm> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_DPM_SLP1_RTM_FLAG_ARG;
                goto atcmd_dpm_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_DPM_SLP1_RTM_FLAG_RANGE;
                goto atcmd_dpm_end;
            }

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            dpm_sleep_start_mode_2(0, tmp_int1);

            return err_code;
        } else if (argc < 2) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // (__SUPPORT_DPM_EXT_WU_MON__)
#if defined ( __SUPPORT_FASTCONN__ )
    //    AT+GETFASTCONN
    else if (strcasecmp(argv[0] + 3, "GETFASTCONN") == 0) {
        // Check DPM mode
        if (dpm_mode_is_enabled() == pdTRUE) {
            err_code = AT_CMD_ERR_DPM_MODE_DISABLED;
            goto atcmd_dpm_end;
        }

        /* AT+GETFASTCONN=? */
        da16x_get_config_int(DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2, &result_int);

        sprintf(result_str, "%d", result_int);
    }
    //    AT+SETFASTCONN
    else if (strcasecmp(argv[0] + 3, "SETFASTCONN") == 0) {
        if (argc == 2) {
            /* AT+SETFASTCONN=<0|1> */
            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_2) != 0 /*SUCCESS*/) ||
                (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE)) {
                err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_dpm_end;
            }

            // Check DPM mode
            if (dpm_mode_is_enabled() == pdTRUE) {
                err_code = AT_CMD_ERR_DPM_MODE_DISABLED;
                goto atcmd_dpm_end;
            }

            da16x_set_config_int(DA16X_CONF_INT_FAST_CONNECT_SLEEP1_2, tmp_int1);
            da16x_set_config_int(DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP, tmp_int1);

            return err_code;
        } else if (argc < 2) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __SUPPORT_FASTCONN__
    else if (strcasecmp(argv[0] + 3, "MCUWUDONE") == 0) {
        atcmd_dpm_ready = pdTRUE;
    }
#if !defined ( __DISABLE_DPM_ABNORM__ )
    //    AT+DPMABN
    else if (strcasecmp(argv[0] + 3, "DPMABN") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+DPMABN=? */
            da16x_get_config_int(DA16X_CONF_INT_DPM_ABN, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+DPMABN=<flag> */
            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) || (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE)) {
                err_code = AT_CMD_ERR_DPM_ABN_ARG;
                goto atcmd_dpm_end;
            }

            if (get_run_mode() != SYSMODE_STA_ONLY) {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                goto atcmd_dpm_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_DPM_ABN, tmp_int1) != CC_SUCCESS) {
                /*
                 * Since the range of the input variable has been checked
                 * in the previous part, no error occurs.
                 */
                err_code = AT_CMD_ERR_DPM_ABN_ARG;
                goto atcmd_dpm_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // !__DISABLE_DPM_ABNORM__
#if defined (__WF_CONN_RETRY_CNT_ABN_DPM__)
    //    AT+DPMABNWFCCNT
    else if (strcasecmp(argv[0] + 3, "DPMABNWFCCNT") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+DPMABNWFCCNT=? */
            int wifi_conn_retry_cnt;

            if (read_nvram_int(NVR_KEY_DPM_AB_WF_CONN_RETRY, &wifi_conn_retry_cnt) != 0) {
                wifi_conn_retry_cnt = 0;
            }

            sprintf(result_str, "%d", wifi_conn_retry_cnt);
        } else if (argc == 2) {
            /* AT+DPMABNWFCCNT=<count> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) == 0 /*SUCCESS*/) {
                if (is_in_valid_range(tmp_int1, 0, 6) == pdFALSE) {
                    err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                } else {
                    // correct range
                    extern int set_dpm_abnormal_wait_time(int count, int mode);
                    set_dpm_abnormal_wait_time(tmp_int1, DPM_ABNORM_DPM_WIFI_RETRY_CNT);
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
            }
        } else if (argc < 2) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __WF_CONN_RETRY_CNT_ABN_DPM__
    else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

    if (reboot != -1) {
        PRINTF_ATCMD("\r\nOK\r\n");
#if defined (__SUPPORT_ATCMD_TLS__)
        atcmd_transport_ssl_reboot();
#endif // (__SUPPORT_ATCMD_TLS__)

        if (ble_combo_ref_flag == pdTRUE)
            combo_ble_sw_reset();

        vTaskDelay(100);
        reboot_func(reboot);
    }

atcmd_dpm_end:
    if (   (strlen(result_str) > 0 && strncmp(result_str, "OK", 2) != 0)
        && err_code == AT_CMD_ERR_CMD_OK) {

        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return err_code;
}

#define ATCMD_WIFI_STR_LEN    (32 + (MAX_SSID_LEN + MAX_PASSKEY_LEN))
int atcmd_wifi(int argc, char *argv[])
{
    int  err_code = AT_CMD_ERR_CMD_OK;
    char *value_str = NULL;
    int  value_int = 0;
    int  tmp_int1 = 0;
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
    int  status;
#endif // __SUPPORT_WIFI_CONCURRENT__
    int  ret = 0;

    const int run_mode = get_run_mode();
    const int sys_mode = da16x_network_main_get_sysmode();

    value_str = (char *)pvPortMalloc(ATCMD_WIFI_STR_LEN);
    if (value_str == NULL) {
        err_code = AT_CMD_ERR_MEM_ALLOC;
        goto atcmd_wifi_end;
    }

    memset(value_str, 0, ATCMD_WIFI_STR_LEN);

    // AT+WFMODE
    if (strcasecmp(argv[0] + 5, "MODE") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFMODE=? */
            da16x_get_config_int(DA16X_CONF_INT_MODE, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFMODE=<mode> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_RUN_MODE_TYPE;
                goto atcmd_wifi_end;
            }
#if defined ( __SUPPORT_P2P__ )
            if (tmp_int1 > SYSMODE_P2PGO) {
                err_code = AT_CMD_ERR_WIFI_RUN_MODE_RANGE;
                goto atcmd_wifi_end;
            }
#endif // __SUPPORT_P2P__

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
  #if defined ( __SUPPORT_P2P__ )
            if (tmp_int1 > SYSMODE_STA_N_P2P) {
                err_code = AT_CMD_ERR_WIFI_RUN_MODE_RANGE;
                goto atcmd_wifi_end;
            }
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__

            if (da16x_set_config_int(DA16X_CONF_INT_MODE, tmp_int1)) {
                err_code = AT_CMD_ERR_WIFI_RUN_MODE_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+WFMAC
    else if (strcasecmp(argv[0] + 5, "MAC") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFMAC=? */
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            char tmp1[18], tmp2[18];
#endif // __SUPPORT_WIFI_CONCURRENT__

            switch (get_run_mode()) {
            case SYSMODE_STA_ONLY:
                da16x_get_config_str(DA16X_CONF_STR_MAC_0, value_str);
                break;

            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // __SUPPORT_P2P__
                da16x_get_config_str(DA16X_CONF_STR_MAC_1, value_str);
                break;

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
                da16x_get_config_str(DA16X_CONF_STR_MAC_0, tmp1);
                da16x_get_config_str(DA16X_CONF_STR_MAC_1, tmp2);
                sprintf(value_str, "%s,%s", tmp1, tmp2);
                break;
#endif // __SUPPORT_WIFI_CONCURRENT__
            }
        } else if (argc == 2) {
            /* AT+WFMAC=<mac> */
            if (da16x_set_config_str(DA16X_CONF_STR_MAC_NVRAM, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_MAC_ADDR;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+WFSPF
    else if (strcasecmp(argv[0] + 5, "SPF") == 0) {
        if (argc == 2) {
            /* AT+WFSPF=<mac> (set only) */
            if (da16x_set_config_str(DA16X_CONF_STR_MAC_SPOOFING, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_MAC_ADDR;
            }
        } else if (argc < 2) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFOTP
    else if (strcasecmp(argv[0] + 5, "OTP") == 0) {
        if (argc == 2) {
            /* AT+WFOTP=<mac> (set only) */
            if (da16x_set_config_str(DA16X_CONF_STR_MAC_OTP, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_MAC_ADDR;
            }
        } else if (argc < 2) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFSTAT
    else if (strcasecmp(argv[0] + 5, "STAT") == 0) {
        char *reply;

        reply = (char *)pvPortMalloc(512);
        memset(reply, 0, 512);

        ret = da16x_cli_reply("status", NULL, reply);
        if (ret < 0 || strcmp(reply, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_STATUS;
            vPortFree(reply);
            goto atcmd_wifi_end;
        }

        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, reply);

        vPortFree(reply);
    }
    // AT+WFSTA
    else if (strcasecmp(argv[0] + 5, "STA") == 0) {
        if (   get_run_mode() == SYSMODE_AP_ONLY
#if defined ( __SUPPORT_P2P__ )
            || get_run_mode() == SYSMODE_P2P_ONLY
            || get_run_mode() == SYSMODE_P2PGO
#endif // __SUPPORT_P2P__
            ) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
        } else {
            if (chk_network_ready(WLAN0_IFACE) == 1) {
                strcpy(value_str, "1");
            } else {
                strcpy(value_str, "0");
            }
        }
    }
    // AT+WFPBC
    else if (strcasecmp(argv[0] + 5, "PBC") == 0) {
        ret = da16x_cli_reply("wps_pbc any", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_WPS_PBC_ANY;
        }
    }
    // AT+WFPIN
    else if (strcasecmp(argv[0] + 5, "PIN") == 0) {
        if (is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPIN=? */
            ret = da16x_cli_reply("wps_pin get", NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_WPS_PIN_GET;
            }
        } else if (argc == 1) {
            /* AT+WFPIN : generation pin */
            ret = da16x_cli_reply("wps_pin any", NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_WPS_PIN_ANY;
            }
        } else if (argc == 2) {
            /* AT+WFPIN=<pin> : try connecting by pin */
            char cmd[32];

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_2) != 0) {
                err_code = AT_CMD_ERR_WIFI_WPS_PIN_NUM;
                goto atcmd_wifi_end;
            }

            if (strlen(argv[1]) != 8) {
                err_code = AT_CMD_ERR_WIFI_WPS_PIN_NUM;
                goto atcmd_wifi_end;
            }

            sprintf(cmd, "wps_pin any %08d", tmp_int1);
            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_WPS_PIN_NUM;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+WFCWPS
    else if (strcasecmp(argv[0] + 5, "CWPS") == 0) {
        ret = da16x_cli_reply("wps_cancel", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_WPS_CANCEL;
        }
    }
    // AT+WFCC
    else if (strcasecmp(argv[0] + 5, "CC") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFCC=? */
            da16x_get_config_str(DA16X_CONF_STR_COUNTRY, value_str);
        } else if (argc == 2) {
            /* AT+WFCC=<country> */
            if (da16x_set_config_str(DA16X_CONF_STR_COUNTRY, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_WRONG_CC;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFSCAN
    else if (strcasecmp(argv[0] + 5, "SCAN") == 0) {
        char *reply;

        // Allocate temporary buffer
        reply = pvPortMalloc(CLI_SCAN_RSP_BUF_SIZE);
        memset(reply, 0, CLI_SCAN_RSP_BUF_SIZE);

        da16x_get_config_str(DA16X_CONF_STR_SCAN, reply);

        if (strcmp(reply, "UNSUPPORTED\n") == 0) {
            err_code = AT_CMD_ERR_WIFI_SCAN_UNSUPPORTED;
        } else {
            PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, reply + 53);
        }

        vPortFree(reply);
    }
#if defined ( __SUPPORT_PASSIVE_SCAN__ )
    // AT+WFPSCAN
    else if (strcasecmp(argv[0] + 5, "PSCAN") == 0) {
    	extern int chk_channel_by_country(char *country, int set_channel, int mode, int *rec_channel);
    	extern int i3ed11_ch_to_freq(int chan, int band);
        char cmd[ATCMD_WIFI_STR_LEN + 1] = {0, };
        char isDuplicate[14] = {0, };
        char country[4] = {0, };

        da16x_get_config_str(DA16X_CONF_STR_COUNTRY, country);

        if (get_run_mode() != SYSMODE_STA_ONLY) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;

            goto atcmd_wifi_end;
        }

        if (argc < 3) {        // WFPSCAN, interval, freq_1, freq_2,,,freq_14
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            goto atcmd_wifi_end;
        } else if (argc > 16) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            goto atcmd_wifi_end;
        } else {            // 3 <= argc <= 16
            strcat(cmd, "passive_scan ");

            tmp_int1 = 0;
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) == 0 && 
                tmp_int1 >= 30000) {
                // duration value validation passed
            } else {
                err_code = AT_CMD_ERR_WIFI_PSCAN_DURATION;
                goto atcmd_wifi_end;
            }

            tmp_int1 = 0;
            strcat(cmd, "channel_time_limit=");
            strcat(cmd, argv[1]);
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) == 0) {
                
                if (tmp_int1 == 0 && argc == 3) {
                    goto AT_WFPSCAN_RUN;
                } else if (tmp_int1 == 0 && argc > 3) {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_wifi_end;
                }

                strcat(cmd, " freq=");
                
                for (int i = 2; i < argc; i++ ) {
                    if (i > 2 && 
                        get_int_val_from_str(argv[i], &tmp_int1, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_wifi_end;
                    }

                    if (tmp_int1 < 1 || tmp_int1 > 14) {
                        err_code = AT_CMD_ERR_WIFI_PSCAN_FREQ_RANGE;
                        goto atcmd_wifi_end;
                    }

                    if(isDuplicate[tmp_int1] == true) // check duplicate channel
                    {
                        err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_wifi_end;
                    } else {
                        isDuplicate[tmp_int1] = true;
                    }
                    if(chk_channel_by_country(country, i3ed11_ch_to_freq(tmp_int1, 0), 1, NULL) < 0) // check country support channel
                    {
                        err_code = AT_CMD_ERR_WIFI_PSCAN_FREQ_RANGE;
                        goto atcmd_wifi_end;
                    }

                    if (i != (argc - 1)) { // check it is last argv or not
                        strcat(cmd, argv[i]);
                        strcat(cmd, "," );
                    } else {
                        strcat(cmd, argv[i]);
                    }
                }
            } else {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_wifi_end;
            }
        }

AT_WFPSCAN_RUN:
        PRINTF_ATCMD("\r\n");
        da16x_cli_reply(cmd, NULL, value_str);
        if (ret < 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_CH_TL;
            goto atcmd_wifi_end;
        }
    } else if (strcasecmp(argv[0] + 5, "PSTOP") == 0) {
        if (get_run_mode() != SYSMODE_STA_ONLY) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("passive_scan_stop", NULL, value_str);
        if (ret < 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_STOP;
            goto atcmd_wifi_end;
        }
    } else if (strcasecmp(argv[0] + 5, "PCDTMAX") == 0) {
        char cmd[ATCMD_WIFI_STR_LEN + 1] = {0, };
        int max_val = 0;

        if (get_run_mode() != SYSMODE_STA_ONLY) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if ((argc == 2 && argv[1][0] == '?') || argc == 1) {
            /* AT+WFPCDTMAX=? */
            ret = da16x_cli_reply("passive_scan_condition_max", NULL, value_str);
            if (ret < 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_CMAX_GET;
                goto atcmd_wifi_end;
            }
        } else if (argc < 3) {            // WFPCDT, bssid, max_threshold
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            goto atcmd_wifi_end;
        } else if (argc > 3) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            goto atcmd_wifi_end;
        } else if (argc == 3 ) {
            if (get_int_val_from_str(argv[2], &max_val, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_PSCAN_CMAX_RANGE;
                goto atcmd_wifi_end;
            }
            if ((max_val < -100) || (max_val > -10)) {
                err_code = AT_CMD_ERR_WIFI_PSCAN_CMAX_RANGE;
                goto atcmd_wifi_end;
            }

            sprintf(cmd, "passive_scan_condition_max bssid=%s max_threshold=%s", argv[1], argv[2]);
            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_CMAX_SET;
                goto atcmd_wifi_end;
            }
        }
    } else if (strcasecmp(argv[0] + 5, "PCDTMIN") == 0) {
        char cmd[ATCMD_WIFI_STR_LEN + 1] = {0, };
        int min_val = 0;

        if (get_run_mode() != SYSMODE_STA_ONLY) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }
        if ((argc == 2 && argv[1][0] == '?') || argc == 1) {
            /* AT+WFPCDTMIN=? */
            ret = da16x_cli_reply("passive_scan_condition_min", NULL, value_str);
            if (ret < 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_CMIN_GET;
                goto atcmd_wifi_end;
            }
        } else if (argc < 3) { // WFPCDT, bssid, min_threshold
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            goto atcmd_wifi_end;
        } else if (argc > 3) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            goto atcmd_wifi_end;
        } else if (argc == 3) {
            if (get_int_val_from_str(argv[2], &min_val, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_PSCAN_CMIN_RANGE;
                goto atcmd_wifi_end;
            }
            if ((min_val < -100) || (min_val > -10)) {
                err_code = AT_CMD_ERR_WIFI_PSCAN_CMIN_RANGE;
                goto atcmd_wifi_end;
            }

            sprintf(cmd, "passive_scan_condition_min bssid=%s min_threshold=%s", argv[1], argv[2]);
            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_PSCAN_CMIN_SET;
                goto atcmd_wifi_end;
            }
        }
    }
#endif // __SUPPORT_PASSIVE_SCAN__

#if defined (__SUPPORT_RSSI_CMD__)
    // AT+WFRSSI
    else if (strcasecmp(argv[0] + 5, "RSSI") == 0) {
        if (!da16x_network_main_check_network_ready(WLAN0_IFACE)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            int rssi  = -1;

            rssi = get_current_rssi(WLAN0_IFACE);
            if (rssi == -1) {
                PRINTF_ATCMD("\r\n+RSSI:NOT_CONN\r\n");
                err_code = AT_CMD_ERR_WIFI_NOT_CONNECTED;
                goto atcmd_wifi_end;
            } else {
                PRINTF_ATCMD("\r\n+RSSI:%d\r\n", rssi);
                PRINTF_ATCMD("\r\nOK\r\n");
                err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            goto atcmd_wifi_end;
        }
    }
#endif // __SUPPORT_RSSI_CMD__
    // AT+WFJAP
    else if (strcasecmp(argv[0] + 5, "JAP") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFJAP=? */
            char val_0[33] = {0, }, val_3[66] = {0, };
            int val_1, val_2;

            if (da16x_get_config_str(DA16X_CONF_STR_SSID_0, val_0) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_JAP_SSID_NO_VALUE;
                goto atcmd_wifi_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_AUTH_MODE_0, &val_1);

            if (val_1 == CC_VAL_AUTH_OPEN) {
                sprintf(value_str, "'%s',%d", val_0, val_1);
            } else if (val_1 == CC_VAL_AUTH_WEP) {
                da16x_get_config_int(DA16X_CONF_INT_WEP_KEY_INDEX, &val_2);
                da16x_get_config_str((DA16X_CONF_STR)(DA16X_CONF_STR_WEP_KEY0 + val_2), val_3);
                sprintf(value_str, "'%s',%d,%d,'%s'", val_0, val_1, val_2, val_3);
            } else if (val_1 > CC_VAL_AUTH_WEP) {
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                if (val_1 == CC_VAL_AUTH_OWE) {
                    sprintf(value_str, "'%s',%d", val_0, val_1);
                } else {
                    // WPA / WPA2 / WPA_AUTO / SAE / RSN_SAE ...

                    da16x_get_config_int(DA16X_CONF_INT_ENCRYPTION_0, &val_2);

                    if (val_1 == CC_VAL_AUTH_SAE || val_1 == CC_VAL_AUTH_RSN_SAE) {
                        da16x_get_config_str(DA16X_CONF_STR_SAE_PASS_0, val_3);
                    } else {
                        da16x_get_config_str(DA16X_CONF_STR_PSK_0, val_3);
                    }
                    sprintf(value_str, "'%s',%d,%d,'%s'", val_0, val_1, val_2, val_3);
                }
#else
                da16x_get_config_int(DA16X_CONF_INT_ENCRYPTION_0, &val_2);
                da16x_get_config_str(DA16X_CONF_STR_PSK_0, val_3);
                sprintf(value_str, "'%s',%d,%d,'%s'", val_0, val_1, val_2, val_3);
#endif //  __SUPPORT_WPA3_PERSONAL__
            }
        } else {
            int auth_mode;
            int is_ssid_hidden = pdFALSE;
            char *psk = NULL;
            char *psk_str = NULL;
            int  psk_len;

            /* AT+WFJAP=<ssid>,<sec>[<,wep_idx|enc><,key>][,<hidden>] */

            /*
                // OPEN
                argv[0]  argv[1] argv[2]
                AT+WFJAP=<ssid>,<sec>
                ---> argc==3 && sec==0

                // OPEN_hidden
                argv[0]  argv[1] argv[2] argv[3]
                AT+WFJAP=<ssid>,<sec>,<1=hidden>
                ---> argc==4 && sec==0

                // WEP
                argv[0]  argv[1] argv[2] argv[3]  argv[4]
                AT+WFJAP=<ssid>,<sec>,<wep_idx>,<key>
                ---> argc==5 && sec==1

                // WPA/WPA2/....
                argv[0]  argv[1] argv[2] argv[3]  argv[4]
                AT+WFJAP=<ssid>,<sec>,<enc>,<key>
                ---> argc==5 && sec==2/3/4

                // WEP_hidden
                argv[0]  argv[1] argv[2] argv[3]  argv[4] argv[5]
                AT+WFJAP=<ssid>,<sec>,<wep_idx>,<key>,<hidden>
                ---> argc==6 && sec==1

                // WPA/WPA2/.... hidden
                argv[0]  argv[1] argv[2] argv[3]  argv[4] argv[5]
                AT+WFJAP=<ssid>,<sec>,<enc>,<key>,<1=hidden>
                ---> argc==6 && sec==2/3/4
            */

            // argc range check
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_wifi_end;
            } else if (argc > 6) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto atcmd_wifi_end;
            }

            // get auth_mode and verify
            if (get_int_val_from_str(argv[2], &auth_mode, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_JAP_SECU_ARG_TYPE;
                goto atcmd_wifi_end;
            } else if (
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                (is_in_valid_range(auth_mode, CC_VAL_AUTH_OPEN, CC_VAL_AUTH_RSN_SAE) == pdFALSE)
#else
                (is_in_valid_range(auth_mode, CC_VAL_AUTH_OPEN, CC_VAL_AUTH_WPA_AUTO) == pdFALSE)
#endif // __SUPPORT_WPA3_PERSONAL__
            ) {
                err_code = AT_CMD_ERR_WIFI_JAP_SECU_ARG_RANGE;
                goto atcmd_wifi_end;
            }

            // get <hidden>
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
            if (auth_mode == CC_VAL_AUTH_OPEN || auth_mode == CC_VAL_AUTH_OWE)
#else
            if (auth_mode == CC_VAL_AUTH_OPEN)
#endif // __SUPPORT_WPA3_PERSONAL__
            {
                if (argc > 4) {
                    err_code = AT_CMD_ERR_WIFI_JAP_OPEN_TOO_MANY_ARG;
                    goto atcmd_wifi_end;
                }

                if (argc == 4) {
                    // OPEN / ... HIDDEN
                    if (get_int_val_from_str(argv[3], &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_JAP_OPEN_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                        err_code = AT_CMD_ERR_WIFI_JAP_OPEN_HIDDEN_RANGE;
                        goto atcmd_wifi_end;
                    }
                }
            } else {
                // WEP / WPA / WPA2 /.... HIDDEN

                if (argc == 6) {
                    if (get_int_val_from_str(argv[5], &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_JAP_SECU_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                        err_code = AT_CMD_ERR_WIFI_JAP_SECU_HIDDEN_RANGE;
                        goto atcmd_wifi_end;
                    }
                }
            }

            if (auth_mode == CC_VAL_AUTH_WEP) {
                // validate wep_idx, wep_key
                if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WEP_IDX_TYPE;
                    goto atcmd_wifi_end;
                } else if (is_in_valid_range(tmp_int1, 0, 3) == pdFALSE) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WEP_IDX_RANGE;
                    goto atcmd_wifi_end;
                }

                tmp_int1 = strlen(argv[4]);
                if (tmp_int1 != 5 && tmp_int1 != 13 && tmp_int1 != 10 && tmp_int1 != 26) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WEP_KEY_LEN;
                    goto atcmd_wifi_end;
                }
            }
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
            else if (auth_mode == CC_VAL_AUTH_OWE) {
                // skip ...
            }
#endif // __SUPPORT_WPA3_PERSONAL__
            else if (auth_mode > CC_VAL_AUTH_WEP) {
                // WPA / WPA2 / WPA1+WPA2 / ... (except OWE) ....

                // validate <enc>, <key>
                if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WPA_MODE_TYPE;
                    goto atcmd_wifi_end;
                } else if (is_in_valid_range(tmp_int1, CC_VAL_ENC_TKIP, CC_VAL_ENC_AUTO) == pdFALSE) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WPA_MODE_RANGE;
                    goto atcmd_wifi_end;
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                } else if ((auth_mode == CC_VAL_AUTH_SAE || 
                            auth_mode == CC_VAL_AUTH_RSN_SAE) && 
                            tmp_int1 != CC_VAL_ENC_CCMP) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WPA_MODE_RANGE;
                    goto atcmd_wifi_end;                    
#endif // __SUPPORT_WPA3_PERSONAL__
                }               
            }

            /* Support Extended ASCII or UTF-8 for SSID
             *    Do not use the argv[1] string directly. */
            memset(value_str, 0, ATCMD_WIFI_STR_LEN);
            value_int = str_decode((u8 *) value_str, MAX_SSID_LEN, argv[1]);
            if (value_int > MAX_SSID_LEN) {
                err_code = AT_CMD_ERR_WIFI_JAP_SSID_LEN;
                goto atcmd_wifi_end;
            } else {
                da16x_cli_reply("remove_network 0", NULL, NULL);
            }

            /* Support Extended ASCII or UTF-8 for SSID */
            da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, value_str);

            da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, auth_mode);

            if (auth_mode == CC_VAL_AUTH_WEP) {
                da16x_set_nvcache_int(DA16X_CONF_INT_WEP_KEY_INDEX, atoi(argv[3]));
                da16x_set_nvcache_str((DA16X_CONF_STR)(DA16X_CONF_STR_WEP_KEY0 + atoi(argv[3])), argv[4]);
            }
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
            else if (auth_mode == CC_VAL_AUTH_OWE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_CCMP); // ccmp
                da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_0, CC_VAL_MAND); // pmf
            }
#endif // __SUPPORT_WPA3_PERSONAL__
            else if (auth_mode > CC_VAL_AUTH_WEP) {
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                // WPA/WPA2/WPA3 ...

                if (auth_mode == CC_VAL_AUTH_SAE || auth_mode == CC_VAL_AUTH_RSN_SAE) {
                    // cli set_network 0 pairwise    ccmp
                    da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_CCMP);

                    // default sae_groups is used, no need to save

                    // cli set_network 0 ieee80211w x
                    if (auth_mode == CC_VAL_AUTH_SAE) {
                        da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_0, CC_VAL_MAND);
                    } else {
                        da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_0, CC_VAL_OPT);
                    }

                    // cli set_network 0 sae_password *********
                    psk = argv[4];
                    psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                    if (psk_str == NULL) {
                        err_code = AT_CMD_ERR_MEM_ALLOC;
                        goto atcmd_wifi_end;
                    }

                    /* Support Extended ASCII or UTF-8 for PSK
                     *    Do not use the argv[2+idx_adj] string directly. */
                    memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                    psk_len = str_decode((u8 *)psk_str, MAX_PASSKEY_LEN, psk);
                    if (psk_len < 8 || psk_len > 63) {
                        err_code = AT_CMD_ERR_WIFI_JAP_WPA_KEY_LEN;

                        if (psk_str != NULL) {
                            vPortFree(psk_str);
                        }
                        goto atcmd_wifi_end;
                    }

                    da16x_set_nvcache_str(DA16X_CONF_STR_SAE_PASS_0, psk_str); // <key>
                } else {
                    da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, atoi(argv[3])); // <enc>

                    psk = argv[4];
                    psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                    if (psk_str == NULL) {
                        err_code = AT_CMD_ERR_MEM_ALLOC;
                        goto atcmd_wifi_end;
                    }

                    /* Support Extended ASCII or UTF-8 for PSK
                     *    Do not use the argv[2+idx_adj] string directly. */
                    memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                    psk_len = str_decode((u8 *)psk_str, MAX_PASSKEY_LEN, psk);
                    if (psk_len < 8 || psk_len > 63) {
                        err_code = AT_CMD_ERR_WIFI_JAP_WPA_KEY_LEN;

                        if (psk_str != NULL) {
                            vPortFree(psk_str);
                        }
                        goto atcmd_wifi_end;
                    }

                    da16x_set_nvcache_str(DA16X_CONF_STR_PSK_0, psk_str); // <key>
                }
#else
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, atoi(argv[3])); // <enc>

                psk = argv[4];
                psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                if (psk_str == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_wifi_end;
                }

                /* Support Extended ASCII or UTF-8 for PSK
                 *    Do not use the argv[2+idx_adj] string directly. */
                memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                psk_len = str_decode((u8 *)psk_str, MAX_PASSKEY_LEN, psk);
                if (psk_len < 8 || psk_len > 63) {
                    err_code = AT_CMD_ERR_WIFI_JAP_WPA_KEY_LEN;

                    if (psk_str != NULL) {
                        vPortFree(psk_str);
                    }
                    goto atcmd_wifi_end;
                }

                da16x_set_nvcache_str(DA16X_CONF_STR_PSK_0, psk_str); // <key>
#endif // __SUPPORT_WPA3_PERSONAL__
            }

            if (psk_str != NULL) {
                vPortFree(psk_str);
            }
			
            if (is_ssid_hidden == pdTRUE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_HIDDEN_0, 1);
            }

            da16x_nvcache2flash();


            ret = da16x_cli_reply("select_network 0", NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_SELECT_NETWORK;
                goto atcmd_wifi_end;
            }

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            atcmd_rsp_wifi_conn();
        }
    }
    // AT+WFJAPA
    else if (strcasecmp(argv[0] + 5, "JAPA") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFJAPA=? */
            char val_0[33] = {0, }, val_1[66] = {0, };
            int temp;

            if (da16x_get_config_str(DA16X_CONF_STR_SSID_0, val_0) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_JAPA_SSID_NO_VALUE;
                goto atcmd_wifi_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_AUTH_MODE_0, &temp);

            if (temp == CC_VAL_AUTH_OPEN) {
                sprintf(value_str, "'%s'", val_0);
            } else if (temp == CC_VAL_AUTH_WEP) {
                err_code = AT_CMD_ERR_WIFI_JAPA_WEP_NOT_SUPPORT;
                goto atcmd_wifi_end;
            } else if (temp > CC_VAL_AUTH_WEP) {
                da16x_get_config_str(DA16X_CONF_STR_PSK_0, val_1);

                sprintf(value_str, "'%s','%s'", val_0, val_1);
            }
        } else {
            char *ssid = NULL;
            char *psk = NULL;
            char *psk_str = NULL;
            unsigned short psk_len = 0;
            int is_ssid_hidden = 0;

            enum _ap_sec_type {
                OPEN,
                OPEN_HIDDEN,
                WPA,
                WPA_HIDDEN
            } ap_sec_type;
            unsigned short idx_adj = 0;

            /* AT+WFJAP=<ssid>[,<psk>][,<hidden>] */

            /*
                // OPEN
                argv[0]   argv[1]
                AT+WFJAPA=<ssid>
                ---> argc=2

                // OPEN + hidden
                argv[0]   argv[1] argv[2]
                AT+WFJAPA=<ssid>,<hidden>
                ---> argc=3, strlen(argv[2])==1

                // WPA ...
                argv[0]  argv[1] argv[2]
                AT+WFJAPA=<ssid>,<psk>
                ---> argc=3

                // WPA + hidden
                argv[0]   argv[1] argv[2] argv[3]
                AT+WFJAPA=<ssid>,<psk>,<hidden>
                ---> argc=4
            */


            // argc range check
            if (argc < (2 + idx_adj)) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_wifi_end;
            } else if (argc > (4 + idx_adj)) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto atcmd_wifi_end;
            }

            ssid = argv[1 + idx_adj];

            /* Support Extended ASCII or UTF-8 for SSID
             *    Do not use the argv[1] string directly. */
            memset(value_str, 0, ATCMD_WIFI_STR_LEN);
            value_int = str_decode((u8 *) value_str, MAX_SSID_LEN, ssid);
            if (value_int > MAX_SSID_LEN) {
                err_code = AT_CMD_ERR_WIFI_JAPA_SSID_LEN;
                goto atcmd_wifi_end;
            }

            // Decide ap_sec_type
            if (argc == 2 + idx_adj) {
                /* AT+WFJAPA=<ssid> */
                ap_sec_type = OPEN;
            } else if (argc == (3 + idx_adj)) {
                char *temp_str = argv[2 + idx_adj];

                if (strlen(temp_str) == 1) {
                    // psk len should be over 8 characters, hence, this is hidden field

                    /* AT+WFJAPA=<ssid>,<hidden> */

                    if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    }

                    if (is_ssid_hidden == 1) {
                        ap_sec_type = OPEN_HIDDEN;
                    } else {
                        ap_sec_type = OPEN;
                    }
                } else {
                    /* AT+WFJAPA=<ssid>,<psk> */
                    ap_sec_type = WPA;
                }
            } else if (argc == 4+idx_adj) {
                char *temp_str = argv[3+idx_adj];

                /* AT+WFJAPA=<ssid>,<psk>,<hidden> */
                if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE;
                    goto atcmd_wifi_end;
                } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                    err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_RANGE;
                    goto atcmd_wifi_end;
                }

                if (is_ssid_hidden == 1) {
                    ap_sec_type = WPA_HIDDEN;
                } else {
                    ap_sec_type = WPA;
                }
            }

            // verify psk
            switch (ap_sec_type) {
            case WPA:
            case WPA_HIDDEN:
                psk = argv[2+idx_adj];

                psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                if (psk_str == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_wifi_end;
                }

                /* Support Extended ASCII or UTF-8 for PSK
                 *    Do not use the argv[2+idx_adj] string directly. */
                memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                psk_len = str_decode((u8 *) psk_str, MAX_PASSKEY_LEN, psk);
                if (psk_len < 8 || psk_len > 63) {
                    err_code = AT_CMD_ERR_WIFI_JAPA_PSK_LEN;

                    if (psk_str != NULL) {
                        vPortFree(psk_str);
                    }
                    goto atcmd_wifi_end;
                }
                break;

            default:
                break;
            }

            da16x_cli_reply("remove_network 0", NULL, NULL);

            /* Support Extended ASCII or UTF-8 for SSID */
            da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, value_str);

            // set config paramters per ap_sec_type
            switch (ap_sec_type) {
            case OPEN:
            case OPEN_HIDDEN:
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_OPEN);
                break;

            case WPA:
            case WPA_HIDDEN:
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_WPA_AUTO);
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_AUTO);
                da16x_set_nvcache_str(DA16X_CONF_STR_PSK_0, psk_str);
                break;

            default:
                break;
            }

            if (psk_str != NULL) {
                vPortFree(psk_str);
            }
            da16x_nvcache2flash();

            // process hidden
            switch (ap_sec_type) {
            case OPEN_HIDDEN:
            case WPA_HIDDEN:
                da16x_set_nvcache_int(DA16X_CONF_INT_HIDDEN_0, 1);
                break;

            default:
                break;
            }
            da16x_nvcache2flash();	

            ret = da16x_cli_reply("select_network 0", NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_SELECT_NETWORK;
                goto atcmd_wifi_end;
            }

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            atcmd_rsp_wifi_conn();
        }
    }
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
    // AT+WFJAPA3
    else if (strcasecmp(argv[0] + 5, "JAPA3") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFJAPA3=? */
            char val_0[33] = {0, }, val_1[66] = {0, };
            int temp;

            if (da16x_get_config_str(DA16X_CONF_STR_SSID_0, val_0) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_JAPA_SSID_NO_VALUE;
                goto atcmd_wifi_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_AUTH_MODE_0, &temp);

            if (temp == CC_VAL_AUTH_OPEN) {
                sprintf(value_str, "'%s'", val_0);
            } else if (temp == CC_VAL_AUTH_WEP) {
                err_code = AT_CMD_ERR_WIFI_JAPA_WEP_NOT_SUPPORT;
                goto atcmd_wifi_end;
            } else if (temp > CC_VAL_AUTH_WEP) {
                if (temp == CC_VAL_AUTH_SAE || temp == CC_VAL_AUTH_RSN_SAE) {
                    da16x_get_config_str(DA16X_CONF_STR_SAE_PASS_0, val_1);
                } else {
                    da16x_get_config_str(DA16X_CONF_STR_PSK_0, val_1);
                }

                sprintf(value_str, "'%s','%s'", val_0, val_1);
            }
        } else {
            char *ssid = NULL;
            char *psk = NULL;
            char *psk_str = NULL;
            unsigned short psk_len = 0;
            int is_ssid_hidden = 0;
            int is_wpa3 = 0;
            unsigned short idx_adj = 1;
            enum _ap_sec_type {
                OPEN,
                OPEN_HIDDEN,
                WPA,
                WPA_HIDDEN,
                OWE,
                OWE_HIDDEN,
                WPA3,
                WPA3_HIDDEN
            } ap_sec_type;

            /*
                AT+WFJAPA3=<is_wpa3>,<ssid>[,<key>][,<hidden>]

                // OPEN
                argv[0]   argv[1]   argv[2]
                AT+WFJAPA3=<is_wpa3>,<ssid>
                ---> argc=3, is_wpa3=0

                // OPEN+hidden
                argv[0]   argv[1]   argv[2] argv[3]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<hidden>
                ---> argc=4, is_wpa3=0 strlen(argv[3])==1

                // WPA
                argv[0]  argv[1]  argv[2]  argv[3]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<psk>
                ---> argc=4, is_wpa3=0, strlen(argv[3])>1

                // WPA+hidden
                argv[0]  argv[1]  argv[2]  argv[3] argv[4]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<psk>,<hidden>
                ---> argc=5, is_wpa3=0

                // OWE
                argv[0]   argv[1]   argv[2]
                AT+WFJAPA3=<is_wpa3>,<ssid>
                ---> argc=3, is_wpa3=1

                // OWE+hidden
                argv[0]   argv[1]   argv[2] argv[3]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<hidden>
                ---> argc=4, is_wpa3=1 strlen(argv[3])==1

                // WPA3 (SAE)
                argv[0]  argv[1]  argv[2]  argv[3]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<psk>
                ---> argc=4, is_wpa3=1, strlen(argv[3])>1

                // WPA3+hidden
                argv[0]  argv[1]  argv[2]  argv[3] argv[4]
                AT+WFJAPA3=<is_wpa3>,<ssid>,<psk>,<hidden>
                ---> argc=5, is_wpa3=1
            */

            // argc range check
            if (argc < (2 + idx_adj)) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_wifi_end;
            } else if (argc > (4 + idx_adj)) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto atcmd_wifi_end;
            }

            if (get_int_val_from_str(argv[idx_adj], &is_wpa3, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_MODE_TYPE;
                goto atcmd_wifi_end;
            } else if (is_in_valid_range(is_wpa3, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_MODE_RANGE;
                goto atcmd_wifi_end;
            }

            ssid = argv[1+idx_adj];

            /* Support Extended ASCII or UTF-8 for SSID
             *    Do not use the argv[1] string directly. */
            memset(value_str, 0, MAX_SSID_LEN + 1);
            value_int = str_decode((u8 *)value_str, MAX_SSID_LEN, ssid);
            if (value_int > MAX_SSID_LEN) {
                err_code = AT_CMD_ERR_WIFI_JAPA_SSID_LEN;
                goto atcmd_wifi_end;
            }

            // decide ap_sec_type
            if (argc == 2 + idx_adj) {
                if (is_wpa3 == pdTRUE) {
                    /* AT+WFJAPA3=<is_wpa>,<ssid> */
                    ap_sec_type = OWE;
                } else {
                    /* AT+WFJAPA3=<ssid> */
                    ap_sec_type = OPEN;
                }
            } else if (argc == (3 + idx_adj)) {
                char* temp_str = argv[2 + idx_adj];

                if (strlen(temp_str) == 1) {
                    // psk len should be over 8 characters, hence, this is hidden field

                    if (is_wpa3 == pdTRUE) {
                        /* AT+WFJAPA3=<is_wpa>,<ssid>,<hidden> */
                        if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                            err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_TYPE;
                            goto atcmd_wifi_end;
                        } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                            err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_RANGE;
                            goto atcmd_wifi_end;
                        }

                        if (is_ssid_hidden == 1) {
                            ap_sec_type = OWE_HIDDEN;
                        } else {
                            ap_sec_type = OWE;
                        }
                    } else {
                        /* AT+WFJAPA3=<ssid>,<hidden> */
                        if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                            err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE;
                            goto atcmd_wifi_end;
                        } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                            err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_RANGE;
                            goto atcmd_wifi_end;
                        }

                        if (is_ssid_hidden == 1) {
                            ap_sec_type = OPEN_HIDDEN;
                        } else {
                            ap_sec_type = OPEN;
                        }
                    }
                } else {
                    if (is_wpa3 == pdTRUE) {
                        /* AT+WFJAPA3=<is_wpa>,<ssid>,<psk> */
                        ap_sec_type = WPA3;
                    } else {
                        /* AT+WFJAPA3=<ssid>,<psk> */
                        ap_sec_type = WPA;
                    }
                }
            } else if (argc == 4+idx_adj) {
                char* temp_str = argv[3+idx_adj];

                if (is_wpa3 == pdTRUE) {
                    /* AT+WFJAPA3=<is_wpa3>,<ssid>,<psk>,<hidden> */
                    if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_HIDDEN_RANGE;
                        goto atcmd_wifi_end;
                    }

                    if (is_ssid_hidden == 1) {
                        ap_sec_type = WPA3_HIDDEN;
                    } else {
                        ap_sec_type = WPA3;
                    }
                } else {
                    /* AT+WFJAPA3=<ssid>,<psk>,<hidden> */
                    if (get_int_val_from_str(temp_str, &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } else if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) {
                        err_code = AT_CMD_ERR_WIFI_JAPA_HIDDEN_RANGE;
                        goto atcmd_wifi_end;
                    }

                    if (is_ssid_hidden == 1) {
                        ap_sec_type = WPA_HIDDEN;
                    } else {
                        ap_sec_type = WPA;
                    }
                }
            }

            // verify psk
            switch (ap_sec_type) {
            case WPA:
            case WPA_HIDDEN:
            case WPA3:
            case WPA3_HIDDEN:
                psk = argv[2+idx_adj];

                /* Support Extended ASCII or UTF-8 for PSK
                 *    Do not use the argv[2+idx_adj] string directly. */
                psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                if (psk_str == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_wifi_end;
                }

                memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                psk_len = str_decode((u8 *) psk_str, MAX_PASSKEY_LEN, psk);
                if (psk_len < 8 || psk_len > 63) {
                    err_code = AT_CMD_ERR_WIFI_JAPA_WPA3_PSK_LEN;

                    if (psk_str != NULL) {
                        vPortFree(psk_str);
                    }
                    goto atcmd_wifi_end;
                }
                break;

            default:
                break;
            }

            da16x_cli_reply("remove_network 0", NULL, NULL);

            /* Support Extended ASCII or UTF-8 for SSID */
            da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, value_str);

            // set config paramters per ap_sec_type
            switch (ap_sec_type) {
            case OPEN:
            case OPEN_HIDDEN:
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_OPEN);
                break;

            case WPA:
            case WPA_HIDDEN:
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_WPA_AUTO);
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_AUTO);
                da16x_set_nvcache_str(DA16X_CONF_STR_PSK_0, psk_str);
                break;

            case WPA3:
            case WPA3_HIDDEN:
                // connect as WPA2+WPA3(SAE) ...

                // cli set_network 0 key_mgmt WPA-PSK SAE
                // cli set_network 0 proto RSN
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_RSN_SAE);

                //cli set_network 0 pairwise ccmp
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_CCMP);

                // cli set_network 0 ieee80211w 1
                da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_0, CC_VAL_OPT);

                //cli set_network 0 sae_password *********
                da16x_set_nvcache_str(DA16X_CONF_STR_SAE_PASS_0, psk_str);
                break;

            case OWE:
            case OWE_HIDDEN:
                // connect as WPA3 OWE
                da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, CC_VAL_AUTH_OWE);
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, CC_VAL_ENC_CCMP); // ccmp
                da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_0, CC_VAL_MAND); // pmf
                break;

            default:
                break;
            }

            if (psk_str != NULL) {
                vPortFree(psk_str);
            }
            da16x_nvcache2flash();

            // process hidden
            switch (ap_sec_type) {
            case OPEN_HIDDEN:
            case WPA_HIDDEN:
            case OWE_HIDDEN:
            case WPA3_HIDDEN:
				da16x_set_nvcache_int(DA16X_CONF_INT_HIDDEN_0, 1);
                break;

            default:
                break;
            }
            da16x_nvcache2flash();
            ret = da16x_cli_reply("select_network 0", NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_SELECT_NETWORK;
                goto atcmd_wifi_end;
            }

            PRINTF_ATCMD("\r\nOK\r\n");
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            atcmd_rsp_wifi_conn();
        }
    }
#endif // __SUPPORT_WPA3_PERSONAL__
    // AT+WFCAP
    else if (strcasecmp(argv[0] + 5, "CAP") == 0) {
        // Check running mode
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (atcmd_chk_wifi_conn() == pdTRUE) {
            err_code = AT_CMD_ERR_WIFI_ALREADY_CONNECTED;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("select_network 0", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_SELECT_NETWORK;
            goto atcmd_wifi_end;
        }

        PRINTF_ATCMD("\r\nOK\r\n");
        err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

        atcmd_rsp_wifi_conn();
    }
    // AT+WFQAP
    else if (strcasecmp(argv[0] + 5, "QAP") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("disconnect", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_DISCONNECT;
        }
    }
    // AT+WFROAP
    else if (strcasecmp(argv[0] + 5, "ROAP") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFROAP=? */
            da16x_get_config_int(DA16X_CONF_INT_ROAM, &value_int);
            sprintf(value_str, "%d", value_int ? 1 : 0);
        } else if (argc == 2) {
            /* AT+WFROAP=<roam> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_ROAM, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFROTH
    else if (strcasecmp(argv[0] + 5, "ROTH") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFROTH=? */
            da16x_get_config_int(DA16X_CONF_INT_ROAM_THRESHOLD, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFROTH=<rssi> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0 ) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_ROAM_THRESHOLD, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFDIS
    else if (strcasecmp(argv[0] + 5, "DIS") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFDIS=? */
            da16x_get_config_int(DA16X_CONF_INT_STA_PROF_DISABLED, &value_int);
            sprintf(value_str, "%d", value_int ? 1 : 0);
        } else if (argc == 2) {
            /* AT+WFDIS=<disabled> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_STA_PROF_DISABLED, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#if defined (__SUPPORT_WPA_ENTERPRISE__)
    // AT+WFENTAP
    else if (strcasecmp(argv[0] + 5, "ENTAP") == 0) {
        int wpa, enc, p1, p2;
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFENTAP=? */
            if (da16x_get_config_str(DA16X_CONF_STR_SSID_0, value_str) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_SSID_NO_VALUE;
                goto atcmd_wifi_end;
            }

            if (da16x_get_config_int(DA16X_CONF_INT_AUTH_MODE_0, &wpa) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_AUTH0_UNSUPPORT;
                goto atcmd_wifi_end;
            } else {
                if (wpa < CC_VAL_AUTH_WPA_EAP) {
                    strcat(value_str, ",0");
                    goto atcmd_wifi_end;
                }
            }

            if (da16x_get_config_int(DA16X_CONF_INT_ENCRYPTION_0, &enc) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_ENC0_UNSUPPORT;
                goto atcmd_wifi_end;
            }

            if (da16x_get_config_int(DA16X_CONF_INT_EAP_PHASE1, &p1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE1;
                goto atcmd_wifi_end;
            }

            if (da16x_get_config_int(DA16X_CONF_INT_EAP_PHASE2, &p2) == CC_SUCCESS) {
                sprintf(value_str, "%s,%d,%d,%d,%d", value_str, wpa, enc, p1, p2);
                goto atcmd_wifi_end;
            } else {
                sprintf(value_str, "%s,%d,%d,%d", value_str, wpa, enc, p1);
                goto atcmd_wifi_end;
            }
        } else {
            int is_ssid_hidden = pdFALSE;
            /* AT+WFENTAP=<ssid>,<auth>,<enc>,<phase1>[,<phase2>][,<hidden>] */
            if (argc < 5) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_wifi_end;
            }

            if (get_int_val_from_str(argv[2], &wpa, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_SECU_MODE;
                goto atcmd_wifi_end;
            }

            if (get_int_val_from_str(argv[3], &enc, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_ENC_MODE;
                goto atcmd_wifi_end;
            }

            if (get_int_val_from_str(argv[4], &p1, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE1;
                goto atcmd_wifi_end;
            }

            if (argc == 6) {
                /* AT+WFENTAP=<ssid>,<auth>,<enc>,<phase1>,<hidden> */
                if (p1 == CC_VAL_EAP_FAST || p1 == CC_VAL_EAP_TLS) {
                    if (get_int_val_from_str(argv[5], &is_ssid_hidden, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_TYPE;
                        goto atcmd_wifi_end;
                    } 
                /* AT+WFENTAP=<ssid>,<auth>,<enc>,<phase1>,<phase2>*/
                } else if (p1 == CC_VAL_EAP_DEFAULT || p1 == CC_VAL_EAP_PEAP0 || CC_VAL_EAP_PEAP1 || CC_VAL_EAP_TTLS) {
                    if (get_int_val_from_str(argv[5], &p2, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE2;
                        goto atcmd_wifi_end;
                    }
                }
            }

            if (argc == 7) {
                /* AT+WFENTAP=<ssid>,<auth>,<enc>,<phase1>,<phase2>,<hidden> */
                if (p1 == CC_VAL_EAP_FAST || p1 == CC_VAL_EAP_TLS) { // when argc == 7, p1 cannot be EAP FAST or EAP TLS
                    err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_MODE;       // because it doesn't need a p2 process.
                    goto atcmd_wifi_end;
                }

                if (get_int_val_from_str(argv[5], &p2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE2;
                    goto atcmd_wifi_end;
                }
				
                if (get_int_val_from_str(argv[6], &is_ssid_hidden, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_TYPE;
                    goto atcmd_wifi_end;
                } 
            }

            if ((wpa < CC_VAL_AUTH_WPA_EAP || wpa > CC_VAL_AUTH_WPA_AUTO_EAP)) { // Wi-Fi Auth mode
                err_code = AT_CMD_ERR_WIFI_ENTAP_SECU_MODE;
                goto atcmd_wifi_end;
            } else if (enc < CC_VAL_ENC_TKIP || enc > CC_VAL_ENC_AUTO) { // Wi-Fi Encryption type
                err_code = AT_CMD_ERR_WIFI_ENTAP_ENC_MODE;
                goto atcmd_wifi_end;
            } else if (p1 < CC_VAL_EAP_DEFAULT || p1 > CC_VAL_EAP_TLS) { // EAP Phase #1
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_MODE;
                goto atcmd_wifi_end;
            }

            if (argc == 6) {              
                if (p1 == CC_VAL_EAP_FAST || p1 == CC_VAL_EAP_TLS) {
                    if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) { // hidden
                        err_code = AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_RANGE;
                        goto atcmd_wifi_end;
                    }
                } else if (p1 == CC_VAL_EAP_DEFAULT || p1 == CC_VAL_EAP_PEAP0 || CC_VAL_EAP_PEAP1 || CC_VAL_EAP_TTLS) {
                    if (p2 < CC_VAL_EAP_PHASE2_MIX || p2 > CC_VAL_EAP_GTC) {   // EAP Phase #2
                        err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE2;
                        goto atcmd_wifi_end;
                    }
                }   
            }

            if (argc == 7) {											
                if (p2 < CC_VAL_EAP_PHASE2_MIX || p2 > CC_VAL_EAP_GTC) {  // EAP Phase #2
                    err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PHASE2;
                    goto atcmd_wifi_end;
                }

                if (is_in_valid_range(is_ssid_hidden, 0, 1) == pdFALSE) { // hidden
                    err_code = AT_CMD_ERR_WIFI_ENTAP_WPA_HIDDEN_RANGE;
                    goto atcmd_wifi_end;
                }
            }

            /* Support Extended ASCII or UTF-8 for SSID
             *    Do not use the argv[1] string directly. */
            memset(value_str, 0, ATCMD_WIFI_STR_LEN);
            value_int = str_decode((u8 *)value_str, MAX_SSID_LEN, argv[1]);
            if (value_int > MAX_SSID_LEN) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_SSID_LEN;
                goto atcmd_wifi_end;
            } else {
                da16x_cli_reply("flush", NULL, NULL);
            }

            /* Save profile */
            da16x_set_nvcache_str(DA16X_CONF_STR_SSID_0, value_str);
            da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_0, wpa);
            da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_0, enc);
            da16x_set_nvcache_int(DA16X_CONF_INT_EAP_PHASE1, p1);

            if (argc == 6) {
                if (p1 == CC_VAL_EAP_DEFAULT || p1 == CC_VAL_EAP_PEAP0 || CC_VAL_EAP_PEAP1 || CC_VAL_EAP_TTLS) {
                    da16x_set_nvcache_int(DA16X_CONF_INT_EAP_PHASE2, p2);
                }
            } else if (argc == 7) {
                da16x_set_nvcache_int(DA16X_CONF_INT_EAP_PHASE2, p2);
            }

			if (is_ssid_hidden == pdTRUE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_HIDDEN_0, 1);
			} else if (is_ssid_hidden == pdFALSE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_HIDDEN_0, 0);
			}
            da16x_nvcache2flash();

        }
    }
    // AT+WFENTLI
    else if (strcasecmp(argv[0] + 5, "ENTLI") == 0) {
        if ((run_mode != SYSMODE_STA_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFENTLI=? */
            if (da16x_get_config_str(DA16X_CONF_STR_EAP_IDENTITY, value_str) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_ID_NO_VALUE;
                goto atcmd_wifi_end;
            } else {
                char tmp_val[65] = {0, };

                if (da16x_get_config_str(DA16X_CONF_STR_EAP_PASSWORD, tmp_val) == CC_SUCCESS) {
                    sprintf(value_str, "%s,%s", value_str, tmp_val);
                }
            }
        } else {
            /* AT+WFENTLI=<id>,<pw> */
            if (strlen(argv[1]) > 64) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_ID_LEN;
                goto atcmd_wifi_end;
            } else if ((argc == 3 && strlen(argv[2]) > 64)) {
                err_code = AT_CMD_ERR_WIFI_ENTAP_EAP_PWD_LEN;
                goto atcmd_wifi_end;
            }

            da16x_set_nvcache_str(DA16X_CONF_STR_EAP_IDENTITY, argv[1]);

            if (argc == 3) {
                da16x_set_nvcache_str(DA16X_CONF_STR_EAP_PASSWORD, argv[2]);
            }

            da16x_nvcache2flash();
        }
    }
#endif // __SUPPORT_WPA_ENTERPRISE__
    // AT+WFSAP
    else if (strcasecmp(argv[0] + 5, "SAP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFSAP=? */
            char val_0[33] = {0, }, val_3[66] = {0, }, val_5[3] = {0, };
            int val_1, val_2, val_4;

            if (da16x_get_config_str(DA16X_CONF_STR_SSID_1, val_0) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_SOFTAP_SSID_NO_VALUE;
                goto atcmd_wifi_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_AUTH_MODE_1, &val_1);
            da16x_get_config_int(DA16X_CONF_INT_CHANNEL, &val_4);
            da16x_get_config_str(DA16X_CONF_STR_COUNTRY, val_5);

            if (val_1 == CC_VAL_AUTH_OPEN
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                || val_1 == CC_VAL_AUTH_OWE
#endif // __SUPPORT_WPA3_PERSONAL__
            ) {
                sprintf(value_str, "'%s',%d,%d,%s", val_0, val_1, val_4, val_5);
            } else if (val_1 > CC_VAL_AUTH_WEP) {
                da16x_get_config_int(DA16X_CONF_INT_ENCRYPTION_1, &val_2);
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                if (val_1 == CC_VAL_AUTH_SAE) {
                    da16x_get_config_str(DA16X_CONF_STR_SAE_PASS_1, val_3);
                } else
#endif // __SUPPORT_WPA3_PERSONAL__
                    da16x_get_config_str(DA16X_CONF_STR_PSK_1, val_3);

                sprintf(value_str, "'%s',%d,%d,'%s',%d,%s", val_0, val_1, val_2, val_3, val_4, val_5);
            }
        } else {
            char *psk = NULL;
            char *psk_str = NULL;
            int auth_mode, enc_mode;

            /*
                AT+WFSAP=<ssid>,<auth>,[enc],[key],[ch],[country]

                <auth>=OPEN / WPA3_OWE:
                case1>argc=3 : AT+WFSAP=<ssid>,<auth>
                case2>argc=4 : AT+WFSAP=<ssid>,<auth>,[ch]
                case3>argc=5 : AT+WFSAP=<ssid>,<auth>,[ch],[country]

                <auth>=WPA / WPA2 / WPA+WPA2 / WPA3_SAE / WPA2+WPA3_SAE
                case4>argc=5 : AT+WFSAP=<ssid>,<auth>,<enc>,<key>
                case5>argc=6 : AT+WFSAP=<ssid>,<auth>,<enc>,<key>,[ch]
                case6>argc=7 : AT+WFSAP=<ssid>,<auth>,<enc>,<key>,[ch],[country]
            */
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_wifi_end;
            }

            if (get_int_val_from_str(argv[2], &auth_mode, POL_1) != 0) {
                err_code = AT_CMD_ERR_WIFI_SOFTAP_SECU_MODE;
                goto atcmd_wifi_end;
            }
            if (auth_mode < 0
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                 || auth_mode > CC_VAL_AUTH_RSN_SAE
#else
                 || auth_mode > CC_VAL_AUTH_WPA_AUTO
#endif // __SUPPORT_WPA3_PERSONAL__
                ) {
                err_code = AT_CMD_ERR_WIFI_SOFTAP_SECU_MODE;
                goto atcmd_wifi_end;
            }

            // sanity check: <ch>, <country>
            if (((auth_mode == CC_VAL_AUTH_OPEN 
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                 || auth_mode == CC_VAL_AUTH_OWE
#endif /* __SUPPORT_WPA3_PERSONAL__ */
                ) && argc >= 4) || 
                (auth_mode > CC_VAL_AUTH_WEP && argc >= 6)) {
                /*
                 *  cases where channel info exists.
                 *      <case2>, <case3>
                 *      <case5>, <case6>
                 */

                int  seq = argc >= 6 ? 6 : 4;
                int  chan;
                char country[4] = {0, };
                int  tmp = 0;
                int validCountry = 0;
                if (get_int_val_from_str(argv[seq - 1], &chan, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_TYPE;
                    goto atcmd_wifi_end;
                }

                if (auth_mode == CC_VAL_AUTH_OPEN && argc >= 6) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_OPEN_TOO_MANY_ARG;
                    goto atcmd_wifi_end;
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                } else if (auth_mode == CC_VAL_AUTH_OWE && argc >= 6) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_OWE_TOO_MANY_ARG;
                    goto atcmd_wifi_end;
#endif /* __SUPPORT_WPA3_PERSONAL__ */
                } else if (chan < 0 || chan > 14) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_RANGE;
                    goto atcmd_wifi_end;
                }

                if (argc == 4 || argc == 6) {
                    da16x_get_config_str(DA16X_CONF_STR_COUNTRY, country);
                } else {
                    strcpy(country, argv[seq]);
                }
                for (unsigned int i = 0; i < NUM_SUPPORTS_COUNTRY; i++) {
                    const struct country_ch_power_level *field = (void *)cc_power_table(i);

                    if (strcasecmp(country, field->country) == 0) {
                        validCountry = 1;
                        if (!chan || field->ch[chan - 1] != 0xF) {
                            tmp = 1;
                        }
                        break;
                    } else {
                        validCountry = 0;
                    }
                }
                if (!validCountry) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_CC_VALUE;
                    goto atcmd_wifi_end;
                }
                if (!tmp) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_RANGE;
                    goto atcmd_wifi_end;
                }
            }

            if (auth_mode == CC_VAL_AUTH_WEP) {
                err_code = AT_CMD_ERR_WIFI_SOFTAP_WEP_NOT_SUPPORT;
                goto atcmd_wifi_end;
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
            } else if (auth_mode == CC_VAL_AUTH_OWE) {
                // skip
#endif // __SUPPORT_WPA3_PERSONAL__
            } else if (auth_mode > CC_VAL_AUTH_WEP) {
                // for cases except where <enc>, and <key> exist

                int key_len;

                if (get_int_val_from_str(argv[3], &enc_mode, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_ENC_MODE_TYPE;
                    goto atcmd_wifi_end;
                }

                if (enc_mode < 0 || 
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                   ((auth_mode == CC_VAL_AUTH_SAE || 
                     auth_mode == CC_VAL_AUTH_RSN_SAE) && enc_mode != CC_VAL_ENC_CCMP) ||
#endif // __SUPPORT_WPA3_PERSONAL__
                    enc_mode > CC_VAL_ENC_AUTO) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_ENC_MODE_RANGE;
                    goto atcmd_wifi_end;
                }

                psk = argv[4];
                psk_str = pvPortMalloc(MAX_PASSKEY_LEN + 1);     // for Hexa code
                if (psk_str == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_wifi_end;
                }

                /* Support Extended ASCII or UTF-8 for PSK
                 *    Do not use the argv[2+idx_adj] string directly. */
                memset(psk_str, 0, MAX_PASSKEY_LEN + 1);
                key_len = str_decode((u8 *)psk_str, MAX_PASSKEY_LEN, psk);
                if (key_len < 8 || key_len > 63) {
                    err_code = AT_CMD_ERR_WIFI_SOFTAP_PASSKEY_LEN;

                    if (psk_str != NULL) {
                        vPortFree(psk_str);
                    }
                    goto atcmd_wifi_end;
                }
            }

            // process <ssid> remove the current profile

            /* Support Extended ASCII or UTF-8 for SSID
             *    Do not use the argv[1] string directly. */
            memset(value_str, 0, ATCMD_WIFI_STR_LEN);
            value_int = str_decode((u8 *)value_str, MAX_SSID_LEN, argv[1]);
            if (value_int > MAX_SSID_LEN) {
                err_code = AT_CMD_ERR_WIFI_SOFTAP_SSID_LEN;
                goto atcmd_wifi_end;
            } else {
                da16x_cli_reply("remove_network 1", NULL, NULL);
            }

            /* Support Extended ASCII or UTF-8 for SSID */
            da16x_set_nvcache_str(DA16X_CONF_STR_SSID_1, value_str);

            // process <auth>
            /*
             *  for WPA3_OWE
             *      cli set_network 0/1 key_mgmt
             *
             *  For WPA3_SAE / RSN_SAE
             *      cli set_network 0/1 key_mgmt
             *      cli set_network 0/1 proto RSN
             */

            da16x_set_nvcache_int(DA16X_CONF_INT_AUTH_MODE_1, auth_mode);

#if defined ( __SUPPORT_WPA3_PERSONAL__ )
            // pmf set for WPA3
            if (auth_mode == CC_VAL_AUTH_OWE || auth_mode == CC_VAL_AUTH_SAE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_1, CC_VAL_MAND);
            } else if (auth_mode == CC_VAL_AUTH_RSN_SAE) {
                da16x_set_nvcache_int(DA16X_CONF_INT_PMF_80211W_1, CC_VAL_OPT);
            }
#endif // __SUPPORT_WPA3_PERSONAL__
            if (   auth_mode == CC_VAL_AUTH_OPEN
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                || auth_mode == CC_VAL_AUTH_OWE
#endif // __SUPPORT_WPA3_PERSONAL__
            ) {
                // process <ch>, <country_code> for OPEN / OWE

                if (argc >= 5) {
                    da16x_set_nvcache_str(DA16X_CONF_STR_COUNTRY, argv[4]);
                }

                if (argc >= 4) {
                    if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_TYPE;
                        goto atcmd_wifi_end;
                    }
                    da16x_set_nvcache_int(DA16X_CONF_INT_CHANNEL, tmp_int1);
                }
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                // add enc for OWE
                if (auth_mode == CC_VAL_AUTH_OWE)
                    da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_1, CC_VAL_ENC_CCMP);
#endif // __SUPPORT_WPA3_PERSONAL__
            } else if (auth_mode > CC_VAL_AUTH_WEP) {
                // process <enc>,<key>,<ch>,<country_code> for the other auth modes

                // <enc>
                da16x_set_nvcache_int(DA16X_CONF_INT_ENCRYPTION_1, enc_mode);
                // <key>
#if defined ( __SUPPORT_WPA3_PERSONAL__ )
                if (auth_mode == CC_VAL_AUTH_SAE) {
                    da16x_set_nvcache_str(DA16X_CONF_STR_SAE_PASS_1, psk_str);
                } else
#endif // __SUPPORT_WPA3_PERSONAL__
                {
                    da16x_set_nvcache_str(DA16X_CONF_STR_PSK_1, psk_str);
                }

                // <country_code>
                if (argc >= 7) {
                    da16x_set_nvcache_str(DA16X_CONF_STR_COUNTRY, argv[6]);
                }

                // <ch>
                if (argc >= 6) {
                    if (get_int_val_from_str(argv[5], &tmp_int1, POL_1) != 0) {
                        err_code = AT_CMD_ERR_WIFI_SOFTAP_CH_VALUE_TYPE;
                        goto atcmd_wifi_end;
                    }
                    da16x_set_nvcache_int(DA16X_CONF_INT_CHANNEL, tmp_int1);
                }
            }

            if ((run_mode == SYSMODE_AP_ONLY) || (run_mode == SYSMODE_STA_N_AP)) {
                da16x_cli_reply("ap stop", NULL, NULL);
                vTaskDelay(1);
                da16x_cli_reply("ap start", NULL, NULL);
            } else {
                da16x_set_nvcache_int(DA16X_CONF_INT_MODE, SYSMODE_AP_ONLY);
            }

            if (psk_str != NULL) {
                vPortFree(psk_str);
            }

            da16x_nvcache2flash();
        }
    }
    // AT+WFOAP
    else if (strcasecmp(argv[0] + 5, "OAP") == 0) {
        if ((run_mode != SYSMODE_AP_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("ap start", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_SOFTAP_START;
        }
    }
    // AT+WFTAP
    else if (strcasecmp(argv[0] + 5, "TAP") == 0) {
        if ((run_mode != SYSMODE_AP_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("ap stop", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_SOFTAP_STOP;
        }
    }
    // AT+WFRAP
    else if (strcasecmp(argv[0] + 5, "RAP") == 0) {
        if ((run_mode != SYSMODE_AP_ONLY) && (run_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        ret = da16x_cli_reply("ap restart", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_SOFTAP_RESTART;
        }
    }
    // AT+WFLCST
    else if (strcasecmp(argv[0] + 5, "LCST") == 0) {
        char *reply;

        if (!da16x_network_main_check_network_ready(WLAN1_IFACE)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        reply = pvPortMalloc(2048);
        memset(reply, 0, 2048);
        ret = da16x_cli_reply("all_sta", NULL, reply);

        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, reply);
        vPortFree(reply);
    }
    // AT+WFAPWM
    else if (strcasecmp(argv[0] + 5, "APWM") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFAPWM=? */
            da16x_get_config_int(DA16X_CONF_INT_WIFI_MODE_1, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFAPWM=<mode> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_WIFI_MODE_1, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPCH
    else if (strcasecmp(argv[0] + 5, "APCH") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFAPCH=? */
            da16x_get_config_int(DA16X_CONF_INT_CHANNEL, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFAPCH=<ch> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_CHANNEL, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_wifi_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPBI
    else if (strcasecmp(argv[0] + 5, "APBI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFAPBI=? */
            da16x_get_config_int(DA16X_CONF_INT_BEACON_INTERVAL, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFAPBI=<interval> */

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }
            if (da16x_set_config_int(DA16X_CONF_INT_BEACON_INTERVAL, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_wifi_end;
            }

            if ((run_mode == SYSMODE_AP_ONLY) || (run_mode == SYSMODE_STA_N_AP)) {
                ret = da16x_cli_reply("ap restart", NULL, value_str);

                if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                    err_code = AT_CMD_ERR_WIFI_CLI_SOFTAP_RESTART;
                }
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPUI
    else if (strcasecmp(argv[0] + 5, "APUI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFAPUI=? */
            da16x_get_config_int(DA16X_CONF_INT_INACTIVITY, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFAPUI=<timeout> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0 /*SUCCESS*/) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }
            if (da16x_set_config_int(DA16X_CONF_INT_INACTIVITY, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPRT
    else if (strcasecmp(argv[0] + 5, "APRT") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFAPRT=? */
            da16x_get_config_int(DA16X_CONF_INT_RTS_THRESHOLD, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFAPRT=<threshold> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
                if (tmp_int1 <= 0 || tmp_int1 > 2347) {
                    err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                    goto atcmd_wifi_end;
                }

                write_nvram_int("rts_threshold", tmp_int1);
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_RTS_THRESHOLD, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPDE
    else if (strcasecmp(argv[0] + 5, "APDE") == 0) {
        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 2) {
            char cmd[64];

            // Check validity for input MAC address string
            if (chk_valid_macaddr((char *)(argv[1])) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_WRONG_MAC_ADDR;
                goto atcmd_wifi_end;
            }

            sprintf(cmd, "deauthenticate %s", argv[1]);
            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_DEAUTHENTICATE;
            }
        } else {
            if (argc < 2)
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            else
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFAPDI
    else if (strcasecmp(argv[0] + 5, "APDI") == 0) {
        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_wifi_end;
        }

        if (argc == 2) {
            char cmd[64];

            // Check validity for input MAC address string
            if (chk_valid_macaddr((char *)(argv[1])) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_WRONG_MAC_ADDR;
                goto atcmd_wifi_end;
            }

            sprintf(cmd, "disassociate %s", argv[1]);
            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_DISASSOCIATE;
            }
        } else {
            if (argc < 2)
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            else
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }

    }
    // AT+WFWMM
    else if (strcasecmp(argv[0] + 5, "WMM") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFWMM=? */
            da16x_get_config_int(DA16X_CONF_INT_WMM, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFWMM=<wmm> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            // In case of STA mode or other run-modes
            if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
                if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                    err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                    goto atcmd_wifi_end;
                }

                write_nvram_int("wmm_enabled", tmp_int1);
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_WMM, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFWMP
    else if (strcasecmp(argv[0] + 5, "WMP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFWMP=? */
            da16x_get_config_int(DA16X_CONF_INT_WMM_PS, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFWMP=<wmmps> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_wifi_end;
            }

            // In case of STA mode or other run-modes
            if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
                if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                    err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                    goto atcmd_wifi_end;
                }

                write_nvram_int("wmm_ps_enabled", tmp_int1);
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_WMM_PS, tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#if defined ( __SUPPORT_P2P__ )
    // AT+WFFP2P
    else if (strcasecmp(argv[0] + 5, "FP2P") == 0) {
        ret = da16x_cli_reply("p2p_find", NULL, value_str);

        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_P2P_FIND;
        }
    }
    // AT+WFSP2P
    else if (strcasecmp(argv[0] + 5, "SP2P") == 0) {
        ret = da16x_cli_reply("p2p_stop_find", NULL, value_str);

        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_P2P_FIND_STOP;
        }
    }
    // AT+WFCP2P
    else if (strcasecmp(argv[0] + 5, "CP2P") == 0) {
        if (argc == 4) {
            /* AT+WFCP2P=<mac>,<wps>,<join> */
            char cmd[64];

            sprintf(cmd, "p2p_connect %s", argv[1]);

            if (atoi(argv[2]) == 0 && argv[2][0] != '0') {
                err_code = AT_CMD_ERR_WIFI_P2P_CONN_WPS_TYPE;
                goto atcmd_wifi_end;
            } else if (atoi(argv[3]) == 0 && argv[3][0] != '0') {
                err_code = AT_CMD_ERR_WIFI_P2P_CONN_JOIN_TYPE;
                goto atcmd_wifi_end;
            }

            if (atoi(argv[2]) == 0) {
                strcat(cmd, " pbc");
            } else if (atoi(argv[2]) == 1) {
                strcat(cmd, " pin");
            } else {
                err_code = AT_CMD_ERR_WIFI_P2P_UNSUPPORT_WPS_TYPE;
                goto atcmd_wifi_end;
            }

            if (atoi(argv[3]) == ATCMD_ON) {
                strcat(cmd, " join");
            } else if (atoi(argv[3]) != ATCMD_OFF) {
                err_code = AT_CMD_ERR_WIFI_P2P_CONN_JOIN_RANGE;
                goto atcmd_wifi_end;
            }

            ret = da16x_cli_reply(cmd, NULL, value_str);
            if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
                err_code = AT_CMD_ERR_WIFI_CLI_P2P_JOIN;
            }
        } else {
            if (argc < 4) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+WFDP2P
    else if (strcasecmp(argv[0] + 5, "DP2P") == 0) {
        ret = da16x_cli_reply("p2p_group_remove", NULL, value_str);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_P2P_GRP_REMOVE;
        }
    }
    // AT+WFPP2P
    else if (strcasecmp(argv[0] + 5, "PP2P") == 0) {
        char *reply;

        reply = pvPortMalloc(2048);
        memset(reply, 0, 2048);
        ret = da16x_cli_reply("p2p_peers", NULL, reply);
        if (ret < 0 || strcmp(value_str, "FAIL") == 0) {
            err_code = AT_CMD_ERR_WIFI_CLI_P2P_PEER_INFO;
        }

        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, reply);
        vPortFree(reply);
    }
    // AT+WFPLCH
    else if (strcasecmp(argv[0] + 5, "PLCH") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPLCH=? */
            da16x_get_config_int(DA16X_CONF_INT_P2P_LISTEN_CHANNEL, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFPLCH=<ch> */
            if (atoi(argv[1]) == 0 && argv[1][0] != '0') {
                err_code = AT_CMD_ERR_WIFI_CLI_P2P_LISTEN_CH_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_P2P_LISTEN_CHANNEL, atoi(argv[1])) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_CLI_P2P_LISTEN_CH_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFPOCH
    else if (strcasecmp(argv[0] + 5, "POCH") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPOCH=? */
            da16x_get_config_int(DA16X_CONF_INT_P2P_OPERATION_CHANNEL, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFPOCH=<ch> */
            if (atoi(argv[1]) == 0 && argv[1][0] != '0') {
                err_code = AT_CMD_ERR_WIFI_P2P_OPERATION_CH_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_P2P_OPERATION_CHANNEL, atoi(argv[1])) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_P2P_OPERATION_CH_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFPGI
    else if (strcasecmp(argv[0] + 5, "PGI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPGI=? */
            da16x_get_config_int(DA16X_CONF_INT_P2P_GO_INTENT, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFPGI=<intent> */
            if (atoi(argv[1]) == 0 && argv[1][0] != '0') {
                err_code = AT_CMD_ERR_WIFI_P2P_GO_INTENT_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_P2P_GO_INTENT, atoi(argv[1])) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_WIFI_P2P_GO_INTENT_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFPFT
    else if (strcasecmp(argv[0] + 5, "PFT") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPFT=? */
            da16x_get_config_int(DA16X_CONF_INT_P2P_FIND_TIMEOUT, &value_int);
            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            /* AT+WFPFT=<timeout> */
            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0)) {
                err_code = AT_CMD_ERR_WIFI_P2P_FIND_TIMEOUT_TYPE;
                goto atcmd_wifi_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_P2P_FIND_TIMEOUT, atoi(argv[1]))) {
                err_code = AT_CMD_ERR_WIFI_P2P_FIND_TIMEOUT_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+WFPDN
    else if (strcasecmp(argv[0] + 5, "PDN") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFPDN=? */
            da16x_get_config_str(DA16X_CONF_STR_DEVICE_NAME, value_str);
        } else if (argc == 2) {
            /* AT+WFPDN=<name> */
            if (da16x_set_config_str(DA16X_CONF_STR_DEVICE_NAME, argv[1])) {
                err_code = AT_CMD_ERR_NVRAM_READ;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // ( __SUPPORT_P2P__ )
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
    // AT+WFSMSAVE
    else if (strcasecmp(argv[0] + 5, "SMSAVE") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+WFSMSAVE=? */
            if (read_nvram_int((const char *)ENV_SWITCH_SYSMODE, &value_int) != 0) {
                err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;

                goto atcmd_wifi_end;
            }

            sprintf(value_str, "%d", value_int);
        } else if (argc == 2) {
            // Check validity
            if ((get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0)) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;

                goto atcmd_wifi_end;
            } else if (is_in_valid_range(tmp_int1, SYSMODE_STA_ONLY, SYSMODE_AP_ONLY) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;

                goto atcmd_wifi_end;
            }

            // Save "SWITCH_SYSMODE" to NVRAM which will be used as return sysmode
            status = write_nvram_int((const char *)ENV_SWITCH_SYSMODE, (int)tmp_int1);
            if (status == -1) {
                err_code = AT_CMD_ERR_DPM_SLEEP_STARTED;
            } else if (status == -2) {
                err_code = AT_CMD_ERR_NVRAM_WRITE;
            }
        }
    }
    // AT+WFMODESWTCH
    else if (strcasecmp(argv[0] + 5, "MODESWTCH") == 0) {
        status = factory_reset_btn_onetouch();
        if (status == pdTRUE) {
            PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, "OK");

            vTaskDelay(1);

            // Reboot to change runinng mode
            reboot_func(SYS_REBOOT);
        } else {
            err_code = AT_CMD_ERR_WIFI_CONCURRENT_NO_PROFILE;
        }
    }
#endif // __SUPPORT_WIFI_CONCURRENT__
    else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_wifi_end :
    if (   (strlen(value_str) > 0 && strncmp(value_str, "OK", 2) != 0)
        && (err_code == AT_CMD_ERR_CMD_OK) ) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, value_str);
    }

    vPortFree(value_str);

    return err_code;
}

int atcmd_network(int argc, char *argv[])
{
    int   err_code = AT_CMD_ERR_CMD_OK;
#if defined (__SUPPORT_OTA__)
    int   ret = 0;
#endif // __SUPPORT_OTA__
    char  result_str[160] = {0, };
    int   result_int;
    char  *dyn_mem = NULL;
    int   tmp_int1 = 0, tmp_int2 = 0;
    int   status;

    const int sys_mode = da16x_network_main_get_sysmode();

    memset(result_str, 0x00, 160);

    // AT+NWIP
    if (strcasecmp(argv[0] + 5, "IP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWIP=? */
            char ip_str[16] = {0, }, sm_str[16] = {0, }, gw_str[16] = {0, };

            switch (get_run_mode()) {
            case SYSMODE_STA_ONLY:
                da16x_get_config_str(DA16X_CONF_STR_IP_0, ip_str);
                da16x_get_config_str(DA16X_CONF_STR_NETMASK_0, sm_str);
                da16x_get_config_str(DA16X_CONF_STR_GATEWAY_0, gw_str);

                if (strlen(gw_str) == 0) {
                    strcpy(gw_str, "0.0.0.0");
                }

                sprintf(result_str, "%d,%s,%s,%s", 0, ip_str, sm_str, gw_str);
                break;

            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // ( __SUPPORT_P2P__ )
                da16x_get_config_str(DA16X_CONF_STR_IP_1, ip_str);
                da16x_get_config_str(DA16X_CONF_STR_NETMASK_1, sm_str);
                da16x_get_config_str(DA16X_CONF_STR_GATEWAY_1, gw_str);

                if (strlen(gw_str) == 0) {
                    strcpy(gw_str, "0.0.0.0");
                }

                sprintf(result_str, "%d,%s,%s,%s", 1, ip_str, sm_str, gw_str);
                break;

#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
                da16x_get_config_str(DA16X_CONF_STR_IP_0, ip_str);
                da16x_get_config_str(DA16X_CONF_STR_NETMASK_0, sm_str);
                da16x_get_config_str(DA16X_CONF_STR_GATEWAY_0, gw_str);

                if (strlen(gw_str) == 0) {
                    strcpy(gw_str, "0.0.0.0");
                }

                sprintf(result_str, "%d,%s,%s,%s", 0, ip_str, sm_str, gw_str);

                da16x_get_config_str(DA16X_CONF_STR_IP_1, ip_str);
                da16x_get_config_str(DA16X_CONF_STR_NETMASK_1, sm_str);
                da16x_get_config_str(DA16X_CONF_STR_GATEWAY_1, gw_str);

                if (strlen(gw_str) == 0) {
                    strcpy(gw_str, "0.0.0.0");
                }

                sprintf(result_str, "%s %d,%s,%s,%s", result_str, 1, ip_str, sm_str, gw_str);
                break;
#endif // __SUPPORT_WIFI_CONCURRENT__
            } // end of switch
        } else if (argc == 5) {
            /* AT+NWIP=<interface>,<ipaddress>,<netmask>,<gateway> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_IP_IFACE_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_IFACE_RANGE;
                goto atcmd_network_end;
            }

            if (is_in_valid_ip_class(argv[2]) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_INVALID_ADDR;
                goto atcmd_network_end;
            } else if (is_in_valid_ip_class(argv[4]) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_GATEWAY;
                goto atcmd_network_end;
            } else if (isvalidip(argv[3]) == pdFALSE
                       || ip4_addr_netmask_valid(ipaddr_addr(argv[3])) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_NETMASK;
                goto atcmd_network_end;
            } else if (isvalidIPsubnetRange(iptolong(argv[4]), iptolong(argv[2]), iptolong(argv[3])) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_GATEWAY;
                goto atcmd_network_end;
            }  else if (isvalidIPsubnetRange(iptolong(argv[2]), iptolong(argv[2]), iptolong(argv[3])) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_IP_INVALID_ADDR;
                goto atcmd_network_end;
            }

            if (ip_change(tmp_int1, argv[2], argv[3], argv[4], 1) != pdPASS) {
                err_code = AT_CMD_ERR_NW_NET_IF_NOT_INITIALIZE;
                goto atcmd_network_end;
            }

            if (tmp_int1 == WLAN0_IFACE) {
                da16x_set_config_int(DA16X_CONF_INT_DHCP_CLIENT_TMP_STATICIP, pdFALSE);
            }

        } else if (argc < 5 ) {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        } else { // argc > 5
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWDNS
    else if (strcasecmp(argv[0] + 5, "DNS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDNS=? */

            switch (get_run_mode()) {
            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // __SUPPORT_P2P__
                err_code = AT_CMD_ERR_NOT_SUPPORTED;
                break;

            case SYSMODE_STA_ONLY:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__
                if (da16x_get_config_str(DA16X_CONF_STR_DNS_0, result_str) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NO_RESULT;
                }
                break;
            }
        } else if (argc == 2) {
            /* AT+NWDNS=<dns_ip> */

            switch (get_run_mode()) {
            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // __SUPPORT_P2P__
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                break;

            case SYSMODE_STA_ONLY:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__
                if (da16x_set_config_str(DA16X_CONF_STR_DNS_0, argv[1])) {
                    err_code = AT_CMD_ERR_NW_IP_ADDR_CLASS;
                }
                break;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWDNS2
    else if (strcasecmp(argv[0] + 5, "DNS2") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDNS2=? */
            switch (get_run_mode()) {
            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // __SUPPORT_P2P__
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                break;

            case SYSMODE_STA_ONLY:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__
                if (da16x_get_config_str(DA16X_CONF_STR_DNS_2ND, result_str) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NO_RESULT;
                }
                break;
            }
        } else if (argc == 2) {
            /* AT+NWDNS2=<dns_2nd_ip> */

            switch (get_run_mode()) {
            case SYSMODE_AP_ONLY:
#if defined ( __SUPPORT_P2P__ )
            case SYSMODE_P2P_ONLY:
            case SYSMODE_P2PGO:
#endif // __SUPPORT_P2P__
                err_code = AT_CMD_ERR_NOT_SUPPORTED;
                break;

            case SYSMODE_STA_ONLY:
#if defined ( __SUPPORT_WIFI_CONCURRENT__ )
            case SYSMODE_STA_N_AP:
  #if defined ( __SUPPORT_P2P__ )
            case SYSMODE_STA_N_P2P:
  #endif // __SUPPORT_P2P__
#endif // __SUPPORT_WIFI_CONCURRENT__
                if (da16x_set_config_str(DA16X_CONF_STR_DNS_2ND, argv[1])) {
                    err_code = AT_CMD_ERR_NW_IP_ADDR_CLASS;
                }
                break;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWHOST
    else if (strcasecmp(argv[0] + 5, "HOST") == 0) {
        if (argc == 2) {
            /* AT+NWHOST=<ip> */
            ULONG    dns_query_wait_option = 400;    // Default 400 msec
            char    *ip = dns_A_Query(argv[1], dns_query_wait_option);

            if (ip == NULL) {
                err_code = AT_CMD_ERR_NW_DNS_A_QUERY_FAIL;
            } else {
                PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, ip);
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWPING
    else if (strcasecmp(argv[0] + 5, "PING") == 0) {
        if (argc == 4) {
            /* AT+NWPING=<iface>,<dst_ip>,<count> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_PING_IFACE_ARG_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_PING_IFACE_ARG_RANGE;
                goto atcmd_network_end;
            }

            if (!da16x_network_main_check_network_ready(tmp_int1)) {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                goto atcmd_network_end;
            }

            if ((is_in_valid_ip_class(argv[2])) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_PING_DST_ADDR;
                goto atcmd_network_end;
            }
            if ((atol(argv[3]) < 1 ) || (strlen(argv[3]) != getDigitNum(atol(argv[3]))) ) {
                err_code = AT_CMD_ERR_NW_PING_TX_COUNT;
                goto atcmd_network_end;
            }
            err_code = da16x_ping_client(tmp_int1,
                                NULL,
                                (ULONG)iptolong(argv[2]),
                                NULL,
                                NULL,
                                DEFAULT_PING_SIZE,
                                atol(argv[3]),
                                DEFAULT_PING_WAIT,
                                DEFAULT_INTERVAL,
                                pdTRUE,
                                result_str);

            if (err_code == pdFAIL) {
                err_code = AT_CMD_ERR_NW_PING_DST_ADDR;
            } else {
                err_code = AT_CMD_ERR_CMD_OK;
            }
        } else {
            if (argc < 4) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+NWDHC
    else if (strcasecmp(argv[0] + 5, "DHC") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHC=? */
            da16x_get_config_int(DA16X_CONF_INT_DHCP_CLIENT, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWDHC=<dhcpc> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            if ((result_int = da16x_set_config_int(DA16X_CONF_INT_DHCP_CLIENT, tmp_int1)) != CC_SUCCESS) {
                if (result_int == CC_FAILURE_RANGE_OUT) {
                    err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                } else if (result_int == CC_FAILURE_NOT_READY) {
                    err_code = AT_CMD_ERR_NW_DHCPC_START_FAIL;
                }
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#if defined ( __USER_DHCP_HOSTNAME__ )
    // AT+NWDHCHN
    else if (strcasecmp(argv[0] + 5, "DHCHN") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHCHN=? */
            if (da16x_get_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, result_str) != CC_SUCCESS) {
                return AT_CMD_ERR_NO_RESULT;
            }
        } else if (argc == 2) {
            if (strlen(argv[1]) > DHCP_HOSTNAME_MAX_LEN) {
                err_code = AT_CMD_ERR_NW_DHCPC_HOSTNAME_LEN;
                goto atcmd_network_end;
            }

            /* AT+NWDHCHN=<hostname> */
            if (da16x_set_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_DHCPC_HOSTNAME_TYPE;
                goto atcmd_network_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWDHCHNDEL
    else if (strcasecmp(argv[0] + 5, "DHCHNDEL") == 0) {
        if (argc == 1) {
            // Remove saved DHCP Client hostname in NVRAM area.
            if (da16x_set_config_str(DA16X_CONF_STR_DHCP_HOSTNAME, "")) {
                err_code = AT_CMD_ERR_NVRAM_ERASE;
                goto atcmd_network_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __USER_DHCP_HOSTNAME__
    // AT+NWDHR
    else if (strcasecmp(argv[0] + 5, "DHR") == 0) {
        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            return AT_CMD_ERR_COMMON_SYS_MODE;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHR=? */
            char s_ip[16] = {0, }, e_ip[16] = {0, };

            if (da16x_get_config_str(DA16X_CONF_STR_DHCP_START_IP, s_ip) != CC_SUCCESS) {
                return AT_CMD_ERR_NW_DHCPS_START_ADDR_NOT_EXIST;
            }

            if (da16x_get_config_str(DA16X_CONF_STR_DHCP_END_IP, e_ip) != CC_SUCCESS) {
                return AT_CMD_ERR_NW_DHCPS_END_ADDR_NOT_EXIST;
            }

            sprintf(result_str, "%s,%s", s_ip, e_ip);
        } else if (argc == 3) {
            /* AT+NWDHR=<start_ip>,<end_ip> */
            ULONG start_ip, end_ip, ip_addr, net_mask, gw_addr = 0;
            extern struct netif   *dhcps_netif;
            int startip_chk, endip_chk;

            start_ip = (ULONG)iptolong(argv[1]);
            end_ip = (ULONG)iptolong(argv[2]);

            da16x_get_config_int(DA16X_CONF_INT_DHCP_SERVER, &result_int);
            if (result_int == 1 /* DHCPD is in running state */) {
                ATCMD_DBG("Info: DHCP is in running state\n");

                ip_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->ip_addr));
                net_mask = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->netmask));
                gw_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->gw));
            } else {
                /* DHCPD is in stopped state */
                struct netif *netif = netif_get_by_index(WLAN1_IFACE + 2); // assume Softap mode

                ATCMD_DBG("Info: DHCP is NOT in running state\n");

                ip_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->ip_addr));
                net_mask = (ULONG)iptolong(ipaddr_ntoa(&netif->netmask));
                gw_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->gw));
            }

            startip_chk = is_in_valid_ip_class(argv[1]);
            endip_chk = is_in_valid_ip_class(argv[2]);

            if (startip_chk == pdTRUE && endip_chk == pdTRUE) {
                if (((ip_addr >> 8) != (start_ip >> 8)) || ((ip_addr >> 8) != (end_ip >> 8))) {
                    PRINTF("ERR: Failed to set range of IP_addr list.\n");
                    err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                    goto atcmd_network_end;
                }

                ATCMD_DBG("ip_addr=%lu, net_mask=%lu, gw_addr=%lu, start_ip=%lu, end_ip=%lu \n",
                            ip_addr, net_mask, gw_addr, start_ip, end_ip);

                if (isvalidIPsubnetRange(start_ip,  gw_addr, net_mask) == pdFALSE) {
                    PRINTF("ERR: Start IP_addr is out of range. \n");
                   err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                   goto atcmd_network_end;
                }

                if (isvalidIPsubnetRange(end_ip,  gw_addr, net_mask) == pdFALSE) {
                    PRINTF("ERR: End IP_addr is out of range. \n");
                   err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                   goto atcmd_network_end;
                }

                if (isvalidIPrange(ip_addr, start_ip, end_ip)) {
                    PRINTF("ERR: IP_addr is out of DHCP range. \n");
                   err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                   goto atcmd_network_end;
                }

                if (start_ip > end_ip) {
                    PRINTF("ERR: start_ip is bigger than end_ip. \n");
                   err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                   goto atcmd_network_end;
                }

                if ((end_ip - start_ip + 1) > DHCPS_MAX_LEASE) {
                    err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_OVERFLOW;
                    goto atcmd_network_end;
                }

                if (da16x_set_config_str(DA16X_CONF_STR_DHCP_START_IP, argv[1]) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_DHCPS_WRONG_START_IP_CLASS;
                    goto atcmd_network_end;
                }

                if (da16x_set_config_str(DA16X_CONF_STR_DHCP_END_IP, argv[2]) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_DHCPS_WRONG_END_IP_CLASS;
                }
            } else {
                if (startip_chk == pdFALSE) {
                    err_code = AT_CMD_ERR_NW_DHCPS_WRONG_START_IP_CLASS;
                } else {
                    err_code = AT_CMD_ERR_NW_DHCPS_WRONG_END_IP_CLASS;
                }
            }
        } else {
            if (argc == 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+NWDHLT
    else if (strcasecmp(argv[0] + 5, "DHLT") == 0) {
        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_network_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHLT=? */
            da16x_get_config_int(DA16X_CONF_INT_DHCP_LEASE_TIME, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWDHLT=<time> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, MIN_DHCP_SERVER_LEASE_TIME, MAX_DHCP_SERVER_LEASE_TIME) == pdFALSE) {
                err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_DHCP_LEASE_TIME, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWDHIP
    else if (strcasecmp(argv[0] + 5, "DHIP") == 0) {
        list_node *p = NULL;
        list_node *p_list = NULL;
        struct dhcps_pool *pdhcps_pool = NULL;
        UCHAR count_assigned = 0;
        char tmp[18];

        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_network_end;
        }

        dyn_mem = (char *)pvPortMalloc(34*6); // max up to ~204 bytes (1 sta = 34 x 6)
        if (dyn_mem == NULL) {
            err_code = AT_CMD_ERR_MEM_ALLOC;
            goto atcmd_network_end;
        }
        memset(dyn_mem, 0x00, 34*6);

        p_list = (list_node *)dhcps_option_info(CLIENT_POOL, 1);
        if (p_list != NULL) {
            p = p_list;

            while (p != NULL) {
                int is_valid = 0;
                memset(tmp, 0x00, 18);
                pdhcps_pool = p->pnode;

                sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
                                            pdhcps_pool->mac[0],
                                            pdhcps_pool->mac[1],
                                            pdhcps_pool->mac[2],
                                            pdhcps_pool->mac[3],
                                            pdhcps_pool->mac[4],
                                            pdhcps_pool->mac[5]);

                for (UCHAR iii = 0; iii < 6; iii++) {
                    if (strlen(atcmd_mac_table[iii]) == 0) {
                        continue;
                    } else if (strcmp(tmp, atcmd_mac_table[iii]) == 0) {
                        is_valid = 1;
                        break;
                    }
                }

                if (!is_valid) {
                    p = p->pnext;
                    continue;
                }

                if (count_assigned != 0) {
                    strcat(dyn_mem, ";");
                }

                sprintf(dyn_mem, "%s%s,%s", dyn_mem, tmp,
                        ipaddr_ntoa(&pdhcps_pool->ip));

                p = p->pnext;

                count_assigned++;
            }
        } else {
            return AT_CMD_ERR_NW_DHCPS_NO_CONNECTED_CLIENT;
        }

        if (!count_assigned) {
            return AT_CMD_ERR_NW_DHCPS_NO_CONNECTED_CLIENT;
        }
    }
#ifdef __SUPPORT_MESH__
    // AT+NWDHDNS
    else if (strcasecmp(argv[0] + 5, "DHDNS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHDNS=? */
            if (da16x_get_config_str(DA16X_CONF_STR_DHCP_DNS, result_str) != CC_SUCCESS) {
                return AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_NOT_EXIST;
            }
        } else if (argc == 2) {
            /* AT+NWDHDNS=<dns_ip> */
            if (da16x_set_config_str(DA16X_CONF_STR_DHCP_DNS, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_IP_ADDR_CLASS;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __SUPPORT_MESH__
    // AT+NWDHS
    else if (strcasecmp(argv[0] + 5, "DHS") == 0) {
        if ((sys_mode != SYSMODE_AP_ONLY) && (sys_mode != SYSMODE_STA_N_AP)) {
            err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            goto atcmd_network_end;
        }

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWDHS=? */
            da16x_get_config_int(DA16X_CONF_INT_DHCP_SERVER, &result_int);
            sprintf(result_str, "%d", result_int);

            if (result_int == 1) {
                char s_ip[16] = {0, }, e_ip[16] = {0, };
#ifdef __SUPPORT_MESH__
                char tmp_ip[16] = {0, };
#endif // __SUPPORT_MESH__

                da16x_get_config_str(DA16X_CONF_STR_DHCP_START_IP, s_ip);
                da16x_get_config_str(DA16X_CONF_STR_DHCP_END_IP, e_ip);
                da16x_get_config_int(DA16X_CONF_INT_DHCP_LEASE_TIME, &result_int);
                sprintf(result_str, "%s,%s,%s,%d", result_str, s_ip, e_ip, result_int);

#ifdef __SUPPORT_MESH__
                if (!da16x_get_config_str(DA16X_CONF_STR_DHCP_DNS, tmp_ip)) {
                    sprintf(result_str, "%s,%s", result_str, tmp_ip);
                }
#endif // __SUPPORT_MESH__
            }
        } else if (argc == 2) {
            /* AT+NWDHS=<dhcpd> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_DHCP_SERVER, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else if (argc >= 4) {
            /* AT+NWDHS=<dhcpd(=1)>,<start_ip>,<end_ip>(,<lease_time>) */
            ULONG start_ip, end_ip, ip_addr, net_mask, gw_addr = 0;
            extern struct netif   *dhcps_netif;
            int startip_chk, endip_chk;
            struct netif *netif;

            /* verify <start_ip>, <end_ip> : argv[2], argv[3] */
            start_ip = (ULONG)iptolong(argv[2]);
            end_ip = (ULONG)iptolong(argv[3]);

            da16x_get_config_int(DA16X_CONF_INT_DHCP_SERVER, &result_int);

            if (result_int == 1 /* DHCPD is in running state */) {
                ip_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->ip_addr));
                net_mask = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->netmask));
                gw_addr = (ULONG)iptolong(ipaddr_ntoa(&dhcps_netif->gw));
            } else {
                /* DHCPD is in stopped state */
                netif = netif_get_by_index(WLAN1_IFACE + 2); // assume Softap mode
                ip_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->ip_addr));
                net_mask = (ULONG)iptolong(ipaddr_ntoa(&netif->netmask));
                gw_addr = (ULONG)iptolong(ipaddr_ntoa(&netif->gw));
            }

            startip_chk = is_in_valid_ip_class(argv[2]);
            endip_chk = is_in_valid_ip_class(argv[3]);

            if (startip_chk == pdFALSE) {
                err_code = AT_CMD_ERR_NW_DHCPS_WRONG_START_IP_CLASS;
                goto atcmd_network_end;
            } else if (endip_chk == pdFALSE) {
                err_code = AT_CMD_ERR_NW_DHCPS_WRONG_END_IP_CLASS;
                goto atcmd_network_end;
            }

            if (((ip_addr >> 8) != (start_ip >> 8)) || ((ip_addr >> 8) != (end_ip >> 8))) {
                err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
                goto atcmd_network_end;
            }

            if (isvalidIPsubnetRange(start_ip,  gw_addr, net_mask) == pdFALSE) {
                PRINTF("ERR: Start IP_addr is out of range. \n");
               err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
               goto atcmd_network_end;
            }

            /* AT+NWDHS=<dhcpd(=1)>,<start_ip>,<end_ip>(,<lease_time>) */
            if (isvalidIPsubnetRange(end_ip,  gw_addr, net_mask) == pdFALSE) {
                PRINTF("ERR: End IP_addr is out of range. \n");
               err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
               goto atcmd_network_end;
            }

            if (start_ip > end_ip) {
                PRINTF("ERR: start_ip is bigger than end_ip. \n");
               err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_MISMATCH;
               goto atcmd_network_end;
            }

            if ((end_ip - start_ip + 1) > DHCPS_MAX_LEASE) {
                err_code = AT_CMD_ERR_NW_DHCPS_IPADDR_RANGE_OVERFLOW;
                goto atcmd_network_end;
            }

            /* verify <dhcpd(=1)> : argv[1] */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_DHCPS_RUN_FLAG_TYPE;
                goto atcmd_network_end;
            }
            if (tmp_int1 != 1) {
                err_code = AT_CMD_ERR_NW_DHCPS_RUN_FLAG_VAL;
                goto atcmd_network_end;
            }

            if (argc == 5) {
                /* verify and set <lease_time> : argv[4] */

                if (get_int_val_from_str(argv[4], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_TYPE;
                    goto atcmd_network_end;
                } else if (is_in_valid_range(tmp_int2, MIN_DHCP_SERVER_LEASE_TIME, MAX_DHCP_SERVER_LEASE_TIME) == pdFALSE) {
                    err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE;
                    goto atcmd_network_end;
                }

                if (da16x_set_config_int(DA16X_CONF_INT_DHCP_LEASE_TIME, tmp_int2) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE;
                    goto atcmd_network_end;
                }
#ifdef __SUPPORT_MESH__
            } else if (argc == 5 && is_in_valid_ip_class(argv[4])) {
                if (da16x_set_config_str(DA16X_CONF_STR_DHCP_DNS, argv[4]) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_CLASS;
                    goto atcmd_network_end;
                }
            } else if (argc == 6) {
                if (get_int_val_from_str(argv[4], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_CLASS;
                    goto atcmd_network_end;
                }
                if (da16x_set_config_int(DA16X_CONF_INT_DHCP_LEASE_TIME, tmp_int2)) {
                    err_code = AT_CMD_ERR_NW_DHCPS_LEASE_TIME_RANGE;
                    goto atcmd_network_end;
                }

                if (da16x_set_config_str(DA16X_CONF_STR_DHCP_DNS, argv[5])) {
                    err_code = AT_CMD_ERR_NW_DHCPS_DNS_SVR_ADDR_CLASS;
                    goto atcmd_network_end;
                }
#endif // __SUPPORT_MESH__
            }

            /* Set <start_ip>, <end_ip>, <dhcpd=1> */
            da16x_set_config_str(DA16X_CONF_STR_DHCP_START_IP, argv[2]);
            da16x_set_config_str(DA16X_CONF_STR_DHCP_END_IP, argv[3]);
            da16x_set_config_int(DA16X_CONF_INT_DHCP_SERVER, tmp_int1);
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWSNS
    else if (strcasecmp(argv[0] + 5, "SNS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNS=? */
            da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_IP, result_str);
        } else if (argc == 2) {
            /* AT+NWSNS=<server_ip> */
            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_IP, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    else if (strcasecmp(argv[0] + 5, "SNS1") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNS1=? */
            da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_1_IP, result_str);
        } else if (argc == 2) {
            /* AT+NWSNS1=<server_ip> */
            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_1_IP, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    else if (strcasecmp(argv[0] + 5, "SNS2") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNS1=? */
            da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_2_IP, result_str);
        } else if (argc == 2) {
            /* AT+NWSNS1=<server_ip> */
            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_2_IP, argv[1])) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWSNUP
    else if (strcasecmp(argv[0] + 5, "SNUP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNUP=? */
            da16x_get_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWSNUP=<period> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 60, 129600) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWSNTP
    else if (strcasecmp(argv[0] + 5, "SNTP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNTP=? */
            da16x_get_config_int(DA16X_CONF_INT_SNTP_CLIENT, &result_int);
            sprintf(result_str, "%d", result_int);

            if (result_int == 1) {
                char tmp_ip[16] = {0, };

                da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_IP, tmp_ip);
                da16x_get_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, &result_int);
                sprintf(result_str, "%s,%s,%d", result_str, tmp_ip, result_int);
            }
        } else if (argc == 2) {
            /* AT+NWSNTP=<sntp> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }

            sntp_name_idx = 0;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
            }
        } else if (argc > 2) {
            /* AT+NWSNTP=<sntp(=1)>,<server_ip>(,<period>) */
            if (atoi(argv[1]) != 1) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_IP, argv[2]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
                goto atcmd_network_end;
            }

            if (argc == 4) {
                if (get_int_val_from_str(argv[3], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_TYPE;
                    goto atcmd_network_end;
                }
                if (da16x_set_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, tmp_int2)) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_RANGE;
                    goto atcmd_network_end;
                }
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }

            sntp_name_idx = 0;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
            }
        }
    }
    else if (strcasecmp(argv[0] + 5, "SNTP1") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNTP1=? */
            da16x_get_config_int(DA16X_CONF_INT_SNTP_CLIENT, &result_int);
            sprintf(result_str, "%d", result_int);

            if (result_int == 1) {
                char tmp_ip[16] = {0, };

                da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_1_IP, tmp_ip);
                da16x_get_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, &result_int);
                sprintf(result_str, "%s,%s,%d", result_str, tmp_ip, result_int);
            }
        } else if (argc == 2) {
            /* AT+NWSNTP1=<sntp> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }

            sntp_name_idx = 1;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1)) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                sntp_name_idx = 0;
            }
        } else if (argc > 2) {
            /* AT+NWSNTP1=<sntp(=1)>,<server_ip>(,<period>) */
            if (atoi(argv[1]) != 1) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_1_IP, argv[2]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
                goto atcmd_network_end;
            }

            if (argc == 4) {
                if (get_int_val_from_str(argv[3], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_TYPE;
                    goto atcmd_network_end;
                }
                if (da16x_set_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, tmp_int2) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_RANGE;
                    goto atcmd_network_end;
                }
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }
            sntp_name_idx = 1;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1)) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                sntp_name_idx = 0;
            }
        }
    }
    else if (strcasecmp(argv[0] + 5, "SNTP2") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWSNTP2=? */
            da16x_get_config_int(DA16X_CONF_INT_SNTP_CLIENT, &result_int);
            sprintf(result_str, "%d", result_int);

            if (result_int == 1) {
                char tmp_ip[16] = {0, };
                da16x_get_config_str(DA16X_CONF_STR_SNTP_SERVER_2_IP, tmp_ip);
                da16x_get_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, &result_int);
                sprintf(result_str, "%s,%s,%d", result_str, tmp_ip, result_int);
            }
        } else if (argc == 2) {
            /* AT+NWSNTP2=<sntp> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }

            sntp_name_idx = 2;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                sntp_name_idx = 0;
            }
        } else if (argc > 2) {
            /* AT+NWSNTP2=<sntp(=1)>,<server_ip>(,<period>) */
            if (atoi(argv[1]) != 1) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_SNTP_SERVER_2_IP, argv[2]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_NOT_SUPPORTED;
                goto atcmd_network_end;
            }

            if (argc == 4) {
                if (get_int_val_from_str(argv[3], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_TYPE;
                    goto atcmd_network_end;
                }

                if (da16x_set_config_int(DA16X_CONF_INT_SNTP_SYNC_PERIOD, tmp_int2) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_SNTP_PERIOD_RANGE;
                    goto atcmd_network_end;
                }
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_TYPE;
                goto atcmd_network_end;
            }

            sntp_name_idx = 2;
            if (da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_SNTP_FLAG_VAL;
                sntp_name_idx = 0;
            }
        }
    }
#if defined ( __SUPPORT_MQTT__ )
    // AT+NWMQBR
    else if (strcasecmp(argv[0] + 5, "MQBR") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQBR=? */
            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, result_str) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_NAME_NOT_FOUND;
                goto atcmd_network_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_MQTT_PORT, &result_int);
            sprintf(result_str, "%s,%d", result_str, result_int);
        } else if (argc == 3) {
            if (dpm_mode_is_enabled() == pdTRUE && is_mqtt_client_thd_alive() == pdTRUE) {
                    err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                    goto atcmd_network_end;
            }

            /* AT+NWMQBR=<ip>,<port> */
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_TYPE;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PORT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_RANGE;
                goto atcmd_network_end;
            }

            tmp_int2 = da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP, argv[1]);
            if (tmp_int2 != CC_SUCCESS) {
                if (tmp_int2 == CC_FAILURE_STRING_LENGTH) {
                    err_code = AT_CMD_ERR_NW_MQTT_BROKER_NAME_LEN;
                } else {
                    err_code = AT_CMD_ERR_NW_MQTT_UNKNOWN_OP_ID;
                }
                goto atcmd_network_end;
            }

            da16x_nvcache2flash();

            mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_BROKER_IP, argv[1], 0, 0);
            mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_PORT, tmp_int1);
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQQOS
    else if (strcasecmp(argv[0] + 5, "MQQOS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQQOS=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_QOS, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQQOS=<qos> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;

                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }
            if (is_in_valid_range(tmp_int1, 0, 2) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            status = da16x_set_config_int(DA16X_CONF_INT_MQTT_QOS, tmp_int1);
            switch (status) {
            case CC_FAILURE_RANGE_OUT :
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                break;
            case CC_FAILURE_NOT_SUPPORTED :
                err_code = AT_CMD_ERR_NW_MQTT_UNKNOWN_OP_ID;
                break;
            }

            if (status != CC_SUCCESS) {
                goto atcmd_network_end;
            }

            mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_QOS, tmp_int1);
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWMQTLS
    else if (strcasecmp(argv[0] + 5, "MQTLS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQTLS=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_TLS, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQTLS=<tls> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                PRINTF("Stop mqtt_client first before (re)configuration.\n");
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }
            if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            status = da16x_set_config_int(DA16X_CONF_INT_MQTT_TLS, tmp_int1);
            switch (status) {
            case CC_FAILURE_RANGE_OUT :
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                break;

            case CC_FAILURE_NOT_SUPPORTED :
                err_code = AT_CMD_ERR_NW_MQTT_UNKNOWN_OP_ID;
                break;
            }

            if (status != CC_SUCCESS) {
                goto atcmd_network_end;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_TLS, tmp_int1);
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#if defined (__MQTT_TLS_OPTIONAL_CONFIG__)
    // AT+NWMQALPN
    else if (strcasecmp(argv[0] + 5, "MQALPN") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQALPN=? */
            if (read_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, &result_int)) {
                char* result_str_pos = result_str;

                strcpy(result_str_pos, "1,\"");
                result_str_pos = result_str_pos + 3;

                err_code = AT_CMD_ERR_NW_MQTT_TLS_ALPN_NOT_EXIST;
                goto atcmd_network_end;
            } else {
                char *result_str_pos;

                if (result_int > 1) {
                    char *res_str;
                    int alloc_bytes = 2; // "[alpn_count],"

                    alloc_bytes = alloc_bytes +
                                  (result_int - 1) +                    // num (,)
                                  (2 * result_int) +                    // num (double quotation)
                                  (MQTT_TLS_ALPN_MAX_LEN * result_int); // num(alpn)

                     res_str = ATCMD_MALLOC(alloc_bytes+1);
                     if (res_str == NULL) {
                         err_code = AT_CMD_ERR_MEM_ALLOC;
                        goto atcmd_network_end;
                     }

                     dyn_mem = res_str; // set for freeing later

                     memset(res_str, 0, alloc_bytes+1);
                     result_str_pos = res_str;
                } else {
                    result_str_pos = result_str;
                }

                sprintf(result_str_pos, "%d", result_int);
                result_str_pos = result_str_pos + 1;

                for (int i = 0; i < result_int; i++) {
                    char nvrName[15] = {0, };
                    char *tmp_str;

                    sprintf(nvrName, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
                    tmp_str = read_nvram_string(nvrName);
                    sprintf(result_str_pos, ",\"%s\"", tmp_str);
                    result_str_pos = result_str_pos + (strlen(tmp_str) + 3); // 3 = 2x(") + 1x(,)
                }
            }
        } else if (argc >= 3) {
            /* AT+NWMQALPN=<#>,<alpn#n>,... */
            int tmp;

            if (is_mqtt_client_thd_alive() == pdTRUE && dpm_mode_is_enabled()) {
                PRINTF("Stop mqtt_client first before (re)configuration.\n");

                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_ALPN_COUNT_TYPE;
                goto atcmd_network_end;
            }

            /* Sanity check */
            if (argc - 2 > tmp_int1) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            } else if (argc - 2 < tmp_int1) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (tmp_int1 > MQTT_TLS_MAX_ALPN || tmp_int1 <= 0) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_ALPN_COUNT_RANGE;
                goto atcmd_network_end;
            }

            for (int i = 0; i < tmp_int1; i++) {
                if (strlen(argv[i + 2]) > MQTT_TLS_ALPN_MAX_LEN) {
                    err_code = AT_CMD_ERR_NW_MQTT_TLS_ALPN_NAME_LEN;
                    goto atcmd_network_end;
                }
            }

            if (!read_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, &tmp)) {
                for (int i = 0; i < tmp; i++) {
                    char nvr_name[15] = {0, };

                    sprintf(nvr_name, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
                    delete_tmp_nvram_env(nvr_name);
                }
            }

            for (int i = 0; i < tmp_int1; i++) {
                char nvr_name[15] = {0, };
                sprintf(nvr_name, "%s%d", MQTT_NVRAM_CONFIG_TLS_ALPN, i);
                write_tmp_nvram_string(nvr_name, argv[i + 2]);
            }

            write_tmp_nvram_int(MQTT_NVRAM_CONFIG_TLS_ALPN_NUM, tmp_int1);
            da16x_nvcache2flash();

            if (err_code == AT_CMD_ERR_CMD_OK && mqtt_client_is_cfg_dpm_mem_intact()) {
                int alpn_count = tmp_int1;

                mqttParams.tls_alpn_cnt = 0;
                for (int i = 0; i < alpn_count; i++) {
                    strncpy(mqttParams.tls_alpn[i], argv[i + 2], strlen(argv[i+2]));
                    mqttParams.tls_alpn_cnt++;
                }

                mqtt_client_save_to_dpm_user_mem();
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQSNI
    else if (strcasecmp(argv[0] + 5, "MQSNI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQSNI=? */
            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_TLS_SNI, result_str) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_SNI_NOT_EXIST;
                goto atcmd_network_end;
            }
        } else if (argc == 2) {
            /* AT+NWMQSNI=<sni> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;

                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_MQTT_TLS_SNI, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_SNI_LEN;
            }

            if ((err_code == AT_CMD_ERR_CMD_OK) && (mqtt_client_is_cfg_dpm_mem_intact())) {
                strcpy(mqttParams.tls_sni, argv[1]);
                mqtt_client_save_to_dpm_user_mem();
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWMQCSUIT
    else if (strcasecmp(argv[0] + 5, "MQCSUIT") == 0) {
        // MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            char *result_str_pos = NULL;
            char *tmp_str = NULL;
            char *res_str = NULL;
            int num_length, alloc_bytes = 0;

            /* AT+NWMQCSUIT=? */
            if (read_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, &result_int)) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NUM_NOT_EXIST;

                goto atcmd_network_end;
            }

            num_length = (result_int >= 10 ? 2 : 1);
            /* Assume that a length of each cipher suit is 4 .
             * Total length = a length of number of cipher suits + Number of comma +
             *                (a length of cipher suit(4) * number of cipher suits).
             */
            alloc_bytes = num_length + (result_int - 1) + (4 * result_int);
            res_str = ATCMD_MALLOC(alloc_bytes + 1);
            if (res_str == NULL) {
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto atcmd_network_end;
            }

            /* Set for freeing later */
            dyn_mem = res_str;
            memset(res_str, 0, alloc_bytes + 1);
            result_str_pos = res_str;

            /* Write number of cipher suits and cipher suits */
            sprintf(result_str_pos, "%d", result_int);
            result_str_pos = result_str_pos + num_length;

            tmp_str = read_nvram_string(MQTT_NVRAM_CONFIG_TLS_CSUITS);
            if (tmp_str == NULL) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NOT_EXIST;
            } else {
                sprintf(result_str_pos, ",%s", tmp_str);
            }
        } else if (argc >= 2) {
            char *result_str_pos;
            char *res_str, num_cipher_suits = 0;
            int arg_idx, alloc_bytes = 0;

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (argc > (MQTT_TLS_MAX_CSUITS + 1)) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto atcmd_network_end;
            }

            num_cipher_suits = argc - 1;
            /* Assume that a length of each cipher suit is 4 .
             * Total length = Number of comma +
             *                (a length of cipher suit(4) * number of cipher suits).
             */
            alloc_bytes = (num_cipher_suits - 1) + (4 * num_cipher_suits);
            res_str = ATCMD_MALLOC(alloc_bytes + 1);
            if (res_str == NULL) {
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto atcmd_network_end;
            }

            /* Delete the data stored */
            delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_TLS_CSUITS);

            result_str_pos = res_str;
            memset(res_str, 0, alloc_bytes + 1);
            if (write_tmp_nvram_int(MQTT_NVRAM_CONFIG_TLS_CSUIT_NUM, num_cipher_suits)) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NUM_NVRAM_WR;
                goto atcmd_network_end;
            }

            arg_idx = 1;
            sprintf(result_str_pos, "%s", argv[arg_idx]);
            result_str_pos += strlen(argv[arg_idx]);
            mqttParams.tls_csuits[0] = htoi(argv[arg_idx++]);

            for (int i = 0; i < argc - 2; i++, arg_idx++) {
                sprintf(result_str_pos, ",%s", argv[arg_idx]);
                mqttParams.tls_csuits[i] = htoi(argv[arg_idx]);
                result_str_pos += (strlen(argv[arg_idx]) + 1);
            }

            mqttParams.tls_csuits_cnt = num_cipher_suits;
            if (write_tmp_nvram_string(MQTT_NVRAM_CONFIG_TLS_CSUITS, res_str)) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_CSUITE_NVRAM_WR;
                goto atcmd_network_end;
            }
            da16x_nvcache2flash();
            mqtt_client_save_to_dpm_user_mem();
            if (res_str) {
                ATCMD_FREE(res_str);
            }
        }
    }
#endif // __MQTT_TLS_OPTIONAL_CONFIG__
    // AT+NWMQTS
    else if (strcasecmp(argv[0] + 5, "MQTS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQTS=? */
            if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &result_int)) {
                char *result_str_pos = result_str;

                strcpy(result_str_pos, "1,\"");
                result_str_pos = result_str_pos + 3;
                if (da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, result_str_pos) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NOT_EXIST;
                    goto atcmd_network_end;
                }

                result_str_pos = result_str_pos + strlen(result_str_pos);
                strcpy(result_str_pos, "\"");
            } else {
                char *result_str_pos;

                if (result_int > 1) {
                    char *res_str_mqts;
                    int alloc_bytes = 2; // "[topic_count],"

                    alloc_bytes = alloc_bytes +
                                  (result_int-1) +                        // num (,)
                                  (2*result_int) +                        // num (double quotation)
                                  (MQTT_TOPIC_MAX_LEN * result_int);    // num(topic)

                     res_str_mqts = (char *)pvPortMalloc(alloc_bytes + 1); // max up to ~270 bytes
                     if (res_str_mqts == NULL) {
                         err_code = AT_CMD_ERR_MEM_ALLOC;
                        goto atcmd_network_end;
                     }

                     dyn_mem = res_str_mqts; // set for freeing later

                     memset(res_str_mqts, 0, alloc_bytes+1);
                     result_str_pos = res_str_mqts;
                } else {
                    result_str_pos = result_str;
                }

                sprintf(result_str_pos, "%d", result_int);
                result_str_pos = result_str_pos + 1;

                for (int i = 0; i < result_int; i++) {
                    char topics[16] = {0, };
                    char *tmp_str;

                    sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
                    tmp_str = read_nvram_string(topics);
                    sprintf(result_str_pos, ",\"%s\"", tmp_str);
                    result_str_pos = result_str_pos + (strlen(tmp_str) + 3); // 3 = 2x(") + 1x(,)
                }
            }
        } else if (argc >= 3) {
            /* AT+NWMQTS=<#>,<topic#n>,... */
            int tmp_sub_topic_cnt;

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_TYPE;
                goto atcmd_network_end;
            }

            // check argument count
            if (argc - 2 > tmp_int1) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            } else if (argc - 2 < tmp_int1) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            // check topic count
            if (tmp_int1 > MQTT_MAX_TOPIC || tmp_int1 <= 0) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_RANGE;
                goto atcmd_network_end;
            }

            // check topic length
            for (int i = 0; i < tmp_int1; i++) {
                if (strlen(argv[i + 2]) > MQTT_TOPIC_MAX_LEN) {
                    err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_LEN;
                    goto atcmd_network_end;
                }
            }

            // check duplicate topic
            extern int is_duplicate_string_found(char **str_array, int str_count);
            if (is_duplicate_string_found(&(argv[2]), tmp_int1)) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_DUP;
                goto atcmd_network_end;
            }

            if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &tmp_sub_topic_cnt) == 0) {
                for (int i = 0; i < tmp_sub_topic_cnt; i++) {
                    char topics[16] = {0, };

                    sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
                    delete_tmp_nvram_env(topics);
                }
            } else {
                char *topic = NULL;

                topic = read_nvram_string(MQTT_NVRAM_CONFIG_SUB_TOPIC);
                if (strlen(topic)) {
                    delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
                }
            }

            for (int i = 0; i < tmp_int1; i++) {
                char topics[16] = {0, };

                sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
                write_tmp_nvram_string(topics, argv[i + 2]);
            }

            if (write_tmp_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, tmp_int1)) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_NVRAM_WR;
                goto atcmd_network_end;
            }

            da16x_nvcache2flash();

            if (err_code == AT_CMD_ERR_CMD_OK && mqtt_client_is_cfg_dpm_mem_intact()) {
                int topic_cnt = tmp_int1;

                mqttParams.topic_count = 0;
                for (int i = 0; i < topic_cnt; i++) {
                    mqttParams.topic_count++;
                    strcpy(mqttParams.topics[mqttParams.topic_count - 1], argv[i + 2]);
                }

                mqtt_client_save_to_dpm_user_mem();
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQATS
    else if (strcasecmp(argv[0] + 5, "MQATS") == 0) {
        if (argc == 2) {
            char *nvram_read_topic;
            char nvram_tag[16] = {0, };
            int sub_topic_num = 0;
            char *tmp_topics = NULL;
            int free_topic_pos = 0;

            if (is_correct_query_arg(argc, argv[1])) {
                err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_network_end;
            }

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (strlen(argv[1]) > MQTT_TOPIC_MAX_LEN) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_LEN;

                goto atcmd_network_end;
            }

            if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &sub_topic_num) == CC_SUCCESS) {
                if (sub_topic_num >= MQTT_MAX_TOPIC) {
                    err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NUM_OVERFLOW;
                    goto atcmd_network_end;
                }

                tmp_topics = ATCMD_MALLOC(MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));
                if (tmp_topics == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_network_end;
                }
                memset(tmp_topics, 0x00, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));

                for (int i = 0; i < sub_topic_num; i++) {
                    memset(nvram_tag, 0x00, 16);
                    sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
                    nvram_read_topic = read_nvram_string(nvram_tag);

                    if (strcmp(nvram_read_topic, argv[1]) == 0) {
                        err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_ALREADY_EXIST;
                        ATCMD_FREE(tmp_topics);
                        goto atcmd_network_end;
                    }

                    if (strlen(nvram_read_topic) > 0) {
                        int str_pos = i * (MQTT_TOPIC_MAX_LEN + 1);

                        strcpy((char*)(tmp_topics+str_pos), nvram_read_topic);
                    }
                }

                sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, sub_topic_num);
                free_topic_pos = sub_topic_num*(MQTT_TOPIC_MAX_LEN + 1);
            } else {
                tmp_topics = ATCMD_MALLOC(MQTT_MAX_TOPIC * (MQTT_TOPIC_MAX_LEN + 1));
                if (tmp_topics == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_network_end;
                }
                memset(tmp_topics, 0x00, MQTT_MAX_TOPIC * (MQTT_TOPIC_MAX_LEN + 1));

                nvram_read_topic = read_nvram_string(MQTT_NVRAM_CONFIG_SUB_TOPIC);

                if (strlen(nvram_read_topic)) {
                    delete_tmp_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
                }

                sub_topic_num = 0;
                memset(nvram_tag, 0x00, 16);
                sprintf(nvram_tag, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, sub_topic_num);
            }

            sub_topic_num += 1;
            write_tmp_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, sub_topic_num);
            write_tmp_nvram_string(nvram_tag, argv[1]);
            strcpy((char*)(tmp_topics+free_topic_pos), argv[1]);
            da16x_nvcache2flash();

            if (mqtt_client_is_cfg_dpm_mem_intact()) {
                mqttParams.topic_count = sub_topic_num;
                memcpy(mqttParams.topics, tmp_topics, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));
                mqtt_client_save_to_dpm_user_mem();
            }

            ATCMD_FREE(tmp_topics);
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQDTS
    else if (strcasecmp(argv[0] + 5, "MQDTS") == 0) {
        if (argc == 2) {
            int  ret_num;
            char *tmp_topics = NULL;
            int  free_topic_pos = 0;

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            tmp_int1 = mqtt_client_del_sub_topic(argv[1], 0);

            /* AT+NWMQDTP=<topic> */
            if (tmp_int1 != 0) {
                if (tmp_int1 == 100) {
                    err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NOT_EXIST;
                } else if (tmp_int1 == 101) {
                    err_code = AT_CMD_ERR_UNKNOWN;
                }
                goto atcmd_network_end;
            }

            if (mqtt_client_is_cfg_dpm_mem_intact()) {
                tmp_topics = ATCMD_MALLOC(MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));
                if (tmp_topics == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_network_end;
                }
                memset(tmp_topics, 0x00, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));

                if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &ret_num) != 0) {
                    if (da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, result_str)) {
                        strcpy(result_str, "");
                    }
                    strcpy((char*)(tmp_topics+free_topic_pos), result_str);
                } else {
                    memset(result_str, '\0', MQTT_TOPIC_MAX_LEN + 1);

                    for (int i = 0; i < ret_num; i++) {
                        char topics[16] = {0, };
                        char *tmp_str;

                        sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
                        tmp_str = read_nvram_string(topics);

                        if (strlen(tmp_str) > 0) {
                            int str_pos = i*(MQTT_TOPIC_MAX_LEN + 1);
                            strcpy((char*)(tmp_topics+str_pos), tmp_str);
                        }
                    }
                }

                mqttParams.topic_count = ret_num;
                memcpy(mqttParams.topics, tmp_topics, MQTT_MAX_TOPIC*(MQTT_TOPIC_MAX_LEN + 1));
                mqtt_client_save_to_dpm_user_mem();

                ATCMD_FREE(tmp_topics);
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQUTS
    else if (strcasecmp(argv[0] + 5, "MQUTS") == 0) {
        if (argc == 2) {
            /* AT+NWMQUTS=<topic> */

            tmp_int1 = mqtt_client_unsub_topic(argv[1]);
            if (tmp_int1 != MOSQ_ERR_SUCCESS) {
                if (tmp_int1 == MOSQ_ERR_NO_CONN) {
                    err_code = AT_CMD_ERR_NW_MQTT_NOT_CONNECTED;
                } else if (tmp_int1 == MOSQ_ERR_NOT_FOUND) {
                    err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_NOT_EXIST;
                } else if (tmp_int1 == MOSQ_ERR_PROTOCOL) {
                    err_code = AT_CMD_ERR_NW_MQTT_PROTOCOL;
                } else if (tmp_int1 == MOSQ_ERR_NOMEM) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                } else {
                    err_code = AT_CMD_ERR_UNKNOWN;
                }
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQTP
    else if (strcasecmp(argv[0] + 5, "MQTP") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQTP=? */
            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, result_str) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_PUB_TOPIC_NOT_EXIST;
                goto atcmd_network_end;
            }
        } else if (argc == 2) {
            /* AT+NWMQTP=<topic> */
            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_PUB_TOPIC_LEN;
                goto atcmd_network_end;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_PUB_TOPIC, argv[1], 0, 0);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWMQPING
    else if (strcasecmp(argv[0] + 5, "MQPING") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQPING=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_PING_PERIOD, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQPING=<period> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_PING_PERIOD_TYPE;
                goto atcmd_network_end;
            }
            if (da16x_set_config_int(DA16X_CONF_INT_MQTT_PING_PERIOD, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_PING_PERIOD_RANGE;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_PING_PERIOD, tmp_int1);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWMQV311         "Use MQTT protocol v3.1.1. default is v3.1"
    else if (strcasecmp(argv[0] + 5, "MQV311") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQV311=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_VER311, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQV311=<1|0> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_MQTT_VER311, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_VER311, MQTT_PROTOCOL_V31 + tmp_int1);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWMQCID
    else if (strcasecmp(argv[0] + 5, "MQCID") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQCID=? */
            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, result_str) != CC_SUCCESS) {
                // generate default cid if there's no cid stored in NVM
                char mac_id[5] = {0,};
                extern void id_number_output(char *id_num);

                id_number_output(mac_id);

                sprintf(result_str, "%s_%s", "da16x", mac_id);
            }
        } else if (argc == 2) {
            /* AT+NWMQCID=<client_id> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_LEN;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, argv[1], 0, 0);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWMQLI
    else if (strcasecmp(argv[0] + 5, "MQLI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQLI=? */
            char tmp_name[MQTT_USERNAME_MAX_LEN+1] = {0, };
            char tmp_pw[MQTT_PASSWORD_MAX_LEN+1] = {0, };

            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_USERNAME, tmp_name) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_USERNAME_NOT_EXIST;
                goto atcmd_network_end;
            }

            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_PASSWORD, tmp_pw) != CC_SUCCESS) {
                sprintf(result_str, "%s", tmp_name);
            } else {
                sprintf(result_str, "%s,%s", tmp_name, tmp_pw);
            }

            goto atcmd_network_end;
        } else if (argc == 2) {
            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (da16x_set_config_str(DA16X_CONF_STR_MQTT_USERNAME, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_USERNAME_LEN;
                goto atcmd_network_end;
            }
        } else if (argc == 3) {
            /* AT+NWMQLI=<name>,<pw> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_USERNAME, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_USERNAME_LEN;
                goto atcmd_network_end;
            }

            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PASSWORD, argv[2]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_PASSWORD_LEN;
                goto atcmd_network_end;
            }

            da16x_nvcache2flash();
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }

        if (err_code == AT_CMD_ERR_CMD_OK && (argc == 2 || argc == 3)) {
            if (argc == 2) {
                mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_USERNAME, argv[1], 0, 0);
            } else if (argc == 3) {
                mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_USERNAME, argv[1], 0, 0);
                mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_PASSWORD, argv[2], 0, 0);
            }
        }
    }
    // AT+NWMQWILL
    else if (strcasecmp(argv[0] + 5, "MQWILL") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQWILL=? */
            char tmp_topic[MQTT_TOPIC_MAX_LEN+1] = {0, };
            char tmp_msg[MQTT_WILL_MSG_MAX_LEN+1] = {0, };

            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_WILL_TOPIC, tmp_topic) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_TOPIC_NOT_EXIST;
                goto atcmd_network_end;
            }

            if (da16x_get_config_str(DA16X_CONF_STR_MQTT_WILL_MSG, tmp_msg) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_MESSAGE_NOT_EXIST;
                goto atcmd_network_end;
            }

            da16x_get_config_int(DA16X_CONF_INT_MQTT_WILL_QOS, &result_int);
            sprintf(result_str, "%s,%s,%d", tmp_topic, tmp_msg, result_int);
        } else if (argc == 4) {
            /* AT+NWMQWILL=<topic>,<msg>,<qos> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_WILL_TOPIC, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_TOPIC_LEN;
                goto atcmd_network_end;
            }

            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_WILL_MSG, argv[2]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_MESSAGE_LEN;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_QOS_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_WILL_QOS, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_QOS_RANGE;
                goto atcmd_network_end;
            }

            da16x_nvcache2flash();

            mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_WILL_TOPIC, argv[1], 0, 0);
            mqtt_client_cfg_sync_rtm(DA16X_CONF_STR_MQTT_WILL_MSG, argv[2], 0, 0);
            mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_WILL_QOS, tmp_int1);

        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQDEL
    else if (strcasecmp(argv[0] + 5, "MQDEL") == 0) {
        if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
            err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
            goto atcmd_network_end;
        }

        mqtt_client_config_initialize();

        if (mqtt_client_is_cfg_dpm_mem_intact()) {
            memset(&mqttParams, 0x00, sizeof(mqttParamForRtm));
            mqttParams.port = MQTT_CONFIG_PORT_DEF;
            mqttParams.keepalive = MQTT_CONFIG_PING_DEF;

            mqtt_client_save_to_dpm_user_mem();
        }
    }
    // AT+NWMQCL
    else if (strcasecmp(argv[0] + 5, "MQCL") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQCL=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_SUB, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQCL=<mqtt_client> */

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }
            if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_MQTT_SUB, tmp_int1) != CC_SUCCESS) {
                // possible return : 9 (mqtt start fail)
                err_code = AT_CMD_ERR_NW_MQTT_CLIENT_TASK_START;
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    // AT+NWMQMSG
    else if (strcasecmp(argv[0] + 5, "MQMSG") == 0) {
        char *top;

        if (argc == 2 || argc == 3) {
            int rsp_val;

            /* AT+NWMQMSG=<msg>(,<topic>) (set only) */
            if (strlen(argv[1]) > MQTT_MSG_MAX_LEN) {
                err_code = AT_CMD_ERR_NW_MQTT_PUB_MESSAGE_LEN;
                goto atcmd_network_end;
            }

            if (argc == 2) {
                top = NULL;
            } else if (argc == 3) {
                if (strlen(argv[2]) > MQTT_TOPIC_MAX_LEN) {
                    err_code = AT_CMD_ERR_NW_MQTT_PUB_TOPIC_LEN;
                    goto atcmd_network_end;
                } else {
                    top = argv[2];
                }
            }

            rsp_val = mqtt_client_send_message(top, argv[1]);

            if (rsp_val == -1) {
                err_code = AT_CMD_ERR_NW_MQTT_NOT_CONNECTED;
            } else if (rsp_val == -2) {
                // in-flight message is in progress
                err_code = AT_CMD_ERR_NW_MQTT_PUB_TX_IN_PROGRESS;
            } else if (rsp_val == -3) {
                // no topic in mqtt configuration
                err_code = AT_CMD_ERR_NW_MQTT_PUB_TOPIC_NOT_EXIST;
            } else if (rsp_val == 9 /* MOSQ_ERR_PAYLOAD_SIZE */) {
                // paylod too long
                err_code = AT_CMD_ERR_NW_MQTT_PUB_MESSAGE_LEN;
            } else {
                // pre-check passed ...
                if (dpm_mode_is_enabled() == pdTRUE) {
                    if (mqtt_client_get_qos() > 0) { // qos 1 or 2
                        if (rsp_val != 0) {
                            mqtt_client_convert_to_atcmd_err_code(&rsp_val, &err_code);
                        }
                    } else {                          // qos 0
                        if (rsp_val != 0) {
                            mqtt_client_convert_to_atcmd_err_code(&rsp_val, &err_code);
                        } else {
                            // make async +NWMQMSGSND:1 to sync print
                            atcmd_mqtt_qos0_msg_send_done_in_dpm = TRUE;
                            atcmd_mqtt_qos0_msg_send_rc = 0;
                        }
                    }
                }
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQTT
    else if (strcasecmp(argv[0] + 5, "MQTT") == 0) {
        if (argc == 7 || argc == 9) {
            int tmp_int3;

            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_TYPE;
                goto atcmd_network_end;
            }
            if (get_int_val_from_str(argv[5], &tmp_int2, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_QOS_TYPE;
                goto atcmd_network_end;
            }
            if (get_int_val_from_str(argv[6], &tmp_int3, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_TYPE;
                goto atcmd_network_end;
            }

            /* AT+NWMQTT=<ip>,<port>,<sub_topic>,<pub_topic>,<qos>,<tls>(,<username>,<password>) (set only) */
            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP, argv[1]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_NAME_LEN;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_PORT, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_BROKER_PORT_NUM_RANGE;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC, argv[3]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_SUBS_TOPIC_LEN;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, argv[4]) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_PUB_TOPIC_LEN;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_QOS, tmp_int2) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_WILL_QOS_RANGE;
                goto atcmd_network_end;
            }
            if (da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_TLS, tmp_int3) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_NW_MQTT_TLS_RANGE;
                goto atcmd_network_end;
            }
            if (argc == 9) {
                if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_USERNAME, argv[7]) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_MQTT_USERNAME_LEN;
                    goto atcmd_network_end;
                }

                if (da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_PASSWORD, argv[8]) != CC_SUCCESS) {
                    err_code = AT_CMD_ERR_NW_MQTT_PASSWORD_LEN;
                    goto atcmd_network_end;
                }
            }

            da16x_set_nvcache_int(DA16X_CONF_INT_MQTT_AUTO, 1);
            da16x_nvcache2flash();
            reboot_func(SYS_REBOOT);
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    // AT+NWMQAUTO
    else if (strcasecmp(argv[0] + 5, "MQAUTO") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQAUTO=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_AUTO, &result_int);

            if (result_int == MQTT_INIT_MAGIC)
                result_int = 1;
            else
                result_int = 0;

            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQAUTO=<auto> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_MQTT_AUTO, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_AUTO, (!tmp_int1)?0:MQTT_INIT_MAGIC);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
    else if (strcasecmp(argv[0] + 5, "MQCS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWMQCS=? */
            da16x_get_config_int(DA16X_CONF_INT_MQTT_CLEAN_SESSION, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWMQCS=<clean_session> */

            if ((is_mqtt_client_thd_alive() == pdTRUE) && (dpm_mode_is_enabled() == pdTRUE)) {
                PRINTF("Stop mqtt_client first before (re)configuration.\n");
                err_code = AT_CMD_ERR_NW_MQTT_NEED_TO_STOP;
                goto atcmd_network_end;
            }

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_MQTT_CLEAN_SESSION, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }

            if (err_code == AT_CMD_ERR_CMD_OK) {
                mqtt_client_cfg_sync_rtm(0, NULL, DA16X_CONF_INT_MQTT_CLEAN_SESSION, tmp_int1);
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
#endif // __SUPPORT_MQTT__

#if defined (__SUPPORT_WPA_ENTERPRISE__)
    // AT+NWTLSV
    else if (strcasecmp(argv[0] + 5, "TLSV") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWTLSV=? */
            da16x_get_config_int(DA16X_CONF_INT_TLS_VER, &result_int);
            sprintf(result_str, "%d", result_int);
        } else if (argc == 2) {
            /* AT+NWTLSV=<ver> */
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            }

            if (da16x_set_config_int(DA16X_CONF_INT_TLS_VER, tmp_int1) != CC_SUCCESS) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __SUPPORT_WPA_ENTERPRISE__
    // AT+NWCCRT
    else if (strcasecmp(argv[0] + 5, "CCRT") == 0) {
        UINT16 chk = 0;

        // cert #1
        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CA_CERT)) {
            chk |= (1 << 2);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT)) {
            chk |= (1 << 1);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY)) {
            chk |= 1;
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_DH_PARAMS)) {
            chk |= (1 << 9);
        }

        // cert #2
        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_CA_CERT)) {
            chk |= (1 << 5);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_CERT)) {
            chk |= (1 << 4);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_PRIVATE_KEY)) {
            chk |= (1 << 3);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_HTTPS_CLIENT, DA16X_CERT_TYPE_DH_PARAMS)) {
            chk |= (1 << 10);
        }

        // cert #3 WPA Enterprise
        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_WPA_ENTERPRISE, DA16X_CERT_TYPE_CA_CERT)) {
            chk |= (1 << 8);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_WPA_ENTERPRISE, DA16X_CERT_TYPE_CERT)) {
            chk |= (1 << 7);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_WPA_ENTERPRISE, DA16X_CERT_TYPE_PRIVATE_KEY)) {
            chk |= (1 << 6);
        }

        if (da16x_cert_is_exist_cert(DA16X_CERT_MODULE_WPA_ENTERPRISE, DA16X_CERT_TYPE_DH_PARAMS)) {
            chk |= (1 << 11);
        }

        sprintf(result_str, "%d", chk);
    }
    // AT+NWDCRT
    else if (strcasecmp(argv[0] + 5, "DCRT") == 0) {
        int modules[] = {
            DA16X_CERT_MODULE_MQTT,
            DA16X_CERT_MODULE_HTTPS_CLIENT,
            DA16X_CERT_MODULE_WPA_ENTERPRISE,
            DA16X_CERT_MODULE_NONE
        };

        int types[] = {
            DA16X_CERT_TYPE_CA_CERT,
            DA16X_CERT_TYPE_CERT,
            DA16X_CERT_TYPE_PRIVATE_KEY,
            DA16X_CERT_TYPE_DH_PARAMS,
            DA16X_CERT_TYPE_NONE
        };

        int m_idx = 0;
        int t_idx = 0;

        for (m_idx = 0 ; modules[m_idx] != DA16X_CERT_MODULE_NONE ; m_idx++) {
            for (t_idx = 0 ; types[t_idx] != DA16X_CERT_TYPE_NONE ; t_idx++) {
                da16x_cert_delete(modules[m_idx], types[t_idx]);
            }
        }
    }
#if defined (__SUPPORT_HTTP_SERVER_FOR_ATCMD__)
    // AT+NWHTS
    else if (strcasecmp(argv[0] + 5, "HTS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWHTS=? */
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            goto atcmd_network_end;
        } else if (argc == 2) {
            /* AT+NWHTS=<http_server> */
            char *_cmd[4];

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            _cmd[1] = "-i";
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);

            _cmd[2] = (result_int == SYSMODE_STA_ONLY) ? "wlan0" : "wlan1";
            _cmd[3] = tmp_int1 == 1 ? "start" : "stop";

            status = run_user_http_server(_cmd, 4);
            if (status != ERR_OK) {
                err_code = AT_CMD_ERR_NW_HTS_TASK_CREATE_FAIL;
                goto atcmd_network_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWHTSS
    else if (strcasecmp(argv[0] + 5, "HTSS") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWHTSS=? */
        } else if (argc == 2) {
            /* AT+NWHTSS=<https_server> */
            char *_cmd[4];

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_network_end;
            } else if (is_in_valid_range(tmp_int1, 0, 1) == FALSE) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto atcmd_network_end;
            }

            _cmd[1] = "-i";
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);

            _cmd[2] = result_int == SYSMODE_STA_ONLY ? "wlan0" : "wlan1";
            _cmd[3] = tmp_int1 == 1 ? "start" : "stop";

            status = run_user_https_server(_cmd, 4);
            if (status != ERR_OK) {
                err_code = AT_CMD_ERR_NW_HTSS_TASK_CREATE_FAIL;
                goto atcmd_network_end;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // (__SUPPORT_HTTP_SERVER_FOR_ATCMD__)
#if defined (__SUPPORT_HTTP_CLIENT_FOR_ATCMD__)
    // AT+NWHTC
    else if ((strcasecmp(argv[0] + 5, "HTC") == 0) || (strcasecmp(argv[0] + 5, "HTCH") == 0)) {
        /* AT+NWHTC=<url>,<option>(,<msg>) (set only) */
        char *_cmd[15] = {0, };
        char option[8] = {0, };
        int arg_cnt = 0;

        _cmd[arg_cnt] = "";
        arg_cnt++;

        if (strncasecmp(argv[0] + 5, "HTCH", 4) == 0) {
            _cmd[arg_cnt] = "-info";
            arg_cnt++;
        }

        _cmd[arg_cnt] = "-i";
        arg_cnt++;
        da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);

        if (   result_int == SYSMODE_STA_ONLY
            || result_int == SYSMODE_STA_N_AP
#if defined ( __SUPPORT_P2P__ )
            || result_int == SYSMODE_STA_N_P2P
#endif // __SUPPORT_P2P__
           ) {
            _cmd[arg_cnt] = "wlan0";
        } else {
            _cmd[arg_cnt] = "wlan1";
        }

        arg_cnt++;

        if (strcmp(argv[1], "stop") == 0) {
            _cmd[arg_cnt] = "-stop";
            arg_cnt++;
            status = run_user_http_client(arg_cnt, _cmd);
            if (status != ERR_OK) {
                err_code = AT_CMD_ERR_NW_HTC_TASK_CREATE_FAIL;
                goto atcmd_network_end;
            }
        } else {
            _cmd[arg_cnt] = argv[1]; //URL
            arg_cnt++;

            if (   strcmp(argv[2], "get") == 0
                || strcmp(argv[2], "head") == 0
                || strcmp(argv[2], "delete") == 0
                || strcmp(argv[2], "connect") == 0
                || strcmp(argv[2], "trace") == 0
                || strcmp(argv[2], "options") == 0
                || strcmp(argv[2], "post") == 0
                || strcmp(argv[2], "put") == 0
                || strcmp(argv[2], "patch") == 0
                || strcmp(argv[2], "message") == 0) {

                sprintf(option, "-%s", argv[2]);
                _cmd[arg_cnt] = option;
                arg_cnt++;

                if (  strcmp(argv[2], "post") == 0
                    || strcmp(argv[2], "put") == 0
                    || strcmp(argv[2], "patch") == 0
                    || strcmp(argv[2], "message") == 0) {
                    if (argc == 4) {
                        _cmd[arg_cnt] = argv[3];
                        arg_cnt++;
                        status = run_user_http_client(arg_cnt, _cmd);
                        if (status != ERR_OK) {
                            err_code = AT_CMD_ERR_NW_HTC_TASK_CREATE_FAIL;
                            goto atcmd_network_end;
                        }
                    } else {
                        if (argc < 4) {
                            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                        } else if (argc > 4) {
                            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                        }
                    }
                } else {
                    status = run_user_http_client(arg_cnt, _cmd);
                    if (status != ERR_OK) {
                        err_code = AT_CMD_ERR_NW_HTC_TASK_CREATE_FAIL;
                        goto atcmd_network_end;
                    }
                }
            } else if (   strncmp(argv[2], "HEAD ", 5) == 0
                       || strncmp(argv[2], "GET ", 4) == 0
                       || strncmp(argv[2], "PUT ", 4) == 0
                       || strncmp(argv[2], "POST ", 5) == 0
                       || strncmp(argv[2], "PATCH ", 6) == 0
                       || strncmp(argv[2], "DELETE ", 7) == 0
                       || strncmp(argv[2], "CONNECT ", 8) == 0
                       || strncmp(argv[2], "TRACE ", 6) == 0
                       || strncmp(argv[2], "OPTIONS ", 8) == 0) {

                if (argc == 3) {
                    _cmd[arg_cnt] = "-message";
                    arg_cnt++;
                    _cmd[arg_cnt] = argv[2];
                    arg_cnt++;

                    status = run_user_http_client(arg_cnt, _cmd);
                    if (status != ERR_OK) {
                        err_code = AT_CMD_ERR_NW_HTC_TASK_CREATE_FAIL;
                        goto atcmd_network_end;
                    }
                } else {
                    if (argc < 3) {
                        err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                    } else if (argc > 3) {
                        err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                    }
                }
            } else {
                if (argc < 2) {
                    err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                } else {
                    err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                }
            }
        }
    }
#endif // (__SUPPORT_HTTP_CLIENT_FOR_ATCMD__)
    //AT+NWHTCTLSAUTH
    else if (strcasecmp(argv[0] + 5, "HTCTLSAUTH") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWHTCAUTH=? */
            char *tmp_str;

            tmp_str = read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_AUTH);
            if (tmp_str == NULL) {
                err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;
                goto atcmd_network_end;
            }
            sprintf(result_str, "%s", tmp_str);

        } else if (argc == 2) {
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
            } else {
                if (http_client_set_tls_auth_mode(tmp_int1)) {
                    err_code = AT_CMD_ERR_NVRAM_WRITE;
                }
            }
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    //AT+NWHTCALPN
    else if (strcasecmp(argv[0] + 5, "HTCALPN") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            /* AT+NWHTCALPN=? */
            if (read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &result_int)) {
                err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;
                goto atcmd_network_end;
            } else {
                char *result_str_pos;
                char nvrName[15] = {0, };
                char *tmp_str;

                if (result_int > 1) {
                    char *res_str;
                    int alloc_bytes = 2; // "[alpn_count],"

                    alloc_bytes = alloc_bytes +
                                  (result_int-1) + // num (,)
                                  (2*result_int) + // num (double quotation)
                                  (HTTPC_MAX_ALPN_LEN * result_int); // num(alpn)

                    res_str = ATCMD_MALLOC(alloc_bytes+1);
                    if (res_str == NULL) {
                         err_code = AT_CMD_ERR_MEM_ALLOC;
                        goto atcmd_network_end;
                    }

                    dyn_mem = res_str; // set for freeing later

                    memset(res_str, 0, alloc_bytes+1);
                    result_str_pos = res_str;
                } else {
                    result_str_pos = result_str;
                }

                sprintf(result_str_pos, "%d", result_int);
                result_str_pos = result_str_pos + 1;

                for (int i = 0; i < result_int; i++) {
                    sprintf(nvrName, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, i);
                    tmp_str = read_nvram_string(nvrName);
                    sprintf(result_str_pos, ",\"%s\"", tmp_str);
                    result_str_pos = result_str_pos + (strlen(tmp_str) + 3); // 3 = 2x(") + 1x(,)
                }
            }
        } else if (argc >= 3) {
            /* AT+NWHTCALPN=<#>,<alpn#n>,... */
            int tmp;

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_NW_HTC_ALPN_CNT_TYPE;
                goto atcmd_network_end;
            }

            /* Sanity check */
            if (argc - 2 > tmp_int1) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            } else if (argc - 2 < tmp_int1) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }

            if (tmp_int1 > HTTPC_MAX_ALPN_CNT || tmp_int1 <= 0) {
                err_code = AT_CMD_ERR_NW_HTC_ALPN_CNT_RANGE;
                goto atcmd_network_end;
            }

            for (int i = 0; i < tmp_int1; i++) {
                if (strlen(argv[i + 2]) > HTTPC_MAX_ALPN_LEN) {
                    switch (i) {
                    case 0 : err_code = AT_CMD_ERR_NW_HTC_ALPN1_STR_LEN; break;
                    case 1 : err_code = AT_CMD_ERR_NW_HTC_ALPN2_STR_LEN; break;
                    case 2 : err_code = AT_CMD_ERR_NW_HTC_ALPN3_STR_LEN; break;
                    }
                    goto atcmd_network_end;
                }
            }

            if (!read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &tmp)) {
                for (int i = 0; i < tmp; i++) {
                    char nvr_name[15] = {0, };

                    sprintf(nvr_name, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, i);
                    delete_nvram_env(nvr_name);
                }

                delete_nvram_env(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM);
            }

            for (int i = 0; i < tmp_int1; i++) {
                char nvr_name[15] = {0, };

                sprintf(nvr_name, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, i);
                write_nvram_string(nvr_name, argv[i + 2]);
            }

            write_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, tmp_int1);
        } else {
            err_code = AT_CMD_ERR_UNKNOWN;
        }
    }
    //AT+NWHTCSNI
    else if (strcasecmp(argv[0] + 5, "HTCSNI") == 0) {
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
            char *tmp_str;

            /* AT+NWHTCSNI=? */
            tmp_str = read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI);
            if (tmp_str == NULL) {
                err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;
                goto atcmd_network_end;
            }

            sprintf(result_str, "%s", tmp_str);
        } else if (argc == 2) {
            /* AT+NWHTCSNI=<sni> */
            if (strlen(argv[1]) > HTTPC_MAX_SNI_LEN) {
                err_code = AT_CMD_ERR_NW_HTC_SNI_LEN;
            } else {
                status = write_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI, argv[1]);
                switch (status) {
                case -1 : err_code = AT_CMD_ERR_DPM_SLEEP_STARTED; break;
                case -2 : err_code = AT_CMD_ERR_NVRAM_WRITE; break;
                }
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWHTCALPNDEL
    else if (strcasecmp(argv[0] + 5, "HTCALPNDEL") == 0) {
        int tmp;

        if (!read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &tmp)) {
            delete_nvram_env(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM);

            for (int i = 0; i < tmp; i++) {
                char nvr_name[15] = {0, };

                sprintf(nvr_name, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, i);
                delete_nvram_env(nvr_name);
            }
        }
    }
    /* AT+NWHTCSNIDEL */
    else if (strcasecmp(argv[0] + 5, "HTCSNIDEL") == 0) {
        if (read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI)) {
            delete_nvram_env(HTTPC_NVRAM_CONFIG_TLS_SNI);
        }
    }

#if defined (__SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__)
    // AT+NWWSC
    else if (strcasecmp(argv[0] + 5, "WSC") == 0) {
        int retval = WS_OK;

        /* AT+NWWSC=<operation>,<uri>(<msg>) */
        if (strcmp(argv[1], "connect") == 0) {
            err_code = AT_CMD_ERR_CMD_OK_WO_PRINT;

            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_network_end;
            } else if (sizeof(argv[2]) < 1) {
                err_code = AT_CMD_ERR_NW_WSC_URL_STR_LEN;
                goto atcmd_network_end;
            }

            retval = websocket_client_connect(argv[2]);
            switch (retval) {
            case WS_ERR_CONN_ALREADY_EXIST :
                err_code = AT_CMD_ERR_NW_WSC_TASK_ALREADY_EXIST;
                break;
            case WS_ERR_NO_MEM :
                err_code = AT_CMD_ERR_MEM_ALLOC;
                break;
            case WS_ERR_INVALID_ARG :
                err_code = AT_CMD_ERR_NW_WSC_INVALID_URL;
                break;
            case WS_ERR_CB_FUNC_NOT_EXIST :
                err_code = AT_CMD_ERR_NW_WSC_CB_FUNC_DOES_NO_EXIST;
                break;
            case WS_ERR_NOT_SUPPORTED :
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
                break;
            case WS_ERR_INVALID_STATE :
                err_code = AT_CMD_ERR_NW_WSC_INVALID_STATE;
                break;
            case WS_FAIL :
                err_code = AT_CMD_ERR_NW_WSC_TASK_CREATE_FAIL;
                break;
            }
        } else if (strcmp(argv[1], "disconnect") == 0) {
            if (argc > 2) {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
                goto atcmd_network_end;
            }

            retval = websocket_client_disconnect();
            switch (retval) {
            case WS_ERR_INVALID_STATE :
                err_code = AT_CMD_ERR_NW_WSC_INVALID_STATE;
                break;
            case WS_FAIL :
                err_code = AT_CMD_ERR_NW_WSC_CLOSE_FAIL;
                break;
            }
        } else if (strcmp(argv[1], "send") == 0) {
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                goto atcmd_network_end;
            } else if (sizeof(argv[2]) < 1) {
                err_code = AT_CMD_ERR_NW_WSC_URL_STR_LEN;
                goto atcmd_network_end;
            }

            if (websocket_client_send_msg(argv[2]) != WS_FAIL) {
                err_code = AT_CMD_ERR_CMD_OK;
            } else {
                err_code = AT_CMD_ERR_NW_WSC_SESS_NOT_CONNECTED;
            }
        } else {
            if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_NW_WSC_UNKNOW_CMD;
            }
        }
    }
#endif //(__SUPPORT_WEBSOCKET_CLIENT_FOR_ATCMD__)

#if defined (__SUPPORT_OTA__)
    /* AT+NWOTATLSAUTH=<tls_auth_mode> */
    else if (strcasecmp(argv[0] + 5, "OTATLSAUTH") == 0) {
        if (argc == 2) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                } else {
                    if (ota_update_set_tls_auth_mode_nvram(tmp_int1)) {
                        err_code = AT_CMD_ERR_NW_OTA_SET_TLS_AUTH_MODE_NVRAM;
                    }
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTADWSTART=<fw_type>,<url>,<mcu_fw_name> */
    else if (strcasecmp(argv[0] + 5, "OTADWSTART") == 0) {
        if (argc != 2) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);

            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                // Init
                atcmd_ota_conf->download_notify = NULL;
                atcmd_ota_conf->renew_notify = NULL;
                atcmd_ota_conf->download_sflash_addr = 0;

                if (strcmp(argv[1], "rtos") == 0) {
                    atcmd_ota_conf->update_type = OTA_TYPE_RTOS;
                    memset(atcmd_ota_conf->url, 0x00, OTA_HTTP_URL_LEN);
                    memcpy(atcmd_ota_conf->url, argv[2], OTA_HTTP_URL_LEN);

                } else if (strcmp(argv[1], "cert_key") == 0) {
                    atcmd_ota_conf->update_type = OTA_TYPE_CERT_KEY;
                    memset(atcmd_ota_conf->url, 0x00, OTA_HTTP_URL_LEN);
                    memcpy(atcmd_ota_conf->url, argv[2], OTA_HTTP_URL_LEN);

#if defined (__BLE_COMBO_REF__)
                } else if (strcmp(argv[1], "ble_fw") == 0) {
                    ATCMD_DBG("[%s] OTADWSTART - ble_fw\n", __func__);

                    atcmd_ota_conf->download_sflash_addr = 0;
                    atcmd_ota_conf->update_type = OTA_TYPE_BLE_FW;
                    memset(atcmd_ota_conf->url_ble_fw, 0x00, OTA_HTTP_URL_LEN);
                    memcpy(atcmd_ota_conf->url_ble_fw, argv[2], OTA_HTTP_URL_LEN);
#endif //(__BLE_COMBO_REF__)

#if defined (__OTA_UPDATE_MCU_FW__)
                } else if (  (strcmp(argv[1], "other_fw") == 0)
                        || (strcmp(argv[1], "mcu_fw") == 0)
                        || (strcmp(argv[1], "fw_1") == 0)) {
                    atcmd_ota_conf->update_type = OTA_TYPE_MCU_FW;
                    memset(atcmd_ota_conf->url, 0x00, OTA_HTTP_URL_LEN);
                    memcpy(atcmd_ota_conf->url, argv[2], OTA_HTTP_URL_LEN);
                    //MCU FW name
                    if ((argv[3] != NULL) && argc == 4) {
                        if (ota_update_set_mcu_fw_name(argv[3]) != 0) {
                            err_code = AT_CMD_ERR_NW_OTA_SET_MCU_FW_NAME;
                        }
                    }
#endif // (__OTA_UPDATE_MCU_FW__)

                } else {
                    err_code = AT_CMD_ERR_NW_OTA_WRONG_FW_TYPE;
                }

                if (err_code == AT_CMD_ERR_CMD_OK) {
                    if (ota_update_start_download(atcmd_ota_conf) != 0) {
                        err_code = AT_CMD_ERR_NW_OTA_DOWN_OK_AND_WAIT_RENEW;
                    }
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTADWSTOP */
    else if (strcasecmp(argv[0] + 5, "OTADWSTOP") == 0) {    /* Download STOP */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                ota_update_stop_download();
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTADWPROG=<fw_type> */
    else if (strcasecmp(argv[0] + 5, "OTADWPROG") == 0) {    /* Progress */
        if (argc == 2) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                if (strcmp(argv[1], "rtos") == 0) {
                    ret = ota_update_get_progress(OTA_TYPE_RTOS);
                }
#if defined (__BLE_COMBO_REF__)
                else if (strcmp(argv[1], "ble_fw") == 0) {
                    ATCMD_DBG("[%s] OTADWPROG - ble_fw\n", __func__);
                    ret = ota_update_get_progress(OTA_TYPE_BLE_FW);
                }
#endif //(__BLE_COMBO_REF__)
#if defined (__OTA_UPDATE_MCU_FW__)
                else if (  (strcmp(argv[1], "mcu_fw") == 0)
                        || (strcmp(argv[1], "other_fw") == 0)
                        || (strcmp(argv[1], "fw_1") == 0)) {
                    ret = ota_update_get_download_progress(OTA_TYPE_MCU_FW);
                }
#endif // (__OTA_UPDATE_MCU_FW__)
                else if (strcmp(argv[1], "cert_key") == 0) {
                    ret = ota_update_get_progress(OTA_TYPE_CERT_KEY);
                } else {
                    err_code = AT_CMD_ERR_NW_OTA_WRONG_FW_TYPE;
                }

                if (err_code == AT_CMD_ERR_CMD_OK) {
                    sprintf(result_str, "%d", ret);
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTARENEW */
    else if (strcasecmp(argv[0] + 5, "OTARENEW") == 0) {    /* Renew */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                atcmd_ota_conf->renew_notify = NULL;
                ota_update_start_renew(atcmd_ota_conf);
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTASETADDR=<sflash_addr> */
    else if (strcasecmp(argv[0] + 5, "OTASETADDR") == 0) {
        if (argc == 2) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                char *end = NULL;

                atcmd_ota_conf->download_sflash_addr = strtol(argv[1], &end, 16);
                sprintf(result_str, "0x%02x", ota_update_set_user_sflash_addr(atcmd_ota_conf->download_sflash_addr));
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAGETADDR=<fw_type> */
    else if (strcasecmp(argv[0] + 5, "OTAGETADDR") == 0) {
        if (argc == 2) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                if (strcmp(argv[1], "cert_key") == 0) {
                    sprintf(result_str, "%x", ota_update_get_new_sflash_addr(OTA_TYPE_CERT_KEY));
                }
#if defined (__BLE_COMBO_REF__)
                else if (strcmp(argv[1], "ble_fw") == 0) {
                    ATCMD_DBG("[%s] OTAGETADDR - ble_fw\n", __func__);
                    sprintf(result_str, "%x", ota_update_get_new_sflash_addr(OTA_TYPE_BLE_FW));
                }
#endif //(__BLE_COMBO_REF__)
#if defined (__OTA_UPDATE_MCU_FW__)
                else if (  (strcmp(argv[1], "mcu_fw") == 0)
                        || (strcmp(argv[1], "other_fw") == 0)
                        || (strcmp(argv[1], "fw_1") == 0)) {
                    sprintf(result_str, "%x", ota_update_get_new_sflash_addr(OTA_TYPE_MCU_FW));
                }
#endif // (__OTA_UPDATE_MCU_FW__)
                else {
                    err_code = AT_CMD_ERR_NW_OTA_WRONG_FW_TYPE;
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAREADFLASH=<sflash_addr>,<size> */
    else if (strcasecmp(argv[0] + 5, "OTAREADFLASH") == 0) {
        if (argc == 3) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                UINT i, send_cnt = 0;
                UINT blk_size = 0, last_blk_size = 0;
                char *buffer = NULL;
                UINT addr, size;
                char *end = NULL;

                addr = strtol(argv[1], &end, 16);

                if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_network_end;
                }

                size = tmp_int1;

                buffer = ATCMD_MALLOC(4096);
                if (buffer == NULL) {
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                } else {
                    memset(buffer, 0x00, 4096);

                    send_cnt = size / 4096;
                    if (size >= 4096) {
                        blk_size = 4096;
                        last_blk_size = size % 4096;
                    } else {
                        last_blk_size = size;
                    }

                    for (i = 0; i < send_cnt; i++) {
                        ota_update_read_flash(addr, (VOID*)buffer, blk_size);
                        addr += 4096;
                        PUTS_ATCMD(buffer, blk_size);
                    }

                    if (last_blk_size > 0) {
                        ota_update_read_flash(addr, (VOID*)buffer, last_blk_size);
                        PUTS_ATCMD(buffer, last_blk_size);
                    }
                }

                if (buffer != NULL) {
                    ATCMD_FREE(buffer);
                    buffer = NULL;
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAERASEFLASH=<sflash_addr>,<size> */
    else if (strcasecmp(argv[0] + 5, "OTAERASEFLASH") == 0) {
        if (argc == 3) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                UINT addr, size;
                char *end = NULL;

                addr = strtol(argv[1], &end, 16);

                if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_OTA_FLASH_ERASE_SIZE_TYPE;
                    goto atcmd_network_end;
                }

                size = tmp_int1;

                if (ota_update_erase_flash(addr, size) == size) {
                    sprintf(result_str, "%s", "COMPLETE");
                } else {
                    sprintf(result_str, "%s", "FAIL");
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTACOPYFLASH=<dest_sflash_addr>,<src_sflash_addr>,<size> */
    else if (strcasecmp(argv[0] + 5, "OTACOPYFLASH") == 0) {
        if (argc == 4) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                UINT dest_addr, src_addr, size;
                char *end = NULL;

                dest_addr = strtol(argv[1], &end, 16);
                src_addr = strtol(argv[2], &end, 16);

                if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_OTA_FLASH_COPY_SIZE_TYPE;
                    goto atcmd_network_end;
                }

                size = tmp_int1;

                if (ota_update_copy_flash(dest_addr, src_addr, size)) {
                    sprintf(result_str, "%s", "FAIL");
                } else {
                    sprintf(result_str, "%s", "COMPLETE");
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
#if defined (__OTA_UPDATE_MCU_FW__)
    /* AT+NWOTAFWNAME */
    else if (strcasecmp(argv[0] + 5, "OTAFWNAME") == 0) {    /* Name */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                char name[OTA_MCU_FW_NAME_LEN+1];

                memset(name, 0x00, OTA_MCU_FW_NAME_LEN+1);
                if (ota_update_get_mcu_fw_info(&name[0], NULL, NULL)) {
                    err_code = AT_CMD_ERR_NO_RESULT;
                } else {
                    sprintf(result_str, "%s", name);
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAFWSIZE */
    else if (strcasecmp(argv[0] + 5, "OTAFWSIZE") == 0) {    /* Size */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                if (ota_update_get_mcu_fw_info(NULL, (UINT*)&ret, NULL) != OTA_SUCCESS) {
                    err_code = AT_CMD_ERR_SFLASH_READ;
                } else {
                    sprintf(result_str, "%d", ret);
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAFWCRC */
    else if (strcasecmp(argv[0] + 5, "OTAFWCRC") == 0) {    /* CRC */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                if (ota_update_get_mcu_fw_info(NULL, NULL, (UINT*)&ret)) {
                    err_code = AT_CMD_ERR_SFLASH_READ;
                } else {
                    sprintf(result_str, "%x", ret);
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAREADFW */
    else if (strcasecmp(argv[0] + 5, "OTAREADFW") == 0) {    /* Read */
        if (argc == 3) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                UINT addr, size;
                char *end = NULL;
                addr = strtol(argv[1], &end, 16);

                if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_NW_OTA_FLASH_READ_SIZE_TYPE;
                    goto atcmd_network_end;
                }

                size = tmp_int1;
                if (ota_update_read_mcu_fw(addr, size) != OTA_SUCCESS) {
                    sprintf(result_str, "%s", "FAIL");
                } else {
                    sprintf(result_str, "%s", "COMPLETE");
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTATRANSFW */
    else if (strcasecmp(argv[0] + 5, "OTATRANSFW") == 0) {    /* Transfer */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {
                if (ota_update_trans_mcu_fw()) {
                    sprintf(result_str, "%s", "FAIL");
                } else {
                    sprintf(result_str, "%s", "COMPLETE");
                }
            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTAERASEFW */
    else if (strcasecmp(argv[0] + 5, "OTAERASEFW") == 0) {    /* Erase */
        if (argc == 1) {
            da16x_get_config_int(DA16X_CONF_INT_MODE, &result_int);
            if (   (result_int == SYSMODE_STA_ONLY)
                || (result_int == SYSMODE_AP_ONLY)
                || (result_int == SYSMODE_STA_N_AP)) {

                if (ota_update_erase_mcu_fw()) {
                    sprintf(result_str, "%s", "FAIL");
                } else {
                    sprintf(result_str, "%s", "COMPLETE");
                }

            } else {
                err_code = AT_CMD_ERR_COMMON_SYS_MODE;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
    /* AT+NWOTABYMCU */
    else if (strcasecmp(argv[0] + 5, "OTABYMCU") == 0) {
        if (argc == 3) {
            /* Initialization to receive DA16200 RTOS from MCU */
            if (strcasecmp(argv[1], "rtos") == 0) {
                if (ota_update_by_mcu_init(OTA_TYPE_RTOS, atoi(argv[2]))) {
                    err_code = AT_CMD_ERR_NW_OTA_BY_MCU_INIT;
                    goto atcmd_network_end;
                }
            } else {
                err_code = AT_CMD_ERR_NW_OTA_WRONG_FW_TYPE;
                goto atcmd_network_end;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
#endif // (__OTA_UPDATE_MCU_FW__)
#endif // (__SUPPORT_OTA__)

#if defined (__SUPPORT_ZERO_CONFIG__) // && defined (__SUPPORT_DNS_SD__)
    // AT+NWMDNSSTART
    else if (strcasecmp(argv[0] + 5, "MDNSSTART") == 0) {
        char hostname[32 + 1] = {0x00, };
        int retval = 0;
        int auto_enabled = pdFALSE;

        err_code = AT_CMD_ERR_CMD_OK;

        if (argc <= 2) {
            if (argc == 1 || is_correct_query_arg(argc, argv[1])) {    //AT+NWMDNSSTART=?
                if (zeroconf_is_running_mdns_service() == pdTRUE) {
                    strcpy(result_str, "started");
                } else {
                    strcpy(result_str, "not started");
                }
            } else {
                if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                    err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                    goto atcmd_network_end;
                } else if (is_in_valid_range(tmp_int1, 0, 1) == FALSE) {
                    err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                    goto atcmd_network_end;
                }

                if (zeroconf_is_running_mdns_service() == pdFALSE) {
                    retval = zeroconf_get_mdns_hostname_from_nvram(hostname, sizeof(hostname));
                    if (retval == 0) {
                        retval = zeroconf_start_mdns_service(atoi(argv[1]), hostname, pdFALSE);
                        if (retval) {
                            switch (retval) {
                            case DA_APP_IN_PROGRESS :
                                err_code = AT_CMD_ERR_NW_MDNS_IN_PROCESS;
                                break;
                            case DA_APP_INVALID_INTERFACE :
                                err_code = AT_CMD_ERR_NW_MDNS_START_RUN_MODE_VAL;
                                break;
                            case DA_APP_NOT_ENABLED :
                                err_code = AT_CMD_ERR_NW_NET_IF_IS_DOWN;
                                break;
                            case DA_APP_SOCKET_NOT_CREATE :
                            case DA_APP_SOCKET_OPT_ERROR :
                            case DA_APP_NOT_BOUND :
                                err_code = AT_CMD_ERR_NW_MDNS_SOCKET_FAIL;
                                break;
                            }
                        } else {
                            //set auto-enabled
                            retval = zeroconf_get_mdns_reg_from_nvram(&auto_enabled);
                            if (retval != 0 || auto_enabled == pdFALSE) {
                                retval = zeroconf_set_mdns_reg_to_nvram(pdTRUE);
                            }
                        }
                    } else {
                        err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;
                    }
                } else {
                    err_code = AT_CMD_ERR_COMMON_SYS_MODE;    // Wrong req-mode
                }
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+NWMDNSHNREG
    else if (strcasecmp(argv[0] + 5, "MDNSHNREG") == 0) {
        int retval = 0;

        err_code = AT_CMD_ERR_CMD_OK;

        if (argc == 2) {
            if (zeroconf_is_running_mdns_service() == pdFALSE) {
                retval = zeroconf_set_mdns_hostname_to_nvram(argv[1]);
                if (retval) {
                    err_code = AT_CMD_ERR_NVR_WRITE;
                }
            } else {
                err_code = AT_CMD_ERR_NW_MDNS_IN_PROCESS;
            }
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
#if defined (__SUPPORT_DNS_SD__)
    // AT+NWMDNSSRVREG
    else if (strcasecmp(argv[0] + 5, "MDNSSRVREG") == 0) {
        int retval = 0;
        int auto_enabled = pdFALSE;

        err_code = AT_CMD_ERR_CMD_OK;

        if (zeroconf_is_running_mdns_service() == pdTRUE) {
            if (zeroconf_is_running_dns_sd_service() == pdFALSE) {
                //AT+NWMDNSSRVREG=<Instance name>,<Protocol>,<Port>,[<TxT>]
                if (argc >= 4) {
                    retval = zeroconf_set_dns_sd_srv_name_to_nvram(argv[1]);
                    if (retval) {
                        err_code = AT_CMD_ERR_NW_DNS_SD_SVC_INST_NAME_NVRAM_WR;
                    }

                    retval = zeroconf_set_dns_sd_srv_protocol_to_nvram(argv[2]);
                    if (retval) {
                        err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PROTOCOL_NVRAM_WR;
                    }

                    retval = zeroconf_set_dns_sd_srv_port_to_nvram(atoi(argv[3]));
                    if (retval) {
                        err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PORT_NO_NVRAM_WR;
                    }

                    if (argc >= 5) {
                        int idx = 0;
                        char txt[ZEROCONF_DNS_SD_SRV_TXT_LEN] = "";


                        if (strlen(argv[4]) >= ZEROCONF_DNS_SD_SRV_TXT_LEN) {
                            err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS;
                        } else {
                            strcpy(txt, argv[4]);
                            for (idx = 0 ; idx < (argc - 5) ; idx++) {
                                if (strlen(argv[idx+5]) + strlen(txt) >= ZEROCONF_DNS_SD_SRV_TXT_LEN) {
                                    err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS;
                                    break;
                                }
                                snprintf(txt, (ZEROCONF_DNS_SD_SRV_TXT_LEN - 1), "%s,%s", txt, argv[idx+5]);
                            }

                            if (err_code == AT_CMD_ERR_CMD_OK) {
                                retval = zeroconf_set_dns_sd_srv_txt_to_nvram(txt);
                                if (retval) {
                                    err_code = AT_CMD_ERR_NW_DNS_SD_SVC_TEXT_NVRAM_WR;
                                }
                            }
                        }
                    }

                    if (err_code == AT_CMD_ERR_CMD_OK) {
                        status = zeroconf_register_dns_sd_service_by_nvram();

                        switch (status) {
                        case DA_APP_MALLOC_ERROR :
                            err_code = AT_CMD_ERR_MEM_ALLOC;
                            break;

                        case DA_APP_ENTRY_NOT_FOUND :
                            err_code = AT_CMD_ERR_NVRAM_NOT_SAVED_VALUE;
                            break;

                        case DA_APP_IN_PROGRESS :
                            err_code = AT_CMD_ERR_NW_MDNS_IN_PROCESS;
                            break;

                        case DA_APP_ALREADY_ENABLED :
                            err_code = AT_CMD_ERR_NW_DNS_SD_ALREADY_RUN;
                            break;

                        case DA_APP_INVALID_PARAMETERS :
                            err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS;
                            break;

                        case DA_APP_NOT_CREATED :
                            err_code = AT_CMD_ERR_NW_DNS_SD_SVC_CREATE_FAIL;
                            break;

                        case DA_APP_SUCCESS :
                            // set auto-enabled
                            retval = zeroconf_get_dns_sd_srv_reg_from_nvram(&auto_enabled);
                            if (retval != 0 || auto_enabled == pdFALSE) {
                                retval = zeroconf_set_dns_sd_srv_reg_to_nvram(pdTRUE);
                            }
                            break;

                        default :
                            err_code = AT_CMD_ERR_UNKNOWN;
                            break;
                        }
                    }
                } else {
                    err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                }
            } else {
                err_code = AT_CMD_ERR_NW_DNS_SD_IN_PROCESS;
            }
        } else {
            err_code = AT_CMD_ERR_NW_MDNS_NOT_RUNNING;
        }
    }
    // AT+NWMDNSSRVDEREG
    else if (strcasecmp(argv[0] + 5, "MDNSSRVDEREG") == 0) {
        int retval = 0;
        int auto_enabled = pdFALSE;

        err_code = AT_CMD_ERR_CMD_OK;

        if (argc > 1) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        } else {
            err_code = zeroconf_unregister_dns_sd_service();
            if (err_code == DA_APP_SUCCESS) {
                //set auto-enabled
                retval = zeroconf_get_dns_sd_srv_reg_from_nvram(&auto_enabled);
                if (retval != 0 || auto_enabled == pdTRUE) {
                    retval = zeroconf_set_dns_sd_srv_reg_to_nvram(pdFALSE);
                }
            } else {
                err_code = AT_CMD_ERR_NW_DNS_SD_IN_PROCESS;
            }
        }
    }
    // AT+NWMDNSUPDATETXT
    else if (strcasecmp(argv[0] + 5, "MDNSUPDATETXT") == 0) {
        int retval = 0;

        err_code = AT_CMD_ERR_CMD_OK;

        if (argc >= 2) {
            if (zeroconf_is_running_dns_sd_service() == pdFALSE) {
                int idx = 0;
                char txt[ZEROCONF_DNS_SD_SRV_TXT_LEN] = "";

                if (strlen(argv[1]) >= ZEROCONF_DNS_SD_SRV_TXT_LEN) {
                    err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS;
                } else {
                    strcpy(txt, argv[1]);
                    for (idx = 0 ; idx < (argc - 2) ; idx++) {
                        if (strlen(argv[idx+2]) + strlen(txt) >= ZEROCONF_DNS_SD_SRV_TXT_LEN) {
                            err_code = AT_CMD_ERR_NW_DNS_SD_SVC_PARAMS;
                            break;
                        }
                        snprintf(txt, (ZEROCONF_DNS_SD_SRV_TXT_LEN - 1), "%s,%s", txt, argv[idx+2]);
                    }

                    if (err_code == AT_CMD_ERR_CMD_OK) {
                        retval = zeroconf_set_dns_sd_srv_txt_to_nvram(txt);
                        if (retval) {
                            err_code = AT_CMD_ERR_NVR_WRITE;
                        }
                    }
                }
            } else {
                err_code = AT_CMD_ERR_NW_DNS_SD_ALREADY_RUN;
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }
#endif // __SUPPORT_DNS_SD__
    // AT+NWMDNSSTOP
    else if (strcasecmp(argv[0] + 5, "MDNSSTOP") == 0) {
        int retval = 0;
        int auto_enabled = pdFALSE;
        err_code = AT_CMD_ERR_CMD_OK;

        if (argc == 1) {
            status = zeroconf_stop_mdns_service();
            switch (status) {
            case DA_APP_IN_PROGRESS :
                err_code = AT_CMD_ERR_NW_MDNS_IN_PROCESS;
                break;

            case DA_APP_NOT_ENABLED :
            case DA_APP_SUCCESS :
                err_code = AT_CMD_ERR_CMD_OK;

                //set auto-enabled
                retval = zeroconf_get_mdns_reg_from_nvram(&auto_enabled);
                if (retval != 0 || auto_enabled == pdTRUE) {
                    zeroconf_set_mdns_reg_to_nvram(pdFALSE);
                }
                break;

            default :
                err_code = AT_CMD_ERR_NW_MDNS_UNKNOWN_FAULT;
                break;
            }
        } else if (is_correct_query_arg(argc, argv[1])) {
            if (zeroconf_is_running_mdns_service()) {
                strcpy(result_str, "running");
            } else {
                strcpy(result_str, "stopped");
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#endif // __SUPPORT_ZERO_CONFIG__
    else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_network_end:

    if (dyn_mem != NULL) {
        if (   (strlen(dyn_mem) > 0 && strncmp(dyn_mem, "OK", 2) != 0)
            && (err_code == AT_CMD_ERR_CMD_OK)) {
            PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, dyn_mem);
        }

        vTaskDelay(1);
        vPortFree(dyn_mem);
        return err_code;
    }

    if (   (strlen(result_str) > 0 && strncmp(result_str, "OK", 2) != 0)
        && (err_code == AT_CMD_ERR_CMD_OK)) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return err_code;
}

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
static int atcmd_transport_create_atcmd_sess_ctx_mutex()
{
    if (!atcmd_sess_ctx_mutex) {
        atcmd_sess_ctx_mutex = xSemaphoreCreateMutex();
        if (atcmd_sess_ctx_mutex == NULL) {
            ATCMD_DBG("Faild to creatre atcmd_sess_ctx_mutex\n");
            return -1;
        }

        ATCMD_DBG("Created atcmd_sess_ctx_mutex\n");
    }

    return 0;
}

static int atcmd_transport_take_atcmd_sess_ctx_mutex(unsigned int timeout)
{
    int ret = 0;

    if (atcmd_sess_ctx_mutex) {
        ret = xSemaphoreTake(atcmd_sess_ctx_mutex, timeout);
        if (ret != pdTRUE) {
            ATCMD_DBG("Failed to task atcmd_sess_ctx_mutex\n");
            return -1;
        }
    }

    return 0;
}

static int atcmd_transport_give_atcmd_sess_ctx_mutex()
{
    int ret = 0;

    if (atcmd_sess_ctx_mutex) {
        ret = xSemaphoreGive(atcmd_sess_ctx_mutex);
        if (ret != pdTRUE) {
            ATCMD_DBG("Failed to give atcmd_sess_ctx_mutex(%d)\n", ret);
            return -1;
        }
    }

    return 0;
}

int atcmd_transport(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK;
    int cid;
    int ret = 0;
    char result_str[128] = { 0, };
    int tmp_int1 = 0, tmp_int2 = 0;

    // AT+TRTS
    if (strcasecmp(argv[0] + 5, "TS") == 0) {
        /* AT+TRTS=<local_port>[,<max allowed peers>] */
        if (argc == 2 || argc == 3) {
            atcmd_sess_context *ctx = NULL;

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_TCP_SERVER_LOCAL_PORT_TYPE;
                goto atcmd_transport_end;
            }

            if (argc == 3) {
                if (get_int_val_from_str(argv[2], &tmp_int2, POL_1) != 0) {
                    err_code = AT_CMD_ERR_TCP_SERVER_MAX_PEER_TYPE;
                    goto atcmd_transport_end;
                }
            }

            // Create context
            ctx = atcmd_transport_create_context(ATCMD_SESS_TCP_SERVER);
            if (ctx == NULL) {
                ATCMD_DBG("[%s:%d]Failed to create tcp server context\n", __func__, __LINE__);
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto atcmd_transport_end;
            }

            ret = atcmd_network_connect_tcps(ctx, tmp_int1, tmp_int2);
            if (ret == 0) {
                err_code = AT_CMD_ERR_CMD_OK;
                snprintf(result_str, sizeof(result_str), "%d", ctx->cid);
            } else {
                err_code = AT_CMD_ERR_TCP_SERVER_TASK_CREATE;

                //delete context in failure
                if (ctx) {
                    atcmd_transport_delete_context(ctx->cid);
                    ctx = NULL;
                }
            }
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRTC
    else if (strcasecmp(argv[0] + 5, "TC") == 0) {
        /* AT+TRTC=<server_ip>,<server_port>(,<local_port>) */
        if (argc == 4 || argc == 3) {
            atcmd_sess_context *ctx = NULL;

            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_TCP_CLIENT_SVR_PORT_TYPE;
                goto atcmd_transport_end;
            }

            if (argc == 4 && get_int_val_from_str(argv[3], &tmp_int2, POL_1) != 0) {
                err_code = AT_CMD_ERR_TCP_CLIENT_LOCAL_PORT_TYPE;
                goto atcmd_transport_end;
            }

            // create context
            ctx = atcmd_transport_create_context(ATCMD_SESS_TCP_CLIENT);
            if (!ctx) {
                ATCMD_DBG("[%s:%d]Failed to create tcp client context\n", __func__, __LINE__);
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto atcmd_transport_end;
            }

            ret = atcmd_network_connect_tcpc(ctx, argv[1], tmp_int1, (argc == 4 ? tmp_int2 : 0));
            if (ret == 0) {
                err_code = AT_CMD_ERR_CMD_OK;
                snprintf(result_str, sizeof(result_str), "%d", ctx->cid);
            } else {
                err_code = AT_CMD_ERR_TCP_CLIENT_TASK_CREATE;

                //delete context in failure
                if (ctx) {
                    atcmd_transport_delete_context(ctx->cid);
                    ctx = NULL;
                }
            }
        } else {
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRUSE
    else if (strcasecmp(argv[0] + 5, "USE") == 0) {
        /* AT+TRUSE=<local_port> */
        if (argc == 2) {
            atcmd_sess_context *ctx = NULL;

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_UDP_SESS_LOCAL_PORT_TYPE;
                goto atcmd_transport_end;
            }

            // Create context
            ctx = atcmd_transport_create_context(ATCMD_SESS_UDP_SESSION);
            if (!ctx) {
                ATCMD_DBG("[%s:%d]Failed to create udp session context\n", __func__, __LINE__);
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto atcmd_transport_end;
            }

            ret = atcmd_network_connect_udps(ctx, NULL, 0, tmp_int1);
            if (ret == 0) {
                err_code = AT_CMD_ERR_CMD_OK;
                snprintf(result_str, sizeof(result_str), "%d", ctx->cid);
            } else {
                err_code = AT_CMD_ERR_UDP_SESS_LOCAL_PORT_RANGE;

                //delete context in failure
                if (ctx) {
                    atcmd_transport_delete_context(ctx->cid);
                    ctx = NULL;
                }
            }
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRUR (Only CID 2)
    else if (strcasecmp(argv[0] + 5, "UR") == 0) {
        atcmd_sess_context *ctx = NULL;
        atcmd_udps_context *udps_ctx= NULL;

        if (argc == 1 || is_correct_query_arg(argc, argv[1])) { // AT+TRUR=?
            char str_ip[16] = {0x00, };

            ctx = atcmd_transport_find_context(ID_US);
            if (!ctx) {
                ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
                err_code = AT_CMD_ERR_UDP_CID2_SESS_NOT_EXIST;
                goto atcmd_transport_end;
            }

            udps_ctx = ctx->ctx.udps;

            if (   (udps_ctx->conf->peer_addr.sin_family != AF_INET)
                || (udps_ctx->conf->peer_addr.sin_port == 0)
                || (udps_ctx->conf->peer_addr.sin_addr.s_addr == 0)) {
                err_code = AT_CMD_ERR_UDP_CID2_SESS_INFO;
                goto atcmd_transport_end;
            }

            inet_ntoa_r(udps_ctx->conf->peer_addr.sin_addr, str_ip, sizeof(str_ip));
            snprintf(result_str, sizeof(result_str), "%s,%d",
                    str_ip, ntohs(udps_ctx->conf->peer_addr.sin_port));
        } else if (argc == 3) { // AT+TRUR=<remote_ip>,<remote_port>
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_UDP_CID2_REMOTE_PORT_TYPE;
                goto atcmd_transport_end;
            }

            ctx = atcmd_transport_find_context(ID_US);
            if (ctx) {
                ret = atcmd_network_set_udps_peer_info(ID_US, argv[1], tmp_int1);
                if (ret) {
                    err_code = AT_CMD_ERR_UDP_CID2_SESS_ALREADY_EXIST;
                }
            } else {
                //create context
                ctx = atcmd_transport_create_context(ATCMD_SESS_UDP_SESSION);
                if (!ctx) {
                    ATCMD_DBG("[%s:%d]Failed to create udp sesion context\n", __func__, __LINE__);
                    err_code = AT_CMD_ERR_MEM_ALLOC;
                    goto atcmd_transport_end;
                }

                ret = atcmd_network_connect_udps(ctx, argv[1], tmp_int1, 0);
                if (ret >= 0) {
                    err_code = AT_CMD_ERR_CMD_OK;
                    snprintf(result_str, sizeof(result_str), "%d", ret);
                } else {
                    err_code = AT_CMD_ERR_UDP_SESS_TASK_CREATE;

                    // Delete context in failure
                    if (ctx) {
                        atcmd_transport_delete_context(ctx->cid);
                        ctx = NULL;
                    }
                }
            }
        } else {
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRPRT
    else if (strcasecmp(argv[0] + 5, "PRT") == 0) {
        if (argc == 2) { // AT+TRPRT=<cid>
            if (get_int_val_from_str(argv[1], &cid, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto atcmd_transport_end;
            }

            if (atcmd_network_display(cid) != 0) {
                err_code = AT_CMD_ERR_NO_CONNECTED_SESSION_EXIST;
            }
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRPALL
    else if (strcasecmp(argv[0] + 5, "PALL") == 0) {
        if (argc > 1) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        } else {
            if (atcmd_network_display(0xFF) != 0) {
                err_code = AT_CMD_ERR_NO_CONNECTED_SESSION_EXIST;
            }
        }
    }
    // AT+TRTRM
    else if (strcasecmp(argv[0] + 5, "TRM") == 0) {
        if (get_int_val_from_str(argv[1], &cid, POL_1) != 0) {
            err_code = AT_CMD_ERR_TRTRM_CID_TYPE;
            goto atcmd_transport_end;
        }

        if (argc == 2) { // AT+TRTRM=<cid>
            ret = atcmd_network_terminate_session(cid);
            if (ret) {
                switch (ret) {
                case -5 : err_code = AT_CMD_ERR_NO_FOUND_REQ_CID_SESSION; break;
                case -6 : err_code = AT_CMD_ERR_CONTEXT_CID_TYPE; break;
                case -7 : err_code = AT_CMD_ERR_CONTEXT_DELETE; break;
                }
                goto atcmd_transport_end;
            }
        } else if (argc == 4) { // AT+TRTRM=<cid>,<remote_ip>,<remote_port>
            atcmd_sess_context *ctx = NULL;

            ctx = atcmd_transport_find_context(cid);
            if (!ctx) {
                ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
                err_code = AT_CMD_ERR_NO_FOUND_REQ_CID_SESSION;
                goto atcmd_transport_end;
            }

            // Allowed remote_ip and remote_port in TCP server
            if (ctx->type != ATCMD_SESS_TCP_SERVER) {
                err_code = AT_CMD_ERR_CONTEXT_TYPE_IS_NOT_TCP_SVR;
                goto atcmd_transport_end;
            }

            ctx = NULL;

            // Get port number
            if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_TRTRM_REMOTE_PORT_NUM_TYPE;
                goto atcmd_transport_end;
            }

            ret = atcmd_network_disconnect_tcps_cli(cid, argv[2], tmp_int1);
            if (ret) {
                err_code = AT_CMD_ERR_TRTRM_TCP_SVR_REMOTE_SESS_DISCON;
                goto atcmd_transport_end;
            }
        } else {
            if (argc == 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            } else {
                err_code = AT_CMD_ERR_TOO_MANY_ARGS;
            }
        }
    }
    // AT+TRTALL
    else if (strcasecmp(argv[0] + 5, "TALL") == 0) {
        if (argc > 1) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        } else {
            ret = atcmd_network_terminate_session(0xFF);
            if (ret) {
                switch (ret) {
                case -1 : err_code = AT_CMD_ERR_TCP_SERVER_TERMINATE; break;
                case -2 : err_code = AT_CMD_ERR_TCP_CLIENT_TERMINATE; break;
                case -3 : err_code = AT_CMD_ERR_UDP_SESSION_TERMINATE; break;
                case -4 : err_code = AT_CMD_ERR_MULTI_SESSION_CID_TERMINATE; break;
                }
            }
        }
    }
    // AT+TRSAVE
    else if (strcasecmp(argv[0] + 5, "SAVE") == 0) {
        if (argc > 1) {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        } else {
            if ((ret = atcmd_network_save_context_nvram()) != 0) {
                switch (ret) {
                case -1 : err_code = AT_CMD_ERR_NO_SESSION_TO_SAVE_NVRAM; break;
                case -2 : err_code = AT_CMD_ERR_CONTEXT_INVALID_SESS_TYPE; break;
                }
            }
        }
    } else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_transport_end:

    if ((strlen(result_str) > 0 && strncmp(result_str, "OK", 2) != 0) && err_code == AT_CMD_ERR_CMD_OK)
    {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return err_code;
}

static int atcmd_transport_set_max_session(const unsigned int cnt)
{
    if (dpm_mode_is_enabled() && (cnt > ATCMD_NW_TR_MAX_SESSION_CNT_DPM)) {
        ATCMD_DBG("[%s:%d]Invalid parameter(%d)\n", __func__, __LINE__, cnt);
        return -1;
    } else if (!dpm_mode_is_enabled() && (cnt > ATCMD_NW_TR_MAX_SESSION_CNT)) {
        ATCMD_DBG("[%s:%d]Invalid parameter(%d)\n", __func__, __LINE__, cnt);
        return -1;
    }

    atcmd_sess_max_session = cnt;

    atcmd_sess_max_cid = (atcmd_sess_max_session +  1);

    ATCMD_DBG("[%s:%d]Set max session(%d), max CID(%d)\n", __func__, __LINE__,
            atcmd_sess_max_session, atcmd_sess_max_cid);

    return 0;
}

static unsigned int atcmd_transport_get_max_session(void)
{
    return atcmd_sess_max_session;
}

static unsigned int atcmd_transport_get_max_cid(void)
{
    return atcmd_sess_max_cid;
}

static atcmd_sess_context *atcmd_transport_alloc_context(const atcmd_sess_type type)
{
    atcmd_sess_context *new_ctx = NULL;
    size_t ctx_size = 0;

    if (type == ATCMD_SESS_TCP_SERVER) {
        ctx_size = sizeof(atcmd_tcps_context);
    } else if (type == ATCMD_SESS_TCP_CLIENT) {
        ctx_size = sizeof(atcmd_tcpc_context);
    } else if (type == ATCMD_SESS_UDP_SESSION) {
        ctx_size = sizeof(atcmd_udps_context);
    } else {
        ATCMD_DBG("Invalid type(%d)\n", __func__, __LINE__, type);
        return NULL;
    }

    ATCMD_DBG("[%s:%d]size info(atcmd_sess_context:%ld, type:%ld, ctx_size:%ld)\n",
            __func__, __LINE__, sizeof(atcmd_sess_context), type, ctx_size);

    new_ctx = ATCMD_MALLOC(sizeof(atcmd_sess_context));
    if (!new_ctx) {
        ATCMD_DBG("Failed to create atcmd_sess_context(%ld)\n", sizeof(atcmd_sess_context));
        return NULL;
    }

    memset(new_ctx, 0x00, sizeof(atcmd_sess_context));

    if (ctx_size) {
        new_ctx->ctx.ptr = ATCMD_MALLOC(ctx_size);
        if (!new_ctx->ctx.ptr) {
            ATCMD_DBG("[%s:%d]Failed to allocate context(%d:%ld)\n", __func__, __LINE__,
                    type, ctx_size);
            goto err;
        }

        memset(new_ctx->ctx.ptr, 0x00, ctx_size);
    }

    return new_ctx;

err:

    atcmd_transport_free_context(&new_ctx);

    return NULL;
}

static void atcmd_transport_free_context(atcmd_sess_context **ctx)
{
    atcmd_sess_context *ctx_ptr = *ctx;

    *ctx = NULL;

    if (ctx_ptr) {
        if (ctx_ptr->ctx.ptr) {
            ATCMD_FREE(ctx_ptr->ctx.ptr);
            ctx_ptr->ctx.ptr = NULL;
        }

        ATCMD_FREE(ctx_ptr);
        ctx_ptr = NULL;
    }

    return ;
}

static atcmd_sess_context *atcmd_transport_create_context(const atcmd_sess_type type)
{
    const int init_cid = -1;
    const int start_cid = 3; // ID_US + 1

    unsigned int ctx_cnt = 0;
    atcmd_sess_context *new_ctx = NULL;
    atcmd_sess_context *cur_ctx = NULL;
    atcmd_sess_context *prev_ctx = NULL;

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]Before CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d(%d), ", cur_ctx->cid, cur_ctx->type);
    }
    ATCMD_DBG("\n");

    ATCMD_DBG("[%s:%d]sizeof(atcmd_sess_context):%ld\n", __func__, __LINE__,
            sizeof(atcmd_sess_context));
#endif // (DEBUG_ATCMD)

    atcmd_transport_create_atcmd_sess_ctx_mutex();

    atcmd_transport_take_atcmd_sess_ctx_mutex(atcmd_sess_ctx_mutex_timeout);

    // Check number of context
    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ctx_cnt++;

        if (ctx_cnt >= atcmd_transport_get_max_session()) {
            ATCMD_DBG("[%s:%d]Over max number of context(Max:%d,%d)\n", __func__, __LINE__,
                    atcmd_transport_get_max_session(), ctx_cnt);
            goto err;
        }
    }

    //allocate atcmd_sess_context
    new_ctx = atcmd_transport_alloc_context(type);
    if (!new_ctx) {
        ATCMD_DBG("[%s:%d]Failed to create atcmd_sess_context\n", __func__, __LINE__);
        return NULL;
    }

    //init cid
    new_ctx->cid = init_cid;

    //assign cid - reserved cid for compatibilty
    switch (type) {
    case ATCMD_SESS_TCP_SERVER:
        if (!atcmd_transport_find_context(ID_TS)) {
            new_ctx->cid = ID_TS;
        }
        break;

    case ATCMD_SESS_TCP_CLIENT:
        if (!atcmd_transport_find_context(ID_TC)) {
            new_ctx->cid = ID_TC;
        }
        break;

    case ATCMD_SESS_UDP_SESSION:
        if (!atcmd_transport_find_context(ID_US)) {
            new_ctx->cid = ID_US;
        }
        break;

    case ATCMD_SESS_NONE:
    default:
        ATCMD_DBG("[%s:%d]Invalid type(%d)\n", __func__, __LINE__, type);
        goto err;
    }

    //assign type
    new_ctx->type = type;

    //add linked-list
    if (atcmd_sess_ctx_header == NULL) {
        atcmd_sess_ctx_header = new_ctx;
    } else if (new_ctx->cid == ID_TS) {
        new_ctx->next = atcmd_sess_ctx_header;
        atcmd_sess_ctx_header = new_ctx;
    } else if (new_ctx->cid == ID_TC) {
        prev_ctx = atcmd_sess_ctx_header;
        cur_ctx = atcmd_sess_ctx_header->next;

        if (prev_ctx->cid == ID_TS) {
            prev_ctx->next = new_ctx;
            new_ctx->next = cur_ctx;
        } else {
            atcmd_sess_ctx_header = new_ctx;
            new_ctx->next = prev_ctx;
        }
    } else if (new_ctx->cid == ID_US) {
        prev_ctx = atcmd_sess_ctx_header;
        cur_ctx = atcmd_sess_ctx_header->next;

        if (prev_ctx->cid == ID_TS && cur_ctx != NULL && cur_ctx->cid == ID_TC) {
            new_ctx->next = cur_ctx->next;
            cur_ctx->next = new_ctx;
        } else if (prev_ctx->cid == ID_TS || prev_ctx->cid == ID_TC) {
            new_ctx->next = prev_ctx->next;
            prev_ctx->next = new_ctx;
        } else {
            atcmd_sess_ctx_header = new_ctx;
            new_ctx->next = prev_ctx;
        }
    } else {
        prev_ctx = atcmd_sess_ctx_header;
        cur_ctx = atcmd_sess_ctx_header->next;

        while ((cur_ctx != NULL) && (cur_ctx->cid == ID_TS || cur_ctx->cid == ID_TC || cur_ctx->cid == ID_US)) {
            prev_ctx = cur_ctx;
            cur_ctx = cur_ctx->next;
        }

        if (cur_ctx == NULL || cur_ctx->cid != start_cid) {
            new_ctx->cid = start_cid;
            new_ctx->next = prev_ctx->next;
            prev_ctx->next = new_ctx;

        } else {
            while (cur_ctx) {
                if ((cur_ctx->cid != start_cid) && (prev_ctx->cid + 1 != cur_ctx->cid)) {
                    new_ctx->cid = prev_ctx->cid + 1;
                    new_ctx->next = prev_ctx->next;
                    prev_ctx->next = new_ctx;

                    break;
                }

                prev_ctx = cur_ctx;
                cur_ctx = cur_ctx->next;
            }

            if (new_ctx->cid == init_cid) {
                new_ctx->cid = prev_ctx->cid + 1;
                new_ctx->next = prev_ctx->next;
                prev_ctx->next = new_ctx;
            }
        }
    }

    atcmd_transport_give_atcmd_sess_ctx_mutex();

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]After CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d(%d), ", cur_ctx->cid, cur_ctx->type);
    }
    ATCMD_DBG("\n");
#endif // (DEBUG_ATCMD)

    return new_ctx;

err:

    atcmd_transport_free_context(&new_ctx);

    atcmd_transport_give_atcmd_sess_ctx_mutex();

    return NULL;
}

static atcmd_sess_context *atcmd_transport_find_context(int cid)
{
    atcmd_sess_context *cur_ctx = NULL;

    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        if (cur_ctx->cid == cid) {
            ATCMD_DBG("[%s:%d]Found cid(%d) - type(%d)\n", __func__, __LINE__,
                    cid, cur_ctx->type);
            return cur_ctx;
        }
    }

    return NULL;
}

static int atcmd_transport_delete_context(int cid)
{
    int ret = 0;
    atcmd_sess_context *cur_ctx = NULL;
    atcmd_sess_context *prev_ctx = NULL;

    //For debug
#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]Before CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d(%d), ", cur_ctx->cid, cur_ctx->type);
    }
    ATCMD_DBG("\n");
#endif // (DEBUG_ATCMD)

    atcmd_transport_take_atcmd_sess_ctx_mutex(atcmd_sess_ctx_mutex_timeout);

    if (atcmd_sess_ctx_header == NULL) {
        ATCMD_DBG("[%s:%d]There is no sess context\n", __func__, __LINE__);
        ret = -1;
    } else if (atcmd_sess_ctx_header->cid == cid) {
        cur_ctx = atcmd_sess_ctx_header;
        atcmd_sess_ctx_header = atcmd_sess_ctx_header->next;

        //Release memory
        atcmd_transport_free_context(&cur_ctx);
    } else {
        prev_ctx = atcmd_sess_ctx_header;
        cur_ctx = atcmd_sess_ctx_header->next;

        while (cur_ctx) {
            if (cur_ctx->cid == cid) {
                prev_ctx->next = cur_ctx->next;

                //Release memory
                atcmd_transport_free_context(&cur_ctx);
                break;
            }

            prev_ctx = cur_ctx;
            cur_ctx = cur_ctx->next;
        }
    }

    if (atcmd_sess_ctx_header == NULL) {
        atcmd_network_delete_sess_info();
    }

    atcmd_transport_give_atcmd_sess_ctx_mutex();

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]After CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_sess_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d(%d), ", cur_ctx->cid, cur_ctx->type);
    }
    ATCMD_DBG("\n");
#endif // (DEBUG_ATCMD)

    return ret;
}

static int atcmd_network_get_nvr_id(int cid, atcmd_sess_nvr_type type)
{
    const DA16X_USER_CONF_INT nvr_cid[ATCMD_NW_TR_MAX_NVR_CNT] = {
        DA16X_CONF_INT_ATCMD_NW_TR_CID_0,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_1,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_2,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_3,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_4,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_5,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_6,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_7,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_8,
        DA16X_CONF_INT_ATCMD_NW_TR_CID_9
    };

    const DA16X_USER_CONF_INT nvr_lport[ATCMD_NW_TR_MAX_NVR_CNT] = {
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_0,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_1,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_2,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_3,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_4,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_5,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_6,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_7,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_8,
        DA16X_CONF_INT_ATCMD_NW_TR_LOCAL_PORT_9
    };

    const DA16X_USER_CONF_INT nvr_pport[ATCMD_NW_TR_MAX_NVR_CNT] = {
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_0,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_1,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_2,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_3,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_4,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_5,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_6,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_7,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_8,
        DA16X_CONF_INT_ATCMD_NW_TR_PEER_PORT_9
    };

    const DA16X_USER_CONF_INT nvr_max_allowed_peer[ATCMD_NW_TR_MAX_NVR_CNT] = {
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_0,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_1,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_2,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_3,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_4,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_5,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_6,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_7,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_8,
        DA16X_CONF_INT_ATCMD_NW_TR_MAX_ALLOWED_PEER_9
    };

    const DA16X_USER_CONF_STR nvr_pipaddr[ATCMD_NW_TR_MAX_NVR_CNT] = {
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_0,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_1,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_2,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_3,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_4,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_5,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_6,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_7,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_8,
        DA16X_CONF_STR_ATCMD_NW_TR_PEER_IPADDR_9
    };

    if ((cid > (int)atcmd_transport_get_max_cid()) || (cid > ATCMD_NW_TR_MAX_NVR_CNT)) {
        ATCMD_DBG("[%s:%d]Invalid cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    if (type == ATCMD_SESS_NVR_CID) {
        return (int)nvr_cid[cid];
    } else if (type == ATCMD_SESS_NVR_LOCAL_PORT) {
        return (int)nvr_lport[cid];
    } else if (type == ATCMD_SESS_NVR_PEER_PORT) {
        return (int)nvr_pport[cid];
    } else if (type == ATCMD_SESS_NVR_PEER_IP_ADDRESS) {
        return (int)nvr_pipaddr[cid];
    } else if (type == ATCMD_SESS_NVR_MAX_ALLOWED_PEER) {
        return (int)nvr_max_allowed_peer[cid];
    }

    return -1;
}

static void atcmd_network_display_sess_info(int cid, atcmd_sess_info *sess_info)
{
#if defined (DEBUG_ATCMD)
    if (sess_info) {
        if (sess_info->type == ATCMD_SESS_TCP_SERVER) {
            ATCMD_DBG("[%s:%d]Session info\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n",
                    __func__, __LINE__,
                    "CID", cid,
                    "Type", sess_info->type,
                    "Local port", sess_info->sess.tcps.local_port,
                    "Max allowed client", sess_info->sess.tcps.max_allow_client);
        } else if (sess_info->type == ATCMD_SESS_TCP_CLIENT) {
            ATCMD_DBG("[%s:%d]Session info\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n"
                    "* %-20s : %s:%d\n",
                    __func__, __LINE__,
                    "CID", cid,
                    "Type", sess_info->type,
                    "Local port", sess_info->sess.tcpc.local_port,
                    "Peer IP Address", sess_info->sess.tcpc.peer_ipaddr, sess_info->sess.tcpc.peer_port);
        } else if (sess_info->type == ATCMD_SESS_UDP_SESSION) {
            ATCMD_DBG("[%s:%d]Session info\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n"
                    "* %-20s : %d\n",
                    "CID", cid,
                    __func__, __LINE__,
                    "Type", sess_info->type,
                    "Local port", sess_info->sess.udps.local_port);
        }
    }
#else
    DA16X_UNUSED_ARG(cid);
    DA16X_UNUSED_ARG(sess_info);
#endif // DEBUG_ATCMD
}

//is_rtm: 0 is to save data to RTM. 1 is to save data to NVRAM.
static int atcmd_network_save_tcps_context(atcmd_sess_context *ctx, int is_rtm)
{
    int ret = 0;
    int nvr_id = -1;
    const int nvr_cache = 1;

    atcmd_sess_info src_sess_info = {0x00,};
    atcmd_sess_info *dst_sess_info = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (ctx == NULL || ctx->type != ATCMD_SESS_TCP_SERVER) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    src_sess_info.type = ctx->type;
    memcpy(&src_sess_info.sess.tcps, ctx->conf.tcps.sess_info, sizeof(atcmd_tcps_sess_info));

    atcmd_network_display_sess_info(ctx->cid, &src_sess_info);

    if (is_rtm) {
        //save session info to rtm
        dst_sess_info = atcmd_network_get_sess_info(ctx->cid);
        if (!dst_sess_info) {
            ATCMD_DBG("[%s:%d]Failed to get memory to save session info\n", __func__, __LINE__);
            return -1;
        }

        memcpy(dst_sess_info, &src_sess_info, sizeof(atcmd_sess_info));
        ret = 0;
    } else {
        //save session info to nvram
        //Write local port
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_LOCAL_PORT);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, src_sess_info.sess.tcps.local_port, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set local port of tcp server(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

        //Write max allowed peer
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_MAX_ALLOWED_PEER);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, src_sess_info.sess.tcps.max_allow_client, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set max allowed client of tcp server(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of max allowed peer(%d/%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

end_save_nvr:

        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_CID);
        if (nvr_id != -1) {
            if (ret != -1) {
                ret = user_set_int(nvr_id, (int)ctx->type, nvr_cache);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to write cid(%d:%d)\n", __func__, __LINE__,
                            ctx->cid, ret);
                    ret = -1;
                }
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of cid(%d)\n", __func__, __LINE__,
                    ctx->cid);
            ret = -1;
        }
    }

    return ret;
}

//is_rtm: 0 is to save data to RTM. 1 is to save data to NVRAM.
static int atcmd_network_save_tcpc_context(atcmd_sess_context *ctx, int is_rtm)
{
    int ret = 0;
    int nvr_id = -1;
    const int nvr_cache = 1;

    atcmd_sess_info src_sess_info = {0x00,};
    atcmd_sess_info *dst_sess_info = NULL;

    if (ctx == NULL || ctx->type != ATCMD_SESS_TCP_CLIENT) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    src_sess_info.type = ctx->type;
    memcpy(&src_sess_info.sess.tcpc, ctx->conf.tcpc.sess_info, sizeof(atcmd_tcpc_sess_info));

    atcmd_network_display_sess_info(ctx->cid, &src_sess_info);

    if (is_rtm) {
        //save session info to rtm
        dst_sess_info = atcmd_network_get_sess_info(ctx->cid);
        if (!dst_sess_info) {
            ATCMD_DBG("[%s:%d]Failed to get memory to save session info\n", __func__, __LINE__);
            return -1;
        }

        memcpy(dst_sess_info, &src_sess_info, sizeof(atcmd_sess_info));
        ret = 0;
    } else {
        //save session info to nvram
        //Write local port
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_LOCAL_PORT);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, src_sess_info.sess.tcpc.local_port, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set local port of tcp client(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

        //Write peer port
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_PEER_PORT);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, src_sess_info.sess.tcpc.peer_port, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set peer port of tcp client(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

        //Write peer IP address
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_PEER_IP_ADDRESS);
        if (nvr_id != -1) {
            ret = user_set_str(nvr_id, src_sess_info.sess.tcpc.peer_ipaddr, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set peer ip address of tcp client(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of peer ip address(%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

end_save_nvr:

        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_CID);
        if (nvr_id != -1) {
            if (ret != -1) {
                ret = user_set_int(nvr_id, (int)ctx->type, nvr_cache);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to write cid(%d:%d)\n", __func__, __LINE__,
                            ctx->cid, ret);
                    ret = -1;
                }
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of cid(%d)\n", __func__, __LINE__,
                    ctx->cid);
            ret = -1;
        }
    }

    return ret;
}

//is_rtm: 0 is to save data to RTM. 1 is to save data to NVRAM.
static int atcmd_network_save_udps_context(atcmd_sess_context *ctx, int is_rtm)
{
    int ret = 0;
    int nvr_id = -1;
    const int nvr_cache = 1;

    atcmd_sess_info src_sess_info = {0x00,};
    atcmd_sess_info *dst_sess_info = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (ctx == NULL || ctx->type != ATCMD_SESS_UDP_SESSION) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    src_sess_info.type = ctx->type;
    memcpy(&src_sess_info.sess.udps, ctx->conf.udps.sess_info, sizeof(atcmd_udps_sess_info));

    atcmd_network_display_sess_info(ctx->cid, &src_sess_info);

    if (is_rtm) {
        //save session info to rtm
        dst_sess_info = atcmd_network_get_sess_info(ctx->cid);
        if (!dst_sess_info) {
            ATCMD_DBG("[%s:%d]Failed to get memory to save session info\n", __func__, __LINE__);
            return -1;
        }

        memcpy(dst_sess_info, &src_sess_info, sizeof(atcmd_sess_info));
        ret = 0;
    } else {
        //save session info to nvram
        //Write local port
        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_LOCAL_PORT);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, src_sess_info.sess.udps.local_port, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to set local port of udp session(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                ret = -1;
                goto end_save_nvr;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                    __func__, __LINE__, ctx->cid);
            ret = -1;
            goto end_save_nvr;
        }

end_save_nvr:

        nvr_id = atcmd_network_get_nvr_id(ctx->cid, ATCMD_SESS_NVR_CID);
        if (nvr_id != -1) {
            if (ret != -1) {
                ret = user_set_int(nvr_id, (int)ctx->type, nvr_cache);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to write cid(%d:%d)\n", __func__, __LINE__,
                            ctx->cid, ret);
                    ret = -1;
                }
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of cid(%d)\n", __func__, __LINE__,
                    ctx->cid);
            ret = -1;
        }
    }

    return ret;
}

static int atcmd_network_save_context_nvram(void)
{
    int ret = 0;

    atcmd_sess_context *ctx = NULL;
    int cid = 0;
    int nvr_id = 0;
    const int nvr_cache = 1;
    const unsigned int max_cid = atcmd_transport_get_max_cid();

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (!atcmd_sess_ctx_header) {
        ATCMD_DBG("[%s:%d]there is no session\n", __func__, __LINE__);
        return -1;
    }

    //clear previous configuration
    for (cid = 0 ; cid <= (const int)max_cid ; cid++) {
        nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_CID);
        if (nvr_id != -1) {
            ret = user_set_int(nvr_id, ATCMD_SESS_NONE, nvr_cache);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to delete cid(%d:%d)\n", __func__, __LINE__,
                        cid, ret);
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of cid(%d)\n",
                    __func__, __LINE__, cid);
        }
    }

    for (ctx = atcmd_sess_ctx_header ; ctx != NULL ; ctx = ctx->next) {
        if (ctx->type == ATCMD_SESS_TCP_SERVER) {
            ret = atcmd_network_save_tcps_context(ctx, 0);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to save tcp server's context to nvram\n",
                        __func__, __LINE__, ret);
            }
        } else if (ctx->type == ATCMD_SESS_TCP_CLIENT) {
            ret = atcmd_network_save_tcpc_context(ctx, 0);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to save tcp client's context to nvram\n",
                        __func__, __LINE__, ret);
            }
        } else if (ctx->type == ATCMD_SESS_UDP_SESSION) {
            ret = atcmd_network_save_udps_context(ctx, 0);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to save udp session's context to nvram\n",
                        __func__, __LINE__, ret);
            }
        } else {
            ATCMD_DBG("[%s:%d]Invalid type(%d)\n", __func__, __LINE__, ctx->type);
            ret = -2;
        }
    }

    if (nvr_cache) {
        save_tmp_nvram();
    }

    return 0;
}

static int atcmd_network_recover_context_nvram(atcmd_sess_info *sess_info, const int sess_info_cnt)
{
    int status = 0;
    int ret = 0;
    int cid = 0;

    int nvr_id = 0;
    int type = 0;
    int lport = 0;
    int pport = 0;
    int max_allow_peer = 0;
    char pipaddr[ATCMD_NVR_NW_TR_PEER_IPADDR_LEN] = {0x00, };

    ATCMD_DBG("[%s:%d]Start(sess_info_cnt:%d)\n", __func__, __LINE__, sess_info_cnt);

    if (atcmd_sess_ctx_header) {
        ATCMD_DBG("[%s:%d]there already is session\n", __func__, __LINE__);
        return 0;
    }

    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    //Read CID & Type
    for (cid = 0 ; cid < sess_info_cnt ; cid++) {
        memset(&(sess_info[cid]), 0x00, sizeof(atcmd_sess_info));

        nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_CID);
        if (nvr_id != -1) {
            status = da16x_get_config_int(nvr_id, &type);
            if (   (status == CC_SUCCESS)
                && ((type == ATCMD_SESS_TCP_SERVER) || (type == ATCMD_SESS_TCP_CLIENT) || (type == ATCMD_SESS_UDP_SESSION))) {
                sess_info[cid].type = (atcmd_sess_type)type;
            } else {
                sess_info[cid].type = ATCMD_SESS_NONE;
            }
        } else {
            ATCMD_DBG("[%s:%d]Failed to get nvram id of cid(%d)\n", __func__, __LINE__, cid);
        }
    }

    //Set session_info & linked-list
    for (cid = 0 ; cid < sess_info_cnt ; cid++) {
        ret = 0;

        lport = 0;
        pport = 0;
        max_allow_peer = 0;
        memset(pipaddr, 0x00, sizeof(pipaddr));

        if (sess_info[cid].type == ATCMD_SESS_TCP_SERVER) {
            //Read local port
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_LOCAL_PORT);
            if (nvr_id != -1) {
                status = da16x_get_config_int(nvr_id, &lport);
                if (status || lport == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read local port of tcp server"
                            "(cid:%d,status(%d),lport(%d)\n", __func__, __LINE__,
                            cid, status, lport);
                    ret = -1;
                    goto end_init_tcps;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to read local port of tcp server(%d:%d)\n",
                        __func__, __LINE__, cid, status);
                ret = -1;
                goto end_init_tcps;
            }

            //Read max allowed peer
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_MAX_ALLOWED_PEER);
            if (nvr_id != -1) {
                status = da16x_get_config_int(nvr_id, &max_allow_peer);
                if (status || max_allow_peer == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read max allowed peer of tcp server"
                            "(cid:%d,status(%d),max_allow_peer(%d)\n",
                            __func__, __LINE__, cid, status, max_allow_peer);
                    ret = -1;
                    goto end_init_tcps;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to get nvram id of max allowed peer(%d/%d)\n",
                        __func__, __LINE__, cid);
                ret = -1;
                goto end_init_tcps;
            }

end_init_tcps:

            if (!ret) {
                //construct session_info
                sess_info[cid].sess.tcps.local_port = lport;
                sess_info[cid].sess.tcps.max_allow_client = max_allow_peer;

                atcmd_network_display_sess_info(cid, &sess_info[cid]);
            } else {
                //unset session_info
                sess_info[cid].type = ATCMD_SESS_NONE;
            }
        } else if (sess_info[cid].type == ATCMD_SESS_TCP_CLIENT) {
            //Read local port
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_LOCAL_PORT);
            if (nvr_id != -1) {
                status = da16x_get_config_int(nvr_id, &lport);
                if (status || lport == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read local port of tcp cliet"
                            "(cid:%d,status(%d),lport(%d)\n", __func__, __LINE__,
                            cid, status, lport);
                    ret = -1;
                    goto end_init_tcpc;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                        __func__, __LINE__, cid);
                ret = -1;
                goto end_init_tcpc;
            }

            //Read peer port
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_PEER_PORT);
            if (nvr_id != -1) {
                status = da16x_get_config_int(nvr_id, &pport);
                if (status || pport == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read peer port of tcp client"
                            "(cid:%d,status(%d),pport(%d)\n", __func__, __LINE__,
                            cid, status, pport);
                    ret = -1;
                    goto end_init_tcpc;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to get nvram id of local port(%d)\n",
                        __func__, __LINE__, cid);
                ret = -1;
                goto end_init_tcpc;
            }

            //Read peer IP address
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_PEER_IP_ADDRESS);
            if (nvr_id != -1) {
                status = da16x_get_config_str(nvr_id, pipaddr);
                if (status || strlen(pipaddr) == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read peer ip address of tcp client"
                            "(cid:%d,status(%d),pipaddr(%s)\n", __func__, __LINE__,
                            cid, status, pipaddr);
                    ret = -1;
                    goto end_init_tcpc;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to get nvram id of peer ip address(%d)\n",
                        __func__, __LINE__, cid);
                ret = -1;
                goto end_init_tcpc;
            }

end_init_tcpc:

            if (!ret) {
                //construct session_info
                sess_info[cid].sess.tcpc.local_port = lport;
                sess_info[cid].sess.tcpc.peer_port = pport;
                strcpy(sess_info[cid].sess.tcpc.peer_ipaddr, pipaddr);

                atcmd_network_display_sess_info(cid, &sess_info[cid]);
            } else {
                //unset session_info
                sess_info[cid].type = ATCMD_SESS_NONE;
            }
        } else if (sess_info[cid].type == ATCMD_SESS_UDP_SESSION) {
            //Read local port
            nvr_id = atcmd_network_get_nvr_id(cid, ATCMD_SESS_NVR_LOCAL_PORT);
            if (nvr_id != -1) {
                status = da16x_get_config_int(nvr_id, &lport);
                if (status || lport == 0) {
                    ATCMD_DBG("[%s:%d]Failed to read local port of udp session"
                            "(cid:%d,status(%d),lport(%d)\n", __func__, __LINE__,
                            cid, status, lport);
                    ret = -1;
                    goto end_init_udps;
                }
            } else {
                ATCMD_DBG("[%s:%d]Failed to get nvram id of udp session(%d)\n",
                        __func__, __LINE__, cid);
                ret = -1;
                goto end_init_udps;
            }

end_init_udps:

            if (!ret) {
                //construct session_info
                sess_info[cid].sess.udps.local_port = lport;

                atcmd_network_display_sess_info(cid, &sess_info[cid]);
            } else {
                //unset session_info
                sess_info[cid].type = ATCMD_SESS_NONE;
            }
        } else {
            //for compatibility
            if (cid == ID_TS) {
                status = read_nvram_int(ATC_NVRAM_TCPS_PORT, &lport);
                if (status == 0) {
                    sess_info[cid].type = ATCMD_SESS_TCP_SERVER;
                    sess_info[cid].sess.tcps.local_port = lport;

                    ATCMD_DBG("Read previous NVRAM(under v2.3.4) for TCP server\n");
                    atcmd_network_display_sess_info(cid, &sess_info[cid]);
                }
            } else if (cid == ID_TC) {
                status = read_nvram_int(ATC_NVRAM_TCPC_RPORT, &pport);
                if (status == 0) {
                    char *pipaddr_ptr = read_nvram_string(ATC_NVRAM_TCPC_IP);
                    if (pipaddr_ptr) {
                        sess_info[cid].type = ATCMD_SESS_TCP_CLIENT;
                        sess_info[cid].sess.tcpc.peer_port = pport;
                        strcpy(sess_info[cid].sess.tcpc.peer_ipaddr, pipaddr_ptr);

                        status = read_nvram_int(ATC_NVRAM_TCPC_LPORT, &lport);
                        if (status == 0) {
                            sess_info[cid].sess.tcpc.local_port = lport;
                        }

                        ATCMD_DBG("Read previous NVRAM(under v2.3.4) for TCP client\n");
                        atcmd_network_display_sess_info(cid, &sess_info[cid]);
                    }

                    pipaddr_ptr = NULL;
                }

            } else if (cid == ID_US) {
                status = read_nvram_int(ATC_NVRAM_UDPS_PORT, &lport);
                if (status == 0) {
                    sess_info[cid].type = ATCMD_SESS_UDP_SESSION;
                    sess_info[cid].sess.udps.local_port = lport;

                    ATCMD_DBG("Read previous NVRAM(under v2.3.4) for UDP session\n");
                    atcmd_network_display_sess_info(cid, &sess_info[cid]);
                }
            }
        }
    }

    return 0;
}

static int atcmd_network_get_sess_info_cnt()
{
    return (atcmd_transport_get_max_session() + 2);
}

static void atcmd_network_init_sess_info(atcmd_sess_info *sess_info)
{
    if (sess_info) {
        memset(sess_info, 0x00, sizeof(atcmd_sess_info));
        sess_info->type = ATCMD_SESS_NONE;
    }

    return;
}

static atcmd_sess_info *atcmd_network_get_sess_info(const int cid)
{
    unsigned int ret = 0;
    unsigned int idx = 0;
    atcmd_sess_info *ptr = NULL;

    unsigned int sess_info_cnt = 0;
    size_t len = 0;
    sess_info_cnt = atcmd_network_get_sess_info_cnt();

    len = (sizeof(atcmd_sess_info) * sess_info_cnt);

    if (atcmd_sess_info_header) {
        return &(atcmd_sess_info_header[cid]);
    }

    ATCMD_DBG("[%s:%d]Load sess_info(len:%ld, cnt:%ld)\n", __func__, __LINE__, len, sess_info_cnt);

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(ATCMD_RTM_NW_TR_NAME, (unsigned char **)&ptr);
        if (ret == 0) {
            ret = dpm_user_mem_alloc(ATCMD_RTM_NW_TR_NAME, (void **)&ptr, len, 0);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to allocate memory to save user session(%ld)\n",
                        __func__, __LINE__, len);
                return NULL;
            }

            for (idx = 0 ; idx < sess_info_cnt ; idx++) {
                atcmd_network_init_sess_info(&ptr[idx]);
            }
        } else if (ret != len) {
            ATCMD_DBG("[%s:%d]Invalid size(%d,%d)\n", __func__, ret, len);
            return NULL;
        }
    } else {
        ptr = (atcmd_sess_info *)ATCMD_MALLOC(len);
        if (!ptr) {
            ATCMD_DBG("[%s:%d]Failed to allocate memory to save user session(%ld)\n",
                    __func__, __LINE__, len);
            return NULL;
        }

        for (idx = 0 ; idx < sess_info_cnt ; idx++) {
            atcmd_network_init_sess_info(&ptr[idx]);
        }
    }

    atcmd_sess_info_header = ptr;

    return &(atcmd_sess_info_header[cid]);
}

static int atcmd_network_clear_sess_info(int cid)
{
    atcmd_sess_info *sess_info = NULL;

    sess_info = atcmd_network_get_sess_info(cid);
    if (sess_info) {
        atcmd_network_init_sess_info(sess_info);
        return 0;
    }

    ATCMD_DBG("[%s:%d]Failed to clear sess_info(%d)\n", __func__, __LINE__, cid);

    return -1;
}

static int atcmd_network_delete_sess_info(void)
{
    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (atcmd_sess_info_header) {
        if (dpm_mode_is_enabled()) {
            dpm_user_mem_free(ATCMD_RTM_NW_TR_NAME);
        } else {
            ATCMD_FREE(atcmd_sess_info_header);
        }

        atcmd_sess_info_header = NULL;
    }

    return 0;
}

static int atcmd_network_recover_session(atcmd_sess_info *sess_info, const int sess_info_cnt)
{
    int ret = 0;
    int cid = 0;
    int *failed_cids = NULL;

    atcmd_sess_context *prev_ctx = NULL;
    atcmd_sess_context *new_ctx = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    if (atcmd_sess_ctx_header) {
        ATCMD_DBG("[%s:%d]there is already session\n", __func__, __LINE__);
        return 0;
    }

    failed_cids = ATCMD_MALLOC(sizeof(int) * sess_info_cnt);
    if (!failed_cids) {
        ATCMD_DBG("[%s:%d]Failed to allocate memory for notification(%ld)\n",
                __func__, __LINE__, sizeof(int) * sess_info_cnt);
    } else {
        memset(failed_cids, 0x00, (sizeof(int) * sess_info_cnt));
    }

    //Create session
    for (cid = 0 ; cid < sess_info_cnt ; cid++) {
        atcmd_network_display_sess_info(cid, &sess_info[cid]);

        if ((sess_info[cid].type == ATCMD_SESS_TCP_SERVER)
                || sess_info[cid].type == ATCMD_SESS_TCP_CLIENT
                || sess_info[cid].type == ATCMD_SESS_UDP_SESSION) {

            //create context
            new_ctx = atcmd_transport_alloc_context(sess_info[cid].type);
            if (!new_ctx) {
                ATCMD_DBG("[%s:%d]Failed to allocate atcmd_sess_context\n",
                        __func__, __LINE__);
                continue;
            }

            //cid & type
            new_ctx->cid = cid;
            new_ctx->type = sess_info[cid].type;

            if (sess_info[cid].type == ATCMD_SESS_TCP_SERVER) {
                ret = atcmd_network_connect_tcps(new_ctx,
                                                sess_info[cid].sess.tcps.local_port,
                                                sess_info[cid].sess.tcps.max_allow_client);
            } else if (sess_info[cid].type == ATCMD_SESS_TCP_CLIENT) {
                ret = atcmd_network_connect_tcpc(new_ctx,
                                                sess_info[cid].sess.tcpc.peer_ipaddr,
                                                sess_info[cid].sess.tcpc.peer_port,
                                                sess_info[cid].sess.tcpc.local_port);
            } else if (sess_info[cid].type == ATCMD_SESS_UDP_SESSION) {
                ret = atcmd_network_connect_udps(new_ctx, NULL, 0, sess_info[cid].sess.udps.local_port);
            }

            //construct context to linked-list
            if (ret == 0) {
                if (prev_ctx) {
                    prev_ctx->next = new_ctx;
                    prev_ctx = new_ctx;
                } else {
                    atcmd_sess_ctx_header = prev_ctx = new_ctx;
                }
            } else {
                if (failed_cids) {
                    failed_cids[cid] = 1;
                }

                ATCMD_DBG("[%s:%d]Failed to recover session(cid:%d,ret:%d)\n",
                        __func__, __LINE__, cid, ret);

                atcmd_transport_free_context(&new_ctx);
            }
        }
    }

    //Notification
    if (failed_cids) {
        for (cid = 0 ; cid < sess_info_cnt ; cid++) {
            if (sess_info[cid].type == ATCMD_SESS_TCP_SERVER) {
                if (failed_cids[cid] == 0 && !dpm_mode_is_wakeup()) {
                    PRINTF("[AT-CMD] %d TCP Server OPEN (Port: %d)\r\n",
                            cid, sess_info[cid].sess.tcps.local_port);
                }  else if (failed_cids[cid] == 1) {
                    ATCMD_DBG("[%s] Failed to start %d TCP Server with saved information ...\n",
                            __func__, cid);

#if defined (__ATCMD_IF_UART1__) || defined (__ATCMD_IF_UART2__)
                    while (init_done_sent == pdFALSE) {
                        vTaskDelay(10);
                    }
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

                    PRINTF_ATCMD("\r\n" ATCMD_TCPS_DISCONN_RX_PREFIX ":%d,0,0\r\n", cid);

                }
            } else if (sess_info[cid].type == ATCMD_SESS_TCP_CLIENT) {
                if (failed_cids[cid] == 0 && !dpm_mode_is_wakeup()) {
                    PRINTF("[AT-CMD] %d, TCP Client CONNECTED (IP: %s, Port: %d)\n",
                            cid,
                            sess_info[cid].sess.tcpc.peer_ipaddr,
                            sess_info[cid].sess.tcpc.peer_port);
                }  else if (failed_cids[cid] == 1) {
                    ATCMD_DBG("[%s] Failed to start %d TCP Client with saved information ...\n",
                            __func__, cid);

#if defined (__ATCMD_IF_UART1__) || defined (__ATCMD_IF_UART2__)
                    while (init_done_sent == pdFALSE) {
                        vTaskDelay(10);
                    }
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

                    PRINTF_ATCMD("\r\n" ATCMD_TCPC_DISCONN_RX_PREFIX ":%d,%s,%d\r\n", cid,
                            sess_info[cid].sess.tcpc.peer_ipaddr,
                            sess_info[cid].sess.tcpc.peer_port);
                }
            } else if (sess_info[cid].type == ATCMD_SESS_UDP_SESSION) {
                if (failed_cids[cid] == 0 && !dpm_mode_is_wakeup()) {

                    PRINTF("[AT-CMD] %d UDP session OPEN (Port: %d)\n",
                            cid, sess_info[cid].sess.udps.local_port);
                }  else if (failed_cids[cid] == 1) {
                    ATCMD_DBG("[%s] Failed to start %d UDP Client with saved information ...\n",
                            __func__, cid);

#if defined (__ATCMD_IF_UART1__) || defined (__ATCMD_IF_UART2__)
                    while (init_done_sent == pdFALSE) {
                        vTaskDelay(10);
                    }
#endif // __ATCMD_IF_UART1__ || __ATCMD_IF_UART2__

                    PRINTF_ATCMD("\r\n" ATCMD_UDP_SESS_FAIL_PREFIX ":%d,%d\r\n",
                            cid, sess_info[cid].sess.udps.local_port);
                }
            }
        }

        ATCMD_FREE(failed_cids);
        failed_cids = NULL;
    }

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]After CID: ", __func__, __LINE__);
    for (new_ctx = atcmd_sess_ctx_header ; new_ctx != NULL ; new_ctx = new_ctx->next) {
        ATCMD_DBG("%d(%d), ", new_ctx->cid, new_ctx->type);
    }
    ATCMD_DBG("\n");
#endif // (DEBUG_ATCMD)

    return 0;
}

#define ATCMD_DPM_NETWORK_TMP "ATCMD_DPM_NET_TEMP"
static int atcmd_network_check_network_ready(const char *module, unsigned int timeout)
{
    int ret = pdTRUE;
    const unsigned int wait_option = 100;
    unsigned int start_time, cur_time, elapsed_time, time_remaining = timeout;

    if (dpm_mode_is_enabled() && !dpm_mode_is_wakeup()) {
    	RM_PMGR_W_dpm_job_name_set(ATCMD_DPM_NETWORK_TMP, 0);
        dpm_app_sleep_ready_clear(ATCMD_DPM_NETWORK_TMP);

        while (time_remaining > 0) {
            start_time = xTaskGetTickCount();

            if (!chk_network_ready(WLAN0_IFACE)) {
                vTaskDelay(wait_option);
            } else {
                break;
            }

            cur_time = xTaskGetTickCount();
            if (cur_time >= start_time) {
                elapsed_time = cur_time - start_time;
            } else {
                elapsed_time =  (((unsigned int) 0xFFFFFFFF) - start_time) + cur_time;
            }

            if (time_remaining > elapsed_time) {
                time_remaining -= elapsed_time;
            } else {
                time_remaining = 0;
            }
        }

        if (!chk_network_ready(WLAN0_IFACE)) {
            ret = pdFALSE;
        }

        dpm_app_unregister(ATCMD_DPM_NETWORK_TMP);
    }

    if (!ret) {
        PRINTF("[%s] Timeout to connect an Wi-Fi AP\n", module);
    }

    return ret;
}

static int atcmd_network_connect_tcps(atcmd_sess_context *ctx, int port, int max_peer_cnt)
{
    int ret = 0;

    atcmd_tcps_context *tcps_ctx = NULL;
    atcmd_tcps_config *tcps_conf = NULL;
    atcmd_sess_info *sess_info = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (!ctx) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    tcps_ctx = ctx->ctx.tcps;
    tcps_conf = &ctx->conf.tcps;

    ret = atcmd_tcps_init_context(tcps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init tcps context(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    sess_info = atcmd_network_get_sess_info(ctx->cid);
    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Failed to get session info\n", __func__, __LINE__, ctx->cid);
        goto err;
    }

    ret = atcmd_tcps_init_config(ctx->cid, tcps_conf, &(sess_info->sess.tcps));
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init tcps config(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcps_set_local_addr(tcps_conf, NULL, port);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set local port(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    if (max_peer_cnt) {
        ret = atcmd_tcps_set_max_allowed_client(tcps_conf, max_peer_cnt);
        if (ret) {
            ATCMD_DBG("[%s:%d]Failed to set max peer cnt(%d)\n", __func__, __LINE__, ret);
            goto err;
        }
    }

    ret = atcmd_tcps_set_config(tcps_ctx, tcps_conf);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set config(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcps_start(tcps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to start tcp server task(%d)\n", ret);
        goto err;
    }

    ret = atcmd_tcps_wait_for_ready(tcps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to active tcps(%d,%d)\n", ret, tcps_ctx->state);
        goto err;
    }

    sess_info->type = ctx->type;

    return 0;

err:

    atcmd_network_disconnect_tcps(ctx->cid);

    return -1;
}

static int atcmd_network_disconnect_tcps(const int cid)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_tcps_context *tcps_ctx = NULL;
    atcmd_tcps_config *tcps_conf = NULL;

    int ret = 0;

    ATCMD_DBG("[%s:%d]Start(cid:%d)\n", __func__, __LINE__, cid);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return 0;
    }

    tcps_ctx = ctx->ctx.tcps;
    tcps_conf = &ctx->conf.tcps;

    ret = atcmd_tcps_stop(tcps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to stop tcp server task(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_tcps_deinit_context(tcps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to deinit tcps context(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_tcps_deinit_config(tcps_conf);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to deinit tcps config(%d)\n", __func__, __LINE__, ret);
    }

    atcmd_network_clear_sess_info(cid);

    return 0;
}

static int atcmd_network_disconnect_tcps_cli(const int cid, const char *ip_addr, const int port)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_tcps_context *tcps_ctx = NULL;
    int ret = 0;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    tcps_ctx = ctx->ctx.tcps;

    ret = atcmd_tcps_stop_cli(tcps_ctx, ip_addr, port);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to disconnect TCP client(%d,%s:%d,ret:%d)\n",
                __func__, __LINE__, cid, ip_addr, port, ret);
    }

    return ret;
}

static int atcmd_network_display_tcps(const int cid, const char *prefix)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_tcps_context *tcps_ctx = NULL;
    atcmd_tcps_config *tcps_conf = NULL;
    atcmd_tcps_cli_context *tcps_cli_ctx = NULL;

    const size_t max_sentence_len = 64;
    char *out = NULL;
    size_t outlen = 0;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    tcps_ctx = ctx->ctx.tcps;
    tcps_conf = &ctx->conf.tcps;

    ATCMD_DBG("[%s:%d]cid(%d), state(%d)\n", __func__, __LINE__, cid, tcps_ctx->state);

    if (prefix) {
        outlen += strlen(prefix);
    }

    outlen += (max_sentence_len * (tcps_ctx->cli_cnt > 0 ? tcps_ctx->cli_cnt : 1));

    out = ATCMD_MALLOC(outlen);
    if (!out) {
        ATCMD_DBG("[%s:%d]Failed to allocate memory to display tcp server info(%d)\n",
                __func__, __LINE__, outlen);
        return -1;
    }

    memset(out, 0x00, outlen);

    if (tcps_ctx->state == ATCMD_TCPS_STATE_ACCEPT) {
        if (prefix) {
            strcpy(out, prefix);
        }

        if (tcps_ctx->cli_cnt > 0) {
            for (tcps_cli_ctx = tcps_ctx->cli_ctx ; tcps_cli_ctx != NULL ; tcps_cli_ctx = tcps_cli_ctx->next) {
                if (tcps_cli_ctx->state == ATCMD_TCPS_CLI_STATE_CONNECTED) {
                    snprintf(out, outlen, "%s%d,%s,%ld.%ld.%ld.%ld,%d,%d\r\n",
                            out, cid, "TCP",
                            (ntohl(tcps_cli_ctx->addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(tcps_cli_ctx->addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(tcps_cli_ctx->addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(tcps_cli_ctx->addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(tcps_cli_ctx->addr.sin_port)),
                            (ntohs(tcps_conf->local_addr.sin_port)));
                }
            }
        } else {
            snprintf(out, outlen, "%s%d,%s,%s,%d,%d\r\n", out, cid, "TCP",
                    "0.0.0.0", 0, ntohs(tcps_conf->local_addr.sin_port));
        }
    } else  {
        snprintf(out, outlen, "%s%d,%s,%s,%d,%d\r\n", out, cid, "TCP", "0.0.0.0", 0, 0);
    }

    ATCMD_DBG("[%s:%d]%s(%ld)\n", __func__, __LINE__, out, strlen(out));

    PUTS_ATCMD(out, strlen(out));

    if (out) {
        ATCMD_FREE(out);
        out = NULL;
    }

    return 0;
}

static int atcmd_network_connect_tcpc(atcmd_sess_context *ctx, char *svr_ip, int svr_port, int port)
{
    int ret = 0;

    atcmd_tcpc_context *tcpc_ctx = NULL;
    atcmd_tcpc_config *tcpc_conf = NULL;
    atcmd_sess_info *sess_info = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (!ctx) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    tcpc_ctx = ctx->ctx.tcpc;
    tcpc_conf = &ctx->conf.tcpc;

    //Check network connection
    if (!atcmd_network_check_network_ready("TCP Client", 1000)) {
        return -1;
    }

    ret = atcmd_tcpc_init_context(tcpc_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init tcpc context(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    sess_info = atcmd_network_get_sess_info(ctx->cid);
    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Failed to get session info\n", __func__, __LINE__, ctx->cid);
        goto err;
    }

    ret = atcmd_tcpc_init_config(ctx->cid, tcpc_conf, &(sess_info->sess.tcpc));
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init tcpc config(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcpc_set_local_addr(tcpc_conf, NULL, port);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set local port(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcpc_set_svr_addr(tcpc_conf, svr_ip, svr_port);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set tcp server address(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcpc_set_config(tcpc_ctx, tcpc_conf);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set config(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcpc_start(tcpc_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to start tcp client task(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_tcpc_wait_for_ready(tcpc_ctx);
    if (ret) {
        ATCMD_DBG("Failed to active tcp client(%d,%d)\n", ret, tcpc_ctx->state);
        goto err;
    }

    sess_info->type = ctx->type;

    return 0;

err:

    atcmd_network_disconnect_tcpc(ctx->cid);

    return ret;
}

static int atcmd_network_disconnect_tcpc(const int cid)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_tcpc_context *tcpc_ctx = NULL;
    atcmd_tcpc_config *tcpc_conf = NULL;

    int ret = 0;

    ATCMD_DBG("[%s:%d]Start(cid:%d)\n", __func__, __LINE__, cid);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return 0;
    }

    tcpc_ctx = ctx->ctx.tcpc;
    tcpc_conf = &ctx->conf.tcpc;

    ret = atcmd_tcpc_stop(tcpc_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to stop tcp client task(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_tcpc_deinit_context(tcpc_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to deinit tcpc context(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_tcpc_deinit_config(tcpc_conf);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to deinit tcpc config(%d)\n", __func__, __LINE__, ret);
    }

    atcmd_network_clear_sess_info(cid);

    return ret;
}

static int atcmd_network_display_tcpc(const int cid, const char *prefix)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_tcpc_context *tcpc_ctx = NULL;
    atcmd_tcpc_config *tcpc_conf = NULL;

    struct sockaddr_in local_addr = {0x00, };
    struct sockaddr_in svr_addr = {0x00, };

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    tcpc_ctx = ctx->ctx.tcpc;
    tcpc_conf = &ctx->conf.tcpc;


    if (tcpc_ctx->state == ATCMD_TCPC_STATE_CONNECTED) {
        memcpy(&local_addr, &tcpc_conf->local_addr, sizeof(struct sockaddr_in));
        memcpy(&svr_addr, &tcpc_conf->svr_addr, sizeof(struct sockaddr_in));
    }

    if (prefix) {
        PRINTF_ATCMD("%s%d,%s,%d.%d.%d.%d,%d,%d\r\n", prefix, cid, "TCP",
                (ntohl(svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(svr_addr.sin_port)),
                (ntohs(local_addr.sin_port)));
    } else {
        PRINTF_ATCMD("%d,%s,%d.%d.%d.%d,%d,%d\r\n", cid, "TCP",
                (ntohl(svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(svr_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(svr_addr.sin_port)),
                (ntohs(local_addr.sin_port)));
    }

    return 0;
}

static int atcmd_network_connect_udps(atcmd_sess_context *ctx, char *peer_ip, int peer_port, int port)
{
    int ret = 0;

    atcmd_udps_context *udps_ctx = NULL;
    atcmd_udps_config *udps_conf = NULL;
    atcmd_sess_info *sess_info = NULL;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    if (!ctx) {
        ATCMD_DBG("[%s:%d]Invalid parameter\n", __func__, __LINE__);
        return -1;
    }

    udps_ctx = ctx->ctx.udps;
    udps_conf = &ctx->conf.udps;

    ret = atcmd_udps_init_context(udps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init udps context(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    sess_info = atcmd_network_get_sess_info(ctx->cid);
    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Failed to get session info\n", __func__, __LINE__, ctx->cid);
        goto err;
    }

    ret = atcmd_udps_init_config(ctx->cid, udps_conf, &(sess_info->sess.udps));
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to init udps config(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_udps_set_local_addr(udps_conf, NULL, port);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set local address(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    if (peer_ip && peer_port) {
        ret = atcmd_udps_set_peer_addr(udps_conf, peer_ip, peer_port);
        if (ret) {
            ATCMD_DBG("Failed to set peer address(%d)\n", __func__, __LINE__, ret);
            goto err;
        }
    }

    ret = atcmd_udps_set_config(udps_ctx, udps_conf);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to set config(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_udps_start(udps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to start udps task(%d)\n", __func__, __LINE__, ret);
        goto err;
    }

    ret = atcmd_udps_wait_for_ready(udps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to active udps(%d,%d)\n", ret, udps_ctx->state);
        goto err;
    }

    sess_info->type = ctx->type;

    return 0;

err:

    atcmd_network_disconnect_udps(ctx->cid);

    return ret;
}

static int atcmd_network_disconnect_udps(const int cid)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_udps_context *udps_ctx = NULL;
    atcmd_udps_config *udps_conf = NULL;

    int ret = 0;

    ATCMD_DBG("[%s:%d]Start(cid:%d)\n", __func__, __LINE__, cid);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return 0;
    }

    udps_ctx = ctx->ctx.udps;
    udps_conf = &ctx->conf.udps;

    ret = atcmd_udps_stop(udps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to stop tcp client task(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_udps_deinit_context(udps_ctx);
    if (ret) {
        ATCMD_DBG("[%s:%d]Failed to deinit udps context(%d)\n", __func__, __LINE__, ret);
    }

    ret = atcmd_udps_deinit_config(udps_conf);
    if (ret) {
        ATCMD_DBG("Failed to deinit udps config(%d)\r\n", ret);
    }

    atcmd_network_clear_sess_info(cid);

    return ret;
}

static int atcmd_network_set_udps_peer_info(const int cid, char *ip, unsigned int port)
{
    atcmd_sess_context *ctx = NULL;

    int ret = 0;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    ret = atcmd_udps_set_peer_addr(&ctx->conf.udps, ip, port);
    if (ret) {
        ATCMD_DBG("Failed to set peer information(%d)\n", ret);
    }

    return ret;
}

static int atcmd_network_display_udps(const int cid, const char *prefix)
{
    atcmd_sess_context *ctx = NULL;
    atcmd_udps_context *udps_ctx = NULL;
    atcmd_udps_config *udps_conf = NULL;

    struct sockaddr_in local_addr = {0x00, };
    struct sockaddr_in peer_addr = {0x00, };

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    ctx = atcmd_transport_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
        return -1;
    }

    udps_ctx = ctx->ctx.udps;
    udps_conf = &ctx->conf.udps;

    if (udps_ctx->state == ATCMD_UDPS_STATE_ACTIVE) {
        memcpy(&local_addr, &udps_conf->local_addr, sizeof(struct sockaddr_in));
        memcpy(&peer_addr, &udps_conf->peer_addr, sizeof(struct sockaddr_in));
    }

    if (prefix) {
        PRINTF_ATCMD("%s%d,%s,%d.%d.%d.%d,%d,%d\r\n", prefix, cid, "UDP",
                (ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(peer_addr.sin_port)),
                (ntohs(local_addr.sin_port)));
    } else {
        PRINTF_ATCMD("%d,%s,%d.%d.%d.%d,%d,%d\r\n", cid, "UDP",
                (ntohl(peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(peer_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(peer_addr.sin_port)),
                (ntohs(local_addr.sin_port)));
    }

    return 0;
}

static int atcmd_network_display(const int cid)
{
    int ret = -1;

    atcmd_sess_context *ctx = NULL;

    const char *prefix_all = "\r\n+TRPALL:";
    const char *prefix_module = "\r\n+TRPRT:";

    const char *prefix;

    if (atcmd_sess_ctx_header == NULL) {
        ATCMD_DBG("[%s:%d]Empty session\n", __func__, __LINE__);
        return ret;
    }

    ATCMD_DBG("[%s:%d]Start(cid:%d)\n", __func__, __LINE__, cid);

    if (cid == 0xFF) {
        prefix = prefix_all;

        for (ctx = atcmd_sess_ctx_header ; ctx != NULL ; ctx = ctx->next) {
            switch (ctx->type) {
            case ATCMD_SESS_TCP_SERVER:
                ret = atcmd_network_display_tcps(ctx->cid, prefix);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to display tcp server(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                }
                break;

            case ATCMD_SESS_TCP_CLIENT:
                ret = atcmd_network_display_tcpc(ctx->cid, prefix);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to display tcp client(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                }
                break;

            case ATCMD_SESS_UDP_SESSION:
                ret = atcmd_network_display_udps(ctx->cid, prefix);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to display udp session(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                }
                break;

                case ATCMD_SESS_NONE:
                default:
                    //unknown type
                    break;
            }

            //printed prefix
            if (ret == 0) {
                prefix = NULL;
            }
        }
    } else {
        ctx = atcmd_transport_find_context(cid);
        if (!ctx) {
            ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
            return AT_CMD_ERR_WRONG_ARGUMENTS;
        }

        switch (ctx->type) {
        case ATCMD_SESS_TCP_SERVER:
            ret = atcmd_network_display_tcps(cid, prefix_module);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to display tcp server(%d)\n",
                        __func__, __LINE__, ret);
                return -1;
            }
            break;

        case ATCMD_SESS_TCP_CLIENT:
            ret = atcmd_network_display_tcpc(cid, prefix_module);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to display tcp client(%d)\n",
                        __func__, __LINE__, ret);
                return -1;
            }
            break;

        case ATCMD_SESS_UDP_SESSION:
            ret = atcmd_network_display_udps(cid, prefix_module);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to display udp session(%d)\n",
                        __func__, __LINE__, ret);
                return -1;
            }
            break;

        case ATCMD_SESS_NONE:
        default:
            //unknown type
            break;
        }
    }

    return 0;
}

static int atcmd_network_terminate_session(const int cid)
{
    atcmd_sess_context *ctx = NULL;
    int ret = 0, retval = 0;

    ATCMD_DBG("[%s:%d]cid(0x%x)\n", __func__, __LINE__, cid);

    if (cid == 0xFF) {
        while (atcmd_sess_ctx_header) {
            ctx = atcmd_sess_ctx_header;

            switch (ctx->type) {
            case ATCMD_SESS_TCP_SERVER:
                ret = atcmd_network_disconnect_tcps(ctx->cid);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to disconnect tcp server(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                    retval = -1;
                }
                break;

            case ATCMD_SESS_TCP_CLIENT:
                ret = atcmd_network_disconnect_tcpc(ctx->cid);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to disconnect tcp client(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                    retval = -2;
                }
                break;

            case ATCMD_SESS_UDP_SESSION:
                ret = atcmd_network_disconnect_udps(ctx->cid);
                if (ret) {
                    ATCMD_DBG("[%s:%d]Failed to disconnect udp session(%d:%d)\n",
                            __func__, __LINE__, ctx->cid, ret);
                    retval = -3;
                }
                break;

            case ATCMD_SESS_NONE:
            default:
                //Unknown type
                break;
            }

            ret = atcmd_transport_delete_context(ctx->cid);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to delete context(%d:%d)\n",
                        __func__, __LINE__, ctx->cid, ret);
                retval = -4;
            }

            ctx = NULL;
        }
    } else {
        ctx = atcmd_transport_find_context(cid);
        if (!ctx) {
            ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
            return -5;
        }

        switch (ctx->type) {
        case ATCMD_SESS_TCP_SERVER:
            ret = atcmd_network_disconnect_tcps(cid);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to disconnect tcp server(%d:%d)\n",
                        __func__, __LINE__, cid, ret);
                return ret;
            }
            break;

        case ATCMD_SESS_TCP_CLIENT:
            ret = atcmd_network_disconnect_tcpc(cid);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to disconnect tcp client(%d:%d)\n",
                        __func__, __LINE__, cid, ret);
                return ret;
            }
            break;

        case ATCMD_SESS_UDP_SESSION:
            ret = atcmd_network_disconnect_udps(cid);
            if (ret) {
                ATCMD_DBG("[%s:%d]Failed to disconnect udp session(%d:%d)\n",
                        __func__, __LINE__, cid, ret);
                return ret;
            }
            break;

        case ATCMD_SESS_NONE:
        default:
            //unknown type
            return -6;
        }

        ret = atcmd_transport_delete_context(cid);
        if (ret) {
            ATCMD_DBG("[%s:%d]Failed to delete context(%d:%d)\n",
                    __func__, __LINE__, cid, ret);
            retval = -7;
        }

        ctx = NULL;
    }

    return retval;
}

#else

int atcmd_transport(int argc, char *argv[])
{
    int    status = AT_CMD_ERR_CMD_OK;
    int cid;
    int    ret = 0;
    char result_str[128] = {0, };
    int    tmp_int1 = 0, tmp_int2 = 0;

    if (strcasecmp(argv[0] + 5, "TS") == 0) { // AT+TRTS
        /* AT+TRTS=<local_port> */
        if (argc == 2) {
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (atcmd_tcps_ctx.state != ATCMD_TCPS_STATE_TERMINATED) {
                status = AT_CMD_ERR_UNKNOWN;
                goto atcmd_tranport_end;
            }

            ret = atcmd_network_connect_tcps(tmp_int1);
            if (ret) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "TC") == 0) { // AT+TRTC
        /* AT+TRTC=<server_ip>,<server_port>(,<local_port>) */
        if (argc == 4 || argc == 3) {
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (argc == 4 && get_int_val_from_str(argv[3], &tmp_int2, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (atcmd_tcpc_ctx.state != ATCMD_TCPC_STATE_TERMINATED) {
                status = AT_CMD_ERR_UNKNOWN;
                goto atcmd_tranport_end;
            }

            ret = atcmd_network_connect_tcpc(argv[1], tmp_int1, (argc == 4 ? tmp_int2 : 0));
            if (ret) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "USE") == 0) { // AT+TRUSE
        /* AT+TRUSE=<local_port> */
        if (argc == 2) {
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (atcmd_udps_ctx.state != ATCMD_UDPS_STATE_TERMINATED) {
                status = AT_CMD_ERR_UNKNOWN;
                goto atcmd_tranport_end;
            }

            ret = atcmd_network_connect_udps(NULL, 0, tmp_int1);
            if (ret) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "UR") == 0) { // AT-TRUR
        if (argc == 1 || is_correct_query_arg(argc, argv[1])) { // AT_TRUR=?
            char str_ip[16] = {0x00, };

            if (   (atcmd_udps_ctx.conf->peer_addr.sin_family != AF_INET)
                || (atcmd_udps_ctx.conf->peer_addr.sin_port == 0)
                || (atcmd_udps_ctx.conf->peer_addr.sin_addr.s_addr == 0)) {
                status = AT_CMD_ERR_NO_RESULT;
                goto atcmd_tranport_end;
            }

            inet_ntoa_r(atcmd_udps_ctx.conf->peer_addr.sin_addr, str_ip, sizeof(str_ip));
            snprintf(result_str, sizeof(result_str), "%s,%d", str_ip, ntohs(atcmd_udps_ctx.conf->peer_addr.sin_port));
        } else if (argc == 3) { // AT+TRUR=<remote_ip>,<remote_port>
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            ret = atcmd_network_set_udps_peer_info(argv[1], tmp_int1);
            if (ret) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "PRT") == 0) { // AT+TRPRT
        if (argc == 2) { // AT+TRPRT=<cid>
            if (get_int_val_from_str(argv[1], &cid, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (cid < ID_TS || cid > ID_US) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            if (atcmd_network_display(cid)) {
                status = AT_CMD_ERR_NO_RESULT;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "PALL") == 0) { // AT+TRPALL
        if (atcmd_network_display(0xFF)) {
            status = AT_CMD_ERR_NO_RESULT;
        }
    } else if (strcasecmp(argv[0] + 5, "TRM") == 0) { // AT+TRTRM
        if (argc == 2) { // AT+TRTRM=<cid>
            if (get_int_val_from_str(argv[1], &cid, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            switch (cid) {
            case ID_TS:
                ret = atcmd_network_disconnect_tcps();
                if (ret) {
                    status = AT_CMD_ERR_UNKNOWN;
                }
                break;

            case ID_TC:
                ret = atcmd_network_disconnect_tcpc();
                if (ret) {
                    status = AT_CMD_ERR_UNKNOWN;
                }
                break;

            case ID_US:
                ret = atcmd_network_disconnect_udps();
                if (ret) {
                    status = AT_CMD_ERR_UNKNOWN;
                }
                break;

            default:
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }
        } else if (argc == 4) { // AT+TRTRM=<cid>,<remote_ip>,<remote_port>
            // Get CID
            if (get_int_val_from_str(argv[1], &cid, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            // Allowed remote_ip and remote_port when CID is 0.
            if (cid != ID_TS) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            // Get port number
            if (get_int_val_from_str(argv[3], &tmp_int1, POL_1) != 0) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }

            ret = atcmd_network_disconnect_tcps_cli(argv[2], tmp_int1);
            if (ret) {
                status = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_tranport_end;
            }
        } else {
            status = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else if (strcasecmp(argv[0] + 5, "TALL") == 0) { // AT+TRTALL
        if (atcmd_tcps_ctx.state != ATCMD_TCPS_STATE_TERMINATED) {
            ret = atcmd_network_disconnect_tcps();
        }

        if (atcmd_tcpc_ctx.state != ATCMD_TCPC_STATE_TERMINATED) {
            ret = atcmd_network_disconnect_tcpc();
        }

        if (atcmd_udps_ctx.state != ATCMD_UDPS_STATE_TERMINATED) {
            ret = atcmd_network_disconnect_udps();
        }

        if (ret)
        {
            status = AT_CMD_ERR_UNKNOWN;
        }
    } else if (strcasecmp(argv[0] + 5, "SAVE") == 0) {
        if (atcmd_network_save_cid()) {
            status = AT_CMD_ERR_NO_RESULT;
        }
    } else {
        status = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_tranport_end :
    if ((strlen(result_str) > 0 && strncmp(result_str, "OK", 2) != 0) && status == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return status;
}

static int atcmd_network_display(int cid)
{
    int ret = 0;

    const char *prefix_all = "\r\n+TRPALL:";
    const char *prefix_module = "\r\n+TRPRT:";

    const char *prefix;

    if (cid == 0xFF) {
        prefix = prefix_all;
        int sess_cnt = 0;

        //tcp server
        ret = atcmd_network_display_tcps(prefix);
        if (ret == 0) {
            sess_cnt++;
        }

        prefix = (ret == 0 ? NULL : prefix_all);

        //tcp client
        ret = atcmd_network_display_tcpc(prefix);
        if (ret == 0) {
            sess_cnt++;
        }

        prefix = (ret == 0 ? NULL : prefix_all);

        //udp session
        ret = atcmd_network_display_udps(prefix);
        if (ret == 0) {
            sess_cnt++;
        }

        if (sess_cnt == 0) {
            return -1;
        }
    } else if (cid == ID_TS) {
        ret = atcmd_network_display_tcps(prefix_module);
        if (ret == -1) {
            return -1;
        }
    } else if (cid == ID_TC) {
        ret = atcmd_network_display_tcpc(prefix_module);
        if (ret == -1) {
            return -1;
        }
    } else if (cid == ID_US) {
        ret = atcmd_network_display_udps(prefix_module);
        if (ret == -1) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

static int atcmd_network_save_cid(void)
{
    char str_ip[16] = {0x00, };
    int port = 0;

    //tcp server
    if (atcmd_tcps_ctx.state == ATCMD_TCPS_STATE_ACCEPT) {
        port = ntohs(atcmd_tcps_ctx.conf->local_addr.sin_port);
        write_tmp_nvram_int(ATC_NVRAM_TCPS_PORT, port);
    } else {
        delete_tmp_nvram_env(ATC_NVRAM_TCPS_PORT);
    }

    //tcp client
    if (atcmd_tcpc_ctx.state == ATCMD_TCPC_STATE_CONNECTED) {
        inet_ntoa_r(atcmd_tcpc_ctx.conf->svr_addr.sin_addr, str_ip, sizeof(str_ip));
        port = ntohs(atcmd_tcpc_ctx.conf->svr_addr.sin_port);

        write_tmp_nvram_string(ATC_NVRAM_TCPC_IP, str_ip);
        write_tmp_nvram_int(ATC_NVRAM_TCPC_RPORT, port);

        port = ntohs(atcmd_tcpc_ctx.conf->local_addr.sin_port);
        if (port != 0) {
            write_tmp_nvram_int(ATC_NVRAM_TCPC_LPORT, port);
        }
    } else {
        delete_tmp_nvram_env(ATC_NVRAM_TCPC_IP);
        delete_tmp_nvram_env(ATC_NVRAM_TCPC_RPORT);
        delete_tmp_nvram_env(ATC_NVRAM_TCPC_LPORT);
    }

    //udp session
    if (atcmd_udps_ctx.state == ATCMD_UDPS_STATE_ACTIVE) {
        port = ntohs(atcmd_udps_ctx.conf->local_addr.sin_port);
        write_tmp_nvram_int(ATC_NVRAM_UDPS_PORT, port);
    } else {
        delete_tmp_nvram_env(ATC_NVRAM_UDPS_PORT);
    }

    save_tmp_nvram();

    return 0;
}

#define ATCMD_DPM_NETWORK_TMP "ATCMD_DPM_NET_TEMP"
static int atcmd_network_check_network_ready(const char *module, unsigned int timeout)
{
    int ret = pdTRUE;
    const unsigned int wait_option = 100;
    unsigned int start_time, cur_time, elapsed_time, time_remaining = timeout;

    if (dpm_mode_is_enabled() && !dpm_mode_is_wakeup()) {
    	RM_PMGR_W_dpm_job_name_set(ATCMD_DPM_NETWORK_TMP, 0);
        dpm_app_sleep_ready_clear(ATCMD_DPM_NETWORK_TMP);

        while (time_remaining > 0) {
            start_time = xTaskGetTickCount();

            if (!chk_network_ready(WLAN0_IFACE)) {
                vTaskDelay(wait_option);
            } else {
                break;
            }

            cur_time = xTaskGetTickCount();
            if (cur_time >= start_time) {
                elapsed_time = cur_time - start_time;
            } else {
                elapsed_time =  (((unsigned int) 0xFFFFFFFF) - start_time) + cur_time;
            }

            if (time_remaining > elapsed_time) {
                time_remaining -= elapsed_time;
            } else {
                time_remaining = 0;
            }
        }

        if (!chk_network_ready(WLAN0_IFACE)) {
            ret = pdFALSE;
        }

        dpm_app_unregister(ATCMD_DPM_NETWORK_TMP);
    }

    if (!ret) {
        PRINTF("[%s] Timeout to connect an Wi-Fi AP\n", module);
    }

    return ret;
}

static atcmd_tcps_config *atcmd_network_new_tcps_config(char *name)
{
    atcmd_tcps_config *conf = NULL;
    unsigned int ret = 0;
    size_t conf_len = sizeof(atcmd_tcps_config);

    if (dpm_mode_is_enabled()) {
        conf = atcmd_network_get_tcps_config(name);
        if (conf) {
            return conf;
        }

        ret = dpm_user_mem_alloc(name, (void **)&conf, conf_len, 0);
        if (ret) {
            return NULL;
        }
    } else {
        conf = ATCMD_MALLOC(conf_len);
        if (conf == NULL) {
            return NULL;
        }
    }

    memset(conf, 0x00, conf_len);

    atcmd_tcps_init_config(ID_TS, conf);

    return conf;
}

static atcmd_tcps_config *atcmd_network_get_tcps_config(char *name)
{
    atcmd_tcps_config *conf = NULL;
    unsigned int ret = 0;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&conf);
        if (ret) {
            return conf;
        }
    }

    return NULL;
}

static int atcmd_network_set_tcps_config(atcmd_tcps_config *config, int port)
{
    int ret = 0;

    if (!config) {
        ATCMD_DBG("Invalid configuration for tcp server\r\n");
        return -1;    //invalid parameter
    }

    ret = atcmd_tcps_set_local_addr(config, NULL, port);
    if (ret) {
        ATCMD_DBG("Failed to set local address(%d)\r\n", ret);
        return ret;
    }

    return ret;
}

static int atcmd_network_del_tcps_config(char *name, atcmd_tcps_config **config)
{
    unsigned int ret = 0;
    atcmd_tcps_config *ptr = NULL;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&ptr);
        if (!ret) {
            return -1;    //Not found
        }

        if (ptr != *config) {
            return -2; //invalid parameter
        }

        ret = dpm_user_mem_free(name);
        if (ret) {
            return ret;
        }
    } else {
        ATCMD_FREE(*config);
    }

    *config = NULL;

    return 0;
}

static int atcmd_network_connect_tcps(int port)
{
    const int wait_time = 10;
    const int max_wait_cnt = 10;
    int wait_cnt = 0;

    int ret = 0;

    if (atcmd_tcps_ctx.state != ATCMD_TCPS_STATE_TERMINATED) {
        ret = atcmd_network_disconnect_tcps();
        if (ret) {
            ATCMD_DBG("Failed to disconnect tcp server(%d)\r\n", ret);
            goto err;
        }
    }

    ret = atcmd_tcps_init_context(&atcmd_tcps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to init tcps context(%d)\r\n", ret);
        goto err;
    }

    if (!atcmd_tcps_conf) {
        atcmd_tcps_conf = atcmd_network_new_tcps_config(ATCMD_TCPS_DPM_NAME);
        if (!atcmd_tcps_conf) {
            ATCMD_DBG("Failed to create tcps config(%s)\n", ATCMD_TCPS_DPM_NAME);
            ret = -1;
            goto err;
        }
    }

    ret = atcmd_network_set_tcps_config(atcmd_tcps_conf, port);
    if (ret) {
        ATCMD_DBG("Failed to set tcps config(%d)\r\n", ret);
        goto err;
    }

    ret = atcmd_tcps_set_config(&atcmd_tcps_ctx, atcmd_tcps_conf);
    if (ret) {
        ATCMD_DBG("Failed to set config(%d)\r\n", ret);
        goto err;
    }

    ret = atcmd_tcps_start(&atcmd_tcps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to start tcp server task(%d)\r\n", ret);
        goto err;
    }

    while ((wait_cnt < max_wait_cnt)
            && (atcmd_tcps_ctx.state == ATCMD_TCPS_STATE_READY)) {
        vTaskDelay(wait_time);
        wait_cnt++;
    }

    if (atcmd_tcps_ctx.state != ATCMD_TCPS_STATE_ACCEPT) {
        ATCMD_DBG("Failed to active tcps(%d)\r\n", atcmd_tcps_ctx.state);
        goto err;
    }

    return 0;

err:

    atcmd_network_disconnect_tcps();

    return ret;
}

static int atcmd_network_disconnect_tcps(void)
{
    int ret = 0, tcps_terminated = pdFALSE;

    if (atcmd_tcps_ctx.state != ATCMD_TCPS_STATE_TERMINATED) {
        ret = atcmd_tcps_stop(&atcmd_tcps_ctx);
        if (ret) {
            ATCMD_DBG("Failed to stop tcp server task(%d)\r\n", ret);
        }
    } else {
        tcps_terminated = pdTRUE; // no tcps sess
    }

    ret = atcmd_tcps_deinit_context(&atcmd_tcps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to deinit tcps context(%d)\r\n", ret);
    }

    if (atcmd_tcps_conf) {
        ret += atcmd_tcps_deinit_config(atcmd_tcps_conf);
        if (ret) {
            ATCMD_DBG("Failed to deinit tcps config(%d)\r\n", ret);
        }

        atcmd_network_del_tcps_config(ATCMD_TCPS_DPM_NAME, &atcmd_tcps_conf);
    }

    if (tcps_terminated == pdTRUE)
        return -1;

    return ret;
}

static int atcmd_network_disconnect_tcps_cli(const char *ip_addr, const int port)
{
    int ret = 0;

    ret = atcmd_tcps_stop_cli(&atcmd_tcps_ctx, ip_addr, port);
    if (ret) {
        ATCMD_DBG("Failed to disconnect TCP client(%s:%d,ret:%d)\n", ip_addr, port, ret);
    }

    return ret;
}

static int atcmd_network_display_tcps(const char *resp)
{
    atcmd_tcps_cli_context *cli_ptr = NULL;

    if (atcmd_tcps_ctx.state == ATCMD_TCPS_STATE_ACCEPT) {
        if (resp) {
            PRINTF_ATCMD("%s", resp);
        }

        if (atcmd_tcps_ctx.cli_cnt > 0) {
            for (cli_ptr = atcmd_tcps_ctx.cli_ctx ; cli_ptr != NULL ; cli_ptr = cli_ptr->next) {
                if (cli_ptr->state == ATCMD_TCPS_CLI_STATE_CONNECTED) {
                    PRINTF_ATCMD("%d,%s,%d.%d.%d.%d,%d,%d\r\n", ID_TS, "TCP",
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >> 24) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >> 16) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr) >>  8) & 0xFF,
                            (ntohl(cli_ptr->addr.sin_addr.s_addr)      ) & 0xFF,
                            (ntohs(cli_ptr->addr.sin_port)),
                            (ntohs(atcmd_tcps_ctx.conf->local_addr.sin_port)));
                }
            }
        } else {
            PRINTF_ATCMD("%d,%s,%s,%d,%d\r\n",
                    ID_TS, "TCP", "0.0.0.0", 0, ntohs(atcmd_tcps_ctx.conf->local_addr.sin_port));
        }

        return 0;
    }

    return -1;
}

static atcmd_tcpc_config *atcmd_network_new_tcpc_config(char *name)
{
    atcmd_tcpc_config *conf = NULL;
    unsigned int ret = 0;
    size_t conf_len = sizeof(atcmd_tcpc_config);

    if (dpm_mode_is_enabled()) {
        conf = atcmd_network_get_tcpc_config(name);
        if (conf) {
            return conf;
        }

        ret = dpm_user_mem_alloc(name, (void **)&conf, conf_len, 0);
        if (ret) {
            return NULL;
        }
    } else {
        conf = ATCMD_MALLOC(conf_len);
        if (conf == NULL) {
            return NULL;
        }
    }

    memset(conf, 0x00, conf_len);

    atcmd_tcpc_init_config(ID_TC, conf);

    return conf;
}

static atcmd_tcpc_config *atcmd_network_get_tcpc_config(char *name)
{
    atcmd_tcpc_config *conf = NULL;
    unsigned int ret = 0;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&conf);
        if (ret) {
            return conf;
        }
    }

    return NULL;
}

static int atcmd_network_set_tcpc_config(atcmd_tcpc_config *config,
                                         char *svr_ip, int svr_port, int port)
{
    int ret = 0;

    if (!config) {
        ATCMD_DBG("Invalid configuration for tcp client\r\n");
        return -1;    //invalid parameter
    }

    ret = atcmd_tcpc_set_local_addr(config, NULL, port);
    if (ret) {
        ATCMD_DBG("Failed to set local address(%d)\r\n", ret);
        return ret;
    }

    ret = atcmd_tcpc_set_svr_addr(config, svr_ip, svr_port);
    if (ret) {
        ATCMD_DBG("Failed to set tcp server address(%d)\r\n", ret);
        return ret;
    }

    return ret;
}

static int atcmd_network_del_tcpc_config(char *name, atcmd_tcpc_config **config)
{
    unsigned int ret = 0;
    atcmd_tcpc_config *ptr = NULL;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&ptr);
        if (!ret) {
            return -1;    //Not found
        }

        if (ptr != *config) {
            return -2; //invalid parameter
        }

        ret = dpm_user_mem_free(name);
        if (ret) {
            return ret;
        }
    } else {
        ATCMD_FREE(*config);
    }

    *config = NULL;

    return 0;
}

static int atcmd_network_connect_tcpc(char *svr_ip, int svr_port, int port)
{
    const int wait_time = 100;
    const int network_wait_time = 1000;

    int ret = 0;
    int cnt = 0;

    if (atcmd_tcpc_ctx.state != ATCMD_TCPC_STATE_TERMINATED) {
        ret = atcmd_network_disconnect_tcpc();
        if (ret) {
            ATCMD_DBG("Failed to disconnect tcp client(%d)\r\n", ret);
            goto err;
        }
    }

    if (!atcmd_network_check_network_ready("TCP Client", network_wait_time)) {
        return -1;
    }

    ret = atcmd_tcpc_init_context(&atcmd_tcpc_ctx);
    if (ret) {
        ATCMD_DBG("Failed to init tcpc context(%d)\r\n", ret);
        goto err;
    }

    if (!atcmd_tcpc_conf) {
        atcmd_tcpc_conf = atcmd_network_new_tcpc_config(ATCMD_TCPC_DPM_NAME);
        if (!atcmd_tcpc_conf) {
            ATCMD_DBG("Failed to create tcpc config(%s)\n", ATCMD_TCPC_DPM_NAME);
            ret = -1;
            goto err;
        }
    }

    if (svr_ip && svr_port) {
        ret = atcmd_network_set_tcpc_config(atcmd_tcpc_conf, svr_ip, svr_port, port);
        if (ret) {
            ATCMD_DBG("Failed to set tcpc config(%d)\r\n", ret);
            goto err;
        }
    }

    ret = atcmd_tcpc_set_config(&atcmd_tcpc_ctx, atcmd_tcpc_conf);
    if (ret) {
        ATCMD_DBG("Failed to set config(%d)\r\n", ret);
        goto err;
    }

    ret = atcmd_tcpc_start(&atcmd_tcpc_ctx);
    if (ret) {
        ATCMD_DBG("Failed to start tcp client task(%d)\r\n", ret);
        goto err;
    }

    //Check tcp client connects
    for (cnt = 0 ; cnt < (ATCMD_TCPC_RECONNECT_TIMEOUT + 1) ; cnt++) {
        if (atcmd_tcpc_ctx.state == ATCMD_TCPC_STATE_CONNECTED) {
            break;
        } else if (atcmd_tcpc_ctx.state == ATCMD_TCPC_STATE_TERMINATED) {
            ret = -1;
            goto err;
        }

        vTaskDelay(wait_time);
    }

    return 0;

err:

    atcmd_network_disconnect_tcpc();

    return ret;
}

static int atcmd_network_disconnect_tcpc(void)
{
    int ret = 0, tcpc_terminated = pdFALSE;

    if (atcmd_tcpc_ctx.state != ATCMD_TCPC_STATE_TERMINATED) {
        ret = atcmd_tcpc_stop(&atcmd_tcpc_ctx);
        if (ret) {
            ATCMD_DBG("Failed to stop tcp client task(%d)\r\n", ret);
        }
    } else {
        tcpc_terminated = pdTRUE; // no tcpc sess
    }

    ret = atcmd_tcpc_deinit_context(&atcmd_tcpc_ctx);
    if (ret) {
        ATCMD_DBG("Failed to deinit tcpc context(%d)\r\n", ret);
    }

    if (atcmd_tcpc_conf) {
        ret += atcmd_tcpc_deinit_config(atcmd_tcpc_conf);
        if (ret) {
            ATCMD_DBG("Failed to deinit tcpc config(%d)\r\n", ret);
        }

        atcmd_network_del_tcpc_config(ATCMD_TCPC_DPM_NAME, &atcmd_tcpc_conf);
    }

    if (tcpc_terminated == pdTRUE)
        return -1;

    return ret;
}

static int atcmd_network_display_tcpc(const char *resp)
{
    if (atcmd_tcpc_ctx.state == ATCMD_TCPC_STATE_CONNECTED) {
        if (resp) {
            PRINTF_ATCMD("%s", resp);
        }

        PRINTF_ATCMD("%d,%s,%d.%d.%d.%d,%d,%d\r\n", ID_TC, "TCP",
                (ntohl(atcmd_tcpc_ctx.conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(atcmd_tcpc_ctx.conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(atcmd_tcpc_ctx.conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(atcmd_tcpc_ctx.conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(atcmd_tcpc_ctx.conf->svr_addr.sin_port)),
                (ntohs(atcmd_tcpc_ctx.conf->local_addr.sin_port)));

        return 0;
    }

    return -1;
}

static atcmd_udps_config *atcmd_network_new_udps_config(char *name)
{
    atcmd_udps_config *conf = NULL;
    unsigned int ret = 0;
    size_t conf_len = sizeof(atcmd_udps_config);

    ATCMD_DBG("[%s:%d]Create new config of UDP session(%s)\n", __func__, __LINE__, name);

    if (dpm_mode_is_enabled()) {
        conf = atcmd_network_get_udps_config(name);
        if (conf) {
            return conf;
        }

        ret = dpm_user_mem_alloc(name, (void **)&conf, conf_len, 0);
        if (ret) {
            return NULL;
        }
    } else {
        conf = ATCMD_MALLOC(conf_len);
        if (conf == NULL) {
            return NULL;
        }
    }

    memset(conf, 0x00, conf_len);

    atcmd_udps_init_config(ID_US, conf);

    return conf;
}

static atcmd_udps_config *atcmd_network_get_udps_config(char *name)
{
    atcmd_udps_config *conf = NULL;
    unsigned int ret = 0;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&conf);
        if (ret) {
            return conf;
        }
    }

    return NULL;
}

static int atcmd_network_set_udps_config(atcmd_udps_config *config, char *peer_ip, int peer_port, int port)
{
    int ret = 0;

    if (!config) {
        ATCMD_DBG("Invalid configuration for udp session\r\n");
        return -1;    //invalid parameter
    }

    ret = atcmd_udps_set_local_addr(config, NULL, port);
    if (ret) {
        ATCMD_DBG("Failed to set local address(%d)\r\n", ret);
        return ret;
    }

    if (peer_ip && peer_port) {
        ret = atcmd_udps_set_peer_addr(config, peer_ip, peer_port);
        if (ret) {
            ATCMD_DBG("Failed to set peer address(%d)\r\n", ret);
            return ret;
        }
    }

    return ret;
}

static int atcmd_network_del_udps_config(char *name, atcmd_udps_config **config)
{
    unsigned int ret = 0;
    atcmd_udps_config *ptr = NULL;

    if (dpm_mode_is_enabled()) {
        ret = dpm_user_mem_get(name, (unsigned char **)&ptr);
        if (!ret) {
            return -1;    //Not found
        }

        if (ptr != *config) {
            return -2; //invalid parameter
        }

        ret = dpm_user_mem_free(name);
        if (ret) {
            return ret;
        }
    } else {
        ATCMD_FREE(*config);
    }

    *config = NULL;

    return 0;
}

static int atcmd_network_connect_udps(char *peer_ip, int peer_port, int port)
{
    const int wait_time = 10;
    const int max_wait_cnt = 10;
    int wait_cnt = 0;

    int ret = 0;

    if (atcmd_udps_ctx.state != ATCMD_UDPS_STATE_TERMINATED) {
        ret = atcmd_network_disconnect_udps();
        if (ret) {
            ATCMD_DBG("Failed to disconnec udp session(%d)\r\n", ret);
            goto err;
        }
    }

    ret = atcmd_udps_init_context(&atcmd_udps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to init udps context(%d)\r\n", ret);
        goto err;
    }

    if (!atcmd_udps_conf) {
        atcmd_udps_conf = atcmd_network_new_udps_config(ATCMD_UDPS_DPM_NAME);
        if (!atcmd_udps_conf) {
            ATCMD_DBG("Failed to create udps config(%s)\n", ATCMD_UDPS_DPM_NAME);
            ret = -1;
            goto err;
        }
    }

    ret = atcmd_network_set_udps_config(atcmd_udps_conf, peer_ip, peer_port, port);
    if (ret) {
        ATCMD_DBG("Failed to set udps config(%d)\r\n", ret);
        goto err;
    }

    ret = atcmd_udps_set_config(&atcmd_udps_ctx, atcmd_udps_conf);
    if (ret) {
        ATCMD_DBG("Failed to set config(%d)\r\n", ret);
        goto err;
    }

    ret = atcmd_udps_start(&atcmd_udps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to start udps task(%d)\r\n", ret);
        goto err;
    }

    while ((wait_cnt < max_wait_cnt)
            && (atcmd_udps_ctx.state == ATCMD_UDPS_STATE_READY)) {
        vTaskDelay(wait_time);
        wait_cnt++;
    }

    if (atcmd_udps_ctx.state != ATCMD_UDPS_STATE_ACTIVE) {
        ATCMD_DBG("Failed to active udps(%d)\r\n", atcmd_udps_ctx.state);
        goto err;
    }

    return 0;

err:

    atcmd_network_disconnect_udps();

    return ret;
}

static int atcmd_network_disconnect_udps(void)
{
    int ret = 0, udps_terminated = pdFALSE;

    if (atcmd_udps_ctx.state != ATCMD_UDPS_STATE_TERMINATED) {
        ret = atcmd_udps_stop(&atcmd_udps_ctx);
        if (ret) {
            ATCMD_DBG("Failed to stop tcp client task(%d)\r\n", ret);
        }
    } else {
        udps_terminated = pdTRUE; // no udp sess
    }

    ret = atcmd_udps_deinit_context(&atcmd_udps_ctx);
    if (ret) {
        ATCMD_DBG("Failed to deinit udps context(%d)\r\n", ret);
    }

    if (atcmd_udps_conf) {
        ret += atcmd_udps_deinit_config(atcmd_udps_conf);
        if (ret) {
            ATCMD_DBG("Failed to deinit udps config(%d)\r\n", ret);
        }

        atcmd_network_del_udps_config(ATCMD_UDPS_DPM_NAME, &atcmd_udps_conf);
    }

    if (udps_terminated == pdTRUE)
        return -1;

    return ret;
}

static int atcmd_network_set_udps_peer_info(char *ip, unsigned int port)
{
    int ret = 0;

    if (!atcmd_udps_conf) {
        atcmd_udps_conf = atcmd_network_new_udps_config(ATCMD_UDPS_DPM_NAME);
        if (!atcmd_udps_conf) {
            ATCMD_DBG("Failed to create udps config(%s)\r\n", ATCMD_UDPS_DPM_NAME);
            ret = -1;
            goto err;
        }
    }

    ret = atcmd_udps_set_peer_addr(atcmd_udps_conf, ip, port);
    if (ret) {
        ATCMD_DBG("Failed to set peer information(%d)\r\n", ret);
    }

err:

    return ret;
}

static int atcmd_network_display_udps(const char *resp)
{
    if (atcmd_udps_ctx.state == ATCMD_UDPS_STATE_ACTIVE) {
        if (resp) {
            PRINTF_ATCMD("%s", resp);
        }

        PRINTF_ATCMD("%d,%s,%d.%d.%d.%d,%d,%d\r\n", ID_US, "UDP",
                (ntohl(atcmd_udps_ctx.conf->peer_addr.sin_addr.s_addr) >> 24) & 0xFF,
                (ntohl(atcmd_udps_ctx.conf->peer_addr.sin_addr.s_addr) >> 16) & 0xFF,
                (ntohl(atcmd_udps_ctx.conf->peer_addr.sin_addr.s_addr) >>  8) & 0xFF,
                (ntohl(atcmd_udps_ctx.conf->peer_addr.sin_addr.s_addr)      ) & 0xFF,
                (ntohs(atcmd_udps_ctx.conf->peer_addr.sin_port)),
                (ntohs(atcmd_udps_ctx.conf->local_addr.sin_port)));

        return 0;
    }

    return -1;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

#ifdef __SUPPORT_TCP_RECVDATA_HEX_MODE__
static int atcmd_tcp_data_mode(int argc, char *argv[])
{
    int mode;

    if (argc < 2) {
        return AT_CMD_ERR_INSUFFICENT_ARGS;
    } else if (argc != 2) {
        return AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    mode = atoi(argv[1]);

    if (mode == 0 || mode == 1) {
        set_tcpc_data_mode(mode);
        set_tcps_data_mode(mode);

        return AT_CMD_ERR_CMD_OK;
    } else {
        return AT_CMD_ERR_COMMON_ARG_RANGE;
    }
}
#endif /* __SUPPORT_TCP_RECVDATA_HEX_MODE__ */

#if defined (__SUPPORT_ATCMD_TLS__)
/**************************************************************************/
/* Related with management of TLS Client/Server                           */
/**************************************************************************/

static void atcmd_transport_ssl_recover_context_nvram(void);
static int atcmd_transport_ssl_save_context_nvram(atcmd_tls_context *ctx);
static int atcmd_transport_ssl_clear_all_context_nvram(void);
static int atcmd_transport_ssl_save_tlsc_nvram(atcmd_tls_context *ctx, int profile_idx);
static int atcmd_transport_ssl_recover_tlsc_nvram(atcmd_tls_context *ctx, int profile_idx);

static void atcmd_transport_ssl_recover_context_nvram(void)
{
    int status = DA_APP_SUCCESS;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    DA16X_USER_CONF_INT tls_nvr_cid_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_CID_0,
        DA16X_CONF_INT_ATCMD_TLS_CID_1
    };

    DA16X_USER_CONF_INT tls_nvr_role_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_ROLE_0,
        DA16X_CONF_INT_ATCMD_TLS_ROLE_1
    };

    DA16X_USER_CONF_INT tls_nvr_profile_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_0,
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_1
    };

    atcmd_tls_context *prev_ctx = NULL;
    atcmd_tls_context *new_ctx = NULL;

    int tls_nvr_idx = -1;

    int tls_nvr_cid_value[ATCMD_TLS_MAX_ALLOW_CNT] = {-1,};
    int tls_nvr_role_value[ATCMD_TLS_MAX_ALLOW_CNT] = {-1,};
    int tls_nvr_profile_value[ATCMD_TLS_MAX_ALLOW_CNT] = {-1,};

    int cid = -1;
    int role = -1;
    int profile = -1;

    if (atcmd_tls_ctx_header) {
        //Already exsited
        ATCMD_DBG("[%s:%d]Already existed atcmd_tls_ctx_header\n", __func__, __LINE__);
        return ;
    }

    for (tls_nvr_idx = 0 ; tls_nvr_idx < ATCMD_TLS_MAX_ALLOW_CNT ; tls_nvr_idx++) {
        cid = -1;
        role = ATCMD_TLS_NONE;
        profile = -1;

        tls_nvr_cid_value[tls_nvr_idx] = cid;
        tls_nvr_role_value[tls_nvr_idx] = role;
        tls_nvr_profile_value[tls_nvr_idx] = profile;

        status = da16x_get_config_int(tls_nvr_cid_name[tls_nvr_idx], &cid);
        if (status || cid < 0 || cid > ATCMD_TLS_MAX_ALLOW_CNT) {
            ATCMD_DBG("[%s:%d]Invaild CID(%d,%d,%d)\n", __func__, __LINE__,
                      status, tls_nvr_cid_name[tls_nvr_idx], cid);
            continue;
        }

        status = da16x_get_config_int(tls_nvr_role_name[tls_nvr_idx], &role);
        if (status || (role != ATCMD_TLS_SERVER && role != ATCMD_TLS_CLIENT)) {
            ATCMD_DBG("[%s:%d]Invalid role(%d,%d,%d)\n", __func__, __LINE__,
                      status, tls_nvr_role_name[tls_nvr_idx], role);
            continue;
        }

        status = da16x_get_config_int(tls_nvr_profile_name[tls_nvr_idx], &profile);
        if (status || profile < 0 || profile >= ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE) {
            ATCMD_DBG("[%s:%d]failed to get profile(%d,%d,%d)\n", __func__, __LINE__,
                      status, tls_nvr_profile_name[tls_nvr_idx], profile);
            continue;
        }

        ATCMD_DBG("[%s:%d]#%d. CID: %d, Role: %d, Profile: %d\n", __func__, __LINE__,
                  tls_nvr_idx, cid, role, profile);

        tls_nvr_cid_value[tls_nvr_idx] = cid;
        tls_nvr_role_value[tls_nvr_idx] = role;
        tls_nvr_profile_value[tls_nvr_idx] = profile;
    }

    //sort
    for (int i = 0 ; i < ATCMD_TLS_MAX_ALLOW_CNT - 1 ; i++) {
        for (int j = 0 ; j < ATCMD_TLS_MAX_ALLOW_CNT - i - 1 ; j++) {
            if (tls_nvr_cid_value[j] > tls_nvr_cid_value[j + 1]) {
                ATCMD_DBG("[%s:%d]#%d. Swap cid(%d,%d)\n", __func__, __LINE__,
                          j, tls_nvr_cid_value[j], tls_nvr_cid_value[j + 1]);
                //swap
                int tmp = tls_nvr_cid_value[j];
                tls_nvr_cid_value[j] = tls_nvr_cid_value[j + 1];
                tls_nvr_cid_value[j + 1] = tmp;

                tmp = tls_nvr_role_value[j];
                tls_nvr_role_value[j] = tls_nvr_role_value[j + 1];
                tls_nvr_role_value[j + 1] = tmp;

                tmp = tls_nvr_profile_value[j];
                tls_nvr_profile_value[j] = tls_nvr_profile_value[j + 1];
                tls_nvr_profile_value[j + 1] = tmp;
            }
        }
    }

    for (tls_nvr_idx = 0 ; tls_nvr_idx < ATCMD_TLS_MAX_ALLOW_CNT ; tls_nvr_idx++) {
        ATCMD_DBG("[%s:%d]tls_nvr_cid_value[%d]=%d, tls_nvr_role_value[%d]=%d, "
                  "tls_nvr_profile_value[%d]=%d\n", __func__, __LINE__,
                  tls_nvr_idx, tls_nvr_cid_value[tls_nvr_idx],
                  tls_nvr_idx, tls_nvr_role_value[tls_nvr_idx],
                  tls_nvr_idx, tls_nvr_profile_value[tls_nvr_idx]);

        if (tls_nvr_cid_value[tls_nvr_idx] >= 0) {
            new_ctx = ATCMD_MALLOC(sizeof(atcmd_tls_context));
            if (!new_ctx) {
                ATCMD_DBG("[%s:%d]failed to create atcmd_tls_context(%ld)\n",
                          __func__, __LINE__, sizeof(atcmd_tls_context));
                continue;
            }

            memset(new_ctx, 0x00, sizeof(atcmd_tls_context));

            //copy
            new_ctx->cid = tls_nvr_cid_value[tls_nvr_idx];
            new_ctx->role = (atcmd_tls_role)tls_nvr_role_value[tls_nvr_idx];

            if (new_ctx->role == ATCMD_TLS_CLIENT) {
                status = atcmd_transport_ssl_recover_tlsc_nvram(new_ctx,
                                                                tls_nvr_profile_value[tls_nvr_idx]);
                if (status) {
                    ATCMD_DBG("[%s:%d]failed to recover tls client(0x%x)\n",
                              __func__, __LINE__, status);

                    ATCMD_FREE(new_ctx);
                    continue;
                }
            }

            //link
            if (prev_ctx) {
                prev_ctx->next = new_ctx;
            } else {
                atcmd_tls_ctx_header = new_ctx;
            }

            prev_ctx = new_ctx;
        }
    }

    return ;
}

static int atcmd_transport_ssl_save_context_nvram(atcmd_tls_context *ctx)
{
    int status = DA_APP_SUCCESS;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    DA16X_USER_CONF_INT tls_nvr_cid_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_CID_0,
        DA16X_CONF_INT_ATCMD_TLS_CID_1
    };

    DA16X_USER_CONF_INT tls_nvr_role_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_ROLE_0,
        DA16X_CONF_INT_ATCMD_TLS_ROLE_1
    };

    DA16X_USER_CONF_INT tls_nvr_profile_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_0,
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_1
    };

    int tls_nvr_idx = -1;
    int tlsc_nvr_idx = -1;
    int tls_nvr_cid_value[ATCMD_TLS_MAX_ALLOW_CNT] = { -1, };
    int tlsc_nvr_profile_idx[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = { pdFALSE, };
    int cid = -1;
    int role = -1;
    int profile = -1;

    if (!atcmd_tls_ctx_header) {
        ATCMD_DBG("[%s:%d]empty CID\n", __func__, __LINE__);
        return DA_APP_SUCCESS;
    }

    for (tls_nvr_idx = 0 ; tls_nvr_idx < ATCMD_TLS_MAX_ALLOW_CNT ; tls_nvr_idx++) {
        cid = -1;
        role = ATCMD_TLS_NONE;
        profile = -1;

        tls_nvr_cid_value[tls_nvr_idx] = cid;

        status = da16x_get_config_int(tls_nvr_cid_name[tls_nvr_idx], &cid);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get cid(%d/%d)\n", __func__, __LINE__,
                      status, tls_nvr_cid_name[tls_nvr_idx]);
        }

        status = da16x_get_config_int(tls_nvr_role_name[tls_nvr_idx], &role);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get role(%d/%d)\n", __func__, __LINE__,
                    status, tls_nvr_role_name[tls_nvr_idx]);
        }

        status = da16x_get_config_int(tls_nvr_profile_name[tls_nvr_idx], &profile);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get profile(%d/%d)\n", __func__, __LINE__,
                      status, tls_nvr_profile_name[tls_nvr_idx]);
        }

        tls_nvr_cid_value[tls_nvr_idx] = cid;

        if (cid == ctx->cid && role == ctx->role) {
            //found previous profile
            if (role == ATCMD_TLS_CLIENT) {
                return atcmd_transport_ssl_save_tlsc_nvram(ctx, profile);
            }
        } else if (role == ctx->role) {
            //keep profile
            if (role == ATCMD_TLS_CLIENT) {
                tlsc_nvr_profile_idx[profile] = pdTRUE;
            }
        }
    }

    //new profile
    for (tls_nvr_idx = 0 ; tls_nvr_idx < ATCMD_TLS_MAX_ALLOW_CNT ; tls_nvr_idx++) {
        if (tls_nvr_cid_value[tls_nvr_idx] == -1) {
            if (ctx->role == ATCMD_TLS_CLIENT) {
                for (tlsc_nvr_idx = 0 ; tlsc_nvr_idx < ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE ; tlsc_nvr_idx++) {
                    if (!tlsc_nvr_profile_idx[tlsc_nvr_idx]) {
                        ATCMD_DBG("[%s:%d]Found Empty profile(%d)\n",
                                __func__, __LINE__, tlsc_nvr_idx);
                        break;
                    }
                }

                if (tlsc_nvr_idx < ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE) {
                    status = da16x_set_config_int(tls_nvr_cid_name[tls_nvr_idx], ctx->cid);
                    if (status) {
                        ATCMD_DBG("[%s:%d]failed to set cid(%d/%d)\n", __func__, __LINE__,
                                  status, tls_nvr_cid_name[tls_nvr_idx]);
                    }

                    status = da16x_set_config_int(tls_nvr_role_name[tls_nvr_idx], ctx->role);
                    if (status) {
                        ATCMD_DBG("[%s:%d]failed to set role(%d/%d)\n", __func__, __LINE__,
                                  status, tls_nvr_role_name[tls_nvr_idx]);
                    }

                    status = da16x_set_config_int(tls_nvr_profile_name[tls_nvr_idx], tlsc_nvr_idx);
                    if (status) {
                        ATCMD_DBG("[%s:%d]failed to set profile(%d/%d)\n", __func__, __LINE__,
                                  status, tls_nvr_role_name[tls_nvr_idx]);
                    }

                    return atcmd_transport_ssl_save_tlsc_nvram(ctx, tlsc_nvr_idx);
                }
            }
        }
    }

    return DA_APP_NOT_FOUND;
}

static int atcmd_transport_ssl_clear_all_context_nvram(void)
{
    int status = DA_APP_SUCCESS;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    DA16X_USER_CONF_INT tls_nvr_cid_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_CID_0,
        DA16X_CONF_INT_ATCMD_TLS_CID_1
    };

    DA16X_USER_CONF_INT tls_nvr_role_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_ROLE_0,
        DA16X_CONF_INT_ATCMD_TLS_ROLE_1
    };

    DA16X_USER_CONF_INT tls_nvr_profile_name[ATCMD_TLS_MAX_ALLOW_CNT] = {
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_0,
        DA16X_CONF_INT_ATCMD_TLS_PROFILE_1
    };

    int tls_nvr_idx = 0;

    int cid = -1;
    int role = -1;
    int profile = -1;

    for (tls_nvr_idx = 0 ; tls_nvr_idx < ATCMD_TLS_MAX_ALLOW_CNT ; tls_nvr_idx++) {
        cid = -1;
        role = ATCMD_TLS_NONE;
        profile = -1;

        status = da16x_get_config_int(tls_nvr_cid_name[tls_nvr_idx], &cid);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get cid(%d/%d)\n", __func__, __LINE__,
                      status, tls_nvr_cid_name[tls_nvr_idx]);
        }

        status = da16x_get_config_int(tls_nvr_role_name[tls_nvr_idx], &role);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get role(%d/%d)\n", __func__, __LINE__,
                      status, tls_nvr_role_name[tls_nvr_idx]);
        }

        status = da16x_get_config_int(tls_nvr_profile_name[tls_nvr_idx], &profile);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to get profile(%d/%d)\n", __func__, __LINE__,
                      status, tls_nvr_profile_name[tls_nvr_idx]);
        }

        if (role == ATCMD_TLS_CLIENT && profile >= 0) {
            status = atcmd_transport_ssl_save_tlsc_nvram(NULL, profile);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to clear tls client(0x%x)\n",
                          __func__, __LINE__, status);
            }
        }

        ATCMD_DBG("[%s:%d]#%d. cid(%d), role(%d), profile(%d)\n",
                  __func__, __LINE__, tls_nvr_idx, cid, role, profile);

        //clear cid
        ATCMD_DBG("[%s:%d]#%d. Clear cid\n", __func__, __LINE__, tls_nvr_idx);
        da16x_set_config_int(tls_nvr_cid_name[tls_nvr_idx], -1);

        //clear role
        ATCMD_DBG("[%s:%d]#%d. Clear role\n", __func__, __LINE__, tls_nvr_idx);
        da16x_set_config_int(tls_nvr_role_name[tls_nvr_idx], -1);

        //clear profile
        ATCMD_DBG("[%s:%d]#%d. Clear profile\n", __func__, __LINE__, tls_nvr_idx);
        da16x_set_config_int(tls_nvr_profile_name[tls_nvr_idx], -1);
    }

    return status;
}

static int atcmd_transport_ssl_save_tlsc_nvram(atcmd_tls_context *ctx, int profile_idx)
{
    int status = DA_APP_SUCCESS;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    DA16X_USER_CONF_STR tlsc_nvr_ca_cert_name[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_CA_CERT_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_CA_CERT_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_cert_name[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_CERT_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_CERT_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_hostname[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_HOST_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_HOST_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_peer_ipaddr[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_PEER_IPADDR_0,
        DA16X_CONF_STR_ATCMD_TLSC_PEER_IPADDR_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_auth_mode[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_AUTH_MODE_0,
        DA16X_CONF_INT_ATCMD_TLSC_AUTH_MODE_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_incoming_len[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_INCOMING_LEN_0,
        DA16X_CONF_INT_ATCMD_TLSC_INCOMING_LEN_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_outgoing_len[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_OUTGOING_LEN_0,
        DA16X_CONF_INT_ATCMD_TLSC_OUTGOING_LEN_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_local_port[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_LOCAL_PORT_0,
        DA16X_CONF_INT_ATCMD_TLSC_LOCAL_PORT_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_peer_port[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_PEER_PORT_0,
        DA16X_CONF_INT_ATCMD_TLSC_PEER_PORT_1,
    };

    atcmd_tlsc_config *tlsc_conf = NULL;

    char *ca_cert_name = NULL;
    char *cert_name = NULL;
    char *hostname = NULL;
    int incoming_len = ATCMD_TLSC_DEF_INCOMING_LEN;
    int outgoing_len = ATCMD_TLSC_DEF_OUTGOING_LEN;
    int auth_mode = pdFALSE;
    int local_port = 0;
    char peer_ipaddr[ATCMD_TLSC_NVR_PEER_IPADDR_LEN] = {0x00,};
    int peer_port = 0;

    if (ctx) {
        tlsc_conf = &(ctx->conf.tlsc_conf);

        //set ca cert name
        if (strlen(tlsc_conf->ca_cert_name)) {
            ca_cert_name = tlsc_conf->ca_cert_name;

            ATCMD_DBG("[%s:%d]CA cert name: %s(%ld)\n", __func__, __LINE__,
                      ca_cert_name, strlen(ca_cert_name));
        }

        //set cert name
        if (strlen(tlsc_conf->cert_name)) {
            cert_name = tlsc_conf->cert_name;

            ATCMD_DBG("[%s:%d]cert name: %s(%ld)\n", __func__, __LINE__,
                      cert_name, strlen(cert_name));
        }

        //set hostname
        if (strlen(tlsc_conf->hostname)) {
            hostname = tlsc_conf->hostname;

            ATCMD_DBG("[%s:%d]host name: %s(%ld)\n", __func__, __LINE__,
                      hostname, strlen(hostname));
        }

        //set incoming_buflen
        incoming_len = tlsc_conf->incoming_buflen;

        ATCMD_DBG("[%s:%d]incoming_len: %ld\n", __func__, __LINE__, incoming_len);

        //set outgoing_buflen
        outgoing_len = tlsc_conf->outgoing_buflen;

        ATCMD_DBG("[%s:%d]outgoing_len: %ld\n", __func__, __LINE__, outgoing_len);

        //set auth mode
        auth_mode = tlsc_conf->auth_mode;

        ATCMD_DBG("[%s:%d]auth_mode: %ld\n", __func__, __LINE__, auth_mode);

        //set local port
        local_port = tlsc_conf->local_port;

        ATCMD_DBG("[%s:%d]local_port: %ld\n", __func__, __LINE__, local_port);

        //set peer IP address
        longtoip(tlsc_conf->svr_addr, peer_ipaddr);

        ATCMD_DBG("[%s:%d]peer_ipaddr: %s(%ld)\n", __func__, __LINE__,
                  peer_ipaddr, strlen(peer_ipaddr));

        //set peer port
        peer_port = tlsc_conf->svr_port;

        ATCMD_DBG("[%s:%d]peer_port: %ld\n", __func__, __LINE__, peer_port);
    }

    status = da16x_set_config_str(tlsc_nvr_ca_cert_name[profile_idx], ca_cert_name);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_ca_cert_name[profile_idx], status);
    }

    status = da16x_set_config_str(tlsc_nvr_cert_name[profile_idx], cert_name);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_cert_name[profile_idx], status);
    }

    if (strlen(peer_ipaddr)) {
        status = da16x_set_config_str(tlsc_nvr_peer_ipaddr[profile_idx], peer_ipaddr);
    } else {
        status = da16x_set_config_str(tlsc_nvr_peer_ipaddr[profile_idx], NULL);
    }

    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_peer_ipaddr[profile_idx], status);
    }

    status = da16x_set_config_str(tlsc_nvr_hostname[profile_idx], hostname);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_hostname[profile_idx], status);
    }

    status = da16x_set_config_int(tlsc_nvr_incoming_len[profile_idx], incoming_len);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_incoming_len[profile_idx], status);
    }

    status = da16x_set_config_int(tlsc_nvr_outgoing_len[profile_idx], outgoing_len);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_outgoing_len[profile_idx], status);
    }

    status = da16x_set_config_int(tlsc_nvr_auth_mode[profile_idx], auth_mode);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_auth_mode[profile_idx], status);
    }

    status = da16x_set_config_int(tlsc_nvr_local_port[profile_idx], local_port);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_local_port[profile_idx], status);
    }

    status = da16x_set_config_int(tlsc_nvr_peer_port[profile_idx], peer_port);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_peer_port[profile_idx], status);
    }

    return status;
}

static int atcmd_transport_ssl_recover_tlsc_nvram(atcmd_tls_context *ctx, int profile_idx)
{
    int status = DA_APP_SUCCESS;

    ATCMD_DBG("[%s:%d]Start\n", __func__, __LINE__);

    DA16X_USER_CONF_STR tlsc_nvr_ca_cert_name[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_CA_CERT_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_CA_CERT_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_cert_name[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_CERT_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_CERT_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_hostname[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_HOST_NAME_0,
        DA16X_CONF_STR_ATCMD_TLSC_HOST_NAME_1
    };

    DA16X_USER_CONF_STR tlsc_nvr_peer_ipaddr[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_STR_ATCMD_TLSC_PEER_IPADDR_0,
        DA16X_CONF_STR_ATCMD_TLSC_PEER_IPADDR_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_auth_mode[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_AUTH_MODE_0,
        DA16X_CONF_INT_ATCMD_TLSC_AUTH_MODE_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_incoming_len[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_INCOMING_LEN_0,
        DA16X_CONF_INT_ATCMD_TLSC_INCOMING_LEN_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_outgoing_len[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_OUTGOING_LEN_0,
        DA16X_CONF_INT_ATCMD_TLSC_OUTGOING_LEN_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_local_port[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_LOCAL_PORT_0,
        DA16X_CONF_INT_ATCMD_TLSC_LOCAL_PORT_1,
    };

    DA16X_USER_CONF_INT tlsc_nvr_peer_port[ATCMD_TLSC_MAX_ALLOW_NVR_PROFILE] = {
        DA16X_CONF_INT_ATCMD_TLSC_PEER_PORT_0,
        DA16X_CONF_INT_ATCMD_TLSC_PEER_PORT_1,
    };

    atcmd_tlsc_config *tlsc_conf = NULL;

    int int_value = -1;

    char peer_ipaddr[ATCMD_TLSC_NVR_PEER_IPADDR_LEN] = {0x00,};

    tlsc_conf = &(ctx->conf.tlsc_conf);

    //get ca cert name
    da16x_get_config_str(tlsc_nvr_ca_cert_name[profile_idx], tlsc_conf->ca_cert_name);

    //get cert name
    da16x_get_config_str(tlsc_nvr_cert_name[profile_idx], tlsc_conf->cert_name);

    //get hostname
    da16x_get_config_str(tlsc_nvr_hostname[profile_idx], tlsc_conf->hostname);

    //get peer IP address
    da16x_get_config_str(tlsc_nvr_peer_ipaddr[profile_idx], peer_ipaddr);
    if (is_in_valid_ip_class(peer_ipaddr)) {
        tlsc_conf->svr_addr = iptolong(peer_ipaddr);
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_peer_ipaddr[profile_idx], status);
        status = DA_APP_NOT_SUCCESSFUL;
        goto end;
    }

    //get auth mode
    int_value = -1;
    status = da16x_get_config_int(tlsc_nvr_auth_mode[profile_idx], &int_value);
    if (int_value == pdTRUE || int_value == pdFALSE) {
        tlsc_conf->auth_mode = int_value;
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_auth_mode[profile_idx], status);
    }

    //get incoming_buflen
    int_value = -1;
    status = da16x_get_config_int(tlsc_nvr_incoming_len[profile_idx], &int_value);
    if (int_value >= ATCMD_TLSC_MIN_INCOMING_LEN) {
        tlsc_conf->incoming_buflen = int_value;
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_incoming_len[profile_idx], status);
    }

    //get outgoing_buflen
    int_value = -1;
    status = da16x_get_config_int(tlsc_nvr_outgoing_len[profile_idx], &int_value);
    if (int_value >= ATCMD_TLSC_MIN_OUTGOING_LEN) {
        tlsc_conf->outgoing_buflen = int_value;
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_outgoing_len[profile_idx], status);
    }

    //get local port
    int_value = -1;
    status = da16x_get_config_int(tlsc_nvr_local_port[profile_idx], &int_value);
    if (int_value >= 0) {
        tlsc_conf->local_port = int_value;
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_local_port[profile_idx], status);
    }

    //get peer port
    int_value = -1;
    status = da16x_get_config_int(tlsc_nvr_peer_port[profile_idx], &int_value);
    if (int_value > 0) {
        tlsc_conf->svr_port = int_value;
    } else {
        ATCMD_DBG("[%s:%d]failed to get data(%d) to nvram(%d)\n", __func__, __LINE__,
                  tlsc_nvr_peer_port[profile_idx], status);
        status = DA_APP_NOT_SUCCESSFUL;
        goto end;
    }

end:

    return status;
}

static void atcmd_transport_ssl_recover_context_dpm(void)
{
    unsigned int len = 0;
    int idx = 0;

    atcmd_tls_context *prev_ctx = NULL;
    atcmd_tls_context *new_ctx = NULL;
    atcmd_tls_context *src_ctx = NULL;

    if (!dpm_mode_is_enabled()) {
        //Only supported in DPM
        return ;
    }

    if (atcmd_tls_ctx_header) {
        //Already exsited
        return ;
    }

    if (!atcmd_tls_ctx_rtm_header) {
        len = dpm_user_mem_get(ATCMD_TLS_DPM_CONTEXT_NAME,
                               (unsigned char **)&atcmd_tls_ctx_rtm_header);
        if (len == 0) {
            ATCMD_DBG("[%s:%d]Not found tls context\n", __func__, __LINE__);
            return ;
        }
    }

    src_ctx = atcmd_tls_ctx_rtm_header;

    while ((idx < ATCMD_TLS_MAX_ALLOW_CNT) && (src_ctx->role != ATCMD_TLS_NONE)) {
        //create
        new_ctx = ATCMD_MALLOC(sizeof(atcmd_tls_context));
        if (!new_ctx) {
            ATCMD_DBG("[%s:%d]Failed to create atcmd_tls_context(%ld)\n",
                      __func__, __LINE__, sizeof(atcmd_tls_context));
            return ;
        }

        memset(new_ctx, 0x00, sizeof(atcmd_tls_context));

        //copy
        new_ctx->cid = src_ctx->cid;
        new_ctx->role = src_ctx->role;

        ATCMD_DBG("[%s:%d]#%d. cid(%ld), role(%ld)\n", __func__, __LINE__,
                  idx, new_ctx->cid, new_ctx->role);

        if (new_ctx->role == ATCMD_TLS_CLIENT) {
            new_ctx->ctx.tlsc_ctx = NULL;

            //copy configuration
            memcpy(&new_ctx->conf.tlsc_conf,
                   &src_ctx->conf.tlsc_conf,
                   sizeof(atcmd_tlsc_config));
        }

        //link
        if (prev_ctx) {
            prev_ctx->next = new_ctx;
        } else {
            atcmd_tls_ctx_header = new_ctx;
        }

        prev_ctx = new_ctx;
        src_ctx++;
        idx++;
    };

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]Recovery CID: ", __func__, __LINE__);
    for (new_ctx = atcmd_tls_ctx_header ; new_ctx != NULL ; new_ctx = new_ctx->next) {
        ATCMD_DBG("%d, ", new_ctx->cid);
    }
    ATCMD_DBG("\n");
#endif // DEBUG_ATCMD

    return ;
}

static void atcmd_transport_ssl_update_context_dpm(void)
{
    const unsigned long wait_option = 100;

    int status = DA_APP_SUCCESS;
    unsigned int len = 0;

    atcmd_tls_context *dst_ctx = NULL;
    atcmd_tls_context *src_ctx = NULL;

    if (!dpm_mode_is_enabled()) {
        return ;
    }

    ATCMD_DBG("[%s:%d]sizeof(atcmd_tls_context):%ld * %d = %ld\n", __func__, __LINE__,
              sizeof(atcmd_tls_context), ATCMD_TLS_MAX_ALLOW_CNT,
              sizeof(atcmd_tls_context) * ATCMD_TLS_MAX_ALLOW_CNT);

    if (!atcmd_tls_ctx_header) {
        status = dpm_user_mem_free(ATCMD_TLS_DPM_CONTEXT_NAME);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to free rtm memory for tls context(0x%x)\n",
                      __func__, __LINE__, status);
            return ;
        }

        ATCMD_DBG("[%s:%d]Clear CID(DPM)\n", __func__, __LINE__);

        atcmd_tls_ctx_rtm_header = NULL;

        return ;
    }

    if (!atcmd_tls_ctx_rtm_header) {
        len = dpm_user_mem_get(ATCMD_TLS_DPM_CONTEXT_NAME, (unsigned char **)&atcmd_tls_ctx_rtm_header);
        if (len == 0) {
            status = dpm_user_mem_alloc(ATCMD_TLS_DPM_CONTEXT_NAME,
                                        (void **)&atcmd_tls_ctx_rtm_header,
                                        sizeof(atcmd_tls_context) * ATCMD_TLS_MAX_ALLOW_CNT,
                                        wait_option);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to allocate rtm memory for tls context(0x%x)\n",
                          __func__, __LINE__, status);
                return ;
            }
        }
    }

    //init
    memset(atcmd_tls_ctx_rtm_header, 0x00,
           sizeof(atcmd_tls_context) * ATCMD_TLS_MAX_ALLOW_CNT);

    for (int cnt = 0 ; cnt < ATCMD_TLS_MAX_ALLOW_CNT ; cnt++) {
        atcmd_tls_ctx_rtm_header[cnt].cid = -1;
        atcmd_tls_ctx_rtm_header[cnt].role = ATCMD_TLS_NONE;
    }

    //update
    dst_ctx = atcmd_tls_ctx_rtm_header;

    ATCMD_DBG("[%s:%d]Update CID(DPM): ", __func__, __LINE__);
    for (src_ctx = atcmd_tls_ctx_header ; src_ctx != NULL ; src_ctx = src_ctx->next) {
        memcpy(dst_ctx, src_ctx, sizeof(atcmd_tls_context));
        ATCMD_DBG("%d, ", dst_ctx->cid);
        dst_ctx++;
    }
    ATCMD_DBG("\n");

    return ;
}

static atcmd_tls_context *atcmd_transport_ssl_create_context(void)
{
    atcmd_tls_context *new_ctx = NULL;
    atcmd_tls_context *cur_ctx = NULL;
    atcmd_tls_context *prev_ctx = NULL;

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]Before CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_tls_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d, ", cur_ctx->cid);
    }
    ATCMD_DBG("\n");
#endif // DEBUG_ATCMD

    new_ctx = ATCMD_MALLOC(sizeof(atcmd_tls_context));
    if (!new_ctx) {
        ATCMD_DBG("Failed to create atcmd_tls_context(%ld)\n", sizeof(atcmd_tls_context));
        return NULL;
    }

    memset(new_ctx, 0x00, sizeof(atcmd_tls_context));

    if (!atcmd_tls_ctx_header) {
        new_ctx->cid = 0;
        atcmd_tls_ctx_header = new_ctx;
    } else if (atcmd_tls_ctx_header->cid >= 1) {
        new_ctx->cid = 0;
        new_ctx->next = atcmd_tls_ctx_header;
        atcmd_tls_ctx_header = new_ctx;
    } else {
        prev_ctx = atcmd_tls_ctx_header;
        cur_ctx = atcmd_tls_ctx_header->next;

        while (cur_ctx) {
            if (prev_ctx->cid + 1 < cur_ctx->cid) {
                new_ctx->cid = prev_ctx->cid + 1;
                prev_ctx->next = new_ctx;
                new_ctx->next = cur_ctx;
                break;
            }

            prev_ctx = cur_ctx;
            cur_ctx = cur_ctx->next;
        }

        if (!cur_ctx) {
            if (prev_ctx->cid + 1 >= ATCMD_TLS_MAX_ALLOW_CNT) {
                ATCMD_DBG("[%s:%d]Over max count(%d)\n", __func__, __LINE__,
                          ATCMD_TLS_MAX_ALLOW_CNT);

                ATCMD_FREE(new_ctx);
                return NULL;
            }

            new_ctx->cid = prev_ctx->cid + 1;
            prev_ctx->next = new_ctx;
        }
    }

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]After CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_tls_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d, ", cur_ctx->cid);
    }
    ATCMD_DBG("\n");
#endif // DEBUG_ATCMD

    return new_ctx;
}

static atcmd_tls_context *atcmd_transport_ssl_find_context(int cid)
{
    atcmd_tls_context *cur_ctx = NULL;

    for (cur_ctx = atcmd_tls_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        if (cur_ctx->cid == cid) {
            ATCMD_DBG("[%s:%d]Found cid(%d)\n", __func__, __LINE__, cid);
            return cur_ctx;
        }
    }

    return NULL;
}

static int atcmd_transport_ssl_delete_context(int cid)
{
    atcmd_tls_context *cur_ctx = NULL;
    atcmd_tls_context *prev_ctx = NULL;

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]Before CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_tls_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d, ", cur_ctx->cid);
    }
    ATCMD_DBG("\n");
#endif // DEBUG_ATCMD

    if (atcmd_tls_ctx_header == NULL) {
        ATCMD_DBG("[%s:%d]There is no tls context\n", __func__, __LINE__);
        return -1;
    } else if (atcmd_tls_ctx_header->cid == cid) {
        cur_ctx = atcmd_tls_ctx_header;
        atcmd_tls_ctx_header = atcmd_tls_ctx_header->next;
        ATCMD_FREE(cur_ctx);
    } else {
        prev_ctx = atcmd_tls_ctx_header;
        cur_ctx = atcmd_tls_ctx_header->next;

        while (cur_ctx) {
            if (cur_ctx->cid == cid) {
                prev_ctx->next = cur_ctx->next;
                ATCMD_FREE(cur_ctx);
                break;
            }

            prev_ctx = cur_ctx;
            cur_ctx = cur_ctx->next;
        }
    }

    atcmd_transport_ssl_update_context_dpm();

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]After CID: ", __func__, __LINE__);
    for (cur_ctx = atcmd_tls_ctx_header ; cur_ctx != NULL ; cur_ctx = cur_ctx->next) {
        ATCMD_DBG("%d, ", cur_ctx->cid);
    }
    ATCMD_DBG("\n");
#endif // DEBUG_ATCMD

    return 0;
}

/**************************************************************************/
/* Related with TLS client                                                */
/**************************************************************************/
static int atcmd_transport_ssl_create_tls_client(int *cid)
{
    int status = DA_APP_SUCCESS;
    atcmd_tls_context *ctx = NULL;

    ctx = atcmd_transport_ssl_create_context();
    if (!ctx) {
        ATCMD_DBG("[%s:%d]failed to create tls context\n", __func__, __LINE__);
        return DA_APP_MALLOC_ERROR;
    }

    ctx->role = ATCMD_TLS_CLIENT;

    ctx->ctx.tlsc_ctx = ATCMD_MALLOC(sizeof(atcmd_tlsc_context));
    if (!ctx->ctx.tlsc_ctx) {
        ATCMD_DBG("[%s:%d]failed to create tls client context\n",
                  __func__, __LINE__, sizeof(atcmd_tlsc_context));
        atcmd_transport_ssl_delete_context(ctx->cid);
        return DA_APP_MALLOC_ERROR;
    }

    status = atcmd_tlsc_init_context(ctx->ctx.tlsc_ctx);
    if (status != DA_APP_SUCCESS) {
        ATCMD_DBG("[%s:%d]failed to init tls client context(0x%x)\n",
                  __func__, __LINE__, status);
        ATCMD_FREE(ctx->ctx.tlsc_ctx);
        atcmd_transport_ssl_delete_context(ctx->cid);
        return DA_APP_NOT_CREATED;
    }

    *cid = ctx->cid;

    return DA_APP_SUCCESS;
}

static int atcmd_transport_ssl_delete_tls_client(int cid)
{
    int status = DA_APP_SUCCESS;
    atcmd_tls_context *ctx = NULL;
    atcmd_tlsc_context *tlsc_ctx = NULL;

    unsigned long max_wait_time = 1000;

    ctx = atcmd_transport_ssl_find_context(cid);
    if (!ctx || ctx->role != ATCMD_TLS_CLIENT) {
        ATCMD_DBG("[%s:%d]Not found(cid:%d)\n", __func__, __LINE__, cid);
        return DA_APP_NOT_FOUND;
    }

    tlsc_ctx = ctx->ctx.tlsc_ctx;

    status = atcmd_tlsc_stop(tlsc_ctx, max_wait_time);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to stop tls client(0x%x)\n", __func__, __LINE__, status);
        return status;
    }

    status = atcmd_tlsc_deinit_context(tlsc_ctx);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to deinit tls client context(0x%x)n", __func__, __LINE__, status);
    }

    atcmd_transport_ssl_delete_context(cid);

    ATCMD_FREE(tlsc_ctx);

    return DA_APP_SUCCESS;
}

static int atcmd_transport_ssl_run_tls_client(atcmd_tls_context *ctx)
{
    int status = DA_APP_SUCCESS;

    atcmd_tlsc_context *tlsc_ctx = ctx->ctx.tlsc_ctx;

    unsigned long sleep_time = 10;
    unsigned long wait_time = 0;
    unsigned long max_wait_time = (ATCMD_TLSC_RECONN_SLEEP_TIMEOUT * ATCMD_TLSC_MAX_CONN_CNT) +
        (ATCMD_TLSC_HANDSHAKE_TIMEOUT * ATCMD_TLSC_MAX_CONN_CNT);

    status = atcmd_tlsc_run(tlsc_ctx, ctx->cid);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to run tls client(0x%x)\n", __func__, __LINE__, status);
        return status;
    }

    ATCMD_DBG("[%s:%d]Waiting for tls session establishes(%ld)\n",
              __func__, __LINE__, max_wait_time);

    do {
        status = DA_APP_CANNOT_START;

        ATCMD_DBG("[%s:%d]sleep_time(%ld), wait_time(%ld), max(%ld)\n",
                  __func__, __LINE__, sleep_time, wait_time, max_wait_time);

        vTaskDelay(sleep_time);

        wait_time += sleep_time;

        if (tlsc_ctx->state == ATCMD_TLSC_STATE_CONNECTED) {
            status = DA_APP_SUCCESS;
            break;
        } else if (tlsc_ctx->state == ATCMD_TLSC_STATE_TERMINATED) {
            break;
        }
    } while (wait_time < max_wait_time);

    if (status == DA_APP_SUCCESS) {
        atcmd_transport_ssl_update_context_dpm();
    }

    return status;
}

static int atcmd_transport_ssl_recover_tls_session(void)
{
    int status = DA_APP_SUCCESS;
    int retval = DA_APP_SUCCESS;

    atcmd_tls_context *ctx = NULL;

    int idx = 0;
    int del_cid_idx = 0;
    int del_cid[ATCMD_TLS_MAX_ALLOW_CNT] = {-1,};

    unsigned long sleep_time = 10;
    unsigned long wait_time = 0;
    unsigned long max_wait_time = (ATCMD_TLSC_RECONN_SLEEP_TIMEOUT * ATCMD_TLSC_MAX_CONN_CNT) +
        (ATCMD_TLSC_HANDSHAKE_TIMEOUT * ATCMD_TLSC_MAX_CONN_CNT);

    if (!atcmd_tls_ctx_header) {
        return 0;
    }

    //wait for network connection
    if (dpm_mode_is_enabled() && !dpm_mode_is_wakeup()) {
        if (chk_network_ready(WLAN0_IFACE) != 1) {
            int wifi_wait_time = 10;

            while (1) {
                if (chk_network_ready(0) == 1) {
                    break;
                }

                vTaskDelay(100);

                if (--wifi_wait_time == 0) {
                    ATCMD_DBG("[TLS Client] Timeout to connect an Wi-Fi AP\n");
                    return DA_APP_NOT_SUCCESSFUL;
                }
            }
        }
    }

    //for tls session
    for (ctx = atcmd_tls_ctx_header ; ctx != NULL ; ctx = ctx->next) {
        if (ctx->role == ATCMD_TLS_CLIENT) {    //tls client
            ctx->ctx.tlsc_ctx = ATCMD_MALLOC(sizeof(atcmd_tlsc_context));
            if (!ctx->ctx.tlsc_ctx) {
                ATCMD_DBG("[%s:%d]failed to create tls client context\n",
                          __func__, __LINE__, sizeof(atcmd_tlsc_context));
                goto end_of_tls_client;
            }

            status = atcmd_tlsc_init_context(ctx->ctx.tlsc_ctx);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to init tls client context(0x%x)\n",
                          __func__, __LINE__, status);
                goto end_of_tls_client;
            }

            status = atcmd_tlsc_setup_config(ctx->ctx.tlsc_ctx, &(ctx->conf.tlsc_conf));
            if (status) {
                ATCMD_DBG("[%s:%d]failed to setup tls client's config(0x%x)\n",
                          __func__, __LINE__, status);
                goto end_of_tls_client;
            }

            status = atcmd_tlsc_run(ctx->ctx.tlsc_ctx, ctx->cid);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to run tls client(0x%x)\n",
                          __func__, __LINE__, status);
                goto end_of_tls_client;
            }

end_of_tls_client:

            if (status) {
                if (ctx->ctx.tlsc_ctx) {
                    ATCMD_FREE(ctx->ctx.tlsc_ctx);
                    ctx->ctx.tlsc_ctx = NULL;
                }

                retval += status;
                del_cid[del_cid_idx++] = ctx->cid;
            }
        }
    }

    for (idx = 0 ; idx < del_cid_idx ; idx++) {
        atcmd_transport_ssl_delete_context(del_cid[idx]);
    }

    //wait for connection
    for (ctx = atcmd_tls_ctx_header ; ctx != NULL ; ctx = ctx->next) {
        if (ctx->role == ATCMD_TLS_CLIENT) {
            do {
                status = DA_APP_CANNOT_START;

                ATCMD_DBG("[%s:%d]cid(%ld), sleep_time(%ld), wait_time(%ld), max(%ld)\n",
                          __func__, __LINE__,
                          ctx->cid, sleep_time, wait_time, max_wait_time);

                vTaskDelay(sleep_time);

                wait_time += sleep_time;

                if (ctx->ctx.tlsc_ctx->state == ATCMD_TLSC_STATE_CONNECTED) {
                    status = DA_APP_SUCCESS;
                    break;
                } else if (ctx->ctx.tlsc_ctx->state == ATCMD_TLSC_STATE_TERMINATED) {
                    break;
                }
            } while (wait_time < max_wait_time);
        }
    }

    atcmd_transport_ssl_update_context_dpm();

    return retval;
}

/**************************************************************************/
/* Related with command of TLS client/server                              */
/**************************************************************************/
static int atcmd_transport_ssl_do_init(int role, int *cid)
{
    int status = DA_APP_SUCCESS;

    if (role == ATCMD_TLS_SERVER) {
        ATCMD_DBG("[%s:%d]Not implemented yet\n", __func__, __LINE__);
        status = DA_APP_NOT_IMPLEMENTED;
        goto ssl_do_init_end;
    } else if (role == ATCMD_TLS_CLIENT) {
        status = atcmd_transport_ssl_create_tls_client(cid);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to create tls client(0x%x)\n", __func__, __LINE__, status);
            goto ssl_do_init_end;
        }
    }

    ATCMD_DBG("[%s:%d]Role(%d), CID(%d)\n", __func__, __LINE__, role, *cid);

ssl_do_init_end :
    return status;
}

static int atcmd_transport_ssl_do_cfg(char *cid_str, char *conf_id_str, char *conf_val_str)
{
    int status = DA_APP_SUCCESS;

    int cid = -1;
    int conf_id = 0;

    atcmd_tls_context *ctx = NULL;
    atcmd_tlsc_config *tlsc_conf = NULL;

    int auth_mode = 0;
    int buflen = 0;

    DA16X_UNUSED_ARG(conf_id_str);

    //check cid
    if (get_int_val_from_str(cid_str, &cid, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]Failed to get CID(%s)\n", __func__, __LINE__, cid_str);
        status = DA_APP_PARAMETER_ERROR;
        goto end;
    }

    ctx = atcmd_transport_ssl_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found(cid:%d)\n", __func__, __LINE__, cid);
        status = DA_APP_NOT_FOUND;
        goto end;
    }

    if (ctx->role == ATCMD_TLS_CLIENT) {
        if (ctx->ctx.tlsc_ctx->state != ATCMD_TLSC_STATE_TERMINATED) {
            ATCMD_DBG("[%s:%d]TLS client is not terminated\n", __func__, __LINE__);
            status = DA_APP_ALREADY_ENABLED;
            goto end;
        }

        tlsc_conf = &(ctx->conf.tlsc_conf);
    } else {
        ATCMD_DBG("[%s:%d]Not implemented(%d)\n", __func__, __LINE__, ctx->role);
        status = DA_APP_NOT_IMPLEMENTED;
        goto end;
    }

    //check configuration ID
    if (get_int_val_from_str(conf_id_str, &conf_id, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]Failed to get configuration ID(%s)\n", __func__, __LINE__, conf_id_str);
        status = DA_APP_NOT_SUCCESSFUL;
        goto end;
    }

    //check configuration value
    switch (conf_id) {
    case 1: // TLS version
        //Not supported
        ATCMD_DBG("[%s:%d]Not supported to set TLS Version\n", __func__, __LINE__);
        status = DA_APP_NOT_SUPPORTED;
        break;

    case 2: // SSL CA certificate name
        if (atcmd_cm_is_exist_cert(conf_val_str, ATCMD_CM_CERT_TYPE_CA_CERT, 0)) {
            if (ctx->role == ATCMD_TLS_CLIENT) {    //tls_client
                strcpy(tlsc_conf->ca_cert_name, conf_val_str);
            }
        } else {
            ATCMD_DBG("[%s:%d]Not found certificate(%s)\n", __func__, __LINE__, conf_val_str);
            status = DA_APP_SSL_CFG_CA_CERT;
        }
        break;

    case 3: // SSL certificate name
        if (   atcmd_cm_is_exist_cert(conf_val_str, ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_CERT)
            && atcmd_cm_is_exist_cert(conf_val_str, ATCMD_CM_CERT_TYPE_CERT, ATCMD_CM_CERT_SEQ_KEY)) {

            if (ctx->role == ATCMD_TLS_CLIENT) {    //tls_client
                strcpy(tlsc_conf->cert_name, conf_val_str);
            }
        } else {
            ATCMD_DBG("[%s:%d]Not found certificate(%s)\n", __func__, __LINE__, conf_val_str);
            status = DA_APP_SSL_CFG_CERT;
        }
        break;

    case 4: // Cipher value bitmap where the its values.
        ATCMD_DBG("[%s:%d]Not implemented to set cipher value\n", __func__, __LINE__);
        status = DA_APP_NOT_IMPLEMENTED;
        break;

    case 5: // Set the Tx Max fragment length
        ATCMD_DBG("[%s:%d]Not supported to set Tx Max fragment length\n", __func__, __LINE__);
        status = DA_APP_NOT_SUPPORTED;
        break;

    case 6: // Set the SNI
        if (ctx->role == ATCMD_TLS_CLIENT) {
            status = atcmd_tlsc_set_hostname(tlsc_conf, conf_val_str);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to set hostname(0x%x)\n", __func__, __LINE__, status);
                status = DA_APP_SSL_CFG_SET_SNI;
            }
        } else {
            status = DA_APP_SSL_ROLE_NOT_SUPPORT;
        }
        break;

    case 7: // Set the Domain
        ATCMD_DBG("[%s:%d]Not implemented to set Domain\n", __func__, __LINE__);
        status = DA_APP_NOT_IMPLEMENTED;
        break;

    case 8: // Set the Max Fragment Length
        ATCMD_DBG("[%s:%d]Not implemented to set Max Fragment length\n", __func__, __LINE__);
        status = DA_APP_NOT_IMPLEMENTED;
        break;

    case 9: // To enale/disale server validation
        if (get_int_val_from_str(conf_val_str, &auth_mode, POL_1) != 0) {
            ATCMD_DBG("[%s:%d]failed to get Auth mode\n", __func__, __LINE__, conf_val_str);
            status = DA_APP_SSL_SVR_VAILD_TYPE;
            break;
        }

        if (is_in_valid_range(auth_mode, 0, 1) == pdFALSE) {
            status = DA_APP_SSL_SVR_VAILD_RANGE;
            break;
        }

        // tls_client
        if (ctx->role == ATCMD_TLS_CLIENT) {
            tlsc_conf->auth_mode = auth_mode == 0 ? pdFALSE : pdTRUE;
        } else {
            status = DA_APP_SSL_ROLE_NOT_SUPPORT;
        }
        break;

    case 10: // Set incoming buffer size
        if (get_int_val_from_str(conf_val_str, &buflen, POL_1) != 0) {
            ATCMD_DBG("[%s:%d]failed to get incoming buflen\n", __func__, __LINE__, conf_val_str);
            status = DA_APP_SSL_RX_BUF_LENTH;
            break;
        }

        //tls_client
        if (ctx->role == ATCMD_TLS_CLIENT) {
            status = atcmd_tlsc_set_incoming_buflen(tlsc_conf, buflen);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to set incoming buflen(0x%x,%ld)\n", __func__, __LINE__, status, buflen);
                break;
            }
        } else {
            status = DA_APP_SSL_ROLE_NOT_SUPPORT;
        }
        break;

    case 11: // Set outgoing buffer size
        if (get_int_val_from_str(conf_val_str, &buflen, POL_1) != 0) {
            ATCMD_DBG("[%s:%d]failed to get outgoing buflen\n", __func__, __LINE__, conf_val_str);
            status = DA_APP_SSL_TX_BUF_LENTH;
            break;
        }

        //tls_client
        if (ctx->role == ATCMD_TLS_CLIENT) {
            atcmd_tlsc_set_outgoing_buflen(tlsc_conf, buflen);
            if (status) {
                ATCMD_DBG("[%s:%d]failed to set outgoing buflen(0x%x,%ld)\n", __func__, __LINE__, status, buflen);
                break;
            }
        } else {
            status = DA_APP_SSL_ROLE_NOT_SUPPORT;
        }
        break;

    default:
        ATCMD_DBG("[%s:%d]Invalid Configuration ID(%d)\n", __func__, __LINE__, conf_id);
        status = DA_APP_SSL_CFG_CONF_ID_NOT_SUPPORT;
        break;
    }

end:

    return status;
}

static int atcmd_transport_ssl_do_co(char *cid_str, char *ip_str, char *port_str)
{
    int status = DA_APP_SUCCESS;
    int cid = -1;

    atcmd_tls_context *ctx = NULL;
    atcmd_tlsc_context *tlsc_ctx = NULL;
    atcmd_tlsc_config *tlsc_conf = NULL;

    int svr_port = 0;

    //check cid
    if (get_int_val_from_str(cid_str, &cid, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get CID(%s)\n", __func__, __LINE__, cid_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    ctx = atcmd_transport_ssl_find_context(cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found(cid:%d)\n", __func__, __LINE__, cid);
        return DA_APP_NOT_FOUND;
    } else if (ctx->role != ATCMD_TLS_CLIENT) {
        ATCMD_DBG("[%s:%d]Not implemented yet(cid:%d)\n", __func__, __LINE__, cid);
        return DA_APP_NOT_IMPLEMENTED;
    }

    tlsc_ctx = ctx->ctx.tlsc_ctx;
    tlsc_conf = &(ctx->conf.tlsc_conf);

    //check tls client' state
    if (tlsc_ctx->state != ATCMD_TLSC_STATE_TERMINATED) {
        ATCMD_DBG("[%s:%d]TLS Client state is not terminated(%d)\n", __func__, __LINE__, tlsc_ctx->state);
        return DA_APP_CANNOT_START;
    }

    if (get_int_val_from_str(port_str, &svr_port, POL_1) == 0) {
        tlsc_conf->svr_port = svr_port;
    } else {
        ATCMD_DBG("[%s:%d]failed to get port(%s)\n", __func__, __LINE__, port_str);
        return DA_APP_SSL_CONN_INVALID_PORT_NUM;
    }

    if (!is_in_valid_ip_class(ip_str)) {
        struct addrinfo hints, *addr_list = NULL;
        struct sockaddr_in svr_addr;

        memset(&hints, 0x00, sizeof(struct addrinfo));
        memset(&svr_addr, 0x00, sizeof(struct sockaddr_in));

        status = getaddrinfo(ip_str, port_str, &hints, &addr_list);
        if ((status!= 0) || !addr_list) {
            ATCMD_DBG("[%s:%d]Failed to get address info(%d)\r\n", __func__, __LINE__, status);
            return DA_APP_SSL_CONN_UNKNOWN_HOST;
        }

        memcpy((struct sockaddr *)&svr_addr, addr_list->ai_addr, sizeof(struct sockaddr));

        freeaddrinfo(addr_list);

        tlsc_conf->svr_addr = htonl(svr_addr.sin_addr.s_addr);
    } else {
        tlsc_conf->svr_addr = iptolong(ip_str);
    }

    if (da16x_network_main_check_net_init(WLAN0_IFACE)) {
        tlsc_conf->iface = WLAN0_IFACE;
    } else if (da16x_network_main_check_net_init(WLAN1_IFACE)) {
        tlsc_conf->iface = WLAN1_IFACE;
    }

    status = atcmd_tlsc_setup_config(tlsc_ctx, tlsc_conf);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to setup tls client's config(0x%x)\n", __func__, __LINE__, status);

        status = DA_APP_SSL_CONN_SETUP_CFG_ERR;
        return status;
    }

    status = atcmd_transport_ssl_run_tls_client(ctx);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to run tls client(0x%x)\n", __func__, __LINE__, status);
        status = DA_APP_SSL_CONN_TLS_CLIENT_RUN_ERR;
        return status;
    }

    return status;
}

static int atcmd_transport_ssl_do_cl(char *cid_str, int *cid)
{
    int status = DA_APP_SUCCESS;

    atcmd_tls_context *ctx = NULL;

    *cid = -1;

    if (get_int_val_from_str(cid_str, cid, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get CID(%s)\n", __func__, __LINE__, cid_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    ctx = atcmd_transport_ssl_find_context(*cid);
    if (!ctx) {
        ATCMD_DBG("[%s:%d]Not found(cid:%d)\n", __func__, __LINE__, *cid);
        return DA_APP_NOT_FOUND;
    }

    if (ctx->role == ATCMD_TLS_CLIENT) {
        status = atcmd_transport_ssl_delete_tls_client(*cid);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to close tls client(%d, 0x%x)\n", __func__, __LINE__, *cid, status);
        }
    } else {
        ATCMD_DBG("[%s:%d]Not implemented yet\n", __func__, __LINE__);
        return DA_APP_NOT_IMPLEMENTED;
    }

    ATCMD_DBG("[%s:%d]CID(%d)\n", __func__, __LINE__, *cid);

    return DA_APP_SUCCESS;
}

static int atcmd_transport_ssl_do_certlist(char *type_str, char **out)
{
    int idx = 0;
    int type = -1;
    const atcmd_cm_cert_info_t *cert_info;

    char *cert_names = NULL;
    char cert_name[ATCMD_CM_MAX_NAME + 4] = {0x00,};

    size_t cert_names_len = sizeof(cert_name) * ATCMD_CM_MAX_CERT_NUM;

    if (get_int_val_from_str(type_str, &type, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get type(%s)\n", __func__, __LINE__, type_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    cert_names = ATCMD_MALLOC(cert_names_len);
    if (!cert_names) {
        ATCMD_DBG("[%s:%d]failed to allocate memory for result(%ld)\n",
                  __func__, __LINE__, (ATCMD_CM_MAX_NAME + 4) * ATCMD_CM_MAX_CERT_NUM);
        return DA_APP_MALLOC_ERROR;
    }

    memset(cert_names, 0x00, cert_names_len);

    cert_info = atcmd_cm_get_cert_info();
    if (cert_info) {
        for (idx = 0 ; idx < ATCMD_CM_MAX_CERT_NUM ; idx++) {
            if ((strlen(cert_info[idx].name) > 0) && (cert_info[idx].type == type)) {
                if (strlen(cert_names) > 0) {
                    sprintf(cert_name, "%d,%s", cert_info[idx].type, cert_info[idx].name);
                    snprintf(cert_names, cert_names_len, "%s,%s", cert_names, cert_name);
                } else {
                    snprintf(cert_names, cert_names_len, "%d,%s", cert_info[idx].type, cert_info[idx].name);
                }
            }
        }
    }

    *out = cert_names;

    return DA_APP_SUCCESS;
}

static int atcmd_transport_ssl_do_certstore(char *type_str, char *seq_str, char *format_str, char *name_str,
                                            char *data, size_t data_len)
{
    int status = DA_APP_SUCCESS;

    int type = -1;
    int seq = -1;
    int format = -1;

    if (get_int_val_from_str(type_str, &type, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get type(%s)\n", __func__, __LINE__, type_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    if (get_int_val_from_str(seq_str, &seq, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get seq(%s)\n", __func__, __LINE__, seq_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    if (get_int_val_from_str(format_str, &format, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get format(%s)\n", __func__, __LINE__, format_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    if (data_len == 0) {
        data_len = (strlen(data) + 1);
    }

    status = atcmd_cm_set_cert(name_str, (unsigned char)type, (unsigned char)seq,
                               (unsigned char)format, data, data_len);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to set certificate(name: %s(%ld), type: %d, cert: (%ld)\n",
                  __func__, __LINE__, name_str, strlen(name_str), type, data_len);
        return status;
    }

    return DA_APP_SUCCESS;
}

static int atcmd_transport_ssl_do_certdelete(char *type_str, char *name_str)
{
    int status = DA_APP_SUCCESS;
    int type = -1;

    if (get_int_val_from_str(type_str, &type, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get type(%s)\n", __func__, __LINE__, type_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    status = atcmd_cm_clear_cert(name_str, type);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to clear certificate(%s,%d,0x%x)\n",
                  __func__, __LINE__, name_str, type, status);
        return status;
    }

    return status;
}

static int atcmd_transport_ssl_do_save(void)
{
    int status = DA_APP_SUCCESS;

    atcmd_tls_context *ctx = NULL;
    int tls_nvr_idx = 0;

    if (!atcmd_tls_ctx_header) {
        ATCMD_DBG("[%s:%d]empty CID\n", __func__, __LINE__);
        status = atcmd_transport_ssl_clear_all_context_nvram();

        if (status != DA_APP_SUCCESS) {
            status = DA_APP_SSL_CLR_CTX_NVRAM_FAIL;
        }

        return status;
    }

    for (ctx = atcmd_tls_ctx_header ; ctx != NULL ; ctx = ctx->next) {
        if (tls_nvr_idx > ATCMD_TLS_MAX_ALLOW_CNT) {
            ATCMD_DBG("[%s:%d]Not able to save context(cid:%d, role:%d)\n", __func__, __LINE__,
                      ctx->cid, ctx->role);
            break;
        }

        status = atcmd_transport_ssl_save_context_nvram(ctx);
        if (status) {
            ATCMD_DBG("[%s:%d]failed to save context(0x%x)\n", __func__, __LINE__,
                      status);
            continue;
        }

        tls_nvr_idx++;
    }

    if (status != DA_APP_SUCCESS) {
        status = DA_APP_SSL_SAVE_CTX_NVRAM_FAIL;
    }

    return status;
}

static int atcmd_transport_ssl_do_delete(void)
{
    return atcmd_transport_ssl_clear_all_context_nvram();
}

int atcmd_transport_ssl(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK;
    char result_str[8] = {0x00, };
    char *certlist_str = NULL;
    int    tmp_int1 = 0;
    int    status;

    //AT+TRSSLINIT
    if (strcasecmp(argv[0] + 8, "INIT") == 0) {
        if (argc == 2) {
            int cid = -1;

            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                goto transport_ssl_end;
            }
            if ((tmp_int1 != ATCMD_TLS_SERVER) && (tmp_int1 != ATCMD_TLS_CLIENT)) {
                err_code = AT_CMD_ERR_COMMON_ARG_RANGE;
                goto transport_ssl_end;
            }

            status = atcmd_transport_ssl_do_init(tmp_int1, &cid);

            switch (status) {
            case DA_APP_SUCCESS:
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_NOT_IMPLEMENTED :
                err_code = AT_CMD_ERR_SSL_ROLE_NOT_SUPPORT;
                goto transport_ssl_end;
                break;

            case DA_APP_MALLOC_ERROR    :
                err_code = AT_CMD_ERR_MEM_ALLOC;
                goto transport_ssl_end;
                break;

            case DA_APP_NOT_CREATED        :
                err_code = AT_CMD_ERR_UNKNOWN;
                goto transport_ssl_end;
                break;
            }

            snprintf(result_str, sizeof(result_str), "%d", cid);
            err_code = AT_CMD_ERR_CMD_OK;
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    //AT+TRSSLCFG
    else if (strcasecmp(argv[0] + 8, "CFG") == 0) {
        if (argc == 4) {
            // CID
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_SSL_CONF_CID_TYPE;
                goto transport_ssl_end;
            }

            // Configuration-ID
            if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_SSL_CONF_ID_TYPE;
                goto transport_ssl_end;
            }
            if (is_in_valid_range(tmp_int1, 0, 11) == pdFALSE) {
                err_code = AT_CMD_ERR_SSL_CONF_ID_RANGE;
                goto transport_ssl_end;
            }

            status = atcmd_transport_ssl_do_cfg(argv[1], argv[2], argv[3]);
            switch (status) {
            case DA_APP_SUCCESS:
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_SSL_ROLE_NOT_SUPPORT :
                err_code = AT_CMD_ERR_SSL_ROLE_NOT_SUPPORT;
                break;

            case DA_APP_NOT_SUPPORTED :
            case DA_APP_NOT_IMPLEMENTED :
            case DA_APP_SSL_CFG_CONF_ID_NOT_SUPPORT :
                err_code = AT_CMD_ERR_SSL_CONF_ID_NOT_SUPPORTED;
                break;

            case DA_APP_NOT_FOUND :
                err_code = AT_CMD_ERR_SSL_CONTEXT_NOT_FOUND;
                break;

            case DA_APP_ALREADY_ENABLED :
                err_code = AT_CMD_ERR_SSL_CONTEXT_ALREADY_EXIST;
                break;

            case DA_APP_SSL_CFG_CA_CERT :
                err_code = AT_CMD_ERR_SSL_CONF_CID_CA_CERT;
                break;

            case DA_APP_SSL_CFG_CERT :
                err_code = AT_CMD_ERR_SSL_CONF_CID_CERT;
                break;

            case DA_APP_SSL_CFG_SET_SNI    :
                err_code = AT_CMD_ERR_SSL_CONF_CID_SNI;
                break;

            case DA_APP_SSL_SVR_VAILD_TYPE :
                err_code = AT_CMD_ERR_SSL_CONF_CID_SVR_VALID_TYPE;
                break;

            case DA_APP_SSL_SVR_VAILD_RANGE    :
                err_code = AT_CMD_ERR_SSL_CONF_CID_SVR_VALID_RANGE;
                break;

            case DA_APP_SSL_RX_BUF_LENTH :
                err_code = AT_CMD_ERR_SSL_CONF_CID_RX_BUF_LEN;
                break;

            case DA_APP_SSL_TX_BUF_LENTH :
                err_code = AT_CMD_ERR_SSL_CONF_CID_TX_BUF_LEN;
                break;
            }
            // Check error
            if (err_code != AT_CMD_ERR_CMD_OK) {
                goto transport_ssl_end;
            }

            err_code = AT_CMD_ERR_CMD_OK;
        } else {
            if (argc < 4) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    //AT+TRSSLCO
    else if (strcasecmp(argv[0] + 8, "CO") == 0) {
        if (argc == 4) {
            status = atcmd_transport_ssl_do_co(argv[1], argv[2], argv[3]);
            switch (status) {
            case DA_APP_SUCCESS:
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_INVALID_PARAMETERS :
                err_code = AT_CMD_ERR_SSL_CONN_CID_TYPE;
                break;

            case DA_APP_NOT_FOUND :
                err_code = AT_CMD_ERR_SSL_CONTEXT_NOT_FOUND;
                break;

            case DA_APP_NOT_IMPLEMENTED :
                err_code = AT_CMD_ERR_SSL_ROLE_NOT_SUPPORT;
                break;

            case DA_APP_CANNOT_START :
                err_code = AT_CMD_ERR_SSL_CONN_ALREADY_CONNECTED;
                break;

            case DA_APP_SSL_CONN_INVALID_PORT_NUM :
                err_code = AT_CMD_ERR_SSL_CONN_PORT_NUM_TYPE;
                break;

            case DA_APP_SSL_CONN_UNKNOWN_HOST :
                err_code = AT_CMD_ERR_SSL_CONN_UNKNOWN_HOSTNAME;
                break;

            case DA_APP_SSL_CONN_SETUP_CFG_ERR :
                err_code = AT_CMD_ERR_SSL_CONN_CFG_SETUP_FAIL;
                break;

            case DA_APP_SSL_CONN_TLS_CLIENT_RUN_ERR :
                err_code = AT_CMD_ERR_SSL_CONN_TLS_CLINET_RUN_FAIL;
                break;

            default :
                err_code = AT_CMD_ERR_UNKNOWN;
                break;
            }
        } else {
            if (argc < 4) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    //AT+TRSSLCL
    else if (strcasecmp(argv[0] + 8, "CL") == 0) {
        if (argc == 2) {
            int cid = -1;

            status = atcmd_transport_ssl_do_cl(argv[1], &cid);
            switch (status) {
            case DA_APP_SUCCESS:
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_INVALID_PARAMETERS :
                err_code = AT_CMD_ERR_COMMON_ARG_TYPE;
                break;

            case DA_APP_NOT_FOUND :
                err_code = AT_CMD_ERR_SSL_CONTEXT_NOT_FOUND;
                break;

            case DA_APP_NOT_IMPLEMENTED :
                err_code = AT_CMD_ERR_SSL_ROLE_NOT_SUPPORT;
                break;
            }
            // Check error
            if (err_code != AT_CMD_ERR_CMD_OK) {
                goto transport_ssl_end;
            }

            snprintf(result_str, sizeof(result_str), "%d", cid);
            err_code = AT_CMD_ERR_CMD_OK;
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    //AT+TRSSLCERTLIST
    else if (strcasecmp(argv[0] + 8, "CERTLIST") == 0) {
        if (argc == 2) {
            status = atcmd_transport_ssl_do_certlist(argv[1], &certlist_str);
            switch (status) {
            case DA_APP_SUCCESS:
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_INVALID_PARAMETERS :
                err_code = AT_CMD_ERR_SSL_CERT_TYPE;
                break;

            case DA_APP_MALLOC_ERROR :
                err_code = AT_CMD_ERR_MEM_ALLOC;
                break;

            }
            // Check error
            if (err_code != AT_CMD_ERR_CMD_OK) {
                goto transport_ssl_end;
            }
        } else {
            if (argc < 2) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    //AT+TRSSLCERTDELETE
    else if (strcasecmp(argv[0] + 8, "CERTDELETE") == 0) {
        if (argc == 3) {
            // Certificate-type
            if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
                err_code = AT_CMD_ERR_SSL_CERT_TYPE;
                goto transport_ssl_end;
            }
            // ATCMD_CM_CERT_TYPE_CA_CERT=0, ATCMD_CM_CERT_TYPE_CERT=1
            if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
                err_code = AT_CMD_ERR_SSL_CERT_RANGE;
                goto transport_ssl_end;
            }

            err_code = AT_CMD_ERR_CMD_OK;

            status = atcmd_transport_ssl_do_certdelete(argv[1], argv[2]);
            switch (status) {
            case DA_APP_SUCCESS :
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_NOT_FOUND :
                err_code = AT_CMD_ERR_SSL_CERT_DEL_LIST_NOT_FOUND;
                break;

            case DA_APP_NOT_CREATED :
                err_code = AT_CMD_ERR_MEM_ALLOC;
                break;

            default :
                err_code = AT_CMD_ERR_UNKNOWN;
                break;
            }
        } else {
            if (argc < 3) {
                err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
            }
        }
    }
    // AT+TRSSLSAVE
    else if (strcasecmp(argv[0] + 8, "SAVE") == 0) {
        if (argc == 1) {
            err_code = AT_CMD_ERR_CMD_OK;

            status = atcmd_transport_ssl_do_save();
            switch (status) {
            case DA_APP_SUCCESS :
                err_code = AT_CMD_ERR_CMD_OK;
                break;

            case DA_APP_SSL_CLR_CTX_NVRAM_FAIL :
                err_code = AT_CMD_ERR_SSL_SAVE_CLR_ALL_NV;
                break;

            case DA_APP_SSL_SAVE_CTX_NVRAM_FAIL :
                err_code = AT_CMD_ERR_SSL_SAVE_FAIL_NV;
                break;
            }
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
    // AT+TRSSLDELETE
    else if (strcasecmp(argv[0] + 8, "DELETE") == 0) {
        if (argc == 1) {
            status = atcmd_transport_ssl_do_delete();
            if (status) {
                ATCMD_DBG("[%s:%d]failed to progress DELETE(0x%x)\n", __func__, __LINE__, status);
                err_code = AT_CMD_ERR_SSL_SAVE_CLR_ALL_NV;
                goto transport_ssl_end;
            }

            err_code = AT_CMD_ERR_CMD_OK;
        } else {
            err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        }
    }
#if defined (DEBUG_ATCMD)
    // AT+TRSSLLIST
    else if (strcasecmp(argv[0] + 8, "LIST") == 0) {
        atcmd_tls_context *ctx = NULL;
        int idx = 1;

        if (atcmd_tls_ctx_header) {
            for (ctx = atcmd_tls_ctx_header ; ctx != NULL ; ctx = ctx->next) {
                ATCMD_DBG("[%s:%d]#%d. CID: %d, Role: %d\n", __func__, __LINE__, idx, ctx->cid, ctx->role);
                idx++;
            }

        } else {
            ATCMD_DBG("[%s:%d]Empty list\n", __func__, __LINE__);
        }

        err_code = AT_CMD_ERR_CMD_OK;
    }
#endif // (DEBUG_ATCMD)
    else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

transport_ssl_end :
    if (certlist_str) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, certlist_str);
        ATCMD_FREE(certlist_str);
    } else if (((strlen(result_str) > 0) && (strncmp(result_str, "OK", 2) != 0))
            && err_code == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, result_str);
    }

    return err_code;
}

static void atcmd_transport_ssl_reboot(void)
{
    int err_code = DA_APP_SUCCESS;
    atcmd_tls_context *ctx = NULL;

    while (atcmd_tls_ctx_header) {
        ctx = atcmd_tls_ctx_header;

        ATCMD_DBG("[%s:%d]cid(%ld), role(%ld)\n", __func__, __LINE__, ctx->cid, ctx->role);

        if (ctx->role == ATCMD_TLS_CLIENT) {
            err_code = atcmd_transport_ssl_delete_tls_client(ctx->cid);
            if (err_code) {
                ATCMD_DBG("[%s:%d]failed to close tls client(%d, 0x%x)\n", __func__, __LINE__, ctx->cid, err_code);
            }
        }
    }

    return;
}
#endif // (__SUPPORT_ATCMD_TLS__)

#if !defined ( __USER_UART_CONFIG__ )
int atcmd_uartconf(int argc, char *argv[])
{
    UINT32 clock;
    UINT32 baud = DEFAULT_UART_BAUD;
    UINT32 temp;
    UINT32 bits = 8;
    UINT32 stopbit = 1;
    UINT32 flow_ctrl = 0;
    volatile char parity = 'n';
    UINT32 parity_temp = UART_PARITY_ENABLE(0) | UART_EVEN_PARITY(0);

    UINT32  fifo_en = 1;
    UINT32  DMA = 0;
    UINT32  swflow = 0;
    UINT32  word_access = 0;
    UINT32  rw_word = 0;

    uart_info_t    *atcmd_uart_info = NULL;
    int    rtm_len;

    HANDLE    atcmd_uart = NULL;
    int tmp_int1 = 0;
    atcmd_error_code        err_code = AT_CMD_ERR_CMD_OK;

#if defined ( __ATCMD_IF_UART2__ )
    atcmd_uart = uart2;
#else
    atcmd_uart = uart1;            // Default
#endif // __ATCMD_IF_UART2__

    if (atcmd_uart == NULL) {
        err_code = AT_CMD_ERR_UART_INTERFACE;
        goto atcmd_uartconf_end;
    }

    // Read current baudrate
    if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
        /* Read new values in NVRAM */
        if (read_nvram_int((const char *)NVR_KEY_ATCMD_UART_BAUDRATE, (int *)&baud) != 0)
            baud = DEFAULT_UART_BAUD;

        PRINTF_ATCMD("\r\n+BAUDRATE:%d bps\r\n", baud);

        return AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

#if defined ( __UART1_FLOW_CTRL_ON__ )
    if (argc > 6) {
#else
    if (argc > 5) {
#endif /* __UART1_FLOW_CTRL_ON__ */
        err_code = AT_CMD_ERR_TOO_MANY_ARGS;
        goto atcmd_uartconf_end;
    }

    if (argc > 1) {
        if (get_int_val_from_str(argv[1], &tmp_int1, POL_1) != 0) {
            err_code = AT_CMD_ERR_UART_BAUDRATE;
            goto atcmd_uartconf_end;
        }

        baud = tmp_int1;

        switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
        case 230400:
        case 460800:
        case 921600:
            break;

        default:
            err_code = AT_CMD_ERR_UART_BAUDRATE;
            goto atcmd_uartconf_end;
            break;
        }

        /* Default Parity-bit */
    }

    if (argc > 2) {
        if (get_int_val_from_str(argv[2], &tmp_int1, POL_1) != 0) {
            err_code = AT_CMD_ERR_UART_DATABITS;
            goto atcmd_uartconf_end;
        }
        bits = tmp_int1;

        if (bits < 5 || bits > 8) {
            err_code = AT_CMD_ERR_UART_DATABITS;
            goto atcmd_uartconf_end;
        }
    }

    if (argc > 3) {
        parity = argv[3][0];

        if (!(parity == 'n' || parity == 'e' || parity == 'o')) {
            err_code = AT_CMD_ERR_UART_PARITY;
            goto atcmd_uartconf_end;
        }

        switch (parity) {
        case 'n':
            parity_temp = UART_PARITY_ENABLE(0) | UART_EVEN_PARITY(0);
            break;

        case 'e':
            parity_temp = UART_PARITY_ENABLE(1) | UART_EVEN_PARITY(1);
            break;

        case 'o':
            parity_temp = UART_PARITY_ENABLE(1) | UART_EVEN_PARITY(0);
        break;
        }
    }

    if (argc > 4) {
        if (get_int_val_from_str(argv[4], &tmp_int1, POL_1) != 0) {
            err_code = AT_CMD_ERR_UART_STOPBIT;
            goto atcmd_uartconf_end;
        }

        stopbit = tmp_int1;

        if (!(stopbit == 1 || stopbit == 2)) {
            err_code = AT_CMD_ERR_UART_STOPBIT;
            goto atcmd_uartconf_end;
        }
    }

#if defined ( __UART1_FLOW_CTRL_ON__ )
    if (argc > 5) {
        if (get_int_val_from_str(argv[5], &tmp_int1, POL_1) != 0) {
            err_code = AT_CMD_ERR_UART_FLOWCTRL;
            goto atcmd_uartconf_end;
        }
        flow_ctrl = tmp_int1;
        
        if (!(flow_ctrl == 0 || flow_ctrl == 1) ) {
            err_code = AT_CMD_ERR_UART_FLOWCTRL;
            goto atcmd_uartconf_end;
        }
    }
#else
    flow_ctrl = UART_FLOWCTL_OFF;
#endif /* __UART1_FLOW_CTRL_ON__ */

    /* Notice to uart1 terminal before change the configuration */
    PRINTF_ATCMD("\r\nOK\r\n");

    vTaskDelay(10);

    clock = 40000000 * 2;

    UART_IOCTL(atcmd_uart, UART_SET_CLOCK, &clock);
    UART_CHANGE_BAUERATE(atcmd_uart, baud);

    temp = UART_WORD_LENGTH(bits) | UART_FIFO_ENABLE(fifo_en) | parity_temp | UART_STOP_BIT(stopbit);
    UART_IOCTL(atcmd_uart, UART_SET_LINECTRL, &temp);

    temp = UART_RECEIVE_ENABLE(1) | UART_TRANSMIT_ENABLE(1) | UART_HWFLOW_ENABLE(flow_ctrl);
    UART_IOCTL(atcmd_uart, UART_SET_CONTROL, &temp);

    temp = UART_RX_INT_LEVEL(UART_ONEEIGHTH) | UART_TX_INT_LEVEL(UART_SEVENEIGHTHS);
    UART_IOCTL(atcmd_uart, UART_SET_FIFO_INT_LEVEL, &temp);

    temp = DMA;
    UART_IOCTL(atcmd_uart, UART_SET_USE_DMA, &temp);

    /* FIFO Enable */
    temp = UART_INTBIT_RECEIVE | UART_INTBIT_TRANSMIT | UART_INTBIT_TIMEOUT | UART_INTBIT_ERROR
           | UART_INTBIT_FRAME | UART_INTBIT_PARITY | UART_INTBIT_BREAK | UART_INTBIT_OVERRUN;

    UART_IOCTL(atcmd_uart, UART_SET_INT, &temp);

    temp = swflow;
    UART_IOCTL(atcmd_uart, UART_SET_SW_FLOW_CONTROL, &temp);

    temp = word_access;
    UART_IOCTL(atcmd_uart, UART_SET_WORD_ACCESS, &temp);

    temp = rw_word;
    UART_IOCTL(atcmd_uart, UART_SET_RW_WORD, &temp);

    /* Save new values in NVRAM */
    if (write_nvram_int((const char *)NVR_KEY_ATCMD_UART_BAUDRATE, baud) != 0) {
        err_code = AT_CMD_ERR_UART_BAUDRATE_NV_WR;
        goto atcmd_uartconf_end;
    }

    if (write_nvram_int((const char *)NVR_KEY_ATCMD_UART_BITS, bits) != 0) {
        err_code = AT_CMD_ERR_UART_DATABITS_NV_WR;
        goto atcmd_uartconf_end;
    }

    if (write_nvram_int((const char *)NVR_KEY_ATCMD_UART_PARITY, parity_temp) != 0) {
        err_code = AT_CMD_ERR_UART_PARITY_NV_WR;
        goto atcmd_uartconf_end;
    }

    if (write_nvram_int((const char *)NVR_KEY_ATCMD_UART_STOPBIT, stopbit) != 0) {
        err_code = AT_CMD_ERR_UART_STOPBIT_NV_WR;
        goto atcmd_uartconf_end;
    }

    if (write_nvram_int((const char *)NVR_KEY_ATCMD_UART_FLOWCTRL, flow_ctrl) != 0) {
        err_code = AT_CMD_ERR_UART_FLOWCTRL_NV_WR;
        goto atcmd_uartconf_end;
    }

    if (dpm_mode_is_enabled() == DPM_ENABLED) {
        UINT    status;

        rtm_len = dpm_user_mem_get(ATCMD_UART_CONF_NAME, (UCHAR **)&atcmd_uart_info);

        if (rtm_len == 0) {
            status = dpm_user_mem_alloc(ATCMD_UART_CONF_NAME, (VOID **)&atcmd_uart_info, sizeof(uart_info_t), 100);

            if (status != ERR_OK) {
                PRINTF("[%s] Failed to allocate RTM area !!!\n", __func__);

                err_code = AT_CMD_ERR_DPM_USER_RTM_ALLOC;
                goto atcmd_uartconf_end;
            }

            memset(atcmd_uart_info, 0x00, sizeof(uart_info_t));

        } else if (rtm_len != sizeof(uart_info_t)) {
            PRINTF("[%s] Invalid RTM alloc size (%d)\n", __func__, rtm_len);

            err_code = AT_CMD_ERR_DPM_USER_RTM_DUP;
            goto atcmd_uartconf_end;
        }

        // Write configuration into user RTM area.
        atcmd_uart_info->baud      = baud;
        atcmd_uart_info->bits      = bits;
        atcmd_uart_info->parity    = parity;
        atcmd_uart_info->stopbit   = stopbit;
        atcmd_uart_info->flow_ctrl = flow_ctrl;
    }

atcmd_uartconf_end :
    return err_code;
}
#endif // ! __USER_UART_CONFIG__

int atcmd_socket_send(char *buf)
{
    atcmd_error_code status;

    status = esc_cmd_parser(buf);

    return status;
}

/******************************************************************************
 * Function Name : esc_cmd_parser
 * Discription : esc_cmd_parser
 * Arguments : line
 * Return Value : result_code
 *****************************************************************************/
#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
atcmd_error_code esc_cmd_parser(char *line)
{
    char      *esc_params[ESC_CMD_MAX_PARAMS];
    uint32_t   param_cnt = 0;
    atcmd_error_code    result_code = AT_CMD_ERR_CMD_OK;
    int   cid = 0;
    int   data_len;
    int   is_m_cmd = pdFALSE;
    char  *ip_addr;
    int   peer_port;

    atcmd_sess_context *ctx = NULL;
    atcmd_tcps_context *tcps_ctx = NULL;
    atcmd_tcpc_context *tcpc_ctx = NULL;
    atcmd_udps_context *udps_ctx = NULL;

    if (line[0] == ESCAPE_CODE) {
        if (strstr(line, ",") == NULL) {
            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto atcmd_esc_end;
        }

        if (strlen(line) < 5) {
            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto atcmd_esc_end;
        }

        switch (line[1]) {
        case esc_cmd_s_upper_case:
        case esc_cmd_s_lower_case:
        case esc_cmd_m_upper_case:
        case esc_cmd_m_lower_case:
        case esc_cmd_h_upper_case:
        case esc_cmd_h_lower_case:
        case 'B':
        case 'b': {
          if ((line[1] == esc_cmd_m_upper_case || line[1] == esc_cmd_m_lower_case)
                  || (line[1] == esc_cmd_h_upper_case || line[1] == esc_cmd_h_lower_case)) {
                is_m_cmd = pdTRUE;

                char *str_cid = strtok(line + 2, ",");    // CID

                if (get_int_val_from_str(str_cid, &cid, POL_1) != 0) {
                    ATCMD_DBG("[%s:%d]Failed to get CID\n", __func__, __LINE__);
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }
            } else {
                cid = line[2] - 0x30;
            }

            ctx = atcmd_transport_find_context(cid);
            if (!ctx) {
                ATCMD_DBG("[%s:%d]Not found cid(%d)\n", __func__, __LINE__, cid);
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                goto atcmd_esc_end;
            }

            if (ctx->type == ATCMD_SESS_TCP_SERVER) {
                tcps_ctx = ctx->ctx.tcps;
            } else if (ctx->type == ATCMD_SESS_TCP_CLIENT) {
                tcpc_ctx = ctx->ctx.tcpc;
            } else if (ctx->type == ATCMD_SESS_UDP_SESSION) {
                udps_ctx = ctx->ctx.udps;
            }

            if (   (ctx->type == ATCMD_SESS_TCP_SERVER)
                && (tcps_ctx->state == ATCMD_TCPS_STATE_ACCEPT)) {
                if (tcps_ctx->cli_cnt > 0) {
                    if (is_m_cmd) {
                        // <ESC>M<cid>,<data length>,0.0.0.0,19999,1234567890
                        esc_params[param_cnt++] = strtok(NULL, ",");    // data length
                        if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                            goto atcmd_esc_end;
                        }
                    } else {
                        /* <ESC>S010,0.0.0.0,19999,1234567890 */
                        esc_params[param_cnt++] = strtok(line + 3, ",");    /* DATA Length */
                        if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                            goto atcmd_esc_end;
                        }
                    }

                    ip_addr = strtok(NULL, ",");            /* IP address */
                    if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }

                    /* Move current point to start address of DATA payload */
                    if (is_m_cmd) {
                        while( *line != 0) {line++;} line++; // Skip cid
                    }
                    while( *line != 0) {line++;} line++; // Skip data size
                    while( *line != 0) {line++;} line++; // Skip IP address
                    while( *line != 0) {line++;} line++; // Skip port number

                    esc_params[param_cnt++] = line;            /* Data */

                    if (atoi(esc_params[0]) == 0) {
                        data_len = strlen(esc_params[1]);
                    }

                    result_code = atcmd_tcps_tx(tcps_ctx, esc_params[1], data_len, ip_addr, peer_port);
                } else {
                    result_code = AT_CMD_ERR_UNKNOWN;
                }
            } else if ((ctx->type == ATCMD_SESS_TCP_CLIENT) && (tcpc_ctx->state == ATCMD_TCPC_STATE_CONNECTED)) {

                if (is_m_cmd) {
                    // <ESC>M<cid>,<data_length>,0,0,1234567890 */
                    esc_params[param_cnt++]    = strtok(NULL, ",");    // data length
                    if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }
                } else {
                    /* <ESC>S110,0,0,1234567890 */
                    esc_params[param_cnt++]    = strtok(line + 3, ",");    /* DATA Length */
                    if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }
                }

                ip_addr = strtok(NULL, ",");
                if (ip_addr == NULL) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                /* Move current point to start address of DATA payload */
                if (is_m_cmd) {
                    while( *line != 0) {line++;} line++; // Skip cid
                }
                while( *line != 0) {line++;} line++;
                while( *line != 0) {line++;} line++;
                while( *line != 0) {line++;} line++;

                esc_params[param_cnt++] = line;            /* Data */

                if (atoi(esc_params[0]) == 0) {
                    data_len = strlen(esc_params[1]);
                }

                result_code = atcmd_tcpc_tx_with_peer_info(tcpc_ctx,
                                                           ip_addr, peer_port,
                                                           esc_params[1], data_len);
            } else if ((ctx->type == ATCMD_SESS_UDP_SESSION) && (udps_ctx->state == ATCMD_UDPS_STATE_ACTIVE)) {
                if (is_m_cmd) {
                    // <ESC>M<cid>,<data_length>,0.0.0.0,19999,1234567890
                    esc_params[param_cnt++] = strtok(NULL, ",");    // data Lengt
                    if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }
                } else {
                    /* <ESC>S210,0.0.0.0,19999,1234567890 */
                    esc_params[param_cnt++] = strtok(line + 3, ",");    // DATA Length
                    if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }
                }

                ip_addr = strtok(NULL, ",");                        // IP address
                if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                /* Move current point to start address of DATA payload */
                if (is_m_cmd) {
                    while( *line != 0) {line++;} line++; // Skip cid
                }
                while( *line != 0) {line++;} line++; // Skip data size
                while( *line != 0) {line++;} line++; // Skip IP address
                while( *line != 0) {line++;} line++; // Skip port number

                esc_params[param_cnt++] = line;                        // Data

                if (atoi(esc_params[0]) == 0) {
                    data_len = strlen(esc_params[1]);
                }

                result_code = atcmd_udps_tx(udps_ctx, esc_params[1], data_len, ip_addr, peer_port);
            } else {
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } break;

        case 'C':
        case 'c': {
            if (strncasecmp(esc_cmd_cert_case, &(line[1]), strlen(esc_cmd_cert_case)) == 0) {
                //<ESC>CERT,<module>,<certificate type>,<mode>[,<format>,<length>,<content>]

                int cert_err = DA16X_CERT_ERR_OK;
                int module = DA16X_CERT_MODULE_NONE;
                int type = DA16X_CERT_TYPE_NONE;
                int mode = DA16X_CERT_MODE_NONE;
                int format = DA16X_CERT_FORMAT_NONE;
                int length = 0;

                char *ptr = NULL;

                //Skipped
                strtok(line + 2, ",");

                //Module
                ptr = strtok(NULL, ",");
                if (ptr && (get_int_val_from_str(ptr, &module, POL_1) != 0)) {
                    ATCMD_DBG("[%s:%d]Failed to get module\n", __func__, __LINE__);
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                //Type
                ptr = strtok(NULL, ",");
                if (ptr && (get_int_val_from_str(ptr, &type, POL_1) != 0)) {
                    ATCMD_DBG("[%s:%d]Failed to get type\n", __func__, __LINE__);
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    break;
                }

                //Mode
                ptr = strtok(NULL, ",");
                if (ptr && (get_int_val_from_str(ptr, &mode, POL_1) != 0)) {
                    ATCMD_DBG("[%s:%d]Failed to get mode\n", __func__, __LINE__);
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    break;
                }

                if (mode == DA16X_CERT_MODE_STORE) {
                    //Format
                    ptr = strtok(NULL, ",");
                    if (ptr && (get_int_val_from_str(ptr, &format, POL_1) != 0)) {
                        ATCMD_DBG("[%s:%d]Failed to get format\n", __func__, __LINE__);
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        break;
                    }

                    //Length
                    ptr = strtok(NULL, ",");
                    if (ptr && (get_int_val_from_str(ptr, &length, POL_1) != 0)) {
                        ATCMD_DBG("[%s:%d]Failed to get length\n", __func__, __LINE__);
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        break;
                    }

                    //content
                    ptr = strtok(NULL, ",");
                    if (ptr == NULL) {
                        ATCMD_DBG("[%s:%d]Failed to get content\n", __func__, __LINE__);
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        break;
                    }

                    //Store certificate
                    cert_err = da16x_cert_write(module, type, format, (unsigned char *)ptr, (size_t)length);
                } else if (mode == DA16X_CERT_MODE_DELETION) {
                    //Delete certificate
                    cert_err = da16x_cert_delete(module, type);
                }

                //convert error code
                switch (cert_err) {
                case DA16X_CERT_ERR_OK:
                    result_code = AT_CMD_ERR_CMD_OK;
                    break;
                case DA16X_CERT_ERR_INVALID_MODULE:
                    result_code = AT_CMD_ERR_SSL_CERT_MODULE;
                    break;
                case DA16X_CERT_ERR_INVALID_TYPE:
                    result_code = AT_CMD_ERR_SSL_CERT_TYPE;
                    break;
                case DA16X_CERT_ERR_INVALID_FORMAT:
                    result_code = AT_CMD_ERR_SSL_CERT_FORMAT;
                    break;
                case DA16X_CERT_ERR_INVALID_LENGTH:
                    result_code = AT_CMD_ERR_SSL_CERT_LENGTH;
                    break;
                case DA16X_CERT_ERR_INVALID_FLASH_ADDR:
                    result_code = AT_CMD_ERR_SSL_CERT_FLASH_ADDR;
                    break;
                case DA16X_CERT_ERR_INVALID_PARAMS:
                    result_code = AT_CMD_ERR_INSUFFICENT_ARGS;
                    break;
                case DA16X_CERT_ERR_FOPEN_FAILED:
                    result_code = AT_CMD_ERR_SFLASH_ACCESS;
                    break;
                case DA16X_CERT_ERR_MEM_FAILED:
                    result_code = AT_CMD_ERR_MEM_ALLOC;
                    break;
                case DA16X_CERT_ERR_EMPTY_CERTIFICATE:
                    result_code = AT_CMD_ERR_SSL_CERT_EMPTY_CERT;
                    break;
                case DA16X_CERT_ERR_NOK:
                default:
                    result_code = AT_CMD_ERR_SSL_CERT_INTERNAL;
                    break;
                }
            } else {
                cid = line[2] - 0x30; /* CertID (0~2) */
                size_t payload_len = strlen(line);

                /* Check minimum and maximum size of the certificate */
                if ((payload_len > 20) && ((payload_len - 4) < CERT_MAX_LENGTH)) {
                    /* <ESC>C0,----- BEGIN CERT... */
                    if (line[3] != ',') {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    } else {
                        int sflash_addr;

                        switch (cid) {
                        case 0: // MQTT - CA1
                            sflash_addr = SFLASH_ROOT_CA_ADDR1;
                            break;
                        case 1: // MQTT - Cert1
                            sflash_addr = SFLASH_CERTIFICATE_ADDR1;
                            break;
                        case 2: // MQTT Private Key1
                            sflash_addr = SFLASH_PRIVATE_KEY_ADDR1;
                            break;
                        case 3: // HTTPS - CA2
                            sflash_addr = SFLASH_ROOT_CA_ADDR2;
                            break;
                        case 4: // HTTPS - Cert2
                            sflash_addr = SFLASH_CERTIFICATE_ADDR2;
                            break;
                        case 5: // HTTPS - Private Key2
                            sflash_addr = SFLASH_PRIVATE_KEY_ADDR2;
                            break;
                        case 6: // WPA Enterprise - CA3
                            sflash_addr = SFLASH_ENTERPRISE_ROOT_CA;
                            break;
                        case 7: // WPA Enterprise - Cert3
                            sflash_addr = SFLASH_ENTERPRISE_CERTIFICATE;
                            break;
                        case 8: // WPA Enterprise - Private Key3
                            sflash_addr = SFLASH_ENTERPRISE_PRIVATE_KEY;
                            break;
                        default:
                            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                            break;
                        }

                        if (result_code == AT_CMD_ERR_CMD_OK) {
                            int ret;

                            ret = cert_flash_write(sflash_addr, line + 4, CERT_MAX_LENGTH);
                            if (ret) {
                                result_code = AT_CMD_ERR_SFLASH_WRITE;
                            }
                        }
                    }
                } else {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                }
            }
        } break;

        default: {
            result_code = AT_CMD_ERR_UNKNOWN_CMD;
        } break;

        } // End of switch

    } else {
        result_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_esc_end:
    if (result_code == AT_CMD_ERR_CMD_OK) {
#if defined (__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
        printf_with_run_time("AT-CMD <esc> OK");
#endif // (__RUNTIME_CALCULATION__)

#if defined (__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
        PRINTF_ESCCMD(AT_CMD_ERR_CMD_OK);
#else
        PRINTF_ATCMD("\r\nOK\r\n");
#endif // (__ATCMD_IF_SPI__) || (__ATCMD_IF_SDIO__)
    } else {
#if defined (__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
        PRINTF_ESCCMD(result_code);
#else
        PRINTF_ATCMD("\r\nERROR:%d\r\n", result_code);
#endif // (__ATCMD_IF_SPI__) || (__ATCMD_IF_SDIO__)
    }

    return result_code;
}

#else

atcmd_error_code esc_cmd_parser(char *line)
{
    int   status = 0;
    char  *esc_params[ESC_CMD_MAX_PARAMS];
    uint32_t param_cnt = 0;
    atcmd_error_code    result_code = AT_CMD_ERR_CMD_OK;
    int  cid = 0;
    int  data_len;
    char *ip_addr;
    int  peer_port;

    if (line[0] == ESCAPE_CODE) {
        if (strstr(line, ",") == NULL) {
            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto atcmd_esc_end;
        }

        if (strlen(line) < 5) {
            result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto atcmd_esc_end;
        }

        switch (line[1]) {
        case 'S':
        case 's':
        case 'B':
        case 'b': {
            cid = line[2] - 0x30;

            if ((cid < ID_TS) || (cid > ID_US)) {
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                ATCMD_DBG("Command error : %s \n", &line[1]);
                break;
            }

            if ((cid == ID_TS)
                && (atcmd_tcps_ctx.state == ATCMD_TCPS_STATE_ACCEPT)) {
                if (atcmd_tcps_ctx.cli_cnt > 0) {
                    /* <ESC>S010,0.0.0.0,19999,1234567890 */
                    esc_params[param_cnt++] = strtok(line + 3, ",");    /* DATA Length */
                    if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }

                    ip_addr = strtok(NULL, ",");            /* IP address */
                    if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                        goto atcmd_esc_end;
                    }

                    /* Move current point to start address of DATA payload */
                    while( *line != 0) {line++;} line++; // Skip data size
                    while( *line != 0) {line++;} line++; // Skip IP address
                    while( *line != 0) {line++;} line++; // Skip port number

                    esc_params[param_cnt++] = line;            /* Data */

                    if (atoi(esc_params[0]) == 0) {
                        data_len = strlen(esc_params[1]);
                    }

                    status = atcmd_tcps_tx(&atcmd_tcps_ctx, esc_params[1], data_len, ip_addr, peer_port);
                    if (status == -2) {
                        result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    } else if (status) {
                        result_code = AT_CMD_ERR_UNKNOWN;
                    } else {
                        result_code = AT_CMD_ERR_CMD_OK;
                    }
                } else {
                    result_code = AT_CMD_ERR_UNKNOWN;
                }
            } else if ((cid == ID_TC)
                    && (atcmd_tcpc_ctx.state == ATCMD_TCPC_STATE_CONNECTED)) {
                /* <ESC>S110,0,0,1234567890 */
                esc_params[param_cnt++]    = strtok(line + 3, ",");    /* DATA Length */
                if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                ip_addr = strtok(NULL, ",");
                if (ip_addr == NULL) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                /* Move current point to start address of DATA payload */
                while( *line != 0) {line++;} line++;
                while( *line != 0) {line++;} line++;
                while( *line != 0) {line++;} line++;

                esc_params[param_cnt++] = line;            /* Data */

                if (atoi(esc_params[0]) == 0) {
                    data_len = strlen(esc_params[1]);
                }

                status = atcmd_tcpc_tx_with_peer_info(&atcmd_tcpc_ctx, ip_addr, peer_port, esc_params[1], data_len);
                if (status == -2) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                } else if (status) {
                    result_code = AT_CMD_ERR_UNKNOWN;
                } else {
                    result_code = AT_CMD_ERR_CMD_OK;
                }
            } else if ((cid == ID_US) && (atcmd_udps_ctx.state == ATCMD_UDPS_STATE_ACTIVE)) {
                /* <ESC>S210,0.0.0.0,19999,1234567890 */
                esc_params[param_cnt++] = strtok(line + 3, ",");    // DATA Length
                if (get_int_val_from_str(esc_params[param_cnt - 1], &data_len, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                ip_addr = strtok(NULL, ",");                        // IP address
                if (get_int_val_from_str(strtok(NULL, ","), &peer_port, POL_1) != 0) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto atcmd_esc_end;
                }

                /* Move current point to start address of DATA payload */
                while( *line != 0) {line++;} line++; // Skip data size
                while( *line != 0) {line++;} line++; // Skip IP address
                while( *line != 0) {line++;} line++; // Skip port number

                esc_params[param_cnt++] = line;                        // Data

                if (atoi(esc_params[0]) == 0) {
                    data_len = strlen(esc_params[1]);
                }

                status = atcmd_udps_tx(&atcmd_udps_ctx, esc_params[1], data_len, ip_addr, peer_port);

                if (status == -2) {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                } else if (status) {
                    result_code = AT_CMD_ERR_UNKNOWN;
                    ATCMD_DBG("status=%d\n", status);
                } else {
                    result_code = AT_CMD_ERR_CMD_OK;
                }
            } else {
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            }
        } break;

        case 'C':
        case 'c': {
            cid = line[2] - 0x30; /* CertID (0~2) */

            if (strlen(line) > 20) {
                /* <ESC>C0,----- BEGIN CERT... */
                if (line[3] != ',') {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                } else if (cid == 0) {    // ca1
                    cert_flash_write(SFLASH_ROOT_CA_ADDR1, line + 4, CERT_MAX_LENGTH);
                } else if (cid == 1) {    // cert1
                    cert_flash_write(SFLASH_CERTIFICATE_ADDR1, line + 4, CERT_MAX_LENGTH);
                } else if (cid == 2) {    // key1
                    cert_flash_write(SFLASH_PRIVATE_KEY_ADDR1, line + 4, CERT_MAX_LENGTH);
                } else if (cid == 3) {    // ca2
                    cert_flash_write(SFLASH_ROOT_CA_ADDR2, line + 4, CERT_MAX_LENGTH);
                } else if (cid == 4) {    // cert2
                    cert_flash_write(SFLASH_CERTIFICATE_ADDR2, line + 4, CERT_MAX_LENGTH);
                } else if (cid == 5) {    // key2
                    cert_flash_write(SFLASH_PRIVATE_KEY_ADDR2, line + 4, CERT_MAX_LENGTH);
                } else {
                    result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                }
            } else {
                result_code = AT_CMD_ERR_WRONG_ARGUMENTS;
            }

        } break;

        default: {
            result_code = AT_CMD_ERR_UNKNOWN_CMD;
        } break;

        } // End of switch
    } else {
        result_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

atcmd_esc_end:
    if (result_code == AT_CMD_ERR_CMD_OK) {
#if defined (__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
        printf_with_run_time("AT-CMD <esc> OK");
#endif // (__RUNTIME_CALCULATION__)

#if defined (__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
        PRINTF_ESCCMD(AT_CMD_ERR_CMD_OK);
#else
        PRINTF_ATCMD("\r\nOK\r\n");
#endif // (__ATCMD_IF_SPI__) || (__ATCMD_IF_SDIO__)
    } else {
#if defined (__ATCMD_IF_SPI__) || defined(__ATCMD_IF_SDIO__)
        PRINTF_ESCCMD(result_code);
#else
        PRINTF_ATCMD("\r\nERROR:%d\r\n", result_code);
#endif // (__ATCMD_IF_SPI__) || (__ATCMD_IF_SDIO__)
    }

    return result_code;
}
#endif // __SUPPORT_ATCMD_MULTI_SESSION__

#if defined (__SUPPORT_DPM_EXT_WU_MON__)
void dpm_ext_wu_mon(void * pvParameters)
{
    DA16X_UNUSED_ARG(pvParameters);

    int sys_wdog_id = -1;
    char    dpm_wu_str[8] = { 0, };
    char    *tmp_str = NULL;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    da16x_sys_watchdog_notify(sys_wdog_id);

    /* Check DPM Wakeup type */
    tmp_str = atcmd_dpm_wakeup_status();
    if (tmp_str == NULL) {
        /* The dpm is registered after this task is created.
         * dpm_app_unregister() function may be called before registeration.
         */
        vTaskDelay(10);
        goto end;
    }

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    strcpy(dpm_wu_str, tmp_str);
#if defined (__SUPPORT_NOTIFY_RTC_WAKEUP__)
    vTaskDelay(DELAY_TICK_TO_RCV_AT_CMD);
#else
    if (strcmp(dpm_wu_str, "EXT") == 0) {    // External Wakeup
        /* Wait until to receive AT-CMD */
        vTaskDelay(DELAY_TICK_TO_RCV_AT_CMD);
#if defined ( __SUPPORT_UC_WU_MON__ )
    } else if (strcmp(dpm_wu_str, "UC") == 0) { // Unicast Rx Wakeup
        /* Wait until to receive AT-CMD */
        vTaskDelay(DELAY_TICK_TO_RCV_AT_CMD);
#endif // __SUPPORT_UC_WU_MON__
    } else {
        goto end;
    }
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    /* Check sleep state */
    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

#if defined ( __SUPPORT_NOTIFY_RTC_WAKEUP__ )
        if (rtc_wakeup_intr_chk_flag == pdTRUE) {
            rtc_wakeup_intr_chk_flag = pdFALSE;

            if (dpm_mode_is_wakeup() == pdFALSE) {
                PRINTF_ATCMD("\r\n+RUN:POR\r\n");
            } else {
                PRINTF_ATCMD("\r\n+RUN:RTCWAKEUP\r\n");
            }
        }
#endif // __SUPPORT_NOTIFY_RTC_WAKEUP__
        if (ext_wu_sleep_flag == pdFALSE) {
            dpm_app_sleep_ready_clear(APP_EXT_WU_MON);
        } else {
            dpm_app_sleep_ready_set(APP_EXT_WU_MON);
        }

        vTaskDelay(1);
    }

end:

    dpm_app_unregister(APP_EXT_WU_MON);

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return ;
}
#endif // (__SUPPORT_DPM_EXT_WU_MON__)

#if defined (__SUPPORT_ATCMD_TLS__)
static int atcmd_transport_ssl_send_tls_client(int cid, char *dst_ip, char *dst_port, char *data, int datalen)
{
    int status = DA_APP_SUCCESS;

    atcmd_tls_context *ctx = NULL;
    atcmd_tlsc_context *tlsc_ctx = NULL;
    atcmd_tlsc_config *tlsc_conf = NULL;

    int port = 0;

    ctx = atcmd_transport_ssl_find_context(cid);
    if (!ctx || ctx->role != ATCMD_TLS_CLIENT) {
        ATCMD_DBG("[%s:%d]Not found(cid:%d)\n", __func__, __LINE__, cid);
        return DA_APP_NOT_FOUND;
    }

    tlsc_ctx = ctx->ctx.tlsc_ctx;
    tlsc_conf = &(ctx->conf.tlsc_conf);

    //check state of tls session
    if (tlsc_ctx->state != ATCMD_TLSC_STATE_CONNECTED) {
        ATCMD_DBG("[%s:%d]Not connected(%d)\n", __func__, __LINE__, tlsc_ctx->state);
        return DA_APP_NOT_CONNECTED;
    }

    if (dst_ip) {
        //check ip address & port
        if (   !is_in_valid_ip_class(dst_ip)
            || (tlsc_conf->svr_addr != (unsigned long)iptolong(dst_ip))
            || (get_int_val_from_str(dst_port, &port, POL_1) != 0)
            || (tlsc_conf->svr_port != (unsigned int)port)) {
            ATCMD_DBG("[%s:%d]Not matched tls server info(%s:%s vs %d.%d.%d.%d:%d)\n",
                    __func__, __LINE__,
                    dst_ip, dst_port,
                    (tlsc_conf->svr_addr >> 24) & 0xff,
                    (tlsc_conf->svr_addr >> 16) & 0xff,
                    (tlsc_conf->svr_addr >>  8) & 0xff,
                    (tlsc_conf->svr_addr      ) & 0xff,
                    tlsc_conf->svr_port);

            return DA_APP_NOT_SUCCESSFUL;
        }
    }

    status = atcmd_tlsc_write_data(tlsc_ctx, (unsigned char *)data, datalen);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to send data(0x%x)\n", __func__, __LINE__);
    }

    return status;
}

static int atcmd_transport_ssl_do_wr(char *cid_str, char *dst_ip_str, char *dst_port_str, char *datalen_str, char *data)
{
    int status = DA_APP_SUCCESS;

    int cid = -1;
    int datalen = 0;

    // Check cid
    if (get_int_val_from_str(cid_str, &cid, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get CID(%s)\n", __func__, __LINE__, cid_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    // Check data length
    if (get_int_val_from_str(datalen_str, &datalen, POL_1) != 0) {
        ATCMD_DBG("[%s:%d]failed to get data lenght(%s)\n", __func__, __LINE__, datalen_str);
        return DA_APP_INVALID_PARAMETERS;
    }

    status = atcmd_transport_ssl_send_tls_client(cid, dst_ip_str, dst_port_str, data, datalen);
    if (status) {
        ATCMD_DBG("[%s:%d]failed to send data(%d,%ld,0x%x)\n", __func__, __LINE__, cid, datalen, -status);
    }

    return status;
}

static atcmd_error_code command_parser_tls_wr(char *line)
{
    atcmd_error_code error = AT_CMD_ERR_CMD_OK;

    const command_t    *cmd_ptr = NULL;
    char *params[MAX_PARAMS] = {0x00, };
    unsigned int param_cnt = 0;
    int data_len = 0;

#if defined (__ATCMD_IF_SPI__) || defined (__ATCMD_IF_SDIO__)
    at_command_table = commands;
#endif // __ATCMD_IF_SPI__ || __ATCMD_IF_SDIO__

    // First call to strtok
    if (strchr(line, '=') != NULL) {
        params[param_cnt++] = strtok(line, DELIMIT_FIRST);
    } else {
        ATCMD_DBG("[%s:%d]failed to find delimit(%s)\n", __func__, __LINE__, DELIMIT_FIRST);
        return AT_CMD_ERR_UNKNOWN_CMD;
    }

    // Find the command (AT+TRSSLWR)
    for (cmd_ptr = at_command_table ; cmd_ptr->name != NULL ; cmd_ptr++) {
        if (strcasecmp(params[0], cmd_ptr->name) == 0) {
            break;
        }
    }

    if (cmd_ptr == NULL || cmd_ptr->command == NULL) {
        ATCMD_DBG("[%s:%d]failed to find command\n", __func__, __LINE__);
        return AT_CMD_ERR_UNKNOWN_CMD;
    }

    //Get CID
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get CID\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //Get Data length or Server IP address
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get 2nd param\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    //Is integer type(data length)?
    if ((get_int_val_from_str(params[param_cnt], &data_len, POL_1) != 0) || (data_len <= 0)) {
        param_cnt++;

        //Get Server port
        params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
        if (!params[param_cnt]) {
            ATCMD_DBG("[%s:%d]Failed to get server port\n", __func__, __LINE__);
            error = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto end;
        }
        param_cnt++;

        //Get data length
        params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
        if (!params[param_cnt]) {
            ATCMD_DBG("[%s:%d]Failed to get data length\n", __func__, __LINE__);
            error = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto end;
        }
    }
    param_cnt++;

    //Get data
    params[param_cnt] = (params[param_cnt - 1] + strlen(params[param_cnt - 1]) + 1);
    param_cnt++;

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]parameter cnt(%d)\n", __func__, __LINE__, param_cnt);
    for (unsigned int x = 0 ; x < (param_cnt - 1) ; x++) {
        if (param_cnt == x) {
            ATCMD_DBG("#%d. data len:%ld\n", x + 1, params[x]);
        } else {
            ATCMD_DBG("#%d. %s(%ld)\n", x + 1, params[x]);
        }
    }
#endif // (DEBUG_ATCMD)

    if (param_cnt == 3) {    // AT+TRSSLWR=<cid>,<length>,<data>
        // check data length
        if ((get_int_val_from_str(params[2], &data_len, POL_1) != 0) || (data_len <= 0)) {
            ATCMD_DBG("[%s:%d]failed to get data length(%s)\n", __func__, __LINE__, params[2]);
            error = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto end;
        }

        params[3] = (params[2] + strlen(params[2]) + 1);

        error = atcmd_transport_ssl_do_wr(params[1], NULL, NULL, params[2], params[3]);
        if (error) {
            ATCMD_DBG("[%s:%d]failed to progress WR(0x%x)\n", __func__, __LINE__, error);
            error = AT_CMD_ERR_UNKNOWN;
            goto end;
        }

        error = AT_CMD_ERR_CMD_OK;
    } else if (param_cnt == 4) {
        error = atcmd_transport_ssl_do_wr(params[1], NULL, NULL, params[2], params[3]);
        if (error) {
            ATCMD_DBG("[%s:%d]failed to progress WR(0x%x)\n", __func__, __LINE__, error);
            error = AT_CMD_ERR_UNKNOWN;
            goto end;
        }

        error = AT_CMD_ERR_CMD_OK;
    } else if (param_cnt == 5) {    // AT+TRSSLWR=<cid>,<ip>,<port>,<length>,<data>
        if ((get_int_val_from_str(params[4], &data_len, POL_1) != 0) || (data_len <= 0)) {
            ATCMD_DBG("[%s:%d]failed to get data length(%s)\n", __func__, __LINE__, params[4]);
            error = AT_CMD_ERR_WRONG_ARGUMENTS;
            goto end;
        }

        params[5] = (params[4] + strlen(params[4]) + 1);

        error = atcmd_transport_ssl_do_wr(params[1], params[2], params[3], params[4], params[5]);
        if (error) {
            ATCMD_DBG("[%s:%d]failed to progress WR(0x%x)\n", __func__, __LINE__, error);
            error = AT_CMD_ERR_UNKNOWN;
            goto end;
        }

        error = AT_CMD_ERR_CMD_OK;
    } else if (param_cnt == 6) {
        error = atcmd_transport_ssl_do_wr(params[1], params[2], params[3], params[4], params[5]);
        if (error) {
            ATCMD_DBG("[%s:%d]failed to progress WR(0x%x)\n", __func__, __LINE__, error);
            error = AT_CMD_ERR_UNKNOWN;
            goto end;
        }

        error = AT_CMD_ERR_CMD_OK;
    } else {
        ATCMD_DBG("[%s:%d]invalid parameter(%d)\n", __func__, __LINE__, param_cnt);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
    }

end :
    if (error == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\nOK\r\n");
    } else  {
        PRINTF_ATCMD("\r\nERROR:%d\r\n", error);
    }

    return error;
}

static atcmd_error_code command_parser_tls_certstore(char *line)
{
    atcmd_error_code error = AT_CMD_ERR_CMD_OK;

    const command_t *cmd_ptr = NULL;
    char *params[MAX_PARAMS] = {0x00, };
    unsigned int param_cnt = 0;

    int tmp_int1 = 0;

    int status = 0;
    char *data = NULL;
    int data_len = 0;
    int cert_format = ATCMD_CM_CERT_FOMRAT_PEM;

#if defined (__ATCMD_IF_SPI__) || defined (__ATCMD_IF_SDIO__)
    at_command_table = commands;
#endif // __ATCMD_IF_SPI__ || __ATCMD_IF_SDIO__

    // First call to strtok
    if (strchr(line, '=') != NULL) {
        params[param_cnt++] = strtok(line, DELIMIT_FIRST);
    } else {
        ATCMD_DBG("[%s:%d]failed to find delimit(%s)\n", __func__, __LINE__, DELIMIT_FIRST);
        return AT_CMD_ERR_UNKNOWN_CMD;
    }

    // Find the command (AT+TRSSLCERTSTORE)
    for (cmd_ptr = at_command_table ; cmd_ptr->name != NULL ; cmd_ptr++) {
        if (strcasecmp(params[0], cmd_ptr->name) == 0) {
            break;
        }
    }

    if (cmd_ptr == NULL || cmd_ptr->command == NULL) {
        ATCMD_DBG("[%s:%d]failed to find command\n", __func__, __LINE__);
        return AT_CMD_ERR_UNKNOWN_CMD;
    }

    //Get cert type
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get CID\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //Get sequence
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get sequence param\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //Get format
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get format param\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //parse cert format
    if (get_int_val_from_str(params[param_cnt - 1], &tmp_int1, POL_1) != 0) {
        error = AT_CMD_ERR_SSL_CERT_STO_FORMAT_TYPE;
        goto end;
    }

    cert_format = tmp_int1;

    //Get name
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt]) {
        ATCMD_DBG("[%s:%d]Failed to get name param\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //Get data length
    params[param_cnt] = strtok(NULL, DELIMIT_COMMA);
    if (!params[param_cnt] && cert_format != ATCMD_CM_CERT_FOMRAT_PEM) {
        ATCMD_DBG("[%s:%d]Failed to get data length param\n", __func__, __LINE__);
        error = AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    param_cnt++;

    //Get data
    params[param_cnt] = (params[param_cnt - 1] + strlen(params[param_cnt - 1]) + 1);
    param_cnt++;

#if defined (DEBUG_ATCMD)
    ATCMD_DBG("[%s:%d]parameter cnt(%d)\n", __func__, __LINE__, param_cnt);
    for (unsigned int x = 0 ; x < (param_cnt - 1) ; x++) {
        if (param_cnt == x) {
            ATCMD_DBG("#%d. data len:%ld\n", x + 1, params[x]);
        } else {
            ATCMD_DBG("#%d. %s(%ld)\n", x + 1, params[x], strlen(params[x]));
        }
    }
#endif // (DEBUG_ATCMD)

    if (param_cnt >= 6) {
        // Certificate-type
        if (get_int_val_from_str(params[1], &tmp_int1, POL_1) != 0) {
            error = AT_CMD_ERR_SSL_CERT_TYPE;
            goto end;
        }

        if (is_in_valid_range(tmp_int1, 0, 1) == pdFALSE) {
            error = AT_CMD_ERR_SSL_CERT_RANGE;
            goto end;
        }

        // Sequence
        if (get_int_val_from_str(params[2], &tmp_int1, POL_1) != 0) {
            error = AT_CMD_ERR_SSL_CERT_STO_SEQ_TYPE;
            goto end;
        }

        if (is_in_valid_range(tmp_int1, 0, 5) == pdFALSE) {
            error = AT_CMD_ERR_SSL_CERT_STO_SEQ_RANGE;
            goto end;
        }

        // Format
        if (is_in_valid_range(cert_format, 0, 1) == pdFALSE) {
            error = AT_CMD_ERR_SSL_CERT_STO_FORMAT_RANGE;
            goto end;
        }

        // Check data length
        if (get_int_val_from_str(params[5], &tmp_int1, POL_1) != 0) {
            //there is no data length
            if (cert_format == ATCMD_CM_CERT_FOMRAT_PEM) {
                data_len = (strlen(params[5]) + 1);
                data = params[5];
            } else  {
                error = AT_CMD_ERR_SSL_CERT_STO_FORMAT_TYPE;
                goto end;
            }
        } else {
            //there is data length.
            data_len = (tmp_int1 + 1);
            data = params[6];
        }

        error = AT_CMD_ERR_CMD_OK;

        status = atcmd_transport_ssl_do_certstore(params[1], params[2], params[3], params[4], data, data_len);
        switch (status) {
        case DA_APP_SUCCESS :
            error = AT_CMD_ERR_CMD_OK;
            break;

        case DA_APP_DUPLICATED_ENTRY :
            error = AT_CMD_ERR_SSL_CERT_STO_ALREADY_EXIST;
            break;

        case DA_APP_OVERFLOW :
            error = AT_CMD_ERR_SSL_CERT_STO_NO_SPACE;
            break;

        case DA_APP_NOT_CREATED :
            error = AT_CMD_ERR_MEM_ALLOC;
            break;

        default :
            error = AT_CMD_ERR_UNKNOWN;
            break;
        }
    } else {
        if (param_cnt < 6) {
            error = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    }

end :

    if (error == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\nOK\r\n");
    } else  {
        PRINTF_ATCMD("\r\nERROR:%d\r\n", error);
    }

    return error;
}
#endif // __SUPPORT_ATCMD_TLS__

void host_process_AT_command(UCHAR *buf)
{
#define ESCAPE_CODE        0x1B // ESC character

    int status = 0;

#if defined (__OTA_UPDATE_MCU_FW__)
    if (ota_update_atcmd_parser((char *)buf) == OTA_SUCCESS) {
        status = AT_CMD_ERR_CMD_OK;
    } else
#endif //(__OTA_UPDATE_MCU_FW__)
	if (strlen((char *)buf) > 0) {
		if (buf[0] == ESCAPE_CODE) {
#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
            printf_with_run_time("[host_process_AT_command] <ESC>");
#endif  // __RUNTIME_CALCULATION__

            esc_cmd_parser((char *)buf); /* DATA MODE */
        }
#if defined (__SUPPORT_ATCMD_TLS__)
        else if (strncmp((const char *)buf, ATCMD_TLS_WR_CMD, strlen(ATCMD_TLS_WR_CMD)) == 0) {
            status = command_parser_tls_wr((char *)buf);
            if (status == AT_CMD_ERR_UNKNOWN_CMD) {
                goto normal_command;
            }
        } else if (strncmp((const char *)buf, ATCMD_TLS_CERTSTORE_CMD, strlen(ATCMD_TLS_CERTSTORE_CMD)) == 0) {
            status = command_parser_tls_certstore((char *)buf);
            if (status == AT_CMD_ERR_UNKNOWN_CMD) {
                goto normal_command;
            }
        }
#endif // __SUPPORT_ATCMD_TLS__
        else {
#if defined (__SUPPORT_ATCMD_TLS__)
normal_command:
#endif // __SUPPORT_ATCMD_TLS__

#if defined( __RUNTIME_CALCULATION__ ) && defined( XIP_CACHE_BOOT )
            printf_with_run_time("[host_process_AT_command] AT-CMD \n");
#endif  // __RUNTIME_CALCULATION__

            status = command_parser((char *)buf); /* AT CMD */
            if (status == AT_CMD_ERR_UNKNOWN_CMD) {
                user_atcmd_parser((char *)buf);
            }
        }
    }
}

void _u2w_init(int iface)
{
    DA16X_UNUSED_ARG(iface);
}

#define ATCMD_DPM_TMP    "ATCMD_DPM_TEMP"

#if defined (__SUPPORT_ATCMD_MULTI_SESSION__)
void start_atcmd(void)
{
    int ret = 0;

    int dpm_resume_flag = pdFALSE;
    int dpm_wu_src;
    int dpm_wu_type;
    int is_reg_tmp_dpm = pdFALSE;

    atcmd_sess_info *sess_info = NULL;
    int sess_info_cnt = 0;

#if defined ( __TRIGGER_DPM_MCU_WAKEUP__ )
    // GPIO wake-up triggering
    trigger_mcu_wakeup_gpio();
#endif // __TRIGGER_DPM_MCU_WAKEUP__

    //Set max session cnt
    atcmd_transport_set_max_session(ATCMD_NW_TR_MAX_SESSION_CNT_DPM);

    if (check_net_init(WLAN0_IFACE) == ERR_OK) {
        _u2w_init(WLAN0_IFACE);
    } else if (check_net_init(WLAN1_IFACE) == ERR_OK) {
        _u2w_init(WLAN1_IFACE);
    }

    // Check and wait until User RTM initialize done...
    if (dpm_mode_is_enabled() && dpm_mode_is_wakeup() == pdFALSE) {
        while (dpm_user_mem_init_check() == pdFALSE) {
            vTaskDelay(1);
        }
    }

    dpm_wu_src = dpm_mode_get_wakeup_source();
    dpm_wu_type = dpm_mode_get_wakeup_type();

    if (dpm_wu_src == WAKEUP_EXT_SIG_WITH_RETENTION) {
        dpm_resume_flag = pdTRUE;
    } else if (   (dpm_wu_src == WAKEUP_COUNTER_WITH_RETENTION)
               && (dpm_wu_type == DPM_RTCTIME_WAKEUP || dpm_wu_type == DPM_PACKET_WAKEUP)) {
        dpm_resume_flag = pdTRUE;
    }

    sess_info_cnt = atcmd_network_get_sess_info_cnt();

    sess_info = atcmd_network_get_sess_info(0);
    if (!sess_info) {
        ATCMD_DBG("[%s:%d]Failed to get user session info\n", __func__, __LINE__);
    }

    if (dpm_mode_is_wakeup() == DPM_WAKEUP) {
        if (dpm_resume_flag == pdTRUE) {
#if defined (__SUPPORT_ATCMD_TLS__)
            atcmd_transport_ssl_recover_context_dpm();
#endif // (__SUPPORT_ATCMD_TLS__)
        }
    } else if (dpm_mode_is_enabled() == DPM_ENABLED) {
        if (sess_info) {
            atcmd_network_recover_context_nvram(sess_info, sess_info_cnt);
        }

#if defined (__SUPPORT_ATCMD_TLS__)
        atcmd_transport_ssl_recover_context_nvram();
#endif // (__SUPPORT_ATCMD_TLS__)
    }

    //is required to regiter temp dpm to wait
    if (dpm_mode_is_enabled() == DPM_ENABLED && dpm_mode_is_wakeup() != DPM_WAKEUP) {
        if (sess_info) {
            for (int cid = 0 ; cid < sess_info_cnt ; cid++) {
                if (sess_info[cid].type != ATCMD_SESS_NONE) {
                    is_reg_tmp_dpm = pdTRUE;
                    break;
                }
            }
        }
#if defined (__SUPPORT_ATCMD_TLS__)
        if (atcmd_tls_ctx_header) {
            is_reg_tmp_dpm = pdTRUE;
        }
#endif // __SUPPORT_ATCMD_TLS__

        if (is_reg_tmp_dpm) {
            ATCMD_DBG("[%s:%d]Register DPM(%s)\n", __func__, __LINE__, ATCMD_DPM_TMP);
            RM_PMGR_W_dpm_job_name_set(ATCMD_DPM_TMP, 0);
            dpm_app_sleep_ready_clear(ATCMD_DPM_TMP);
        }
    }

    ret = xTaskCreate(atcmd_task,
                      "AT-CMD",
#if defined (__SUPPORT_AWS_IOT__) || defined(__SUPPORT_AZURE_IOT__) //APP_CORE
                    (2048 / 4),
#else
                    (ATCMD_STACK_SIZE / 4),
#endif // (__SUPPORT_AWS_IOT__ || __SUPPORT_AZURE_IOT__)
                      (void *)NULL,
                      (OS_TASK_PRIORITY_USER + 6),
                      &atcmd_task_handler);
    if (ret != pdPASS) {
        ATCMD_DBG("[%s] ERROR(%d)\n", __func__, ret);

        if (sess_info) {
            APP_FREE(sess_info);
            sess_info = NULL;
        }
        return ;
    }

    // Recover tcp server, client and udp session in DPM
    if (dpm_mode_is_enabled() && sess_info) {
        atcmd_network_recover_session(sess_info, sess_info_cnt);
    } else {
        // Not required sess_info.
        atcmd_network_delete_sess_info();
    }

#if defined (__SUPPORT_ATCMD_TLS__)
    //Recover tls session
    atcmd_transport_ssl_recover_tls_session();
#endif // (__SUPPORT_ATCMD_TLS__)

    if (is_reg_tmp_dpm) {
        while (1) {
            if (chk_network_ready(0) == 1) {
                break;
            }
            vTaskDelay(10);
        }

        atcmd_network_display(0xFF);
        dpm_app_unregister(ATCMD_DPM_TMP);
    }

    return;
}

#else    //////////////////

void start_atcmd(void)
{
    int ret = 0;

    int dpm_resume_flag = pdFALSE;
    int dpm_wu_src;
    int dpm_wu_type;

    //tcp server
    unsigned int atcmd_tcps_run_flag = pdFALSE;
    int saved_atcmd_tcps_port = 0;

    //tcp client
    unsigned int atcmd_tcpc_run_flag = pdFALSE;
    char *saved_atcmd_tcpc_ip = NULL;
    unsigned int saved_atcmd_tcpc_lport = 0;
    unsigned int saved_atcmd_tcpc_rport = 0;

    //udp session
    unsigned int atcmd_udps_run_flag = pdFALSE;
    int saved_atcmd_udps_port = 0;

#if defined ( __TRIGGER_DPM_MCU_WAKEUP__ )
    // GPIO wake-up triggering
    trigger_mcu_wakeup_gpio();
#endif // __TRIGGER_DPM_MCU_WAKEUP__

    if (check_net_init(WLAN0_IFACE) == ERR_OK) {
        _u2w_init(WLAN0_IFACE);
    } else if (check_net_init(WLAN1_IFACE) == ERR_OK) {
        _u2w_init(WLAN1_IFACE);
    }

    // Check and wait until User RTM initialize done...
    if (dpm_mode_is_enabled() && dpm_mode_is_wakeup() == pdFALSE) {
        while (dpm_user_mem_init_check() == pdFALSE) {
            vTaskDelay(1);
        }
    }

    dpm_wu_src = dpm_mode_get_wakeup_source();
    dpm_wu_type = dpm_mode_get_wakeup_type();

    if (dpm_wu_src == WAKEUP_EXT_SIG_WITH_RETENTION) {
        dpm_resume_flag = pdTRUE;
    } else if (   (dpm_wu_src == WAKEUP_COUNTER_WITH_RETENTION)
               && (dpm_wu_type == DPM_RTCTIME_WAKEUP || dpm_wu_type == DPM_PACKET_WAKEUP)) {
        dpm_resume_flag = pdTRUE;
    }

    if (dpm_mode_is_wakeup() == DPM_WAKEUP) {
        if (dpm_resume_flag == pdTRUE) {
            // Get configuration for tcp server
            atcmd_tcps_conf = atcmd_network_get_tcps_config(ATCMD_TCPS_DPM_NAME);
            if (atcmd_tcps_conf) {
                atcmd_tcps_run_flag = pdTRUE;
            }

            // Get configuration for tcp client
            atcmd_tcpc_conf = atcmd_network_get_tcpc_config(ATCMD_TCPC_DPM_NAME);
            if (atcmd_tcpc_conf) {
                atcmd_tcpc_run_flag = pdTRUE;
            }

            // Get configuration for udp session
            atcmd_udps_conf = atcmd_network_get_udps_config(ATCMD_UDPS_DPM_NAME);
            if (atcmd_udps_conf) {
                atcmd_udps_run_flag = pdTRUE;
            }

#if defined (__SUPPORT_ATCMD_TLS__)
            atcmd_transport_ssl_recover_context_dpm();
#endif // (__SUPPORT_ATCMD_TLS__)
        }
    } else if (dpm_mode_is_enabled() == DPM_ENABLED) {
        //check configuration for tcp server
        if (!read_nvram_int(ATC_NVRAM_TCPS_PORT, &saved_atcmd_tcps_port)) {
            atcmd_tcps_run_flag = pdTRUE;
        }

        //check configuration for tcp client
        if (!read_nvram_int(ATC_NVRAM_TCPC_RPORT, (int *)(&saved_atcmd_tcpc_rport))) {
            saved_atcmd_tcpc_ip = read_nvram_string(ATC_NVRAM_TCPC_IP);
            if (saved_atcmd_tcpc_ip) {
                if (read_nvram_int(ATC_NVRAM_TCPC_LPORT, (int *)(&saved_atcmd_tcpc_lport))) {
                    saved_atcmd_tcpc_lport = 0;
                }

                atcmd_tcpc_run_flag = pdTRUE;
            }
        }

        //check configuration for udp session
        if (!read_nvram_int(ATC_NVRAM_UDPS_PORT, &saved_atcmd_udps_port)) {
            atcmd_udps_run_flag = pdTRUE;
        }

#if defined ( __SUPPORT_ATCMD_TLS__ )
        atcmd_transport_ssl_recover_context_nvram();

        if (atcmd_tcps_run_flag || atcmd_udps_run_flag || atcmd_tcpc_run_flag || atcmd_tls_ctx_header) {
        	RM_PMGR_W_dpm_job_name_set(ATCMD_DPM_TMP, 0);
            dpm_app_sleep_ready_clear(ATCMD_DPM_TMP);
        }
#else
        if (atcmd_tcps_run_flag || atcmd_udps_run_flag || atcmd_tcpc_run_flag) {
        	RM_PMGR_W_dpm_job_name_set(ATCMD_DPM_TMP, 0);
            dpm_app_sleep_ready_clear(ATCMD_DPM_TMP);
        }
#endif // __SUPPORT_ATCMD_TLS__
    }

    ret = xTaskCreate(atcmd_task,
                      "AT-CMD",
#if defined (__SUPPORT_AWS_IOT__) || defined(__SUPPORT_AZURE_IOT__) //APP_CORE
                    (2048 / 4),
#else
                      (ATCMD_STACK_SIZE/4),
#endif
                      (void *)NULL,
                      OS_TASK_PRIORITY_USER + 6,
                      &atcmd_task_handler);
    if (ret != pdPASS) {
        ATCMD_DBG("[%s] ERROR(%d)\n", __FUNCTION__, ret);
        return;
    }

    //start tcp server
    if (atcmd_tcps_run_flag) {
        ret = atcmd_network_connect_tcps(saved_atcmd_tcps_port);
        if (!ret) {
            if (!dpm_mode_is_wakeup()) {
                PRINTF("[AT-CMD] TCP Server OPEN (Port: %d)\r\n",
                        ntohs(atcmd_tcps_conf->local_addr.sin_port));
            }
        } else {
            ATCMD_DBG("[%s] Failed to start %s TCP Server with saved information ...\n",
                    __func__, ATCMD_TCPS_DPM_NAME);

            while (init_done_sent == pdFALSE) {
                vTaskDelay(10);
            }

            PRINTF_ATCMD("\r\n" ATCMD_TCPS_DISCONN_RX_PREFIX ":%d,0,0\r\n", ID_TS);
        }
    }

    //start tcp client
    if (atcmd_tcpc_run_flag) {
        ret = atcmd_network_connect_tcpc(saved_atcmd_tcpc_ip, saved_atcmd_tcpc_rport, saved_atcmd_tcpc_lport);
        if (!ret) {
            if (!dpm_mode_is_wakeup()) {
                PRINTF("[AT-CMD] TCP Client CONNECTED (IP: %d.%d.%d.%d, Port: %d)\n",
                        (ntohl(atcmd_tcpc_conf->svr_addr.sin_addr.s_addr) >> 24) & 0xFF,
                        (ntohl(atcmd_tcpc_conf->svr_addr.sin_addr.s_addr) >> 16) & 0xFF,
                        (ntohl(atcmd_tcpc_conf->svr_addr.sin_addr.s_addr) >>  8) & 0xFF,
                        (ntohl(atcmd_tcpc_conf->svr_addr.sin_addr.s_addr)      ) & 0xFF,
                        (ntohs(atcmd_tcpc_conf->svr_addr.sin_port)));
            }
        } else {
            ATCMD_DBG("[%s] Failed to start %s TCP Client with saved information ...\n",
                      __func__, ATCMD_TCPC_DPM_NAME);

             while (init_done_sent == pdFALSE) {
                 vTaskDelay(10);
             }

            PRINTF_ATCMD("\r\n" ATCMD_TCPC_DISCONN_RX_PREFIX ":%d,%s,%d\r\n", ID_TC,
                                 saved_atcmd_tcpc_ip, saved_atcmd_tcpc_rport);
        }
    }

    //start udp session
    if (atcmd_udps_run_flag) {
        ret = atcmd_network_connect_udps(NULL, 0, saved_atcmd_udps_port);
        if (!ret) {
            if (!dpm_mode_is_wakeup()) {
                PRINTF("[AT-CMD] UDP session OPEN (Port: %d)\n",
                        ntohs(atcmd_udps_conf->local_addr.sin_port));
            }
        } else {
            ATCMD_DBG("[%s] Failed to start %s UDP Client with saved information ...\n",
                      __func__, ATCMD_UDPS_DPM_NAME);

            while (init_done_sent == pdFALSE) {
                vTaskDelay(10);
            }

            PRINTF_ATCMD("\r\n" ATCMD_UDP_SESS_FAIL_PREFIX "\r\n", ID_US);
        }
    }

#if defined (__SUPPORT_ATCMD_TLS__)
    atcmd_transport_ssl_recover_tls_session();

    if (   (dpm_mode_is_enabled() == DPM_ENABLED && dpm_mode_is_wakeup() != DPM_WAKEUP)
        && (atcmd_tcps_run_flag || atcmd_udps_run_flag || atcmd_tcpc_run_flag || atcmd_tls_ctx_header)) {
        while (1) {
            if (chk_network_ready(0) == 1) {
                break;
            }
            vTaskDelay(10);
        }

        atcmd_network_display(0xFF);
        dpm_app_unregister(ATCMD_DPM_TMP);
    }
#else
    if (   (dpm_mode_is_enabled() == DPM_ENABLED && dpm_mode_is_wakeup() != DPM_WAKEUP)
        && (atcmd_tcps_run_flag || atcmd_udps_run_flag || atcmd_tcpc_run_flag)) {
        while (1) {
            if (chk_network_ready(0) == 1) {
                break;
            }
            vTaskDelay(10);
        }

        atcmd_network_display(0xFF);
        dpm_app_unregister(ATCMD_DPM_TMP);
    }
#endif // (__SUPPORT_ATCMD_TLS__)

    return ;
}
#endif // (__SUPPORT_ATCMD_MULTI_SESSION__)

static int atcmd_chk_wifi_conn(void)
{
    char *reply;
    int  result = pdFALSE;

    reply = (char *)pvPortMalloc(512);
    memset(reply, 0, 512);
    da16x_cli_reply("status", NULL, reply);

    if (strstr(reply, "wpa_state=COMPLETED")) {
        result = pdTRUE;
    }

    vPortFree(reply);

    return result;
}

#define	EVT_WFJAP_CONN_SENT	0x01
static EventGroupHandle_t evt_grp_wfjap = NULL;

static void atcmd_wfjap_conn_sent(void)
{
    if (evt_grp_wfjap) {
        xEventGroupSetBits(evt_grp_wfjap, EVT_WFJAP_CONN_SENT);
    }
}

static void atcmd_rsp_wifi_conn(void)
{
    int old_reason_code = 0;

    EventBits_t event = 0;
    const int wait_count_initial_value = 3000; // Sync with #define MAX_INIT_WIFI_CONN_TIME 30
    TickType_t wait_opt = wait_count_initial_value / 2;

    is_waiting_for_wf_jap_result_in_progress = TRUE;

    if (dpm_mode_is_enabled()) {
        dpm_abnormal_chk_hold();
    }

    if (evt_grp_wfjap == NULL) {
        evt_grp_wfjap = xEventGroupCreate();
    }

LBL_WIFI_RSP_CHK:
    event = xEventGroupWaitBits(evt_grp_wfjap, EVT_WFJAP_CONN_SENT,
                                pdTRUE, pdFALSE, wait_opt);

    if (event & EVT_WFJAP_CONN_SENT) {
        is_waiting_for_wf_jap_result_in_progress = FALSE;
        if (dpm_mode_is_enabled()) {
            force_dpm_abnormal_sleep_by_wifi_conn_fail();
            dpm_abnormal_chk_resume();
        }
        goto LBL_WIFI_RSP_CHK_END;

    } else {
        // timeout
        if (wifi_conn_fail_reason_code != 45 /* WLAN_REASON_PEERKEY_MISMATCH */) {
            if (wait_opt == wait_count_initial_value / 2) {
                wait_opt = wait_count_initial_value;
                goto LBL_WIFI_RSP_CHK;
            }
        }
    }

    da16x_cli_reply("disconnect", NULL, NULL);
    is_waiting_for_wf_jap_result_in_progress = FALSE;
    old_reason_code = wifi_conn_fail_reason_code;

    if (wifi_conn_fail_reason_code != 45 /* WLAN_REASON_PEERKEY_MISMATCH */) {
        wifi_conn_fail_reason_code = WLAN_REASON_TIMEOUT;
    }

    atcmd_wf_jap_dap_print_with_cause(1); // send connect trial failure
    wifi_conn_fail_reason_code = old_reason_code;
#if defined (__SUPPORT_MQTT__)
    atcmd_wfdap_condition_resolved = FALSE;
#endif // __SUPPORT_MQTT__

    if (dpm_mode_is_enabled()) {
        force_dpm_abnormal_sleep_by_wifi_conn_fail();
        dpm_abnormal_chk_resume();
    }

LBL_WIFI_RSP_CHK_END:
    if (evt_grp_wfjap) {
        vEventGroupDelete(evt_grp_wfjap);
        evt_grp_wfjap = NULL;
    }
}

void atcmd_asynchony_event(int index, char *buf)
{
#if defined(__SUPPORT_WIFI_PROVISIONING__)
#if !defined(__BLE_COMBO_REF__) && defined(__SUPPORT_FACTORY_RST_CONCURR_MODE__)
    extern void AP_connected_notification_on_concurrent(int _index, char* _buf);
    AP_connected_notification_on_concurrent(index, buf);
#endif // !defined(__BLE_COMBO_REF__) && defined(__SUPPORT_FACTORY_RST_CONCURR_MODE__)
#endif // (__SUPPORT_WIFI_PROVISIONING__)

    switch (index) {
    case 0: {    //ATC_EV_STACONN:
        char ssid_str[32 * 4 + 1] = {0, }, ip_str[16] = { 0, };

        if (atoi(buf) == 1) {
            int tmp;

            da16x_get_config_int(DA16X_CONF_INT_DHCP_CLIENT, &tmp);

            if (tmp == CC_VAL_ENABLE && !dpm_mode_is_wakeup()) {
                return;
            }
        }

        if ((da16x_get_config_str(DA16X_CONF_STR_SSID_0, ssid_str) != CC_SUCCESS) || strlen(ssid_str) == 0) {
#if defined (__SUPPORT_MQTT__)
            atcmd_wfjap_not_send_by_err = TRUE;
#endif //__SUPPORT_MQTT__
            return;
        }

        da16x_get_config_str(DA16X_CONF_STR_IP_0, ip_str);

        if (strcmp(ip_str, "0.0.0.0") != 0) {
            PRINTF_ATCMD("\r\n+WFJAP:1,'%s',%s\r\n", ssid_str, ip_str);
            atcmd_wfjap_conn_sent();
#if defined (__SUPPORT_MQTT__)
            atcmd_wfdap_condition_resolved = TRUE;
            atcmd_wfjap_not_send_by_err = FALSE;
        } else {
            atcmd_wfjap_not_send_by_err = TRUE;
#endif
        }
    } break;

    case 1:    //ATC_EV_STADISCONN:
        atcmd_wf_jap_dap_print_with_cause(0);
#if defined (__SUPPORT_MQTT__)
        atcmd_wfdap_condition_resolved = FALSE;
#endif // __SUPPORT_MQTT__
        break;

    case 2:    //ATC_EV_APCONN:
        PRINTF_ATCMD("\r\n+WFCST:%s\r\n", buf);

        for (int i = 0; i < 6; i++) {
            if (strlen(atcmd_mac_table[i]) == 0) {
                strcpy(atcmd_mac_table[i], buf);
                break;
            }
        }

        break;

    case 3:    //ATC_EV_APDISCONN:
        PRINTF_ATCMD("\r\n+WFDST:%s\r\n", buf);

        for (int i = 0; i < 6; i++) {
            if (strcmp(atcmd_mac_table[i], buf) == 0) {
                strcpy(atcmd_mac_table[i], "");
                break;
            }
        }
        break;

#if defined ( __SUPPORT_MQTT__ )
    case 4: {    //ATC_EV_MQTTSUBCONN:
        int mqtt_connected = atoi(buf);

        if (mqtt_connected) {
            int wait_count = 100;    // 1 sec

            while (!atcmd_wfdap_condition_resolved) {
                if (atcmd_wfjap_not_send_by_err || --wait_count == 0) {
                    PRINTF("atcmd_wfjap_not_send_by_err(%d), wait_count(%d)\n", atcmd_wfjap_not_send_by_err, wait_count);
                    break;
                }
                vTaskDelay(1);
            }

            if (!atcmd_wfjap_not_send_by_err) {
                if (atcmd_mqtt_connected_ind_sent == FALSE) {
                    PRINTF_ATCMD("\r\n+NWMQCL:1\r\n");
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
                    atcmd_mqtt_connected_ind_sent = TRUE;
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
                }
            }
        } else {
#if defined (__MQTT_CLEAN_SESSION_MODE_SUPPORT__)
            if (is_disconn_by_too_long_msg_rx_in_cs0()) {
                PRINTF_ATCMD("\r\n+NWMQCL:0,TOO_LONG_MSG_RX\r\n");
                set_disconn_by_too_long_msg_rx_in_cs0(FALSE);
            } else
#endif // __MQTT_CLEAN_SESSION_MODE_SUPPORT__
            {
                PRINTF_ATCMD("\r\n+NWMQCL:0\r\n");
            }
            atcmd_mqtt_connected_ind_sent = FALSE;
        }
    } break;
#endif // __SUPPORT_MQTT__

#if defined (__SUPPORT_OTA__)
    case 6: {    //ATC_EV_OTADWSTART:
        PRINTF_ATCMD("\r\n+NWOTADWSTART:%s\r\n", buf);
    } break;

    case 7: {   //ATC_EV_OTARENEW:
        PRINTF_ATCMD("\r\n+NWOTARENEW:%s\r\n", buf);
    } break;
#endif // (__SUPPORT_OTA__)

#if defined (__SUPPORT_HTTP_CLIENT_FOR_ATCMD__)
    case 8: {   //ATC_EV_HTC:
        PRINTF_ATCMD("\r\n+NWHTC:%s", buf);
    } break;

    case 9: {    //ATC_EV_HTCSTATUS:
        PRINTF_ATCMD("\r\n+NWHTCSTATUS:%s\r\n", buf);
    } break;
#endif // (__SUPPORT_HTTP_CLIENT_FOR_ATCMD__)
    case 10: {    // +INIT:DONE,1
        if (   get_run_mode() == SYSMODE_AP_ONLY
            && (get_INIT_DONE_sent_flag() == pdFALSE && init_done_sent == pdTRUE)) {
            PRINTF_ATCMD("\r\n+INIT:DONE,%d\r\n", get_run_mode());
            set_INIT_DONE_msg_to_MCU_on_SoftAP_mode(pdTRUE);
        }
    } break;

    } // End of Switch
}

void atcmd_asynchony_event_for_mqttmsgrecv(int index, char *buf1, char *buf2, int len)
{
    switch (index) {
#if defined ( __SUPPORT_MQTT__ )
    case 0: {    //ATC_EV_MQTTMGSRECV:
        PRINTF_ATCMD("\r\n+NWMQMSG:%s,%s,%d\r\n", buf1, buf2, len);
    } break;
#else
    DA16X_UNUSED_ARG(buf1);
    DA16X_UNUSED_ARG(buf2);
    DA16X_UNUSED_ARG(len);
#endif // __SUPPORT_MQTT__
    }
}

/*
 * Information for Console Version display
 */
static void atcmd_sdk_version(void)
{
    PRINTF_ATCMD("\r\n+SDKVER:%d.%d.%d.%d", SDK_MAJOR, SDK_MINOR, SDK_REVISION, SDK_ENG_VER);
}

#if defined ( __PROVISION_ATCMD__ )
atcmd_provision_stat Prov_LastStat = ATCMD_PROVISION_IDLE;

/*
 *    MCU Provisioning Control
 */
int    atcmd_app_provision(int argc, char *argv[])
{
    DA16X_UNUSED_ARG(argc);

    int status = AT_CMD_ERR_CMD_OK;

    // AT+PROVSTART  : facory reset is provisioning ready
    if (strcasecmp(argv[0] + 7, "START") == 0) {
        factory_reset_default(1);
    } else if (strcasecmp(argv[0] + 7, "STAT") == 0) {
        char tBuf[128] = {0, };

        sprintf(tBuf,"\r\n+ATPROV=STATUS %d\r\n",Prov_LastStat);
        PRINTF_ATCMD(tBuf);
    }

    return status;
};

/*
 * send provisioninig status to MCU
 */
void atcmd_provstat(int status)
{
    char tBuf[128] = {0, };

    Prov_LastStat = status;

    sprintf(tBuf,"\r\n+ATPROV=STATUS %d\r\n",status);
    PRINTF_ATCMD(tBuf);
    PRINTF(tBuf);

};
#endif  // __PROVISION_ATCMD__

int atcmd_wdog_service(int argc, char *argv[])
{
    int err_code = AT_CMD_ERR_CMD_OK;
    char result_str[32] = {0x00,};
    unsigned int rescale_time = 0;

    //AT+WDOGRESCALETIME=<rescale_time>
    if (strcasecmp(argv[0] + 7, "RESCALETIME") == 0) {
        if (argc == 2) {
            if (strcasecmp(argv[1], "?") == 0) {
                rescale_time = da16x_sys_watchdog_get_rescale_time();
                if (rescale_time) {
                    snprintf(result_str, sizeof(result_str), "%u", rescale_time);
                    err_code = AT_CMD_ERR_CMD_OK;
                } else {
                    err_code = AT_CMD_ERR_INSUFFICENT_CONFIG;
                }
            } else {
                if (get_int_val_from_str(argv[1], (int *)&rescale_time, POL_1) != 0) {
                    err_code = AT_CMD_ERR_WRONG_ARGUMENTS;
                    goto end;
                }

                if (da16x_sys_watchdog_set_rescale_time(rescale_time)) {
                    err_code = AT_CMD_ERR_INSUFFICENT_CONFIG;
                    ATCMD_DBG("[%s]Failed to set rescale time of watchdog service(%u <= %u <= %u)\n",
                            __func__,
                            DA16X_SYS_WDOG_MIN_RESCALE_TIME,
                            rescale_time,
                            DA16X_SYS_WDOG_MAX_RESCALE_TIME);
                    goto end;
                }
            }
        } else {
            err_code = AT_CMD_ERR_INSUFFICENT_ARGS;
        }
    } else {
        err_code = AT_CMD_ERR_UNKNOWN_CMD;
    }

end:

    ATCMD_DBG("[%s]err_code(%d)\n", __func__, err_code);

    if (strlen(result_str) > 0 && err_code == AT_CMD_ERR_CMD_OK) {
        PRINTF_ATCMD("\r\n%s", result_str);
    }

    return err_code;
}

static int hex_2_num(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
    } else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
    }

	return -1;
}

static int hex_2_byte(const char *hex)
{
	int a, b;

	a = hex_2_num(*hex++);
	if (a < 0) {
		return -1;
    }
	b = hex_2_num(*hex++);
	if (b < 0) {
		return -1;
    }

	return (a << 4) | b;
}

static size_t str_decode(u8 *buf, size_t maxlen, const char *str)
{
    const char *pos = str;
    size_t len = 0;
    int val;

    while (*pos) {
        if (len >= maxlen) {
            len = 999;    // Too long string length
            break;
        }

        /* Check '\' character */
        switch (*pos) {
        case '\\':
            pos++;

            // Check hexa code or not ...
            switch (*pos) {
            case 'x':
                pos++;
                val = hex_2_byte(pos);
                if (val < 0) {
                    val = hex_2_num(*pos);
                    if (val < 0) {
                        break;
                    }
                    buf[len++] = val;
                    pos++;
                } else {
                    buf[len++] = val;
                    pos += 2;
                }
                break;

            default: // Not control character...
                buf[len++] = *(pos-1);
                break;
            }
            break;

        default:
            buf[len++] = *pos++;
            break;
        }
    }

    if (len == maxlen) {
        buf[len+1] = '\0';
    } else if (maxlen > len) {
        buf[len] = '\0';
    }

    return len;
}


#else    ////////////////////////////////////////////////////////

#include "task.h"
#include "da16x_types.h"

void start_atcmd(void)
{
    ;
}

void dpm_ext_wu_mon(void * pvParameters)
{
    DA16X_UNUSED_ARG(pvParameters);
    vTaskDelete(NULL);
}

void atcmd_asynchony_event(int index, char *buf) {
#if defined(__SUPPORT_WIFI_PROVISIONING__)
#if !defined(__BLE_COMBO_REF__) && defined(__SUPPORT_FACTORY_RST_CONCURR_MODE__)
    extern void AP_connected_notification_on_concurrent(int _index, char* _buf);

    AP_connected_notification_on_concurrent(index, buf);
#else
    DA16X_UNUSED_ARG(index);
    DA16X_UNUSED_ARG(buf);
#endif // !defined(__BLE_COMBO_REF__) && defined(__SUPPORT_FACTORY_RST_CONCURR_MODE__)
#else
    DA16X_UNUSED_ARG(index);
    DA16X_UNUSED_ARG(buf);
#endif // (__SUPPORT_WIFI_PROVISIONING__)
}

void atcmd_asynchony_event_for_mqttmsgrecv(int index, char *buf1, char *buf2, int len) {
    DA16X_UNUSED_ARG(index);
    DA16X_UNUSED_ARG(buf1);
    DA16X_UNUSED_ARG(buf2);
    DA16X_UNUSED_ARG(len);
}

void atcmd_wf_jap_dap_print_with_cause(int is_jap)
{
    DA16X_UNUSED_ARG(is_jap);
}

void atcmd_asynchony_event_for_mqttmsgpub(int result, int err_code)
{
    DA16X_UNUSED_ARG(result);
    DA16X_UNUSED_ARG(err_code);
}

void atcmd_wf_jap_print_with_cause(int cause)
{
    DA16X_UNUSED_ARG(cause);
}


#endif // __SUPPORT_ATCMD__
#endif //	azure_check]]

/* EOF */
