/***********************************************************************************************************************
 * File Name    : new_thread0_entry.c
 * Description  : Handles wifi connection.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "new_thread0.h"
#if CFG_CLI
 #include "rm_cli_w.h"
#endif

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

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

 #if CFG_PMGR
  #if TC_WIFI_ON_DPM

    /*
     *  open PMGR (PMGR should be opened once globally)
     *  internally call add_sleep_constraint(..., PMGR_CONSTRAINT_POWER_RAM) to block sleep
     */
    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    assert(FSP_SUCCESS == err);

    /* set job name */
    RM_PMGR_W_dpm_job_name_set("my_main", 0);

    /* add wake source */
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    /* add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RETENTION);

   #ifdef RM_MAP_PERSISTANT_W

    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
   #endif

    BaseType_t sem_err = xSemaphoreGive(g_sys_init_semaphore);
    assert(pdTRUE == sem_err);
  #endif
 #endif                                /* CFG_PMGR  */
 #if CFG_CLI
    cli_open();
 #endif                                /* CFG_CLI  */

    WIFI_On();

 #if (ATCMD_IF_SUPPORT == 1)

    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
 #endif

 #if CFG_PMGR
  #if TC_WIFI_ON_DPM

    /* remove constraint (RAM) */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RAM);
  #endif
 #endif                                /* CFG_PMGR */
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
