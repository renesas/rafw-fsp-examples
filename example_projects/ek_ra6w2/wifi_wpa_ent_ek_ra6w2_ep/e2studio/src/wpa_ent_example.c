/***********************************************************************************************************************
 * File Name    : wpa_ent_example.c
 * Description  : Example of WPA enterprise.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "app_task.h"
#include "dhcp_util.h"
#include "lwip/dhcp.h"
#include "wpa_ent_example.h"

void print_ep_info()
{
    fsp_pack_version_t version;
    R_FSP_VersionGet(&version);
    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3, EP_VERSION);
    APP_PRINT(BANNER_4, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
}

char *wpa_ent_connect_info()
{
	struct netif *iface_sta = WIFI_GetNetIf(eWiFiModeStation);

	APP_PRINT("\n>>> WPA Enterprise AP Connect: Success\n")
	APP_PRINT("IP assigned: %s\n", ipaddr_ntoa(&iface_sta->ip_addr));
	APP_PRINT("Net Mask: %s\n", ipaddr_ntoa(&iface_sta->netmask));
	APP_PRINT("Gateway IP: %s\n", ipaddr_ntoa(&iface_sta->gw));

	ip_addr_t srv;
    WIFIReturnCode_t err;

    err = dhcp_start(iface_sta);
    if (err)
    {
    	APP_PRINT("DHCP client start failed with wifi_err=%d\n", err);
    }
    else
    {
    	int rc = dhcp_get_server_ip(iface_sta, &srv);
		if (rc == 0) {
			APP_PRINT("DHCP server: %s\n", ipaddr_ntoa(&srv));
			return ipaddr_ntoa(&srv);
		} else {
			APP_PRINT("Failed to get DHCP server IP (rc=%d)\n", rc);
		}
    }
    return NULL;
}

/*******************************************************************************************************************//**
 * Right align (string)
 **********************************************************************************************************************/
static void pad_right(char * dst, size_t width, const char * src)
{
    size_t len = strlen(src);

    if (len > width)
    {
        len = width;
    }

    memset(dst, ' ', width);
    memcpy(dst, src, len);
    dst[width] = '\0';
}

/*******************************************************************************************************************//**
 * Left align (number string)
 **********************************************************************************************************************/
static void pad_left(char * dst, size_t width, const char * src)
{
    size_t len = strlen(src);

    if (len > width)
    {
        len = width;
    }

    memset(dst, ' ', width);
    memcpy(dst + (width - len), src, len);
    dst[width] = '\0';
}

/*******************************************************************************************************************//**
 * Display Wi-Fi Scan Result
 **********************************************************************************************************************/
void display_scan_result(WIFIScanResult_t * scan_data)
{
    if (NULL == scan_data)
    {
        APP_PRINT("\nNo scan result..\n");

        return;
    }

    APP_PRINT("\n================================================================================");
    APP_PRINT("\nNUM | SSID                             | RSSI  | CH   | Encryption     ");
    APP_PRINT("\n================================================================================");

    for (uint8_t i = 0; i < MAX_WIFI_SCAN_RESULTS; i++)
    {
        if (scan_data[i].cRSSI == 0)
        {
            continue;
        }

        const char * security_str = "-";

        switch ((WIFISecurityExt_t) scan_data[i].xSecurity)
        {
            case eWiFiSecurityOpen:
                security_str = "Open";
                break;

            case eWiFiSecurityWEP:
                security_str = "WEP";
                break;

            case eWiFiSecurityWPA:
                security_str = "WPA";
                break;

            case eWiFiSecurityWPA2:
                security_str = "WPA2";
                break;

            case eWiFiSecurityWPA2_ent:
                security_str = "WPA2_ENT";
                break;

            case eWiFiSecurityWPA3:
                security_str = "WPA3";
                break;

            case eWiFiSecurityWPA_ent_ext:
                security_str = "WPA_ENT";
                break;

            case eWiFiSecurityWPA_WPA2_ent_ext:
                security_str = "WPA_WPA2_ENT";
                break;

            case eWiFiSecurityWPA2_WPA3_ent_ext:
                security_str = "WPA2_WPA3_ENT";
                break;

            case eWiFiSecurityWPA3_ent_ext:
                security_str = "WPA3_ENT";
                break;

            case eWiFiSecurityWPA3_192B_ent_ext:
                security_str = "WPA3_192B_ENT";
                break;

            case eWiFiSecurityWPA_WPA2_ext:
                security_str = "WPA_WPA2";
                break;

            case eWiFiSecurityWPA2_WPA3_ext:
                security_str = "WPA2_WPA3";
                break;

            case eWiFiSecurityWPA3_OWE_ext:
                security_str = "WPA3_OWE";
                break;

            case eWiFiSecurityNotSupported:
                security_str = "NotSupported";
                break;

            default:
                break;
        }

        char ssid_field[SSID_DISPLAY_WIDTH + 1];
        char rssi_field[RSSI_DISPLAY_WIDTH + 1];
        char ch_field[CH_DISPLAY_WIDTH + 1];
        char enc_field[ENC_DISPLAY_WIDTH + 1];

        char tmp[32];
        char ssid_buf[SSID_DISPLAY_WIDTH + 1] = {0};

        /* SSID */
        if (scan_data[i].ucSSID[0] == HIDDEN_SSID_DETECTION_CHAR)
        {
            strcpy(ssid_buf, "[Hidden]");
        }
        else
        {
            memcpy(ssid_buf,
                   scan_data[i].ucSSID,
                   SSID_DISPLAY_WIDTH);

            ssid_buf[SSID_DISPLAY_WIDTH] = '\0';
        }

        pad_right(ssid_field,
                  SSID_DISPLAY_WIDTH,
                  ssid_buf);

        /* RSSI */
        snprintf(tmp,
                 sizeof(tmp),
                 "%d",
                 scan_data[i].cRSSI);

        pad_left(rssi_field,
                 RSSI_DISPLAY_WIDTH,
                 tmp);

        /* Channel */
        snprintf(tmp,
                 sizeof(tmp),
                 "%d",
                 scan_data[i].ucChannel);

        pad_left(ch_field,
                 CH_DISPLAY_WIDTH,
                 tmp);

        /* Encryption */
        pad_right(enc_field,
                  ENC_DISPLAY_WIDTH,
                  security_str);

        APP_PRINT("\n%3d | %s | %s | %s | %s",
                  i,
                  ssid_field,
                  rssi_field,
                  ch_field,
                  enc_field);
    }

    APP_PRINT("\n================================================================================\n\n");

    vTaskDelay(pdMS_TO_TICKS(10));
}

char *process_input_data(void) {
    char *line = NULL;
    uint32_t num_bytes = RESET_VALUE;
    size_t len = 0;

    while (RESET_VALUE == num_bytes) {
        uint8_t buf[BUF_SIZE] = {INITIAL_VALUE};
        num_bytes = APP_READ(buf);
        if (num_bytes > 0) {
            for (uint32_t i = 0; i < num_bytes; ++i) {
                char c = (char)buf[i];

                if (c == '\r' || c == '\n') {
                    if (len > 0) {
                        line[len] = '\0';
                        len = 0;
                    }
                } else if (len < sizeof(buf) - 1) {
                    line[len++] = c;
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    return line;
}
