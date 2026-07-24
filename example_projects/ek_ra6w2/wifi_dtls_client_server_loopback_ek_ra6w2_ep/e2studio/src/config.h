/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef DTLS_W1_CONFIG_H_
#define DTLS_W1_CONFIG_H_

/***********************************************************************************************************************
 * Wi-Fi Configuration
 **********************************************************************************************************************/

/* Wi-Fi Access Point SSID */
#define SSID                           "SSID"

/* Length of SSID string */
#define SSID_LEN                       (sizeof(SSID) - 1)

/* Wi-Fi Access Point Password */
#define PSWD                           "PASSWORD"

/* Length of Password string */
#define PSWD_LEN                       (sizeof(PSWD) - 1)

/* Wi-Fi Security Type */
#define WIFI_SECURITY                  eWiFiSecurityWPA2

/***********************************************************************************************************************
 * DTLS Server Configuration
 **********************************************************************************************************************/

/* PC / Server IP Address
 * Example:
 * "192.168.1.10"
 */
#define DTLS_SERVER_IP                 "YOUR_SERVER_IP"

/* DTLS Server Port
 * Must match OpenSSL DTLS server accept port
 * Example:
 * openssl s_server -dtls1_2 -accept 44330 ...
 */
#define DTLS_SERVER_PORT               44330

#endif /* DTLS_W1_CONFIG_H_ */
