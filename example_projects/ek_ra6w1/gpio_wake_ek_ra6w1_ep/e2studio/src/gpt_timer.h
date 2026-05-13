/***********************************************************************************************************************
 * File Name    : gpt_timer.h
 * Description  : Contains Macros and function declarations.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef GPT_TIMER_H_
#define GPT_TIMER_H_

#include "r_timer_api.h"

/* Function declaration */
fsp_err_t init_gpt_timer(timer_ctrl_t *const p_timer_ctl, timer_cfg_t const *const p_timer_cfg);
fsp_err_t start_gpt_timer (timer_ctrl_t *const p_timer_ctl);
void deinit_gpt_timer(timer_ctrl_t *const p_timer_ctl);
void set_led(bsp_io_level_t led_state);

#endif /* GPT_TIMER_H_ */
