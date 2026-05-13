/***********************************************************************************************************************
 * File Name    : cli_ble_msg.h
 * Description  : Header file for basic console user interface of the host application.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef CLI_BLE_MSG_H_
#define CLI_BLE_MSG_H_

#include <stdbool.h>

typedef struct st_cli_ble_msg
{
    unsigned char type;
    unsigned char val;
    unsigned char idx;
} cli_ble_msg_t;

typedef enum e_cli_ble_msg_type
{
    CLI_BLE_MSG_TYPE_SVC_START,
    CLI_BLE_MSG_TYPE_SVC_STOP,
    CLI_BLE_MSG_TYPE_ADV_START,
    CLI_BLE_MSG_TYPE_ADV_STOP,
    CLI_BLE_MSG_TYPE_DISCONNECT,
    CLI_BLE_MSG_TYPE_BD_ADDR_GET,
} cli_ble_msg_type_t;

#endif //CLI_BLE_MSG_H_
