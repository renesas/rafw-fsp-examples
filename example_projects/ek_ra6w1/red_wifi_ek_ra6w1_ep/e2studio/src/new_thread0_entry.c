/***********************************************************************************************************************

* File Name    : new_thread0_entry.c

* Description  : Red Wi-Fi application.

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#include "new_thread0.h"
#include "lwip/prot/dhcp.h"
#include "rm_atcmd_w_core.h"

#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "rm_map_persistant_w.h"
#include "bsp_common.h"
#include "common/setup_params.h"
#if defined(__SUPPORT_WIFI_USER_GPIO__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
#include "common/ieee802_11_defs.h"
#include dg_configADNVPARAM_PROJ_FILE

#include "common/app_gpio.h"
#include "secure_asset/secure_storage.h"
#include "provisioning/provisioning.h"

#define EASY_SETUP_TASK_EVENT_FINISH     BIT1

#define VEE_ID_WRONG_PASSWD_CNT			(1970)
#define WRONG_PASSWD_DELAY_MS			(30000)
#define WRONG_PASSWD_CNT_MAX			(5)

extern TaskHandle_t cli_handle_task;

EventGroupHandle_t my_app_event;
uint8_t key[32];

static uint32_t wrong_passwd_count;

static void my_ap_sta_discon_handler(WIFIEvent_t *xEvent)
{
    WIFIReason_t disconnect_reason = xEvent->xInfo.xAPStationDisconnected.xReason;
    if (disconnect_reason == WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT)
    {
        PRINTF(GREEN_COLOR"\n### User Call-back : WRONG PW ( reason_code = %d )...\n"CLEAR_COLOR, disconnect_reason);

        wrong_passwd_count++;
        if (WRONG_PASSWD_CNT_MAX < wrong_passwd_count)
        {
            RM_VEE_FLASH_W_RecordWrite(&g_vee0_ctrl, VEE_ID_WRONG_PASSWD_CNT, (uint8_t *) &(uint32_t){1}, 4);
            vTaskDelay(100);
            reset();
        }
    }
}

/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
void new_thread0_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    uint32_t  vee_read_length = 4;
    uint8_t *vee_data_read;
    err_t err;

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
     */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif
#endif

#if SUPPORT_FSP_RM_OTA_W
    g_ota0.p_api->open(g_ota0.p_ctrl, g_ota0.p_cfg);
#endif

#if CFG_CLI
    cli_open();
#endif

    WIFI_On();

#if defined(__SUPPORT_WIFI_USER_GPIO__)
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);
#endif

#if SUPPORT_FSP_RM_OTA_W
    /* RED OTA Task */
    red_ota_task();
#endif

    err = RM_VEE_FLASH_W_RecordPtrGet(&g_vee0_ctrl, VEE_ID_WRONG_PASSWD_CNT, &vee_data_read, &vee_read_length);

    PRINTF(GREEN_COLOR"\n### RM_VEE_FLASH_W_RecordPtrGet ( err = %d , count = %d)...\n"CLEAR_COLOR, err, *vee_data_read);

    if ((0 != *vee_data_read) && (FSP_ERR_NOT_FOUND != err))
    {
        vTaskDelay(WRONG_PASSWD_DELAY_MS);

        RM_VEE_FLASH_W_RecordWrite(&g_vee0_ctrl, VEE_ID_WRONG_PASSWD_CNT, (uint8_t *) &(uint32_t){0}, 4);
    }

#if (ATCMD_IF_SUPPORT == 1)
    add_red_atcmd();
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif

    if (FSP_SUCCESS == R_RED_SecureAssetLoad(SECURE_ASSET_AT_KEY, (uint8_t *) key, sizeof(key_info_t)))
    {
        RM_ATCMD_W_CORE_SecureChannelKeySet(&g_at_core_instance.at_ctrl, key);
    }

    provision_app_gpio_handle_create_event();
    provision_app_gpio_handle_task_start();

    WIFIDeviceMode_t mode = eWiFiModeStation;
    WIFI_GetMode(&mode);
    if (eWiFiModeAP == mode || eWiFiModeAPStation == mode)
    {
        execute_AP_profile();
        provisioning_Open();
#if (ATCMD_IF_SUPPORT == 1)
        /* This code is instead of RM_AWSIOT_W_Open(&g_awsiot_w0_ctrl, &g_awsiot_w0_cfg); */
        rm_awsiot_w_app_instance_ctrl_t *p_instance_ctrl = (rm_awsiot_w_app_instance_ctrl_t *) &g_awsiot_w0_ctrl;
        gp_awsiot_w_app_instance = p_instance_ctrl;
        p_instance_ctrl->p_cfg   = &g_awsiot_w0_cfg;
#endif
    }
    else
    {
        if (!RM_PMGR_W_dpm_is_wakeup())
        {
            execute_WIFI_profile();
        }
        /* This code is instead of RM_AWSIOT_W_Open(&g_awsiot_w0_ctrl, &g_awsiot_w0_cfg); */
        rm_awsiot_w_app_instance_ctrl_t *p_instance_ctrl = (rm_awsiot_w_app_instance_ctrl_t *) &g_awsiot_w0_ctrl;
        gp_awsiot_w_app_instance = p_instance_ctrl;
        p_instance_ctrl->p_cfg   = &g_awsiot_w0_cfg;
    }

    WIFI_RegisterEvent(eWiFiEventAPStationDisconnected, my_ap_sta_discon_handler);

    xTaskNotify(cli_handle_task, EASY_SETUP_TASK_EVENT_FINISH, eSetBits);
    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
