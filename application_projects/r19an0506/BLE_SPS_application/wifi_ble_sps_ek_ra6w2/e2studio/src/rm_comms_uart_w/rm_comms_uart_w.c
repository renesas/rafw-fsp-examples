/***********************************************************************************************************************
 * File Name    : rm_comms_uart_w.c
 * Description  : UART Comms implementation
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include "rm_comms_uart_w.h"
#include "../rm_comms_lock/rm_comms_lock.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Definitions of Open flag "COUT" */
#define RM_COMMS_UART_W_OPEN    (0x434F5554UL)

#define MODULO(a, N)            ((a) % (N) + (N)) % (N)
#define WATERMARK_REACHED(x)    ((x) > (RM_COMMS_UART_W_RX_BUFF_SIZE * 4 / 5)) /* 4/5 of buffer capacity */
#define BUFFERED_BYTES(x)       ((uint32_t) MODULO((int) ((x)->rx_buff_head - (x)->rx_buff_tail), \
                                                   RM_COMMS_UART_W_RX_BUFF_SIZE))

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static void rm_comms_uart_w_callback(uart_callback_args_t * p_args);
static void rm_comms_uart_w_notify_application(rm_comms_uart_w_instance_ctrl_t const * p_ctrl,
                                               rm_comms_event_t                        event);
static fsp_err_t rm_comms_uart_w_rx_buff_read(rm_comms_uart_w_instance_ctrl_t * p_ctrl, uint8_t ** rbuf,
                                              uint32_t * rlen);

/***********************************************************************************************************************
 * Global variables
 **********************************************************************************************************************/
rm_comms_api_t const g_comms_on_comms_uart_w =
{
    .open        = RM_COMMS_UART_W_Open,
    .read        = RM_COMMS_UART_W_Read,
    .write       = RM_COMMS_UART_W_Write,
    .writeRead   = RM_COMMS_UART_W_WriteRead,
    .callbackSet = RM_COMMS_UART_W_CallbackSet,
    .close       = RM_COMMS_UART_W_Close,
};

/***********************************************************************************************************************
 * @addtogroup RM_COMMS_UART_W
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

static fsp_err_t rm_comms_uart_w_rx_buff_read (rm_comms_uart_w_instance_ctrl_t * p_ctrl,
                                               uint8_t                        ** rbuf,
                                               uint32_t                        * rlen)
{
    uint32_t  rx_buffered_bytes;
    uint32_t  rx_available_bytes;
    fsp_err_t ret = FSP_SUCCESS;
    rm_comms_uart_w_extended_cfg_t const * p_extend = p_ctrl->p_extend;
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;
    FSP_CRITICAL_SECTION_DEFINE;

    FSP_CRITICAL_SECTION_ENTER;

    rx_buffered_bytes  = BUFFERED_BYTES(p_ctrl);
    rx_available_bytes = rx_buffered_bytes;

    while (*rlen && rx_available_bytes)
    {
        *(*rbuf)++           = p_ctrl->rx_buff[p_ctrl->rx_buff_tail];
        p_ctrl->rx_buff_tail = (p_ctrl->rx_buff_tail + 1) % RM_COMMS_UART_W_RX_BUFF_SIZE;
        --(*rlen);
        --rx_available_bytes;
    }

    if ((0 != rx_buffered_bytes) && (0 == rx_available_bytes) && p_extend->buff_ovrw_prot)
    {
        ret = p_uart_api->receiveResume(p_extend->p_uart->p_ctrl);
    }

    FSP_CRITICAL_SECTION_EXIT;

    return ret;
}

/*******************************************************************************************************************//**
 * @brief Opens and configures the UART Comms module. Implements @ref rm_comms_api_t::open.
 *
 *
 * @retval FSP_SUCCESS                  UART Comms module successfully configured.
 * @retval FSP_ERR_ASSERTION            Null pointer, or one or more configuration options is invalid.
 * @retval FSP_ERR_ALREADY_OPEN         Module is already open.  This module can only be opened once.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_Open (rm_comms_ctrl_t * const p_api_ctrl, rm_comms_cfg_t const * const p_cfg)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_uart_w_instance_ctrl_t * p_ctrl = (rm_comms_uart_w_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_cfg);
    FSP_ERROR_RETURN(RM_COMMS_UART_W_OPEN != p_ctrl->open, FSP_ERR_ALREADY_OPEN);
#endif

    rm_comms_uart_w_extended_cfg_t * p_extend = (rm_comms_uart_w_extended_cfg_t *) p_cfg->p_extend;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_extend);
    FSP_ASSERT(NULL != p_extend->p_uart);
#endif

    p_ctrl->p_cfg      = p_cfg;
    p_ctrl->p_extend   = p_extend;
    p_ctrl->p_callback = p_cfg->p_callback;
    p_ctrl->p_context  = p_cfg->p_context;

    p_ctrl->rx_buff_tail = 0;
    p_ctrl->rx_buff_head = 0;
    memset(p_ctrl->rx_buff, 0, sizeof(p_ctrl->rx_buff));

    if (NULL != p_extend->p_tx_mutex)
    {
        /* Init mutex for writing */
        err = rm_comms_recursive_mutex_initialize(p_extend->p_tx_mutex);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    if (NULL != p_extend->p_rx_mutex)
    {
        /* Init mutex for reading */
        err = rm_comms_recursive_mutex_initialize(p_extend->p_rx_mutex);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    if (NULL != p_extend->p_tx_semaphore)
    {
        /* Init semaphore for writing */
        err = rm_comms_semaphore_initialize(p_extend->p_tx_semaphore);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    if (NULL != p_extend->p_rx_semaphore)
    {
        /* Init semaphore for reading */
        err = rm_comms_semaphore_initialize(p_extend->p_rx_semaphore);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    /*  Calls open function of UART HAL driver */
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;
    err = p_uart_api->open(p_extend->p_uart->p_ctrl, p_extend->p_uart->p_cfg);
    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    /* Set callback function */
    err = p_uart_api->callbackSet(p_extend->p_uart->p_ctrl, rm_comms_uart_w_callback, p_ctrl, NULL);
    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    /* Set open flag */
    p_ctrl->open = RM_COMMS_UART_W_OPEN;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Disables specified UART Comms module. Implements @ref rm_comms_api_t::close.
 *
 * @retval FSP_SUCCESS              Successfully closed.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_Close (rm_comms_ctrl_t * const p_api_ctrl)
{
    rm_comms_uart_w_instance_ctrl_t * p_ctrl = (rm_comms_uart_w_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ERROR_RETURN(RM_COMMS_UART_W_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_uart_w_extended_cfg_t const * p_extend = p_ctrl->p_extend;
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;

    p_uart_api->close(p_extend->p_uart->p_ctrl);
    if (NULL != p_extend->p_tx_mutex)
    {
        rm_comms_recursive_mutex_destroy(p_extend->p_tx_mutex);
    }

    if (NULL != p_extend->p_rx_mutex)
    {
        rm_comms_recursive_mutex_destroy(p_extend->p_rx_mutex);
    }

    if (NULL != p_extend->p_tx_semaphore)
    {
        rm_comms_semaphore_destroy(p_extend->p_tx_semaphore);
    }

    if (NULL != p_extend->p_rx_semaphore)
    {
        rm_comms_semaphore_destroy(p_extend->p_rx_semaphore);
    }

    /* Clear open flag */
    p_ctrl->open = 0;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Updates the UART Comms callback. Implements @ref rm_comms_api_t::callbackSet.
 *
 * @retval FSP_SUCCESS              Successfully set.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_CallbackSet (rm_comms_ctrl_t * const p_api_ctrl,
                                       void (                * p_callback)(rm_comms_callback_args_t *),
                                       void * const            p_context)
{
    rm_comms_uart_w_instance_ctrl_t * p_ctrl = (rm_comms_uart_w_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_callback);
    FSP_ERROR_RETURN(RM_COMMS_UART_W_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    /* Store callback and context */
    p_ctrl->p_callback = p_callback;
    p_ctrl->p_context  = p_context;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Performs a read from the UART device. Implements @ref rm_comms_api_t::read.
 *
 * @retval FSP_SUCCESS              Successfully data decoded.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_Read (rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_dest, uint32_t const bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_uart_w_instance_ctrl_t * p_ctrl = (rm_comms_uart_w_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_dest);
    FSP_ERROR_RETURN(RM_COMMS_UART_W_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_uart_w_extended_cfg_t const * p_extend = p_ctrl->p_extend;
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;

    if (NULL != p_extend->p_rx_mutex)
    {
        /* Acquire read mutex */
        err = rm_comms_recursive_mutex_acquire(p_extend->p_rx_mutex, p_extend->mutex_timeout);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    uint32_t  rx_bytes_left  = bytes;
    uint8_t * p_updated_dest = p_dest;

    err = rm_comms_uart_w_rx_buff_read(p_ctrl, &p_updated_dest, &rx_bytes_left);
    if (err != FSP_SUCCESS)
    {
        rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (0 == rx_bytes_left)
    {
        if (NULL == p_extend->p_rx_semaphore)
        {
            rm_comms_uart_w_notify_application(p_ctrl, RM_COMMS_EVENT_RX_OPERATION_COMPLETE);
        }

        if (NULL != p_extend->p_rx_mutex)
        {
            err = rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
            FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
        }

        return FSP_SUCCESS;
    }

    /* Use UART driver to read data */
    err = p_uart_api->read(p_extend->p_uart->p_ctrl, p_updated_dest, rx_bytes_left);
    if (err != FSP_SUCCESS)
    {
        rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (NULL != p_extend->p_rx_semaphore)
    {
        /* Wait for read to complete */
        err = rm_comms_semaphore_acquire(p_extend->p_rx_semaphore, p_ctrl->p_cfg->semaphore_timeout);
        if (err != FSP_SUCCESS)
        {
            p_uart_api->communicationAbort(p_extend->p_uart->p_ctrl, UART_DIR_RX);
            rm_comms_recursive_mutex_release(p_extend->p_tx_mutex);
        }

        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    if (NULL != p_extend->p_rx_mutex)
    {
        /* Release read mutex */
        err = rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Performs a write to the UART device. Implements @ref rm_comms_api_t::write.
 *
 * @retval FSP_SUCCESS              Successfully writing data .
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_Write (rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_src, uint32_t const bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_uart_w_instance_ctrl_t * p_ctrl = (rm_comms_uart_w_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_UART_W_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_src);
    FSP_ERROR_RETURN(RM_COMMS_UART_W_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_uart_w_extended_cfg_t const * p_extend = p_ctrl->p_extend;
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;

    if (NULL != p_extend->p_tx_mutex)
    {
        /* Acquire write mutex */
        err = rm_comms_recursive_mutex_acquire(p_extend->p_tx_mutex, p_extend->mutex_timeout);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    rm_comms_semaphore_acquire(p_extend->p_tx_semaphore, 0);

    /* Use UART driver to write data */
    err = p_uart_api->write(p_extend->p_uart->p_ctrl, p_src, bytes);
    if (err != FSP_SUCCESS)
    {
        rm_comms_recursive_mutex_release(p_extend->p_tx_mutex);
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (NULL != p_extend->p_tx_semaphore)
    {
        /* Wait for write to complete */
        err = rm_comms_semaphore_acquire(p_extend->p_tx_semaphore, p_ctrl->p_cfg->semaphore_timeout);
        if (err != FSP_SUCCESS)
        {
            p_uart_api->communicationAbort(p_extend->p_uart->p_ctrl, UART_DIR_TX);
            rm_comms_recursive_mutex_release(p_extend->p_tx_mutex);
        }

        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    if (NULL != p_extend->p_tx_mutex)
    {
        /* Release write mutex */
        err = rm_comms_recursive_mutex_release(p_extend->p_tx_mutex);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Performs a write to, then a read from the UART device. Implements @ref rm_comms_api_t::writeRead.
 *
 * @retval FSP_ERR_UNSUPPORTED      Not supported.
 *
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_UART_W_WriteRead (rm_comms_ctrl_t * const            p_api_ctrl,
                                     rm_comms_write_read_params_t const write_read_params)
{
    FSP_PARAMETER_NOT_USED(p_api_ctrl);
    FSP_PARAMETER_NOT_USED(write_read_params);

    return FSP_ERR_UNSUPPORTED;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup RM_COMMS_UART_W)
 **********************************************************************************************************************/

static void rm_comms_uart_w_notify_application (rm_comms_uart_w_instance_ctrl_t const * p_ctrl, rm_comms_event_t event)
{
    if (p_ctrl->p_callback)
    {
        FSP_CRITICAL_SECTION_DEFINE;
        FSP_CRITICAL_SECTION_ENTER;
        uint32_t rx_buff_len = (uint32_t) MODULO((int) (p_ctrl->rx_buff_head - p_ctrl->rx_buff_tail),
                                                 RM_COMMS_UART_W_RX_BUFF_SIZE);
        FSP_CRITICAL_SECTION_EXIT;

        rm_comms_uart_state_info_t state_info =
        {
            .rx_buff_size = RM_COMMS_UART_W_RX_BUFF_SIZE,
            .rx_buff_len  = rx_buff_len,
        };

        rm_comms_callback_args_t args =
        {
            .p_instance_args = (void *) &state_info,
            .p_context       = p_ctrl->p_context,
            .event           = event,
        };

        p_ctrl->p_callback(&args);
    }
}

/*******************************************************************************************************************//**
 * @brief Common callback function called in the UART driver callback function.
 **********************************************************************************************************************/
static void rm_comms_uart_w_callback (uart_callback_args_t * p_args)
{
    rm_comms_uart_w_instance_ctrl_t      * p_ctrl   = (rm_comms_uart_w_instance_ctrl_t *) (p_args->p_context);
    rm_comms_uart_w_extended_cfg_t const * p_extend = p_ctrl->p_extend;
    uart_api_t const * p_uart_api = p_extend->p_uart->p_api;

    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
        {
            if (NULL != p_extend->p_tx_semaphore)
            {
                rm_comms_semaphore_release(p_extend->p_tx_semaphore);
            }
            else
            {
                rm_comms_uart_w_notify_application(p_ctrl, RM_COMMS_EVENT_TX_OPERATION_COMPLETE);
            }

            break;
        }

        case UART_EVENT_RX_COMPLETE:
        {
            if (NULL != p_extend->p_rx_semaphore)
            {
                rm_comms_semaphore_release(p_extend->p_rx_semaphore);
            }
            else
            {
                rm_comms_uart_w_notify_application(p_ctrl, RM_COMMS_EVENT_RX_OPERATION_COMPLETE);
            }

            break;
        }

        case UART_EVENT_RX_CHAR:
        {
            uint32_t rx_buff_len;
            FSP_CRITICAL_SECTION_DEFINE;

            FSP_CRITICAL_SECTION_ENTER;
            p_ctrl->rx_buff[p_ctrl->rx_buff_head] = (uint8_t) p_args->data;
            p_ctrl->rx_buff_head = (p_ctrl->rx_buff_head + 1) % RM_COMMS_UART_W_RX_BUFF_SIZE;

            rx_buff_len = BUFFERED_BYTES(p_ctrl);

            if (p_extend->buff_ovrw_prot && WATERMARK_REACHED(rx_buff_len))
            {
                fsp_err_t err = p_uart_api->receiveSuspend(p_extend->p_uart->p_ctrl);

                if (FSP_SUCCESS != err)
                {
                    assert(false);
                }
            }

            FSP_CRITICAL_SECTION_EXIT;

            break;
        }

        case UART_EVENT_TX_DATA_EMPTY: // Continue Tx/Rx
        {
            break;
        }

        default:                       // Stop both Tx and Rx on UART Error
        {
            if (NULL != p_extend->p_tx_semaphore)
            {
                rm_comms_semaphore_release(p_extend->p_tx_semaphore);
            }

            if (NULL != p_extend->p_rx_semaphore)
            {
                rm_comms_semaphore_release(p_extend->p_rx_semaphore);
            }

            if ((NULL == p_extend->p_tx_semaphore) || (NULL == p_extend->p_rx_semaphore))
            {
                rm_comms_uart_w_notify_application(p_ctrl, RM_COMMS_EVENT_ERROR);
            }

            break;
        }
    }
}
