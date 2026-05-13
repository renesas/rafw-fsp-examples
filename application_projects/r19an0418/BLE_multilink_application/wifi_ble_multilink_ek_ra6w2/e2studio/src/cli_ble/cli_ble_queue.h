/***********************************************************************************************************************
 * File Name    : cli_ble_queue.h
 * Description  : Header file for queues and threads definitions.
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef CLI_BLE_QUEUE_H_
#define CLI_BLE_QUEUE_H_

#include "cli_ble_msg.h"

void cli_ble_queue_init(void);
void cli_ble_queue_enqueue(cli_ble_msg_t * pmsg);
bool cli_ble_queue_dequeue(cli_ble_msg_t * pmsg);
void cli_ble_queue_uninit(void);

#endif                                 // CLI_BLE_QUEUE_H_
