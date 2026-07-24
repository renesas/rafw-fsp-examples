/***********************************************************************************************************************
 * File Name    : uart.c
 * Description  : Contains UART function definitions.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "common_utils.h"
#include "uart.h"

/*******************************************************************************************************************//**
 * @addtogroup r_uart_b_ep
 * @{
 ***********************************************************************************************************************/

/* Private global variables */
static uint8_t g_temp_buffer[DATA_LENGTH] = { RESET_VALUE };
static volatile uint8_t g_counter_var     = RESET_VALUE;
static volatile uint8_t g_data_received_flag = false;
static volatile uint8_t g_uart_event      = RESET_VALUE;

int g_mcu_write_length = 0;

/***********************************************************************************************************************
 * Function Name: uart_initialize
 * Description  : Initializes UART module.
 * Arguments    : None
 * Return Value : FSP status
 ***********************************************************************************************************************/
fsp_err_t uart_initialize(void)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_UART_W_Open(&g_uart1_ctrl, &g_uart1_cfg);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** R_UART_B_Open API failed **\r\n");
    }

    return err;
}

/***********************************************************************************************************************
 * Function Name: uart_print_user_msg
 * Description  : Sends data to terminal over UART.
 * Arguments    : p_msg - pointer to message buffer
 *                p_len - message length
 * Return Value : FSP status
 ***********************************************************************************************************************/
fsp_err_t uart_print_user_msg(uint8_t * p_msg, uint32_t p_len)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t msg_len = p_len;
    uint32_t local_timeout = (DATA_LENGTH * UINT16_MAX);
    char * p_temp_ptr = (char *) p_msg;

    msg_len = (uint8_t) strlen(p_temp_ptr);
    g_mcu_write_length += msg_len;

    g_uart_event = RESET_VALUE;

    err = R_UART_W_Write(&g_uart1_ctrl, p_msg, msg_len);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** R_UART_B_Write API Failed **\r\n");
        return err;
    }

    while ((UART_EVENT_TX_COMPLETE != g_uart_event) && (--local_timeout))
    {
        if (UART_ERROR_EVENTS == g_uart_event)
        {
            APP_ERR_PRINT("\r\n** UART Error Event Received **\r\n");
            return FSP_ERR_TRANSFER_ABORTED;
        }
    }

    if (RESET_VALUE == local_timeout)
    {
        err = FSP_ERR_TIMEOUT;
    }

    return err;
}

/***********************************************************************************************************************
 * Function Name: deinit_uart
 * Description  : Deinitializes UART module.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
void deinit_uart(void)
{
    fsp_err_t err = FSP_SUCCESS;

    err = R_UART_W_Close(&g_uart1_ctrl);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** R_UART_B_Close API failed **\r\n");
    }
}

/***********************************************************************************************************************
 * Function Name: user_uart_callback
 * Description  : UART callback handler.
 * Arguments    : p_args - UART callback arguments
 * Return Value : None
 ***********************************************************************************************************************/
void user_uart_callback(uart_callback_args_t * p_args)
{
    g_uart_event = (uint8_t) p_args->event;

    if (DATA_LENGTH == g_counter_var)
    {
        g_counter_var = RESET_VALUE;
    }

    if (UART_EVENT_TX_COMPLETE == p_args->event)
    {
        g_uart_event = UART_EVENT_TX_COMPLETE;
    }
}

/*******************************************************************************************************************//**
 * @} (end addtogroup r_uart_b_ep)
 ***********************************************************************************************************************/
