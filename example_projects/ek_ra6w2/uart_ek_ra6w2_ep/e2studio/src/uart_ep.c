/***********************************************************************************************************************
 * File Name    : uart_ep.c
 * Description  : Contains UART functions definition.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "common_utils.h"
#include "uart_ep.h"
#include "hal_data.h"

/*******************************************************************************************************************//**
 * @addtogroup r_uart_b_ep
 * @{
 **********************************************************************************************************************/

/*
 * Private function declarations
 */

/*
 * Private global variables
 */
/* Temporary buffer to save data from receive buffer for further processing */
static uint8_t g_temp_buffer[DATA_LENGTH] = {RESET_VALUE};

/* Counter to update g_temp_buffer index */
static volatile uint8_t g_counter_var = RESET_VALUE;

/* Flag to check whether data is received or not */
static volatile uint8_t g_data_received_flag = false;

/* Flag for user callback */
static volatile uint8_t g_uart_event = RESET_VALUE;

#define LED_ON      (2)
#define LED_OFF     (1)

/*****************************************************************************************************************
 *  @brief       Change LED status
 *  @param[in]   onoff
 *  @retval      FSP_SUCCESS     Upon success
 *  @retval      Any Other Error code apart from FSP_SUCCESS
 ****************************************************************************************************************/
static fsp_err_t set_led_status(uint32_t onoff)
{
    fsp_err_t err = FSP_SUCCESS;

    if (onoff == LED_ON)
    {
        err = R_GPIO_W_PinWrite(&IOPORT_CFG_CTRL, (bsp_io_port_pin_t) BSP_IO_PORT_00_PIN_10, BSP_IO_LEVEL_HIGH);
    }
    else if (onoff == LED_OFF)
    {
        err = R_GPIO_W_PinWrite(&IOPORT_CFG_CTRL, (bsp_io_port_pin_t) BSP_IO_PORT_00_PIN_10, BSP_IO_LEVEL_LOW);
    }
    else
    {
        APP_ERR_PRINT("\r\n **  Invalid parameter of LED on/off  ** \r\n");
        err = FSP_ERR_INVALID_ARGUMENT;
    }

    return err;
}

/*****************************************************************************************************************
 *  @brief       UART Example project to demonstrate the functionality
 *  @param[in]   None
 *  @retval      FSP_SUCCESS     Upon success
 *  @retval      Any Other Error code apart from FSP_SUCCESS
 ****************************************************************************************************************/
fsp_err_t uart_ep_demo(void)
{
    fsp_err_t err = FSP_SUCCESS;
    volatile bool b_valid_data = false;

    err = uart_print_user_msg((uint8_t*) "\r\nType LED status (on or off)\r\n");
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    while (true)
    {
        if (g_data_received_flag)
        {
            g_data_received_flag = false;

            uint8_t input_length = RESET_VALUE;
            volatile uint32_t led_onoff = RESET_VALUE;

            /* Calculate g_temp_buffer length */
            input_length = ((uint8_t) (strlen((char*) &g_temp_buffer)));

            /* Check if input data length is in limit */
            if (DATA_LENGTH > (uint8_t) input_length)
            {
                if ((strncmp((char*) g_temp_buffer, "on", strlen("on")) == 0))
                {
                    led_onoff = LED_ON;
                    b_valid_data = true;
                }
                else if (strncmp((char*) g_temp_buffer, "off", strlen("off")) == 0)
                {
                    led_onoff = LED_OFF;
                    b_valid_data = true;
                }

                /* Validation input data is "on" or "off". */
                if (led_onoff == RESET_VALUE)
                {
                    /* Resetting the g_temp_buffer as data is out of limit */
                    memset(g_temp_buffer, RESET_VALUE, DATA_LENGTH);
                    b_valid_data = false;

                    /* Application is being run on Serial terminal hence transmitting error message to the same */
                    err = uart_print_user_msg((uint8_t*) "\r\n#Invalid input. It should be on or off\r\n");
                    if (FSP_SUCCESS != err)
                    {
                        APP_ERR_PRINT("\r\n **  UART WRITE FAILED  ** \r\n");

                        return err;
                    }
                }
            }
            else
            {
                /* Clear data_valid flag as data is not valid, Clear the g_temp_buffer */
                memset(g_temp_buffer, RESET_VALUE, DATA_LENGTH);
                b_valid_data = false;

                err = uart_print_user_msg((uint8_t*) "\r\n*Invalid input. It should be on or off\r\n");
                if (FSP_SUCCESS != err)
                {
                    APP_ERR_PRINT("\r\n **  UART WRITE FAILED  ** \r\n");

                    return err;
                }
            }

            /* Set intensity only for valid data */
            if (b_valid_data)
            {
                /* Change status of LED */
                err = set_led_status(led_onoff);
                if (FSP_SUCCESS != err)
                {
                    APP_ERR_PRINT("\r\n** set_led_status failed while changing status ** \r\n");

                    return err;
                }

                /* Resetting the temporary buffer */
                memset(g_temp_buffer, RESET_VALUE, DATA_LENGTH);
                b_valid_data = false;
                err = uart_print_user_msg((uint8_t*) "\r\nType next LED status\r\n");
                if (FSP_SUCCESS != err)
                {
                    return err;
                }
            }
        }
    }
}

/*******************************************************************************************************************//**
 * @brief       Initialize  UART.
 * @param[in]   None
 * @retval      FSP_SUCCESS         Upon successful open and start of timer
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful open
 ***********************************************************************************************************************/
fsp_err_t uart_initialize(void)
{
    fsp_err_t err = FSP_SUCCESS;

    if (g_uart1_cfg.channel != 0)
    {
        APP_PRINT("Enable uart clock %d\r\n", g_uart1_cfg.channel);
        hw_clk_enable_uart_w_clk(g_uart1_cfg.channel);
    }

    /* Initialize UART channel with baud rate 115200 */
    err = R_UART_W_Open(&g_uart1_ctrl, &g_uart1_cfg);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n**  R_UART_B_Open API failed  **\r\n");
    }

    return err;
}

/*****************************************************************************************************************
 *  @brief       print user message to terminal
 *  @param[in]   p_msg
 *  @retval      FSP_SUCCESS                Upon success
 *  @retval      FSP_ERR_TRANSFER_ABORTED   Upon event failure
 *  @retval      Any Other Error code apart from FSP_SUCCESS,  Unsuccessful write operation
 ****************************************************************************************************************/
fsp_err_t uart_print_user_msg(uint8_t *p_msg)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t msg_len = RESET_VALUE;
    uint32_t local_timeout = (DATA_LENGTH *UINT16_MAX);
    char *p_temp_ptr = (char*) p_msg;

    /* Calculate length of message received */
    msg_len = ((uint8_t) (strlen(p_temp_ptr)));

    /* Reset callback capture variable */
    g_uart_event = RESET_VALUE;

    /* Writing to terminal */
    err = R_UART_W_Write(&g_uart1_ctrl, p_msg, msg_len);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n**  R_UART_B_Write API Failed  **\r\n");

        return err;
    }

    /* Check for event transfer complete */
    while ((UART_EVENT_TX_COMPLETE != g_uart_event) && (--local_timeout))
    {
        /* Check if any error event occurred */
        if (UART_ERROR_EVENTS == g_uart_event)
        {
            APP_ERR_PRINT("\r\n**  UART Error Event Received  **\r\n");

            return FSP_ERR_TRANSFER_ABORTED;
        }
    }

    if (RESET_VALUE == local_timeout)
    {
        err = FSP_ERR_TIMEOUT;
    }

    return err;
}

/*******************************************************************************************************************//**
 *  @brief       Deinitialize UART module
 *  @param[in]   None
 *  @retval      None
 **********************************************************************************************************************/
void deinit_uart(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close module */
    err = R_UART_W_Close(&g_uart1_ctrl);
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n**  R_UART_B_Close API failed  ** \r\n");
    }
}

/*****************************************************************************************************************
 *  @brief      UART user callback
 *  @param[in]  p_args
 *  @retval     None
 ****************************************************************************************************************/
void user_uart_callback(uart_callback_args_t *p_args)
{
    /* Logged the event in global variable */
    g_uart_event = (uint8_t) p_args->event;

    /* Reset g_temp_buffer index if it exceeds than buffer size */
    if (DATA_LENGTH == g_counter_var)
    {
        g_counter_var = RESET_VALUE;
    }

    if (UART_EVENT_RX_CHAR == p_args->event)
    {
        switch (p_args->data)
        {
            /* If Enter is pressed by user, set flag to process the data */
            case CARRIAGE_ASCII:
            {
                g_counter_var = RESET_VALUE;
                g_data_received_flag = true;
                break;
            }
                /* Read all data provided by user until enter button is pressed */
            default:
            {
                g_temp_buffer[g_counter_var++] = (uint8_t ) p_args->data;
                break;
            }
        }
    }
}

/*******************************************************************************************************************//**
 * @} (end addtogroup r_uart_b_ep)
 **********************************************************************************************************************/
