/***********************************************************************************************************************
 * File Name    : http_svr.h
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef HTTP_SVR_H_
#define HTTP_SVR_H_

#include "common_utils.h"

#define SSID "username"
#define PASSPHRASE "password"
#define CHANNEL 0
#define PASSS_LEN strlen(PASSPHRASE)
#define SSID_LEN strlen(SSID)

fsp_err_t init_server();
void deinit_server();
static int http_get_values_json(char *buf, int buflen, const char *key);
static const char *cgi_set_presets(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
void format_fs();
int write_file(char *p_path, char *p_buf);

#endif //HTTP_SVR_H_
