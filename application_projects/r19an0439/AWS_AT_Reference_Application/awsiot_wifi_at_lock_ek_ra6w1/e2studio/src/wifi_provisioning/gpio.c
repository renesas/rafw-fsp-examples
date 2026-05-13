/***********************************************************************************************************************
* File Name    : gpio.c
* Description  : Handle pin configuration and intrupt configuration
**********************************************************************************************************************/

/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/***********************************************************************************************************************
 * External variables
 **********************************************************************************************************************/
#include "r_ext_irq_w.h"
#include "r_gpio_w.h"
#include "rm_wifi.h"
#include "event_groups.h"
#include "r_ext_irq_w.h"
#include "provisioning.h"
#include "rm_wifi_helper.h"
#include "common_data.h"

#define PROV_APP_GPIO_EV_TASK_NAME                  ("prov_GPIO_ev")
#define PROV_APP_GPIO_EV_TASK_STACK_SIZE            ((1024 * 4) / sizeof(unsigned long))

#define PROV_APP_GPIO_EVENT_BTN_FR                  (0x02)
#define PROV_APP_GPIO_EVENT_WAIT_TICK               (100)
/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
static EventGroupHandle_t g_gpio_evn_grp_gpio = NULL;
static TaskHandle_t g_gpio_evn_task = NULL;
/***********************************************************************************************************************
 * External variables
 **********************************************************************************************************************/
extern const ioport_instance_t g_ioport;
extern const wifi_cfg_t g_wifi_cfg;
extern ext_irq_w_instance_ctrl_t g_external_irq3_ctrl;
extern const external_irq_instance_t g_external_irq3;

static void provision_app_gpio_factory_reset_default(int reboot_flag);
static void provision_app_gpio_event_task(void *param);

int provision_app_gpio_handle_create_event (void)
{
    if (!g_gpio_evn_grp_gpio)
    {
        /* Create sync-up event. */
        g_gpio_evn_grp_gpio = xEventGroupCreate();
        if (NULL == g_gpio_evn_grp_gpio)
        {
            printf("\n\n>>> Failed to create GPIO event group !!!\n\n");

            return pdFALSE;
        }
    }

    return pdTRUE;
}

void provision_app_gpio_handle_task_start (void)
{
    /* Create GPIO event handling task. */
    xTaskCreate(provision_app_gpio_event_task,
                         PROV_APP_GPIO_EV_TASK_NAME,
                         PROV_APP_GPIO_EV_TASK_STACK_SIZE,
                         (void *) NULL,
                         tskIDLE_PRIORITY + 2,    // Don't assign as 0 priority
                         &g_gpio_evn_task);
}

static void provision_app_gpio_factory_reset_default (int reboot_flag)
{
    printf("\nFactory Reseting...\n");
    provisioning_reboot_ap_mode(WIFI_DEVICE_MODE_EXT_AP, 1, 1);
    vTaskDelay(portCONVERT_MS_2_TICKS(100));

    if (pdTRUE == reboot_flag)
    {
        reset();
        /* Wait for system-reboot. */
        while (1)
        {
            vTaskDelay(portCONVERT_MS_2_TICKS(100));
        }
    }
}

static void provision_app_gpio_event_task(void *param)
{
    FSP_PARAMETER_NOT_USED(param);

    EventBits_t gpio_ev_bits   = 0;
    EventBits_t target_ev_bits = 0;

    target_ev_bits = target_ev_bits | PROV_APP_GPIO_EVENT_BTN_FR;

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    uint8_t sys_wdog_id = WATCHDOG_SERVICE_W_NOT_REGISTERED_ID;
    g_wifi_cfg.p_watchdog_service->p_api->registerTask(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                       g_wifi_cfg.p_watchdog_service->p_cfg, &sys_wdog_id);
#endif

    while (pdTRUE)
    {
#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
        g_wifi_cfg.p_watchdog_service->p_api->notify(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                     g_wifi_cfg.p_watchdog_service->p_cfg, sys_wdog_id);
        g_wifi_cfg.p_watchdog_service->p_api->suspend(g_wifi_cfg.p_watchdog_service->p_ctrl, sys_wdog_id);
#endif

        gpio_ev_bits = xEventGroupWaitBits(g_gpio_evn_grp_gpio,
                                           target_ev_bits,
                                           pdTRUE,
                                           pdFALSE,
                                           portCONVERT_MS_2_TICKS(PROV_APP_GPIO_EVENT_WAIT_TICK));

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
        g_wifi_cfg.p_watchdog_service->p_api->resumeAndNotify(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                              g_wifi_cfg.p_watchdog_service->p_cfg,
                                                              sys_wdog_id);
#endif
        if (gpio_ev_bits & PROV_APP_GPIO_EVENT_BTN_FR)
        {
            printf("factory reset...\n");
            xEventGroupClearBits(g_gpio_evn_grp_gpio, PROV_APP_GPIO_EVENT_BTN_FR);
        }
        else
        {
            continue;
        }

        if (gpio_ev_bits & PROV_APP_GPIO_EVENT_BTN_FR) /* Factory Reset BTN */
        {
 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
            g_wifi_cfg.p_watchdog_service->p_api->notify(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                         g_wifi_cfg.p_watchdog_service->p_cfg, sys_wdog_id);
            g_wifi_cfg.p_watchdog_service->p_api->suspend(g_wifi_cfg.p_watchdog_service->p_ctrl, sys_wdog_id);
 #endif
            provision_app_gpio_factory_reset_default(pdTRUE);

 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
            g_wifi_cfg.p_watchdog_service->p_api->resumeAndNotify(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                                  g_wifi_cfg.p_watchdog_service->p_cfg,
                                                                  sys_wdog_id);
 #endif
        }
    }

#if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
    g_wifi_cfg.p_watchdog_service->p_api->unregisterTask(g_wifi_cfg.p_watchdog_service->p_ctrl, sys_wdog_id);
#endif

    vTaskDelete(NULL);
}

void provisioning_gpio_fr_handler(external_irq_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    /* This callback is in interrupt. */
    BaseType_t xResult;

    if (g_gpio_evn_grp_gpio)
    {
        /* Set event to GPIO_event task. : callback function for GPIO interrupt */
        xResult = xEventGroupSetBitsFromISR(g_gpio_evn_grp_gpio,
                                            PROV_APP_GPIO_EVENT_BTN_FR,
                                            pdFALSE);    // xHigherPriorityTaskWoken

        if (pdFAIL != xResult)
        {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
             * switch should be requested. The macro used is port specific and will
             * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
             * the documentation page for the port being used. */
            portYIELD_FROM_ISR(pdFALSE);
        }
    }
}
