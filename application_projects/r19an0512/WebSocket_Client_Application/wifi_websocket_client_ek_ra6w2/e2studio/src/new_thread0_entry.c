/***********************************************************************************************************************
 * File Name    : new_thread0_entry.c
 * Description  : Entry file for the thread New Thread
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "new_thread0.h"
#include "common_utils.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif

#if defined(__SUPPORT_WEBSOCKET_CLIENT__)
/* WebSocket client demo task — defined in websocket_client_app.c */
extern void ws_app_demo_task(void *pvParameters);
#endif

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

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

#if CFG_CLI
    cli_open();
#endif // CFG_CLI

    WIFI_On();

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif
#endif // CFG_WIFI

    /* Set wakeup button and ISR */
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq0);

#if CFG_WIFI
#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif
#endif // CFG_WIFI
#if CFG_CLI
 #if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
 #endif /* __SUPPORT_APP_CONSOLE_INPUT__ */
#endif // CFG_CLI

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

#if defined(__SUPPORT_WEBSOCKET_CLIENT__)
    /* Start WebSocket client demo task */
    xTaskCreate(ws_app_demo_task, "ws_app", 1024, NULL, 6, NULL);
#endif

    while (1)
        vTaskDelay(portMAX_DELAY);

    WIFI_Off();
}
