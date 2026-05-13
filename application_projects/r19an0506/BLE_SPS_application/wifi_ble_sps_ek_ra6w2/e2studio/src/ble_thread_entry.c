/***********************************************************************************************************************
 * File Name    : ble_thread_entry.c
 * Description  : Contains ble_thread_entry function
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "ble_thread.h"
#include "common_utils.h"

/* BLE Thread entry function */
extern void app_main(void);
extern void custom_conf_ble(void);

/* pvParameters contains TaskHandle_t */
void ble_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    print_ep_info_banner(MODULE_NAME, EP_VERSION, EP_INFO);

#if CFG_WIFI && CFG_PMGR && TC_WIFI_ON_DPM
    BaseType_t sem_err = xSemaphoreTake(g_sys_init_semaphore, pdMS_TO_TICKS(1000));
    assert(pdTRUE == sem_err);
#endif                                 /* CFG_WIFI && CFG_PMGR && TC_WIFI_ON_DPM */

    /* TODO: add your own code here */

    custom_conf_ble();
    app_main();

    while (1)
    {
        vTaskDelay(1);
    }
}
