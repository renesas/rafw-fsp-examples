/***********************************************************************************************************************
 * File Name    : https_server_client.h
 * Description  : Contains functions prototypes defined in https_server_client.c and app_main_entry.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef HTTPS_SERVER_CLIENT_H
#define HTTPS_SERVER_CLIENT_H

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define WIFI_STATUS_EVENT 1
#define HTTPS_SERVER_RECV_EVENT 2
#define HTTPS_CLIENT_RECV_EVENT 3
#define HTTPS_SERVER_START_EVENT 4
#define HTTPS_SERVER_UNKNOWN_EVENT 5

/***********************************************************************************************************************
 * Private global functions
 **********************************************************************************************************************/
void server_start();
void set_sys_time();
void segger_print();
void deinit_server();
void wifi_init();
void client_request();

#endif //HTTPS_SERVER_CLIENT_H
