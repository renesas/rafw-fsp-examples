/***********************************************************************************************************************
 * File Name    : wifi_ctrl_thread_entry.c
 * Description  : Wifi thread handling.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "wifi_ctrl_thread.h"
#if CFG_CLI
 #include "rm_cli_w.h"
 #include "rm_cli_w_easysetup.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
 #include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "common_utils.h"

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void wifi_ctrl_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: add your own code here */
#if CFG_WIFI
    fsp_err_t err;

    print_ep_info_banner(MODULE_NAME, EP_VERSION, EP_INFO);

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

 #if CFG_PMGR

    /*
     * Open PMGR (PMGR should be opened once globally)
     * internally call add_sleep_constraint(..., PMGR_CONSTRAINT_POWER_RAM) to block sleep
     */
    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    assert(FSP_SUCCESS == err);

    /* Set job name */
    RM_PMGR_W_dpm_job_name_set("pmgrmon", 0);

    /* Add wake source */
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    /* add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RETENTION);
 #endif                                /* CFG_PMGR */
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
 #endif

    WIFI_On();

    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);

 #if defined(__SUPPORT_FACTORY_RESET_BTN__)

    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
 #endif
#endif                                 // CFG_WIFI

#if (ATCMD_IF_SUPPORT == 1)

    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif

#if CFG_WIFI
 #if CFG_PMGR
  #if (ATCMD_DA14XXX_CODELESS == 1) && (ATCMD_PMGR_SUPPORT_ENABLE == 1)
    BaseType_t sem_err = xSemaphoreTake(g_atcmd_init_semaphore, pdMS_TO_TICKS(5000));
    assert(pdTRUE == sem_err);
  #endif

    /* remove constraint (RAM) */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RAM);
 #endif                                /* CFG_PMGR*/
#endif

#if CFG_CLI
 #if defined(__SUPPORT_APP_CONSOLE_INPUT__)
    create_easy_setup_task(true);
 #endif                                /* __SUPPORT_APP_CONSOLE_INPUT__ */
#endif                                 /* CFG_CLI */

    while (1)
    {
        vTaskDelay(10);
    }
}
