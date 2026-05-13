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

#include "r_timer_api.h"
#include "r_tim_w.h"
#include "lwip/netif.h"
#include "https.h"

#define WEATHER_TIMER_EVENT   (1U << 8)


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

/* ===================== TIMER APIs ===================== */

//fsp_err_t init_gpt_timer(tim_w_instance_ctrl_t *const p_timer_ctl,
//                         timer_cfg_t const *const p_timer_cfg,
//                         uint8_t timer_mode);
//
//fsp_err_t start_gpt_timer(pmgr_ctrl_t *const p_timer_ctl);
//void deinit_gpt_timer(tim_w_instance_ctrl_t *const p_timer_ctl);
//fsp_err_t timer_init(void);

void user_gpt_one_shot_callback(timer_callback_args_t *p_args);

/* ===================== HTTPS / CERT ===================== */

fsp_err_t set_cert(httpc_secure_connection_t *https_conf,
                   u8 *cert,
                   size_t cert_len);

void prepare_client_request_owm(https_client_opcode_t op_code,
                                http_client_request_t *req,
                                char *curr_location);

#endif /* WEATHER_APP_H_ */
