/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : App main and Wi-Fi initialization.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "app_task.h"
#include "common_utils.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "lwip/netif.h"
#include "config.h"
#include "ota.h"
#include "common_data.h"
#include "ota_update.h"
#include "ota_update_common.h"

uint32_t notified_value = EVENT_VAL;
TaskHandle_t g_app_main_task_handle = NULL;
extern UINT app_http_ota_renew(void);

uint32_t g_http_req_fail = 0;
uint32_t g_header_done   = 0;
uint32_t g_length_err    = 0;

/* Forward declarations to silence -Wmissing-declarations */
void netif_status_callback(struct netif * netif);
void set_sys_time(void);
void print_ep_info(void);
void wifi_init(void);

/***********************************************************************************************************************
 * WIFI NETWORK PARAMS
 **********************************************************************************************************************/
WIFINetworkParams_t net_params =
{
    .ucChannel                  = CHANNEL,
    .xPassword.xWPA.cPassphrase = PASSPHRASE,
    .ucSSID                     = SSID,
    .xPassword.xWPA.ucLength    = strlen(PASSPHRASE),
    .ucSSIDLength               = strlen(SSID),
    .xSecurity                  = WIFI_SECURITY,
};

/***********************************************************************************************************************
 * Function Name: netif_status_callback
 ***********************************************************************************************************************/
void netif_status_callback(struct netif * netif)
{
    if (netif_is_up(netif) && !ip_addr_isany_val(netif->ip_addr))
    {
        xTaskNotifyGive(g_app_main_task_handle);
    }
}

/***********************************************************************************************************************
 * Function Name: set_sys_time
 ***********************************************************************************************************************/
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

/***********************************************************************************************************************
 * Function Name: print_ep_info
 ***********************************************************************************************************************/
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

/***********************************************************************************************************************
 * Function Name: wifi_init
 ***********************************************************************************************************************/
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

/***********************************************************************************************************************
 * Function Name: app_task_entry
 ***********************************************************************************************************************/
void app_task_entry(void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    int itr = 0;

    g_app_main_task_handle = xTaskGetCurrentTaskHandle();

    WIFI_SetMode(eWiFiModeStation);

    print_ep_info();

    /*******************************************************************
     * BOOT SLOT INFO
     ******************************************************************/
    {
        UINT boot_idx = ota_update_get_boot_index();

        uint32_t slot0_reset_vec = *((volatile uint32_t *)0x00000004U);
        uint32_t slot1_reset_vec = *((volatile uint32_t *)0x00400004U);

        APP_PRINT("\r\n========================================\r\n");
        APP_PRINT(" BOOT SLOT : %d  (%s)\r\n",
                  boot_idx,
                  (boot_idx == 1) ? "OTA UPDATED FIRMWARE" : "ORIGINAL FIRMWARE");
        APP_PRINT(" Slot 0 reset vector = 0x%08X\r\n", slot0_reset_vec);
        APP_PRINT(" Slot 1 reset vector = 0x%08X\r\n", slot1_reset_vec);

        if (slot0_reset_vec == slot1_reset_vec)
        {
            APP_PRINT(" [WARN] Both slots have IDENTICAL reset vectors!\r\n");
            APP_PRINT(" [WARN] OTA image may be same as current firmware.\r\n");
        }
        else
        {
            APP_PRINT(" [OK] Slot vectors differ - images are different.\r\n");
        }

        APP_PRINT("========================================\r\n");
    }

    /*******************************************************************
     * WIFI INIT
     ******************************************************************/
    wifi_init();

    RM_WIFI_mbedtls_setup_psa_crypto();

#if CFG_PMGR && TC_WIFI_ON_DPM

    fsp_err_t err;

    err = g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);

    assert(FSP_SUCCESS == err);

    RM_PMGR_W_dpm_job_name_set("pmgrmon", 0);

    RM_PMGR_W_set_wake_source(g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_WIFI);

    g_pmgr_w_ins.p_api->add_sleep_constraint(
        g_pmgr_w_ins.p_ctrl,
        PMGR_CONSTRAINT_POWER_RETENTION);

#endif

#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif

    set_sys_time();

    netif_set_status_callback(netif_default, netif_status_callback);

    /*******************************************************************
     * WIFI CONNECT
     ******************************************************************/
    APP_PRINT("\r\n========================================\r\n");
    APP_PRINT(" WIFI CONNECTION START\r\n");
    APP_PRINT("========================================\r\n");

    WIFI_ConnectAP(&net_params);

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    APP_PRINT("[APP] WiFi Connected\r\n");

    /*******************************************************************
     * OTA START
     ******************************************************************/
    {
        UINT active_slot = ota_update_get_boot_index();

        APP_PRINT("\r\n========================================\r\n");
        APP_PRINT(" WIFI HTTP OTA START\r\n");
        APP_PRINT(" Active Boot Slot : %d\r\n", active_slot);
        APP_PRINT("========================================\r\n");
    }

    if (FSP_SUCCESS != ota_update_start())
    {
        APP_PRINT("[APP] OTA start failed\r\n");
        WIFI_Off();
        vTaskDelete(NULL);
    }

    APP_PRINT("[APP] OTA request started\r\n");

    /*******************************************************************
     * MAIN LOOP
     ******************************************************************/
    while (1)
    {
        if (g_http_req_fail == 1)
        {
            APP_PRINT("[APP] HTTP request failed\r\n");
            break;
        }

        xTaskNotifyWait(0x00,
                        0xFFFFFFFF,
                        &notified_value,
                        portMAX_DELAY);

        switch (notified_value)
        {
            case HTTPS_EVENT_CLIENT_GET_DONE:
            {
                APP_PRINT("[APP] HTTP Header Received\r\n");
                g_header_done = 1;
                break;
            }

            case TASK_COMPLETE_EVENT:
            {
                UINT ota_status;

                APP_PRINT("\r\n========================================\r\n");
                APP_PRINT(" OTA PROCESS COMPLETE\r\n");
                APP_PRINT("========================================\r\n");

                ota_status = app_http_ota_renew();

                APP_PRINT("[APP] OTA RENEW STATUS = 0x%02X\r\n", ota_status);

                if (ota_status == OTA_SUCCESS)
                {
                    APP_PRINT("[APP] OTA RENEW SUCCESS\r\n");
                }
                else
                {
                    APP_PRINT("[APP] OTA RENEW FAILED\r\n");
                }

                break;
            }

            case HTTPS_EVENT_CLIENT_RESULT:
            {
                if (g_header_done)
                {
                    g_header_done = 0;
                    APP_PRINT("[APP] HTTP transfer complete\r\n");
                }
                else
                {
                    if (itr < LOOP_ITR_MAX)
                    {
                        itr++;
                        APP_PRINT("[APP] HTTP retry %d\r\n", itr);
                        http_restart();
                    }
                    else
                    {
                        APP_PRINT("[APP] HTTP retries exceeded\r\n");
                        g_http_req_fail = 1;
                    }
                }
                break;
            }

            case LENGTH_ERROR:
            {
                APP_PRINT("[APP] OTA image size error\r\n");
                break;
            }

            case WIFI_EVENT_CONNECTED:
            {
                APP_PRINT("[APP] WiFi Connected Event\r\n");
                break;
            }

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    WIFI_Off();
    vTaskDelete(NULL);
}
