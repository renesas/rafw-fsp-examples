/***********************************************************************************************************************
 * File Name    : rm_ble_msg_queue.c
 * Description  : BLE MSG Queue
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include "FreeRTOS.h"
#include "queue.h"
#include "interrupts.h"
#include "ble_msg_queue.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define RM_BLE_MSG_QUEUE_LENGTH    (20)

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

static StaticQueue_t          g_ble_msg_queue_info;
static uint8_t                g_ble_msg_queue_data[RM_BLE_MSG_QUEUE_LENGTH * sizeof(rm_ble_msg_t)];
static volatile QueueHandle_t g_ble_msg_queue;

/***********************************************************************************************************************
 * Externs
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Initialize BLE message queue.
 *
 * @retval FSP_SUCCESS              BLE message queue successfully configured.
 * @retval FSP_ERR_ALREADY_OPEN     BLE message queue already initialized.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_BLE_MSG_QUEUE_Init (void)
{
    if (NULL != g_ble_msg_queue)
    {
        return FSP_ERR_ALREADY_OPEN;
    }

    g_ble_msg_queue = xQueueCreateStatic(RM_BLE_MSG_QUEUE_LENGTH,
                                         sizeof(rm_ble_msg_t),
                                         g_ble_msg_queue_data,
                                         &g_ble_msg_queue_info);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Enqueues message to the BLE message queue.
 *
 * @retval FSP_SUCCESS              Successfully enqueued.
 * @retval FSP_ERR_NOT_OPEN         BLE message queue not initialized.
 * @retval FSP_ERR_OUT_OF_MEMORY    No more free room in the queue.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_BLE_MSG_QUEUE_Enqueue (const rm_ble_msg_t * p_msg)
{
    BaseType_t ret = pdPASS;

    if (NULL == g_ble_msg_queue)
    {
        return FSP_ERR_NOT_OPEN;
    }

    if (in_interrupt())
    {
        ret = xQueueSendToBackFromISR(g_ble_msg_queue, p_msg, NULL);
    }
    else
    {
        ret = xQueueSendToBack(g_ble_msg_queue, p_msg, portMAX_DELAY);
    }

    if (errQUEUE_FULL == ret)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Push message to the from of BLE message queue.
 *
 * @retval FSP_SUCCESS              Successfully pushed.
 * @retval FSP_ERR_OUT_OF_MEMORY    No more free room in the queue.
 * @retval FSP_ERR_NOT_OPEN         BLE message queue not initialized.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_BLE_MSG_QUEUE_Push (const rm_ble_msg_t * p_msg)
{
    BaseType_t ret = pdPASS;

    if (NULL == g_ble_msg_queue)
    {
        return FSP_ERR_NOT_OPEN;
    }

    if (in_interrupt())
    {
        ret = xQueueSendToFrontFromISR(g_ble_msg_queue, p_msg, NULL);
    }
    else
    {
        ret = xQueueSendToFront(g_ble_msg_queue, p_msg, portMAX_DELAY);
    }

    if (errQUEUE_FULL == ret)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Dequeues message from the BLE message queue.
 *
 * @retval FSP_SUCCESS              Successfully dequeued.
 * @retval FSP_ERR_NOT_OPEN         BLE message queue not initialized.
 * @retval FSP_ERR_NOT_FOUND        Not found available element.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_BLE_MSG_QUEUE_Dequeue (rm_ble_msg_t * p_msg)
{
    BaseType_t ret = pdPASS;

    if (NULL == g_ble_msg_queue)
    {
        return FSP_ERR_NOT_OPEN;
    }

    if (in_interrupt())
    {
        ret = xQueueReceiveFromISR(g_ble_msg_queue, p_msg, NULL);
    }
    else
    {
        ret = xQueueReceive(g_ble_msg_queue, p_msg, 0);
    }

    if (pdFAIL == ret)
    {
        return FSP_ERR_NOT_FOUND;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Closes the BLE message queue.
 *
 * @retval FSP_SUCCESS              BLE message queue successfully closed.
 * @retval FSP_ERR_NOT_OPEN         BLE message queue not initialized.
 *
 * @return See @ref RENESAS_ERROR_CODES or functions called by this function for other possible return codes.
 **********************************************************************************************************************/
fsp_err_t RM_BLE_MSG_QUEUE_Uninit (void)
{
    if (NULL == g_ble_msg_queue)
    {
        return FSP_ERR_NOT_OPEN;
    }

    vQueueDelete(g_ble_msg_queue);

    g_ble_msg_queue = NULL;

    return FSP_SUCCESS;
}
