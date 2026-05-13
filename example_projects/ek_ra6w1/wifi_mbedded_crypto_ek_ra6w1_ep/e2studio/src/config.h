/***********************************************************************************************************************
 * File Name    : config.h
 * Description  : Contains configuration for Wi-Fi provisioning.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "common_utils.h"

#define CHANNEL 6
#define SSID "SSID"
#define PSWD "PASSWORD"
#define SSID_LEN strlen(SSID)
#define PSWD_LEN strlen(PSWD)
#define EVENT_VAL -1

#define SNTP_ENABLE 2
#define SNTP_SYNC_PERIOD (60) //sec
#define SNTP_SERVER_DOMAIN_0 "pool.ntp.org"
#define SNTP_SERVER_DOMAIN_1 "1.pool.ntp.org"
#define SNTP_SERVER_DOMAIN_2 "2.pool.ntp.org"
#define SNTP_TIME_ZONE (9*3600) //sec

#define WIFI_STATUS_EVENT 1
