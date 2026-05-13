/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Handles Wi-Fi connection and TWT configurations.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "app_task.h"

#if CFG_CLI
 #include "rm_cli_w.h"
#endif

#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
 #include "rm_wifi_user_app_gpio_handle.h"
#endif

/* Function prototypes */
void wifi_normal_scan_example(void);
static void wifi_init(void);
void print_ep_info_banner(void);

/***********************************************************************************************************************
 * Function Name: wifi_init
 * Description  : Initializes Wi-Fi module and PMGR (if enabled).
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
static void wifi_init(void)
{
#if CFG_WIFI
 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                               g_wifi_cfg.p_watchdog_service->p_cfg);
 #else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);

    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
                        dg_configWDOG_IDLE_RESET_VALUE);

    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
 #endif

 #if CFG_PMGR
  #if TC_WIFI_ON_DPM
    fsp_err_t err;
    /* PMGR should be opened once globally */
    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    assert(FSP_SUCCESS == err);

    RM_PMGR_W_dpm_job_name_set("my_main", 0);
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl,
                                             PMGR_CONSTRAINT_POWER_RETENTION);
  #endif
 #endif /* CFG_PMGR */

    WIFI_On();
#endif /* CFG_WIFI */
}

/***********************************************************************************************************************
 * Function Name: app_task_entry
 * Description  : Application main task entry function.
 * Arguments    : pvParameters - task parameter (unused)
 * Return Value : None
 ***********************************************************************************************************************/
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    print_ep_info_banner();
    wifi_init();

#if CFG_WIFI
 #if defined(__SUPPORT_FACTORY_RESET_BTN__)
    rm_wifi_app_gpio_handle_create_event();
    rm_wifi_app_gpio_handle_task_start();
 #endif

 #if CFG_PMGR
  #if TC_WIFI_ON_DPM
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl,
                                                PMGR_CONSTRAINT_POWER_RAM);
  #endif
 #endif
#endif /* CFG_WIFI */

#if CFG_CLI
 #if defined(__SUPPORT_APP_CONSOLE_INPUT__)
 #endif

    cli_open();

#endif /* CFG_CLI */

    wifi_normal_scan_example();

    while (1)
    {
        vTaskDelay(200);
    }

    WIFI_Off();
    vTaskDelete(NULL);
}
