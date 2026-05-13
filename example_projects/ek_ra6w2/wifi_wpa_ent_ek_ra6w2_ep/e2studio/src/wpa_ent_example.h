/***********************************************************************************************************************
 * File Name    : wpa_ent_example.h
 * Description  : the header of WPA enterprise example.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef WPA_ENT_EXAMPLE_H
#define WPA_ENT_EXAMPLE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "rm_wifi_api.h"
#include "rm_wifi.h"
#include "r_rtc_w.h"
#include "rm_vee_flash_w_rrq_nvram.h"
#include "rm_map_persistant_w.h"

#include "os.h"
#include "common_utils.h"

typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

#define MAX_WIFI_SCAN_RESULTS    30
#define RTT_LINE_MAX_LEN         128
#define BUF_SIZE                 (128U)
#define INITIAL_VALUE            '\0'
#define WIFI_EVENT_CONNECTED     5

#define eWiFiSecurityWPA_ent_ext		7	// WPA Enterprise Security
#define eWiFiSecurityWPA_WPA2_ent_ext	8	// WPA + WPA2 Enterprise Security
#define eWiFiSecurityWPA2_WPA3_ent_ext	9   // WPA2 + WPA3 Enterprise Security
#define eWiFiSecurityWPA3_ent_ext		10  // WPA3 128 Bits Enterprise Security
#define eWiFiSecurityWPA3_192B_ent_ext	11  // WPA3 192 Bits Enterprise Security
#define eWiFiSecurityWPA_WPA2_ext		12  // WPA + WPA2 Security
#define eWiFiSecurityWPA2_WPA3_ext		13  // WPA2 + WPA3 Security
#define eWiFiSecurityWPA3_OWE_ext		14  // WPA3-OWE Security

void print_ep_info();
char *wpa_ent_connect_info();
void display_scan_result(WIFIScanResult_t * scan_data);
char *process_input_data(void);

#endif /* WPA_ENT_EXAMPLE_H */
