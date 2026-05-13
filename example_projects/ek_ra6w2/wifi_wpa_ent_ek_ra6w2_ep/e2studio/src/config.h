/***********************************************************************************************************************
 * File Name    : config.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "common_utils.h"

/* WPA enterprise setting */
#define BAND 2 /* 0:2.4G, 1:5G 2: Dual */
#define ENT_AUTH 2 /* PEAP */
#define AUTH_PRO 1 /* MSCHAPv2_GTC */

/* Ping test setting */
#define PING_DUT	1 /* Ping device - 0: DUT, 1: DCHP Server */
#define PING_IP_ADDRESS   "192.168.0.227"
#define PING_COUNT        10
#define PING_TIMEOUT	  3000

/* Event status */
#define EVENT_VAL -1
#define WIFI_STATUS_EVENT 1
#define WIFI_EVENT_CONNECTED 5
