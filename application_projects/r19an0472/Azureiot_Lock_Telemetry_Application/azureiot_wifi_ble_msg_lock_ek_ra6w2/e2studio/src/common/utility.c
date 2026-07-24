/***********************************************************************************************************************
* File Name    : utility.c
* Description  : Common utility functions for the application.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>

#include "utility.h"

int32_t string_to_ip(const char *ip_str, uint8_t *binary_ip) {
#define IP_ADDR_LEN 4

    uint32_t octets[IP_ADDR_LEN];
    uint32_t result;

    result = sscanf(ip_str, "%u.%u.%u.%u", &octets[0], &octets[1], &octets[2], &octets[3]);
    if (result != IP_ADDR_LEN) {
        return -1;
    }

    for (uint32_t i = 0; i < IP_ADDR_LEN; i++) {
        if (octets[i] > 255) {
            return -1;
        }
        binary_ip[i] = (uint8_t)octets[i];
    }

    return 0;
#undef IP_ADDR_LEN
}

const char* ip_to_string(uint32_t binary_ip, const char *ip_str, uint32_t len) {
     snprintf(ip_str, len, "%u.%u.%u.%u",
         (binary_ip >>  0) & 0xff, (binary_ip >>  8) & 0xff,
         (binary_ip >> 16) & 0xff, (binary_ip >> 24) & 0xff);
     return ip_str;
}
