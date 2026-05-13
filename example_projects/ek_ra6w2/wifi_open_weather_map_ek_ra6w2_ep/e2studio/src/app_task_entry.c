/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Handles Wi-Fi connection and weather API queries.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "app_task.h"
#include "common_utils.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "lwip/netif.h"
#include "common_data.h"

#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif

#include "weather_app.h"
#include <stdbool.h>

/* ===================== MACROS ===================== */

#define WIFI_EVENT_CONNECTED    (5)

/* ===================== GLOBAL VARIABLES ===================== */

uint32_t notified_value = EVENT_VAL;
TaskHandle_t g_app_main_task_handle = NULL;
static bool already_notified = false;

/* ===================== WIFI PARAMETERS ===================== */

WIFINetworkParams_t net_params =
{
    .ucChannel                  = 0,
    .xPassword.xWPA.cPassphrase = PSWD,
    .ucSSID                     = SSID,
    .xPassword.xWPA.ucLength    = PSWD_LEN,
    .ucSSIDLength               = SSID_LEN,
    .xSecurity                  = eWiFiSecurityWPA2,
};

/* ===================== NETIF CALLBACK ===================== */

void netif_status_callback(struct netif *netif)
{
    if (!already_notified &&
        netif_is_up(netif) &&
        !ip_addr_isany_val(netif->ip_addr))
    {
        APP_PRINT("\nIP assigned: %s\n", ipaddr_ntoa(&netif->ip_addr));

        xTaskNotify(g_app_main_task_handle,
                    WIFI_EVENT_CONNECTED,
                    eSetValueWithOverwrite);

        already_notified = true;
    }
}

/* ===================== WIFI INITIALIZATION ===================== */

void wifi_init(void)
{
#if CFG_WIFI

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

    WIFI_On();

#endif
}

/* ===================== MAIN APPLICATION TASK ===================== */

void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    WIFI_SetMode(eWiFiModeStation);

    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    print_ep_info_banner();

    wifi_init();

    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    /* Register network callback */
    netif_set_status_callback(netif_default, netif_status_callback);

    /* Connect to WiFi */
    APP_PRINT("\nConnecting to WiFi SSID: %s\n", SSID);

    WIFI_ConnectAP(&net_params);

    /* ===================== EVENT LOOP ===================== */

    while (1)
    {
        xTaskNotifyWait(0x00,
                        0xFFFFFFFF,
                        &notified_value,
                        portMAX_DELAY);

        switch (notified_value)
        {
            case WIFI_EVENT_CONNECTED:
            {
                APP_PRINT("\nStarting Weather Monitor...\n");

                if (start_weather_monitor() != FSP_SUCCESS)
                {
                    APP_PRINT("Weather monitor start failed\n");
                }

                break;
            }

            case HTTPS_EVENT_CLIENT_RESULT:
            {
                /* Restart request every 5 seconds */

                vTaskDelay(pdMS_TO_TICKS(5000));

                start_weather_monitor();

                break;
            }

            default:
                break;
        }
    }

    WIFI_Off();

    vTaskDelete(NULL);
}
