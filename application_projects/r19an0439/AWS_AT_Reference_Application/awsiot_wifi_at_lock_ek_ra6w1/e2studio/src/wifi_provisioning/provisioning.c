/***********************************************************************************************************************
* File Name    : provisioning.c
* Description  : provisioning threads and functions
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "provisioning.h"
#include "provisioning_api.h"
#include "provisioning_helper.h"
/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "rm_wifi.h"                  /* Socket and WiFi interface includes. */
#include "rm_vee_flash_w_rrq_nvram.h"
#include "cJSON.h"
#include "lwip/sockets.h"
#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#include dg_configADNVPARAM_PROJ_FILE
#endif
#if defined(__SUPPORT_AWS_IOT_W__) && defined(__PROVISION_ATCMD__)
#include "app_aws_atcmd_parse.h"
#endif  //__SUPPORT_AWS_IOT_W__ && __PROVISION_ATCMD__

#ifdef __SUPPORT_USR_NVRAM__
#include "api_usr_nvram.h"
#endif  //__SUPPORT_USR_NVRAM__

/***********************************************************************************************************************
 * Defines
 **********************************************************************************************************************/
#define PROVISIONING_OPEN                   (0X50524F56ULL)
#define provisioning_CLOSE                  (0)
#define provisioning_TASK_NAME              "customer_provision"
#define provisioning_TASK_SIZE              256
#define provisioning_NVRAM_CFG_THINGNAME    AWSIOT_CFG_THINGNAME
#define provisioning_NVRAM_CFG_SLEEPMODE    AWSIOT_CFG_DPM_SLEEP_MODE        ///< sleep mode value on NVRAM
#define provisioning_NVRAM_CFG_RTC_TIME     AWSIOT_CFG_SLEEP_MODE2_RTC_TIME  ///< wakeup timer interval by seconds on NVRAM
#define provisioning_NVRAM_CFG_USE_DPM      AWSIOT_CFG_USE_DPM               ///< use DPM or not on NVRAM

#if !defined(PROV_OAL_MSLEEP)
#define PROV_OAL_MSLEEP(wtime)              {                                                   \
                                                portTickType xFlashRate, xLastFlashTime;        \
                                                uint32_t mtime = wtime;                         \
                                                if (wtime < 10) mtime = 1;                      \
                                                    xFlashRate = mtime/portTICK_RATE_MS;        \
                                                xLastFlashTime = xTaskGetTickCount();           \
                                                vTaskDelayUntil( &xLastFlashTime, xFlashRate ); \
                                            }
#endif

typedef SemaphoreHandle_t provisioning_mutex_t;

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static fsp_err_t provisioning_initialize(provisioning_type_t provisioning_type);
static fsp_err_t provisioning_set_ap_config(int32_t security_mode);
static fsp_err_t provisioning_set_ssid_without_mac(char *prefix,
                                                   int interface,
                                                   int quotation,
                                                   char *ssid,
                                                   int size);

static int provisioning_dpm_status_get(provisioning_dpm_value_type_t value_type);
static fsp_err_t provisioning_dpm_status_set(int sleep_mode,
                                             int dpm_mode,
                                             int rtc_time,
                                             int dpm_keep_alive_time,
                                             int user_wakeup_time,
                                             int tim_wakeup_time);

static fsp_err_t provisioning_reset_ap_to_station(char *p_ssid,
                                                  char *p_password,
                                                  provisioning_ap_security_t prov_security,
                                                  provisioning_dpm_mode_t dpm_mode,
                                                  bool is_ssid_hidden);

static fsp_err_t provisioning_add_mode_to_json(cJSON *p_root_json, provisioning_type_t device_mode);
static fsp_err_t provisioning_add_thing_name_to_json(cJSON *p_root_json, char *p_thing_name);
static char * provisioning_add_command_to_json(char *p_command, int32_t command_type);


static fsp_err_t provisioning_send_tcp_data(char *p_data, int32_t data_size);
static fsp_err_t provisioning_send_tls_data(provisioning_tls_server_cfg_t *p_config,
                                            char *p_data,
                                            int32_t data_size);
static fsp_err_t provisioning_send_tcp_data_as_json(char *p_data, int32_t command_type);
static fsp_err_t provisioning_send_tls_data_as_json(provisioning_tls_server_cfg_t *p_config,
                                                    char *p_data,
                                                    int32_t command_type);

static fsp_err_t provisioning_create_ap_list_json(cJSON *p_root_json);
static fsp_err_t provisioning_delete_ap_list_json(cJSON *p_ap_list);

static provisioning_ap_cmd_type_t provisioning_parse_json_data(const char *p_received_data);
static provisioning_socket_type_t provisioning_get_socket_type(void);
static fsp_err_t provisioning_send_msg_to_client(provisioning_user_data_t *p_send_data);
static provisioning_ap_cmd_type_t provisioning_recv_msg_from_server(void);

static void provisioning_run_tls_server(provisioning_tls_server_cfg_t *config);
static void provisioning_client_thread(void *tls_info);
static void provisioning_tls_server_thread(void *tls_info);
static void provisioning_tcp_server_thread(void *tls_info);
static void provisioning_start_thread();
static fsp_err_t provisioning_add_ap_to_json(provisioning_scan_callback_args_t *p_args);

static void provisioning_get_app_thing_name();
static int provisioning_get_fleet_provisioning_status();
static char * provisioning_initialize_string(const char *string_data);
static void provisioning_get_fleet_provisioning_thing_name();

static void provisioning_mutex_init(provisioning_mutex_t *p_mutex, char *p_name);
static int32_t provisioning_mutex_lock(provisioning_mutex_t *p_mutex);
static int32_t provisioning_mutex_unlock(provisioning_mutex_t *p_mutex);

extern void execute_AP_profile(void);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
/*
 * @brief DPM mode setting
 */
int     sleep_mode_value          = 0;
int     dpm_mode_value            = 0; // If dpm_mode_value == 3, DPM mode is enabled.
int     dpm_keep_alive_time_value = 0;
int     user_wakeup_time_value    = 0;
int     tim_wakeup_time_value     = 0;
int     rtc_time_value            = 0;
cJSON *scan_ap_list_json;
char  *ap_list_buffer = NULL;

int ap_conn_status      = -1;
int station_conn_status = -1;
int try_connect_on_sta  = 0;

/*!TCP socket point for provisioning TCP communication   */
int provision_TCP_socket  = -1;
int provision_TCP_csocket = -1;

provisioning_mutex_t provision_mutex;

/**
 * @var SSID received from phone
 */
char *new_SSID = NULL;

/**
 * @var SSID size received from phone
 */
int32_t new_SSID_size = 0;

/**
 * @var password of SSID received from phone
 */
char *new_pw = NULL;

/**
 * @var password size of SSID received from phone
 */
int32_t new_pw_size = 0;

/**
 * @var server url received from phone
 */
char *server_URL = NULL;

/**
 * @var password size of SSID received from phone
 */
int32_t server_URL_size = 0;

/**
 * @var HIdden SSID connection flag received from Mobile App.
 */
bool use_hidden_ssid = false;

/**
 * @var Security Type received from Mobile App.
 */
provisioning_ap_security_t security_type = eAPSecurityNotSupported;

/**
 * @var EAP Type received from Mobile App.
 */
provisioning_eap_type_t eap_type = TYPE_EAP_DEFAULT;

/**
 * @var EAP Protocol received from Mobile App.
 */
provisioning_eap_protocol_t eap_protocol = PROTO_EAP_PHASE2_MIX;

/**
 * @var EAP ID received from phone
 */

char *eap_ID = NULL;

/**
 * @var EAP Password received from phone
 */

char *eap_PW = NULL;

/* Socket that we are using */ 
provisioning_socket_type_t socket_app = PROV_TLS_SOCKET;
provisioning_socket_type_t socket_dev = PROV_TLS_SOCKET;

/* Message queue between receiving thread and sending thread */
QueueHandle_t provision_message_queue = NULL; // static
static provisioning_tls_server_cfg_t provisioning_tls_svr_config = {0, };
int32_t check_connection_ap = 0;

provisioning_feature_t g_provisioning_feature;
provisioning_type_t    g_provisioning_type;

/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/
extern softap_config_t *ap_config_param; // Pointer to the structure for AP mode setting
extern bool reset(int flag);

/***********************************************************************************************************************
 * Static Globals
 **********************************************************************************************************************/
static TaskHandle_t gs_provisioning_task_handle = NULL;
static char *gs_app_thing_name;
static char gs_app_fleet_thing_name[128] = {0, };


/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
fsp_err_t init_wifi_provisioning (void)
{
    fsp_err_t err = FSP_SUCCESS;

    printf("start AP profile load\n");
    cJSON_Hooks hooks =
    {
        .malloc_fn = pvPortMalloc,
        .free_fn   = vPortFree
    };
    cJSON_InitHooks(&hooks);
    execute_AP_profile();
    /* Create PROVISIONING_W Task */
    err = xTaskCreate(provisioning_start_thread,
                      (const char *) provisioning_TASK_NAME,
                      provisioning_TASK_SIZE,
                      NULL,
                      (OS_TASK_PRIORITY_USER + 1),
                      &gs_provisioning_task_handle);

    if (pdPASS != err)
    {
        err = FSP_ERR_OUT_OF_MEMORY;
    }
    else
    {
        err = FSP_SUCCESS;
    }

    return err;
}

fsp_err_t provisioning_reboot_ap_mode(long const ap_mode_flag,
                                      long const security_mode_flag,
                                      long const factory_reset_flag)
{
    fsp_err_t err = FSP_SUCCESS;

    char default_ssid[PROV_MAX_SSID_LEN + 3];
    char default_psk[PROV_MAX_PW_LEN + 3];
    char *app_thing_name = NULL;
    char *char_buffer = NULL;
    int buffer_size = 0;
    char *tmp_thing = NULL;

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, provisioning_NVRAM_CFG_THINGNAME, &tmp_thing);
#else
    tmp_thing = read_nvram_appcfg_string(provisioning_NVRAM_CFG_THINGNAME);
#endif
    if (NULL != tmp_thing)
    {
        provisioning_get_app_thing_name();
        char_buffer = gs_app_thing_name;
        buffer_size = strlen(char_buffer);

        app_thing_name = (char *) pvPortMalloc(buffer_size + 1);
        memset(app_thing_name, 0x00, buffer_size + 1);
        strcpy(app_thing_name, char_buffer);
    }

    if (1 == factory_reset_flag)
    {
        printf("\nFactory Reset\n");
        factory_reset(0);
    }

    vTaskDelay(portCONVERT_MS_2_TICKS(100)); // Temporary delay due to lack of API catching reset finish
    printf("\n" ANSI_COLOR_RED "Setting RA6W1/RA6W2 for Provisioning, AP mode = %ld ", ap_mode_flag);
    switch (ap_mode_flag)
    {
        case WIFI_DEVICE_MODE_EXT_AP:
        {
            printf("(\"AP_ONLY\")....\n");
            break;
        }

        case WIFI_DEVICE_MODE_EXT_AP_STATION:
        {
            printf("(\"STA_N_AP\")....\n");
            break;
        }

        default:
        {
            printf("(\"???\")....\n");
            break;
        }
    }

    if (app_thing_name != NULL)
    {
#if !(defined(__SUPPORT_AWS_IOT_W__) && defined(__PROVISION_ATCMD__))
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_APPCFG,
                                         (const char *) provisioning_NVRAM_CFG_THINGNAME,
                                         app_thing_name);
#endif
#endif
        printf(".");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));

        vPortFree(app_thing_name);
    }

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SYSMODE, (int) DFLT_SYSMODE);
    set_sys_mode((int)eWiFiModeAP);
#endif
    printf(".");
    vTaskDelay(portCONVERT_MS_2_TICKS(10));

    /*
     * Run customer's configuration
     */
    provisioning_set_ap_config(security_mode_flag);

    if (MODE_DISABLE == ap_config_param->customer_cfg_flag)
    {
        printf("\nRebootAPMode on MODE_DISABLE ...\n");
    }
    else                               // Customer configuration
    {
        char tmp_buf[128] = {0, };

        printf("\nRebootAPMode setting up Customer configuration ...\n");

        /* Prefix + Mac Address */
        if (strlen(ap_config_param->ssid_name) > 0)
        {
            strncpy(tmp_buf, ap_config_param->ssid_name, PROV_MAX_SSID_LEN);
        }
        else
        {
            sprintf(tmp_buf, "%s", CHIPSET_NAME);
        }

        memset(default_ssid, 0, PROV_MAX_SSID_LEN + 3);

        /* Set default_ssid to Predifined SSID */
        err = provisioning_set_ssid_without_mac(tmp_buf, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid));
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

        /* PSK */
        memset(default_psk, 0, PROV_MAX_PW_LEN + 3);

        if (strlen(ap_config_param->psk) > 0)
        {
            sprintf(default_psk, "\"%s\"", ap_config_param->psk);

        }
        else
        {
            sprintf(default_psk, "12345678");
        }

#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_WIFIPROFILE,
                                         (const char *) WIFI_PROFILE_SSID_1,
                                         default_ssid);
#endif

        printf(".");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));

        printf("\nSetting default_ssid = %s ..., ap_config_param->ssid_name=%s\n",
               default_ssid,
               ap_config_param->ssid_name);

        /* Auth Type */
        if (strlen(ap_config_param->psk) > 0)
        {
            char tmp_psk[MAX_PASSKEY_LEN + 3];
            memset(tmp_psk, 0, MAX_PASSKEY_LEN + 3);
            tmp_psk[0] = 0x22;
            strcpy(&tmp_psk[1], ap_config_param->psk);
            tmp_psk[strlen(ap_config_param->psk) + 1] = 0x22;


#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_ENCKEY_1,
                                             tmp_psk);
            printf("\n PW = %s \n", ap_config_param->psk);
            char security = eWiFiSecurityWPA; // 2
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_1,
                                          security);
#endif
            printf("\n PW = %s  completed\n", ap_config_param->psk);
        }

#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_WIFIPROFILE,
                                      WIFI_PROFILE_NETMODE_1,
                                      (int) DFLT_NETMODE_1);
#endif
        printf(".");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_WIFIPROFILE,
                                      WIFI_PROFILE_BAND,
                                      (int) WPA_SETBAND_AUTO);
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_WIFIPROFILE,
                                      WIFI_PROFILE_CHANNEL,
                                      (int) DFLT_AP_CHANNEL);

#endif
        printf(".");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));

        if (strlen(ap_config_param->country_code) > 0)
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_COUNTRY_CODE,
                                             ap_config_param->country_code);
#endif
        }
        else
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_COUNTRY_CODE,
                                             (const char *) DFLT_AP_COUNTRY_CODE);
#endif
        }

        printf("."); vTaskDelay(portCONVERT_MS_2_TICKS(10));

        if (IPADDR_CUSTOMER == ap_config_param->customer_ip_address)
        {
            printf("\nRebootAPMode setting IPADDR_CUSTOMER...\n");

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_IPADDR_1,
                                             ap_config_param->ip_addr);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_NETMASK_1,
                                             ap_config_param->subnet_mask);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_GATEWAY_1,
                                             ap_config_param->default_gw);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *) WIFI_PROFILE_DNSSVR_1,
                                             ap_config_param->dns_ip_addr);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));
        }
        else
        {
            printf("\nRebootAPMode has no IPADDR_CUSTOMER set\n");
        }

        if (DHCPD_CUSTOMER == ap_config_param->customer_dhcpd_flag)
        {
            printf("\nRebootAPMode setting DHCPD_CUSTOMER...\n");

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_SYSCFG,
                                          (const char *) NVR_KEY_DHCPD,
                                          MODE_ENABLE);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_SYSCFG,
                                          (const char *) NVR_KEY_DHCP_TIME,
                                          ap_config_param->dhcpd_lease_time);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_SYSCFG,
                                             (const char *) NVR_KEY_DHCP_S_IP,
                                             ap_config_param->dhcpd_start_ip);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_SYSCFG,
                                             (const char *) NVR_KEY_DHCP_E_IP,
                                             ap_config_param->dhcpd_end_ip);
#endif
            printf(".");
            vTaskDelay(portCONVERT_MS_2_TICKS(10));
        }
        else
        {
            printf("\nRebootAPMode has no DHCPD_CUSTOMER Set\n");
        }
    }

    printf("\nOK\n");
    printf(ANSI_COLOR_DEFULT "\n\n");
    vTaskDelay(portCONVERT_MS_2_TICKS(30));

    reset(0);

    /*
     * Wait for system-reboot
     */
    while (1)
    {
        vTaskDelay(portCONVERT_MS_2_TICKS(100));
    }

    return err;
}

static fsp_err_t provisioning_reboot_station_mode (provisioning_param_t const * const p_param,
                                                   int32_t factory_reset_flag)
{
    fsp_err_t err = FSP_SUCCESS;
    char *app_thing_name = NULL;
    char tmp_psk[PROV_MAX_PW_LEN + 3];
    char *char_buffer = NULL;
    int buffer_size = 0;
    char *tmp_thing = NULL;

    if (NULL == p_param)
    {
        printf("\n\n>> passed parameter invalid(NULL) \n");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ASSERT(NULL != p_param);

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, provisioning_NVRAM_CFG_THINGNAME, &tmp_thing);
#else
    tmp_thing = read_nvram_appcfg_string(provisioning_NVRAM_CFG_THINGNAME);
#endif
    if (NULL != tmp_thing)
    {
        provisioning_get_app_thing_name();
        char_buffer = gs_app_thing_name;
        buffer_size = strlen(char_buffer);

        app_thing_name = (char *) pvPortMalloc(buffer_size + 1);
        memset(app_thing_name, 0x00, buffer_size + 1);
        strcpy(app_thing_name, char_buffer);
    }

    if (1 == factory_reset_flag)
    {
        if (PROV_FEATURE_ATCMD != g_provisioning_feature)
        {
            printf("\n\nfactory_reset ...\n");
            factory_reset(0);
        }

        vTaskDelay(portCONVERT_MS_2_TICKS(100));
    }
    else
    {
        /* Clear All NVRAM area */
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Erase_GROUP(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG);
#endif

        printf("\n>> Initialize NVRAM done ...\n");
        vTaskDelay(portCONVERT_MS_2_TICKS(500));
    }

    printf("\n" ANSI_COLOR_RED "\n\nRA6W1/RA6W2 Provisioning connecting to AP station\n");

    /* Re-write Thing name */
    if (app_thing_name != NULL)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_APPCFG,
                                         (const char *) provisioning_NVRAM_CFG_THINGNAME,
                                         app_thing_name);
#endif
        printf(".");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));

        vPortFree(app_thing_name);
    }

    /* For Wi-Fi connection */
    if (eAPSecurityOpen == p_param->auth_type)
    {
        printf("\n\n>>> reboot station mode ssid:%s \n", p_param->ssid);
    }
    else
    {
        printf("\n\n>>> reboot station mode ssid:%s , pw:%s\n",
               p_param->ssid,
               p_param->psk[0] > 0 ? p_param->psk : "???");
    }

    if (strlen(p_param->ssid) > 0)       // SSID
    {
        char tmp_ssid[PROV_MAX_SSID_LEN + 3];

        memset(tmp_ssid, 0, PROV_MAX_SSID_LEN + 3);
        tmp_ssid[0] = 0x22;
        strcpy(&tmp_ssid[1], p_param->ssid);
        tmp_ssid[strlen(p_param->ssid) + 1] = 0x22;

#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_WIFIPROFILE,
                                         WIFI_PROFILE_SSID_0,
                                         tmp_ssid);
#endif
    }

    if (strlen(p_param->psk) > 0)        // PW
    {
        memset(tmp_psk, 0, PROV_MAX_PW_LEN + 3);
        tmp_psk[0] = 0x22;
        strcpy(&tmp_psk[1], p_param->psk);
        tmp_psk[strlen(p_param->psk) + 1] = 0x22;
    }
    printf("\n>>> set auth_type : %d \n", p_param->auth_type); // AUTH
    switch (p_param->auth_type)
    {
        case eAPSecurityOpen:          //< NONE
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityOpen_ext);

#endif
            break;
        }

        case eAPSecurityWEP:           ///< WEP
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityWEP_ext);
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, NVR_KEY_WEPINDEX_0, 0);
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             WIFI_PROFILE_WEPKEY0_0,
                                             tmp_psk);
#endif
            break;
        }

        case eAPSecurityWPA:           ///< WPA-PSK
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityWPA_ext);
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             WIFI_PROFILE_ENCKEY_0,
                                             tmp_psk);
#endif
            break;
        }

        case eAPSecurityWPA2:          ///< WPA2-PSK
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0, eWiFiSecurityWPA2_ext);
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0,tmp_psk);
#endif
            break;
        }

        case eAPSecurityOWE:           ///< WPA3 OWE
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityWPA3_OWE_ext);
#endif
            break;
        }

        case eAPSecuritySAE:           ///< WPA3 SAE
        case eAPSecurityRSN_SAE:       ///< WPA2 (RSN) & WPA3 SAE
        {
            if (eAPSecuritySAE == p_param->auth_type)
            {
#ifdef RM_MAP_PERSISTANT_W
                    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                                  WIFI_PROFILE_SECURITY_0,
                                                  eWiFiSecurityWPA3_ext);
                    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_PMF, 2);
#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                                  WIFI_PROFILE_SECURITY_0,
                                                  eWiFiSecurityWPA2_WPA3_ext);
                    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_PMF, 1);
#endif
            }

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, NVR_KEY_PROTO_0, proto_RSN);
#endif
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "N0_wep_key0");
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "N0_wep_key1");
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "N0_wep_key2");
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "N0_wep_key3");
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, NVR_KEY_WEPINDEX_0);
#endif

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             WIFI_PROFILE_ENCKEY_0,
                                             tmp_psk);

#endif
            break;
        }

        case eAPSecurityWPA_EAP:       ///< WPA Enterprise
        case eAPSecurityWPA2_EAP:      ///< WPA2 Enterprise
        case eAPSecurityWPA_AUTO_EAP:  ///< WPA & WPA2 Enterprise
        case eAPSecurityWPA3_EAP:      ///< WPA3 Enterprise
        case eAPSecurityWPA2_AUTO_EAP: ///< WPA2 & WPA3 Enterprise
        {
            if (strlen(p_param->eap_identity) > 0) // eap_id
            {
                char eap_id[PROV_MAX_SSID_LEN + 3];

                memset(eap_id, 0, PROV_MAX_SSID_LEN + 3);
                eap_id[0] = 0x22;
                strcpy(&eap_id[1], p_param->eap_identity);
                eap_id[strlen(p_param->eap_identity) + 1] = 0x22;

#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                 ENV_GROUP_WIFIPROFILE,
                                                 "N0_identity", eap_id);
#endif
            }

            if (strlen(p_param->eap_password) > 0) // eap_pw
            {
                char eap_pw[PROV_MAX_PW_LEN + 3];
                memset(eap_pw, 0, PROV_MAX_PW_LEN + 3);
                eap_pw[0] = 0x22;
                strcpy(&eap_pw[1], p_param->eap_password);
                eap_pw[strlen(p_param->eap_password) + 1] = 0x22;

#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, "N0_password", eap_pw);//TODO
#endif
            }

            /* wpa_eap */
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityWPA_ent_ext);
#endif
            if (p_param->auth_type == eAPSecurityWPA_EAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0,
                                              eWiFiSecurityWPA_ent_ext);
#endif
            }
            else if (p_param->auth_type == eAPSecurityWPA2_EAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_SECURITY_0,
                                              eWiFiSecurityWPA2_ent_ext);
#endif
            }
            else if (p_param->auth_type == eAPSecurityWPA_AUTO_EAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_SECURITY_0,
                                              eWiFiSecurityWPA_WPA2_ent_ext);
#endif
            }
            else if (p_param->auth_type == eAPSecurityWPA2_AUTO_EAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_SECURITY_0,
                                              eWiFiSecurityWPA2_WPA3_ent_ext);
#endif
            }
            else if (p_param->auth_type == eAPSecurityWPA3_EAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_SECURITY_0,
                                              eWiFiSecurityWPA3_ent_ext);
#endif
            }
            else                       // eAPSecurityWPA_AUTO_EAP
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, NVR_KEY_PROTO_0);
#endif
            }

            /* Phase 1 auth type */
            if (p_param->eap_type == TYPE_EAP_DEFAULT)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, ENT_AUTH_TYPE_PEAP_TTLS_FAST);
#endif
            }
            else if (p_param->eap_type == TYPE_EAP_PEAP)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, ENT_AUTH_TYPE_PEAP);
#endif
            }
            else if (p_param->eap_type == TYPE_EAP_TTLS)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, ENT_AUTH_TYPE_TTLS);
#endif
            }
            else if (p_param->eap_type == TYPE_EAP_FAST)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, ENT_AUTH_TYPE_FAST);
#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_EAP_AUTH_MODE, ENT_AUTH_TYPE_TLS);
#endif
            }

            /* Erase old fast_pac, fast_pac_len for EAP-FAST */
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "fast_pac");
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFICFG, "fast_pac_len");
#endif

            /* Phase 2 auth protocol */
            if (p_param->eap_protocol == PROTO_EAP_PHASE2_MIX)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_EAP_PHASE2,
                                              ENT_AUTH_PROTO_MSCHAPv2_GTC);
#endif
            }
            else if (p_param->eap_protocol == PROTO_EAP_MSCHAPV2)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_EAP_PHASE2,
                                              ENT_AUTH_PROTO_MSCHAPv2);
#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_EAP_PHASE2,
                                              ENT_AUTH_PROTO_GTC);
#endif
            }

            break;
        }

        case eAPSecurityWPA_AUTO:
        default:
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_SECURITY_0,
                                          eWiFiSecurityWPA_WPA2_ext);
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0,tmp_psk);
#endif
            break;
        }
    }

    /* Hidden SSID */
    if (p_param->hidden)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_HIDDEN_SSID, 1);
#endif
    }

    /* For System Running mode */
#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SYSMODE, WIFI_DEVICE_MODE_EXT_STATION);
    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_WIFI_MODE, WIFI_DEVICE_MODE_EXT_STATION);
    WIFI_SetModeExt((e_wifi_device_mode_ext_t)WIFI_DEVICE_MODE_EXT_STATION);
    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),ENV_GROUP_WIFIPROFILE,
                                  WIFI_PROFILE_SYS_MODE, WIFI_DEVICE_MODE_EXT_STATION);
#endif

    /* Country */
    if (strlen(p_param->country) > 0)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_WIFIPROFILE,
                                         WIFI_PROFILE_COUNTRY_CODE,
                                         p_param->country);
#endif
    }
    else
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                         ENV_GROUP_WIFIPROFILE,
                                         WIFI_PROFILE_COUNTRY_CODE,
                                         DFLT_STA_COUNTRY_CODE);
#endif
    }

    /* Static or DHCP */
    if (1 == p_param->ip_addr_mode)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_SYSCFG,
                                      NVR_KEY_NETMODE_0,
                                      ENABLE_STATIC_IP);
#endif
    }
    else
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_SYSCFG,
                                      NVR_KEY_NETMODE_0,
                                      ENABLE_DHCP_CLIENT);
#endif
    }

    /* For SNTP client */
    if (TRUE == p_param->sntp_flag)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_SNTP_C, p_param->sntp_flag);
#endif

        if (strlen(p_param->sntp_server) > 0)
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_SYSCFG,
                                             NVR_KEY_SNTP_SERVER_DOMAIN,
                                             p_param->sntp_server);
#endif
        }
        else
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_SYSCFG,
                                             NVR_KEY_SNTP_SERVER_DOMAIN,
                                             DFLT_SNTP_SERVER_DOMAIN);
#endif
        }

        if (p_param->sntp_period > 0)
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_SYSCFG,
                                          NVR_KEY_SNTP_SYNC_PERIOD,
                                          p_param->sntp_period);
#endif
        }
        else
        {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_SYSCFG,
                                          NVR_KEY_SNTP_SYNC_PERIOD,
                                          DFLT_SNTP_SYNC_PERIOD);
#endif
        }
    }

    /* For DPM mode */
    if (PROV_DPM_ON == p_param->dpm_mode)
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENABLE_DPM, p_param->dpm_mode);
#endif

        if (provisioning_dpm_status_get(TYPE_SLEEP_MODE) != 3)
        {
            if (p_param->dpm_ka > 0)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME,
                                              p_param->dpm_ka);
#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME,
                                              DFLT_DPM_KEEPALIVE_TIME);
#endif
            }

            if (p_param->dpm_user_wu > 0)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_USER_WAKEUP_TIME,
                                              p_param->dpm_user_wu);
#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_USER_WAKEUP_TIME,
                                              MIN_DPM_USER_WAKEUP_TIME);
#endif
            }

            if (p_param->dpm_tim_wu > 0)
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT,
                                              p_param->dpm_tim_wu);

#endif
            }
            else
            {
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_WIFIPROFILE,
                                              WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT,
                                              DFLT_DPM_TIM_WAKEUP_COUNT);
#endif
            }
        }
        else
        {
            printf("[SleepMode 3] Set DPM , KA : %d , UW : %d , TW : %d \n",
                   provisioning_dpm_status_get(TYPE_DPM_KEEP_ALIVE),
                   provisioning_dpm_status_get(TYPE_USER_WAKE_UP),
                   provisioning_dpm_status_get(TYPE_TIM_WAKE_UP));

#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME,
                                          provisioning_dpm_status_get(TYPE_DPM_KEEP_ALIVE));
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_DPM_USER_WAKEUP_TIME,
                                          provisioning_dpm_status_get(TYPE_USER_WAKE_UP));
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                          ENV_GROUP_WIFIPROFILE,
                                          WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT,
                                          provisioning_dpm_status_get(TYPE_TIM_WAKE_UP));
#endif
        }
    }
    else
    {
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENABLE_DPM, 0);
#endif
    }

    if (p_param->prov_type > (int8_t) WIFI_DEVICE_MODE_EXT_NOT_SUPPORTED)
    {
        printf(">>> Save Sleep mode to %d \n", provisioning_dpm_status_get(TYPE_SLEEP_MODE));
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_APPCFG,
                                      provisioning_NVRAM_CFG_SLEEPMODE,
                                      provisioning_dpm_status_get(TYPE_SLEEP_MODE));
#endif

        printf(">>> Save Sleep mode2 RTC time %d \n", provisioning_dpm_status_get(TYPE_RTC_TIME));
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                      ENV_GROUP_APPCFG,
                                      provisioning_NVRAM_CFG_RTC_TIME,
                                      provisioning_dpm_status_get(TYPE_RTC_TIME));
#endif
    }

    RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, 1);

    /* Check Reboot status */
    if (TRUE == p_param->auto_restart_flag)
    {
        printf(">>> System reboot ..." ANSI_COLOR_DEFULT "\n");
        vTaskDelay(portCONVERT_MS_2_TICKS(30));

        reset(0);                      // reboot;

        while (1)
        {
            vTaskDelay(portCONVERT_MS_2_TICKS(100));
        }
    }

    return err;
}

static void update_scan_array(WIFIScanResult_t *scan_data, provisioning_scan_callback_args_t *scan_res_array)
{
    int scan_res_num = -1;

    if (scan_data)
    {
        for (scan_res_num = 0; scan_res_num < SCAN_RESULT_MAX;)
        {
            scan_res_array[scan_res_num].frequency = scan_data[scan_res_num].ucChannel;
            scan_res_array[scan_res_num].signal_strength = scan_data[scan_res_num].cRSSI;
            memcpy(scan_res_array[scan_res_num].p_ssid, scan_data[scan_res_num].ucSSID, strlen((const char *)scan_data[scan_res_num].ucSSID));

            switch ((WIFISecurityExt_t)scan_data[scan_res_num].xSecurity)
            {
                case eWiFiSecurityOpen_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityOpen; /* OPEN */
                break;
                case eWiFiSecurityWEP_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWEP;  /* WEP */
                break;
                case eWiFiSecurityWPA_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA; /* WPA */
                break;
                case eWiFiSecurityWPA2_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA2;     /* WPA2 / OWE */
                break;
                case eWiFiSecurityWPA2_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA2_EAP;   /* WPA2 Enterprise */
                break;
                case eWiFiSecurityWPA3_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecuritySAE;    /* WPA3 SAE */
                break;
                case eWiFiSecurityWPA_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA_EAP;    /* WPA Enterprise */
                break;
                case eWiFiSecurityWPA_WPA2_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA_AUTO_EAP;   /* WPA & WPA2 Enterprise */
                break;
                case eWiFiSecurityWPA2_WPA3_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA2_AUTO_EAP;  /* WPA2 & WPA3 Enterprise */
                break;
                case eWiFiSecurityWPA3_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA3_EAP;   /* WPA3 Enterprise */
                break;
                case eWiFiSecurityWPA3_192B_ent_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA3_EAP_192B;  /* WPA3 192B Enterprise */
                break;
                case eWiFiSecurityWPA_WPA2_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityWPA_AUTO;   /* WPA & WPA2 (RSN) */
                break;
                case eWiFiSecurityWPA2_WPA3_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityRSN_SAE;  /* WPA2 & WPA3 Enterprise */
                break;
                case eWiFiSecurityWPA3_OWE_ext:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityOWE;    /* WPA3 OWE */
                break;
                default:
                    scan_res_array[scan_res_num].security_mode = eAPSecurityNotSupported;   /* eAPSecurityNotSupported */
                break;
            }

            printf("(%d) %s / %d / %d / %d \n",
                   scan_res_num,
                   scan_res_array[scan_res_num].p_ssid,
                   scan_res_array[scan_res_num].security_mode,
                   scan_res_array[scan_res_num].signal_strength,
                   scan_res_array[scan_res_num].frequency);

            scan_res_num++;
            }

        }

    for (int idx = 0; idx < scan_res_num; idx++)
    {
        scan_res_array[idx].is_listed = true;
        provisioning_add_ap_to_json(&scan_res_array[idx]);
    }
}

static fsp_err_t start_wifi_scan(int32_t scan_mode, provisioning_type_t provisioning_mode)
{
    WIFIReturnCode_t wifi_err;
    WIFIScanResult_t *wifi_scan_data;
    fsp_err_t err = FSP_SUCCESS;
    cJSON *root_ap_list;
    provisioning_scan_callback_args_t *scan_res_array;

    RA6W1_UNUSED_ARG(scan_mode);
    root_ap_list = cJSON_CreateObject();
    if (NULL == root_ap_list)
    {
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    provisioning_get_app_thing_name();
    err = provisioning_add_thing_name_to_json(root_ap_list, gs_app_thing_name);
    err = provisioning_add_mode_to_json(root_ap_list, provisioning_mode);
    err = provisioning_create_ap_list_json(root_ap_list);

    scan_res_array = (provisioning_scan_callback_args_t *)pvPortMalloc(sizeof(provisioning_scan_callback_args_t) * SCAN_RESULT_MAX);
    if (NULL == scan_res_array)
    {
        printf("[%s] malloc failed. \n", __func__);
        err = FSP_ERR_OUT_OF_MEMORY;
        err = provisioning_delete_ap_list_json(root_ap_list);
    }

    memset(scan_res_array, 0, sizeof(provisioning_scan_callback_args_t) * SCAN_RESULT_MAX);
    wifi_scan_data = (WIFIScanResult_t *)pvPortMalloc(sizeof(WIFIScanResult_t) * SCAN_RESULT_MAX);
    if (NULL == wifi_scan_data)
    {
        printf("[%s] malloc failed. \n", __func__);
        if (NULL != scan_res_array)
        {
            vPortFree(scan_res_array);
        }
        provisioning_delete_ap_list_json(root_ap_list);
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    memset(wifi_scan_data, 0, sizeof(WIFIScanResult_t) * SCAN_RESULT_MAX);

    /* Cli scan */
     wifi_err = WIFI_Scan(&wifi_scan_data[0], SCAN_RESULT_MAX);
     if (eWiFiSuccess != wifi_err)
    {
        printf("[%s] Wi-Fi Scan request failed. \n", __func__);
        if (NULL != scan_res_array)
        {
            vPortFree(scan_res_array);
        }

        if (NULL != wifi_scan_data)
        {
            vPortFree(wifi_scan_data);
        }

        provisioning_delete_ap_list_json(root_ap_list);
        err = FSP_ERR_WIFI_FAILED;

        return err;
    }

    printf("\n> Wi-Fi Scan request success.\n");

    update_scan_array(wifi_scan_data, scan_res_array);
    if (wifi_scan_data)
    {
        vPortFree(wifi_scan_data);
    }

    vPortFree(scan_res_array);
    err = provisioning_delete_ap_list_json(root_ap_list);

    return err;
}

static void string_to_ip4(const char *str, uint8_t ip[4])
{
    sscanf(str, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
}

static fsp_err_t provisioning_ping_client(char *p_ip_address)
{
    fsp_err_t err = FSP_SUCCESS;
    int32_t interval = 0;
    uint32_t count = 0;
    ip_addr_t tmp_addr;
    int result = 0;
    uint8_t ip_bin[4];
    WIFIReturnCode_t wifi_err = eWiFiSuccess;

    /* result string */
    result = ipaddr_aton(p_ip_address, &tmp_addr);
    string_to_ip4(p_ip_address, ip_bin);

    if (0 == result)
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    count    = 4;                      // only 2
    interval = 1000;                   // interval default 1sec
    /* Ping client api execution with nodisplay as 1 and getting the string of result */
    wifi_err = WIFI_Ping(ip_bin, count, interval);

    if (wifi_err == eWiFiSuccess)               /* Success */
    {
        printf("Ping reply is ok\n");

        return FSP_SUCCESS;
    }

    printf("Ping reply is fail\n");
    err = FSP_ERR_WIFI_NO_RESULT;

    return err;
}

/*******************************************************************************************************************//**
 * Sets AP configuration with predefined ID/PW on Based on saved result that user selected
 *
 * @param[in]  ap_mode              Indicates RA6W1/RA6W2 is set for security mode or not.
 * @note AP_OPEN_MODE(0) for no Security, AP_SECURITY_MODE for secupity
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT ap_mode is in invalid value.
 **********************************************************************************************************************/
static fsp_err_t provisioning_set_ap_config(int32_t ap_mode)
{
    fsp_err_t err = FSP_SUCCESS;

    printf("[%s] set AP config mode = %ld \n", __func__, ap_mode);
    ap_config_param->customer_cfg_flag = MODE_ENABLE;
    sprintf(ap_config_param->ssid_name, "%s", PREDEFINE_SSID);

    if (ap_mode == AP_SECURITY_MODE)
    {
        sprintf(ap_config_param->psk, "%s", PREDEFINE_PW);
    }

    if ((AP_OPEN_MODE == ap_mode) || (AP_SECURITY_MODE == ap_mode))
    {
        ap_config_param->auth_type = (char) ap_mode; ///< AP_OPEN_MODE, AP_SECURITY_MODE
    }
    else
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    /*
     * Country Code : Default is "US"
     */
    sprintf(ap_config_param->country_code, "%s", "KR");
    /*
     * Network IP address configuration
     */
    ap_config_param->customer_ip_address = IPADDR_CUSTOMER;
    if (IPADDR_CUSTOMER == ap_config_param->customer_ip_address)
    {
        sprintf(ap_config_param->ip_addr, "%s", "10.0.0.1");
        sprintf(ap_config_param->subnet_mask, "%s", "255.255.255.0");
        sprintf(ap_config_param->default_gw, "%s", "10.0.0.1");
        sprintf(ap_config_param->dns_ip_addr, "%s", "8.8.8.8"); // 10.0.0.1 check
    }
    /*
     * DHCP Server configuration
     */
    ap_config_param->customer_dhcpd_flag = DHCPD_CUSTOMER;
    if (DHCPD_CUSTOMER == ap_config_param->customer_dhcpd_flag)
    {
        ap_config_param->dhcpd_lease_time = 3600;
        sprintf(ap_config_param->dhcpd_start_ip, "%s", "10.0.0.2");
        sprintf(ap_config_param->dhcpd_end_ip, "%s", "10.0.0.11");
    }

    return err;
}

/*******************************************************************************************************************//**
 * Set SSID without MAC address
 *
 * @param[in]  p_prefix             Buffer for SSID base string
 * @param[in]  interface            Interface of AP
 * @param[in]  quotation            Option to set quotation
 * @param[out] p_ssid               SSID string
 * @param[out] size                 SSID string size
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_OUT_OF_MEMORY    prefix is too small or NULL to copy SSID.
 **********************************************************************************************************************/
static fsp_err_t provisioning_set_ssid_without_mac(char *p_prefix,
                                                   int interface,
                                                   int quotation,
                                                   char *p_ssid,
                                                   int size)
{
    fsp_err_t err = FSP_SUCCESS;

    RA6W1_UNUSED_ARG(interface);

    if ((quotation ? (int) (strlen(p_prefix) + 7 + 2) : (int) (strlen(p_prefix) + 7)) > size)
    {
        if (strlen(p_prefix) >= (PROV_MAX_SSID_LEN))
        {
            sprintf(p_ssid, "%s%s%s", quotation ? "\"" : "", p_prefix, quotation ? "\"" : "");
        }
        else
        {
            err = FSP_ERR_OUT_OF_MEMORY;
        }
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    snprintf(p_ssid,
             (size_t) ((size > (PROV_MAX_SSID_LEN + 2)) ? (quotation ? (PROV_MAX_SSID_LEN + 2) : PROV_MAX_SSID_LEN) : (size)),
             "%s%s%s",
             quotation ? "\"" : "",
             p_prefix,
             quotation ? "\"" : "");

    return err;
}

/*******************************************************************************************************************//**
 * Fetch NVRAM and get value of DPM Setting of the type.
 *
 * @param[in]  value_type           DPM value type to get.
 *
 * @return Value requested from value_type as int
 **********************************************************************************************************************/
static int provisioning_dpm_status_get(provisioning_dpm_value_type_t value_type)
{
    if (PROV_FEATURE_ATCMD == g_provisioning_feature)
    {
        int ret = 0;
        char *char_buffer = NULL;

        switch (value_type)
        {
            case TYPE_SLEEP_MODE:
            {
                char_buffer = provisioning_NVRAM_CFG_SLEEPMODE;
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_APPCFG,
                                             (const char *)char_buffer,
                                             &ret);
#endif
                return (uint8_t)ret;
            }

            case TYPE_USE_DPM:
            {
                char_buffer = provisioning_NVRAM_CFG_USE_DPM;
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_APPCFG,
                                             (const char *)char_buffer,
                                             &ret);
#endif
                return (uint8_t)ret;
            }

            case TYPE_DPM_KEEP_ALIVE:
            {
                char_buffer = WIFI_PROFILE_DPM_DPM_KEEPALIVE_TIME;
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *)char_buffer,
                                             (int *)&ret);
#endif
                return ret;
            }

            case TYPE_USER_WAKE_UP:
            {
                char_buffer = WIFI_PROFILE_DPM_USER_WAKEUP_TIME;
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *)char_buffer,
                                             (int *)&ret);
#endif
                return ret;
            }

            case TYPE_TIM_WAKE_UP:
            {
                char_buffer = WIFI_PROFILE_DPM_TIM_WAKEUP_COUNT;

#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_WIFIPROFILE,
                                             (const char *)char_buffer,
                                             (int *)&ret);
#endif
                return ret;
            }

            case TYPE_RTC_TIME:
            {
                char_buffer = provisioning_NVRAM_CFG_RTC_TIME; // need to check it right
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),
                                             ENV_GROUP_APPCFG,
                                             (const char *)char_buffer,
                                             &ret);
#endif
                return ret;
            }
        }
    }
    else
    {
        if (TYPE_SLEEP_MODE == value_type)
        {
            return sleep_mode_value;
        }

        if (TYPE_USE_DPM == value_type)
        {
            return dpm_mode_value;
        }

        if (TYPE_DPM_KEEP_ALIVE == value_type)
        {
            return dpm_keep_alive_time_value;
        }

        if (TYPE_USER_WAKE_UP == value_type)
        {
            return user_wakeup_time_value;
        }

        if (TYPE_TIM_WAKE_UP == value_type)
        {
            return tim_wakeup_time_value;
        }

        if (TYPE_RTC_TIME == value_type)
        {
            return rtc_time_value;
        }
    }
    printf("getDPMSetValue error [Undfined type]\n");

    return -1;
}

/*******************************************************************************************************************//**
 * Set sleep mode and DTIM value.
 *
 * @param[in]  sleep_mode           Sleep mode setting value, must be 0 < sleep_mode < 4.
 * @param[in]  dpm_mode             DPM Mode value.
 * @param[in]  rtc_time             RTC wakeup time setting value
 * @param[in]  dpm_keep_alive_time  DPM Keep-alive time setting value
 * @param[in]  user_wakeup_time     User wakeup time setting value
 * @param[in]  tim_wakeup_time      TIM wakeup time setting value
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT Sleep mode is not 0 < sleep_mode < 4.
 **********************************************************************************************************************/
static fsp_err_t provisioning_dpm_status_set(int sleep_mode,
                                             int dpm_mode,
                                             int rtc_time,
                                             int dpm_keep_alive_time,
                                             int user_wakeup_time,
                                             int tim_wakeup_time)
{
    fsp_err_t err = FSP_SUCCESS;

    if ((sleep_mode >= 1) && (sleep_mode <= 3))
    {
        sleep_mode_value = sleep_mode;
    }
    else if (0 == sleep_mode)
    {
        printf("Using Default Sleep mode \n");
    }
    else
    {
        printf("Unsupported Sleep mode \n");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    dpm_mode_value            = dpm_mode;
    dpm_keep_alive_time_value = dpm_keep_alive_time;
    user_wakeup_time_value    = user_wakeup_time;
    tim_wakeup_time_value     = tim_wakeup_time;
    rtc_time_value            = rtc_time;

    printf("[Set] SleepMode: %d ,DPM: %d, rtc_time_value : %d sec, DPM_KeepAlive: %d, userWakeUp: %d, timWakeUp: %d\n",
           sleep_mode_value,
           dpm_mode_value,
           rtc_time_value,
           dpm_keep_alive_time_value,
           user_wakeup_time_value,
           tim_wakeup_time_value);

    return err;
}


/*******************************************************************************************************************//**
 * Reset all Wi-Fi configuration and reboot to station mode
 *
 * @param[in]  p_ssid               Pointer to Provisioning instance control structure.
 * @param[in]  p_password           Pointer to Provisioning configuration structure.
 * @param[in]  prov_security        Security type of Wi-Fi that AP is Using.
 * @param[in]  dpm_mode             DPM mode enabled / disabled.
 * @param[in]  is_ssid_hidden       If true, find hidden AP. If false, do a normal find.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT NULL pointer on p_ssid or wrong value on dpm_mode
 *
 * @return     See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
static fsp_err_t provisioning_reset_ap_to_station(char *p_ssid,
                                                  char *p_password,
                                                  provisioning_ap_security_t prov_security,
                                                  provisioning_dpm_mode_t dpm_mode,
                                                  bool is_ssid_hidden)
{
    fsp_err_t err = FSP_SUCCESS;
    provisioning_param_t config;

    if (PROV_DPM_OFF == 0)
    {
        printf("After soon reboot to [ No DPM ] mode !!\n");
    }
    else if (PROV_DPM_ON == 1)
    {
        printf("After soon reboot to [ DPM ] mode !!\n");
    }
    else
    {
        printf("NOT support reboot mode !!\n");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (NULL == p_ssid)
    {
        printf("No SSID is given");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    printf("\n[CMD APP] STA_mode_reset....");
    memset(&config, 0, sizeof(provisioning_param_t));
    strcpy(config.ssid, p_ssid);

    if (eAPSecurityWEP == prov_security)
    {
        config.auth_type = eAPSecurityWEP; // WEP
        strcpy(config.psk, p_password);
    }
    else if (eAPSecurityWPA == prov_security)
    {
        config.auth_type = eAPSecurityWPA; // WPA
        strcpy(config.psk, p_password);
    }
    else if (eAPSecurityWPA2 == prov_security)
    {
        config.auth_type = eAPSecurityWPA2; // WPA2
        strcpy(config.psk, p_password);
    }
    else if (eAPSecurityWPA_AUTO == prov_security)
    {
        config.auth_type = eAPSecurityWPA_AUTO; // WPA2
        strcpy(config.psk, p_password);
    }
    else if ((prov_security >= eAPSecurityWPA_EAP) && (prov_security <= eAPSecurityWPA3_EAP_192B)) // WPA-Ent
    {
        config.auth_type = prov_security;
        strcpy(config.eap_identity, eap_ID);
        strcpy(config.eap_password, eap_PW);
        config.eap_protocol = eap_protocol;
        config.eap_type     = eap_type;
    }
    else if ((prov_security >= eAPSecurityOWE) && (prov_security <= eAPSecurityRSN_SAE))
    {
        config.auth_type = prov_security;
        if (prov_security == eAPSecurityOWE)
        {
            strcpy(config.psk, "");
        }
        else
        {
            strcpy(config.psk, p_password);
        }
    }
    else
    {
        config.auth_type = eAPSecurityOpen; // OPEN
        strcpy(config.psk, "");
    }

    config.auto_restart_flag = 1;                   ///< restart
    config.dpm_mode          = dpm_mode;
    config.sntp_flag         = 0;
    config.hidden            = is_ssid_hidden;      // use Hidden SSID
    config.prov_type         = g_provisioning_type; // provision check for Platform
    config.auth_type         = security_type;
    config.eap_type          = eap_type;
    config.eap_protocol      = eap_protocol;
    strcpy(config.eap_identity, eap_ID);
    strcpy(config.eap_password, eap_PW);

#if defined(__PROVISION_ATCMD__)
    atcmd_provstat(ATCMD_PROVISION_REBOOT_ACK);
#endif  // __PROVISION_ATCMD__

    return provisioning_reboot_station_mode(&config, 1);
}


/*******************************************************************************************************************//**
 * Adds provisioning mode to p_root_json in JSON format
 *
 * @param[in]  p_root_json          Pointer to target JSON object.
 * @param[in]  device_mode          Current Provisioning mode.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT At least one of the function's argument is NULL
 **********************************************************************************************************************/
static fsp_err_t provisioning_add_mode_to_json(cJSON *p_root_json, provisioning_type_t device_mode)
{
    fsp_err_t err = FSP_SUCCESS;

    if (NULL == p_root_json)
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    cJSON_AddNumberToObject(p_root_json, "mode", device_mode);

    return err;
}

/*******************************************************************************************************************//**
 * Adds thing name to p_root_json in JSON format
 *
 * @param[in]  p_root_json          Pointer to target JSON object.
 * @param[in]  p_thing_name         Pointer to string that has thing name for the device.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT At least one of the function's argument is NULL
 **********************************************************************************************************************/
static fsp_err_t provisioning_add_thing_name_to_json(cJSON *p_root_json, char *p_thing_name)
{
    fsp_err_t err = FSP_SUCCESS;

    if ((NULL == p_root_json) || (NULL == p_thing_name))
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    cJSON_AddStringToObject(p_root_json, "thingName", p_thing_name);

    return err;
}

/*******************************************************************************************************************//**
 * Make command and data to JSON format
 *
 * @param[in]  p_command            Pointer to command string
 * @param[in]  command_type         Status or Result that will add to command type.
 *
 * @return String array pointer that has JSON formatted command as String
 **********************************************************************************************************************/
static char * provisioning_add_command_to_json(char *p_command, int32_t command_type)
{
    fsp_err_t err = FSP_SUCCESS;
    cJSON   * root;
    char    * buf;

    root = cJSON_CreateObject();
    if (NULL == root)
    {
        printf("provisioning_add_command_to_json Json not generated");
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    if (FSP_SUCCESS != err)
    {
        FSP_ERROR_LOG(err);

        return NULL;
    }

    if (NULL == p_command)
    {
        printf("provisioning_add_command_to_json Command argument not given");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    cJSON_AddNumberToObject(root, p_command, command_type);
    buf = cJSON_Print(root);
    cJSON_Delete(root);

    if (FSP_SUCCESS != err)
    {
        FSP_ERROR_LOG(err);
    }

    return buf;
}

/*******************************************************************************************************************//**
 * Send char data to phone application using TCP.
 *
 * @param[in]  p_data               Pointer to data to send.
 * @param[in]  data_size            Size of p_data.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_STATE    Mutex cannot be locked because of other processes.
 * @retval FSP_ERR_ABORTED          Error on lwip_send function.
 * @retval FSP_ERR_NOT_INITIALIZED  Provisioning TCP Socket is not valid.
 **********************************************************************************************************************/
static fsp_err_t provisioning_send_tcp_data(char *p_data, int32_t data_size)
{
    fsp_err_t err = FSP_SUCCESS;

    int status = 0;

    int32_t result = provisioning_mutex_lock(&provision_mutex);
    if (1 != result)
    {
        err = FSP_ERR_INVALID_STATE;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    printf("[Prov TCP Send] :::  size = %ld \n", data_size);
    if (0 < provision_TCP_csocket)
    {
        status = send(provision_TCP_csocket, p_data, (size_t) data_size, 0);
        if (0 >= status)
        {
            printf("[%s:%d]failed to send packet(0x%02x)\n", __func__, __LINE__, status);
            err = FSP_ERR_ABORTED;
            close(provision_TCP_csocket);
        }
    }
    else
    {
        printf("[%s:%d]client socket id invalid (0x%02x)\n", __func__, __LINE__, provision_TCP_csocket);
        err = FSP_ERR_NOT_INITIALIZED;
    }

    result = provisioning_mutex_unlock(&provision_mutex);
    if (1 != result)
    {
        err = FSP_ERR_INVALID_STATE;
    }

    return err;
}

/*******************************************************************************************************************//**
 * Send char data to phone application using TLS.
 *
 * @param[in]  p_config             Pointer to TLS Server configuration.
 * @param[in]  p_data               Pointer to data to send.
 * @param[in]  data_size            Size of p_data.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_STATE    Mutex cannot be locked because of other processes.
 * @retval FSP_ERR_ABORTED          Error on MbedTLS SSL write function.
 **********************************************************************************************************************/
static fsp_err_t provisioning_send_tls_data(provisioning_tls_server_cfg_t *p_config, char *p_data, int32_t data_size)
{
    fsp_err_t err = FSP_SUCCESS;

    int result = provisioning_mutex_lock(&provision_mutex);
    if (1 != result)
    {
        err = FSP_ERR_INVALID_STATE;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    while ((result = mbedtls_ssl_write(p_config->ssl_ctx, (const unsigned char *)p_data, (size_t)data_size)) <= 0)
    {
        if ((result != MBEDTLS_ERR_SSL_WANT_READ) && (result != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            printf("[%s:%d] failed to write ssl packet(0x%x)\n",
                   __func__,
                   __LINE__,
                   -result);
            err = FSP_ERR_ABORTED;
        }
    }

    result = provisioning_mutex_unlock(&provision_mutex);
    if (1 != result)
    {
        err = FSP_ERR_INVALID_STATE;
    }

    return err;
}

/*******************************************************************************************************************//**
 * Send char data to phone application as JSON format using TCP.
 *
 * @param[in]  p_data               Pointer to String data to send.
 * @param[in]  command_type         Status or Result that will add to command type.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_OUT_OF_MEMORY    Heap is too small or NULL to create a data buffer for JSON.
 *
 * @return     See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
static fsp_err_t provisioning_send_tcp_data_as_json (char *p_data, int32_t command_type)
{
    fsp_err_t err = FSP_SUCCESS;
    char *data_buf = NULL;

    data_buf = provisioning_add_command_to_json(p_data, command_type);
    if (data_buf != NULL)
    {
        int32_t buf_size = (int32_t) strlen(data_buf);
        err = provisioning_send_tcp_data(data_buf, buf_size);
        vPortFree(data_buf);
    }
    else
    {
        printf("provisioning_send_tcp_data_as_json error \n");
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    return err;
}

/*******************************************************************************************************************//**
 * Send char data to phone application as JSON format using TLS.
 *
 * @param[in]  p_config             Pointer to TLS Server configuration.
 * @param[in]  p_data               Pointer to String data to send.
 * @param[in]  command_type         Status or Result that will add to command type.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_OUT_OF_MEMORY    Heap is too small or NULL to create a data buffer for JSON.
 *
 * @return     See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
static fsp_err_t provisioning_send_tls_data_as_json(provisioning_tls_server_cfg_t *p_config,
                                                    char * p_data,
                                                    int32_t command_type)
{
    fsp_err_t err = FSP_SUCCESS;
    char *data_buf = NULL;

    data_buf = provisioning_add_command_to_json(p_data, command_type);
    if (data_buf != NULL)
    {
        int32_t buf_size = (int32_t)strlen(data_buf);

        provisioning_send_tls_data(p_config, data_buf, buf_size);
        printf("Sending TLS data \n");
        vPortFree(data_buf);
    }
    else
    {
        printf("provisioning_send_tls_data_as_json error \n");
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    return err;
}

/*******************************************************************************************************************//**
 * Create Json object for AP list.
 *
 * @param[in] p_root_json           Pointer to root JSON object.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT Root JSON object is not initialized.
 **********************************************************************************************************************/
static fsp_err_t provisioning_create_ap_list_json(cJSON *p_root_json)
{
    fsp_err_t err = FSP_SUCCESS;
    if (NULL == p_root_json)
    {
        printf("provisioning_create_ap_list_json root json is not initialized\n");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    scan_ap_list_json = cJSON_CreateArray();
    cJSON_AddItemToObject(p_root_json, "APList", scan_ap_list_json);

    return err;
}

/*******************************************************************************************************************//**
 * Delete Json object for AP list.
 *
 * @param[in]  p_ap_list            Pointer to AP list JSON.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_ARGUMENT AP List is not initialized.
 **********************************************************************************************************************/
static fsp_err_t provisioning_delete_ap_list_json(cJSON *p_ap_list)
{
    fsp_err_t err = FSP_SUCCESS;
    if (NULL == p_ap_list)
    {
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    char *buf;
    buf = cJSON_Print(p_ap_list);
    ap_list_buffer = buf;
    cJSON_Delete(p_ap_list);

    return err;
}

/*******************************************************************************************************************//**
 * Parse received data from phone application
 *
 * @param[in] p_received_data           Received data from phone in JSON as String
 *
 * @retval CONNECTED                    Device connected with phone application.
 * @retval SET_AP_SSID_PW               Phone application sent AP SSID and Password to set.
 * @retval REQ_HOMEAP_RESULT            Phone application requests AP Lists.
 * @retval REQ_RESCAN                   Phone application requests rescan.
 * @retval REQ_REBOOT                   Phone application requests reboot for the device.
 * @retval REQ_SET_DPM                  Phone application requests to set DPM mode.
 * @retval REQ_SOCKET_TYPE              Phone application requests Provisioning socket type
 * @retval CMD_ERROR                    Provisioning command parsing error.
 **********************************************************************************************************************/
static provisioning_ap_cmd_type_t provisioning_parse_json_data(const char *p_received_data)
{
    provisioning_ap_cmd_type_t return_status = CMD_ERROR;
    provisioning_ap_cmd_type_t command;
    cJSON *json_recv_data = NULL;
    cJSON *child_object   = NULL;
    cJSON *child_finish = NULL;
    cJSON *child_ssid = NULL;
    cJSON *child_pw = NULL;
    cJSON *child_url = NULL;
    cJSON *child_hidden = NULL;
    cJSON *child_security = NULL;
    cJSON *child_eap_type = NULL;
    cJSON *child_eap_proto = NULL;
    cJSON *child_eap_identity = NULL;
    cJSON *child_eap_password = NULL;
    cJSON *sleepMode = NULL;
    cJSON *useDPM = NULL;
    cJSON *dpmKeepAlive = NULL;
    cJSON *userWakeup = NULL;
    cJSON *timWakeup = NULL;
    cJSON *rtcTimer = NULL;
    cJSON *socketType = NULL;

    json_recv_data = cJSON_Parse(p_received_data);
    child_object = cJSON_GetObjectItem(json_recv_data, "msgType");
    if (child_object != NULL)
    {
        printf("MSG Type [%d] \n", child_object->valueint);
        command = (provisioning_ap_cmd_type_t) child_object->valueint;
        if (SET_AP_SSID_PW == command)
        {
#if defined(__PROVISION_ATCMD__)
            atcmd_provstat(ATCMD_PROVISION_SELECTED_AP_SUCCESS);
#endif
            printf("[SET SSID , PW]\n");

            child_ssid = cJSON_GetObjectItem(json_recv_data, "ssid");
            child_pw = cJSON_GetObjectItem(json_recv_data, "pw");
            child_url = cJSON_GetObjectItem(json_recv_data, "url");
            child_hidden = cJSON_GetObjectItem(json_recv_data, "isHidden");
            child_security = cJSON_GetObjectItem(json_recv_data, "securityType");
            child_eap_type = cJSON_GetObjectItem(json_recv_data, "authType");
            child_eap_proto = cJSON_GetObjectItem(json_recv_data, "authProtocol");
            child_eap_identity = cJSON_GetObjectItem(json_recv_data, "authID");
            child_eap_password = cJSON_GetObjectItem(json_recv_data, "authPW");
            eap_ID = (char *)pvPortMalloc(MAX_AP_SSID_SIZE);
            eap_PW = (char *)pvPortMalloc(MAX_AP_PASSWORD_SIZE);
            memset(eap_ID, 0, MAX_AP_SSID_SIZE);
            memset(eap_PW, 0, MAX_AP_PASSWORD_SIZE);

            if (child_eap_identity != NULL)
            {
                strcpy(eap_ID, child_eap_identity->valuestring);
            }

            if (child_eap_password != NULL)
            {
                strcpy(eap_PW, child_eap_identity->valuestring);
            }

            if (child_eap_type != NULL)
            {
                eap_type = child_eap_type->valueint;
            }

            if (child_eap_proto != NULL)
            {
                eap_protocol = child_eap_proto->valueint;
            }

            printf(">>> eap ID: %s, eap PW: %s, eap type: %d, eap_protocol: %d \n", eap_ID, eap_PW, eap_type, eap_protocol);

            memset(new_SSID, 0, MAX_AP_SSID_SIZE);
            memset(new_pw, 0, MAX_AP_PASSWORD_SIZE);
            memset(server_URL, 0, MAX_CUSTOMER_SERVER_SIZE);
            new_pw_size = 0;
            new_SSID_size = 0;
            server_URL_size = 0;

            if (NULL != child_ssid)
            {
                printf("[SSID] -> %s  size = %d\n", child_ssid->valuestring, strlen(child_ssid->valuestring));
                new_SSID_size = (int32_t)strlen(child_ssid->valuestring);
                strcpy(new_SSID, child_ssid->valuestring);
            }

            if (NULL != child_pw)
            {
                printf("[PW] -> %s size = %d\n", child_pw->valuestring, strlen(child_pw->valuestring));
                new_pw_size = (int32_t)strlen(child_pw->valuestring);
                strcpy(new_pw, child_pw->valuestring);
            }

            if (NULL != child_url)
            {
                printf("[SERVER URL] -> %s size = %d\n", child_url->valuestring, strlen(child_url->valuestring));
                server_URL_size = (int32_t)strlen(child_url->valuestring);
                strcpy(server_URL, child_url->valuestring);
            }

            /* Hidden SSID use set */
            if (1 == child_hidden->valueint)
            {
                printf("[isHidden] -> %d\n", child_hidden->valueint);
                use_hidden_ssid = true;
            }

            /* Security Mode set */
            if (0 <= child_security->valueint)
            {
                printf("[securityType] -> %d\n", child_security->valueint);
                security_type = child_security->valueint;
            }

            return_status = SET_AP_SSID_PW;
        }
        else if (CONNECTED == command)
        {
            printf("[CONNECTED]\n");
            return_status = CONNECTED;
        }
        else if (REQ_HOMEAP_RESULT == command)
        {
            printf("[REQ_HOMEAP_RESULT]\n");
            return_status = REQ_HOMEAP_RESULT;
        }
        else if (REQ_RESCAN == command)
        {
            printf("[REQ_RESCAN]\n");
            return_status = REQ_RESCAN;
        }
        else if (REQ_REBOOT == command)
        {
#if defined(__PROVISION_ATCMD__)
            atcmd_provstat(ATCMD_PROVISION_REBOOT_ACK);
#endif //__PROVISION_ATCMD__
            child_finish = cJSON_GetObjectItem(json_recv_data, "finishCMD");
            if (NULL != child_finish)
            {
                printf("Finish CMD  %d\n", child_finish->valueint);
            }

            printf("[REQ_REBOOT]\n");
            return_status = REQ_REBOOT;
        }
        else if (REQ_SET_DPM == command)
        {
            sleepMode    = cJSON_GetObjectItem(json_recv_data, "sleepMode");
            useDPM       = cJSON_GetObjectItem(json_recv_data, "useDPM");
            dpmKeepAlive = cJSON_GetObjectItem(json_recv_data, "dpmKeepAlive");
            userWakeup   = cJSON_GetObjectItem(json_recv_data, "userWakeup");
            timWakeup    = cJSON_GetObjectItem(json_recv_data, "timWakeup");
            rtcTimer     = cJSON_GetObjectItem(json_recv_data, "rtcTimer");

            printf("[SET Sleep mode , DTIM]\n");
            printf("[SET Sleep mode , data]%s \n", p_received_data);

            if ((NULL != sleepMode) && (NULL != useDPM) && (NULL != dpmKeepAlive) && (NULL != userWakeup) &&
                (NULL != rtcTimer) && (NULL != timWakeup))
            {
                provisioning_dpm_status_set(sleepMode->valueint,
                                            useDPM->valueint,
                                            rtcTimer->valueint,
                                            dpmKeepAlive->valueint,
                                            userWakeup->valueint,
                                            timWakeup->valueint);
            }
            else
            {
                printf("Set DPM value  parsing error \n");
            }

            return_status = REQ_SET_DPM;
        }
        else if (REQ_SOCKET_TYPE == command)
        {
            socketType = cJSON_GetObjectItem(json_recv_data, "SOCKET_TYPE");
            socket_app = (UINT8)socketType->valueint;
            printf("[REQ_SOCKET_TYPE] %d\n", socket_app);
            return_status = REQ_SOCKET_TYPE;
        }
    }
    else
    {
        printf("LocalJSONParse  parsing error \n");
        return_status = CMD_ERROR;
    }

    cJSON_Delete(json_recv_data);

    return return_status;
}

/*******************************************************************************************************************//**
 * Send message from server thread to client thread
 *
 * @param[in]  p_send_data          Pointer to sending data.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_STATE    Queue sending blocked.
 **********************************************************************************************************************/
static fsp_err_t provisioning_send_msg_to_client(provisioning_user_data_t * p_send_data)
{
    fsp_err_t err = FSP_SUCCESS;
    int status = 0;

    status = xQueueSend(provision_message_queue, p_send_data, portMAX_DELAY);
    if (status != pdPASS)
    {
        printf("[%s] xQueueSend error !!! (%d)\n", __func__, status);
        err = FSP_ERR_INVALID_STATE;
    }

    return err;
}

/*******************************************************************************************************************//**
 * Receive message from server thread
 *
 * @retval PROV_COMMAND_ERROR           Provisioning command is error.
 * @retval CONNECTED                    Device connected with phone application.
 * @retval SET_AP_SSID_PW               Phone application sent AP SSID and Password to set.
 * @retval REQ_HOMEAP_RESULT            Phone application requests AP Lists.
 * @retval REQ_RESCAN                   Phone application requests rescan.
 * @retval REQ_REBOOT                   Phone application requests reboot for the device.
 * @retval REQ_SET_DPM                  Phone application requests to set DPM mode.
 * @retval REQ_SOCKET_TYPE              Phone application requests Provisioning socket type
 * @retval CMD_ERROR                    Provisioning command parsing error.
 **********************************************************************************************************************/
static provisioning_ap_cmd_type_t provisioning_recv_msg_from_server(void)
{
    fsp_err_t err = FSP_SUCCESS;
    int status = 0;
    provisioning_user_data_t proQData;

    status = xQueueReceive(provision_message_queue, &proQData, portMAX_DELAY);
    if (status != pdPASS)
    {
        printf("[%s] xQueueReceive error !!! (%d)\n", __func__, status);
        err = FSP_ERR_INVALID_STATE;
    }

    if (FSP_SUCCESS != err)
    {
        FSP_ERROR_LOG(err);
    }

    return proQData.status;
}

/*******************************************************************************************************************//**
 * Get provisioning socket type.
 *
 * @retval PROV_TCP_SOCKET     Provisioning uses TCP socket.
 * @retval PROV_TLS_SOCKET     Provisioning uses TLS socket.
 **********************************************************************************************************************/
static provisioning_socket_type_t provisioning_get_socket_type(void)
{
    return socket_dev;
}

/*******************************************************************************************************************//**
 * TLS server running instance
 *
 * @param[in]  p_config       Pointer to Provisioning TLS Server configuration.
 *
 * @return void
 **********************************************************************************************************************/
static void provisioning_run_tls_server(provisioning_tls_server_cfg_t * p_config)
{
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    uint8_t task_wdog_id = WATCHDOG_SERVICE_W_NOT_REGISTERED_ID;
#endif
    int             status     = ERR_OK;
    int             recv_bytes = 0;
    unsigned char * buf        = NULL;
    size_t          buflen     = (8 * 1024);
    mbedtls_net_context client_ctx;
    provisioning_user_data_t sample_data;

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->registerTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
        &task_wdog_id);
#endif
    printf("\r\n>>> Start Provisioning Server (TLS) ...\r\n");
    mbedtls_net_init(&client_ctx);
    /* Allocate buffer */
    buf = (unsigned char *)provisioning_calloc(1, buflen);
    if (NULL == buf)
    {
        printf("[%s] Failed to allocate buffer to receive\r\n", __func__);

        return;
    }
    /* Init TLS server sample's configuration */
    status = provisioning_tls_svr_init_config(p_config);
    if (status)
    {
        printf("[%s] Failed to init sample tls server config(0x%x)\n", __func__, status);
        provisioning_free(buf);

        return;
    }
    /* Init TCP socket */
    status = provisioning_tls_svr_init_socket(p_config);
    if (status)
    {
        printf("[%s] Failed to init tcp socket(0x%x)\n", __func__, status);
        provisioning_free(buf);

        return;
    }
    /* Init TLS session */
    status = provisioning_tls_svr_init_ssl(p_config);
    if (status)
    {
        printf("[%s] Failed to init ssl(0x%x)\r\n", __func__, status);

        provisioning_tls_svr_deinit_socket(p_config);
        provisioning_free(buf);

        return;
    }
    /* Setup TLS session */
    status = provisioning_tls_svr_setup_ssl(p_config);
    if (status)
    {
        printf("[%s] Failed to setup ssl(0x%x)\r\n", __func__, -status);

        provisioning_tls_svr_deinit_ssl(p_config);
        provisioning_tls_svr_deinit_socket(p_config);
        provisioning_free(buf);

        return;
    }

connect:
    mbedtls_net_free(&client_ctx);
    provisioning_tls_svr_shutdown_ssl(p_config);
    printf("Wait Accept (TLS)...\n");

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->notify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                               gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                                                               task_wdog_id);
    gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                task_wdog_id);
#endif

    /* Wait until a client connects */
    status = mbedtls_net_accept(&p_config->sock_ctx, &client_ctx, NULL, 0, NULL);
    if (status)
    {
        printf("[%s] Failed to accept client connects(0x%x)\r\n", __func__, -status);
        goto connect;
    }
    /* Set callbacks to write & read data. */
    mbedtls_ssl_set_bio(p_config->ssl_ctx, &client_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);
    /* Handshake TLS session */
    status = provisioning_tls_svr_do_handshake(p_config);
    if (status)
    {
        printf("[%s] Failed to progress handshake(0x%x)\r\n", __func__, -status);
        goto connect;
    }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
        task_wdog_id);
#endif

    /* Data Transmit: */
    while (pdTRUE)
    {
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->notify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                   gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                                                                   task_wdog_id);
        gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                    task_wdog_id);
#endif
        memset(buf, 0x00, buflen);

        status = mbedtls_ssl_read(p_config->ssl_ctx, buf, buflen);

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
            gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
            gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
            task_wdog_id);
        gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                    task_wdog_id);
#endif

        if (status <= 0)
        {
            /* For log */
            switch (status)
            {
                case MBEDTLS_ERR_SSL_WANT_READ:
                case MBEDTLS_ERR_SSL_WANT_WRITE:
                {
                    continue;
                }

                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                {
                    printf("[%s]Connection was closed gracefully\r\n", __func__);
                    break;
                }

                case MBEDTLS_ERR_NET_CONN_RESET:
                {
                    printf("[%s]Connection was reset by peer\r\n", __func__);
                    break;
                }

                default:
                {
                    printf("[%s]Failed to read data(0x%x)\r\n", __func__, -status);
                    break;
                }
            }

            goto connect;
        }

        recv_bytes = status;
        if (recv_bytes > 0)
        {
            sample_data.status = provisioning_parse_json_data((char const *) buf);

            if (check_connection_ap)
            {
                char *data_buffer = NULL;
                printf("Now checking to connect Home AP...\n");
                data_buffer = provisioning_add_command_to_json("RESULT_HOMEAP", 0);
                if (data_buffer != NULL)
                {
                    int32_t buffer_size = (int32_t) strlen(data_buffer);
                    provisioning_send_tls_data(p_config, data_buffer, buffer_size);
                    vPortFree(data_buffer);
                }
            }
            else
            {
                provisioning_send_msg_to_client(&sample_data);
            }
        }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
            gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
            gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
            task_wdog_id);
#endif
    }
    /* Socket terminate */
    mbedtls_net_free(&client_ctx);
    /* Shutdown TLS session */
    provisioning_tls_svr_shutdown_ssl(p_config);
    /* Deinit TLS session */
    provisioning_tls_svr_deinit_ssl(p_config);
    /* Deinit TCP socket */
    provisioning_tls_svr_deinit_socket(p_config);
    /* Release memory */
    if (buf)
    {
        provisioning_free(buf);
    }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->unregisterTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        task_wdog_id);
#endif
    printf("[%s] Terminated Provision Server (TLS) ...\r\n", __func__);
}


/*******************************************************************************************************************//**
 * Thread for sending TCP/TLS data to phone.
 *
 * @param[in] tls_info Provisioning TLS information.
 *
 * @return  void
 **********************************************************************************************************************/
static void provisioning_client_thread(void * tls_info)
{
    fsp_err_t err = FSP_SUCCESS;
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    uint8_t task_wdog_id = WATCHDOG_SERVICE_W_NOT_REGISTERED_ID;
#endif
    provisioning_ap_cmd_type_t status = 0;
    int32_t ap_connection_result = 0;
    provisioning_tls_info_t *prov_mode = (provisioning_tls_info_t*)tls_info;
    provisioning_type_t prov_type = prov_mode->provisioning_type;

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->registerTask(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, gp_provisioning_app_cfg->p_watchdog_service->p_cfg, &task_wdog_id);
#endif
    printf("[%s] Create...(status=%d) [%d] \n", __func__, status, prov_type);

    while (1)
    {
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->notify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, gp_provisioning_app_cfg->p_watchdog_service->p_cfg, task_wdog_id);
        gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, task_wdog_id);
#endif
        vTaskDelay(portCONVERT_MS_2_TICKS(100));
        status = provisioning_recv_msg_from_server();

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, gp_provisioning_app_cfg->p_watchdog_service->p_cfg, task_wdog_id);
        gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, task_wdog_id);
#endif
        if (PROV_COMMAND_ERROR != status)
        {
            printf("Recv MSG .. [%d] ", status);

            switch (status)
            {
               case CONNECTED:
               {
                   /* Scan list update */
                   err = start_wifi_scan(1, prov_type);

                   if (ap_list_buffer)
                   {
                       int32_t buffer_size = (int32_t)strlen(ap_list_buffer);
                       if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                       {
                           err = provisioning_send_tls_data(prov_mode->config, ap_list_buffer, buffer_size);
                       }
                        else
                       {
                           err = provisioning_send_tcp_data(ap_list_buffer, buffer_size);
                       }
                   }
                   else
                   {
                       printf("ERROR : SCAN LIST is null .... \n");
                   }
               }
               break;
               case SET_AP_SSID_PW:
               {
                   provisioning_dpm_mode_t dpm_mode_on_reboot = PROV_DPM_OFF;   // default set is STATION_MODE_DPM
                   try_connect_on_sta =1;

                   if (g_provisioning_type > (int8_t) WIFI_DEVICE_MODE_EXT_NOT_SUPPORTED)
                   {
                       /* for Platform */
                       dpm_mode_on_reboot = PROV_DPM_ON;
                   }

                   if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                   {
                       err = provisioning_send_tls_data_as_json(prov_mode->config, "RESULT_REBOOT", 0);
                   }
                   else
                   {
                       err = provisioning_send_tcp_data_as_json("RESULT_REBOOT", 0);
                   }

                   vTaskDelay(portCONVERT_MS_2_TICKS(500));

                   if (0 != provisioning_dpm_status_get(TYPE_SLEEP_MODE))
                   {
                       dpm_mode_on_reboot = (provisioning_dpm_mode_t) provisioning_dpm_status_get(TYPE_USE_DPM);
                       printf("Reset DPM mode to %d \n", dpm_mode_on_reboot);
                   }

                   if(WIFI_DEVICE_MODE_EXT_AP == get_run_mode())
                   {
                       printf("[Soft AP mode] \n");

                       if (0 != new_pw_size)
                       {
                           if (0 != new_SSID_size)
                           {
                               err = provisioning_reset_ap_to_station(new_SSID, new_pw, security_type,
                                                                      dpm_mode_on_reboot, use_hidden_ssid);
                           }
                           else
                           {
                               printf("ERROR : Something wrong.... SSID is null \n");
                           }
                       }
                       else
                       {
                           if (0 != new_SSID_size)
                           {
                               err = provisioning_reset_ap_to_station(new_SSID, new_pw, security_type,
                                                                      dpm_mode_on_reboot, use_hidden_ssid);
                           }
                           else
                           {
                               printf("ERROR : Something wrong.... SSID is null \n");
                           }
                       }
                   }
                }
                break;
                case REQ_HOMEAP_RESULT:
                {
                    if(1 == try_connect_on_sta)
                    {
                        printf("Wating to complete AP connection ... \n");
                        ap_connection_result=10;
                        if (PROV_TLS_SOCKET ==  provisioning_get_socket_type())
                        {
                            err = provisioning_send_tls_data_as_json(prov_mode->config, "RESULT_HOMEAP",ap_connection_result);
                        }
                        else
                        {
                            err = provisioning_send_tcp_data_as_json("RESULT_HOMEAP",ap_connection_result);
                        }
                        break;
                    }

                    if(1 != station_conn_status)
                    {
                        printf("NO STA connection\n");
                        ap_connection_result = 2;
                    }
                    else
                    {
                        int32_t ping_rc =0;
                        char* pingAddr = "8.8.8.8";

                        ping_rc = provisioning_ping_client(pingAddr);

                        if(FSP_SUCCESS != ping_rc)
                        {
                            ap_connection_result = 3;
                        }
                        else
                        {
                            ap_connection_result = 1;
                        }
                    }

                    if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                    {
                        err = provisioning_send_tls_data_as_json(prov_mode->config, "RESULT_HOMEAP",ap_connection_result);
                    }
                    else
                    {
                        err = provisioning_send_tcp_data_as_json("RESULT_HOMEAP",ap_connection_result);
                    }
                }
                break;

                case REQ_RESCAN:
                {
                   /* Need rescan */
                   int32_t buffer_size = 0;
                   printf("REQ reScan \n");

                   if (ap_list_buffer != NULL)
                   {
                       vPortFree(ap_list_buffer);
                       ap_list_buffer = NULL;
                   }

                   err = start_wifi_scan(1, prov_type);
                   buffer_size = (int32_t)strlen(ap_list_buffer);

                   if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                   {
                       provisioning_send_tls_data(prov_mode->config, ap_list_buffer, buffer_size);
                   }
                   else
                   {
                       provisioning_send_tcp_data(ap_list_buffer, buffer_size);
                   }
               }
               break;

               case REQ_REBOOT:
               {
                   provisioning_dpm_mode_t dpm_mode_on_reboot = PROV_DPM_OFF;   // default set is STATION_MODE_DPM

                   if (g_provisioning_type > (int8_t) WIFI_DEVICE_MODE_EXT_NOT_SUPPORTED)
                   {
                       /* for Platform */
                       dpm_mode_on_reboot = PROV_DPM_ON;
                   }

                   if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                   {
                       err = provisioning_send_tls_data_as_json(prov_mode->config, "RESULT_REBOOT", 0);
                   }
                   else
                   {
                       err = provisioning_send_tcp_data_as_json("RESULT_REBOOT", 0);
                   }

                   vTaskDelay(portCONVERT_MS_2_TICKS(500));

                   if (provisioning_dpm_status_get(TYPE_SLEEP_MODE) != 0)
                   {
                       dpm_mode_on_reboot = (provisioning_dpm_mode_t) provisioning_dpm_status_get(TYPE_USE_DPM);
                       printf("Reset DPM mode to %d \n", dpm_mode_on_reboot);
                   }

                   if (0 != new_pw_size)
                   {
                       if (0 != new_SSID_size)
                       {
                           err = provisioning_reset_ap_to_station(new_SSID, new_pw, security_type, dpm_mode_on_reboot, use_hidden_ssid);
                       }
                       else
                       {
                           printf("ERROR : something wrong.... SSID null 1 \n");
                       }
                   }
                   else
                   {
                       if (0 != new_SSID_size)
                       {
                           err = provisioning_reset_ap_to_station(new_SSID, new_pw, security_type, dpm_mode_on_reboot, use_hidden_ssid);
                       }
                       else
                       {
                           printf("ERROR : something wrong.... SSID null 2 \n");
                       }
                   }
               }
               break;
               case REQ_SOCKET_TYPE:
               {
                   if (socket_app == provisioning_get_socket_type())
                   {
                       if (PROV_TLS_SOCKET == provisioning_get_socket_type())
                       {
                           /* tls */
                           err = provisioning_send_tls_data_as_json(prov_mode->config, "SOCKET_TYPE", socket_app);
                       }
                       else
                       {
                          /* tcp */
                           err = provisioning_send_tcp_data_as_json("SOCKET_TYPE", socket_app);
                       }
                   }
                   else if (socket_app != provisioning_get_socket_type())
                   {
                       if (socket_app)
                       {
                           /* Swtiching to TLS socket from TCP mode */
                           printf("move to TLS\n");
                           err = provisioning_send_tcp_data_as_json("SOCKET_TYPE", socket_app);
                       }
                       else
                       {
                           /* Switching to TCP socket from TLS mode */
                           printf("move to TCP\n");
                           err = provisioning_send_tls_data_as_json(prov_mode->config, "SOCKET_TYPE", socket_app);
                       }
                       socket_dev = socket_app;
                   }
                   else
                   {
                       printf("\n[%s]SOCKET_TYPE UNDEFINE\n",__func__);
                   }
               }
               break;

               case CMD_ERROR:
               {
                   printf("Command Error\n");
               }
               break;

               default:
               break;
            }
        }
        else
        {
            printf("\recv MSG error .... \n\n");
        }
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, gp_provisioning_app_cfg->p_watchdog_service->p_cfg, task_wdog_id);
#endif
    }
    if(FSP_SUCCESS != err)
    {
        FSP_ERROR_LOG(err);
    }
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->unregisterTask(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl, task_wdog_id);
#endif
    vTaskDelete(NULL);
}

/*******************************************************************************************************************//**
 * Thread for receiving TLS data from phone.
 *
 * @param[in] tls_info Provisioning TLS information.
 *
 * @return  void
 **********************************************************************************************************************/
static void provisioning_tls_server_thread(void *tls_info)
{
    provisioning_tls_info_t *provisioning_mode = (provisioning_tls_info_t *)tls_info;

    printf("[%s] Create TLS... \n", __func__);

    provisioning_run_tls_server(provisioning_mode->config);
}

/*******************************************************************************************************************//**
 * Thread for receiving TCP data from phone.
 *
 * @param[in] tls_info Provisioning TLS information.
 *
 * @return  void
 **********************************************************************************************************************/
static void provisioning_tcp_server_thread(void *tls_info)
{
    fsp_err_t err = FSP_SUCCESS;
    int status;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addrlen;
    char data_buf[PROVISION_TCP_RX_BUF_SZ] = {0x00, };
    uint32_t rx_bytes;
    provisioning_user_data_t sample_data;
    int32_t buffer_size;

    if (NULL == tls_info)
    {
        printf("[%s] tls_info is NULL \n", __func__);
        err = FSP_ERR_INVALID_ARGUMENT;
        vTaskDelete(NULL);
        return;
    }
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    uint8_t task_wdog_id = WATCHDOG_SERVICE_W_NOT_REGISTERED_ID;
#endif

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->registerTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
        &task_wdog_id);
#endif
    new_SSID = (char *)pvPortMalloc(MAX_AP_SSID_SIZE);
    new_pw = (char *)pvPortMalloc(MAX_AP_PASSWORD_SIZE);
    server_URL = (char *)pvPortMalloc(MAX_CUSTOMER_SERVER_SIZE);

    printf("[%s] Create ... \n", __func__);
    provisioning_mutex_init(&provision_mutex, "provision_mutex");
    /* Create MSG Queue  */
    provision_message_queue = xQueueCreate(PROVISION_MSG_CMD, sizeof(provisioning_user_data_t));

    if (provision_message_queue == NULL)
    {
        printf("[%s] Failed to create queue \"provision_message_queue\" \n", __func__);
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->unregisterTask(
            gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
            task_wdog_id);
#endif
        vTaskDelete(NULL);

        return;
    }

    provision_TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (provision_TCP_socket < 0)
    {
        printf("[%s] Failed to assign listen socket\r\n", __func__);
        goto exit;
    }

    /* Listen */
    printf("\n[%s] socket().. status=%d  \n ", __func__, provision_TCP_socket);

    memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port        = htons(TCP_PROVISION_PORT_NUM);

    status = bind(provision_TCP_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (status < 0)
    {
        printf("[%s] Failed to bind socket(%d)\r\n", __func__, status);
        goto exit;
    }

    status = listen(provision_TCP_socket, 1); // diafreertoswork: org. backlog # 4
    if (status < 0)
    {
        printf("[%s] Failed to listen socket(%d)\r\n", __func__, status);
        goto exit;
    }

    while (pdTRUE)
    {
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->notify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                   gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                                                                   task_wdog_id);
#endif
        printf("Wait Accept...\n");

        /* Accept */
        memset(&client_addr, 0x00, sizeof(client_addr));
        client_addrlen = sizeof(client_addr);
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                    task_wdog_id);
#endif
        provision_TCP_csocket = accept(provision_TCP_socket, (struct sockaddr *) &client_addr, &client_addrlen);
        if (provision_TCP_csocket < 0)
        {
            printf("[%s] Failed to accept client connects\r\n", __func__);
            continue;
        }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
        gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
            gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
            gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
            task_wdog_id);
#endif
        printf("[%s] Connected client(%d.%d.%d.%d:%d)\r\n",
               __func__,
               (int16) (ntohl(client_addr.sin_addr.s_addr) >> 24) & 0x0ff,
               (int16) (ntohl(client_addr.sin_addr.s_addr) >> 16) & 0x0ff,
               (int16) (ntohl(client_addr.sin_addr.s_addr) >> 8) & 0x0ff,
               (int16) (ntohl(client_addr.sin_addr.s_addr)) & 0x0ff,
               (int16) (ntohs(client_addr.sin_port)));

        printf("Accept OK\n");

        while (1)
        {
            memset(data_buf, 0, sizeof(data_buf));
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
            gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(
                gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                task_wdog_id);
#endif
            status = recv(provision_TCP_csocket, data_buf, sizeof(data_buf), 0);
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
            gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
                gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                task_wdog_id);

            gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(
                gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                task_wdog_id);
#endif
            if (status > 0)
            {
                /* Parsing json */
                rx_bytes = (uint32_t)status;

                printf("[%s] read ..  status = %d length = %ld \n", __func__, status, rx_bytes);

                if (rx_bytes > 0)
                {
                    sample_data.status = provisioning_parse_json_data(data_buf);

                    if (check_connection_ap) // concurrent mode::not used
                    {
                        char * data_buffer = NULL;
                        printf("Now checking to connect Home AP...\n");

                        data_buffer = provisioning_add_command_to_json("RESULT_HOMEAP", 0);

                        if (data_buffer != NULL)
                        {
                            buffer_size = (int32_t) strlen(data_buffer);

                            err = provisioning_send_tcp_data(data_buffer, buffer_size);
                            vPortFree(data_buffer);
                        }
                    }
                    else
                    {
                        err = provisioning_send_msg_to_client(&sample_data);
                    }
                }
            }
            else if (status == 0)
            {
                PROV_OAL_MSLEEP(200);
                printf("[%s] recv timeout...\n", __func__);
            }
            else
            {
                PROV_OAL_MSLEEP(200);
                printf("dis connection...\n");
                close(provision_TCP_csocket);
                break;;
            }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
            gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
                gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                task_wdog_id);
#endif
        }
    }

exit:
    printf("Provisioning TCP Server Exit\n");
    if(FSP_SUCCESS != err)
    {
        FSP_ERROR_LOG(err);
    }
    close(provision_TCP_socket);
    close(provision_TCP_csocket);

    if (new_SSID)
    {
        vPortFree(new_SSID);
    }

    if (new_pw)
    {
        vPortFree(new_pw);
    }

    if (server_URL)
    {
        vPortFree(server_URL);
    }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->unregisterTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        task_wdog_id);
#endif
    vTaskDelete(NULL);
}

/*******************************************************************************************************************//**
 * Entry function for APP provisioning
 *
 * @param[in]  provisioning_type    Provisioning type.
 *
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_OUT_OF_MEMORY    Heap is too small or NULL to create a Provisioning task thread.
 **********************************************************************************************************************/
static fsp_err_t provisioning_initialize(provisioning_type_t provisioning_type)
{
    fsp_err_t err = FSP_SUCCESS;
    int status;
    TaskHandle_t p_tcp_server_thread = NULL;
    TaskHandle_t p_tls_server_thread = NULL;
    TaskHandle_t p_provisioning_client_thread = NULL;
    provisioning_tls_info_t * tlsinfo = NULL;

#if defined(__PROVISION_ATCMD__)
    atcmd_provstat(ATCMD_PROVISION_START);
    g_provisioning_feature = PROV_FEATURE_ATCMD;
#endif // __PROVISION_ATCMD__

    if (GENERIC_AP_SDK == provisioning_type)
    {
        printf("\n=======================================================\n");
        printf("[Start Provisioning with TCP/TLS] .. Soft AP Mode \n");
        printf("=======================================================\n");
    }
    else if (GENERIC_CONCUR_SDK == provisioning_type)
    {
        printf("\n=======================================================\n");
        printf("[Start Provisioning with TCP/TLS] .. Concurrent Mode(STA/AP) \n");
        printf("=======================================================\n");
    }

    tlsinfo = (provisioning_tls_info_t *) pvPortMalloc(sizeof(provisioning_tls_info_t));
    tlsinfo->provisioning_type = provisioning_type;
    tlsinfo->config = &provisioning_tls_svr_config;
    g_provisioning_type = provisioning_type;

    /* TCP provisioning */

    /* Create provision TCP thread on Soft-AP mode */
    status = xTaskCreate(provisioning_tcp_server_thread,
                         APP_SOFTAP_PROV_NAME,
                         APP_SOFTAP_PROV_STACK_SZ,
                         (void *) tlsinfo,
                         tskIDLE_PRIORITY + 2,
                         &p_tcp_server_thread);
    if (status != pdPASS)
    {
        printf("[%s] Failed to create TCP svr thread\r\n", __func__);
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    /* TLS provisioning */

    /* Create provision TLS thread on Soft-AP mode */
    status = xTaskCreate(provisioning_tls_server_thread,
                         APP_SOFTAP_APROV_NAME,
                         APP_SOFTAP_PROV_STACK_SZ,
                         (void *) tlsinfo,
                         tskIDLE_PRIORITY + 2,
                         &p_tls_server_thread);
    if (status != pdPASS)
    {
        printf("[%s] Failed to create TLS svr thread\r\n", __func__);
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    /* Create client task */
    status = xTaskCreate(provisioning_client_thread,
                         "ProvClient",
                         APP_SOFTAP_PROV_STACK_SZ,
                         (void *) tlsinfo,
                         tskIDLE_PRIORITY + 3,
                         &p_provisioning_client_thread);
    if (status != pdPASS)
    {
        printf("[%s] Failed to create prov client thread\r\n", __func__);
        err = FSP_ERR_OUT_OF_MEMORY;
    }

    return err;
}


/*******************************************************************************************************************//**
 * SoftAP Provisioning application thread calling function.
 *
 * @param[in]  p_ctrl               Pointer to Provisioning instance control structure.
 *
 **********************************************************************************************************************/
static void provisioning_start_thread()
{
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    uint8_t task_wdog_id = WATCHDOG_SERVICE_W_NOT_REGISTERED_ID;
#endif
    int sysmode = 0;
    bool first_run = true;
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->registerTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
        &task_wdog_id);
    gp_provisioning_app_cfg->p_watchdog_service->p_api->notify(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                               gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
                                                               task_wdog_id);
#endif
#if defined(__PROVISION_ATCMD__)
    atcmd_provstat(ATCMD_PROVISION_IDLE);
#endif // __PROVISION_ATCMD__

SOFTAP_MODE:
#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->suspend(gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
                                                                task_wdog_id);
#endif

    sysmode = get_sys_mode();
    if (((int) WIFI_DEVICE_MODE_EXT_AP == sysmode) || ((int) WIFI_DEVICE_MODE_EXT_AP_STATION == sysmode))
    {
        printf("Starting AP Provisioning\r\n");
        provisioning_type_t provisioning_type = PROVISIONING_TYPE;
        provisioning_initialize((provisioning_type_t) provisioning_type); // support only AP_MODE
    }
    else
    {
        vTaskDelay(portCONVERT_MS_2_TICKS(5000));
        if (1 != chk_network_ready(WLAN0_IFACE))
        {
            if(first_run)
            {
                printf("To start AP Provisioning, press the Reset Button (BTN1) for 5 seconds\r\n");
                first_run = false;
            }
            goto SOFTAP_MODE;
        }
    }

#if 1 == provisioning_WATCHDOG_SERVICE_ENABLE
    gp_provisioning_app_cfg->p_watchdog_service->p_api->resumeAndNotify(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        gp_provisioning_app_cfg->p_watchdog_service->p_cfg,
        task_wdog_id);
    gp_provisioning_app_cfg->p_watchdog_service->p_api->unregisterTask(
        gp_provisioning_app_cfg->p_watchdog_service->p_ctrl,
        task_wdog_id);
#endif

    vTaskDelete(NULL);
}



/*******************************************************************************************************************//**
 * Initialize the Provisioning service.
 *
 * @param[in]  p_args               Pointer to provisioning scan callback argument.
 *
 * @retval FSP_SUCCESS              Function completed successfully.
 * @retval FSP_ERR_INVALID_STATE    AP List is not initialized.
 * @retval FSP_ERR_NOT_ENABLED      User disabled current argument (AP).
 **********************************************************************************************************************/
static fsp_err_t provisioning_add_ap_to_json(provisioning_scan_callback_args_t *p_args)
{
    fsp_err_t err = FSP_SUCCESS;
    cJSON *newAP;

    if (scan_ap_list_json == NULL)
    {
        printf("NO AP LIST buffer\n");
        err = FSP_ERR_INVALID_STATE;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (!p_args->is_listed)
    {
        err = FSP_ERR_NOT_ENABLED;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    cJSON_AddItemToArray(scan_ap_list_json, newAP = cJSON_CreateObject());
    cJSON_AddItemToObject(newAP, "index", cJSON_CreateNumber(p_args->index));
    cJSON_AddItemToObject(newAP, "SSID", cJSON_CreateString(p_args->p_ssid));
    cJSON_AddItemToObject(newAP, "securityType", cJSON_CreateNumber(p_args->security_mode));
    cJSON_AddItemToObject(newAP, "signal", cJSON_CreateNumber(p_args->signal_strength));

    return err;
}

static void provisioning_get_app_thing_name()
{
    char *nvram_saved_name = NULL;

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, provisioning_NVRAM_CFG_THINGNAME, &nvram_saved_name);
#endif

    if ((GENERIC_AWS == g_provisioning_type) || (ATCMD_AWS == g_provisioning_type))
    {
        /* If Fleet Provisioning is enabled, get the Thing Name from Fleet Provisioning */
        if (provisioning_get_fleet_provisioning_status() == 1)
        {
            provisioning_get_fleet_provisioning_thing_name();
        }

        if ((nvram_saved_name == NULL) && (provisioning_get_fleet_provisioning_status() == 1))
        {
            gs_app_thing_name = (char *) gs_app_fleet_thing_name;
        }
        else if ((nvram_saved_name == NULL) && (provisioning_get_fleet_provisioning_status() == 0))
        {
            gs_app_thing_name = provisioning_initialize_string((const char *) PROVISIONING_APP_THING_NAME);
        }
        else
        {
            if ((provisioning_get_fleet_provisioning_status() == 1) &&
                (strncmp(nvram_saved_name, gs_app_fleet_thing_name, strlen(gs_app_fleet_thing_name)) != 0))
            {
                gs_app_thing_name = (char *) gs_app_fleet_thing_name;
            }
            else
            {
                gs_app_thing_name = nvram_saved_name;
            }
        }
    }
    else
    {
        if (nvram_saved_name == NULL)
        {
            gs_app_thing_name = provisioning_initialize_string((const char *) PROVISIONING_APP_THING_NAME);
        }
        else
        {
            gs_app_thing_name = nvram_saved_name;
        }
    }
}

static int provisioning_get_fleet_provisioning_status()
{
    return 0;
}

static char * provisioning_initialize_string(const char *string_data)
{
    size_t string_length;
    char *target_string;

    string_length = strlen(string_data);

    if(string_length > 0)
    {
        target_string = (char *)pvPortMalloc(string_length + 1);
        memset(target_string, 0x00, (string_length + 1));
        snprintf(target_string, string_length + 1, "%s", string_data);
    }

    printf("[%s] cJSON_CreateObject string_length %d [%s]\n", __func__, string_length, target_string);

    return target_string;
}

static void provisioning_get_fleet_provisioning_thing_name ()
{
    return;
}

static void provisioning_mutex_init (provisioning_mutex_t *p_mutex, char *p_name)
{
    char result[128];

    sprintf(result, "provisioning_mutex_t_%lu", (long unsigned int) p_mutex);

    if (NULL == p_name)
    {
        p_name = result;
    }

    *p_mutex = xSemaphoreCreateMutex();
    if (NULL == *p_mutex)
    {
        printf("[%s] tx_xmit_lock Semaphore Create Error!\n", __func__);
        configASSERT(0);
    }
}

static int32_t provisioning_mutex_lock (provisioning_mutex_t *p_mutex)
{
    long status;

    status = xSemaphoreTake(*p_mutex, (TickType_t)portMAX_DELAY);
    if (pdTRUE != status)
    {
        configASSERT(0);

        return 0;
    }

    return 1;
}

static int32_t provisioning_mutex_unlock(provisioning_mutex_t *p_mutex)
{
    long status;

    status = xSemaphoreGive(*p_mutex);
    if (pdTRUE != status)
    {
        configASSERT(0);

        return 0;
    }

    return 1;
}
