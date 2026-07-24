/**
 ****************************************************************************************
 *
 * @file cli_ble_msg.h
 *
 * @brief Header file for basic console user interface of the host application.
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
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
