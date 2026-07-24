/***********************************************************************************************************************
* File Name    : usb_console_entry.c
* Description  : USB console thread entry function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "usb_console.h"
#include "usb_console_main.h"


/* USB Console entry function */
/* pvParameters contains TaskHandle_t */
void usb_console_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    usb_console_main();

    /* TODO: add your own code here */
    while (1) {
        vTaskDelay (1);
    }
}
