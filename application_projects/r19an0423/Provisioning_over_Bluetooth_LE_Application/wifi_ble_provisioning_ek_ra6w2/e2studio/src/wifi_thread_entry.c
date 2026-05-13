/***********************************************************************************************************************
 * File Name    : wifi_thread_entry.c
 * Description  : Handle wifi connection.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "wifi_thread.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif //CFG_CLI
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif //CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)

/* WiFi Thread entry function */
/* pvParameters contains TaskHandle_t */

void wifi_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

/* TODO: add your own code here */
#if CFG_WIFI
#if TC_WIFI_ON_DPM
    fsp_err_t err;
#endif //TC_WIFI_ON_DPM

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                               g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,
                                                             g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif //WIFI_CFG_WATCHDOG_SERVICE_ENABLE

    /* Init CC312 HW engine and psa crypto */
    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

#if CFG_PMGR
#if TC_WIFI_ON_DPM
    /*
        open PMGR (PMGR should be opened once globally)
        internally call add_sleep_constraint(..., PMGR_CONSTRAINT_POWER_RAM) to block sleep
    */
    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);

    assert(FSP_SUCCESS == err);

    /* set job name */
    RM_PMGR_W_dpm_job_name_set("my_main", 0);

    /* add wake source */
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    /* add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RETENTION);
#endif //TC_WIFI_ON_DPM
#endif //CFG_PMGR

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif //SUPPORT_FSP_RM_OTA_W

#if CFG_CLI
    cli_open();
#endif //CFG_CLI

    BaseType_t sem_err = xSemaphoreGive(g_sys_init_semaphore);

    assert(pdTRUE == sem_err);
    WIFI_On();
#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif //defined(__SUPPORT_FACTORY_RESET_BTN__)

 #if (ATCMD_IF_SUPPORT == 1)

    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
 #endif //(ATCMD_IF_SUPPORT == 1)

#if CFG_PMGR
#if TC_WIFI_ON_DPM

    /* remove constraint (RAM) */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RAM);
#endif //TC_WIFI_ON_DPM
#endif //CFG_PMGR
#endif //CFG_WIFI

#if CFG_CLI
#if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
#endif //__SUPPORT_APP_CONSOLE_INPUT__
#endif // CFG_CLI

    while (1)
    {
        vTaskDelay (200);
    }

    WIFI_Off();
}
