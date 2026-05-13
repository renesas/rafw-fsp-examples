/***********************************************************************************************************************
 * File Name    : adc_ep.c
 * Description  : Contains variables and functions used in adc_ep.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "hal_data.h"
#include "common_utils.h"
#include "adc_ep.h"

/*******************************************************************************************************************//**
 * @addtogroup adc_ep
 * @{
 **********************************************************************************************************************/

/* local variables */
volatile bool b_ready_to_read = false;
static uint16_t g_adc_data;
static bool g_window_comp_event = false;
char volt_str[10] = {RESET_VALUE};
float adc_volt = {RESET_VALUE};
int adc_volt_int = 0;
int adc_volt_decimal = 0;
 
/*
 * private function declarations
 */
/* Open the adc module, configures and initiates the scan*/
static fsp_err_t adc_scan_start(void);

/* stops the adc scan if the adc is continuous scan and then close the module */
static fsp_err_t adc_scan_stop(void);

/* Callback to handle window compare event */
void adc_callback(adc_callback_args_t *p_args);

/*******************************************************************************************************************//**
 * @brief     This function reads the command (input) from RTT and process the command(input).
 * @param[IN]   None
 * @retval FSP_SUCCESS                  Upon successful, action of adc start or adc stop takes place
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful start or stop
 ***********************************************************************************************************************/
fsp_err_t read_process_input_from_rtt(void)
{
    /* Error status */
    fsp_err_t err = FSP_SUCCESS;
    uint8_t read_buff[BUFF_SIZE] = {RESET_VALUE};

    /* Read user input */
    uint32_t r_byte = APP_READ(read_buff);

    /* function returns the converted integral number as an int value.*/
    int32_t input_read = atoi((char *)read_buff);

    if (r_byte > RESET_VALUE)
    {
        switch (input_read)
        {
            case SCAN_START:
            {
               err = adc_scan_start();
            }
            break;

            case SCAN_STOP:
            {
               err = adc_scan_stop();
            }
            break;

            default:
            {
                /* Menu for User Selection */
                APP_PRINT_INFO(MENUOPTION1);
                APP_PRINT_INFO(MENUOPTION2);
                APP_PRINT_INFO(MENUOPTION3);
                APP_PRINT_INFO(MENUOPTION4);
            }
            break;
        }
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief    This function open the ADC, configures and starts the scan
 * @param[IN]   None
 * @retval FSP_SUCCESS                  Upon successful open,configure adc,on success or calibrate and start adc scan
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful open or configure or start
 ***********************************************************************************************************************/
static fsp_err_t adc_scan_start(void)
{
    /*  Error status */
    fsp_err_t err = FSP_SUCCESS;
    g_window_comp_event = false;

    if (false == b_ready_to_read)
    {
        /* Open/Initialize ADC module */
        err = R_ADC_W_Open(&g_adc0_ctrl, &g_adc0_cfg);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_Open API failed ** \r\n");

            return err;
        }

        /* Configures the ADC scan parameters */
        err = R_ADC_W_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_ScanCfg API failed ** \r\n");

            return err;
        }

        /* Start the ADC scan*/
        err = R_ADC_W_ScanStart(&g_adc0_ctrl);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_ScanStart API failed ** \r\n");

            return err;
        }

        /* Disable interrupts */
        R_BSP_IrqDisable((IRQn_Type)ADC_EVENT_SCAN_COMPLETE);
        APP_PRINT_INFO("\r\nADC Started Scan\r\n");

        /* Indication to start reading the adc data */
        b_ready_to_read = true;
    }
    else
    {
        APP_PRINT_INFO("\r\nADC Scan already in progress\r\n");
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief    This function stops the scanning of adc
 * @param[IN]   None
 * @retval FSP_SUCCESS                  Upon successful stops the adc scan and closes the adc
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful stop or close of adc
 ***********************************************************************************************************************/
static fsp_err_t adc_scan_stop(void)
{
    /* Error status */
    fsp_err_t err = FSP_SUCCESS;

    /* Stop the scan if adc scan is started in continuous scan mode else ignore */
    if ((ADC_MODE_SINGLE_SCAN != g_adc0_cfg.mode) && (true == b_ready_to_read))
    {
        /* Stop ADC scan */
        err = R_ADC_W_ScanStop(&g_adc0_ctrl);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_ScanStop API failed ** \r\n");

            return err;
        }

        APP_PRINT_INFO("\r\nADC Scan stopped\r\n");

        /* reset to indicate stop reading the adc data */
        b_ready_to_read = false;

        /* Close the ADC module*/
        err = R_ADC_W_Close(&g_adc0_ctrl);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_Close API failed ** \r\n");

            return err;
        }
    }
    else
    {
        APP_PRINT_INFO("\r\nStop command is not supported in Single Scan mode "
                       "or User not pressed Start Scan in Continuous mode \r\n");
    }

    APP_PRINT_INFO("\r\nPress any other key (except 1 and 2) to go back to the main menu\r\n");

    return err;
}

/*******************************************************************************************************************//**
 * @brief    This function reads the adc output data from the prescribed channel and checks adc status
 * @param[IN]   None
 * @retval FSP_SUCCESS                  Upon successful stops the adc scan and closes the adc
 * @retval Any Other Error code apart from FSP_SUCCES  Unsuccessful stop or close of adc
 ***********************************************************************************************************************/
fsp_err_t adc_read_data(void)
{
    /* Error status */
    fsp_err_t err = FSP_SUCCESS;

    /* Read the result */
    err = R_ADC_W_Read(&g_adc0_ctrl, ADC_CHANNEL_0, &g_adc_data);

    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* ADC Failure message */
        APP_PRINT_ERR("** R_ADC_Read API failed ** \r\n");

        return err;
    }

    g_adc_data = g_adc_data >> 4;

    /* 12-bit ADC, we receive as 16-bit result */
    adc_volt = (float)((g_adc_data * V_ref)/ADC_12_BIT);
    adc_volt_int = (int) adc_volt;
    adc_volt_decimal = (int)((adc_volt - (float)adc_volt_int) * 1000);

    APP_PRINT_INFO("\r\nThe Voltage Reading from ADC: %d\r\n", g_adc_data);
    APP_PRINT_INFO("\r\nThe ADC input voltage: %d.%dV\r\n", adc_volt_int, adc_volt_decimal);

    /* In adc single scan mode after reading the data, it stops.So reset the b_ready_to_read state to
       avoid reading unnecessarily. close the adc module as it gets opened in start scan command.*/
    if (ADC_MODE_SINGLE_SCAN == g_adc0_cfg.mode || g_window_comp_event == true)
    {
        b_ready_to_read = false;

        /* Stop ADC scan */
        err = R_ADC_W_ScanStop(&g_adc0_ctrl);

        /* Handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC ScanStop message */
            APP_PRINT_ERR("** R_ADC_ScanStop API failed ** \r\n");
            APP_ERR_TRAP(err);
        }

        if (((g_window_comp_event == true) && (ADC_MODE_SINGLE_SCAN == g_adc0_cfg.mode)) ||
            (ADC_MODE_CONTINUOUS_SCAN == g_adc0_cfg.mode))
        {
            /* Print temperature status warning to RTT Viewer */
            if (ADC_L_LMT > g_adc_data)
            {
                APP_PRINT_INFO("\r\nADC Voltage is below the Lower Limit. \r\n");
            }
            else if (ADC_H_LMT < g_adc_data)
            {
                APP_PRINT_INFO("\r\nADC Voltage is above the Upper Limit.  \r\n");
            }
        }

        /* Close the ADC module*/
        err = R_ADC_W_Close(&g_adc0_ctrl);

        /* handle error */
        if (FSP_SUCCESS != err)
        {
            /* ADC Failure message */
            APP_PRINT_ERR("** R_ADC_Close API failed ** \r\n");

            return err;
        }

        APP_PRINT_INFO("\r\nPress any other key(except 1 and 2) to go back to the main menu\r\n");
    }

    /* 1 Seconds Wait time between successive readings */
    R_BSP_SoftwareDelay (ADC_READ_DELAY, BSP_DELAY_UNITS_SECONDS);

    return err;
}

/*******************************************************************************************************************//**
 * @brief    Close the adc driver and Handle the return closing API error, to the Application.
 * @param[IN]   None
 * @retval None.
 ***********************************************************************************************************************/
void deinit_adc_module(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* close the ADC driver */
    err = R_ADC_W_Close(&g_adc0_ctrl);

    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* GPT Close failure message */
        APP_PRINT_ERR("** R_ADC_Close API failed **  \r\n");
    }
}

/* Callback procedure for when window A compare event occurs */
void adc_callback(adc_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    g_window_comp_event = true;
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    R_BSP_IrqDisable(irq);
}
/*******************************************************************************************************************//**
 * @} (end addtogroup adc_ep)
 **********************************************************************************************************************/
