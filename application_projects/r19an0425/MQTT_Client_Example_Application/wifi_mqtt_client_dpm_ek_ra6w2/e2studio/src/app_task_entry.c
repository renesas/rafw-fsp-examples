/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Entry file for the thread New Thread
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "app_task.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#include "../ra/eclipse/mqtt_client/inc/client/mqtt_client.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__) && defined(__SUPPORT_WIFI_USER_GPIO__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "mqtt_client.h"
#include "mqtt.h"
#include "common_utils.h"

extern mqttParamForRtm mqttParams;

TaskHandle_t g_app_main_task_handle = NULL;
uint32_t event = EVENT_VAL;
#define WIFI_EVENT_CONNECTED 7


/* New Thread entry function */
/* pvParameters contains TaskHandle_t */

void netif_status_callback(struct netif *netif)
{
    if (netif_is_up(netif) && !ip_addr_isany_val(netif->ip_addr))
    {
       APP_PRINT("IP assigned: %s\n", ipaddr_ntoa(&netif->ip_addr));
       xTaskNotify(g_app_main_task_handle, WIFI_EVENT_CONNECTED, eSetBits);
    }
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry (void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

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
    netif_set_status_callback(netif_default, netif_status_callback);

#if defined(__SUPPORT_FACTORY_RESET_BTN__) && defined(__SUPPORT_WIFI_USER_GPIO__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif
#endif // CFG_WIFI

#if defined(__SUPPORT_WIFI_USER_GPIO__)
    /* Set wakeup button and ISR */
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq0);
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

    if (RM_PMGR_W_dpm_is_wakeup() == pdFALSE)
    {
    while(event != WIFI_EVENT_CONNECTED )
    {
       printf("wait \n");
       xTaskNotifyWait(0, 0xFFFFFFFF, &event, portMAX_DELAY);
    }
    }

#if CFG_PMGR
    if ((RM_PMGR_W_dpm_wakeup_type_get(0) == DPM_RTCTIME_WAKEUP))
    {
    	printf(" Waking from RTC DPM\n");
        mqtt_client_send_message( MQTT_PUB_TOPIC , MQTT_PUBLISH_MSG);
    }
#endif

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

#if CFG_PMGR
    /* Set 10 second periodic wake up ,DPM wake up user time */
    int ret = RM_PMGR_W_dpm_user_wakeup_timer_set(DPM_WAKEUP_INTERVAL_US);
#endif

    if (!RM_PMGR_W_dpm_is_wakeup())
    {
    /* enabled mqtt TLS */
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_TLS, 1);

    mqtt_client_cfg_sync_rtm(0, NULL, RRQ61X_CONF_INT_MQTT_TLS, 1);

    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_BROKER_IP, MQTT_BROKER_IP);

    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_PORT, MQTT_BROKER_PORT);

    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_BROKER_IP,
                             MQTT_BROKER_IP,
                             0, 0);

    mqtt_client_cfg_sync_rtm(0,
                             NULL,
                             RRQ61X_CONF_INT_MQTT_PORT,
                             MQTT_BROKER_PORT);

    /* save subscribed topic */
    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_SUB_TOPIC, MQTT_SUB_TOPIC);

    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_SUB_TOPIC,
                             MQTT_SUB_TOPIC,
                             0,
                             0);

    /* Save to DPM memory */
    mqtt_client_save_to_dpm_user_mem();

    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_PUB_TOPIC, MQTT_PUB_TOPIC);

    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_PUB_TOPIC,
                             MQTT_PUB_TOPIC,
                             0,
                             0);

    /* Enable AUTO reconnect (critical for DPM) */
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_AUTO, 1);

    mqtt_client_cfg_sync_rtm(0, NULL,
                             RRQ61X_CONF_INT_MQTT_AUTO,
                             MQTT_AUTO_MODE);
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_SUB, 1);

    mqtt_client_cfg_sync_rtm(0, NULL,
                             RRQ61X_CONF_INT_MQTT_SUB,
                             1);
    }

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

    while (1)
        vTaskDelay(portMAX_DELAY);
    WIFI_Off();
}
