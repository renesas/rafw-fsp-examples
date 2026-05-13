/***********************************************************************************************************************
 * File Name    : wifi_scan_example.c
 * Description  : Contains data structures and functions used in Wi-Fi scan example.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp_api.h"
#include "rm_wifi_api.h"
#include "common_utils.h"
#include "rm_wifi.h"

/* Macros */
#define MAX_WIFI_SCAN_RESULTS     (100u)

/* Global Variables */
WIFIScanResult_t wifi_scan_data[MAX_WIFI_SCAN_RESULTS];

/* Function prototypes */
void wifi_normal_scan_example(void);
void print_ep_info_banner(void);

/***********************************************************************************************************************
 * Function Name: print_ep_info_banner
 * Description  : Prints example project banner and version information.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
void print_ep_info_banner(void)
{
    fsp_pack_version_t version;

    R_FSP_VersionGet(&version);

    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2);
    APP_PRINT(BANNER_3, EP_VERSION);
    APP_PRINT(BANNER_4, version.version_id_b.major,
                       version.version_id_b.minor,
                       version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6);
    APP_PRINT(EP_INFO);
}

/***********************************************************************************************************************
 * Function Name: display_scan_result
 * Description  : Displays scan results obtained from Wi-Fi scan API.
 * Arguments    : scan_data - pointer to scan result array
 * Return Value : None
 ***********************************************************************************************************************/
static void display_scan_result(WIFIScanResult_t *scan_data)
{
    if (scan_data)
    {
        APP_PRINT("\n=================================================================\n");
        printf("\n=================================================================\n");

        APP_PRINT("Nr | SSID                             | RSSI | CH | Encryption");
        printf("Nr | SSID                             | RSSI | CH | Encryption");

        for (uint8_t i = 0; i < MAX_WIFI_SCAN_RESULTS; i++)
        {
            if (scan_data[i].cRSSI != 0)
            {
            	// Cast the security field to the extended enum for display
            	WIFISecurityExt_t sec = (WIFISecurityExt_t)(scan_data[i].xSecurity);
                APP_PRINT("\n%2d | %-32.32s | %d | %d | ",
                          (i + 1),
                          scan_data[i].ucSSID,
                          scan_data[i].cRSSI,
                          scan_data[i].ucChannel);
                printf("\n%2d | %-32.32s | %d | %d | ",
                                         (i + 1),
                                         scan_data[i].ucSSID,
                                         scan_data[i].cRSSI,
                                         scan_data[i].ucChannel);

                switch (sec)
                {
                    case eWiFiSecurityOpen:
                        APP_PRINT("Open");
                        printf("Open");

                        break;

                    case eWiFiSecurityWEP:
                        APP_PRINT("WEP");
                        printf("WEP");
                        break;

                    case eWiFiSecurityWPA:
                        APP_PRINT("WPA");
                        printf("WPA");
                        break;

                    case eWiFiSecurityWPA2:
                        APP_PRINT("WPA2");
                        printf("WPA2");
                        break;

                    case eWiFiSecurityWPA2_ent:
                        APP_PRINT("WPA2_ent");
                        printf("WPA2_ent");
                        break;

                    case eWiFiSecurityWPA3:
                        APP_PRINT("WPA3");
                        printf("WPA3");
                        break;

                    case eWiFiSecurityNotSupported:
                        APP_PRINT("NotSupported");
                        printf("NotSupported");
                        break;

                    case eWiFiSecurityWPA_ent_ext:
                    	APP_PRINT("WPA_ent");
                    	printf("WPA_ent");
                    	break;

                    case eWiFiSecurityWPA_WPA2_ent_ext:
                    	APP_PRINT("WPA_WPA2_ent");
                    	printf("WPA_WPA2_ent");
                    	break;

                    case eWiFiSecurityWPA2_WPA3_ent_ext:
                    	APP_PRINT("WPA2_WPA3_ent");
                    	printf("WPA2_WPA3_ent");
                    	break;

                    case eWiFiSecurityWPA3_ent_ext:
                    	APP_PRINT("WPA3_ent");
                    	printf("WPA3_ent");
                    	break;

                    case eWiFiSecurityWPA3_192B_ent_ext:
                    	APP_PRINT("WPA3_192B_ent");
                    	printf("WPA3_192B_ent");
                    	break;

                    case eWiFiSecurityWPA_WPA2_ext:
                    	APP_PRINT("WPA_WPA2");
                    	printf("WPA_WPA2");
                    	break;

                    case eWiFiSecurityWPA2_WPA3_ext:
                    	APP_PRINT("WPA2_WPA3");
                        printf("WPA2_WPA3");
                        break;

                    case eWiFiSecurityWPA3_OWE_ext:
                    	APP_PRINT("WPA3_OWE");
                    	printf("WPA3_OWE");
                    	break;

                    default:
                        APP_PRINT("-");
                        printf("-");
                        break;
                }
            }
        }

        APP_PRINT("\n=================================================================\n");
    }
}

/***********************************************************************************************************************
 * Function Name: wifi_normal_scan_example
 * Description  : Performs normal Wi-Fi scan and displays available APs.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
void wifi_normal_scan_example(void)
{
    WIFIReturnCode_t wifi_err;
    WIFIDeviceMode_t wifi_mode_station = 0;   /* eWiFiModeStation */

    memset(wifi_scan_data, 0, sizeof(WIFIScanResult_t) *MAX_WIFI_SCAN_RESULTS);

    wifi_err = WIFI_SetMode(wifi_mode_station);
    assert(eWiFiSuccess == wifi_err);

    wifi_err = WIFI_Disconnect();
    assert(eWiFiSuccess == wifi_err);

    vTaskDelay(100);

    APP_PRINT("\nWi-Fi normal scanning started..\n");
    printf("\nWi-Fi normal scanning started..\n");

    wifi_err = WIFI_Scan(&wifi_scan_data[0], MAX_WIFI_SCAN_RESULTS);
    assert(eWiFiSuccess == wifi_err);

    display_scan_result(wifi_scan_data);

    APP_PRINT("\nWi-Fi normal scanning Done..\n");
    printf("\nWi-Fi normal scanning Done..\n");
}
