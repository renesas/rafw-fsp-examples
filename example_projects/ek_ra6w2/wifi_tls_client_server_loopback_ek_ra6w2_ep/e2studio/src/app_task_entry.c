/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : init wifi, tls client init.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "app_task.h"
#include "lwip/netif.h"
#include "common_utils.h"
#include "tls_server_client.h"

#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif //CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)

#define EP_APP_VERSION 1.0
#define EP_APP_MODULE_NAME "rm_mbedtls_w"
#define EP_APP_DESCRIPTION \
    "This example shows TLS loop back echo server client communication\r" \
    "upon successful initialization, Client will receive echo from server"

TaskHandle_t xAppTaskHandle;

static void wifi_init()
{
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

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

    WIFI_On();
#endif //CFG_WIFI
}

/*
    app_task entry function
    pvParameters contains TaskHandle_t
*/
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);
    xAppTaskHandle = xTaskGetCurrentTaskHandle();
    wifi_init();
    start_tls_server();//start tls server
    vTaskDelay (10);
    tls_client_init();//start tls client
    while (1)
    {
        vTaskDelay (1);
    }

    WIFI_Off();
    vTaskDelete(NULL);
}
