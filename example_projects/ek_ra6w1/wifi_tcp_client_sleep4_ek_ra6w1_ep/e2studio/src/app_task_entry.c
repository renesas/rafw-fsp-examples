/***********************************************************************************************************************

* File Name    : app_task_entry.c

* Description  : Handle wifi connection and twt configurations

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/
#include "app_task.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif
#include "lwip/netif.h"
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "tcp_client_dpm.h"
#include "common_utils.h"
#include "config.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif // CFG_PMGR

#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "rm_tcp_client_w"
#define EP_APP_DESCRIPTION \
    "This example acts as a TCP client in DPM mode and " \
    "establishes a connection with a TCP server running on the Wi-Fi network."

extern TaskHandle_t app_task;
uint32_t event = EVENT_VAL;
#if !CFG_CLI
WIFINetworkParams_t net_params =
 {
     .ucChannel                  = CHANNEL,
    .xPassword.xWPA.cPassphrase = PASSPHRASE,
    .ucSSID                     = SSID,
    .xPassword.xWPA.ucLength    = PASSS_LEN,
    .ucSSIDLength               = SSID_LEN,
     .xSecurity                  = eWiFiSecurityWPA2,
 };
#endif

/*
 * Static functions
 */
/**
 ****************************************************************************************
 * @brief callback function for checking interface status
 * @param[in] p_netif
 * @return None
 ****************************************************************************************
 */
static void netif_status_callback(struct netif *p_netif)
{

    /* Check interface is up and have ip assigned */
    if (netif_is_up(p_netif) && !ip_addr_isany_val(p_netif->ip_addr))
    {
        APP_PRINT_INFO("IP assigned: %s\n", ipaddr_ntoa(&p_netif->ip_addr));
#if CFG_PMGR
        WIFI_SetListenInterval(10);
        WIFI_SetPsMode(true);
        APP_PRINT_INFO("Sleep 4 enabled\n");
#endif //CFG_PMGR
        if (app_task != NULL)
        {
            xTaskNotify(app_task, WIFI_EVENT_CONNECTED, eSetBits);
        }
    }

}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);

#if CFG_WIFI
 #if TC_WIFI_ON_DPM
    fsp_err_t err;

 #endif

 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                 g_wifi_cfg.p_watchdog_service->p_cfg);
 #else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
                                                               g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
 #endif

    /* Init CC312 HW engine and psa crypto */
    RM_WIFI_mbedtls_setup_psa_crypto();

#if CFG_CLI
    cli_open();
#endif // CFG_CLI

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    WIFI_On();
    netif_set_status_callback(netif_default, netif_status_callback);
#if defined(__SUPPORT_FACTORY_RESET_BTN__)

    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

#endif

#if CFG_CLI
#if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
#endif // __SUPPORT_APP_CONSOLE_INPUT__

#endif // CFG_CLI

#if CFG_PMGR
    if (RM_PMGR_W_dpm_is_wakeup() == pdFALSE)
    {
#endif
        /* Wait for Wi-Fi Connection Event */
        while (event != WIFI_EVENT_CONNECTED)
        {
            xTaskNotifyWait(0x00, 0xFFFFFFFF, &event, portMAX_DELAY);
        }
        APP_PRINT_INFO("WiFi connected\n");
#if CFG_PMGR
    }
#endif

    /* tcp_client task */
    tcp_client_init();

    while (1)
        vTaskDelay (portMAX_DELAY);

    WIFI_Off();
}

