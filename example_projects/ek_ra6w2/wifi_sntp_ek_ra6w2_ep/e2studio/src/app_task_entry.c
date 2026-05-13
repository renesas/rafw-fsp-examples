/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Initialize Wi-Fi and run SNTP example.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "app_task.h"
#include "config.h"
#include "lwip/netif.h"
#include "sntp_example.h"

void wifi_init();
void netif_status_callback(struct netif *netif);

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif

/* Global Variables */
TaskHandle_t g_app_main_task_handle = NULL;
uint32_t notified_value = EVENT_VAL;
WIFIScanResult_t wifi_scan_data[MAX_WIFI_SCAN_RESULTS];
WIFINetworkParams_t net_params = {0};

void netif_status_callback(struct netif *netif)
{
    if (netif_is_up(netif) && !ip_addr_isany_val(netif->ip_addr))
    {
        APP_PRINT("\n>>> WIFI ConnectAP: Success\n");
        APP_PRINT("IP assigned: %s\n", ipaddr_ntoa(&netif->ip_addr));
        xTaskNotify(g_app_main_task_handle, WIFI_EVENT_CONNECTED, eSetBits);
    }
}

void wifi_init()
{
#if CFG_WIFI
    WIFIReturnCode_t wifi_err;
    char *passphrase = NULL;

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

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    /* Wi-Fi On */
    WIFI_On();
 
    /* setting callback for netif status */
    netif_set_status_callback(netif_default, netif_status_callback);

    wifi_err = WIFI_SetMode(eWiFiModeStation);
    if (wifi_err)
    {
        APP_PRINT("\n%s: WIFI_SetMode failed with wifi_err=%d\n", __func__, wifi_err);
    }

    /* Start WIFI Scanning */
    APP_PRINT("\n>>> Wi-Fi normal scanning started..\n");
    wifi_err = WIFI_Scan(&wifi_scan_data[0], MAX_WIFI_SCAN_RESULTS);
    if (wifi_err)
    {
        APP_PRINT("\n%s: WIFI_Scan failed with wifi_err=%d\n", __func__, wifi_err);
    }

    /* Print Wi-Fi scanning result */
    display_scan_result(wifi_scan_data);
    APP_PRINT("Wi-Fi normal scanning done..\n");

    /* Input the wanted AP number */
    APP_PRINT("\n>>> Input the wanted AP number in RTT Viewer..\n");
    uint32_t i = atoi(process_input_data());
    APP_PRINT("\tSelected AP: %s\n", wifi_scan_data[i].ucSSID);

    if (wifi_scan_data[i].xSecurity == eWiFiSecurityOpen)
    {
    	APP_PRINT("\n>>> Security is Open..\n");
    }
    else
    {
    APP_PRINT("\n>>> Input the password in RTT Viewer..\n");
    passphrase = process_input_data();
    APP_PRINT("\tThe password: %s\n", passphrase);
	}

	/* Configure Wi-Fi connection information */
    net_params.ucChannel = wifi_scan_data[i].ucChannel;
    memcpy(net_params.ucSSID, wifi_scan_data[i].ucSSID, wifi_scan_data[i].ucSSIDLength);
    net_params.ucSSIDLength = wifi_scan_data[i].ucSSIDLength;	
    net_params.xSecurity = wifi_scan_data[i].xSecurity;
    net_params.xPassword.xWPA.ucLength = strlen(passphrase);
    memcpy(net_params.xPassword.xWPA.cPassphrase, passphrase,net_params.xPassword.xWPA.ucLength);

    /* Connect the selected AP */
    wifi_err = WIFI_ConnectAP(&net_params);
    if (wifi_err)
    {
        APP_PRINT("\n%s: WIFI_ConnectAP failed with wifi_err=%d\n", __func__, wifi_err);
    }
#endif
}

void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    print_ep_info();
    wifi_init();

    while(notified_value != WIFI_EVENT_CONNECTED)
    {
        xTaskNotifyWait(0, 0xFFFFFFFF, &notified_value, portMAX_DELAY);
    }

    sntp_example();
    print_current_time();

    while (1)
    	vTaskDelay (200);
	
    WIFI_Off();
}
