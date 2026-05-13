/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Handles Wi-Fi connection and TWT configurations.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **********************************************************************************************************************/
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

/* ===================== GLOBAL FLAGS ===================== */

volatile bool g_wifi_connected = false;

#define WEATHER_TIMER_EVENT   (1 << 1)

/* ===================== MACROS ===================== */

#define WIFI_EVENT_CONNECTED    (5)

/* ===================== GLOBALS ===================== */

uint32_t notified_value = EVENT_VAL;
TaskHandle_t g_app_main_task_handle = NULL;
static bool already_notified = false;

/* ===================== WIFI PARAMS ===================== */

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

void set_sys_time(void)
{
    struct tm correction;
    memset(&correction, 0x00, sizeof(struct tm));

    correction.tm_year  = 2025 - 1900;
    correction.tm_mon   = 12 - 1;
    correction.tm_mday  = 2;
    correction.tm_isdst = -1;

    R_RTC_W_CalendarTimeSet(R_RTC_W_GetCtrl(), &correction);
}

void print_ep_info(void)
{
    fsp_pack_version_t version;
    R_FSP_VersionGet(&version);

    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3, EP_VERSION);
    APP_PRINT(BANNER_4,
              version.version_id_b.major,
              version.version_id_b.minor,
              version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
    APP_PRINT(EP_INFO);
}

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

/* ===================== APP TASK ===================== */

void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    WIFI_SetMode(eWiFiModeStation);
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    print_ep_info();
    wifi_init();

    RM_WIFI_mbedtls_setup_psa_crypto();

#if CFG_PMGR
#if TC_WIFI_ON_DPM
    g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    RM_PMGR_W_dpm_job_name_set("pmgrmon", 0);
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);
    g_pmgr_w_ins.p_api->add_sleep_constraint(
        g_pmgr_w_ins.p_ctrl,
        PMGR_CONSTRAINT_POWER_RETENTION);
#endif
#endif

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    set_sys_time();
    netif_set_status_callback(netif_default, netif_status_callback);

    g_timer_one_shot_mode.p_api->open(
        g_timer_one_shot_mode.p_ctrl,
        g_timer_one_shot_mode.p_cfg);

    g_timer_one_shot_mode.p_api->start(
        g_timer_one_shot_mode.p_ctrl);

    WIFI_ConnectAP(&net_params);

    if (start_weather_monitor() != FSP_SUCCESS)
    {
        APP_PRINT("Error starting weather monitor\n");
    }

    while (1)
    {
        xTaskNotifyWait(0x00,
                        0xFFFFFFFF,
                        &notified_value,
                        portMAX_DELAY);

        if (notified_value & WEATHER_TIMER_EVENT)
        {
            APP_PRINT("TIMER EVENT: restarting weather monitor\n");
            start_weather_monitor();

            /* Clear the bit so switch doesn't get confused. */
            notified_value &= ~WEATHER_TIMER_EVENT;
        }

        switch (notified_value)
        {
            case HTTPS_EVENT_SERVER_RECVED:
                break;

            case HTTPS_EVENT_SERVER_ERR_RESULT:
                break;

            case HTTPS_EVENT_CLIENT_RESULT:
                vTaskDelay(pdMS_TO_TICKS(5000));
                start_weather_monitor();
                break;

            case HTTPS_EVENT_CLIENT_RECVED:
                break;

            case HTTPS_EVENT_CLIENT_RECVED_DECODED:
                break;

            case HTTPS_EVENT_CLIENT_GET_DONE:
                break;

            case WIFI_EVENT_CONNECTED:
                break;

            default:
                stop_weather_monitor();
                break;
        }
    }

    WIFI_Off();
    vTaskDelete(NULL);
}
