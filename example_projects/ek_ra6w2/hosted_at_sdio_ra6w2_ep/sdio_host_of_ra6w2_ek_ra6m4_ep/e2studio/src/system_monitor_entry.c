/***********************************************************************************************************************
* File Name    : system_monitor_entry.c
* Description  : System monitor thread entry function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <system_monitor.h>
#include "usb_console.h"
#include "usb_console_main.h"
#include "common_utils.h"

extern bool b_usb_configured;
extern uint8_t LED_BLINK;
extern void PRINTF(char *fmt, ...);

/* System Monitor entry function */
/* pvParameters contains TaskHandle_t */
void system_monitor_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);
    static bool green_light = 0;
#if (USE_UART_PRINTF != 1)
    static bool isFirstShow = 0;
#endif    

    SYSTEM_OK        // LED GREEN ON

#if (USE_UART_PRINTF == 1)
    printf("RA6 to DA16xxx Interface Test\r\n");
#endif

    /* TODO: add your own code here */
    while (1)
    {
#if (USE_UART_PRINTF != 1)
        if (b_usb_configured == true) {
            if (isFirstShow == 0) {
                print_to_console("RA6 to RRQ61xxx Interface Test\r\n");
                PRINTF("VERSION: %s\r\n",EP_VERSION);
                PRINTF("MODULE_NAME: %s\r\n",MODULE_NAME);
#if (RA_SDIO_VER_IS_REL != 1)
                PRINTF("APP Build Time  : %s %s\n", __DATE__, __TIME__);
#endif
                isFirstShow = 1;
            }
        }
#endif

        if (LED_BLINK == true) {
            if (green_light) {
                green_light = 0;
                TURN_GREEN_OFF
            } else {
                green_light = 1;
                TURN_GREEN_ON
            }
        } else {
            if (green_light) {
                green_light = 0;
                TURN_GREEN_OFF
            }
        }

        vTaskDelay (200);
    }
}
