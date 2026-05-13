/***********************************************************************************************************************
 * File Name    : common_utils.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

/* generic headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* SEGGER RTT and error related headers */
#include "SEGGER_RTT.h"

#define DBG_LVL_LOG (0)
#define DBG_LVL_ERROR (1)
#define DBG_LVL_WARNING (2)
#define DBG_LVL_INFO (3)
#define BIT_SHIFT_8 (8u)
#define SIZE_64 (64u)
#define DBG_LVL (DBG_LVL_INFO)
#define RESET_VALUE (0x00)
#define SEGGER_INDEX (0)

void print_ep_info_banner(const char *, const char *, const char *);

#define DBG_PRINT(lvl, lvl_str, fmt, ...) \
    do { \
        if (lvl <= DBG_LVL) \
            SEGGER_RTT_printf(SEGGER_INDEX, "[" lvl_str "] " fmt, ##__VA_ARGS__); \
       } while(0)

#define DBG_PRINT_DIRECT(lvl, fmt, ...) \
    do { \
        if (lvl <= DBG_LVL) \
            SEGGER_RTT_printf(SEGGER_INDEX, fmt, ##__VA_ARGS__); \
       } while(0)

#define APP_PRINT_LOG(...) DBG_PRINT(DBG_LVL_LOG, "LOG", ##__VA_ARGS__)
#define APP_PRINT_ERR(...) DBG_PRINT(DBG_LVL_ERROR, "ERR", ##__VA_ARGS__)
#define APP_PRINT_WARN(...) DBG_PRINT(DBG_LVL_WARNING, "WARNING", ##__VA_ARGS__)
#define APP_PRINT_INFO(...) DBG_PRINT(DBG_LVL_INFO, "INFO", ##__VA_ARGS__)

#define APP_PRINT(...) DBG_PRINT_DIRECT(DBG_LVL_LOG, ##__VA_ARGS__)

#define APP_ERR_PRINT(fn_, ...) if(DBG_LVL)\
                                   SEGGER_RTT_printf (SEGGER_INDEX, "[ERR] In Function: %s(), %s",__FUNCTION__,(fn_),##__VA_ARGS__);

#define APP_ERR_TRAP(err) if(err) {\
                                      SEGGER_RTT_printf(SEGGER_INDEX, "\r\nReturned Error Code: 0x%x  \r\n", err);\
                                      __asm("BKPT #0\n");} /* trap upon the error  */

#define APP_READ(read_data) SEGGER_RTT_Read (SEGGER_INDEX, read_data, sizeof(read_data));

#define APP_CHECK_DATA SEGGER_RTT_HasKey()

#define _STRINGFY_T(x) #x
#define _STRINGFY(x) _STRINGFY_T(x)

#endif //COMMON_UTILS_H_
