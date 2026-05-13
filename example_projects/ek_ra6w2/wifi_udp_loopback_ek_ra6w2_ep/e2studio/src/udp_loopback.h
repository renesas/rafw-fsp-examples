/***********************************************************************************************************************
 * File Name    : udp_loopback.h
 * Description  : Contains functions prototypes defined in udp_server.c udp_client.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef UDP_SAMPLE_H
#define UDP_SAMPLE_H

#include "FreeRTOS.h"
#include "task.h"
#include "common_utils.h"

#define TASK_STACK_SIZE 1024
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define SVR_RCV_PRIORITY (tskIDLE_PRIORITY + 1)
#define SVR_SEND_PRIORITY (tskIDLE_PRIORITY + 1)

#define MAX_DATA_COUNT 1

/*
 * server info
 */
#define UDP_SERVER_IP "127.0.0.1"
#define UDP_SERVER_PORT 12345
#define UDP_SERVER_BUFFER_SIZE 256
#define QUEUE_SIZE 10

#define UDP_CLIENT_BUFFER_SIZE 256
#define CLIENT_TIMEOUT_MS 5000

int udp_server_init(void);
void udp_client1_init(void);
void udp_client2_init(void);

#endif //UDP_SAMPLE_H
