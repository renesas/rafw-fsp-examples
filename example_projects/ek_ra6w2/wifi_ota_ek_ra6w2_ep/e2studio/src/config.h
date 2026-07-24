/***********************************************************************************************************************
* File Name    : config.h
* Description  : Configuration definitions for the Wi-Fi OTA example project.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include "common_utils.h"
#include "rm_ota_w_api.h"

#undef HTTPS

#define EVENT_VAL -1
#define WIFI_EVENT_CONNECTED 3
#define LENGTH_ERROR 7
#define TASK_COMPLETE_EVENT 5
#define LOOP_ITR_MAX 3

#define SSID    "Hehe"
#define PASSPHRASE "Aleenaal"
#define CHANNEL 0
#define WIFI_SECURITY eWiFiSecurityWPA2
/*  example HOST 192.168.30.220  */
#define HOST    "10.127.198.35"
/*  example PATH http://192.168.30.220/wifi_psram_ek_ra6w1_ep_ota.img   */
#define PATH "http://10.127.198.35/wifi_tcp_client_sleep4_ek_ra6w2_ep_ota.img"
#ifdef HTTPS
#define PORT 443
#define AUTH_MODE   MBEDTLS_SSL_VERIFY_NONE
#define TLS_VER MBEDTLS_SSL_VERSION_TLS1_2
#else
#define PORT 80
#endif

/*  configure OTA_UPDATE_TYPE as RM_OTA_W_TYPE_MCU_FW for MCU_FW and
    RM_OTA_W_TYPE_RTOS for RTOS image   */

#define OTA_UPDATE_TYPE RM_OTA_W_TYPE_RTOS

#ifdef HTTPS
/*  Sample CA certificate  */
static uint8_t  ca_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
        "MIIDpDCCAoygAwIBAgIUHlzAOnTscWjuMqqCVo5dndl0+6kwDQYJKoZIhvcNAQEL\n"
        "BQAwYjELMAkGA1UEBhMCSU4xDzANBgNVBAgMBktlcmFsYTEOMAwGA1UEBwwFS29j\n"
        "aGkxEjAQBgNVBAoMCU15Q29tcGFueTELMAkGA1UECwwCSVQxETAPBgNVBAMMCE15\n"
        "Um9vdENBMB4XDTI1MDcwOTA2NDI0NloXDTI3MTAxMjA2NDI0NlowaDELMAkGA1UE\n"
        "BhMCSU4xDzANBgNVBAgMBktlcmFsYTEOMAwGA1UEBwwFS29jaGkxEjAQBgNVBAoM\n"
        "CU15Q29tcGFueTEMMAoGA1UECwwDV2ViMRYwFAYDVQQDDA0xOTIuMTY4LjUwLjg3\n"
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAr1oXX2JDXwkHhFEGz9Iu\n"
        "11/xkeZm7hL7lajZkGoKZBiTbtKo9Q2ez1WPYn8WSd62HO20OW8acZ1CL6J0JBgo\n"
        "w/KXnrtoL3tr51C4V7eibDADEnoxZo6IT9l4zfs/94ViHT1i3kusUB7DaB1eIt4q\n"
        "+UBK/pZH38ts7U37dx4Ju64LStdlcsUMkPDTq8H3aFczj+HhtkGQqvTGcyb+7zq/\n"
        "3LQwpxdynz4ZWnUOYCDEpXJuE7GtRJvyws9DITKnKbkWKXFG/OsvnhsjFo9bNzCL\n"
        "tHSTXVzvu7hLaqkgBNGtuHuyGkCgn/VcbeHww2/cYNjih9LV43oXKJPnJIIEvWv6\n"
        "MwIDAQABo0wwSjAfBgNVHSMEGDAWgBTDgaMdY+rE8pwN6Ano4YwOkI68STAJBgNV\n"
        "HRMEAjAAMAsGA1UdDwQEAwIFoDAPBgNVHREECDAGhwTAqDJXMA0GCSqGSIb3DQEB\n"
        "CwUAA4IBAQAX0JpF4FL18B4MWZ4t8EZW3okShjyWFVyNNN5MEvYql8Obr2a1egyE\n"
        "h2Z15U5VHk+yfs30TVEPjuLojAneWNHPapueqFXD1SbBh7ezrqYIJkENWYNwOoEk\n"
        "oh9R/DorckMB8ZJIhGy3onE8ycGk+Ubgjp3dcdRGZSSVq1kWldpt5tHvlJnR6N2g\n"
        "jAockdcXNSllJk2vJ+9bX+KGvk6xCD0wHVNuSvAK3PweroY3HDuHuGhyzMHn6PBH\n"
        "/7yXI4pSsVAFxr7C5anifhVVaK6Ks32jceETWIXC/UZzNL5G44N+vlUSBUHj85dH\n"
        "CJHVetiUIsVh+Sp6v+pkcn4KjAWq7OTo\n"
    "-----END CERTIFICATE-----\n";
static size_t ca_cert_len = sizeof(ca_cert);
#endif
#endif
