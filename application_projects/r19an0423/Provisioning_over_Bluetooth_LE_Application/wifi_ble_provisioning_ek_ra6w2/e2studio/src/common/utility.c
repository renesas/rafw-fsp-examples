/***********************************************************************************************************************
 * File Name    : utility.c
 * Description  : Contains functions for parsing and formatting IP addresses.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include <stdio.h>

#include "utility.h"

int32_t string_to_ip(const char *ip_str, uint32_t *binary_ip)
{
#define IP_ADDR_LEN 4
    long octets[IP_ADDR_LEN];
    int result;
    result = sscanf(ip_str, "%ld.%ld.%ld.%ld", &octets[0], &octets[1], &octets[2], &octets[3]);
    if (result != IP_ADDR_LEN)
    {
        return -1;
    }

    uint8_t temp[IP_ADDR_LEN];
    for (uint32_t i = 0; i < IP_ADDR_LEN; i++)
    {
        if (octets[i] > 255)
        {
            return -1;
        }

        temp[i] = (uint8_t)octets[i];
    }
    memcpy(binary_ip, temp, sizeof(temp));

    return 0;
#undef IP_ADDR_LEN
}

const char* ip_to_string(uint32_t binary_ip,char *ip_str, uint32_t len)
{
     snprintf(ip_str, len, "%d.%d.%d.%d",
    		 (int)(binary_ip >>  0) & 0xff, (int)(binary_ip >>  8) & 0xff,
			 (int)(binary_ip >> 16) & 0xff, (int)(binary_ip >> 24) & 0xff);

     return ip_str;
}
