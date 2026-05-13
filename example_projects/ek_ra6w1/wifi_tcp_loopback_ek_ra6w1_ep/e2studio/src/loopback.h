/***********************************************************************************************************************
 * File Name    : loopback.h
 * Description  : Contains functions prototypes defined in loopback.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef LOOPBACK_H_
#define LOOPBACK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "common_utils.h"

void tcp_server_task(void *pvParameters);
void tcp_client_task(void *pvParameters);
void client_handler_task(void *pvParameters);
void print_ep_info();

#endif //LOOPBACK_H_
