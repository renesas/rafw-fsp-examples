/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : init lwip, tcp server and clients.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "app_task.h"
#include "loopback.h"
#include "rm_wifi_user_app_gpio_handle.h"
#include "common_utils.h"

#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define TASK_STACK_SIZE 1024
#define EP_APP_VERSION 1.0
#define EP_APP_MODULE_NAME "rm_lwip_w"
#define EP_APP_DESCRIPTION \
    "This example shows tcp loop back echo server client communication\r" \
    "upon successful initialization, Client will receive echo from server"

TaskHandle_t g_taskHandle = NULL;

/* app_task entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);
    g_taskHandle = xTaskGetCurrentTaskHandle();

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

#ifdef RM_MAP_PERSISTANT_W
    /* Initialize and open the peristant storage.
     * Before any Read/Write/Erase open in persistant storage
     * RM_MAP_PERSISTANT_W_Open should be called
     * */
    RM_MAP_PERSISTANT_W_Open(&g_map_persistant_w_ctrl);
#endif //RM_MAP_PERSISTANT_W

    WIFI_On();

#if defined(__SUPPORT_FACTORY_RESET_BTN__)
    /* Create gpio handler event */
    rm_wifi_app_gpio_handle_create_event();

    /* Start GPIO event task */
    rm_wifi_app_gpio_handle_task_start();
#endif //defined(__SUPPORT_FACTORY_RESET_BTN__)

    xTaskCreate(tcp_server_task, "TCP Server Task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xTaskCreate(tcp_client_task, "TCP Client Task1", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    xTaskCreate(tcp_client_task, "TCP Client Task2", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    while (1)
    {
        vTaskDelay (1);
    }

    WIFI_Off();
}
