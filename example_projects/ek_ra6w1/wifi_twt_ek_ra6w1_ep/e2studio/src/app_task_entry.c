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
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "twt_config.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif // CFG_PMGR
#include "iface_defs.h"
#include "net_network_main.h"
 
/*
* Global variables
*/
TaskHandle_t g_app_main_task_handle = NULL;
uint32_t notify_val = 0;
int twt_setup_retry_cnt = 0;
twt_rtm_t *p_twt = NULL;
static twt_rtm_t g_twt_local = {0}; 
struct twt_setup_req setup_params = {0};
struct twt_teardown_req teardown_params = {0};
int twt_ap_not_support = 0;
 
static void wifi_twt_config(void);
 
/* Wi-Fi events: association, disconnection */
static void wifi_event_handler(WIFIEvent_t *arg)
{
	const WIFIEvent_t *pxEvent = (const WIFIEvent_t *)arg;
    switch (pxEvent->xEventType)
    {
        case eWiFiEventConnected:
            printf("[WIFI] Association complete.\n");
            if (RM_PMGR_W_dpm_is_wakeup() == pdFALSE)
            {
                xTaskNotify(g_app_main_task_handle, eWiFiEventConnected, eSetValueWithOverwrite);
            }
            break;
 
        case eWiFiEventDisconnected:
            printf("[WIFI] Disconnected from AP.\n");
            p_twt->twt_session_active = 0;
            xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
            break;
 
        default:
            printf("[WIFI] Other event: %d\n", pxEvent->xEventType);
            break;
    }
}
 
/**
****************************************************************************************
* @brief callback function for twt events
* @param[in] pxEvent
* @return None
****************************************************************************************
*/
static void twt_event_callback(WIFIEvent_t *pxEvent)
{
    const WIFIEvent_t *p_event = (const WIFIEvent_t *)pxEvent;

    switch (p_event->xEventType)
    {
        case eWiFiEventExtTWT:
        {
        	const WIFIEventExt_t *pxExtEvent = (const WIFIEventExt_t *)pxEvent;
            switch (pxExtEvent->xInfo.xTWT.xEvent)
            {
                case WIFI_TWT_EVENT_SESSION_SUCCESS:
                    printf("[TWT] Session established successfully.\n");
                    p_twt->twt_session_active = 1;
                    break;
 
                case WIFI_TWT_EVENT_AP_REJECTED:
                    printf("[TWT] AP rejected the TWT request.\n");
                    break;
 
                case WIFI_TWT_EVENT_AP_RESPONSE_PARAM_NOT_MATCHED:
                    printf("[TWT] AP response parameters did not match request.\n");
                    break;
 
                case WIFI_TWT_EVENT_TEARDOWN_SUCCESS:
                    printf("[TWT] TWT session teardown successful.\n");
                    xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
                    break;
 
                case WIFI_TWT_EVENT_AP_TEARDOWN_SUCCESS:
                    printf("[TWT] AP tore down the TWT session.\n");
                    p_twt->twt_session_active = 0;
                    xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
                    break;
 
                case WIFI_TWT_EVENT_MAX_RETRIES_REACHED:
                    printf("[TWT] Maximum retries reached for TWT setup.\n");
                    break;
 
                case WIFI_TWT_EVENT_AP_NO_SUPPORT:
                    //printf("[TWT] AP does not support TWT.\n");
                    twt_ap_not_support = 1;
                    xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
                    break;
 
                case  WIFI_TWT_EVENT_TWT_REQ_ABORT:
                    printf("[TWT] TWT setup aborted.\n");
                    break;
 
                default:
                   printf("[TWT] Unknown sub-event: %d\n", pxExtEvent->xInfo.xTWT.xEvent);
                   break;
               }
            break;
        }
 
        default:
            printf("[TWT] Unknown event: %d\n", p_event->xEventType);
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
 
#if CFG_PMGR

    if (RM_PMGR_W_dpm_is_enabled())
    {
        unsigned int len = RM_PMGR_W_user_rtm_get(TWT_RTM_NAME, (unsigned char **)&p_twt);
        
        if (len == 0)
        {
            unsigned int ret = (int)RM_PMGR_W_user_rtm_pool_alloc(TWT_RTM_NAME,(void **)&p_twt, sizeof(twt_rtm_t), 0);
            if (ret)
            {
                printf("[APP] Failed to allocate RTM for TWT(%u)\n", ret);
                p_twt = NULL;
            }
            else
            {
                memset(p_twt, 0, sizeof(twt_rtm_t));
            }
        }
    }
#endif

    if (p_twt == NULL)
    {
        p_twt = &g_twt_local;
        memset(p_twt, 0, sizeof(twt_rtm_t));
        printf("[APP] Using fallback local TWT storage at %p\n", (void *)p_twt);
    }
}
 
/* App task entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
 
    /* TODO: add your own code here */
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();
 
#if CFG_WIFI
#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl, g_wifi_cfg.p_watchdog_service->p_cfg);
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
 
    wifi_twt_config();
 
    WIFI_RegisterEvent(eWiFiEventConnected,   wifi_event_handler);
    WIFI_RegisterEvent(eWiFiEventDisconnected,wifi_event_handler);
    WIFI_RegisterEvent((WIFIEventType_t)eWiFiEventExtTWT, twt_event_callback);
 
#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();
 
    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif
#endif // CFG_WIFI
 
    /* Set wakeup button and ISR */
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);
 
#if (ATCMD_IF_SUPPORT == 1)
    // Initialize and start the AT command interface
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif
 
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

    if ((RM_PMGR_W_dpm_is_wakeup()==pdTRUE) && !p_twt->twt_session_active)
    {
        xTaskNotify(g_app_main_task_handle, eTWTCFG, eSetValueWithOverwrite);
    }

    while (1)
    {
        xTaskNotifyWait(0, 0xFFFFFFFF, &notify_val, portMAX_DELAY);
        switch (notify_val)
        {
            case eTWTCFG:
                //printf("[APP] TWT event\n");
                break;
 
            case eWiFiEventConnected:
                printf("[APP] Wifi connect event\n");
                break;
 
            default:
                printf("[APP] Unknown event: %lu\n", (unsigned long)notify_val);
        }
 
        if (!p_twt->twt_session_active)
        {
            twt_setup_retry_cnt = 0;
#if CFG_PMGR
            /* Hold sleep */
            RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
#endif
            while (!RM_WIFI_dpm_supp_is_connected() || (ra6w1_network_main_check_ip_addr(WLAN0_IFACE)==pdFALSE))
            {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
 
            while ((!p_twt->twt_session_active) && (twt_setup_retry_cnt < 3) && (!twt_ap_not_support))
            {
                twt_setup_retry_cnt++;
                WIFI_TwtSetup(&setup_params);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
 
#if CFG_PMGR
            /* Resume sleep */
            RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
#endif
        }
    }
 
    WIFI_Off();
}
