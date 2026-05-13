/***********************************************************************************************************************
 * File Name    : rm_comms_ble.c
 * Description  : BLE Comms implementation
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include <sys/param.h>
#include "../rm_comms_lock/rm_comms_lock.h"
#include "ble_msg_queue.h"
#include "event_groups.h"
#include "comms_ble.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Definitions of Open flag "C" */
#define RM_COMMS_BLE_OPEN    (0x43424C45UL)
#define RM_COMMS_BLE_MTU     (244)
#define BLE_EVENT_PATTERN    (0x0A0A)
#define MODULO(a, N)            ((a) % (N) + (N)) % (N)
#define WATERMARK_REACHED(x)    ((x) > (RM_COMMS_BLE_RX_BUFF_SIZE * 3 / 4)) /* 3/4 of buffer capacity */

/***********************************************************************************************************************
 * Externs
 **********************************************************************************************************************/

extern EventGroupHandle_t g_ble_event_group_handle;

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static void rm_comms_ble_notify_application(rm_comms_ble_instance_ctrl_t const * p_ctrl,
                                            rm_comms_event_t                     event);
static void rm_comms_ble_handle_rx_error(rm_comms_ble_instance_ctrl_t * const p_ctrl, fsp_err_t err);
static void rm_comms_ble_handle_tx_error(rm_comms_ble_instance_ctrl_t * const p_ctrl, fsp_err_t err);
static void rm_comms_ble_rx_buff_read(rm_comms_ble_instance_ctrl_t * p_ctrl);
static void rm_comms_ble_rx_buff_write(rm_comms_ble_instance_ctrl_t * p_ctrl,
                                       const uint8_t                * p_data,
                                       uint8_t                        len);
static inline uint32_t rm_comms_ble_rx_buff_occupation_size(const rm_comms_ble_instance_ctrl_t * p_ctrl);
static fsp_err_t       rm_comms_ble_write(rm_comms_ble_instance_ctrl_t * p_ctrl,
                                          const uint8_t                * p_src,
                                          uint32_t const                 bytes);
static fsp_err_t rm_comms_ble_read(rm_comms_ble_instance_ctrl_t * p_ctrl, const uint8_t * p_data, uint8_t len);

/***********************************************************************************************************************
 * Global variables
 **********************************************************************************************************************/
rm_comms_api_t const g_comms_on_comms_ble =
{
    .open        = RM_COMMS_BLE_Open,
    .read        = RM_COMMS_BLE_Read,
    .write       = RM_COMMS_BLE_Write,
    .writeRead   = RM_COMMS_BLE_WriteRead,
    .callbackSet = RM_COMMS_BLE_CallbackSet,
    .close       = RM_COMMS_BLE_Close,
};

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

static inline uint32_t rm_comms_ble_rx_buff_occupation_size (const rm_comms_ble_instance_ctrl_t * p_ctrl)
{
    return MODULO((int) (p_ctrl->rx_buff_head - p_ctrl->rx_buff_tail), RM_COMMS_BLE_RX_BUFF_SIZE);
}

static void rm_comms_ble_rx_buff_read (rm_comms_ble_instance_ctrl_t * p_ctrl)
{
    uint32_t rx_buffered_bytes = rm_comms_ble_rx_buff_occupation_size(p_ctrl);

    while (p_ctrl->rx_bytes_pending && rx_buffered_bytes)
    {
        *p_ctrl->p_dest++    = p_ctrl->rx_buff[p_ctrl->rx_buff_tail];
        p_ctrl->rx_buff_tail = (p_ctrl->rx_buff_tail + 1) % RM_COMMS_BLE_RX_BUFF_SIZE;
        p_ctrl->rx_bytes_pending--;
        rx_buffered_bytes--;
    }
}

static void rm_comms_ble_rx_buff_write (rm_comms_ble_instance_ctrl_t * p_ctrl, const uint8_t * p_data, uint8_t len)
{
    uint32_t to_copy = MIN(RM_COMMS_BLE_RX_BUFF_SIZE - p_ctrl->rx_buff_head, len);
    memcpy(p_ctrl->rx_buff + p_ctrl->rx_buff_head, p_data, to_copy);
    p_data += to_copy;
    to_copy = len - to_copy;
    memcpy(p_ctrl->rx_buff, p_data, to_copy);
    p_ctrl->rx_buff_head = (p_ctrl->rx_buff_head + len) % RM_COMMS_BLE_RX_BUFF_SIZE;
}

/*******************************************************************************************************************//**
 * @brief Opens and configures the BLE Comms module. Implements @ref rm_comms_api_t::open.
 *
 *
 * @retval FSP_SUCCESS                  BLE Comms module successfully configured.
 * @retval FSP_ERR_ASSERTION            Null pointer, or one or more configuration options is invalid.
 * @retval FSP_ERR_ALREADY_OPEN         Module is already open.  This module can only be opened once.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_Open (rm_comms_ctrl_t * const p_api_ctrl, rm_comms_cfg_t const * const p_cfg)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_ble_instance_ctrl_t * p_ctrl = (rm_comms_ble_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_cfg);
    FSP_ERROR_RETURN(RM_COMMS_BLE_OPEN != p_ctrl->open, FSP_ERR_ALREADY_OPEN);
#endif

    rm_comms_ble_extended_cfg_t * p_extend = (rm_comms_ble_extended_cfg_t *) p_cfg->p_extend;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_extend);
#endif

    p_ctrl->p_cfg      = p_cfg;
    p_ctrl->p_extend   = p_extend;
    p_ctrl->p_callback = p_cfg->p_callback;
    p_ctrl->p_context  = p_cfg->p_context;

    p_ctrl->rx_buff_tail = 0;
    p_ctrl->rx_buff_head = 0;
    memset(p_ctrl->rx_buff, 0, sizeof(p_ctrl->rx_buff));

    p_ctrl->p_dest           = 0;
    p_ctrl->p_src            = 0;
    p_ctrl->rx_bytes_pending = 0;
    p_ctrl->tx_bytes_pending = 0;

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

    /* Set open flag */
    p_ctrl->open = RM_COMMS_BLE_OPEN;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Disables specified BLE Comms module. Implements @ref rm_comms_api_t::close.
 *
 * @retval FSP_SUCCESS              Successfully closed.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_Close (rm_comms_ctrl_t * const p_api_ctrl)
{
    rm_comms_ble_instance_ctrl_t * p_ctrl = (rm_comms_ble_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ERROR_RETURN(RM_COMMS_BLE_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

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
        /* Release pending transmission */
        rm_comms_semaphore_release(p_extend->p_tx_semaphore);
        rm_comms_semaphore_destroy(p_extend->p_tx_semaphore);
    }

    if (NULL != p_extend->p_rx_semaphore)
    {
        /* Release pending reception */
        rm_comms_semaphore_release(p_extend->p_rx_semaphore);
        rm_comms_semaphore_destroy(p_extend->p_rx_semaphore);
    }

    /* Clear open flag */
    p_ctrl->open = 0;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Updates the BLE Comms callback. Implements @ref rm_comms_api_t::callbackSet.
 *
 * @retval FSP_SUCCESS              Successfully set.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_CallbackSet (rm_comms_ctrl_t * const p_api_ctrl,
                                    void (                * p_callback)(rm_comms_callback_args_t *),
                                    void const * const      p_context)
{
    rm_comms_ble_instance_ctrl_t * p_ctrl = (rm_comms_ble_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_callback);
    FSP_ERROR_RETURN(RM_COMMS_BLE_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    /* Store callback and context */
    p_ctrl->p_callback = p_callback;
    p_ctrl->p_context  = p_context;

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Performs a read from the BLE device. Implements @ref rm_comms_api_t::read.
 *
 * @retval FSP_SUCCESS              Successfully data decoded.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_Read (rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_dest, uint32_t const bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_ble_instance_ctrl_t * p_ctrl = (rm_comms_ble_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_dest);
    FSP_ERROR_RETURN(RM_COMMS_BLE_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

    if (NULL != p_extend->p_rx_mutex)
    {
        /* Acquire read mutex */
        err = rm_comms_recursive_mutex_acquire(p_extend->p_rx_mutex, p_extend->mutex_timeout);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    p_ctrl->rx_bytes_pending = bytes;
    p_ctrl->p_dest           = p_dest;

    /* Try to read buffered data */
    err = rm_comms_ble_read(p_ctrl, NULL, 0);
    if (err != FSP_SUCCESS)
    {
        p_ctrl->rx_bytes_pending = 0;
        rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

    if (0 == p_ctrl->rx_bytes_pending)
    {
        if (NULL == p_extend->p_rx_semaphore)
        {
            rm_comms_ble_notify_application(p_ctrl, RM_COMMS_EVENT_RX_OPERATION_COMPLETE);
        }

        if (NULL != p_extend->p_rx_mutex)
        {
            err = rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
            FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
        }

        return FSP_SUCCESS;
    }

    if (NULL != p_extend->p_rx_semaphore)
    {
        /* Wait for read to complete */
        err = rm_comms_semaphore_acquire(p_extend->p_rx_semaphore, p_ctrl->p_cfg->semaphore_timeout);
        if (err != FSP_SUCCESS)
        {
            p_ctrl->rx_bytes_pending = 0;
            rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
        }

        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

        /* Check receive status */
        err = (fsp_err_t) p_ctrl->rx_bytes_pending;
        if (err != FSP_SUCCESS)
        {
            p_ctrl->rx_bytes_pending = 0;
            rm_comms_recursive_mutex_release(p_extend->p_rx_mutex);
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
 * @brief Performs a write to the BLE device. Implements @ref rm_comms_api_t::write.
 *
 * @retval FSP_SUCCESS              Successfully writing data .
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_Write (rm_comms_ctrl_t * const p_api_ctrl, uint8_t * const p_src, uint32_t const bytes)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_ble_instance_ctrl_t * p_ctrl = (rm_comms_ble_instance_ctrl_t *) p_api_ctrl;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_src);
    FSP_ERROR_RETURN(RM_COMMS_BLE_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

    if (NULL != p_extend->p_tx_mutex)
    {
        /* Acquire write mutex */
        err = rm_comms_recursive_mutex_acquire(p_extend->p_tx_mutex, p_extend->mutex_timeout);
        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    err = rm_comms_ble_write(p_ctrl, p_src, bytes);
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
            rm_comms_recursive_mutex_release(p_extend->p_tx_mutex);
        }

        FSP_ERROR_RETURN(FSP_SUCCESS == err, err);

        /* Check transmit status */
        err = (fsp_err_t) p_ctrl->tx_bytes_pending;
        if (err != FSP_SUCCESS)
        {
            p_ctrl->tx_bytes_pending = 0;
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
 * @brief Performs a write to, then a read from the BLE device. Implements @ref rm_comms_api_t::writeRead.
 *
 * @retval FSP_ERR_UNSUPPORTED      Not supported.
 *
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BLE_WriteRead (rm_comms_ctrl_t * const            p_api_ctrl,
                                  rm_comms_write_read_params_t const write_read_params)
{
    FSP_PARAMETER_NOT_USED(p_api_ctrl);
    FSP_PARAMETER_NOT_USED(write_read_params);

    return FSP_ERR_UNSUPPORTED;
}

static fsp_err_t rm_comms_ble_read (rm_comms_ble_instance_ctrl_t * p_ctrl, const uint8_t * p_data, uint8_t len)
{
    bool      watermark_before = false;
    bool      watermark_after  = false;
    fsp_err_t ret              = FSP_SUCCESS;

    FSP_CRITICAL_SECTION_DEFINE;
    FSP_CRITICAL_SECTION_ENTER;

    watermark_before = WATERMARK_REACHED(rm_comms_ble_rx_buff_occupation_size(p_ctrl));

    /* Try to fill requested data from the local buffer */
    rm_comms_ble_rx_buff_read(p_ctrl);

    if ((NULL == p_data) || (0 == len))
    {
        /* No more data available for now */
        goto Exit;
    }

    /* Try to fill requested data from the incoming buffer */
    if (0 != p_ctrl->rx_bytes_pending)
    {
        uint8_t len2 = MIN(p_ctrl->rx_bytes_pending, len);

        memcpy(p_ctrl->p_dest, p_data, len2);

        p_ctrl->p_dest           += len2;
        p_ctrl->rx_bytes_pending -= len2;
        len    -= len2;
        p_data += len2;
    }

    /* Buffer unused data */
    if (0 != len)
    {
        rm_comms_ble_rx_buff_write(p_ctrl, p_data, len);
    }

Exit:

    /* Calculate watermark after the read operation in order to know whether SPS_FLOW_CTRL_STATE should be updated. */
    watermark_after = WATERMARK_REACHED(rm_comms_ble_rx_buff_occupation_size(p_ctrl));

    if (watermark_before == watermark_after)
    {
        /* No need to update SPS_FLOW_CTRL_STATE */

        FSP_CRITICAL_SECTION_EXIT;

        return ret;
    }

    /* SPS_FLOW_CTRL_STATE has changed. Need to update it on the remote */
    rm_ble_msg_t * p_flow_ctrl_msg = pvPortMalloc(sizeof(rm_ble_msg_t));

    if (NULL != p_flow_ctrl_msg)
    {
        /* Perform BLE flow control signaling */
        p_flow_ctrl_msg->type = RM_BLE_MSG_TYPE_FLOW_CTRL_NOTIFY;
        if (watermark_after)
        {
            p_flow_ctrl_msg->data.flow_ctrl_data = BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_ON;
        }
        else
        {
            p_flow_ctrl_msg->data.flow_ctrl_data = BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_OFF;
        }

        ret = RM_BLE_MSG_QUEUE_Enqueue(p_flow_ctrl_msg);

        xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);

        vPortFree(p_flow_ctrl_msg);
    }
    else
    {
        ret = FSP_ERR_OUT_OF_MEMORY;
    }

    FSP_CRITICAL_SECTION_EXIT;

    if (FSP_SUCCESS != ret)
    {
        p_ctrl->p_dest = NULL;

        rm_comms_ble_handle_rx_error(p_ctrl, ret);
    }

    return ret;
}

static fsp_err_t rm_comms_ble_write (rm_comms_ble_instance_ctrl_t * p_ctrl, const uint8_t * p_src, uint32_t const bytes)
{
    fsp_err_t      ret      = FSP_SUCCESS;
    rm_ble_msg_t * p_tx_msg = pvPortMalloc(sizeof(rm_ble_msg_t));
    uint32_t       to_send  = MIN(RM_COMMS_BLE_MTU, bytes);

    if (NULL != p_tx_msg)
    {
        p_tx_msg->type                = RM_BLE_MSG_TYPE_TX_NOTIFY;
        p_tx_msg->data.tx_data.tx_len = (uint8_t) to_send;
        memset(p_tx_msg->data.tx_data.tx_data, 0x0, sizeof(p_tx_msg->data.tx_data.tx_data));
        memcpy(p_tx_msg->data.tx_data.tx_data, p_src, to_send);

        ret = RM_BLE_MSG_QUEUE_Enqueue(p_tx_msg);

        xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);

        vPortFree(p_tx_msg);
    }
    else
    {
        ret = FSP_ERR_OUT_OF_MEMORY;
    }

    if (FSP_SUCCESS != ret)
    {
        p_ctrl->p_src = NULL;

        rm_comms_ble_handle_tx_error(p_ctrl, ret);

        return ret;
    }

    p_ctrl->p_src            = p_src + to_send;
    p_ctrl->tx_bytes_pending = bytes - to_send;

    return ret;
}

static void rm_comms_ble_handle_rx_error (rm_comms_ble_instance_ctrl_t * const p_ctrl, fsp_err_t err)
{
    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

    if (NULL != p_extend->p_rx_semaphore)
    {
        p_ctrl->rx_bytes_pending = (uint32_t) err;
        rm_comms_semaphore_release(p_extend->p_rx_semaphore);
    }
    else
    {
        p_ctrl->rx_bytes_pending = 0;
        rm_comms_ble_notify_application(p_ctrl, RM_COMMS_EVENT_ERROR);
    }
}

static void rm_comms_ble_handle_tx_error (rm_comms_ble_instance_ctrl_t * const p_ctrl, fsp_err_t err)
{
    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

    if (NULL != p_extend->p_tx_semaphore)
    {
        p_ctrl->tx_bytes_pending = (uint32_t) err;
        rm_comms_semaphore_release(p_extend->p_tx_semaphore);
    }
    else
    {
        p_ctrl->tx_bytes_pending = 0;
        rm_comms_ble_notify_application(p_ctrl, RM_COMMS_EVENT_ERROR);
    }
}

static void rm_comms_ble_notify_application (rm_comms_ble_instance_ctrl_t const * p_ctrl, rm_comms_event_t event)
{
    if (p_ctrl->p_callback)
    {
        FSP_CRITICAL_SECTION_DEFINE;

        FSP_CRITICAL_SECTION_ENTER;
        uint32_t rx_buff_len = rm_comms_ble_rx_buff_occupation_size(p_ctrl);
        FSP_CRITICAL_SECTION_EXIT;

        rm_comms_ble_state_info_t state_info =
        {
            .rx_buff_size = RM_COMMS_BLE_RX_BUFF_SIZE,
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
 * @brief Common callback function called in the BLE driver callback function.
 **********************************************************************************************************************/
void rm_comms_ble_callback (rm_comms_ble_event_t event, const uint8_t * p_data, uint8_t len, void * p_context)
{
    rm_comms_ble_instance_ctrl_t      * p_ctrl   = (rm_comms_ble_instance_ctrl_t *) p_context;
    rm_comms_ble_extended_cfg_t const * p_extend = p_ctrl->p_extend;

    switch (event)
    {
        case RM_COMMS_BLE_EVENT_TX_COMPLETE:
        {
            if (0 == p_ctrl->tx_bytes_pending)
            {
                if (NULL != p_extend->p_tx_semaphore)
                {
                    rm_comms_semaphore_release(p_extend->p_tx_semaphore);
                }
                else
                {
                    rm_comms_ble_notify_application(p_ctrl, RM_COMMS_EVENT_TX_OPERATION_COMPLETE);
                }
            }
            else
            {
                rm_comms_ble_write(p_ctrl, p_ctrl->p_src, p_ctrl->tx_bytes_pending);
            }

            break;
        }

        case RM_COMMS_BLE_EVENT_DATA_AVAILABLE:
        {
            fsp_err_t ret;
            uint32_t  rx_bytes_pending = p_ctrl->rx_bytes_pending;

            ret = rm_comms_ble_read(p_ctrl, p_data, len);

            /* Check whether we finished with the read request */
            if ((FSP_SUCCESS == ret) && (0 != rx_bytes_pending) && (0 == p_ctrl->rx_bytes_pending))
            {
                if (NULL != p_extend->p_rx_semaphore)
                {
                    rm_comms_semaphore_release(p_extend->p_rx_semaphore);
                }
                else
                {
                    rm_comms_ble_notify_application(p_ctrl, RM_COMMS_EVENT_RX_OPERATION_COMPLETE);
                }
            }

            break;
        }

        case RM_COMMS_BLE_EVENT_TX_ERROR:
        {
            p_ctrl->p_src = NULL;

            rm_comms_ble_handle_tx_error(p_ctrl, FSP_ERR_WRITE_FAILED);

            break;
        }

        case RM_COMMS_BLE_EVENT_FLOW_CTRL_ERROR:
        {
            /* Trying to send again actual SPS_FLOW_CTRL_STATE state. */
            fsp_err_t      ret             = FSP_SUCCESS;
            rm_ble_msg_t * p_flow_ctrl_msg = pvPortMalloc(sizeof(rm_ble_msg_t));

            if (NULL != p_flow_ctrl_msg)
            {
                /* Perform BLE flow control signaling */
                p_flow_ctrl_msg->type = RM_BLE_MSG_TYPE_FLOW_CTRL_NOTIFY;
                if (WATERMARK_REACHED(rm_comms_ble_rx_buff_occupation_size(p_ctrl)))
                {
                    p_flow_ctrl_msg->data.flow_ctrl_data = BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_ON;
                }
                else
                {
                    p_flow_ctrl_msg->data.flow_ctrl_data = BLE_SPS_SERVICES_SPS_FLOW_CTRL_STATE_OFF;
                }

                ret = RM_BLE_MSG_QUEUE_Enqueue(p_flow_ctrl_msg);

                xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);

                vPortFree(p_flow_ctrl_msg);
            }
            else
            {
                ret = FSP_ERR_OUT_OF_MEMORY;
            }

            if (FSP_SUCCESS != ret)
            {
                p_ctrl->p_dest = NULL;

                rm_comms_ble_handle_rx_error(p_ctrl, ret);
            }

            break;
        }

        default:                       /* Some bad things happened */
        {
            rm_comms_ble_handle_tx_error(p_ctrl, FSP_ERR_INVALID_STATE);
            rm_comms_ble_handle_rx_error(p_ctrl, FSP_ERR_INVALID_STATE);

            break;
        }
    }
}
