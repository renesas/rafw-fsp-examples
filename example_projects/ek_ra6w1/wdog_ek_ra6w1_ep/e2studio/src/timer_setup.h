/***********************************************************************************************************************
 * File Name    : timer_setup.h
 * Description  : Contains data structures and functions used in timer_setup.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **********************************************************************************************************************/

#ifndef TIMER_SETUP_H_
#define TIMER_SETUP_H_

#define BUFFER_SIZE                                 (16u)

/* User command input value */
#define ENABLE_WDOG                                 (1u)

/* Default value of Reset status register */
#define RESET_STATUS_REGISTER_POR_DEFAULT           (0x1F)

/* WDOG detect reset value */
#define RESET_STATUS_REGISTER_DETECT_WDOG_RESET     (0x16)

/* GPIO pin for LED notification */
#define LED_POR_AND_TIMER_ACTIVE_IND                (BSP_IO_PORT_01_PIN_12)
#define LED_WATCHDOG_RESET_IND                      (BSP_IO_PORT_01_PIN_13)

/*
 * function declarations
 */
fsp_err_t init_gpt_module(void);
fsp_err_t timer_start(void);
void deinit_gpt_module(void);

#endif /* TIMER_SETUP_H_ */
