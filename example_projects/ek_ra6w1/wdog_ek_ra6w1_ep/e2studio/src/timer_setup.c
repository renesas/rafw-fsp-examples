/***********************************************************************************************************************
 * File Name    : timer_setup.c
 * Description  : Contains data structures and functions used in timer_setup.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **********************************************************************************************************************/

#include "common_utils.h"
#include "timer_setup.h"

/*******************************************************************************************************************//**
 * @addtogroup r_wdog_ep
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * @brief     Initialize the GPT timer in Periodic mode.
 * @param[IN]   None
 * @retval FSP_SUCCESS                  Upon successful open
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful open
 **********************************************************************************************************************/
fsp_err_t init_gpt_module(void)
{
    /* variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;

    /* Open GPT module */
    err = R_TIM_W_Open(&g_timer_ctrl, &g_timer_cfg);
    if (FSP_SUCCESS != err)
    {
        /* Print Error on RTT console */
        APP_ERR_PRINT("\r\n ** R_TIM_W_Open API failed ** \r\n");
    }

    return err;
}

/***********************************************************************************************************************
 * @brief    Start the GPT HAL timer and return the error to the Application.
 * @param[IN]   None
 * @retval FSP_SUCCESS                 Timer driver started successfully.
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful to start timer
 **********************************************************************************************************************/
fsp_err_t timer_start(void)
{
    /* variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;

    /* Start GPT timer */
    err = R_TIM_W_Start(&g_timer_ctrl);
    if (FSP_SUCCESS != err)
    {
        /* Print Error on RTT console */
        APP_ERR_PRINT("\r\n ** R_TIM_W_Start API failed ** \r\n");
    }

    return err;
}

/***********************************************************************************************************************
 * @brief    Close the GPT HAL driver and Handle the return closing API error, to the Application.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
void deinit_gpt_module(void)
{
    /* variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;

    /* Close the GPT module */
    err = R_TIM_W_Close(&g_timer_ctrl);
    if (FSP_SUCCESS != err)
    {
        /* Print Error on RTT console */
        APP_ERR_PRINT("\r\n ** R_TIM_W_Close API failed ** \r\n");
    }
}

/***********************************************************************************************************************
 * This function is called when gpt timer's counter wrapped around.
 * @brief    Refresh WDOG counter and toggle LED state.
 * @param[IN]   p_args   Callback function parameter data
 * @retval      None
 **********************************************************************************************************************/
void gpt_callback(timer_callback_args_t *p_args)
{
    /* variable to track error and return values */
    fsp_err_t err = FSP_SUCCESS;

    /* variable to toggle LED */
    static bsp_io_level_t level_led = BSP_IO_LEVEL_HIGH;

    FSP_PARAMETER_NOT_USED(p_args);

    err = R_WDOG_W_Refresh(&g_wdog_ctrl);
    if (FSP_SUCCESS != err)
    {
        /* Turn ON LED to indicate error, along with output on RTT*/
        R_GPIO_W_PinWrite(&g_gpio_ctrl, LED_POR_AND_TIMER_ACTIVE_IND, BSP_IO_LEVEL_HIGH);

        /* Print Error on RTT console */
        APP_ERR_PRINT("\r\n ** R_WDOG_W_Refresh API failed ** \r\n");
    }
    else
    {
        /* Toggle LED */
        level_led ^= BSP_IO_LEVEL_HIGH;
        R_GPIO_W_PinWrite(&g_gpio_ctrl, LED_POR_AND_TIMER_ACTIVE_IND, level_led);
        APP_PRINT("\r\nWDOG counter Refreshed.");
    }
}

/***********************************************************************************************************************
 * This function is called when the WDOG reset happened.
 * @brief    Print out the debug message.
 * @param[IN]   p_args   Callback function parameter data
 * @retval      None
 **********************************************************************************************************************/
void g_wdog_reset_callback(wdt_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    APP_PRINT("\r\n Watchdog Reset Callback called\r\n");
}

/*******************************************************************************************************************//**
 * @} (end addtogroup r_wdog_ep)
 **********************************************************************************************************************/
