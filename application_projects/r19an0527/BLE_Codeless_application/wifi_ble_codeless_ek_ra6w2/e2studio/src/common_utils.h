/***********************************************************************************************************************
 * File Name    : common_utils.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

/* generic headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "SEGGER_RTT.h"

/* Add macros to enable/disable workaround code snippets */
#define CFG_LLS_WRITE_REQ_WORKAROUND    (1)

#define BIT_SHIFT_8                     (8u)
#define SIZE_64                         (64u)

#define LVL_ERR                         (1u) /* error conditions   */

#define RESET_VALUE                     (0x00)

#define EP_VERSION                      ("1.0")
#define MODULE_NAME                     "ble_codeless"
#define EP_INFO                                                                                               \
    "The example project showcases how we can transfer raw data in a binary mode using the BLE_CODELESS.\r\n" \

#define AUTOTEST_ENABLE                 (1)

#define SEGGER_INDEX                    (0)

#if AUTOTEST_ENABLE == 1
 #define APP_PRINT(fn_, ...)    SEGGER_RTT_printf(SEGGER_INDEX, (fn_), ## __VA_ARGS__);
#else
 #define APP_PRINT(fn_, ...)
#endif

#if AUTOTEST_ENABLE
 #define DECLARE_DUMMY_READ_BATTERY_LEVEL
#endif

#if AUTOTEST_ENABLE
 #define AUTOTEST_PRINT(...)       printf(__VA_ARGS__);
#endif

#define APP_ERR_PRINT(fn_, ...)    if (LVL_ERR) { \
        AUTOTEST_PRINT("[ERR] In Function: %s(), %s", __FUNCTION__, (fn_));}

#define APP_ERR_TRAP(err)          if (err) {                                        \
        SEGGER_RTT_printf(SEGGER_INDEX, "\r\nReturned Error Code: 0x%x  \r\n", err); \
        __asm("BKPT #0\n");}           /* trap upon the error  */

#define APP_READ(read_data)        SEGGER_RTT_Read(SEGGER_INDEX, read_data, sizeof(read_data));

#define APP_CHECK_DATA    SEGGER_RTT_HasKey()

void print_ep_info_banner(const char * s_module, const char * s_version, const char * s_info);

#endif                                 /* COMMON_UTILS_H_ */
