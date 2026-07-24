/***********************************************************************************************************************
* File Name    : rxcmd_thread_entry.c
* Description  : RxCmd thread entry function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "rxcmd_thread.h"
#include "common_init.h"
#include "sdio_cmd.h"


extern uint8_t exirq_flag;
extern MODE_FLAG mode_flag;
extern uint32_t g_app_at_start;

extern fsp_err_t Received_SPI_Data_From_DA16xxx(void);
extern fsp_err_t Received_SDIO_Data_From_DA16xxx(void);

/* RxCmd Thread entry function */
/* pvParameters contains TaskHandle_t */
void rxcmd_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
    EventBits_t uxBits;

    /* TODO: add your own code here */
    while (1) {
        uxBits = xEventGroupWaitBits(g_update_console_event
            , STATUS_UART7_RX_EVENT | STATUS_INTERRUPT_IRQ0 | STATUS_INTERRUPT_IRQ10 | STATUS_INTERRUPT_IRQ09
            , pdTRUE, pdFALSE, portMAX_DELAY);

#if (CHIP_RRQ61x == 1) // for PMOD2
        if ((uxBits & (STATUS_INTERRUPT_IRQ09)) == (STATUS_INTERRUPT_IRQ09))
#else
        if ((uxBits & (STATUS_INTERRUPT_IRQ0)) == (STATUS_INTERRUPT_IRQ0)) 
#endif
        {
#if (SUPPORT_SPI == 1)
            Received_SPI_Data_From_DA16xxx();
#elif (SUPPORT_SDIO == 1)
            Received_SDIO_Data_From_DA16xxx();
#endif
        }
        if ((uxBits & (STATUS_UART7_RX_EVENT)) == (STATUS_UART7_RX_EVENT)) {
#if (SUPPORT_UART == 1)
            Received_UART_Data_From_DA16xxx();
#endif
        }
        if ((uxBits & (STATUS_INTERRUPT_IRQ10)) == (STATUS_INTERRUPT_IRQ10)) {
            if (g_app_at_start) {
                exirq_flag = 1; // push the button SW1
                mode_flag = RUN_MODE;
            } else {
                extern void wake_up_wifi(void);
                wake_up_wifi();
            }
        }
    }
}

/**********************************************************************************************************************
 * Function Name: button_irq10_callback
 * Description  : SW1 Interrupt handler.
 * Argument     : p_args
 * Return Value : None
 *********************************************************************************************************************/
void button_irq10_callback(external_irq_callback_args_t *p_args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = pdFAIL;
    EventBits_t uxBits;

    FSP_PARAMETER_NOT_USED(p_args);

    uxBits = xEventGroupGetBitsFromISR(g_update_console_event);

    if ( ( uxBits & (STATUS_INTERRUPT_IRQ10)) != (STATUS_INTERRUPT_IRQ10 )) {
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, STATUS_INTERRUPT_IRQ10, &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if ( pdFAIL != xResult) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}


/**********************************************************************************************************************
 * Function Name: external_irq0_callback
 * Description  :  interrupt handler.
 * Argument     : p_args
 * Return Value : None
 *********************************************************************************************************************/
void external_irq0_callback(external_irq_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = pdFAIL;
    EventBits_t uxBits;

    //Event set routine ==> It maks through put lower.
    uxBits = xEventGroupGetBitsFromISR(g_update_console_event);

    if ( ( uxBits & (STATUS_INTERRUPT_IRQ0)) != (STATUS_INTERRUPT_IRQ0)) {
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, STATUS_INTERRUPT_IRQ0, &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if ( pdFAIL != xResult) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}

/**********************************************************************************************************************
 * Function Name: external_irq09_callback
 * Description  :  interrupt handler.
 * Argument     : p_args
 * Return Value : None
 *********************************************************************************************************************/
void external_irq09_callback(external_irq_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = pdFAIL;
    EventBits_t uxBits;

    //Event set routine ==> It maks through put lower.
    uxBits = xEventGroupGetBitsFromISR(g_update_console_event);

    if ( ( uxBits & (STATUS_INTERRUPT_IRQ09)) != (STATUS_INTERRUPT_IRQ09)) {
        xResult = xEventGroupSetBitsFromISR(g_update_console_event, STATUS_INTERRUPT_IRQ09, &xHigherPriorityTaskWoken);

        /* Was the message posted successfully? */
        if ( pdFAIL != xResult) {
            /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
            switch should be requested.  The macro used is port specific and will
            be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
            the documentation page for the port being used. */
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}
