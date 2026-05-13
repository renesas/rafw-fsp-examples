/***********************************************************************************************************************
* File Name    : app_task_entry.c
* Description  : init Qspi, WiFi.
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

#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

#if CFG_WIFI

#if TC_WIFI_ON_DPM
    fsp_err_t err;
#endif

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(
        g_wifi_cfg.p_watchdog_service->p_ctrl,
        g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);

    R_WDOG_W_Freeze(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
        true);

    R_WDOG_W_TimeoutSet(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
        dg_configWDOG_IDLE_RESET_VALUE);

    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(
        g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif

    /* Init CC312 HW engine and psa crypto */
    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the persistent storage */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

#if CFG_PMGR
#if TC_WIFI_ON_DPM
    /*
     * open PMGR (PMGR should be opened once globally)
     */
    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    assert(FSP_SUCCESS == err);

    pmgr_instance_ctrl_t * p_instance_ctrl = RM_PMGR_W_get_ctrl();

    /* add wake source */
    RM_PMGR_W_set_wake_source(
        g_pmgr_w_ins.p_ctrl,
        PMGR_WAKE_SOURCE_WIFI);

    /* add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(
        g_pmgr_w_ins.p_ctrl,
        PMGR_CONSTRAINT_POWER_RETENTION);
#endif
#endif /* CFG_PMGR */

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif

#if CFG_CLI
    cli_open();
#endif /* CFG_CLI */

    WIFI_On();
    vTaskDelay(2000);

    void r_qspi_w_psram_basic_example(void);

    /* PSRAM test */
    r_qspi_w_psram_basic_example();

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif

#if (ATCMD_IF_SUPPORT == 1)
    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif

#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif

#if CFG_PMGR
#if TC_WIFI_ON_DPM
    /* remove constraint (RAM) */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(
        g_pmgr_w_ins.p_ctrl,
        PMGR_CONSTRAINT_POWER_RAM);

    if (p_instance_ctrl->debug_runtime_flag == PMGR_DEBUG_RAM_ENABLED)
    {
        p_instance_ctrl->ram_counter_cause
            [PMGR_DBG_RAM_CONSTRAINT_PMGR_OPEN_CALLER]--;
    }
#endif
#endif /* CFG_PMGR */

#endif /* CFG_WIFI */

#if CFG_CLI
#if defined(__SUPPORT_APP_CONSOLE_INPUT__)
    create_easy_setup_task(true);
#endif
#endif /* CFG_CLI */

    while (1)
    {
        vTaskDelay(200);
    }

    WIFI_Off();
}
