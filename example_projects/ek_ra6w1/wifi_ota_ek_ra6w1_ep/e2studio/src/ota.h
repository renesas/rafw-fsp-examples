/***********************************************************************************************************************
* File Name    : ota.h
* Description  : OTA update function declarations for the Wi-Fi OTA example project.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef OTA_H_
#define OTA_H_

#include "hal_data.h"
#include "config.h"

fsp_err_t ota_update_start(void);
void post_http_result(void);
void http_restart(void);


#endif
