/***********************************************************************************************************************
 * File Name    : provisioning.h
 * Description  : provisioning threads , functions, macros, struct and enum declarations
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef provisioning_H

 #define provisioning_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
 #include "rm_wifi.h"
 #include "r_spi_flash_api.h"
 #include "provisioning_cfg.h"
 #include "provisioning_api.h"
 #include <stdbool.h>

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER
/** Current provisioning application TLS type */
typedef struct st_provisioning_tls_info
{
    provisioning_tls_server_cfg_t *config;
    provisioning_type_t             provisioning_type;
} provisioning_tls_info_t;

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

 #define TCP_PROVISION_PORT_NUM      9999 /** Port for phone connection. Phone port should be changed same port number if this port is changed. */

 #define PROVISION_MSG_CMD           4    /** Max message number between receiving thread  and sending thread */

 #define PROVISION_TCP_RX_BUF_SZ     1024 /** Buffer size of receiving message from phone */

 #define APP_SOFTAP_PROV_NAME        "APROV_TCP"
 #define APP_SOFTAP_APROV_NAME       "APROV_TLS"
 #define APP_SOFTAP_PROV_STACK_SZ    1024

 #define MAX_AP_SSID_SIZE            128 /** Max SSID size for home AP */
 #define MAX_AP_PASSWORD_SIZE        128 /** Max password size for home AP */
 #define MAX_CUSTOMER_SERVER_SIZE    128 /** Max customer server side for home AP */

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/** Command sent from phone application */
typedef enum e_provisioning_ap_cmd_type
{
    PROV_COMMAND_ERROR = -1,            ///< Error
    CONNECTED          = 0,             ///< Connected to phone
    SET_AP_SSID_PW,                     ///< Set AP SSID and Password
    REQ_HOMEAP_RESULT,                  ///< Request Scan result
    REQ_RESCAN,                         ///< Request Rescan
    REQ_REBOOT,                         ///< Request RRQ device Reboot
    REQ_SET_DPM,                        ///< Request Set DPM
    REQ_SOCKET_TYPE,                    ///< Request for current socket type
    CMD_ERROR,                          ///< Command Error
} provisioning_ap_cmd_type_t;

/** DPM Setting value types */
typedef enum e_provisioning_dpm_value_type
{
    TYPE_SLEEP_MODE = 0,                ///< Sleep mode
    TYPE_USE_DPM,                       ///< Use DPM or Not
    TYPE_RTC_TIME,                      ///< RTC Time
    TYPE_DPM_KEEP_ALIVE,                ///< DPM Keep Alive
    TYPE_USER_WAKE_UP,                  ///< User Wake Up Time
    TYPE_TIM_WAKE_UP,                   ///< Timer Wake Up Time
} provisioning_dpm_value_type_t;

/** Message structure between receiving thread / sending thread */
typedef struct st_provisioning_user_data
{
    provisioning_ap_cmd_type_t status;   ///< Command sent from phone application
    void *userData;                        ///< User data
} provisioning_user_data_t;

/***********************************************************************************************************************
 * Public APIs
 **********************************************************************************************************************/
fsp_err_t init_wifi_provisioning(void);
fsp_err_t provisioning_RebootAPMode(long const ap_mode_flag,
                                    long const security_mode_flag,
                                    long const factory_reset_flag);
fsp_err_t provisioning_RebootStationMode(provisioning_param_t const *const p_param,
                                         int32_t factory_reset_flag);
fsp_err_t provisioning_Scan(int32_t option,
                            provisioning_type_t provisioning_mode);
fsp_err_t provisioning_Ping(char *p_ip_address);
fsp_err_t provisioning_NotifyConnectionOnConcurrent(int concurrent_status,
                                                    char *p_buf);
                                                       
int provision_app_gpio_handle_create_event(void);
void provision_app_gpio_handle_task_start(void);
void execute_AP_profile(void);
fsp_err_t provisioning_reboot_ap_mode(long const ap_mode_flag,
                                      long const security_mode_flag,
                                      long const factory_reset_flag);

/** Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER

#endif

