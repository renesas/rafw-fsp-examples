/**
 ****************************************************************************************
 *
 * @file ota_update_http.h
 *
 * @brief Over the air firmware update by http protocol.
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


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
