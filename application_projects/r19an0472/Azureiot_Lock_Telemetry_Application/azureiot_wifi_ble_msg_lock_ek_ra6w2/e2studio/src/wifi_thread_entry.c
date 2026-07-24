/***********************************************************************************************************************
* File Name    : wifi_thread_entry.c
* Description  : Wi-Fi thread entry function for the Azure IoT application.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "wifi_thread.h"
#include "ra6w1_platform_nvparam.h"
#include "lwip/prot/dhcp.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "iface_defs.h"
#define AZUREIOT_W_TASK_NAME    "customer_azureiot_w"
#define AZUREIOT_W_TASK_SIZE    512
static TaskHandle_t gs_azureiot_task_handle = NULL;

static unsigned char sntp_client;
static int sntp_client_period_time;
static char sntp_gmt_timezone[8];
static int sntp_timezone_int;
static char sntp_svr_addr[256];
static char sntp_svr_addr1[256];
static char sntp_svr_addr2[256];
void azure_twin_dpm_auto_start(void *arg);

#ifndef SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL
#define SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    3600*36 /*131072*/
#endif

static void load_WIFI_profile(map_persistant_w_instance_ctrl_t * p_ctrl, WIFINetworkParams_t *params)
{
    char * result_ptr = NULL;
    int encryption = 0;

    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SSID_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0) {
        strncpy((char*)params->ucSSID, result_ptr, strlen(result_ptr));
        params->ucSSIDLength = strlen((char*)params->ucSSID);
    }

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_SECURITY_0, &encryption);
    params->xSecurity = encryption;

    result_ptr = NULL;
    RM_MAP_PERSISTANT_W_Read_STRING(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_ENCKEY_0, &result_ptr);
    if (result_ptr && strlen(result_ptr) > 0) {
        strncpy((char*)params->xPassword.xWPA.cPassphrase, result_ptr, strlen(result_ptr));
        params->xPassword.xWPA.ucLength = strlen(params->xPassword.xWPA.cPassphrase);
    }
}

static unsigned char get_ready_DHCP(void)
{
    int iface = 0;    //iface_select
    struct netif *netif = NULL;

    netif = netif_get_by_index(iface+2);

    return (dhcp_get_state(netif) == DHCP_STATE_BOUND);
}

static void execute_SNTP(unsigned char sntp_client, int client_period_time, int timezone_int,
    char *p_svr_addr, char *p_svr_addr1, char *p_svr_addr2)
{

    if (sntp_client) {
        long temp;
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                    NVR_KEY_SNTP_SYNC_PERIOD, client_period_time);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                        NVR_KEY_SNTP_SERVER_DOMAIN, p_svr_addr);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                        NVR_KEY_SNTP_SERVER_DOMAIN_1, p_svr_addr1);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                        NVR_KEY_SNTP_SERVER_DOMAIN_2, p_svr_addr2);
#else
        write_nvram_syscfg_int(NVR_KEY_SNTP_SYNC_PERIOD, client_period_time);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN, p_svr_addr);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_1, p_svr_addr1);
        write_nvram_syscfg_string(NVR_KEY_SNTP_SERVER_DOMAIN_2, p_svr_addr2);
#endif

        temp = (timezone_int / 60) * 60;
        if (timezone_int != 0) {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG,
                                        NVR_KEY_TIMEZONE, timezone_int);
#else
            write_nvram_syscfg_int(NVR_KEY_TIMEZONE, timezone_int);
#endif
            R_RTC_W_CalendarTimeZoneSet(R_RTC_W_GetCtrl(), &temp);
        } else {
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_SYSCFG, NVR_KEY_TIMEZONE);
#else
            delete_nvram_syscfg_env(NVR_KEY_TIMEZONE);
#endif
        }

        /* Set run flag */
        set_sntp_use(1);
    }
}

static void execute_WIFI_profile()
{
    WIFINetworkParams_t xNetworkParams = {0};
    int is_profile_present = 0;
    map_persistant_w_instance_ctrl_t *p_ctrl = RM_MAP_PERSISTANT_W_get_ctrl();

    RM_MAP_PERSISTANT_W_Read_INT(p_ctrl, ENV_GROUP_WIFIPROFILE, WIFI_PROFILE_COMPLETE, &is_profile_present);
    if(is_profile_present == 1)
    {
        printf("Loading wifi profile..\n");
        load_WIFI_profile(p_ctrl, &xNetworkParams);
    }
    else
    {
        printf("No wifi profile found\n");
        return;
    }

    //check SNTP params
    sntp_client = 1;
    sntp_client_period_time = 3600 * (SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL / 3600);
    strcpy(sntp_gmt_timezone, "00:00");
    sntp_timezone_int = 0;
    memset(sntp_svr_addr, 0, 256);
    strcpy(sntp_svr_addr, DFLT_SNTP_SERVER_DOMAIN);
    memset(sntp_svr_addr1, 0, 256);
    strcpy(sntp_svr_addr1, DFLT_SNTP_SERVER_DOMAIN_1);
    memset(sntp_svr_addr2, 0, 256);
    strcpy(sntp_svr_addr2, DFLT_SNTP_SERVER_DOMAIN_2);

    //apply SNTP params
    execute_SNTP(sntp_client, sntp_client_period_time, sntp_timezone_int, sntp_svr_addr, sntp_svr_addr1, sntp_svr_addr2);

    printf("SSID: %s\n", (char*)xNetworkParams.ucSSID);
    printf("Passphrase: %s\n", (char*)xNetworkParams.xPassword.xWPA.cPassphrase);
    printf("Encryption: %d\n", xNetworkParams.xSecurity);

    WIFI_ConnectAP(&(xNetworkParams));

    if (!RM_PMGR_W_dpm_is_wakeup())
    {
        if (get_netmode(0) == DHCPCLIENT) {
            while (!get_ready_DHCP())
            {
                printf("Waiting DHCP bound...\n");
                vTaskDelay( pdMS_TO_TICKS( 1000 ) );
            }
            //apply DNS params
            set_dhcpCientIP_to_staticIP();
        }
    }
}
void door_lock_app_task_start() {
    void * arg;

    xTaskCreate(azure_twin_dpm_auto_start,
                (const char *) AZUREIOT_W_TASK_NAME,
				AZUREIOT_W_TASK_SIZE,
                (void *) NULL,
                (OS_TASK_PRIORITY_USER + 1),
                &gs_azureiot_task_handle);
};

/* WiFi Thread entry function */
/* pvParameters contains TaskHandle_t */

void wifi_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
#if CFG_WIFI
    fsp_err_t err;

 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                 g_wifi_cfg.p_watchdog_service->p_cfg);
 #else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
                                                               g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(
            g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
 #endif

    /* Init CC312 HW engine and psa crypto */
    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif
#if CFG_CLI
    cli_open();
#endif //CFG_CLI


    BaseType_t sem_err = xSemaphoreGive(g_sys_init_semaphore);
    assert(pdTRUE == sem_err);

    WIFI_On();
#if defined(__SUPPORT_WIFI_USER_GPIO__)
    rm_wifi_app_gpio_wakeup_set(BTN_WAKEUP_PIN, BSP_WAKEUP_EDGE_LOW, NULL);
#endif //__SUPPORT_WIFI_USER_GPIO__

 #if defined(__SUPPORT_FACTORY_RESET_BTN__)
 #if defined(__SUPPORT_WIFI_USER_GPIO__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif //__SUPPORT_WIFI_USER_GPIO__
#endif
#endif

#if (ATCMD_IF_SUPPORT == 1)
    // Initialize and start the AT command interface
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif
#if CFG_CLI
 #if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
 #endif /* __SUPPORT_APP_CONSOLE_INPUT__ */
#endif // CFG_CLI

    if (!RM_PMGR_W_dpm_is_wakeup())
        {
           // execute_WIFI_profile(); //azure_check
        }

    door_lock_app_task_start();

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

    while (1)
        vTaskDelay (200);

    WIFI_Off();

}
