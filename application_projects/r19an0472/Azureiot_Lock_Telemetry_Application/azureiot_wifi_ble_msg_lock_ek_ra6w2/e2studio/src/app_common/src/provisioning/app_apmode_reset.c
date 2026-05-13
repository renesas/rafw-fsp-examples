/**
 ****************************************************************************************
 *
 * @file app_apmode_reset.c
 *
 * @brief Reset API for Soft-AP mode or station mode on provisioning
 *
 * @see app_provision.h
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

#if defined (__SUPPORT_WIFI_PROVISIONING__)

#include "nvedit.h"
#include "environ.h"
#include "sys_feature.h"
#include "da16x_system.h"
#include "command.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_network_common.h"
#include "da16x_ping.h"
#include "da16x_cert.h"

#include "common_utils.h"
#include "app_provision.h"
#include "app_common_util.h"
#include "app_common_memory.h"

#include "app_thing_manager.h"

#if defined ( __SUPPORT_AWS_IOT__)
#include "command_net.h"
#endif // ( __SUPPORT_AWS_IOT__)

/*
 * DEFINES
 ****************************************************************************************
 */

#define CON_MAX_PASSKEY_LEN 64 /** @def Maximum number of passkey */

#define MODEL_NAME "DA16200" /** @def spare SSID */

extern softap_config_t *ap_config_param; // < a structure for AP mode setting 

extern int gen_default_psk(char *default_psk);   // < generate password

/**
 ****************************************************************************************
 * @brief set ssid without mac address 
 * @param[in] prefix - ssid base string
 * @param[in] iface - network define
 * @param[in] quotation - quotation
 * @param[out] ssid - ssid string
 * @param[out] size - ssid string 
 * @return void
 ****************************************************************************************
 */
int gen_ssid_wo_mac(char *prefix, int iface, int quotation, char *ssid, int size)
{
    DA16X_UNUSED_ARG(iface);

    if ((quotation ? (int)(strlen(prefix) + 7 + 2) : (int)(strlen(prefix) + 7)) > size) {
        if (strlen(prefix) >= (MAX_SSID_LEN)) {
            sprintf(ssid, "%s%s%s", quotation ? "\"" : "", prefix, quotation ? "\"" : "");
            return 0;
        } else {
            return -1;
        }
    }

    snprintf(ssid, (size_t)((size > (MAX_SSID_LEN + 2)) ? (quotation ? (MAX_SSID_LEN + 2) : MAX_SSID_LEN) : (size)), "%s%s%s",
        quotation ? "\"" : "", prefix, quotation ? "\"" : "");

    return 0;
}

/**
 ****************************************************************************************
 * @brief set ap mode after rebooting 
 * @param[in] _isSecurity  exist security mode or not ,0 : AP_OPEN_MODE  1 : AP_SECURITY_MODE for APP_PROVISION
 * @return void
 ****************************************************************************************
 */
void app_set_customer_ap_configure(int32_t _security)
{
    APRINTF("[%s] set AP config mode = %d \n", __func__, _security);

    ap_config_param->customer_cfg_flag = MODE_ENABLE;

    sprintf(ap_config_param->ssid_name, "%s", PREDEFINE_SSID);

    if (_security == AP_SECURITY_MODE || _security == AP_OPEN_MODE) {
        ap_config_param->auth_type = (char)_security; ///< AP_OPEN_MODE, AP_SECURITY_MODE
    } else {
        SYS_ASSERT(0);
    }

    if (ap_config_param->auth_type == AP_SECURITY_MODE) {
        sprintf(ap_config_param->psk, "%s", PREDEFINE_PW);
    }

    /*
     * Country Code : Default is "US"
     */
    sprintf(ap_config_param->country_code, "%s", "KR");

    /*
     * Network IP address configuration
     */
    ap_config_param->customer_ip_address = IPADDR_CUSTOMER;
    if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER) {
        sprintf(ap_config_param->ip_addr, "%s", "10.0.0.1");
        sprintf(ap_config_param->subnet_mask, "%s", "255.255.255.0");
        sprintf(ap_config_param->default_gw, "%s", "10.0.0.1");
        sprintf(ap_config_param->dns_ip_addr, "%s", "8.8.8.8");
    }

    /*
     * DHCP Server configuration
     */
    ap_config_param->customer_dhcpd_flag = DHCPD_CUSTOMER;
    if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER) {
        ap_config_param->dhcpd_lease_time = 3600;
        sprintf(ap_config_param->dhcpd_start_ip, "%s", "10.0.0.2");
        sprintf(ap_config_param->dhcpd_end_ip, "%s", "10.0.0.11");
        sprintf(ap_config_param->dhcpd_dns_ip_addr, "%s", "8.8.8.8");
    }
}

/**
 ****************************************************************************************
 * @brief reboot AP mode 
 * @param[in] _apMode  set mode to SYSMODE_AP_ONLY or SYSMODE_STA_N_AP (concurrent mode)
 * @param[in] _isSecurity   exist security mode or not ,0 : AP_OPEN_MODE  1 : AP_SECURITY_MODE 
 * @param[in] _removeNV   NVRAM remove or not, 0 : not run factory_reset , 1 : run factory_reset 
 * @return void
 ****************************************************************************************
 */
void app_reboot_ap_mode(int32_t _apMode, int32_t _isSecurity, int32_t _removeNV)
{
    char default_ssid[33];
    char default_psk[CON_MAX_PASSKEY_LEN];

    char *APPThingName = NULL;
#if defined ( __SUPPORT_AZURE_IOT__ )
    char *APPDevPrimaryKey = NULL;
    char *APPHostName = NULL;
    char *APPIotHubConnString = NULL;
#endif  // __SUPPORT_AZURE_IOT__
#if defined ( __SUPPORT_AWS_IOT__)
    int stat = 0;
    size_t readLen = CERT_MAX_LENGTH;
    int format = DA16X_CERT_FORMAT_NONE;
    int private_key_len = 0;
    int certificate_key_len = 0;
    char *AppPrivateKey = NULL;
    char *AppCertificateKey = NULL;
#endif // ( __SUPPORT_AWS_IOT__)

    char *tempChar = NULL;
    int tempLen = 0;

    if (read_nvram_string(APP_NVRAM_CONFIG_THINGNAME) != NULL) {
        tempChar = getAppThingName();
        tempLen = strlen(tempChar);

        APPThingName = (char*)app_common_malloc(tempLen + 1);
        memset(APPThingName, 0x00, tempLen + 1);
        strcpy(APPThingName, tempChar);
    }

#if defined ( __SUPPORT_AZURE_IOT__ )
    if (read_nvram_string(APP_NVRAM_CONFIG_DEV_PRIMARY_KEY)!=NULL) {
        tempChar = getAppDevPrimaryKey();
        tempLen = strlen(tempChar);

        APPDevPrimaryKey = (char*)app_common_malloc(tempLen + 1);
        memset(APPDevPrimaryKey, 0x00, tempLen + 1);
        strcpy(APPDevPrimaryKey, tempChar);
    }

    if (read_nvram_string(APP_NVRAM_CONFIG_HOST_NAME)!=NULL) {
        tempChar = getAppHostName();
        tempLen = strlen(tempChar);

        APPHostName = (char*)app_common_malloc(tempLen + 1);
        memset(APPHostName, 0x00, tempLen + 1);
        strcpy(APPHostName, tempChar);
    }

    if (read_nvram_string(APP_NVRAM_CONFIG_IOTHUB_CONN_STRING)!=NULL) {
        tempChar = getAppIotHubConnString();
        tempLen = strlen(tempChar);

        APPIotHubConnString = (char*)app_common_malloc(tempLen + 1);
        memset(APPIotHubConnString, 0x00, tempLen + 1);
        strcpy(APPIotHubConnString, tempChar);
    }
#endif
#if defined ( __SUPPORT_AWS_IOT__)
    if (getUseFPstatus()) {
        AppPrivateKey = (char*)app_common_malloc(CERT_MAX_LENGTH);
        stat = da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY, &format, (unsigned char*)AppPrivateKey, &readLen);
        if (stat == 0) {
            private_key_len = readLen;
        }

        AppCertificateKey = (char*)app_common_malloc(CERT_MAX_LENGTH);
        readLen = CERT_MAX_LENGTH;
        format = DA16X_CERT_FORMAT_NONE;
        stat = da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT, &format, (unsigned char*)AppCertificateKey, &readLen);
        if (stat == 0) {
            certificate_key_len = readLen;
        }
    }
#endif // ( __SUPPORT_AWS_IOT__)
    APRINTF("\n\33[31mDA16200 factory reset AP mode = %d (\"%s\")....\n", _apMode,
        (_apMode == SYSMODE_AP_ONLY) ? "AP_ONLY" : (_apMode == SYSMODE_STA_N_AP ? "STA_N_AP" : "???"));

    if (_removeNV == 1) {
        APRINTF("\nFactory Reset\n");
        factory_reset(0);
    }

    vTaskDelay(10);

    // re-write Thing name
    if (APPThingName != NULL) {
        write_tmp_nvram_string((const char*)APP_NVRAM_CONFIG_THINGNAME, APPThingName);
        PUTC('.');
        vTaskDelay(1);

        app_common_free(APPThingName);
    }

#if defined ( __SUPPORT_AZURE_IOT__ )
    if (APPDevPrimaryKey != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_DEV_PRIMARY_KEY, APPDevPrimaryKey);
        PUTC('.'); vTaskDelay(1);

        app_common_free(APPDevPrimaryKey);
    }

    if (APPHostName != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_HOST_NAME, APPHostName);
        PUTC('.'); vTaskDelay(1);

        app_common_free(APPHostName);
    }

    if (APPIotHubConnString != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_IOTHUB_CONN_STRING, APPIotHubConnString);
        PUTC('.'); vTaskDelay(1);

        app_common_free(APPIotHubConnString);
    }
#endif  // __SUPPORT_AZURE_IOT__
#if defined ( __SUPPORT_AWS_IOT__)
    if (getUseFPstatus()) {
        if ((UCHAR)(AppPrivateKey[0]) != 0xFF && private_key_len > 0) {
            da16x_cert_write(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY, format, (unsigned char*)AppPrivateKey, private_key_len);
            vTaskDelay(1);
            app_common_free(AppPrivateKey);
        }

        if ((UCHAR)(AppCertificateKey[0]) != 0xFF && certificate_key_len > 0) {
            da16x_cert_write(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT, format, (unsigned char*)AppCertificateKey, certificate_key_len);
            vTaskDelay(1);
            app_common_free(AppCertificateKey);
        }
    }
#endif // ( __SUPPORT_AWS_IOT__)

    /*
     * Default value set-up
     */
    write_tmp_nvram_int((const char*)NVR_KEY_PROFILE_1, (int)MODE_ENABLE);
    PUTC('.');
    vTaskDelay(1);

    write_tmp_nvram_int((const char*)"N1_mode", (int)2);
    PUTC('.');
    vTaskDelay(1);

    write_tmp_nvram_int((const char*)NVR_KEY_SYSMODE, (int)_apMode);
    PUTC('.');
    vTaskDelay(1);

    write_tmp_nvram_int("N1_mode", 2);
    PUTC('.');
    vTaskDelay(1);

    /*
     *Run customer's configuration
     */
    app_set_customer_ap_configure(_isSecurity);
    if (ap_config_param->customer_cfg_flag == MODE_DISABLE) {
        APRINTF("\napps_reboot_ap_mode  MODE_DISABLE ...\n");
        /* SSID+MACADDRESS */
    } else { // Customer configuration
        char tmp_buf[128] = {0, };

        APRINTF("\napps_reboot_ap_mode  Customer configuration ...\n");

        /*
         * Prefix + Mac Address
         */
        if (strlen(ap_config_param->ssid_name) > 0) {
            strncpy(tmp_buf, ap_config_param->ssid_name, PROV_MAX_SSID_LEN);
        } else {
            sprintf(tmp_buf, "%s", MODEL_NAME);
        }

        memset(default_ssid, 0, 33);

        // predefined fixed SSID
        if (gen_ssid_wo_mac(tmp_buf, WLAN1_IFACE, 1, default_ssid, sizeof(default_ssid)) == -1) {
            APRINTF("SSID Error\n");
        }

        // PSK
        memset(default_psk, 0, CON_MAX_PASSKEY_LEN);
        if (strlen(ap_config_param->psk) > 0) {
            strncpy(default_psk, ap_config_param->psk, CON_MAX_PASSKEY_LEN);
            default_psk[CON_MAX_PASSKEY_LEN - 1] = '\0';
        } else {
            gen_default_psk(default_psk);
        }

        write_tmp_nvram_string((const char*)NVR_KEY_SSID_1, default_ssid);
        PUTC('.');
        vTaskDelay(1);

        APRINTF("\n default_ssid = %s ..., ap_config_param->ssid_name \n", default_ssid, ap_config_param->ssid_name);

        // PW
        if (strlen(ap_config_param->psk) > 0) {
            char tmp_psk[64 + 3];

            memset(tmp_psk, 0, 64 + 3);
            tmp_psk[0] = 0x22;
            strcpy(&tmp_psk[1], ap_config_param->psk);
            tmp_psk[strlen(ap_config_param->psk) + 1] = 0x22;

            write_tmp_nvram_string((const char*)NVR_KEY_ENCKEY_1, tmp_psk);
            APRINTF("\n PW = %s \n", ap_config_param->psk);

            write_tmp_nvram_string(NVR_KEY_PROTO_1, proto_RSN);
            write_nvram_string(NVR_KEY_AUTH_TYPE_1, MODE_AUTH_WPA_PSK_STR); ///<  WPA-PSK

            APRINTF("\n PW = %s  completed\n", ap_config_param->psk);
        }

        if (strlen(ap_config_param->country_code) > 0) {
            write_tmp_nvram_string((const char*)NVR_KEY_COUNTRY_CODE, ap_config_param->country_code);
        } else {
            write_tmp_nvram_string((const char*)NVR_KEY_COUNTRY_CODE, (const char*)DFLT_AP_COUNTRY_CODE);
        }

        PUTC('.');
        vTaskDelay(1);

        if (ap_config_param->customer_ip_address == IPADDR_CUSTOMER) {
            APRINTF("\napps_reboot_ap_mode  IPADDR_CUSTOMER...\n");

            write_tmp_nvram_string((const char*)NVR_KEY_IPADDR_1, ap_config_param->ip_addr);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_NETMASK_1, ap_config_param->subnet_mask);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_GATEWAY_1, ap_config_param->default_gw);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_DNSSVR_1, ap_config_param->dns_ip_addr);
            PUTC('.');
            vTaskDelay(1);
        } else {
            APRINTF("\n\apps_reboot_ap_mode  IPADDR_CUSTOMER else...\n");
        }

        if (ap_config_param->customer_dhcpd_flag == DHCPD_CUSTOMER) {
            APRINTF("\napps_reboot_ap_mode  customer_dhcpd_flag == DHCPD_CUSTOMER..\n");

            write_tmp_nvram_int((const char*)NVR_KEY_DHCPD, MODE_ENABLE);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_int((const char*)NVR_KEY_DHCPD_IPCNT, 10);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_int((const char*)NVR_KEY_DHCP_TIME, ap_config_param->dhcpd_lease_time);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_DHCP_S_IP, ap_config_param->dhcpd_start_ip);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_DHCP_E_IP, ap_config_param->dhcpd_end_ip);
            PUTC('.');
            vTaskDelay(1);

            write_tmp_nvram_string((const char*)NVR_KEY_DHCP_DNS_IP, ap_config_param->dhcpd_dns_ip_addr);
            PUTC('.');
            vTaskDelay(1);
        } else {
            APRINTF("\napps_reboot_ap_mode  customer_dhcpd_flag == DHCPD_CUSTOMER  else..\n");
        }
    }

    if (save_tmp_nvram()) {
        APRINTF("\nOK\n");
    } else {
        APRINTF("\nError\n");
    }

    APRINTF("\33[0m\n\n");

    vTaskDelay(30);

    reboot_func(SYS_REBOOT_POR);

    /*
     * Wait for system-reboot
     */
    while (1) {
        vTaskDelay(10);
    }
}

/**
 ****************************************************************************************
 * @brief reboot STA mode 
 * @param[in] _factoryReset   factory reset mode is erased NV memory of app environment (1 is factory reset)
 * @param[in] _config  structure pointer having provisioning information such as SSID, PW, DPM mode, etc
 * @return void
 ****************************************************************************************
 */
void app_reset_to_station_mode(int32_t _factoryReset, app_prov_config_t *_config)
{
    char *APPThingName = NULL;
    char tmp_psk[PROV_MAX_PW_LEN + 3];

#if defined ( __SUPPORT_AZURE_IOT__ )
    char *APPDevPrimaryKey = NULL; 
    char *APPHostName = NULL; 
    char *APPIotHubConnString = NULL; 
#endif  // __SUPPORT_AZURE_IOT__
#if defined ( __SUPPORT_AWS_IOT__)
    int stat = 0;
    size_t readLen = 0;
    int format = DA16X_CERT_FORMAT_NONE;
    int private_key_len = 0;
    int certificate_key_len = 0;
    char *AppPrivateKey = NULL;
    char *AppCertificateKey = NULL;
#endif // ( __SUPPORT_AWS_IOT__)

    char *tempChar = NULL;
    int tempLen = 0;

    if (_config == NULL) {
        APRINTF("\n\n>> passed parameter invalid(NULL) \n");
        SYS_ASSERT(0);

        return;
    }

    if (read_nvram_string(APP_NVRAM_CONFIG_THINGNAME) != NULL) {
        tempChar = getAppThingName();
        tempLen = strlen(tempChar);

        APPThingName = (char*)app_common_malloc(tempLen + 1);
        memset(APPThingName, 0x00, tempLen + 1);
        strcpy(APPThingName, tempChar);
    }

#if defined ( __SUPPORT_AZURE_IOT__ )
    if (read_nvram_string(APP_NVRAM_CONFIG_DEV_PRIMARY_KEY)!=NULL) {
        tempChar = getAppDevPrimaryKey();
        tempLen = strlen(tempChar);

        APPDevPrimaryKey = (char*)app_common_malloc(tempLen + 1);
        memset(APPDevPrimaryKey, 0x00, tempLen + 1);
        strcpy(APPDevPrimaryKey, tempChar);
    }

    if (read_nvram_string(APP_NVRAM_CONFIG_HOST_NAME)!=NULL) {
        tempChar = getAppHostName();
        tempLen = strlen(tempChar);

        APPHostName = (char*)app_common_malloc(tempLen + 1);
        memset(APPHostName, 0x00, tempLen + 1);
        strcpy(APPHostName, tempChar);
    }

    if (read_nvram_string(APP_NVRAM_CONFIG_IOTHUB_CONN_STRING) != NULL) {
        tempChar = getAppIotHubConnString();
        tempLen = strlen(tempChar);

        APPIotHubConnString = (char*)app_common_malloc(tempLen + 1);
        memset(APPIotHubConnString, 0x00, tempLen + 1);
        strcpy(APPIotHubConnString, tempChar);
    }
#endif  // __SUPPORT_AZURE_IOT__
#if defined ( __SUPPORT_AWS_IOT__)
    if (getUseFPstatus()) {
        AppPrivateKey = (char*)app_common_malloc(CERT_MAX_LENGTH);
        stat = da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY, &format, (unsigned char*)AppPrivateKey, &readLen);
        if (stat == 0) {
            private_key_len = readLen;
        }

        AppCertificateKey = (char*)app_common_malloc(CERT_MAX_LENGTH);
        readLen = CERT_MAX_LENGTH;
        format = DA16X_CERT_FORMAT_NONE;
        stat = da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT, &format, (unsigned char*)AppCertificateKey, &readLen);
        if (stat == 0) {
            certificate_key_len = readLen;
        }
    }
#endif // ( __SUPPORT_AWS_IOT__)
    if (_factoryReset == 1) {
        if (get_prov_feature() != APP_AT_CMD) {
            APRINTF("\n\nfactory_reset ...\n");
            factory_reset(0);
        }

        vTaskDelay(10); // 100msec
    } else {
        // Clear All NVRAM area
        da16x_clearenv(ENVIRON_DEVICE, "app");

        APRINTF("\n>> Initialize NVRAM done ...\n");
        vTaskDelay(50); // 500msec
    }

    // re-write Thing name
    if (APPThingName != NULL) {
        write_tmp_nvram_string((const char*)APP_NVRAM_CONFIG_THINGNAME, APPThingName);
        PUTC('.');
        vTaskDelay(1);

        app_common_free(APPThingName);
    }

#if defined ( __SUPPORT_AZURE_IOT__ )
    if (APPDevPrimaryKey != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_DEV_PRIMARY_KEY, APPDevPrimaryKey);
        PUTC('.');
        vTaskDelay(1);
        app_common_free(APPDevPrimaryKey);
    }

    if (APPHostName != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_HOST_NAME, APPHostName);
        PUTC('.');
        vTaskDelay(1);
        app_common_free(APPHostName);
    }

    if (APPIotHubConnString != NULL) {
        write_tmp_nvram_string((const char *)APP_NVRAM_CONFIG_IOTHUB_CONN_STRING, APPIotHubConnString);
        PUTC('.');
        vTaskDelay(1);
        app_common_free(APPIotHubConnString);
    }
#endif
#if defined ( __SUPPORT_AWS_IOT__)
    if (getUseFPstatus()) {
        if ((UCHAR)(AppPrivateKey[0]) != 0xFF && private_key_len > 0) {
            da16x_cert_write(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY, format, (unsigned char*)AppPrivateKey, private_key_len);
            vTaskDelay(1);
            app_common_free(AppPrivateKey);
        }

        if ((UCHAR)(AppCertificateKey[0]) != 0xFF && certificate_key_len > 0) {
            da16x_cert_write(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT, format, (unsigned char*)AppCertificateKey, certificate_key_len);
            vTaskDelay(1);
            app_common_free(AppCertificateKey);
        }
    }

#endif // ( __SUPPORT_AWS_IOT__)
    // For Wi-Fi connection
    if (_config->auth_type == 0) {
        APRINTF("\n\n>>> reboot station mode ssid:%s \n", _config->ssid);
    } else {
        APRINTF("\n\n>>> reboot station mode ssid:%s , pw:%s\n", _config->ssid, _config->psk[0] <= 0x7F ? _config->psk : "???");
    }

    // SSID
    if (strlen(_config->ssid) > 0) {
        char tmp_ssid[PROV_MAX_SSID_LEN + 3];

        memset(tmp_ssid, 0, PROV_MAX_SSID_LEN + 3);
        tmp_ssid[0] = 0x22;
        strcpy(&tmp_ssid[1], _config->ssid);
        tmp_ssid[strlen(_config->ssid) + 1] = 0x22;

        write_tmp_nvram_string(NVR_KEY_SSID_0, tmp_ssid);
    }

    // PW
    if (strlen(_config->psk) > 0) {
        memset(tmp_psk, 0, PROV_MAX_PW_LEN + 3);
        tmp_psk[0] = 0x22;
        strcpy(&tmp_psk[1], _config->psk);
        tmp_psk[strlen(_config->psk) + 1] = 0x22;
    }

    // AUTH
    APRINTF("\n>> set auth_type : %d \n", _config->auth_type);
    switch (_config->auth_type) {
    case eWiFiSecurityOpen: ///< NONE
        write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, MODE_AUTH_OPEN_STR);
        break;

    case eWiFiSecurityWEP: ///< WEP
        write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, "NONE");
        write_tmp_nvram_int(NVR_KEY_WEPINDEX_0, 0); //< wep-index  default set 0
        write_tmp_nvram_string(NVR_KEY_WEPKEY0_0, tmp_psk); // add PW from above
        break;

    case eWiFiSecurityWPA: ///< WPA-PSK
        write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, MODE_AUTH_WPA_PSK_STR);
        write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_WPA);
        delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0); //  set CC_VAL_ENC_AUTO
        write_tmp_nvram_string(NVR_KEY_ENCKEY_0, tmp_psk); // add PW from above
        break;

    case eWiFiSecurityWPA2: 	///< WPA2-PSK
        write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, MODE_AUTH_WPA_PSK_STR);
        write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
        write_tmp_nvram_string(NVR_KEY_ENCKEY_0, tmp_psk); // add PW from above
        break;
#if defined (__SUPPORT_WPA3_PERSONAL__)
    case eWiFiSecurityOWE: 		///< WPA3 OWE
    	write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, "OWE");
		delete_tmp_nvram_env("N0_wep_key0");
		delete_tmp_nvram_env("N0_wep_key1");
		delete_tmp_nvram_env("N0_wep_key2");
		delete_tmp_nvram_env("N0_wep_key3");
		delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
    	break;

	case eWiFiSecuritySAE: 		///< WPA3 SAE
	case eWiFiSecurityRSN_SAE:	///< WPA2 (RSN) & WPA3 SAE
		if (_config->auth_type == eWiFiSecuritySAE) {
			write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, "SAE");
		} else {
			write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, "WPA-PSK SAE");
		}

		write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
		delete_tmp_nvram_env("N0_wep_key0");
		delete_tmp_nvram_env("N0_wep_key1");
		delete_tmp_nvram_env("N0_wep_key2");
		delete_tmp_nvram_env("N0_wep_key3");
		delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);

		write_tmp_nvram_string(NVR_KEY_SAE_PASS_0, tmp_psk);
		break;
#endif	// (__SUPPORT_WPA3_PERSONAL__)
#if defined (__SUPPORT_WPA_ENTERPRISE__)
    case eWiFiSecurityWPA_EAP:               	///< WPA Enterprise
    case eWiFiSecurityWPA2_EAP:              	///< WPA2 Enterprise
    case eWiFiSecurityWPA_AUTO_EAP:          	///< WPA & WPA2 Enterprise
    case eWiFiSecurityWPA3_EAP:              	///< WPA3 Enterprise
    case eWiFiSecurityWPA2_AUTO_EAP:        	///< WPA2 & WPA3 Enterprise
    	//eap_ID
		if (strlen(_config->ent_id) > 0) {
			char eap_id[PROV_MAX_SSID_LEN + 3];

			memset(eap_id, 0, PROV_MAX_SSID_LEN + 3);
			eap_id[0] = 0x22;
			strcpy(&eap_id[1], _config->ent_id);
			eap_id[strlen(_config->ent_id) + 1] = 0x22;

			write_tmp_nvram_string("N0_identity", eap_id);
		}

		// eap_PW
		if (strlen(_config->ent_pw) > 0) {
			char eap_pw[PROV_MAX_PW_LEN +3];
			memset(eap_pw, 0, PROV_MAX_PW_LEN + 3);
			eap_pw[0] = 0x22;
			strcpy(&eap_pw[1], _config->ent_pw);
			eap_pw[strlen(_config->ent_pw) + 1] = 0x22;

			write_tmp_nvram_string("N0_password", eap_pw);
		}

		// encrypt
		delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0);	// AUTO: TKIP CCMP

		// wpa_eap
    	write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, key_mgmt_WPA_EAP);
    	if (_config->auth_type == eWiFiSecurityWPA_EAP) {
			write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_WPA);
		} else if (_config->auth_type == eWiFiSecurityWPA2_EAP) {
			write_tmp_nvram_string(NVR_KEY_PROTO_0, proto_RSN);
		} else { //eWiFiSecurityWPA_AUTO_EAP
			delete_tmp_nvram_env(NVR_KEY_PROTO_0);	// WPA_AUTO_EAP
//			write_tmp_nvram_string(NVR_KEY_PROTO_0, "WPA RSN");		// WPA_AUTO_EAP
		}

    	// Phase 1 auth type
    	if (_config->ent_type == TYPE_EAP_DEFAULT) {
			write_tmp_nvram_string("N0_eap", "PEAP TTLS FAST");
		} else if (_config->ent_type == TYPE_EAP_PEAP) {
			write_tmp_nvram_string("N0_eap", "PEAP");
		} else if (_config->ent_type == TYPE_EAP_TTLS) {
			write_tmp_nvram_string("N0_eap", "TTLS");
		} else if (_config->ent_type == TYPE_EAP_FAST) {
			write_tmp_nvram_string("N0_eap", "FAST");
			write_tmp_nvram_string("N0_phase1", "'fast_provisioning=2'");
		} else {
			write_tmp_nvram_string("N0_eap", "TLS");
		}
		// Erase old fast_pac, fast_pac_len for EAP-FAST
		delete_tmp_nvram_env("fast_pac");
		delete_tmp_nvram_env("fast_pac_len");

		// Phase 2 auth protocol
		if (_config->ent_protocol == PROTO_EAP_PHASE2_MIX) {
			write_tmp_nvram_string("N0_phase2", "'auth=MSCHAPV2 GTC'");
		} else if (_config->ent_protocol == PROTO_EAP_MSCHAPV2) {
			write_tmp_nvram_string("N0_phase2", "'auth=MSCHAPV2'");
		} else {
			write_tmp_nvram_string("N0_phase2", "'auth=GTC'");
		}
    	break;
#endif //(__SUPPORT_WPA_ENTERPRISE__)
    case eWiFiSecurityWPA_AUTO:
    default:
        write_tmp_nvram_string(NVR_KEY_AUTH_TYPE_0, MODE_AUTH_WPA_PSK_STR);
        write_tmp_nvram_string(NVR_KEY_PROTO_0, "WPA RSN");
        write_tmp_nvram_string(NVR_KEY_ENCKEY_0, tmp_psk); // add PW from above
        break;
    }

    // Hidden SSID
    if (_config->hidden) {
        write_tmp_nvram_int("N0_scan_ssid", 1);
    }

    // For System Running mode
    write_tmp_nvram_int(NVR_KEY_SYSMODE, SYSMODE_STA_ONLY);
    write_tmp_nvram_int(NVR_KEY_PROFILE_0, MODE_ENABLE);            // Profile #0

    // Country
    if (strlen(_config->country) > 0) {
        write_tmp_nvram_string(NVR_KEY_COUNTRY_CODE, _config->country);
    } else {
        write_tmp_nvram_string(NVR_KEY_COUNTRY_CODE, DFLT_STA_COUNTRY_CODE);
    }

    // Static or DHCP
    if (_config->ip_addr_mode == 1) {
        write_tmp_nvram_int(NVR_KEY_NETMODE_0, ENABLE_STATIC_IP);
    } else {
        write_tmp_nvram_int(NVR_KEY_NETMODE_0, ENABLE_DHCP_CLIENT);
    }

    // For SNTP client
    if (_config->sntp_flag == TRUE) {
        write_tmp_nvram_int(NVR_KEY_SNTP_C, _config->sntp_flag);

        if (strlen(_config->sntp_server) > 0) {
            write_tmp_nvram_string(NVR_KEY_SNTP_SERVER_DOMAIN, _config->sntp_server);
        } else {
            write_tmp_nvram_string(NVR_KEY_SNTP_SERVER_DOMAIN, DFLT_SNTP_SERVER_DOMAIN);
        }

        if (_config->sntp_period > 0) {
            write_tmp_nvram_int(NVR_KEY_SNTP_SYNC_PERIOD, _config->sntp_period);
        } else {
            write_tmp_nvram_int(NVR_KEY_SNTP_SYNC_PERIOD, DFLT_SNTP_SYNC_PERIOD);
        }
    }

    // For DPM mode
    if (_config->dpm_mode == 1) {
        write_tmp_nvram_int(NVR_KEY_DPM_MODE, _config->dpm_mode);

        if (app_get_DPM_set_value(TYPE_SLEEP_MODE) != 3) {
            if (_config->dpm_ka > 0) {
                write_tmp_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, _config->dpm_ka);
            } else {
                write_tmp_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, DFLT_DPM_KEEPALIVE_TIME);
            }

            if (_config->dpm_user_wu > 0) {
                write_tmp_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, _config->dpm_user_wu);
            } else {
                write_tmp_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, MIN_DPM_USER_WAKEUP_TIME);
            }

            if (_config->dpm_tim_wu > 0) {
                write_tmp_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, _config->dpm_tim_wu);
            } else {
                write_tmp_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, DFLT_DPM_TIM_WAKEUP_COUNT);
            }
        } else {
            APRINTF("[SleepMode 3] Set DPM , KA : %d , UW : %d , TW : %d \n", app_get_DPM_set_value(TYPE_DPM_KEEP_ALIVE),
                app_get_DPM_set_value(TYPE_USER_WAKE_UP), app_get_DPM_set_value(TYPE_TIM_WAKE_UP));

            write_tmp_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, app_get_DPM_set_value(TYPE_DPM_KEEP_ALIVE));
            write_tmp_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, app_get_DPM_set_value(TYPE_USER_WAKE_UP));
            write_tmp_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, app_get_DPM_set_value(TYPE_TIM_WAKE_UP));
        }
    } else {
        write_tmp_nvram_int(NVR_KEY_DPM_MODE, 0);
    }

    if (_config->prov_type > SYSMODE_TEST) {
        APRINTF(">>> Save Sleep mode to %d \n", app_get_DPM_set_value(TYPE_SLEEP_MODE));
        write_tmp_nvram_int(SLEEP_MODE_FOR_NARAM, app_get_DPM_set_value(TYPE_SLEEP_MODE));

        APRINTF(">>> Save Sleep mode2 RTC time %d \n", app_get_DPM_set_value(TYPE_RTC_TIME));
        write_tmp_nvram_int(SLEEP_MODE2_RTC_TIME, app_get_DPM_set_value(TYPE_RTC_TIME));
    }

    // Save
    {
        extern int save_tmp_nvram(void);
        APRINTF(">>> Save profile\n");

        save_tmp_nvram();

        vTaskDelay(30);
    }

    // Check Reboot status
    if (_config->auto_restart_flag == TRUE) {
        PRINTF(">>> System reboot ...\n");
        vTaskDelay(10);

        reboot_func(SYS_REBOOT);

        while (1) {
            vTaskDelay(10);
        }
    }
}

#endif  // (__SUPPORT_WIFI_PROVISIONING__)

/* EOF */
