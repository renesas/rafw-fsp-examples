/***********************************************************************************************************************
* File Name    : ble_thread_entry.c
* Description  : BLE thread entry function for the Azure IoT Wi-Fi BLE application.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "ble_thread.h"
/* BLE Thread entry function */

extern void app_main(void);

/* pvParameters contains TaskHandle_t */
void ble_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

#if CFG_WIFI && CFG_PMGR
    BaseType_t sem_err = xSemaphoreTake(g_sys_init_semaphore, pdMS_TO_TICKS(1000));
    assert(pdTRUE == sem_err);
#endif /* CFG_WIFI && CFG_PMGR */

    app_main();
    /* TODO: add your own code here */
    while (1)
    {
        vTaskDelay (1);
    }
}
