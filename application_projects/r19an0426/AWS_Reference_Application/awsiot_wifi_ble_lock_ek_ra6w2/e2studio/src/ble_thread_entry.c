/***********************************************************************************************************************
 * File Name    : ble_thread_entry.c 
 * Description  : Entry file for BLE Thread.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
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

#if CFG_WIFI && TC_WIFI_ON_DPM
    BaseType_t sem_err = xSemaphoreTake(g_sys_init_semaphore, pdMS_TO_TICKS(1000));
    assert(pdTRUE == sem_err);
#endif /* CFG_WIFI && CFG_PMGR && TC_WIFI_ON_DPM */

    app_main();
    /* TODO: add your own code here */
    while (1)
    {
        vTaskDelay (1);
    }
}
