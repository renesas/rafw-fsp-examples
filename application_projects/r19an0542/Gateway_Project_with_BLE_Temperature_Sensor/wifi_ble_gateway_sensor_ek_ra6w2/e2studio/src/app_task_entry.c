/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Wi-Fi connect → IP acquire → BLE Gateway (MQTT) start
 **********************************************************************************************************************/

#include "app_task.h"
#include "common_utils.h"
#include "ra6w1_platform_nvparam.h"
#include "custom_app.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "lwip/netif.h"
#include "ip4_addr.h"
#include "common_data.h"
#include <stdbool.h>

/* =====================================================================================================================
 * Macros
 * ===================================================================================================================*/
#define WIFI_EVENT_IP_READY         (1 << 0)
#define APP_IDLE_DELAY_MS           1000
#define WIFI_HARD_CODED_SSID        "SSID"
#define WIFI_HARD_CODED_PASSWORD    "PASSWORD"
#define WIFI_HARD_CODED_SECURITY    eWiFiSecurityWPA2

/* =====================================================================================================================
 * Global Variables
 * ===================================================================================================================*/
TaskHandle_t xAppTaskHandle;
static bool ip_notified = false;

/* =====================================================================================================================
 * Wi-Fi Parameters
 * ===================================================================================================================*/
WIFINetworkParams_t net_params =
{
    .ucChannel                  = 0,
    .ucSSID                     = WIFI_HARD_CODED_SSID,
    .ucSSIDLength               = sizeof(WIFI_HARD_CODED_SSID) - 1,
    .xPassword.xWPA.cPassphrase = WIFI_HARD_CODED_PASSWORD,
    .xPassword.xWPA.ucLength    = sizeof(WIFI_HARD_CODED_PASSWORD) - 1,
    .xSecurity                  = WIFI_HARD_CODED_SECURITY,
};

/* =====================================================================================================================
 * Private Functions
 * ===================================================================================================================*/
/***********************************************************************************************************************
 * Function Name: netif_status_callback
 * Description  : Callback triggered when network interface status changes.
 * Argument     : netif
 * Return Value : None
 **********************************************************************************************************************/
static void netif_status_callback(struct netif *netif)
{
    if ((!ip_notified) &&
        netif_is_up(netif) &&
        (!ip_addr_isany(&netif->ip_addr)))
    {
        APP_PRINT(" IP assigned: %s\r\n", ip4addr_ntoa(netif_ip4_addr(netif)));
        xTaskNotify(xAppTaskHandle, WIFI_EVENT_IP_READY, eSetBits);
        ip_notified = true;
    }
}

/* =====================================================================================================================
 * Public Functions
 * ===================================================================================================================*/
/***********************************************************************************************************************
 * Function Name: app_task_entry
 * Description  : Main application task entry.
 * Argument     : pvParameters
 * Return Value : None
 **********************************************************************************************************************/
void app_task_entry(void *pvParameters)
{
    uint32_t notify_val = 0;
    FSP_PARAMETER_NOT_USED(pvParameters);
    xAppTaskHandle = xTaskGetCurrentTaskHandle();
    print_ep_info_banner("BLE_GATEWAY", "1.0", "WiFi + BLE Gateway");

#if CFG_WIFI
    /* -------------------------------------------------------------------------------------------------------------
     * Wi-Fi Mode Configuration
     * -----------------------------------------------------------------------------------------------------------*/
    WIFI_SetMode(eWiFiModeStation);
#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl,g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl,dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif

    /* -------------------------------------------------------------------------------------------------------------
     * PSA Crypto Setup
     * -----------------------------------------------------------------------------------------------------------*/
    RM_WIFI_mbedtls_setup_psa_crypto();
#if CFG_PMGR && TC_WIFI_ON_DPM
    g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
    RM_PMGR_W_dpm_job_name_set("pmgrmon", 0);
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RETENTION);
#endif

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    /* Signal BLE thread to proceed after storage initialisation */
#if CFG_WIFI && CFG_PMGR
    xSemaphoreGive(g_sys_init_semaphore);
#endif

    /* -------------------------------------------------------------------------------------------------------------
     * Wi-Fi Start
     * -----------------------------------------------------------------------------------------------------------*/
    WIFI_On();

    /* -------------------------------------------------------------------------------------------------------------
     * Network Interface Callback Registration
     * -----------------------------------------------------------------------------------------------------------*/
    netif_set_status_callback(netif_default, netif_status_callback);

    /* -------------------------------------------------------------------------------------------------------------
     * Connect to Access Point
     * -----------------------------------------------------------------------------------------------------------*/
    APP_PRINT(" Connecting to WiFi\r\n");
    WIFI_ConnectAP(&net_params);

    /* -------------------------------------------------------------------------------------------------------------
     * Wait for IP Address Assignment
     * -----------------------------------------------------------------------------------------------------------*/
    xTaskNotifyWait(0, WIFI_EVENT_IP_READY, &notify_val, portMAX_DELAY);
    APP_PRINT(" WiFi connected — IP: %s\r\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
#endif /* CFG_WIFI */

    /* -------------------------------------------------------------------------------------------------------------
     * Start BLE Gateway (MQTT) Task
     * -----------------------------------------------------------------------------------------------------------*/
    APP_PRINT(" Starting BLE Gateway task...\r\n");
    mqtt_initialize();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(APP_IDLE_DELAY_MS));
    }
}
