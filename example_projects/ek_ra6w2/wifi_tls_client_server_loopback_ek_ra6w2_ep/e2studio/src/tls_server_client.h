/***********************************************************************************************************************
 * File Name    : tls_server_client.h
 * Description  : Contains functions prototypes defined in tls_server.c tls_client.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "common_utils.h"
#include "lwip/err.h"

#define TLS_PORT 8443

err_t start_tls_server(void);
void tls_server_task(void *pvParameters);
void tls_client_init(void);
