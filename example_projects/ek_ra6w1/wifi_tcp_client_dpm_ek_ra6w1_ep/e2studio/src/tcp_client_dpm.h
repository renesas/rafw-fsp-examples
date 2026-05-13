/***********************************************************************************************************************
 * File Name    : tcp_client_dpm.h
 * Description  : Declarations and definitions for tcp client dpm functions
 **********************************************************************************************************************/
/**********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef TCP_CLIENT_DPM_H_
#define TCP_CLIENT_DPM_H_

#if !CFG_CLI
#define SSID                            "SSID"
#define PASSPHRASE                      "12345678"
#define CHANNEL 1
#define PASSS_LEN                       strlen(PASSPHRASE)
#define SSID_LEN                        strlen(SSID)
#endif
#define TCPC_PEER_IP_ADDR_LEN           64
#define TCPCL_RTM_NAME                  "TCPCL_CONF"
#define TCPC_DEF_LOCAL_PORT             12345
#define TCPC_DEF_BUF_SIZE               (1024 * 4)
#define TCPC_DEF_PEER_IP_ADDR           "192.168.50.58"
#define TCPC_DEF_PEER_PORT              TCPC_DEF_LOCAL_PORT
#define TCPC_DEF_SEND_PERIOD            10000   //ms
#define TCPC_MIN_SEND_PERIOD            1000   // ms
#define TCPC_DEF_SEND_DATA_SIZE         1024   // bytes
#define TCPC_DEF_RECV_TIMEOUT           100    //ms
#define TCPC_DEF_SEND_TIMEOUT           3000   //ms
#define TCPC_REMAIN_DATA_INIT           -1
#define TCPC_DEF_AUTO_RESTART_AT_EXIT   1       // restart tcpc after exit due to connection failure (e.g. AP reboot)
#define TCPC_DEF_KA_ENABLE              1
#define TCPC_DEF_KA_IDLE_TIME           30
#define TCPC_DEF_KA_INTVL_TIME          30
#define TCPC_DEF_KA_MAX_PROBES          9

/*  Define the jobs of TCP_Client that join PMGR_DPM system */
#define JOB_ID_SEND                     "JOB_SEND"
#define JOB_ID_RECV                     "JOB_RECV"
#define TCPC_DPM_TIMER_NAME             "TCPC_T"

#define EVT_TCPC_DISCONN                (1UL << (0))
#define EVT_TCPC_CONN                   (1UL << (1))
#define EVT_TCPC_EXIT                   (1UL << (2))
#define EVT_TCPC_ANY                    (EVT_TCPC_DISCONN | EVT_TCPC_CONN | EVT_TCPC_EXIT)

#define WIFI_EVENT_CONNECTED 7
#define EVENT_VAL -1

typedef struct \
{
    char peer_ip_addr[TCPC_PEER_IP_ADDR_LEN];
    int peer_port;
    int send_period;
    int send_data_size;
    int autostart_atexit;

    /* TCP Keepalive params */
    int ka_enable;
    int ka_idle_time;
    int ka_intvl_time;
    int ka_max_probes;
} tcpcl_conf_t;

void tcp_client_init();

#endif // TCP_CLIENT_DPM_H_
