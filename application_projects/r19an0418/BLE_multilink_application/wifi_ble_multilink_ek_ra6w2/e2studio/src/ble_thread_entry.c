/***********************************************************************************************************************
 * File Name    : ble_thread_entry.c
 * Description  : Contains ble_thread_entry function.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2024 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "ble_thread.h"
#include "common_utils.h"

#define EP_APP_VERSION        1.0
#define EP_APP_MODULE_NAME    "rm_ble_abs_gtl_b"
#define EP_APP_DESCRIPTION \
    "This application demonstrates Multi-link over BLE.\r"

extern void app_main(void);

/* BLE Thread entry function */
/* pvParameters contains TaskHandle_t */
void ble_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    print_ep_info_banner(EP_APP_MODULE_NAME, _STRINGFY(EP_APP_VERSION), EP_APP_DESCRIPTION);
#if CFG_WIFI && TC_WIFI_ON_DPM
    BaseType_t sem_err = xSemaphoreTake(g_sys_init_semaphore, pdMS_TO_TICKS(1000));
    assert(pdTRUE == sem_err);
#endif                                 // CFG_WIFI && CFG_PMGR && TC_WIFI_ON_DPM

    /* TODO: add your own code here */
    cust_conf_ble();
    app_main();

    while (1)
    {
        vTaskDelay(1);
    }
}
