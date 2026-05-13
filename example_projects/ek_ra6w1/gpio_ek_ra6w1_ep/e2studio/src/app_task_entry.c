/***********************************************************************************************************************z
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

/* For logging */
#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "rm_gpio_w"
#define EP_APP_DESCRIPTION \
    "This example project demonstrates how to use FSP APIs to perform simple GPIO read and write operations."

/* App Task entry function */
/* pvParameters contains TaskHandle_t */
void app_task_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* Holds level to set for pins */
    bsp_io_level_t pin_level, prev_pin_level = -1;
    const char *gpio_state[] = {"LOW", "HIGH"};

    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);

    while (1)
    {
        /* read GPIO value */
        R_GPIO_W_PinRead(&g_gpio_w_ctrl, BSP_IO_PORT_01_PIN_14, &pin_level);
        if (prev_pin_level != pin_level)
        {
            APP_PRINT_INFO("\nP1_14 state : %s\n", gpio_state[pin_level]);
            APP_PRINT_INFO("setting P1_15 state : %s\n", gpio_state[pin_level]);

            /* write GPIO value */
            R_GPIO_W_PinWrite(&g_gpio_w_ctrl, BSP_IO_PORT_01_PIN_15, pin_level);
            prev_pin_level = pin_level;
        }

        vTaskDelay (10);
    }
}
