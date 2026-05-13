/***********************************************************************************************************************

* File Name    : app_gpio.c

* Description  : Handle pin configuration and interrupt configuration

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "r_ext_irq_w.h"
#include "r_gpio_w.h"
#include "rm_wifi.h"
#include "event_groups.h"
#include "r_ext_irq_w.h"
#include "setup_params.h"
#include "app_gpio.h"

/***********************************************************************************************************************
 * Macros
 **********************************************************************************************************************/
#define APP_GPIO_CHECK_STEP_FACTORY_BTN_MS     (100)          /* 100ms */

#define PROV_APP_GPIO_EV_TASK_NAME             ("prov_GPIO_ev")
#define PROV_APP_GPIO_EV_TASK_STACK_SIZE       ((1024 * 4) / sizeof(unsigned long))

#define PROV_APP_GPIO_EVENT_BTN_FR             (0x02)
#define PROV_APP_GPIO_EVENT_WAIT_TICK          (100)

#define BTN_PROV                               (BSP_IO_PORT_00_PIN_10)
#define BTN_PROV_PORT                          (BTN_PROV >> BSP_IO_PORT_OFFSET)
#define BTN_PROV_PIN                           (BTN_PROV & BSP_IO_PIN_BITS)
#define BTN_PROV_MODE                          (IOPORT_CFG_PULLUP_ENABLE)
#define BTN_PROV_FUNC                          (IOPORT_PERIPHERAL_GPIO)
#define BTN_PROV_INT_POL                       (BSP_IO_LEVEL_HIGH)
#define BTN_PROV_INT_SEL                       (HW_GPIO_INT_EDGE)
#define BTN_PROV_CHK_TIME                      5  /* 5 sec. */

#define APP_GPIO_BTN_WPS_ACTIVE_STATE          (!BTN_WPS_INT_POL)
#define APP_GPIO_BTN_FR_ACTIVE_STATE           (!BTN_FR_INT_POL)

#define APP_GPIO_FACTORY_BUTTON_FALSE          (0)
#define APP_GPIO_FACTORY_BUTTON_FACTORY_RESET  (1)
#define APP_GPIO_FACTORY_BUTTON_REBOOT         (2)

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
static EventGroupHandle_t g_gpio_evn_grp_gpio = NULL;
static TaskHandle_t g_gpio_evn_task = NULL;
static int (*gp_button1_one_touch_cb)(void) = NULL;

/***********************************************************************************************************************
 * External variables
 **********************************************************************************************************************/
extern const ioport_instance_t g_gpio_w;
extern const wifi_cfg_t g_wifi_cfg;
extern ext_irq_w_instance_ctrl_t g_external_irq1_ctrl;
extern const external_irq_instance_t g_external_irq1;

/***********************************************************************************************************************
 * Private functions prototypes
 **********************************************************************************************************************/
static void provision_app_gpio_factory_reset_default (int reboot_flag);
static void provision_app_gpio_event_task (void *param);
static void provisioning_gpio_set_interrupt (void);
static unsigned int app_gpio_check_factory_button (int btn_gpio_port, int btn_gpio_num, int check_time);

void rm_wifi_app_gpio_p0_fr_handler(void *param);

/***********************************************************************************************************************
 * Public Functions
 **********************************************************************************************************************/
int provision_app_gpio_handle_create_event(void)
{
    g_external_irq1.p_api->open(&g_external_irq1_ctrl, g_external_irq1.p_cfg);
    provisioning_gpio_set_interrupt();

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

void provision_app_gpio_handle_task_start(void)
{
    /* Create GPIO event handling task. */
    (void)xTaskCreate(provision_app_gpio_event_task,
                         PROV_APP_GPIO_EV_TASK_NAME,
                         PROV_APP_GPIO_EV_TASK_STACK_SIZE,
                         (void *) NULL,
                         tskIDLE_PRIORITY + 2,    /* Don't assign as 0 priority */
                         &g_gpio_evn_task);
}

static void provision_app_gpio_factory_reset_default(int reboot_flag)
{
    printf("\nFactory Reseting...\n");

    execute_factory_default();

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

    EventBits_t gpio_ev_bits;
    EventBits_t target_ev_bits;

    unsigned int status;

    target_ev_bits = PROV_APP_GPIO_EVENT_BTN_FR;

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
            const ext_irq_w_extended_cfg_t *p_irq1_ext_cfg = (const ext_irq_w_extended_cfg_t *) g_external_irq1.p_cfg->p_extend;
            bsp_io_port_pin_t irq1_pin = p_irq1_ext_cfg->irq_pin;
            bsp_io_port_t irq1_port = irq1_pin >> BSP_IO_PORT_OFFSET;

 #if WIFI_CFG_WATCHDOG_SERVICE_ENABLE
            g_wifi_cfg.p_watchdog_service->p_api->notify(g_wifi_cfg.p_watchdog_service->p_ctrl,
                                                         g_wifi_cfg.p_watchdog_service->p_cfg, sys_wdog_id);
            g_wifi_cfg.p_watchdog_service->p_api->suspend(g_wifi_cfg.p_watchdog_service->p_ctrl, sys_wdog_id);
 #endif
            status = app_gpio_check_factory_button(irq1_port, (irq1_pin & BSP_IO_PIN_BITS), BTN_FR_CHK_TIME);

            if (APP_GPIO_FACTORY_BUTTON_FACTORY_RESET == status)
            {
                provision_app_gpio_factory_reset_default(pdTRUE);
            }
            else if (APP_GPIO_FACTORY_BUTTON_REBOOT == status) /* Reboot */
            {
                reset();
            }

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

static void provisioning_gpio_set_interrupt(void)
{
    if (0U == g_external_irq1_ctrl.open)
    {
        const ext_irq_w_extended_cfg_t *p_irq1_ext_cfg = (const ext_irq_w_extended_cfg_t *) g_external_irq1.p_cfg->p_extend;
        bsp_io_port_pin_t irq1_pin = p_irq1_ext_cfg->irq_pin;
        bsp_io_port_t irq1_port = irq1_pin >> BSP_IO_PORT_OFFSET;

        g_gpio_w.p_api->pinCfg(g_gpio_w.p_ctrl, ((irq1_port << BSP_IO_PORT_OFFSET) | (irq1_pin & BSP_IO_PIN_BITS)),
                               (uint32_t) (BTN_FR_MODE | IOPORT_CFG_IRQ_ENABLE));
    }

    if (0U == BTN_PROV_PORT)
    {
        g_external_irq1.p_api->callbackSet(&g_external_irq1_ctrl, (void *) g_external_irq1.p_cfg->p_callback, NULL, NULL);
    }

    g_external_irq1.p_api->enable(&g_external_irq1_ctrl);
}

void rm_wifi_app_gpio_p0_fr_handler(void *param)
{
    /* This callback is in interrupt. */
    BaseType_t xResult;
    FSP_PARAMETER_NOT_USED(param);
    FSP_PARAMETER_NOT_USED(g_gpio_evn_task);

    if (g_gpio_evn_grp_gpio)
    {
        /* Set event to GPIO_event task. : callback function for GPIO interrupt */
        xResult = xEventGroupSetBitsFromISR(g_gpio_evn_grp_gpio,
                                            PROV_APP_GPIO_EVENT_BTN_FR,
                                            pdFALSE);    /* xHigherPriorityTaskWoken */

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

static unsigned int app_gpio_check_factory_button(int btn_gpio_port, int btn_gpio_num, int check_time)
{
    unsigned int result = APP_GPIO_FACTORY_BUTTON_FALSE;
    int pin_status = 0;
    int check_time_cnt = 0;
    int first_loop = 0;

    /* STEP #1 : Check button pushed. */
    do
    {
        g_gpio_w.p_api->pinRead(g_gpio_w.p_ctrl, ((btn_gpio_port << BSP_IO_PORT_OFFSET) | btn_gpio_num),
                                (bsp_io_level_t *) &pin_status);

        if (APP_GPIO_BTN_FR_ACTIVE_STATE == pin_status)
        {
            /* Button is in 'pressed' state. */
            check_time_cnt++;
        }
        else
        {
            /* Button is in 'released' state. */
            if ((check_time_cnt > 10) && (check_time_cnt < (10 * check_time)))
            {
                /* 1 ~ reset_time Sec. */
                result = APP_GPIO_FACTORY_BUTTON_REBOOT; /* Reboot */
                goto end;
            }
            else if ((check_time_cnt > 0) && (check_time_cnt < 10))
            {
                /* In case of one-touch button push ... */

                /* Registered in system_start.c by application operation. */
                if (NULL != gp_button1_one_touch_cb)
                {
                    if (gp_button1_one_touch_cb() == pdTRUE)
                    {
                        /* Reboot to change runinng mode. */
                        reset();
                    }
                }
            }

            result = APP_GPIO_FACTORY_BUTTON_FALSE; /* false */
            goto end;
        }

        vTaskDelay(portCONVERT_MS_2_TICKS(APP_GPIO_CHECK_STEP_FACTORY_BTN_MS));

        if (0 == first_loop)
        {
            printf("\33[2K" "Factory reset after %d seconds\n",
                   check_time - (check_time_cnt / (APP_GPIO_CHECK_STEP_FACTORY_BTN_MS/10)));
        }
        else
        {
            if ((check_time_cnt % 10) == 0)
            {
                printf("%d", check_time - (check_time_cnt / (APP_GPIO_CHECK_STEP_FACTORY_BTN_MS/10)));
            }
            else
            {
                printf(".");
            }
        }

        first_loop++;
    } while (check_time_cnt < ((APP_GPIO_CHECK_STEP_FACTORY_BTN_MS/10) * check_time));

    printf("\33[2K" "\nReady to Factory Reset.\n");

    /* STEP #2 : Check button released. */
    do
    {
        g_gpio_w.p_api->pinRead(g_gpio_w.p_ctrl, ((btn_gpio_port << BSP_IO_PORT_OFFSET) | btn_gpio_num),
                                (bsp_io_level_t *) &pin_status);

        if (pin_status != APP_GPIO_BTN_WPS_ACTIVE_STATE)
        {
            printf("\33[2K"  "Start Factory Reset.\n");
            result = APP_GPIO_FACTORY_BUTTON_FACTORY_RESET;
            break;
        }
        else
        {
            /* Wait until button is released. */
#if defined(__SUPPORT_EVK_LED__)
            /* Factory Status LED Blink */
            srm_wifi_app_gpio_set_led_state(led_gpio_port, led_gpio_num, WIFI_APP_GPIO_LED_STATE_ON);
            vTaskDelay(portCONVERT_MS_2_TICKS(WIFI_APP_GPIO_CHECK_STEP_FACTORY_BTN_MS)); /* 100ms */
            srm_wifi_app_gpio_set_led_state(led_gpio_port, led_gpio_num, WIFI_APP_GPIO_LED_STATE_OFF);
            vTaskDelay(portCONVERT_MS_2_TICKS(WIFI_APP_GPIO_CHECK_STEP_FACTORY_BTN_MS)); /* 100ms */
#endif
        }
    } while (1);

end:
    return result;
}
