/**
 ****************************************************************************************
 *
 * @file cli_ble.c
 *
 * @brief RTOS command functions
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
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

/**
 * \addtogroup REFERENCE_APPS
 * \{
 */
#include "rm_cli_w_debug_utils.h"

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
    return debug_handle_message(argc, argv, ble_handlers);
}

/**
 * \}
 */
