/**
 ****************************************************************************************
 *
 * @file provisioning_helper.h
 *
 * @brief reboot AP mode to concurrent or soft AP.
 *
 * Copyright (c) 2016-2026 Renesas Electronics. All rights reserved.
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

#if !defined(PROVISIONING_W_HELPER_H)
#define PROVISIONING_W_HELPER_H

#include "FreeRTOS.h"
#include "provisioning_api.h"
#include "cJSON.h"
#include "bsp_api.h"
#include "hal_data.h"
/**
 * @brief Definition for the SoftAP Provisioning
 */

/** @def Predefine SSID for provision */

#define PREDEFINE_SSID           "Renesas_IoT_WiFi"

/** @def Predefine Password */
#define PREDEFINE_PW             "1234567890"

/** @def maximum length of SSID */
#define PROV_MAX_SSID_LEN       128

/** @def maximum length of WPA style passkey */
#define PROV_MAX_PW_LEN         128

/** @def maximum length of WEP style passkey */
#define PROV_MAX_WEP_KEY_LEN    16

/** @def maximum  scan channel */
#define SCAN_CHANNEL_MAX        10

/** @def scan buffer. need change */
#define PROV_SCAN_BUF_SIZE      3584

/** @def maximum scanning params. need to change */
#define MAX_CLI_PARAMS          SCAN_CHANNEL_MAX + 1

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "mbedtls/config.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"

#define TCP_SERVER_WIN_SIZE     (1024 * 4)
#define TCP_LISTEN_BAGLOG       5
#define PROV_TLS_SVR_TIMEOUT    1000  // 500      EU FAE TLS Fail issue 500 ==> 1000

///////////////////////////////////////////////////

/**
 * @brief external functions
 */
extern int provisioning_tls_svr_init_config(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_init_socket(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_deinit_socket(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_init_ssl(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_setup_ssl(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_deinit_ssl(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_shutdown_ssl(provisioning_tls_server_cfg_t *config);
extern int provisioning_tls_svr_do_handshake(provisioning_tls_server_cfg_t *config);

extern void* provisioning_calloc(size_t n, size_t size);
extern void provisioning_free(void *f);

#endif                                 // PROVISIONING_W_HELPER_H

/* EOF */
