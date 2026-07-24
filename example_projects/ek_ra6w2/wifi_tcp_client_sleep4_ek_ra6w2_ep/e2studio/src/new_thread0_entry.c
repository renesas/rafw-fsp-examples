/***********************************************************************************************************************
* File Name    : new_thread0_entry.c
* Description  : Handle wifi connection and twt configurations
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "new_thread0.h"
#include "tcp_client_dpm.h"
#include "common_utils.h"
#include "config.h"
#include "lwip/netif.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif // CFG_PMGR

#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "rm_tcp_client_w"
#define EP_APP_DESCRIPTION \
    "This example acts as a TCP client in DPM mode and " \
    "establishes a connection with a TCP server running on the Wi-Fi network."
extern TaskHandle_t new_thread0;
uint32_t event = EVENT_VAL;
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
       vTaskDelay(portCONVERT_MS_2_TICKS(1000)); // 1 sec delay for BLE thread.
       WIFI_SetListenInterval(10);
       WIFI_SetPsMode(true);
       APP_PRINT_INFO("Sleep 4 enabled\n");
#endif //CFG_PMGR
       if (new_thread0 != NULL)
       {
           xTaskNotify(new_thread0, WIFI_EVENT_CONNECTED, eSetBits);
       }
   }
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);

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
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
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
#endif // CFG_CLI

    WIFI_On();
    netif_set_status_callback(netif_default, netif_status_callback);
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);

#if CFG_PMGR
    BaseType_t sem_err = xSemaphoreGive(g_sys_init_semaphore);
    assert(pdTRUE == sem_err);
#endif //CFG_PMGR
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

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

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

