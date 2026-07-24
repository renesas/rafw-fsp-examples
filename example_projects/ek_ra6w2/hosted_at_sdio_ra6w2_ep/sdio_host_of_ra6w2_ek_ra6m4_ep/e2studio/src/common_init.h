/***********************************************************************************************************************
* File Name    : common_init.h
* Description  : Common init functions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "hal_data.h"
#include "board_cfg.h"

#ifndef COMMON_INIT_H_
#define COMMON_INIT_H_

#define NUM_STRING_DESCRIPTOR               (7U)

/* g_update_console_event */
#define STATUS_INTERRUPT_IRQ0       ( 1 << 0 )
#define STATUS_INTERRUPT_IRQ10      ( 1 << 1 )
#define STATUS_INTERRUPT_IRQ09      ( 1 << 2 )

#define STATUS_WRITE_COMPLETE       ( 1 << 3 )    /* Update USB Write EVENT */
#define STATUS_USB_EVENT            ( 1 << 4 )    /* Update USB EVENT */

#define STATUS_UART7_RX_EVENT       ( 1 << 8 )
#define STATUS_CONSOLE_RX_EVENT     ( 1 << 9 )

#define MATTER_RESPONSE_EVENT       ( 1 << 12 )

/* Macro definitions */
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define APP_ERR_TRAP(a)             if(a) {__asm("BKPT #0\n");} /* trap the error location */


extern fsp_err_t common_init(void);
extern fsp_err_t print_to_console(char *p_data);
#endif /* COMMON_INIT_H_ */
