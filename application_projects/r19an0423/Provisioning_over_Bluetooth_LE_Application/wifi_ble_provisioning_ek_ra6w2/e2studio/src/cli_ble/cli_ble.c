/***********************************************************************************************************************
 * File Name    : cli_ble.c
 * Description  : RTOS command functions
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/**
 * \addtogroup REFERENCE_APPS
 * \{
 */
#include "rm_cli_w_debug_utils.h"
#include "cli_ble.h"

extern bool cmd_ble_start_svc(int argc, char *argv[]);
extern bool cmd_ble_stop_svc(int argc, char *argv[]);
extern bool cmd_ble_start_adv(int argc, char *argv[]);
extern bool cmd_ble_stop_adv(int argc, char *argv[]);
extern bool cmd_ble_disconnect(int argc, char *argv[]);
extern bool cmd_ble_write_public_bd_addr(int argc, char *argv[]);
extern bool cmd_ble_get_public_bd_addr(int argc, char *argv[]);
extern bool cmd_ble_read_public_bd_addr(int argc, char *argv[]);
extern bool cmd_ble_clear_public_bd_addr(int argc, char *argv[]);


static const debug_handler_t ble_handlers[] = {
    { "svc_start",             "BLE Service start",                   (debug_callback_t)cmd_ble_start_svc                 },
    { "svc_stop",              "BLE Service stop",                    (debug_callback_t)cmd_ble_stop_svc                  },
    { "adv_start",             "BLE ADV start",                       (debug_callback_t)cmd_ble_start_adv                 },
    { "adv_stop",              "BLE ADV stop",                        (debug_callback_t)cmd_ble_stop_adv                  },
    { "disconnect",            "Disconnect BLE connection",           (debug_callback_t)cmd_ble_disconnect                },
    { "get_bd_addr",           "Gets current operating BD address",   (debug_callback_t)cmd_ble_get_public_bd_addr        },
    { "write_public_bd_addr",  "BD public addr set to NVRAM",         (debug_callback_t)cmd_ble_write_public_bd_addr      },
    { "read_public_bd_addr",   "BD public addr read from NVRAM",      (debug_callback_t)cmd_ble_read_public_bd_addr       },
    { "clear_public_bd_addr",  "BD public addr clear from NVRAM",     (debug_callback_t)cmd_ble_clear_public_bd_addr      },
    { NULL },
};

bool ble_command (int argc, const char *argv[], void *user_data)
{
	(void)user_data;
    return debug_handle_message(argc, argv, ble_handlers);
}

/**
 * \}
 */
