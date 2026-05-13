/***********************************************************************************************************************
 * File Name    : sntp_example.c
 * Description  : Example of SNTP.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "sntp_example.h"
#include "config.h"

extern int sntp_get_period(void);
extern unsigned int get_sntp_use(void);
extern unsigned int set_sntp_use(int use);
extern u8_t sntp_get_use(void);
extern void get_sntp_server(char *svraddr, unsigned int index);

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

void print_current_time(void)
{
    rtc_w_instance_ctrl_t *rtc_w_ctrl = R_RTC_W_GetCtrl();
    struct tm ts;
    char buf[32];

    if (rtc_w_ctrl != NULL)
    {
        R_RTC_W_CalendarTimeGet(rtc_w_ctrl, &ts);
        R_RTC_W_Time2Str(rtc_w_ctrl, &ts, buf, sizeof(buf), "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        snprintf(buf, sizeof(buf), "RTC-NA");
    }

    APP_PRINT("\n>>> Current time : %s\n", buf);
}

void sntp_example(void)
{
    UINT8 status;

    /* Configure SNTP server domain */
    APP_PRINT("\n>>> Set SNTP configuration...\n");

    status = set_sntp_server((unsigned char*) SNTP_SERVER_DOMAIN_0, 0);
    if (status == pdTRUE)
    {
        APP_PRINT("\tSNTP Server 0 : %s\n", SNTP_SERVER_DOMAIN_0);
    }

    status = set_sntp_server((unsigned char*) SNTP_SERVER_DOMAIN_1, 1);
    if (status == pdTRUE)
    {
        APP_PRINT("\tSNTP Server 1 : %s\n", SNTP_SERVER_DOMAIN_1);
    }

    status = set_sntp_server((unsigned char*) SNTP_SERVER_DOMAIN_2, 2);
    if (status == pdTRUE)
    {
        APP_PRINT("\tSNTP Server 2 : %s\n", SNTP_SERVER_DOMAIN_2);
    }

    /* Configure SNTP sync period: seconds */
    status = set_sntp_period((int) SNTP_SYNC_PERIOD);
    if (status == pdTRUE)
    {
        APP_PRINT("\tSNTP Sync Period : %d seconds\n", SNTP_SYNC_PERIOD);
    }

    /* Configure SNTP time zone: seconds */
    set_time_zone((long) SNTP_TIME_ZONE);

    /* Set SNTP client */
    set_sntp_use(pdTRUE);

    if (get_sntp_use())
    {
        APP_PRINT("\n>>> SNTP client is started\n");
    }

    /* Check SNTP sync status */
    while (1)
    {
        if (is_sntp_sync() == TRUE)
        {
            APP_PRINT("\n>>> SNTP client is sync\n");
            break;

        }
        vTaskDelay(200);
    }
}

void display_scan_result(WIFIScanResult_t * scan_data)
{
    if (scan_data)
    {
        APP_PRINT("\n==================================================");
        APP_PRINT("\nNUM | SSID: RSSI CH Encryption");
        for (uint8_t i = 0; i < MAX_WIFI_SCAN_RESULTS; ++i) {
            if(scan_data[i].cRSSI != 0)
            {
                /* Print wifi_normal_scan_exampleSSID and RSSI for each network */
                APP_PRINT("\n%3d | ", i);
                if (scan_data[i].ucSSID[0] == HIDDEN_SSID_DETECTION_CHAR)
                {
                	APP_PRINT(" %-32s: ", "[Hidden]");
                }
                else
                {
                    APP_PRINT("%-32s: ", scan_data[i].ucSSID);
                }
                APP_PRINT("%4d ", scan_data[i].cRSSI);
                APP_PRINT("%3d ", scan_data[i].ucChannel);
                switch ((WIFISecurityExt_t) scan_data[i].xSecurity) {
                    case eWiFiSecurityOpen:
                        APP_PRINT("Open");
                        break;
                    case eWiFiSecurityWEP:
                        APP_PRINT("WEP");
                        break;
                    case eWiFiSecurityWPA:
                        APP_PRINT("WPA");
                        break;
                    case eWiFiSecurityWPA2:
                        APP_PRINT("WPA2");
                        break;
                    case eWiFiSecurityWPA2_ent:
                        APP_PRINT("WPA2_ENT");
                        break;
                    case eWiFiSecurityWPA3:
                        APP_PRINT("WPA3");
                        break;
                    case eWiFiSecurityWPA_ent_ext:
                    	APP_PRINT("WPA_ENT");
                        break;
                    case eWiFiSecurityWPA_WPA2_ent_ext:
                    	APP_PRINT("WPA_WPA2_ENT");
                        break;
                    case eWiFiSecurityWPA2_WPA3_ent_ext:
                    	APP_PRINT("WPA2_WPA3_ENT");
                        break;
                    case eWiFiSecurityWPA3_ent_ext:
                    	APP_PRINT("WPA3_ENT");
                        break;
                    case eWiFiSecurityWPA3_192B_ent_ext:
                    	APP_PRINT("WPA3_192B_ENT");
                        break;
                    case eWiFiSecurityWPA_WPA2_ext:
                    	APP_PRINT("WPA_WPA2");
                        break;
                    case eWiFiSecurityWPA2_WPA3_ext:
                    	APP_PRINT("WPA2_WPA3");
                        break;
                    case eWiFiSecurityWPA3_OWE_ext:
                    	APP_PRINT("WPA3_OWE");
                        break;
                    case eWiFiSecurityNotSupported:
                        APP_PRINT("NotSupported");
                        break;
                    default:
                        APP_PRINT("-");
                        break;
                }
            }
        }
        APP_PRINT("\n==================================================\n");
        vTaskDelay(pdMS_TO_TICKS(10));
    }else {
        APP_PRINT("\nNo scan result..");
    }
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
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    return line;
}
