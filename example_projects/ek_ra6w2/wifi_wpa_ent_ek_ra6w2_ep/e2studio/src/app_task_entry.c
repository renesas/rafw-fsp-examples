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
#include "ping_socket.h"
#include "wpa_ent_example.h"

void wifi_init();
void netif_status_callback(struct netif *netif);
int lwip_ping(const char *host, int count, int timeout_ms);

/* Global Variables */
TaskHandle_t g_app_main_task_handle = NULL;
uint32_t notified_value = EVENT_VAL;
WIFIScanResult_t wifi_scan_data[MAX_WIFI_SCAN_RESULTS];
WIFINetworkParamsExt_t net_params = {0};

/* Network connection status callback */
void netif_status_callback(struct netif *netif)
{
    if (netif_is_up(netif) && !ip_addr_isany_val(netif->ip_addr))
    {
        xTaskNotify(g_app_main_task_handle, WIFI_EVENT_CONNECTED, eSetBits);
    }
}

/* Wi-Fi module initialization */
void wifi_init()
{
#if CFG_WIFI
    WIFIReturnCode_t wifi_err;
    char *wpa_id = NULL;
    char *wpa_pw = NULL;

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->open(g_wifi_cfg.p_watchdog_service->p_ctrl, g_wifi_cfg.p_watchdog_service->p_cfg);
#else
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->open(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_cfg);
    R_WDOG_W_Freeze(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, true);
    R_WDOG_W_TimeoutSet(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl, dg_configWDOG_IDLE_RESET_VALUE);
    g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_api->refresh(g_wifi_cfg.p_watchdog_service->p_cfg->p_wdt->p_ctrl);
#endif

    RM_WIFI_mbedtls_setup_psa_crypto();

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the persistant storage */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    /* Wi-Fi On */
    WIFI_On();

    /* Setting callback for NETIF status */
    netif_set_status_callback(netif_default, netif_status_callback);

    /* Setting station mode */
    wifi_err = WIFI_SetMode(eWiFiModeStation);
    if (wifi_err)
    {
        APP_PRINT("\n%s: WIFI_SetMode failed with wifi_err=%d\n", __func__, wifi_err);
    }

    /* Disconnect WIFI if any connection is ongoing*/
   wifi_err = WIFI_Disconnect();
   if (wifi_err)
   {
       APP_PRINT("\n%s: WIFI_Disconnect failed with wifi_err=%d\n", __func__, wifi_err);
   }

   /* Start WIFI Scanning */
   APP_PRINT("\n>>> WPA enterprise scanning started..\n");
   wifi_err = WIFI_Scan(&wifi_scan_data[0], MAX_WIFI_SCAN_RESULTS);
   if (wifi_err)
   {
       APP_PRINT("\n%s: WIFI_Scan failed with wifi_err=%d\n", __func__, wifi_err);
   }

   /* Print Wi-Fi scanning result */
   display_scan_result(wifi_scan_data);
   APP_PRINT("WPA enterprise scanning done..\n");

   /* Input the wanted AP number */
   APP_PRINT("\n>>> Input the wanted WPA enterprise AP number in RTT Viewer..\n");
   uint32_t i = atoi(process_input_data());
   APP_PRINT("\tSelected AP: %s\n", wifi_scan_data[i].ucSSID);

  /* Configure Wi-Fi connection information */
  net_params.ucBand = BAND;
  memcpy(net_params.xNetworkParams.ucSSID, wifi_scan_data[i].ucSSID, wifi_scan_data[i].ucSSIDLength);
  net_params.xNetworkParams.ucSSIDLength = wifi_scan_data[i].ucSSIDLength;
  net_params.xNetworkParams.xSecurity = wifi_scan_data[i].xSecurity;
  net_params.xEntNetParams.ucEntAuthType = ENT_AUTH,
  net_params.xEntNetParams.ucEntAuthProto = AUTH_PRO,

   /* Input the radius server ID of the wanted WPA enterprise AP */
   APP_PRINT("\n>>> Input the WPA enterprise ID in RTT Viewer..\n");
   wpa_id = process_input_data();
   APP_PRINT("\tThe WPA enterprise ID: %s\n", wpa_id);
   net_params.xEntNetParams.ucIDLength = strlen(wpa_id);
   memcpy(net_params.xEntNetParams.ucID,  wpa_id, net_params.xEntNetParams.ucIDLength);

   /* Input the radius server password of the wanted WPA enterprise AP */
   APP_PRINT("\n>>> Input the WPA enterprise password in RTT Viewer..\n");
   wpa_pw = process_input_data();
   APP_PRINT("\tThe WPA enterprise password: %s\n", wpa_pw);
   net_params.xEntNetParams.ucPasswordLength = strlen(wpa_pw);
   memcpy(net_params.xEntNetParams.ucPassword, wpa_pw, net_params.xEntNetParams.ucPasswordLength);

    /* Connect WPA enterprise AP */
    wifi_err = WIFI_ConnectAPExt(&net_params);
    if (wifi_err)
    {
        APP_PRINT("\n%s: WIFI_ConnectAPExt failed with wifi_err=%d\n", __func__, wifi_err);
    }
#endif
}

void app_task_entry(void *pvParameters)
{
	int ret;
	char *dhcp_server_ip = NULL;

	FSP_PARAMETER_NOT_USED (ret);
    FSP_PARAMETER_NOT_USED (pvParameters);
    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    print_ep_info();
    wifi_init();

    while(notified_value != WIFI_EVENT_CONNECTED)
    {
        xTaskNotifyWait(0, 0xFFFFFFFF, &notified_value, portMAX_DELAY);
    }

    dhcp_server_ip = wpa_ent_connect_info();

#if PING_DUT
    ret = lwip_ping((const char *) dhcp_server_ip, PING_COUNT, PING_TIMEOUT);
#else
	FSP_PARAMETER_NOT_USED (dhcp_server_ip);
	APP_PRINT("PING IP address: %s\n", PING_IP_ADDRESS);
    ret = lwip_ping(PING_IP_ADDRESS, PING_COUNT, PING_TIMEOUT);
#endif
    while (1)
        vTaskDelay (200);
	
    WIFI_Off();
}
