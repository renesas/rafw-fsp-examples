/***********************************************************************************************************************
 * File Name    : ble_thread_entry.c
 * Description  : Contains ble_thread_entry function.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "ble_thread.h"
#include "common_utils.h"

void app_main(void);

/* BLE Thread entry function */
/* pvParameters contains TaskHandle_t */
void ble_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    print_ep_info_banner(MODULE_NAME, EP_VERSION, EP_INFO);

    /* TODO: add your own code here */

    /* Initialize UART module for logging in autotest mode*/
    custom_conf_ble();
    app_main();

    while (1)
    {
        vTaskDelay(1);
    }
}
