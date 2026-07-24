/***********************************************************************************************************************
* File Name    : common_init.c
* Description  : Common init function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>

#include "common_init.h"
#include "board_cfg.h"

#include "hal_data.h"
#if (SUPPORT_SDIO == 1)
#include "sdio_drv.h"
#endif
#define NUM_SWITCH            (sizeof(irq_pins) / sizeof(irq_pins[0]))

static const struct
{
    const external_irq_instance_t * const p_irq;
} irq_pins[] =
{
        {&g_external_irq0},
        {&g_external_irq10},
        {&g_external_irq9},
};

extern spi_instance_ctrl_t *g_spi_ctrl;

#if (SUPPORT_UART == 1)
EventGroupHandle_t uart7_rx_evt;
StaticEventGroup_t uart7_rx_event_memory;
#endif
/**********************************************************************************************************************
 * Function Name: ICU_Initialize
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
static fsp_err_t ICU_Initialize(void)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    for (uint32_t i = 0; i < NUM_SWITCH; i++) {
        fsp_err = R_ICU_ExternalIrqOpen(irq_pins[i].p_irq->p_ctrl, irq_pins[i].p_irq->p_cfg);
        if (FSP_SUCCESS != fsp_err) {
            return fsp_err;
        }

        fsp_err = R_ICU_ExternalIrqEnable(irq_pins[i].p_irq->p_ctrl);
        if (FSP_SUCCESS != fsp_err) {
            return fsp_err;
        }
    }
    return fsp_err;
}
/**********************************************************************************************************************
 End of function ICU_Initialize
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Function Name: common_init
 * Description  : .
 * Return Value : .
 *********************************************************************************************************************/
fsp_err_t common_init(void)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    fsp_err = ICU_Initialize();
    if (FSP_SUCCESS != fsp_err) {
        return fsp_err;
    }

#if (SUPPORT_UART == 1)
    fsp_err = R_SCI_UART_Open(&g_uart7_ctrl, &g_uart7_cfg);
    if (FSP_SUCCESS != fsp_err) {
        return fsp_err;
    }
    
    uart7_rx_evt = xEventGroupCreateStatic(&uart7_rx_event_memory);
#endif

#if (SUPPORT_SPI == 1)
    /* Open/Initialize SPI Master module */
#if (CHIP_RRQ61x == 1)
    g_spi_ctrl = &g_spi1_ctrl;
    fsp_err = R_SPI_Open(g_spi_ctrl, &g_spi1_cfg);
#else
    g_spi_ctrl = &g_spi0_ctrl;
    fsp_err = R_SPI_Open(g_spi_ctrl, &g_spi0_cfg);
#endif
    //R_SPI0->SPCR3_b.BFDS = 1;
    //R_SPI0->SPCMD_b[0].SSLKP = 1;
    if (FSP_SUCCESS != fsp_err) {
        return fsp_err;
    }
#endif

#if (SUPPORT_SDIO == 1)
    fsp_err = init_drv_sdio();
    if (FSP_SUCCESS != fsp_err) {
        return fsp_err;
    }
#endif

    return fsp_err;
}
/**********************************************************************************************************************
 End of function common_init
 *********************************************************************************************************************/
