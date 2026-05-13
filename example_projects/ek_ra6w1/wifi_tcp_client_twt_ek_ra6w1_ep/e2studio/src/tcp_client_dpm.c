/***********************************************************************************************************************
 * File Name    : tcp_client_dpm.c
 * Description  : Handle tcp client dpm  and TWT functions
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "tcp_client_dpm.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif // CFG_PMGR
#include "iface_defs.h"
#include "net_network_main.h"

/*******************************************************************************************************************//**
 * @addtogroup tcp_client_dpm
 * @{
 **********************************************************************************************************************/
/*
 * Macros
 */
#define TCP_CLIENT_STACK_SIZE   1024
#define TID_VALUE 16

/*
 * Global variables
 */
extern uint32_t event;
extern TaskHandle_t g_app_main_task_handle;
void tcp_client_dpm_task();
extern struct twt_setup_req setup_params;
extern struct twt_teardown_req teardown_params;
tcpcl_conf_t *p_conf = NULL;
uint32_t notify_val;
int twt_setup_retry_cnt;

/*
 * Static variables
 */
static unsigned char g_tcpc_recv_buf[TCPC_DEF_BUF_SIZE] = {0x00,};
static unsigned char g_tcpc_send_buf[TCPC_DEF_BUF_SIZE] = {0x00,};
static int g_tcpc_send_cnt = 0;
static TaskHandle_t tcp_client_dpm_tasks_ptr = NULL;
static EventGroupHandle_t tcp_client_mgmt_evt = NULL;
static bool g_restart_tcp_client_app = false;
static tcpcl_conf_t tcpcl_conf = {0x00,};
static bool g_restart_tcp_app_send_timer = false;
int dpm_wake = 0;

/*
 * Static functions
 */
 /**
 ****************************************************************************************
 * @brief TWT events callback function
 * @param[in] pxEvent
 * @return None
 ****************************************************************************************
 */
void twt_event_callback(WIFIEvent_t *pxEvent)
{
    switch (pxEvent->xEventType)
    {
        case eWiFiEventExtTWT:
        {
        	const WIFIEventExt_t *pxExtEvent = (const WIFIEventExt_t *)pxEvent;

        	switch (pxExtEvent->xInfo.xTWT.xEvent)
            {
                case WIFI_TWT_EVENT_SESSION_SUCCESS:
                    printf("[TWT] Session established successfully.\n");
                    p_conf->twt_session_active = 1;
                    break;

                case WIFI_TWT_EVENT_AP_REJECTED:
                    printf("[TWT] AP rejected the TWT request.\n");
                    break;

                case WIFI_TWT_EVENT_AP_RESPONSE_PARAM_NOT_MATCHED:
                    printf("[TWT] AP response parameters did not match request.\n");
                    break;

                case WIFI_TWT_EVENT_TEARDOWN_SUCCESS:
                    printf("[TWT] TWT session teardown successful.\n");
                    p_conf->twt_session_active = 0;
                    xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
                    break;

                case WIFI_TWT_EVENT_AP_TEARDOWN_SUCCESS:
                    printf("[TWT] AP tore down the TWT session.\n");
                    p_conf->twt_session_active = 0;
                    xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
                    break;

                case WIFI_TWT_EVENT_MAX_RETRIES_REACHED:
                    printf("[TWT] Maximum retries reached for TWT setup.\n");
                    break;

                case WIFI_TWT_EVENT_AP_NO_SUPPORT:
                    printf("[TWT] AP does not support TWT.\n");
                    break;

                case WIFI_TWT_EVENT_TWT_REQ_ABORT:
                    printf("[TWT] TWT setup abort.\n");
                    break;

                default:

                   printf("[TWT] Unknown sub-event: %d\n", pxExtEvent->xInfo.xTWT.xEvent);
                   break;
               }
            break;
        }

        default:
            printf("[TWT] Unknown event: %d\n", pxEvent->xEventType);
            break;
    }
}

/**
 ****************************************************************************************
 * @brief check whether ipv4 address
 * @param[in] str
 * @param[in] valid_ip
 * @return success or failure
 ****************************************************************************************
 */
static bool isIPv4Address(char *str, struct sockaddr_in *valid_ip)
{
#if defined ( __SUPPORT_IPV4__ )
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, str, &(sa.sin_addr)) == 1)
    {
        if (valid_ip)
        {
            memcpy(valid_ip, &sa, sizeof(struct sockaddr_in));
        }

        return 1; // success
    }
    else
    {
        return 0; // fail
    }
#else

    return 0;
#endif // __SUPPORT_IPV4__
}

/**
 ****************************************************************************************
 * @brief check whether ipv6 address
 * @param[in] str
 * @param[in] valid_ip
 * @return success or failure
 ****************************************************************************************
 */
static bool isIPv6Address(char *str, struct sockaddr_in6 *valid_ip)
{
#if defined ( __SUPPORT_IPV6__ )
    struct sockaddr_in6 sa;

    if (inet_pton(AF_INET6, str, &(sa.sin6_addr)) == 1)
    {
        if (valid_ip)
        {
            memcpy(valid_ip, &sa, sizeof(struct sockaddr_in6));
        }

        return 1; // success
    }
    else
    {
        return 0; // fail
    }
#else

    return 0;
#endif // __SUPPORT_IPV6__
}

/**
 ****************************************************************************************
 * @brief find ip type from address string
 * @param[in] str
 * @param[in] ipaddr
 * @return ip type
 ****************************************************************************************
 */
static int select_ipaddr_type_from_str(char *str, struct sockaddr_storage *ipaddr)
{
#if defined ( __SUPPORT_IPV4__ )
    if (isIPv4Address(str, (struct sockaddr_in *)ipaddr))
    {
        return IPADDR_TYPE_V4;
    }
#endif // __SUPPORT_IPV4__

#if defined ( __SUPPORT_IPV6__ )
    if (isIPv6Address(str, (struct sockaddr_in6 *)ipaddr))
    {
        return IPADDR_TYPE_V6;
    }
#endif // __SUPPORT_IPV6__

    return -1;
}


#if CFG_PMGR
/**
 ****************************************************************************************
 * @brief Timer callback function
 * @param[in] timer_name
 * @return None
 ****************************************************************************************
 */
static void tcpc_send_timer_cb(char *timer)
{
    pmgr_instance_ctrl_t *p_instance_ctrl = RM_PMGR_W_get_ctrl();
    const char *timer_name = timer;

    if (timer_name)
    {
        printf("[APP] TCP Timer(%s)\n", timer_name);
    }

    /* Constrain RAM till all TCP send is done */
    if (g_tcpc_send_cnt == 0)
    {
        RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
        if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
        {
            p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]++;
        }
    }

    g_tcpc_send_cnt++;
}

/**
 ****************************************************************************************
 * @brief Function to register timer
 * @param[in] send_period
 * @return success or failure
 ****************************************************************************************
 */
static int tcpc_register_timer(int send_period)
{
    if (!RM_PMGR_W_dpm_is_enabled())
    {
        return 0;
    }

    if (!send_period)
    {
        return 0;
    }

    /* Short period will result with effectivly no DPM */
    if (send_period < TCPC_MIN_SEND_PERIOD)
    {
        return -1;
    }

    /* Create the timer only once at POR, or if timer was deregistered */
    if (!RM_PMGR_W_dpm_is_wakeup() || g_restart_tcp_app_send_timer)
    {
        int tid = RM_PMGR_W_dpm_timer_create(JOB_ID_SEND,
                                         TCPC_DPM_TIMER_NAME,
                                         tcpc_send_timer_cb,
                                         send_period,
                                         send_period);
        if (tid == TID_VALUE)
        {
            return -1;
        }

        g_restart_tcp_app_send_timer = false;
    }

    g_tcpc_send_cnt = 0;
    RM_PMGR_W_dpm_job_name_set(JOB_ID_SEND, 0);

    return 0;
}

/**
 ****************************************************************************************
 * @brief Function to de-register timer
 * @param[in] None
 * @return success or failure
 ****************************************************************************************
 */
static int tcpc_deregister_timer()
{
    pmgr_instance_ctrl_t *p_instance_ctrl = RM_PMGR_W_get_ctrl();
    int ret = -1;

    if (!RM_PMGR_W_dpm_is_enabled())
    {
        return -1;
    }

    if (tcpcl_conf.send_period == 0)
    {
        return 0;
    }

    ret = RM_PMGR_W_dpm_timer_delete(JOB_ID_SEND, TCPC_DPM_TIMER_NAME);
    if (ret < 0)
    {
        printf("[APP] Failed to deregister TCP client's timer(%d)\n", ret);
    }

    if (tcpcl_conf.autostart_atexit)
    {
        g_restart_tcp_app_send_timer = true;
    }

    if (g_tcpc_send_cnt)
    {
        /* Remove the existing send constraint (the send will never happen) */
        RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
        if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
        {
            p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
        }

        g_tcpc_send_cnt = 0;
    }

    return ret;
}
#endif // CFG_PMGR

/**
 ****************************************************************************************
 * @brief Function to load TCPC configurations
 * @param[in] p_tcpc_conf
 * @return Success or failure
 ****************************************************************************************
 */
static int tcpc_load_conf(tcpcl_conf_t *p_tcpc_conf)
{
    int is_exist_conf = pdFALSE;

    if (!p_tcpc_conf)
    {
        return -1;
    }

#if CFG_PMGR
    /*Read configuration from RTM. */
    if (RM_PMGR_W_dpm_is_enabled())
    {
    	unsigned int rtm_len = RM_PMGR_W_user_rtm_get(TCPCL_RTM_NAME, (unsigned char **)&p_conf);
        if (rtm_len == 0)
        {
        	unsigned int rtm_ret = (int)RM_PMGR_W_user_rtm_pool_alloc(TCPCL_RTM_NAME, (void **)&p_conf, sizeof(tcpcl_conf_t), 0);
            if (rtm_ret)
            {
                printf("[APP] Failed to allocate RTM for configuration(%u)\n", rtm_ret);
                p_conf = NULL;
                return -1;
            }
            else
            {
                memset(p_conf, 0x00, sizeof(tcpcl_conf_t));
            }
        }
        else
        {
            is_exist_conf = pdTRUE;
            printf("[APP] TCPC SESS restored ... \n");
        }
    }
#endif // CFG_PMGR

    if (!p_conf)
    {
        p_conf = p_tcpc_conf;
        memset(p_conf, 0x00, sizeof(*p_conf));
    }

    if (!is_exist_conf)
    {
        strcpy(p_conf->peer_ip_addr, TCPC_DEF_PEER_IP_ADDR);
        p_conf->peer_port = TCPC_DEF_PEER_PORT;
        p_conf->send_period = TCPC_DEF_SEND_PERIOD;
        p_conf->send_data_size = TCPC_DEF_SEND_DATA_SIZE;
        p_conf->autostart_atexit = TCPC_DEF_AUTO_RESTART_AT_EXIT;
        p_conf->ka_enable = TCPC_DEF_KA_ENABLE;
        p_conf->ka_idle_time = TCPC_DEF_KA_IDLE_TIME;
        p_conf->ka_intvl_time = TCPC_DEF_KA_INTVL_TIME;
        p_conf->ka_max_probes = TCPC_DEF_KA_MAX_PROBES;
        p_conf->twt_session_active = 0;
    }

    /* Copy configuration */
    memcpy(p_tcpc_conf, p_conf, sizeof(tcpcl_conf_t));
    if (RM_PMGR_W_dpm_is_wakeup() == pdFALSE)
    {
        printf("%-20s : %s\n", "Peer IP Address", p_tcpc_conf->peer_ip_addr);
        printf("%-20s : %d\n", "Peer Port", p_tcpc_conf->peer_port);
        printf("%-20s : %d\n", "Period", p_tcpc_conf->send_period);
        printf("%-20s : %d\n", "Data size", p_tcpc_conf->send_data_size);
        printf("%-20s : %d\n", "Auto-start at exit", p_tcpc_conf->autostart_atexit);
        printf("%-20s : %d\n", "KA enable", p_tcpc_conf->ka_enable);
        if (p_tcpc_conf->ka_enable)
        {
            printf("%-20s : %d (sec)\n", "KA idle time", p_tcpc_conf->ka_idle_time);
            printf("%-20s : %d (sec)\n", "KA interval time", p_tcpc_conf->ka_intvl_time);
            printf("%-20s : %d\n", "KA max probe cnt", p_tcpc_conf->ka_max_probes);
        }
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Function to load TCPC keepalive configurations
 * @param[in] socket_fd
 * @param[in] enable
 * @param[in] keep_idle
 * @param[in] keep_intvl
 * @param[in] keep_cnt
 * @return Success or failure
 ****************************************************************************************
 */
static int tcp_client_keepalive_conf(int socket_fd, int enable, int keep_idle, int keep_intvl, int keep_cnt)
{
    int ret = 0;

    ret = setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    if (ret != 0)
    {
        printf("[APP] Failed to set SO_KEEPALIVE (%d)\n", ret);
        return ret;
    }
#if LWIP_TCP_KEEPALIVE

    if (keep_idle)
    {
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
        if (ret != 0)
        {
            printf("[APP] Failed to set TCP_KEEPIDLE (%d)\n", ret);
            return ret;
        }
    }

    if (keep_intvl)
    {
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intvl, sizeof(keep_intvl));
        if (ret != 0)
        {
            printf("[APP] Failed to set TCP_KEEPINTVL (%d)\n", ret);
            return ret;
        }
    }

    if (keep_cnt)
    {
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof(keep_cnt));
        if (ret != 0)
        {
            printf("[APP] Failed to set TCP_KEEPCNT (%d)\n", ret);
            return ret;
        }
    }
#else

    if (keep_idle)
    {
        keep_idle *= 1000; // Convert to ms
        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPALIVE, &keep_idle, sizeof(keep_idle));
        if (ret != 0)
        {
            printf("[APP] Failed to set TCP_KEEPALIVE\n");
            return ret;
        }
    }
#endif

    return ret;
}

#if defined ( __SUPPORT_IPV6__ )
/**
 ****************************************************************************************
 * @brief Function to wait for IPv6 address
 * @param[in] None
 * @return None
 ****************************************************************************************
 */
static void wait_ready_connect_ipv6()
{
    extern struct netif wlan0_iface;
    struct in6_addr peer_in6addr;
    ip6_addr_t peer_ip6addr;

    /* Convert string to workable IPv6 */
    inet_pton(AF_INET6, tcpcl_conf.peer_ip_addr, &peer_in6addr);
    inet6_addr_to_ip6addr(&peer_ip6addr, &peer_in6addr)
    if (ip6_addr_isglobal(&peer_ip6addr))
    {
    	int wait_cnt = 0;
        /* Wait till our own global address becomes valid */
        while (!ip6_addr_isvalid(netif_ip6_addr_state(&wlan0_iface, 1)))
        {
            vTaskDelay(portCONVERT_MS_2_TICKS(100));
            wait_cnt++;

            /* Wait max 10 seconds */
            if (wait_cnt > 100)
            {
                printf("[APP] Global IPv6 is not valid, attempting to connect with Link-local IPv6 address\n");
                break;
            }
        }
    }
}
#endif

/**
 ****************************************************************************************
 * @brief TCPC main task
 * @param[in] None
 * @return None
 ****************************************************************************************
 */
void tcp_client_dpm_task()
{
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    int ret = 0;
    int ip_type = 0;
    int socket_fd = -1;
    int remaining_data = TCPC_REMAIN_DATA_INIT;
    WIFI_RegisterEvent((WIFIEventType_t)eWiFiEventExtTWT, twt_event_callback);

#if CFG_PMGR
    pmgr_instance_ctrl_t *p_instance_ctrl = RM_PMGR_W_get_ctrl();
#endif // CFG_PMGR

    /* socket option */
    int sockopt_reuse = 1;

    struct timeval sockopt_timeout = {0x00,};
    struct sockaddr_storage valid_ip = {0x00,};
#if defined ( __SUPPORT_IPV4__ )
    struct sockaddr_in local_addr4;
    struct sockaddr_in *srv_addr4 = (struct sockaddr_in *)&valid_ip;
#endif // __SUPPORT_IPV4__
#if defined ( __SUPPORT_IPV6__ )
    struct sockaddr_in6 local_addr6;
    struct sockaddr_in6 *srv_addr6 = (struct sockaddr_in6 *)&valid_ip;
#endif // __SUPPORT_IPV6__
    int len = 0;
    unsigned char *p_recv_buf = g_tcpc_recv_buf;

    strcpy((char*)g_tcpc_send_buf, "hello_there!!");
    unsigned char *p_send_buf = g_tcpc_send_buf;
    size_t recv_buflen = sizeof(g_tcpc_recv_buf);
    ret = tcpc_load_conf(&tcpcl_conf);
    if (ret != 0)
    {
        printf("[APP] Failed to read TCP client's configuration\n");
        xEventGroupSetBits(tcp_client_mgmt_evt, EVT_TCPC_EXIT);
        vTaskDelete(NULL);

        return;
    }

#if CFG_PMGR
    ret = tcpc_register_timer(tcpcl_conf.send_period);
    if (ret != 0)
    {
        printf("[APP] Failed to set TCP send timer\n");
        xEventGroupSetBits(tcp_client_mgmt_evt, EVT_TCPC_EXIT);
        vTaskDelete(NULL);

        return;
    }

    /* JOB_ID_RECV - INIT with TCP port */
    RM_PMGR_W_dpm_job_name_set(JOB_ID_RECV, TCPC_DEF_LOCAL_PORT);

    /* JOB_ID_RECV - Set TCP port filter */
    RM_WIFI_dpm_tcp_port_filter_set(TCPC_DEF_LOCAL_PORT);
#endif /* CFG_PMGR */

    printf("[APP] Start of TCP Client application\n");
    ip_type = select_ipaddr_type_from_str(tcpcl_conf.peer_ip_addr, &valid_ip);

    /* Common */
    if ((strcmp(tcpcl_conf.peer_ip_addr, "0") != 0)
#if defined ( __SUPPORT_IPV4__ )
            && (ip_type != IPADDR_TYPE_V4)
#endif // __SUPPORT_IPV4__
#if defined ( __SUPPORT_IPV6__ )
            && (ip_type != IPADDR_TYPE_V6)
#endif // __SUPPORT_IPV6__
       )
    {
        printf("[APP] IP address is invalid\n");
        goto end_of_task;
    }
#if defined ( __SUPPORT_IPV4__ )

    if (ip_type == IPADDR_TYPE_V4)
    {
        memset(&local_addr4, 0x00, sizeof(struct sockaddr_in));
        memset(srv_addr4, 0x00, sizeof(struct sockaddr_in));
#if CFG_PMGR
        socket_fd = socket_dpm(JOB_ID_RECV, PF_INET, SOCK_STREAM, 0);
#else
        socket_fd = socket(PF_INET, SOCK_STREAM, 0);
#endif /* CFG_PMGR */
        if (socket_fd < 0)
        {
            printf("[APP] Failed to assign socket descriptor\n");
            goto end_of_task;
        }
    }
#endif // __SUPPORT_IPV4__
#if defined ( __SUPPORT_IPV6__ )

    if (ip_type == IPADDR_TYPE_V6)
    {
        memset(&local_addr6, 0x00, sizeof(struct sockaddr_in6));
        memset(srv_addr6, 0x00, sizeof(struct sockaddr_in6));
#if CFG_PMGR
        socket_fd = socket_dpm(JOB_ID_RECV, PF_INET6, SOCK_STREAM, 0);
#else
        socket_fd = socket(PF_INET, SOCK_STREAM, 0);
#endif /* CFG_PMGR */

        if (socket_fd < 0)
        {
            printf("[APP] Failed to assign socket descriptor\n");
            goto end_of_task;
        }
    }
#endif // __SUPPORT_IPV6__

    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt_reuse, sizeof(sockopt_reuse));
    if (ret)
    {
        printf("[APP] Failed to set socket option - SO_REUSEADDR(%d)\n", ret);
    }

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = TCPC_DEF_RECV_TIMEOUT * 1000;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (ret)
    {
        printf("[APP] Failed to set socket option - SO_RCVTIMEOUT(%d:%d)\n", ret, errno);
    }

    sockopt_timeout.tv_sec = 0;
    sockopt_timeout.tv_usec = TCPC_DEF_SEND_TIMEOUT * 1000;
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &sockopt_timeout, sizeof(sockopt_timeout));
    if (ret)
    {
        printf("[APP] Failed to set socket option - SO_SNDTIMEO(%d)\n", TCPC_DEF_SEND_TIMEOUT);
    }

    /* Set TCP keepalive socket options */
    tcp_client_keepalive_conf(socket_fd, tcpcl_conf.ka_enable, tcpcl_conf.ka_idle_time,
                             tcpcl_conf.ka_intvl_time, tcpcl_conf.ka_max_probes);
#if defined ( __SUPPORT_IPV4__ )
    if (ip_type == IPADDR_TYPE_V4)
    {
        local_addr4.sin_family = AF_INET;
        local_addr4.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr4.sin_port = htons(TCPC_DEF_LOCAL_PORT);
        ret = bind(socket_fd, (struct sockaddr *)&local_addr4, sizeof(struct sockaddr_in));
        if (ret == -1)
        {
            printf("[APP] Failed to bind socket\n");
            close(socket_fd);
            goto end_of_task;
        }

        srv_addr4->sin_family = AF_INET;
        srv_addr4->sin_addr.s_addr = inet_addr(tcpcl_conf.peer_ip_addr);
        srv_addr4->sin_port = htons(tcpcl_conf.peer_port);
        printf("[APP] Connecting(%s:%d)\n", tcpcl_conf.peer_ip_addr, tcpcl_conf.peer_port);
        ret = connect(socket_fd, (struct sockaddr *)srv_addr4, sizeof(struct sockaddr_in));
    }
#endif // __SUPPORT_IPV4__
#if defined ( __SUPPORT_IPV6__ )

    if (ip_type == IPADDR_TYPE_V6)
    {
        local_addr6.sin6_family = AF_INET6;
        local_addr6.sin6_addr = in6addr_any;
        local_addr6.sin6_port = htons(TCPC_DEF_LOCAL_PORT);
        ret = bind(socket_fd, (struct sockaddr *)&local_addr6, sizeof(struct sockaddr_in6));
        if (ret == -1)
        {
            printf("[APP] Failed to bind socket\n");
            close(socket_fd);
            goto end_of_task;
        }

        srv_addr6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, tcpcl_conf.peer_ip_addr, &srv_addr6->sin6_addr);
        srv_addr6->sin6_port = htons(tcpcl_conf.peer_port);
        printf("[APP] Connecting(%s:%d)\n", tcpcl_conf.peer_ip_addr, tcpcl_conf.peer_port);
        wait_ready_connect_ipv6();
        ret = connect(socket_fd, (struct sockaddr *)srv_addr6, sizeof(struct sockaddr_in6));
    }
#endif // __SUPPORT_IPV6__

    if (ret < 0)
    {
        printf("[APP] Failed to establish TCP session(%d,%d)\n", ret, errno);
        close(socket_fd);
        goto end_of_task;
    }

    printf("[APP] Connected(%s:%d)\n", tcpcl_conf.peer_ip_addr, tcpcl_conf.peer_port);

#if CFG_PMGR
    if (!p_conf->twt_session_active)
    {
        /* Hold sleep */
        RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
#endif

        vTaskDelay(pdMS_TO_TICKS(200));
        twt_setup_retry_cnt = 0;
        while ((!p_conf->twt_session_active) && (twt_setup_retry_cnt < 3))
        {
            twt_setup_retry_cnt++;
            WIFI_TwtSetup(&setup_params);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
            
 #if CFG_PMGR
        /* Resume sleep */
        RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
    }
#endif

    if (tcp_client_mgmt_evt)
    {
        /* Notify the starter task that TCP connection is done */
        xEventGroupSetBits(tcp_client_mgmt_evt, EVT_TCPC_CONN);
    }

#if CFG_PMGR
    if (tcpcl_conf.send_period)
    {
        /* TCP send timer callback may now execute and schedule Tx */
        RM_PMGR_W_dpm_wakeup_done(JOB_ID_SEND);
    }
#endif // CFG_PMGR

    remaining_data = TCPC_REMAIN_DATA_INIT;
    while (1)
    {
        memset(p_recv_buf, 0x00, recv_buflen);
        if (g_tcpc_send_cnt > 0)
        {
            g_tcpc_send_cnt--;
            len = send(socket_fd, p_send_buf, tcpcl_conf.send_data_size, 0);
            if (len <= 0)
            {
                printf("[APP] Failed to send data(%d:%d)\n", len, errno);
                if (errno == EHOSTUNREACH)
                {
#if CFG_PMGR
                    /* TCP send failure - Remove constraint */
                    RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
                    if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
                    {
                        p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
                    }
#endif // CFG_PMGR
                    /* Either TCPC is disconnected or WI-Fi is disconnected */
                    break;
                }
            }
            else
            {
                printf("[APP] Sending tcp data (%s) done !!\n", p_send_buf);
            }

#if CFG_PMGR

            if (g_tcpc_send_cnt == 0)
            {
                /* All TCP send are success - Remove constraint */
                RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
                if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
                {
                    p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
                }
            }
#endif // CFG_PMGR
            /* NOTE: user application logic for 'DONE (or failure)' case -
               user may want to call send() again if reply does not arrive, if then, add exception handler is required  */
        }

#if CFG_PMGR

        /* JOB_ID_RECV - ready to receive data */
        RM_PMGR_W_dpm_rcv_ready_set(JOB_ID_RECV);
#endif // CFG_PMGR

        if (remaining_data == TCPC_REMAIN_DATA_INIT)
        {
#if CFG_PMGR
            /* Constrain RAM till TCP recv done */
            RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
            if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
            {
                p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]++;
            }
#endif // CFG_PMGR
        }

        len = recv(socket_fd, p_recv_buf, recv_buflen, 0);
        if (len > 0)
        {
            p_recv_buf[len] = '\0';
            printf("< Read from server: (%d) bytes read\n", len);
        }
        else
        {
            if (errno != EAGAIN)
            {
                printf("[APP] Failed to receive data(%d,%d), closing tcp client ... \n", len, errno);
#if CFG_PMGR
                /* TCP recv failure - Remove constraint */
                RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
                if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
                {
                    p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
                }

#endif // CFG_PMGR

                /* Either TCPC is disconnected or WI-Fi is disconnected */
                break;
            }
        }
#if CFG_PMGR

        remaining_data = RM_PMGR_W_socket_rx_data_is_remaining(socket_fd);
        if (remaining_data == 0)
        {
            remaining_data = TCPC_REMAIN_DATA_INIT;

            /* TCP recv done - Remove constraint */
            RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
            if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
            {
                p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
            }
        }
#endif // CFG_PMGR

        BaseType_t result = xTaskNotifyWait(0, 0xFFFFFFFF, &notify_val, portCONVERT_MS_2_TICKS(500));

        if (result == pdPASS)
        {
            if (notify_val == eWiFiEventDisconnected)
            {
                printf("[APP] wifi disconnected event %lu\n", (unsigned long)notify_val);
                break;
            }
            else if (notify_val == eTWTCFG)
            {
                printf("[APP] Unactive TWT event\n");
                
                if (!p_conf->twt_session_active)
                {
                    twt_setup_retry_cnt = 0;
#if CFG_PMGR
                    /* Hold sleep */
                    RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
#endif
                    while ((!p_conf->twt_session_active) && (twt_setup_retry_cnt < 3))
                    {
                        twt_setup_retry_cnt++;
                        WIFI_TwtSetup(&setup_params);
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }

#if CFG_PMGR
                    /* Resume sleep */
                    RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
#endif

                }
            }
        }
    }

    close(socket_fd);
    
    if (tcpcl_conf.autostart_atexit == pdTRUE)
    {
#if CFG_PMGR

        /* Constrain RAM till TCP connection done */
        RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
        if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
        {
            p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]++;
        }
#endif // CFG_PMGR
    }

end_of_task:
#if CFG_PMGR

    /* JOB_ID_SEND - delete DPM timer */
    tcpc_deregister_timer();

    /* JOB_ID_RECV - delete port filter */
    RM_WIFI_dpm_tcp_port_delete(TCPC_DEF_LOCAL_PORT);
#endif // CFG_PMGR
    if (tcpcl_conf.autostart_atexit == pdTRUE)
    {
        if (tcp_client_mgmt_evt)
        {
            xEventGroupSetBits(tcp_client_mgmt_evt, EVT_TCPC_DISCONN);
        }
    }

    vTaskDelete(NULL);

    return;
}

static BaseType_t tcp_client_app_task_start(void)
{
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    while ((ra6w1_network_main_check_ip_addr(WLAN0_IFACE) == pdFALSE) || (!RM_WIFI_dpm_supp_is_connected()))
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
   
    return xTaskCreate(tcp_client_dpm_task,
                       JOB_ID_RECV,
                       (TCP_CLIENT_STACK_SIZE),
                       NULL,
                       (OS_TASK_PRIORITY_USER + 6),
                       &tcp_client_dpm_tasks_ptr);
}

void tcp_client_init()
{
#if !defined (__SUPPORT_MATTER_IOT__)
    EventBits_t events = 0;
    BaseType_t status = pdPASS;

    /* Event flag group init */
    if (tcp_client_mgmt_evt == NULL)
    {
        tcp_client_mgmt_evt = xEventGroupCreate();
        if (tcp_client_mgmt_evt == NULL)
        {
            printf(RED_COLOR "Event group Create Error!");
            goto TCPC_STARTER_END;
        }
    }

    g_restart_tcp_client_app = false;
    if (!g_restart_tcp_client_app)
    {
        pmgr_instance_ctrl_t *p_instance_ctrl = RM_PMGR_W_get_ctrl();

        /* Constrain RAM till TCP connection done */
        RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
        if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
        {
            p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]++;
        }
    }

    status = tcp_client_app_task_start();
    if (status != pdPASS)
    {
        goto TCPC_STARTER_END;
    }

    while (1)
    {
        events = xEventGroupWaitBits(tcp_client_mgmt_evt,
                                     EVT_TCPC_ANY,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);

        if (events & EVT_TCPC_DISCONN)
        {
            printf("[%s:%d] TCPC restarting ...\n", __func__, __LINE__);
            g_restart_tcp_client_app = true;
            WIFI_TwtTeardown(&teardown_params);
            p_conf->twt_session_active = 0;
            status = tcp_client_app_task_start();
            if (status != pdPASS)
            {
                printf("[%s:%d] tcp_client_task_start failed %ld\n", __func__, __LINE__, status);
                goto TCPC_STARTER_END;
            }
        }
        else if (events & EVT_TCPC_CONN)
        {
#if CFG_PMGR
            pmgr_instance_ctrl_t *p_instance_ctrl = RM_PMGR_W_get_ctrl();

            /* TCP connection done - Remove constraint */
            RM_PMGR_W_remove_sleep_constraint(p_instance_ctrl, PMGR_CONSTRAINT_POWER_RAM);
            if (p_instance_ctrl && p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
            {
                p_instance_ctrl->ram_counter_cause[PMGR_DBG_RAM_CONSTRAINT_TCP_CLIENT]--;
            }
#endif // CFG_PMGR
        }
        else if (events & EVT_TCPC_EXIT)
        {
            goto TCPC_STARTER_END;
        }
        else
        {
            printf("[APP] Unknown event! %lu \n", events);
        }
    }

TCPC_STARTER_END:
    if (tcp_client_mgmt_evt)
    {
        vEventGroupDelete(tcp_client_mgmt_evt);
        tcp_client_mgmt_evt = NULL;
    }
#endif  // !__SUPPORT_MATTER_IOT__

    vTaskDelete(NULL);

    return;
}
