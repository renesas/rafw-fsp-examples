/***********************************************************************************************************************
 * File Name    : ble_thread_entry.c
 * Description  : BLE thread entry function for the BLE Gateway application.
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include "ble_thread.h"
#include "common_utils.h"

#define BLE_INIT_SEMAPHORE_TIMEOUT_MS    1000
#define BLE_IDLE_DELAY_TICKS             1

extern void app_main(void);
extern void custom_conf_ble(void);

/* pvParameters contains TaskHandle_t */
void ble_thread_entry (void * pvParameters)
{
#if CFG_WIFI && CFG_PMGR
    BaseType_t sem_err;
#endif

    FSP_PARAMETER_NOT_USED(pvParameters);
#if CFG_WIFI && CFG_PMGR
    sem_err = xSemaphoreTake(g_sys_init_semaphore, pdMS_TO_TICKS(BLE_INIT_SEMAPHORE_TIMEOUT_MS));
    assert(pdTRUE == sem_err);
#endif                                 /* CFG_WIFI && CFG_PMGR */
    custom_conf_ble();
    app_main();
    while (1)
    {
        vTaskDelay(BLE_IDLE_DELAY_TICKS);
    }
}
