/***********************************************************************************************************************
 * File Name    : cli_ble.h
 * Description  : Header file for BLE commands on console
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "stdbool.h"

/**
 * \addtogroup REFERENCE_APPS
 * \{
 */
#ifndef CLI_BLE_H_
 #define CLI_BLE_H_

bool ble_command(int argc, const char * argv[], void * user_data);
bool cmd_ble_start_adv(int argc, char * argv[]);
bool cmd_ble_stop_adv(int argc, char * argv[]);
bool cmd_ble_start_svc(int argc, char * argv[]);
bool cmd_ble_stop_svc(int argc, char * argv[]);
bool cmd_ble_write_public_bd_addr(int argc, char * argv[]);
bool cmd_ble_get_public_bd_addr(int argc, char * argv[]);
bool cmd_ble_clear_public_bd_addr(int argc, char * argv[]);
bool cmd_ble_disconnect(int argc, char * argv[]);
bool cmd_ble_read_public_bd_addr(int argc, char * argv[]);

#endif                                 /* CLI_BLE_H_ */

/**
 * \}
 */
