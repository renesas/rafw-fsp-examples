/***********************************************************************************************************************
 * File Name    : sntp_example.h
 * Description  : the header of SNTP example.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef SNTP_EXAMPLE_H
#define SNTP_EXAMPLE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "r_rtc_w.h"
#include "lwip/netif.h"
#include "net_sntp_client.h"
#include "common_utils.h"

typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;

#define MAX_WIFI_SCAN_RESULTS    50
#define RTT_LINE_MAX_LEN         128
#define BUF_SIZE                 (128U)           /* Size of buffer for RTT input data */
#define INITIAL_VALUE            '\0'
#define WIFI_EVENT_CONNECTED     5

void print_ep_info();
void print_current_time(void);
void sntp_example(void);
void display_scan_result(WIFIScanResult_t * scan_data);
char *process_input_data(void);

#endif /* SNTP_EXAMPLE_H */
