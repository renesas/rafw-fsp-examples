/***********************************************************************************************************************
 * File Name    : gpt_timer.c
 * Description  : Contains function definition.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "common_utils.h"
#include "gpt_timer.h"
#include "r_tim_w.h"
#include "hal_data.h"

/*******************************************************************************************************************//**
 * @addtogroup r_gpt_ep
 * @{
 **********************************************************************************************************************/

/* Boolean flag to determine one-shot mode timer is expired or not.*/
bool volatile g_one_shot_expired  = false;

/*****************************************************************************************************************
 * @brief       Initialize GPT timer.
 * @param[in]   p_timer_ctl     Timer instance control structure
 * @param[in]   p_timer_cfg     Timer instance Configuration structure
 * @param[in]   timer_mode      Mode of GPT Timer
 * @retval      FSP_SUCCESS     Upon successful open of timer.
 * @retval      Any Other Error code apart from FSP_SUCCES on Unsuccessful open .
 ****************************************************************************************************************/
fsp_err_t init_gpt_timer(timer_ctrl_t *const p_timer_ctl, timer_cfg_t const *const p_timer_cfg)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Initialize GPT Timer */
    err = R_TIM_W_Open(p_timer_ctl, p_timer_cfg);

    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT ("\r\n ** R_GPT_B_TimerOpen FAILED ** \r\n");

        return err;
    }
    return err;
}

/*****************************************************************************************************************
 * @brief       Start GPT timers in periodic, one shot, PWM mode.
 * @param[in]   p_timer_ctl     Timer instance control structure
 * @retval      FSP_SUCCESS     Upon successful start of timer.
 * @retval      Any Other Error code apart from FSP_SUCCES on Unsuccessful start .
 ****************************************************************************************************************/
fsp_err_t start_gpt_timer (timer_ctrl_t *const p_timer_ctl)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Starts GPT timer */
    err = R_TIM_W_Start(p_timer_ctl);

    if (FSP_SUCCESS != err)
    {
        /* In case of GPT_open is successful and start fails, requires a immediate cleanup.
         * Since, cleanup for GPT open is done in start_gpt_timer,Hence cleanup is not required */
        APP_ERR_PRINT ("\n** R_GPT_Start API failed **\n");
    }
    return err;
}

/*****************************************************************************************************************
 * @brief      Close the GPT HAL driver.
 * @param[in]  p_timer_ctl     Timer instance control structure
 * @retval     None
 ****************************************************************************************************************/
void deinit_gpt_timer(timer_ctrl_t *const p_timer_ctl)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Timer Close API call*/
    err = R_TIM_W_Close(p_timer_ctl);

    if (FSP_SUCCESS != err)
    {
        /* GPT Close failure message */
        APP_ERR_PRINT ("\n** R_GPT_Close FAILED **\n");
    }
}

void g_timer_oneshot_cb(timer_callback_args_t *p_args)
{
    if (NULL != p_args)
    {
        if (TIMER_EVENT_CYCLE_END  == p_args->event)
        {
            /* Set boolean flag on one-shot mode timer expired. */
            g_one_shot_expired = true;
        }
    }
}
/*******************************************************************************************************************//**
 * @} (end addtogroup r_gpt_ep)
 **********************************************************************************************************************/
