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
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
#if CFG_WIFI
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
#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif

    /* Set wakeup button and ISR */
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);
#endif
#if (ATCMD_IF_SUPPORT == 1)
    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif
#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif
#if CFG_CLI
#if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
#endif // __SUPPORT_APP_CONSOLE_INPUT__
#endif // CFG_CLI
#if CFG_PMGR
    /*
     * DPM (auto power-save / sleep) control.
     *
     * RM_PMGR_W_Open() (see hal_entry.c) adds one PMGR_CONSTRAINT_SLEEP_PROHIBITED
     * constraint at startup, which blocks all sleep and keeps the console/UART alive.
     * Removing it here is what arms auto-DPM after WiFi connects (console is then lost).
     *
     * APP_DPM_AUTO_ENABLE:
     *   0 (default) - DPM disabled by default: the constraint stays held, so the
     *                 device never auto-enters DPM and the console stays available.
     *   1           - original behavior: remove the constraint so DPM engages
     *                 automatically after WiFi connects.
     *
     * To enable DPM at runtime (no rebuild) once the user is ready:
     *   AT : AT+PMGRCONSTRAINT=2,4        (2 = remove, 4 = SLEEP_PROHIBITED)
     *   CLI: pmgr remove_constraint PMGR_CONSTRAINT_SLEEP_PROHIBITED
     *   Code: g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl,
     *                                                     PMGR_CONSTRAINT_SLEEP_PROHIBITED);
     * The constraint is a counter: DPM only engages when the SLEEP_PROHIBITED count
     * reaches 0. Check it with "AT+PMGRCONSTRAINT=?" (4th value) or "pmgr status".
     * This runtime enable is session-only; after a reboot DPM is disabled again.
     */
#ifndef APP_DPM_AUTO_ENABLE
#define APP_DPM_AUTO_ENABLE (0)
#endif
#if APP_DPM_AUTO_ENABLE
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //APP_DPM_AUTO_ENABLE
#endif //CFG_PMGR
    while (1)
        vTaskDelay(portMAX_DELAY);
    WIFI_Off();
}
