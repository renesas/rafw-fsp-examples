/***********************************************************************************************************************
 * File Name    : weather_app.h
 * Description  : Contains data structures and functions used in weather_app.h/c
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef WEATHER_APP_H_
#define WEATHER_APP_H_

#include "lwip/netif.h"
#include "https.h"

/* ===================== WIFI CONFIG ===================== */

#define SSID        "SSID"
#define PSWD        "PWD"
#define SSID_LEN    strlen(SSID)
#define PSWD_LEN    strlen(PSWD)
#define EVENT_VAL   (-1)

/* ===================== WEATHER APP APIs ===================== */

fsp_err_t start_weather_monitor(void);
void stop_weather_monitor(void);
void print_ep_info_banner(void);

/* ===================== WIFI / NETIF ===================== */

void wifi_init(void);
void netif_status_callback(struct netif *netif);

/* ===================== HTTPS / CERT ===================== */

fsp_err_t set_cert(httpc_secure_connection_t *https_conf,
                   u8 *cert,
                   size_t cert_len);

void prepare_client_request_owm(https_client_opcode_t op_code,
                                http_client_request_t *req,
                                char *curr_location);

#endif /* WEATHER_APP_H_ */
