/***********************************************************************************************************************
 * File Name    : spi_ep.c
 * Description  : Contains data structures and functions used in spi_ep.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **********************************************************************************************************************/

#include "common_utils.h"
#include "spi_ep.h"

/***********************************************************************************************************************
 * @addtogroup spi_ep
 * @{
 **********************************************************************************************************************/

/*
 * private function declarations
 */
static fsp_err_t spi_wait_transfer_complete(void);
static void error_print(const char *module, const char *error_type, fsp_err_t err_code);
static fsp_err_t read_user_input(uint32_t * buf, uint16_t buf_len, uint32_t * p_num_bytes);

/* Wait counter for wait operation monitoring */
static volatile uint32_t g_wait_count = MAX_COUNT;

/* Event flags for master and slave */
static volatile spi_event_t g_master_event_flag;    // Master Transfer Event completion flag
static volatile spi_event_t g_slave_event_flag;     // Slave Transfer Event completion flag

/* SPI module buffers for Master and Slave */
static uint32_t g_master_tx_buff[BUFF_LEN];   // Master Transmit Buffer
static uint32_t g_master_rx_buff[BUFF_LEN];   // Master Receive Buffer
static uint32_t g_slave_tx_buff[BUFF_LEN];    // Slave Transmit Buffer
static uint32_t g_slave_rx_buff[BUFF_LEN];    // Slave Receive Buffer

/***********************************************************************************************************************
 * @brief       This functions initializes SPI master and slave modules.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful open of SPI module
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful open
 **********************************************************************************************************************/
fsp_err_t spi_init(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status

    g_wait_count = MAX_COUNT;
    g_master_event_flag = (spi_event_t) RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = (spi_event_t) RESET_VALUE;  // Reseting slave_event flag

    /* Open/Initialize SPI Master module */
    err = R_SPI_W_Open(&g_spi_master_ctrl, &g_spi_master_cfg);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Master Failure message */
        APP_ERR_PRINT("** R_SPI_W_Open API for SPI Master failed ** \r\n");

        return err;
    }

    /* Open/Initialize SPI Slave module */
    err = R_SPI_W_Open(&g_spi_slave_ctrl, &g_spi_slave_cfg);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* Close SPI master */
        if ((FSP_SUCCESS != R_SPI_W_Close(&g_spi_master_ctrl)))
        {
            /* SPI Master Close Failure message */
            APP_ERR_PRINT("** R_SPI_W_Close API for SPI Master failed ** \r\n");
        }
        /* SPI Slave Failure message */
        APP_ERR_PRINT("** R_SPI_W_Open API for SPI Slave failed ** \r\n");
    }

    return err;
}

/***********************************************************************************************************************
 * @brief 		This function demos both R_SPI_W_Write() and R_SPI_W_Read() individually.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful SPI Write and SPI Read
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful Write and Read
 **********************************************************************************************************************/
fsp_err_t spi_write_and_read(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status
    uint32_t num_bytes = RESET_VALUE;  // Number of bytes read by SEGGER real-time-terminal

    /* Cleaning buffers */
    memset(&g_master_tx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_master_rx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_slave_rx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_slave_tx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);

    /* Input to master buffer */
    APP_PRINT("\r\nEnter text input for Master buffer. Data size should not exceed 64 bytes. \r\n");
    err = read_user_input(g_master_tx_buff, BUFF_LEN, &num_bytes);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* Slave receive data from Master */
    err = R_SPI_W_Read(&g_spi_slave_ctrl, g_slave_rx_buff, num_bytes, SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (err != FSP_SUCCESS)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_W_Read() failed");

        return err;
    }

    /* Master send data to Slave */
    err = R_SPI_W_Write(&g_spi_master_ctrl, g_master_tx_buff, num_bytes, SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_W_Write() failed");

        return err;
    }

    /* Wait until master write and slave read complete */
    err = spi_wait_transfer_complete();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_wait_count = MAX_COUNT;
    g_master_event_flag = (spi_event_t) RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = (spi_event_t) RESET_VALUE;  // Reseting slave_event flag

    /* Display Master to Slave transmission */
    APP_PRINT("\r\nMaster transmitted data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s \r\n" RTT_CTRL_RESET, (char *)g_master_tx_buff);
    APP_PRINT("\r\nSlave received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s \r\n" RTT_CTRL_RESET, (char *)g_slave_rx_buff);

    /* Slave send data to Master */
    err = R_SPI_W_Write(&g_spi_slave_ctrl, g_slave_rx_buff, num_bytes, SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_W_Write() failed");

        return err;
    }

    /* Master receive data from Slave */
    err = R_SPI_W_Read(&g_spi_master_ctrl, g_master_rx_buff, num_bytes, SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_W_Read() failed");

        return err;
    }

    /* Wait until slave write and master read complete */
    err = spi_wait_transfer_complete();
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    
    g_wait_count = MAX_COUNT;
    g_master_event_flag = (spi_event_t) RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = (spi_event_t) RESET_VALUE;  // Reseting slave_event flag

    /* Display Slave to Master transmission */
    APP_PRINT("\r\nSlave transmitted data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s \r\n" RTT_CTRL_RESET, (char *)g_slave_rx_buff);
    APP_PRINT("\r\nMaster received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s \r\n" RTT_CTRL_RESET, (char *)g_master_rx_buff);

    /* Check whether transmitted data is equal to received data */
    if (BUFF_EQUAL == memcmp(g_master_tx_buff, g_master_rx_buff, num_bytes * sizeof(uint32_t)))
    {
        APP_PRINT("\r\nData transmitted and received are same.\r\n");
    }
    else
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_PRINT("\r\nReceived data does not match with transmitted Data.\r\n");
        error_print("SPI", "PARITY_ERROR", err);
        return err;
    }

    /* Delay of one second for user to verify the output */
    R_BSP_SoftwareDelay(DELAY_ONE_SEC, BSP_DELAY_UNITS_SECONDS);

    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * @brief       This function demos R_SPI_W_WriteRead() for both Master and Slave simultaneously.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful Write and Read for both Master and Slav
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful Write and Read
 **********************************************************************************************************************/
fsp_err_t spi_write_read(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status
    /* Number of bytes read by SEGGER real-time-terminal for master and slave inputs */
    uint32_t num_bytes_master = RESET_VALUE;
    uint32_t num_bytes_slave = RESET_VALUE;
    uint32_t transmit_bytes = RESET_VALUE;

    /* Cleaning buffers */
    memset(&g_master_tx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_master_rx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_slave_tx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);
    memset(&g_slave_rx_buff[0], 0, sizeof(uint32_t) * BUFF_LEN);

    /* Input to master buffer */
    APP_PRINT("\r\nEnter text input for Master buffer. Data size should not exceed 64 bytes.\r\n");
    err = read_user_input(g_master_tx_buff, BUFF_LEN, &num_bytes_master);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* Input to slave buffer */
    APP_PRINT("\r\nEnter text input for Slave buffer. Data size should not exceed 64 bytes.\r\n");
    err = read_user_input(g_slave_tx_buff, BUFF_LEN, &num_bytes_slave);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    transmit_bytes = (num_bytes_master > num_bytes_slave) ? num_bytes_master : num_bytes_slave;

    /* Slave send data to Master and receive data from Master */
    err = R_SPI_W_WriteRead(&g_spi_slave_ctrl, g_slave_tx_buff, g_slave_rx_buff, transmit_bytes,
                            SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_W_WriteRead() failed");

        return err;
    }

    /* Master send data to Slave and receive data from Slave */
    err = R_SPI_W_WriteRead(&g_spi_master_ctrl, g_master_tx_buff, g_master_rx_buff, transmit_bytes,
                            SPI_BIT_WIDTH_32_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_W_WriteRead() failed");

        return err;
    }

    /* Wait until master and slave WriteRead() complete */
    err = spi_wait_transfer_complete();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_wait_count = MAX_COUNT;
    g_master_event_flag = (spi_event_t) RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = (spi_event_t) RESET_VALUE;  // Reseting slave_event flag

    /* Display Master's received data on RTT */
    APP_PRINT("\r\nMaster received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s\r\n" RTT_CTRL_RESET, (char *)g_master_rx_buff);

    /* Display Slave's received data on RTT */
    APP_PRINT("\r\nSlave received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s\r\n" RTT_CTRL_RESET, (char *)g_slave_rx_buff);

    /* Check whether Slave transmitted data is not equal to Master received data */
    if (BUFF_EQUAL != memcmp(g_slave_tx_buff, g_master_rx_buff, num_bytes_slave * sizeof(uint32_t)))
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_ERR_PRINT("\r\nMaster received data does not match with Slave transmitted Data.\r\n");
        error_print("SPI Master", "PARITY_ERROR", err);
        return err;
    }

    /* Check whether Master transmitted data is not equal to Slave received data */
    if (BUFF_EQUAL != memcmp(g_master_tx_buff, g_slave_rx_buff, num_bytes_master * sizeof(uint32_t)))
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_ERR_PRINT("\r\nSlave received data does not match with Master transmitted Data.\r\n");
        error_print("SPI Slave", "PARITY_ERROR", err);
        return err;
    }

    /* Delay of one second for user to verify the output */
    R_BSP_SoftwareDelay(DELAY_ONE_SEC, BSP_DELAY_UNITS_SECONDS);

    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * @brief       This function close both SPI Master and Slave modules.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  SPI module closed successfully
 * @retval      Any Other Error code apart from FSP_SUCCESS  Unsuccessful close
 **********************************************************************************************************************/
fsp_err_t spi_exit_demo(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status

    /* Closing SPI Master module */
    err = R_SPI_W_Close(&g_spi_master_ctrl);
    /* Error Handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_W_Close() failed");
        error_print("SPI Master", "CLOSE_FAILED", err);
    }

    /* Closing SPI Slave module */
    err = R_SPI_W_Close(&g_spi_slave_ctrl);
    /* Error Handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_W_Close() failed");
        error_print("SPI Slave", "CLOSE_FAILED", err);
    }

    return err;
}

/***********************************************************************************************************************
 * @brief       Master SPI callback function.
 * @param[in]   p_args
 * @retval      None
 **********************************************************************************************************************/
void spi_master_callback(spi_callback_args_t *p_args)
{
    g_master_event_flag = p_args->event;
}

/***********************************************************************************************************************
 * @brief       Slave SPI callback function.
 * @param[in]   p_args
 * @retval      None
 **********************************************************************************************************************/
void spi_slave_callback(spi_callback_args_t *p_args)
{
    g_slave_event_flag = p_args->event; 
}

/***********************************************************************************************************************
 * @brief       This function closes all the opened SPI modules before the project ends up in an Error Trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
void spi_clean_up(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close SPI module */
    err = R_SPI_W_Close(&g_spi_master_ctrl);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Close failure message */
        error_print("SPI Master", "CLOSE_FAILED", err);
    }

    err = R_SPI_W_Close(&g_spi_slave_ctrl);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Close failure message */
        error_print("SPI Slave", "CLOSE_FAILED", err);
    }
}

/***********************************************************************************************************************
 * @brief       This function waits until SPI transfer is complete and monitors the wait operation to avoid infinite wait.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  SPI transfer completed successfully
 * @retval      FSP_ERR_TIMEOUT              SPI transfer failed to complete within expected time
 * @retval      FSP_ERR_TRANSFER_ABORTED     SPI transfer was aborted
 **********************************************************************************************************************/
static fsp_err_t spi_wait_transfer_complete(void)
{
    fsp_err_t err = FSP_SUCCESS;
    spi_event_t master_flag, slave_flag;

    /* Wait until SPI transfer is complete. If transfer fails to complete within expected time, return time-out error.
     * If transfer is aborted, return transfer aborted error. */
    while (1)
    {
        master_flag = g_master_event_flag;
        slave_flag = g_slave_event_flag;

        /* If both master and slave event flags are set to SPI_EVENT_TRANSFER_COMPLETE, break the wait loop and return success */
        if ((SPI_EVENT_TRANSFER_COMPLETE == master_flag) && (SPI_EVENT_TRANSFER_COMPLETE == slave_flag))
        {
            break;
        }

        /* Time out operation if SPI operation fails to complete */
        g_wait_count--;
        if (MIN_COUNT >= g_wait_count)
        {
            /* Return time out error if SPI operation fails to complete */
            err = FSP_ERR_TIMEOUT;
            if (SPI_EVENT_TRANSFER_COMPLETE != master_flag)
            {
                error_print("SPI Master", "TIMEOUT", err);
            }
            else
            {
                error_print("SPI Slave", "TIMEOUT", err);
            }
            return err;
        }

        /* Return transfer aborted error if SPI event flag is set to any other value apart from SPI_EVENT_TRANSFER_COMPLETE */
        if ((SPI_EVENT_TRANSFER_COMPLETE != master_flag) && (master_flag != RESET_VALUE))
        {
            err = FSP_ERR_TRANSFER_ABORTED;
            APP_PRINT("[INFO] Master Event Flag: 0x%x\r\n", master_flag);
            error_print("SPI Master", "TRANSFER_ABORTED", err);
            return err;
        }

        /* Return transfer aborted error if SPI event flag is set to any other value apart from SPI_EVENT_TRANSFER_COMPLETE */
        if ((SPI_EVENT_TRANSFER_COMPLETE != slave_flag) && (slave_flag != RESET_VALUE))
        {
            err = FSP_ERR_TRANSFER_ABORTED;
            APP_PRINT("[INFO] Slave Event Flag: 0x%x\r\n", slave_flag);
            error_print("SPI Slave", "TRANSFER_ABORTED", err);
            return err;
        }
    }

    return err;
}

/***********************************************************************************************************************
 * @brief       This function prints the error message on RTT console.
 * @param[IN]   module      Module name where the error occurred
 * @param[IN]   error_type  Type of error occurred
 * @param[IN]   err_code    Error code returned by the API
 * @retval      None
 **********************************************************************************************************************/
static void error_print(const char *module, const char *error_type, fsp_err_t err_code)
{
    APP_PRINT("\r\n[ERROR] Module: %s | Type: %s | Code: 0x%x\r\n",
              module, error_type, err_code);
    APP_PRINT("Reset the MCU...\r\n");
}

/***********************************************************************************************************************
 * @brief       This function reads user input from RTT console and calculates the number of bytes to be transferred.
 * @param[IN]   buf         Buffer to store user input
 * @param[IN]   buf_len     Length of the buffer
 * @param[IN]   p_num_bytes Pointer to store number of bytes to be transferred
 * @retval      FSP_SUCCESS                  User input read successfully
 * @retval      FSP_ERR_INVALID_SIZE         User input exceeds buffer size
 **********************************************************************************************************************/
static fsp_err_t read_user_input(uint32_t *buf, uint16_t buf_len, uint32_t *p_num_bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t num_bytes = RESET_VALUE;

    while (BYTES_RECEIVED_ZERO == num_bytes)
    {
        if (APP_CHECK_DATA)
        {
            num_bytes = APP_READ_BYTES(buf, buf_len * sizeof(uint32_t));
            if (BYTES_RECEIVED_ZERO == num_bytes)
            {
                APP_PRINT("\r\nNo Input\r\n");
            }
        }
    }

    /* Remove new line character */
    num_bytes -= 1U;

    /* RTT Reads user input data 1 byte at a time. SPI transfers the data 4 bytes at a time.
     * With the below logic, we will calculate how many length of data has to be transferred. */
    if (num_bytes % BITS_TO_BYTES != RESET_VALUE)
    {
        num_bytes = (num_bytes / BITS_TO_BYTES) + 1U;
    }
    else
    {
        num_bytes = num_bytes / BITS_TO_BYTES;
        if (num_bytes < buf_len)
        {
            buf[num_bytes] = RESET_VALUE;
        }
        else
        {
            /* Return Invalid Size error if user input exceeds buffer size */
            err = FSP_ERR_INVALID_SIZE;
            APP_ERR_PRINT("\r\nInput data size exceeds buffer size.\r\n");
            return err;
        }
    }

    *p_num_bytes = num_bytes;

    return err;
}

/***********************************************************************************************************************
 * @} (end addtogroup spi_ep)
 **********************************************************************************************************************/
