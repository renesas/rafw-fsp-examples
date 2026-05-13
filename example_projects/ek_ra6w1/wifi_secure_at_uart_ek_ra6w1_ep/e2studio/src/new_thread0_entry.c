/***********************************************************************************************************************
 * File Name    : new_thread0_entry.c
 * Description  : Entry file for the thread New Thread
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "new_thread0.h"
#if CFG_CLI
#include "rm_cli_w.h"
#include "rm_cli_w_easysetup.h"
#endif
#if CFG_WIFI && defined(__SUPPORT_FACTORY_RESET_BTN__)
#include "rm_wifi_user_app_gpio_handle.h"
#endif
/* New Thread entry function */
/* pvParameters contains TaskHandle_t */
#include "r_cc312_secureboot.h"
#include "bsv_api.h"

#define SECURE_ASSET_ATKEY_OFFSET            (128) 
#define ASSET_ID                             (0x1234)
#define RM_SEC_UART_DEVICE_MANUFACTURE_LCS   (0x1u)

void new_thread0_entry(void *pvParameters)
{
#if ATCMD_SECURE_CHANNEL
    fsp_err_t err;
    uint8_t asset[16];
    uint32_t package_addr = BSP_FEATURE_OSPI_W_DEVICE_0_START_ADDRESS | (SF_ASSET_PROD_BASE_ADDR + SECURE_ASSET_ATKEY_OFFSET); // Example address
    uint8_t package_buffer[64];
    size_t package_buffer_size = sizeof(package_buffer);
    int32_t asset_size;
    uint32_t lcs;
    uint32_t rc  = CC_BsvLcsGet(RRQ61X_ACRYPT_BASE, &lcs);
    /*Test key instead of KCP */
    static const uint8_t test_key_cert[] = {0xd5, 0xe9, 0xda, 0x41, 0xa6, 0x5b, 0x7f, 0xd2, 0xe5, 0xad, 0xf4, 0xb8, 0xf8, 0x43, 0x25, 0x3f};
    static AssetUserKeyData_t userKeyData_cert =
    {
        .pKey    = (uint8_t *) test_key_cert,
        .keySize = 16,
    };
#endif // ATCMD_SECURE_CHANNEL

    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */
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

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();
    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif

    /* Set wakeup button and ISR */
    rm_wifi_app_gpio_wakeup_set(EXT_INTR0_PIN, BSP_WAKEUP_EDGE_LOW, &g_external_irq_wakeup);
#endif
#if (ATCMD_IF_SUPPORT == 1)
    /* Initialize and start the AT command interface */
    atcmd_w_start();
    atcmd_print_initdone_resp();
#endif

#if (HTTPS_W_CFG_SERVER_ENABLE || HTTPS_W_CFG_CLIENT_ENABLE)
    g_https_w.open(&g_https_w0_ctrl, &g_https_w0_cfg);
#endif

#if CFG_CLI
#if defined ( __SUPPORT_APP_CONSOLE_INPUT__ )
    create_easy_setup_task(true);
#endif // __SUPPORT_APP_CONSOLE_INPUT__
#endif // CFG_CLI

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR

#if ATCMD_SECURE_CHANNEL
    memcpy(package_buffer, (void *)package_addr, package_buffer_size);
    if ((rc == 0U) && (lcs >= (uint32_t) RM_SEC_UART_DEVICE_MANUFACTURE_LCS))
    {
        asset_size = R_CC312_Secure_Asset_RuntimeUnpack(ASSET_KCP_KEY,
                                                        NULL,
                                                        ASSET_ID,
                                                        package_buffer,
                                                        package_buffer_size,
                                                        asset);
    }
    else
    {
        asset_size = R_CC312_Secure_Asset_RuntimeUnpack(ASSET_USER_KEY,
                                                        &userKeyData_cert,
                                                        ASSET_ID,
                                                        package_buffer,
                                                        package_buffer_size,
                                                        asset);
    }

    if (asset_size <= 0 || asset_size > (int) sizeof(asset))
    {
          err = FSP_ERR_INVALID_ARGUMENT;
          printf("Unpacking asset failed. Error Code: %u\r\n", err);

    }
    else
    {
        if(FSP_SUCCESS == RM_ATCMD_W_CORE_SecureChannelKeySet(&g_at_core_instance.at_ctrl, asset))
        {
            err = FSP_SUCCESS;
            printf("Secure Channel key Set\r\n");
        }
        else
        {
            err = FSP_ERR_INVALID_ARGUMENT;
            printf("Secure channel key Set failed. Error Code: %u\r\n", err);
        }
    }
#endif //ATCMD_SECURE_CHANNEL

    while (1)
        vTaskDelay(portMAX_DELAY);

    WIFI_Off();
}
