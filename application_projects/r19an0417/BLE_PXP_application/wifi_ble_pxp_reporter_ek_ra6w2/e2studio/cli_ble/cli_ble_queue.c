/***********************************************************************************************************************
 * File Name    : cli_ble_queue.c
 * Description  : Managing queue between BLE / WIFI
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include "FreeRTOS.h"
#include "msg_queues.h"
#include "cli_ble_queue.h"

#define CLI_BLE_QUEUE_LENGTH    (10)

msg_queue g_cli_ble_queue;

void cli_ble_queue_init (void)
{
    msg_queue_create(&g_cli_ble_queue, CLI_BLE_QUEUE_LENGTH, NULL);
}

void cli_ble_queue_enqueue (cli_ble_msg_t * pmsg)
{
    if (msg_queue_send(&g_cli_ble_queue, 0, 0, (void *) pmsg, sizeof(cli_ble_msg_t), portMAX_DELAY) == errQUEUE_FULL)
    {
        assert(0);
    }
}

bool cli_ble_queue_dequeue (cli_ble_msg_t * pmsg)
{
    msg queue_msg;

    if (msg_queue_get(&g_cli_ble_queue, &queue_msg, 0) == errQUEUE_EMPTY)
    {
        return false;
    }

    memcpy(pmsg, queue_msg.data, sizeof(*pmsg));

    msg_release(&queue_msg);

    return true;
}

void cli_ble_queue_uninit (void)
{
    msg_queue_delete(&g_cli_ble_queue);
}
