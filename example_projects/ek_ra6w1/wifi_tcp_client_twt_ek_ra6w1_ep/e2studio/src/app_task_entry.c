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
#include "iface_defs.h"
 
TaskHandle_t g_app_main_task_handle = NULL;
uint32_t event = EVENT_VAL;
 
struct twt_setup_req setup_params = {0};
struct twt_teardown_req teardown_params = {0};
static void wifi_twt_config(void);
 
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
* @brief callback function for handling WiFi events
* @param[in] pxEvent
* @return None
****************************************************************************************
*/
static void wifi_event_handler(WIFIEvent_t *arg)
{
	const WIFIEvent_t *pxEvent = (const WIFIEvent_t *)arg;
    switch (pxEvent->xEventType)
    {
        case eWiFiEventConnected:
        printf("[WIFI] Association complete.\n");
        //xTaskNotify(g_app_main_task_handle, WIFI_CONNECTED_EVENT, eSetValueWithOverwrite);
        break;

        case eWiFiEventDisconnected:
            printf("[WIFI] Disconnected from AP.\n");
            if (!p_conf)
            {
#if CFG_PMGR
 
                if (RM_PMGR_W_dpm_is_enabled())
                {
                	unsigned int rtm_len = RM_PMGR_W_user_rtm_get(TCPCL_RTM_NAME, (unsigned char **)&p_conf);
                    if (rtm_len == 0)
                    {
                        printf("Failed to load twt_session_active flag in RTM\n");
                        p_conf = NULL;
                    }
                    else
                    {
                        p_conf->twt_session_active = 0;
                    }
                }
#endif // CFG_PMGR
            }
            else
            {
                p_conf->twt_session_active = 0;
            }
            xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
            break;
        default:
            printf("[WIFI] Other event: %d\n", pxEvent->xEventType);
            break;
    }
}
 
/**
****************************************************************************************
* @brief configure twt parameters
* @param[in] None
* @return None
****************************************************************************************
*/
static void wifi_twt_config(void)
{
    teardown_params.neg_type =  TWT_NEG_TYPE;
    teardown_params.all_twt = 1;
    /* configure TWT parameters*/
    setup_params.vif_idx    = 0;
    setup_params.setup_type = 1;
    setup_params.conf.wake_int_mantissa = TWT_WAKE_INT_MANTISSA;
    setup_params.conf.wake_int_exp      = TWT_WAKE_INT_EXPONENT;
    setup_params.conf.min_twt_wake_dur  = TWT_WAKE_INT_MIN_TWT_WAKE_DUR;
    setup_params.conf.wake_dur_unit     = 1;  /* Fixed value */
    setup_params.conf.flow_type         = TWT_FLOW_TYPE;
    setup_params.conf.trigger           = TWT_TRIGGER;
    setup_params.conf.neg_type          = TWT_NEG_TYPE;
    setup_params.auto_setup             = TWT_AUTO_SETUP;
}
 
/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
 
    /* TODO: add your own code here */
    WIFI_RegisterEvent(eWiFiEventConnected,   wifi_event_handler);
    WIFI_RegisterEvent(eWiFiEventDisconnected, wifi_event_handler);
#if CFG_WIFI
#if CFG_PMGR
#if TC_WIFI_ON_DPM
    fsp_err_t err;
#endif
#endif
#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl, g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif
 
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
#endif
#endif //CFG_PMGR
#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif
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
    
#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();
 
    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif
#if (ATCMD_IF_SUPPORT == 1)
    // Initialize and start the AT command interface
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif
#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif
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
 
    /*configure TWT parameters*/
    wifi_twt_config();

    /* tcp_client task */
    tcp_client_init();
 
    while (1)
        vTaskDelay (200);
    WIFI_Off();
}
