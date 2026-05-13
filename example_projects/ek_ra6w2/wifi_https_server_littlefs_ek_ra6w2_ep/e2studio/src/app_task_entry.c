/***********************************************************************************************************************
* File Name    : app_task_entry.c
* Description  : app main and wifi init
**********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "app_task.h"
#include "http_svr.h"
#include "common_utils.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "lwip/netif.h"
#include "ip4_addr.h"

#define EP_APP_VERSION 1.0
#define EP_APP_MODULE_NAME "rm_https_w"
#define EP_APP_DESCRIPTION \
    "This example demonstrates a secure HTTPS server over Wi-Fi with littlefs ,\r"\
	"accessing a webpage through an HTTP server and upload files to filesystem through web gui"\
    "Clients can connect to the device using TLS 1.3 for encrypted communication."

TaskHandle_t xAppTaskHandle;
WIFINetworkParams_t net_params =
{
    .ucChannel = CHANNEL,
    .xPassword.xWPA.cPassphrase = PASSPHRASE,
    .ucSSID = SSID,
    .xPassword.xWPA.ucLength = PASSS_LEN,
    .ucSSIDLength = SSID_LEN,
    .xSecurity = eWiFiSecurityWPA2,
};

static void netif_status_callback(struct netif *netif)
{
    (void)netif;
    if (netif_is_up(netif_default) && !ip_addr_isany(&netif_default->ip_addr))
        xTaskNotify(xAppTaskHandle, 0, eSetValueWithOverwrite);
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    xAppTaskHandle = xTaskGetCurrentTaskHandle();
    fsp_err_t err;
    char *p_buf = "<html><body><h2>Welcome</h2><p>Select a file to upload and store into LittleFS:</p><form action=\"/post\" method=\"post\" \
                   enctype=\"multipart/form-data\"><input type=\"file\" name=\"datafile\"><br><br><input type=\"submit\" value=\"Upload\"></form></body></html>";
    char *p_path = "/upload.html";

    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);

    /* TODO: add your own code here */
#if CFG_WIFI
    WIFIReturnCode_t wifi_err = WIFI_SetMode(eWiFiModeStation);

    if (wifi_err != eWiFiSuccess)
    {
        APP_PRINT_INFO("Failed to set Wi-Fi mode, error: %d\n", wifi_err);
    }

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
    RM_PMGR_W_dpm_job_name_set("pmgrmon", 0);

    /* add wake source */
    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    /* add constraint (RETENTION) */
    g_pmgr_w_ins.p_api->add_sleep_constraint(g_pmgr_w_ins.p_ctrl,
                                             PMGR_CONSTRAINT_POWER_RETENTION);
#endif //TC_WIFI_ON_DPM
#endif //CFG_PMGR

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif //SUPPORT_FSP_RM_OTA_W

#ifdef RM_MAP_PERSISTANT_W
    /*
     * Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

    /* wifi subsystem on and connect to AP with some pre configured params */
    WIFI_On();
    netif_set_status_callback(netif_default, netif_status_callback);
    WIFIReturnCode_t wifi_error = WIFI_ConnectAP(&net_params);
    (void)wifi_error;

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

#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif //(HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)

#if CFG_PMGR
#if TC_WIFI_ON_DPM
#if (ATCMD_DA14XXX_CODELESS == 1) && (ATCMD_PMGR_SUPPORT_ENABLE == 1)
    BaseType_t sem_err = xSemaphoreTake(g_atcmd_init_semaphore, pdMS_TO_TICKS(5000));
    assert(pdTRUE == sem_err);
#endif //(ATCMD_DA14XXX_CODELESS == 1) && (ATCMD_PMGR_SUPPORT_ENABLE == 1)
    /* remove constraint (RAM) */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_POWER_RAM);
#endif //TC_WIFI_ON_DPM
#endif //CFG_PMGR
#endif //CFG_WIFI

    /* Wait until the network interface is up and a valid IP address is assigned */
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    APP_PRINT_INFO("IP4 Address : %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    err = init_server();
    format_fs();
    write_file(p_path, p_buf);

    if (err != FSP_SUCCESS)
    {
        vTaskDelete(NULL);
    }

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }

    deinit_server();
    WIFI_Off();
}
