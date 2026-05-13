/***********************************************************************************************************************
 * File Name    : utility.h
 * Description  : Header file for macros and IP address conversion functions.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define UNUSED(x) (void) (x)

int32_t string_to_ip(const char *ip_str, uint32_t *binary_ip);
const char* ip_to_string(uint32_t binary_ip, char *ip_str, uint32_t len);

#endif // __COMMON_H__
