/***********************************************************************************************************************
 * File Name    : app_task_entry.c
 * Description  : Handle wifi connection and twt configurations
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include "app_task.h"
#include "common_utils.h"
#include "r_pm_if.h"
#include "gpt_timer.h"

/* For logging */
#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "rm_pmgr_w"
#define EP_APP_DESCRIPTION \
            "The example project demonstrates the typical use of the PMGR HAL module APIs using PMGR and GPIO modules."
#define LED_ON             BSP_IO_LEVEL_HIGH
#define LED_OFF            BSP_IO_LEVEL_LOW

extern bool volatile g_one_shot_expired;

void set_led(bsp_io_level_t led_state)
{
    R_GPIO_W_PinWrite(&IOPORT_CFG_CTRL, (bsp_io_port_pin_t) BSP_IO_PORT_00_PIN_10, led_state);
}

/* App Task entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    fsp_err_t err;
    int32_t one_shot_timeout = INT32_MAX;

    /*Initialize ONE-SHOT Timer */
    err = init_gpt_timer(&g_timer0_ctrl, &g_timer0_cfg);

    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);
    set_led(LED_ON);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\n** GPT TIMER INIT FAILED **\n");
    }

    APP_PRINT("\nOpened Timer in ONE-SHOT Mode\n");

    /* Start ONE-SHOT Timer */
    err = start_gpt_timer(&g_timer0_ctrl);

    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\n** GPT TIMER START FAILED **\n");

        /*Close ONE-SHOT Timer instance */
        deinit_gpt_timer(&g_timer0_ctrl);
    }

    APP_PRINT("\nStarted Timer in ONE-SHOT Mode for 2 seconds\n");
    R_BSP_WakeupSourcePinSet(BSP_WAKEUP_GPIO_P0_13, BSP_WAKEUP_EDGE_LOW);
    APP_PRINT("\nWake pin set as GPIO P0_13\n");
    err = g_pmgr_w_ins.p_api->open (&g_pmgr_w_ctrl, &g_pmgr_w_cfg);

    assert(FSP_SUCCESS == err);
    APP_PRINT("\nPMGR open done\n");

    /* Add wake source GPIO */
    RM_PMGR_W_set_wake_source (g_pmgr_w_ins.p_ctrl, PMGR_WAKE_SOURCE_GPIO);
    APP_PRINT("\nPMGR wake source set as GPIO\n");
    APP_PRINT("\nBoard will sleep when timer expires\n");

    /* wait for one-shot mode timer to expire. */
    while (true != g_one_shot_expired)
    {
        /* start checking for time out to avoid infinite loop */
        --one_shot_timeout;
        if (RESET_VALUE == one_shot_timeout)
        {
            /* we have reached to a scenario where One-shot event did not occur */
            APP_ERR_PRINT("callback event not received during One-Shot operation\r\n");
            break;
        }
    }

    deinit_gpt_timer (&g_timer0_ctrl);
    set_led (LED_OFF);

#if CFG_PMGR
    /* Remove SLEEP_PROHIBITED constraint */
    g_pmgr_w_ins.p_api->remove_sleep_constraint(g_pmgr_w_ins.p_ctrl, PMGR_CONSTRAINT_SLEEP_PROHIBITED);
#endif //CFG_PMGR
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}
