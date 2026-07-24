/**
 ****************************************************************************************
 *
 * @file cli_ble_queue.c
 *
 * @brief Managing queue between BLE / WIFI
 *
 * Copyright (c) 2016-2024 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */
#include "FreeRTOS.h"
#include "msg_queues.h"
#include "cli_ble_queue.h"

#define CLI_BLE_QUEUE_LENGTH	(10)

msg_queue g_cli_ble_queue;

void cli_ble_queue_init (void)
{
    msg_queue_create(&g_cli_ble_queue, CLI_BLE_QUEUE_LENGTH, NULL);
}

void cli_ble_queue_enqueue (cli_ble_msg_t * pmsg)
{
    if (msg_queue_send(&g_cli_ble_queue, 0, 0, (void *)pmsg, sizeof(cli_ble_msg_t), portMAX_DELAY) == errQUEUE_FULL) {
        assert(0);
    }
}

bool cli_ble_queue_dequeue (cli_ble_msg_t *pmsg, uint32_t timeout)
{
    msg queue_msg;
    
    if (msg_queue_get(&g_cli_ble_queue, &queue_msg, timeout) == errQUEUE_EMPTY) {
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

