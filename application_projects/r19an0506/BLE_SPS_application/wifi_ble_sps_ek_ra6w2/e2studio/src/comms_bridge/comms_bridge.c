/***********************************************************************************************************************
 * File Name    : rm_comms_bridge.c
 * Description  : Comms Bridge implementation
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
#include "comms_bridge.h"
#include "common_utils.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Definitions of Open flag "CBRI" */
#define RM_COMMS_BRIDGE_OPEN          (0x43425249UL)
#define RM_COMMS_BRIDGE_TASK_STACK    (1024)
#define RM_COMMS_BRIDGE_TASK_PRIO     (6)

#define MODULO(a, N)            ((a) % (N) + (N)) % (N)
#define WATERMARK_REACHED(x)    ((x) > (RM_COMMS_BRIDGE_RX_BUFF_SIZE * 4 / 5)) /* 4/5 of buffer capacity */

/** Additional operation info. It is compatible with rm_comms_ble_state_info_t and rm_comms_uart_state_info_t. */
typedef struct st_rm_comms_state_info
{
    uint32_t rx_buff_size;             ///< Internal buffer size.
    uint32_t rx_buff_len;              ///< Octets buffered.
} rm_comms_state_info_t;

/***********************************************************************************************************************
 * Externs
 **********************************************************************************************************************/

static void rm_comms_bridge_dispatch_thread_func_a(void * p_param);
static void rm_comms_bridge_dispatch_thread_func_b(void * p_param);
static void rm_comms_bridge_callback_a(rm_comms_callback_args_t * p_args);
static void rm_comms_bridge_callback_b(rm_comms_callback_args_t * p_args);

volatile uint32_t cnt_overflow = 0;
unsigned char     p_data_to_write[1024];

/*******************************************************************************************************************//**
 * @brief Opens and configures the RM_COMMS bridge module.
 *
 *
 * @retval FSP_SUCCESS                  RM_COMMS bridge module successfully configured.
 * @retval FSP_ERR_ASSERTION            Null pointer, or one or more configuration options is invalid.
 * @retval FSP_ERR_ALREADY_OPEN         Module is already open.  This module can only be opened once.
 * @retval FSP_ERR_OUT_OF_MEMORY        Insufficient memory to start bridging.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BRIDGE_Open (rm_comms_bridge_instance_ctrl_t * const p_ctrl,
                                rm_comms_bridge_cfg_t const * const     p_cfg)
{
    BaseType_t rtos_err;
    fsp_err_t  err = FSP_SUCCESS;

#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_cfg);
    FSP_ERROR_RETURN(RM_COMMS_BRIDGE_OPEN != p_ctrl->open, FSP_ERR_ALREADY_OPEN);
#endif

    memset(p_ctrl->buff_a, 0x0, sizeof(p_ctrl->buff_a));
    memset(p_ctrl->buff_b, 0x0, sizeof(p_ctrl->buff_b));

    p_ctrl->head_a          = 0;
    p_ctrl->tail_a          = 0;
    p_ctrl->head_b          = 0;
    p_ctrl->tail_b          = 0;
    p_ctrl->p_cfg           = p_cfg;
    p_ctrl->dispatch_task_b = NULL;
    p_ctrl->dispatch_task_a = NULL;
    p_ctrl->dispatch_sema_b = NULL;
    p_ctrl->dispatch_sema_a = xSemaphoreCreateBinary();

    if (NULL == p_ctrl->dispatch_sema_a)
    {
        err = FSP_ERR_OUT_OF_MEMORY;

        goto RM_COMMS_BRIDGE_Open_Error;
    }

    p_ctrl->dispatch_sema_b = xSemaphoreCreateBinary();

    if (NULL == p_ctrl->dispatch_sema_b)
    {
        err = FSP_ERR_OUT_OF_MEMORY;

        goto RM_COMMS_BRIDGE_Open_Error;
    }

    err = p_cfg->comms_instance_a->p_api->open(p_cfg->comms_instance_a->p_ctrl, p_cfg->comms_instance_a->p_cfg);
    if (FSP_SUCCESS != err)
    {
        goto RM_COMMS_BRIDGE_Open_Error;
    }

    err = p_cfg->comms_instance_b->p_api->open(p_cfg->comms_instance_b->p_ctrl, p_cfg->comms_instance_b->p_cfg);
    if (FSP_SUCCESS != err)
    {
        p_cfg->comms_instance_a->p_api->close(p_cfg->comms_instance_a->p_ctrl);

        goto RM_COMMS_BRIDGE_Open_Error;
    }

    err = p_cfg->comms_instance_a->p_api->callbackSet(p_cfg->comms_instance_a->p_ctrl,
                                                      rm_comms_bridge_callback_a,
                                                      p_ctrl);
    if (FSP_SUCCESS != err)
    {
        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    err = p_cfg->comms_instance_b->p_api->callbackSet(p_cfg->comms_instance_b->p_ctrl,
                                                      rm_comms_bridge_callback_b,
                                                      p_ctrl);
    if (FSP_SUCCESS != err)
    {
        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    /* Set open flag */
    p_ctrl->open = RM_COMMS_BRIDGE_OPEN;

    rtos_err = xTaskCreate(rm_comms_bridge_dispatch_thread_func_a,
                           "COMMS_BRIDGE Thread A",
                           RM_COMMS_BRIDGE_TASK_STACK / 4,
                           (void *) p_ctrl,
                           RM_COMMS_BRIDGE_TASK_PRIO,
                           &p_ctrl->dispatch_task_a);

    if (pdPASS != rtos_err)
    {
        err = FSP_ERR_OUT_OF_MEMORY;

        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    rtos_err = xTaskCreate(rm_comms_bridge_dispatch_thread_func_b,
                           "COMMS_BRIDGE Thread B",
                           RM_COMMS_BRIDGE_TASK_STACK / 4,
                           (void *) p_ctrl,
                           RM_COMMS_BRIDGE_TASK_PRIO,
                           &p_ctrl->dispatch_task_b);

    if (pdPASS != rtos_err)
    {
        err = FSP_ERR_OUT_OF_MEMORY;

        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    /* Initiate polling operation */
    p_ctrl->pending_bytes_a = 1;
    err = p_cfg->comms_instance_a->p_api->read(p_cfg->comms_instance_a->p_ctrl, p_ctrl->buff_a, 1);
    if (FSP_SUCCESS != err)
    {
        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    p_ctrl->pending_bytes_b = 1;
    err = p_cfg->comms_instance_b->p_api->read(p_cfg->comms_instance_b->p_ctrl, p_ctrl->buff_b, 1);
    if (FSP_SUCCESS != err)
    {
        goto RM_COMMS_BRIDGE_Open_Channel_Error;
    }

    return FSP_SUCCESS;

RM_COMMS_BRIDGE_Open_Channel_Error:

    /* Clear open flag */
    p_ctrl->open = 0;

    if (NULL != p_ctrl->dispatch_task_a)
    {
        vTaskDelete(p_ctrl->dispatch_task_a);
    }

    if (NULL != p_ctrl->dispatch_task_b)
    {
        vTaskDelete(p_ctrl->dispatch_task_b);
    }

    p_ctrl->dispatch_task_b = NULL;
    p_ctrl->dispatch_task_a = NULL;

    p_cfg->comms_instance_a->p_api->close(p_cfg->comms_instance_a->p_ctrl);
    p_cfg->comms_instance_b->p_api->close(p_cfg->comms_instance_b->p_ctrl);

RM_COMMS_BRIDGE_Open_Error:
    if (NULL != p_ctrl->dispatch_sema_a)
    {
        xSemaphoreGive(p_ctrl->dispatch_sema_a);
        vSemaphoreDelete(p_ctrl->dispatch_sema_a);
    }

    if (NULL != p_ctrl->dispatch_sema_b)
    {
        xSemaphoreGive(p_ctrl->dispatch_sema_b);
        vSemaphoreDelete(p_ctrl->dispatch_sema_b);
    }

    p_ctrl->dispatch_sema_a = NULL;
    p_ctrl->dispatch_sema_b = NULL;

    return err;
}

/*******************************************************************************************************************//**
 * @brief Disables specified RM_COMMS bridge module.
 *
 * @retval FSP_SUCCESS              Successfully closed.
 * @retval FSP_ERR_ASSERTION        Null pointer passed as a parameter.
 * @retval FSP_ERR_NOT_OPEN         Module is not open.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_COMMS_BRIDGE_Close (rm_comms_bridge_instance_ctrl_t * const p_ctrl)
{
#if RM_COMMS_BLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ERROR_RETURN(RM_COMMS_BRIDGE_OPEN == p_ctrl->open, FSP_ERR_NOT_OPEN);
#endif

    /* Clear open flag */
    p_ctrl->open = 0;

    vTaskDelete(p_ctrl->dispatch_task_a);
    vTaskDelete(p_ctrl->dispatch_task_b);

    p_ctrl->dispatch_task_b = NULL;
    p_ctrl->dispatch_task_a = NULL;

    p_ctrl->p_cfg->comms_instance_a->p_api->close(p_ctrl->p_cfg->comms_instance_a->p_ctrl);
    p_ctrl->p_cfg->comms_instance_b->p_api->close(p_ctrl->p_cfg->comms_instance_b->p_ctrl);

    xSemaphoreGive(p_ctrl->dispatch_sema_a);
    vSemaphoreDelete(p_ctrl->dispatch_sema_a);
    xSemaphoreGive(p_ctrl->dispatch_sema_b);
    vSemaphoreDelete(p_ctrl->dispatch_sema_b);

    p_ctrl->dispatch_sema_a = NULL;
    p_ctrl->dispatch_sema_b = NULL;

    return FSP_SUCCESS;
}

static void rm_comms_bridge_callback_a (rm_comms_callback_args_t * p_args)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_bridge_instance_ctrl_t * const p_ctrl = (rm_comms_bridge_instance_ctrl_t *) p_args->p_context;
    rm_comms_state_info_t const * const     p_info = (rm_comms_state_info_t *) p_args->p_instance_args;

    if (RM_COMMS_EVENT_RX_OPERATION_COMPLETE == p_args->event)
    {
        uint32_t to_read;
        uint32_t rx_buffered_bytes;

        FSP_CRITICAL_SECTION_DEFINE;

        FSP_CRITICAL_SECTION_ENTER;
        p_ctrl->head_a    = (p_ctrl->head_a + p_ctrl->pending_bytes_a) % RM_COMMS_BRIDGE_RX_BUFF_SIZE;
        rx_buffered_bytes = MODULO((int) (p_ctrl->head_a - p_ctrl->tail_a), RM_COMMS_BRIDGE_RX_BUFF_SIZE);

        /* Read all the buffered content if there is none read at least 1 byte in order to avoid stack overflow */
        to_read = MAX(MIN(RM_COMMS_BRIDGE_RX_BUFF_SIZE - p_ctrl->head_a, p_info->rx_buff_len), 1);
        FSP_CRITICAL_SECTION_EXIT;

        p_ctrl->pending_bytes_a = to_read;
        if (WATERMARK_REACHED(rx_buffered_bytes))
        {
            /* Stop reading until we finish with buffered data */
            xSemaphoreGiveFromISR(p_ctrl->dispatch_sema_a, NULL);

            return;
        }

        err = p_ctrl->p_cfg->comms_instance_a->p_api->read(p_ctrl->p_cfg->comms_instance_a->p_ctrl,
                                                           p_ctrl->buff_a + p_ctrl->head_a,
                                                           to_read);

        /* Cannot proceed bridging as one of the channels is broken - give-up. */
        assert(FSP_SUCCESS == err);

        xSemaphoreGiveFromISR(p_ctrl->dispatch_sema_a, NULL);
    }
}

static void rm_comms_bridge_callback_b (rm_comms_callback_args_t * p_args)
{
    fsp_err_t err = FSP_SUCCESS;
    rm_comms_bridge_instance_ctrl_t * const p_ctrl = (rm_comms_bridge_instance_ctrl_t *) p_args->p_context;
    rm_comms_state_info_t const * const     p_info = (rm_comms_state_info_t *) p_args->p_instance_args;

    if (RM_COMMS_EVENT_RX_OPERATION_COMPLETE == p_args->event)
    {
        uint32_t to_read;
        uint32_t rx_buffered_bytes;

        FSP_CRITICAL_SECTION_DEFINE;
        FSP_CRITICAL_SECTION_ENTER;
        p_ctrl->head_b    = (p_ctrl->head_b + p_ctrl->pending_bytes_b) % RM_COMMS_BRIDGE_RX_BUFF_SIZE;
        rx_buffered_bytes = MODULO((int) (p_ctrl->head_b - p_ctrl->tail_b), RM_COMMS_BRIDGE_RX_BUFF_SIZE);

        /* Read all the buffered content if there is none read at least 1 byte in order to avoid stack overflow */
        to_read = MAX(MIN(RM_COMMS_BRIDGE_RX_BUFF_SIZE - p_ctrl->head_b, p_info->rx_buff_len), 1);
        FSP_CRITICAL_SECTION_EXIT;

        p_ctrl->pending_bytes_b = to_read;
        if (WATERMARK_REACHED(rx_buffered_bytes))
        {
            /* Stop reading until we finish with buffered data */
            xSemaphoreGiveFromISR(p_ctrl->dispatch_sema_b, NULL);

            return;
        }

        err = p_ctrl->p_cfg->comms_instance_b->p_api->read(p_ctrl->p_cfg->comms_instance_b->p_ctrl,
                                                           p_ctrl->buff_b + p_ctrl->head_b,
                                                           to_read);

        /* Cannot proceed bridging as one of the channels is broken - give-up. */
        assert(FSP_SUCCESS == err);

        xSemaphoreGiveFromISR(p_ctrl->dispatch_sema_b, NULL);
    }
}

static void rm_comms_bridge_dispatch_thread_func_a (void * p_param)
{
    rm_comms_bridge_instance_ctrl_t * const p_ctrl = (rm_comms_bridge_instance_ctrl_t *) p_param;

    while (1)
    {
        /* Wait for the external event */
        xSemaphoreTake(p_ctrl->dispatch_sema_a, portMAX_DELAY);

        if (RM_COMMS_BRIDGE_OPEN != p_ctrl->open)
        {
            /* We've closed the bridge - stop. */
            break;
        }

        FSP_CRITICAL_SECTION_DEFINE;

        /* Process external event */
        FSP_CRITICAL_SECTION_ENTER;
        uint32_t data_idx          = 0;
        uint32_t rx_buffered_bytes = MODULO((int) (p_ctrl->head_a - p_ctrl->tail_a), RM_COMMS_BRIDGE_RX_BUFF_SIZE);
#if 0
        unsigned char * p_data = pvPortMalloc(rx_buffered_bytes);

        if (NULL == p_data)
        {
            FSP_CRITICAL_SECTION_EXIT;

            APP_PRINT("%s:%d Unable to allocate memory for the transfer.\n", __FUNCTION__, __LINE__);

            continue;
        }

#else
        if (rx_buffered_bytes > 1024)
        {
            FSP_CRITICAL_SECTION_EXIT;
            cnt_overflow++;
            continue;
        }
#endif

        for (data_idx = 0; data_idx < rx_buffered_bytes; data_idx++)
        {
            p_data_to_write[data_idx] = p_ctrl->buff_a[p_ctrl->tail_a];
            p_ctrl->tail_a            = (p_ctrl->tail_a + 1) % RM_COMMS_BRIDGE_RX_BUFF_SIZE;
        }

        FSP_CRITICAL_SECTION_EXIT;

        /* Transfer buffered data */
        fsp_err_t err = p_ctrl->p_cfg->comms_instance_b->p_api->write(p_ctrl->p_cfg->comms_instance_b->p_ctrl,
                                                                      p_data_to_write,
                                                                      rx_buffered_bytes);

        if (FSP_SUCCESS != err)
        {
            APP_PRINT("%s:%d Error transferring the data.\n", __FUNCTION__, __LINE__);
        }

        if (WATERMARK_REACHED(rx_buffered_bytes))
        {
            /* Resume reading the data */
            err = p_ctrl->p_cfg->comms_instance_a->p_api->read(p_ctrl->p_cfg->comms_instance_a->p_ctrl,
                                                               p_ctrl->buff_a + p_ctrl->head_a,
                                                               p_ctrl->pending_bytes_a);

            if (FSP_SUCCESS != err)
            {
                APP_PRINT("%s:%d Error during resume of reading the data.\n", __FUNCTION__, __LINE__);
            }
        }

#if 0
        vPortFree(p_data);
#endif
    }
}

static void rm_comms_bridge_dispatch_thread_func_b (void * p_param)
{
    rm_comms_bridge_instance_ctrl_t * const p_ctrl = (rm_comms_bridge_instance_ctrl_t *) p_param;
    fsp_err_t err = FSP_SUCCESS;
    while (1)
    {
        /* Wait for the external event */
        xSemaphoreTake(p_ctrl->dispatch_sema_b, portMAX_DELAY);

        if (RM_COMMS_BRIDGE_OPEN != p_ctrl->open)
        {
            /* We've closed the bridge - stop. */
            break;
        }

        FSP_CRITICAL_SECTION_DEFINE;

        /* Process external event */
        FSP_CRITICAL_SECTION_ENTER;

        uint32_t data_idx          = 0;
        uint32_t rx_buffered_bytes =
            MODULO((int) (p_ctrl->head_b - p_ctrl->tail_b), RM_COMMS_BRIDGE_RX_BUFF_SIZE);
        unsigned char * p_data = pvPortMalloc(rx_buffered_bytes);

        if (NULL == p_data)
        {
            FSP_CRITICAL_SECTION_EXIT;

            APP_PRINT("%s:%d Unable to allocate memory for the transfer.\n", __FUNCTION__, __LINE__);

            continue;
        }

        for (data_idx = 0; data_idx < rx_buffered_bytes; data_idx++)
        {
            p_data[data_idx] = p_ctrl->buff_b[p_ctrl->tail_b];
            p_ctrl->tail_b   = (p_ctrl->tail_b + 1) % RM_COMMS_BRIDGE_RX_BUFF_SIZE;
        }

        FSP_CRITICAL_SECTION_EXIT;
        if (sps_flow_enabled == true)
        {
            /* Transfer buffered data */
            err = p_ctrl->p_cfg->comms_instance_a->p_api->write(p_ctrl->p_cfg->comms_instance_a->p_ctrl,
                                                                p_data,
                                                                rx_buffered_bytes);
        }

        if (FSP_SUCCESS != err)
        {
            APP_PRINT("%s:%d Error transferring the data.\n", __FUNCTION__, __LINE__);
        }

        if (WATERMARK_REACHED(rx_buffered_bytes))
        {
            /* Resume reading the data */
            err = p_ctrl->p_cfg->comms_instance_b->p_api->read(p_ctrl->p_cfg->comms_instance_b->p_ctrl,
                                                               p_ctrl->buff_b + p_ctrl->head_b,
                                                               p_ctrl->pending_bytes_b);

            if (FSP_SUCCESS != err)
            {
                APP_PRINT("%s:%d Error during resume of reading the data.\n", __FUNCTION__, __LINE__);
            }
        }

        vPortFree(p_data);
    }
}
