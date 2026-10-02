/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
#if !defined (__OTA_UPDATE_HTTP_H__)
#define	__OTA_UPDATE_HTTP_H__

#include "sdk_type.h"

#include "common_def.h"
#include "ota_update.h"
#include "ota_update_common.h"
#include "iface_defs.h"
#include "common_utils.h"
#include "user_http_client.h"
#include "mbedtls/ssl.h"

/// Receive buffer size.
#define OTA_HTTP_RX_BUF_SZ		OTA_SFLASH_BUF_SZ

/// NVRAM name of  TLS auth mode
#define OTA_HTTP_NVRAM_TLS_AUTHMODE	"OTA_TLS_AUTHMODE"
#define OTA_HTTP_TLS_AUTHMODE_DEF	MBEDTLS_SSL_VERIFY_NONE

UINT ota_update_http_client_request(ota_update_proc_t *ota_proc);
UINT ota_update_httpc_set_tls_auth_mode_nvram(int tls_auth_mode);

#endif	// (__OTA_UPDATE_HTTP_H__)

/* EOF */
