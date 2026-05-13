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

bool ble_command(int argc, const char *argv[], void *user_data);

#endif /* CLI_BLE_H_ */

/**
 * \}
 */
