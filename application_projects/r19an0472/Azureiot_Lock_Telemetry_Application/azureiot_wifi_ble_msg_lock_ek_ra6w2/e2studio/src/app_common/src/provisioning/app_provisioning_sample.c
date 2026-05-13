/**
 ****************************************************************************************
 *
 * @file app_provisioning_sample.c
 *
 * @brief Sample application how to get softap provisioning via TCP or TLS
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

/**
 ************************************************************************************
 * @To test softap provisioning change below definitions
 * @ config_generic_sdk.h : #define __SUPPORT_WIFI_PROVISIONING__
 ************************************************************************************
 */



#include "nvedit.h"
#include "environ.h"
#include "sys_feature.h"
#include "da16x_system.h"


#include "json.h"
#include "da16x_system.h"
#include "common_def.h"
#include "da16x_network_main.h"
#include "common_config.h"
#include "app_common_memory.h"
#include "app_common_util.h"
#include "app_provision.h"
#include "app_thing_manager.h"
#include "command.h" 
#include "da16x_sys_watchdog.h"

#if defined (__SUPPORT_ATCMD__)
#include "atcmd.h"
#endif

/**
 * @ breif port for phone connection
 * @note Phone port should be changed same port number if this port is changed.
 */
#define TCP_PROVISION_PORT_NUM			9999

/**
 * @def after finishing provisioning and reboot , will be start mode with DPM or not
 */
#define STATION_MODE_NO_DPM				0
#define STATION_MODE_DPM				1

/**
 * @def max message number between receiving thread  and sending thread
 */
#define PROVISION_MSG_CMD				4

/*!max buffer size of receiving message from phone    */
#define PROVISION_TCP_RX_BUF_SZ			1024

#define APP_SOFTAP_PROV_NAME			"APROV_TCP"
#define APP_SOFTAP_APROV_NAME			"APROV_TLS"
#define APP_SOFTAP_PROV_STACK_SZ		1024

/**
 * @def max SSID size for home AP
 */
#define MAX_AP_SSID_SIZE				64

/**
 * @def max password size for home AP
 */
#define MAX_AP_PASSWORD_SIZE			64

/**
 * @def max customer server side for home AP
 */
#define MAX_CUSTOMER_SERVER_SIZE		128

/**
 * @enum connection enum/step value from phone
 */
typedef enum {
    CONNECTED = 0,
    SET_AP_SSID_PW,
    REQ_HOMEAP_RESULT,
    REQ_RESCAN,
    REQ_REBOOT,
    REQ_SET_DPM,
    REQ_SOCKET_TYPE,	///<  TLS : 1, TCP : 0
    CMD_ERROR,
} AP_CMD_TYPE;

/**
 * @struct info
 * @brief message structure between receiving thread and sending thread
 */
typedef struct _provisionData {
    int32_t status;
    void *userData;
} provisionData;

/*!TCP socket point for provisioning TCP communication   */
int provision_TCP_socket = -1;
int provision_TCP_csocket = -1;

/*!mutex for TCP packet sending    */
app_mutex provision_mutex;

/*!message queue  between receiving thread  and sending thread   */
QueueHandle_t provisionMsgQ = NULL;

/** 
 * @var SSID received from phone
 */
char *new_SSID = NULL;

/** 
 * @var SSID size received from phone
 */
int32_t new_SSID_size = 0;

/**
 * @var password of SSID received from phone
 */
char *new_pw = NULL;

/** 
 * @var password size of SSID received from phone
 */
int32_t new_pw_size = 0;

/** 
 * @var server url received from phone
 */
char *server_URL = NULL;

/**
 * @var password size of SSID received from phone
 */
int32_t server_URL_size = 0;

/**
 * @var char buffer for AP list
 */
char *ap_list_buffer = NULL;

/**
 * @var buffer for sending AP list to  Json format
 */
cJSON *ap_list;

/**
 * @var Whether trying to connect AP or not
 */
static int32_t check_connection_ap = 0;

/** 
 * @var HIdden SSID connection flag get from Mobile App.
 */
int32_t use_Hidden_SSID = 0;

/** 
 * @var Security Type get from Mobile App.
 */
int32_t Security_Type = -1;

/** 
 * @var Provisioning Type get from app_start_provisioning mode.
 *    Generic SDK:1,
 *    Platform AWS: Generic:10, ATCMD:11, ...
 *    Platform AZURE: Generic:20, ATCMD:21, ...
 */
int8_t Provision_Type = -1;

/* TLS config struct initialize*/
static prov_tls_svr_conf_t app_prov_tls_svr_config = {0, };

#if defined (__SUPPORT_WPA_ENTERPRISE__)

/**
 * @var Provisioning Type get from app_start_provisioning mode.
 *    type 1:PEAP or TTLS or FAST (Recommend), 2: PEAP, 3: TTLS, 4: FAST, 5:TLS
 */
int8_t eap_type = 0;

/**
 * @var Provisioning Type get from app_start_provisioning mode.
 * protocol 1: MSCHAPv2 or GTC (Recommend), 2: MSCHAPv2, 3: GTC
 */
int8_t eap_protocol = 0;

/**
 * @var EAP ID received from phone
 */

char *eap_ID = NULL;

/**
 * @var EAP Password received from phone
 */

char *eap_PW = NULL;

#endif	// __SUPPORT_WPA_ENTERPRISE__

/*
 * brief DPM mode setting
 */
int sleepModeVal = 0;

/*! When DPM mode is 3, DPM */
int useDpmVal = 0;
int dpmKeepAliveVal = 0;
int userWakeupVal = 0;
int timWakeupVal = 0;
int rtcTime = 0;
int conAPStatus = -1; 
int conSTAStatus = -1; 
int tryConnectOnSTA =0; 

/*
 *  New SoftAP provisioning for security 
 */
#define	HIDDEN_SSID_DETECTION_CHAR	'\t'
#define	CLI_SCAN_RSP_BUF_SIZE		3584
#define	SCAN_RESULT_MAX				30
#define	SCAN_RESULT_BUF_SIZE		4096
#define	SSID_BASE64_LEN_MAX			48

/* TLS data send fucnition to Phone */
static int app_provision_send_tls_data(prov_tls_svr_conf_t *config, char *data, int32_t dataSize);

/**
 * @struct info
 * @brief TLS message struct server and client thread
 */
typedef struct tlsInfo {
    prov_tls_svr_conf_t *config;
    int32_t mode;
} TLSInfo;

/*  0 : tcp,  1 :tls default is 1 */
UINT8 socket_app = 1;
UINT8 socket_dev = 1;

/**
 * @struct info
 * @brief ap list params
 */
typedef struct _APLISTnChannel {
    char *SSID;
    char *freQ;

} APLISTnChannel;

APLISTnChannel aplistwithFreQ[SCAN_CHANNEL_MAX];

/**
 ****************************************************************************************
 * @brief Get DPM set value 
 * @param[in] _type - get  value type 
 * @return  success : set value, fail : -1
 ****************************************************************************************
 */
int app_get_DPM_set_value(GetValueType _type)
{
    if (get_prov_feature() == APP_AT_CMD) {
        int ret = 0;
        char *temStr = NULL;

        switch (_type) {
        case TYPE_SLEEP_MODE:
            temStr = SLEEP_MODE_FOR_NARAM;
            break;

        case TYPE_USE_DPM:
            temStr = USE_DPM_FOR_NARAM;
            break;

        case TYPE_DPM_KEEP_ALIVE:
            temStr = NVR_KEY_DPM_KEEPALIVE_TIME;
            break;

        case TYPE_USER_WAKE_UP:
            temStr = NVR_KEY_DPM_USER_WAKEUP_TIME;
            break;

        case TYPE_TIM_WAKE_UP:
            temStr = NVR_KEY_DPM_TIM_WAKEUP_TIME;
            break;

        case TYPE_RTC_TIME:
            temStr = SLEEP_MODE2_RTC_TIME;
            break;
        }

        if (temStr != NULL) {
            read_nvram_int((const char*)temStr, (int*)&ret);
            return ret;
        }
    } else {
        if (_type == TYPE_SLEEP_MODE)
            return sleepModeVal;

        if (_type == TYPE_USE_DPM)
            return useDpmVal;

        if (_type == TYPE_DPM_KEEP_ALIVE)
            return dpmKeepAliveVal;

        if (_type == TYPE_USER_WAKE_UP)
            return userWakeupVal;

        if (_type == TYPE_TIM_WAKE_UP)
            return timWakeupVal;

        if (_type == TYPE_RTC_TIME)
            return rtcTime;
    }

    APRINTF_E("getDPMSetValue error [Undfined type]\n");

    return -1;
}

/**
 ****************************************************************************************
 * @brief Set Sleep mode and DTIM value 
 * @param[in] _sleepMode - sleepMode setting value 
 * @param[in] _useDpmSet -dpm setting value
 * @param[in] _rtcTime -rtc wakeup setting value
 * @param[in] _dpmKeepAlive -dpm Keep Alive settging value
 * @param[in] _userWakeup -user wakeup setting value
 * @param[in] _timWakeup -tim wakeup setting value
 * @return  void
 ****************************************************************************************
 */
void setSleepModeDtim(int _sleepMode, int _useDpmSet, int _rtcTime, int _dpmKeepAlive, int _userWakeup, int _timWakeup)
{
    if (_sleepMode >= 1 && _sleepMode <= 3) {
        sleepModeVal = _sleepMode;
    } else {
        APRINTF_E("Unsupport Sleep mode error \n");
        return;
    }

    useDpmVal = _useDpmSet;
    dpmKeepAliveVal = _dpmKeepAlive;
    userWakeupVal = _userWakeup;
    timWakeupVal = _timWakeup;
    rtcTime = _rtcTime;

    APRINTF("[Set] SleepMode: %d ,DPM: %d,rtcTime : %d sec,  DPM_KeepAlive: %d, userWakeUp: %d, timWakeUp: %d \n", sleepModeVal,
        useDpmVal, rtcTime, dpmKeepAliveVal, userWakeupVal, timWakeupVal);
}

#if defined (__ENABLE_UNUSED__)
/**
 ****************************************************************************************
 * @brief string duplicate
 * @param[in] str -string to duplicate
 * @return  duplicated string
 ****************************************************************************************
 */
static char* provStrdup(char* str)
{
	int32_t len = 0;
	char *copy = NULL;

	if (str == NULL) {
		return NULL;
	}

	len = strlen((const char *)str) + 1;
	if (!(copy = (char *)malloc(len))) {
		return NULL;
	}

	memcpy(copy, str, len);

	return copy;
}

/**
 ****************************************************************************************
 * @brief ap list arrange by index
 * @param[in] _SSID -ap's ssid
 * @param[in] _freQ -ap's channel freq.
 * @param[in] _index -ap's arrange index 
 * @return  void
 ****************************************************************************************
 */
static void addAPnChannel(char* _SSID, char* _freQ, int32_t _index)
{
	char* ssid = aplistwithFreQ[_index-1].SSID;
	char* freQ = aplistwithFreQ[_index-1].freQ;

	if (ssid != NULL) {
		free(ssid); 
		ssid = NULL;
	}

	if (freQ != NULL) {
		free(freQ); 
		freQ = NULL;
	}

	ssid = provStrdup(_SSID);
	freQ = provStrdup(_freQ);

	aplistwithFreQ[_index-1].SSID = ssid;
	aplistwithFreQ[_index-1].freQ = freQ;
	
}
#endif // __ENABLE_UNUSED__

/**
 ****************************************************************************************
 * @brief add provisioning mode
 * @param[in] _opt -scanning option
 * @param[in] _func - AP list call back function
 * @return  1 success
 ****************************************************************************************
 */
int32_t app_find_home_ap(int32_t _opt, app_ap_list_from_scan_cb _func)
{
    char *scan_res_text, *p;
    int scan_res_num = -1;

    typedef struct _struct_scan_res {
        char ssid[SSID_BASE64_LEN_MAX];
        int security_type;
        int signal_strength;
        int freq;
    } struct_scan_res;

    DA16X_UNUSED_ARG(_opt);

    struct_scan_res *scan_res_array = (struct_scan_res*)app_common_malloc(sizeof(struct_scan_res) * SCAN_RESULT_MAX);
    if (scan_res_array == NULL) {
        APRINTF_E("[%s] malloc failed. \n", __func__);
        return 0;
    }
    memset(scan_res_array, 0, sizeof(struct_scan_res) * SCAN_RESULT_MAX);

    scan_res_text = (char*)app_common_malloc(CLI_SCAN_RSP_BUF_SIZE);
    if (scan_res_text == NULL) {
        APRINTF_E("[%s] malloc failed. \n", __func__);
        return 0;
    }
    memset(scan_res_text, '\0', CLI_SCAN_RSP_BUF_SIZE);

    /* cli scan */
    if (da16x_get_config_str(DA16X_CONF_STR_SCAN, scan_res_text)) {
        APRINTF_E("[%s] Wi-Fi Scan request failed. \n", __func__);
        app_common_free(scan_res_text);
        return 0;
    }

    APRINTF("\n> Wi-Fi Scan request success.\n");
    if (strtok(scan_res_text, "\n") == NULL) { /* Title Skip */
        app_common_free(scan_res_text);
        return 0;
    }

    for (scan_res_num = 0; scan_res_num < SCAN_RESULT_MAX;) {
        if (strtok(NULL, "\t") == NULL) { /* BSSID => Skip! */
            break;
        }

        p = strtok(NULL, "\t"); /* frequency */
        scan_res_array[scan_res_num].freq = atoi(p);

        p = strtok(NULL, "\t"); /* RSSI */
        scan_res_array[scan_res_num].signal_strength = atoi(p);

        p = strtok(NULL, "\t"); /* Security */

        if (strstr(p, "WPA") != 0) {
#if defined (__SUPPORT_WPA_ENTERPRISE__)
        	if (strstr(p, "EAP") != 0) {
				//  WPA2-EAP-SUITE-B-192-GCMP-256 : WPA3-Enterprise 192bits
				if (strstr(p, "WPA2-EAP-SUITE-B-192-GCMP-256") != 0) { // WPA3-Enterprise 192Bits Mode
					scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA3_EAP_192B; /* WPA3-ENT */
				} else {
					//<WPA2/WPA3-ENT> PMF 1	: WPA2-EAP-CCMP, EAP-SHA256-CCMP
					if (strstr(p, "WPA2-EAP-CCMP") != 0
						&& strstr(p, "EAP-SHA256-CCMP") != 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA2_AUTO_EAP; /* WPA2 WPA3-ENT */
					}
					//<WPA3-ENT> PMF 2	: EAP-SHA256-CCMP : WPA3-Enterprise 128bits Only
					else if (strstr(p, "EAP-SHA256-CCMP") != 0
						&& strstr(p, "EAP-CCMP") == 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA3_EAP; /* WPA3-ENT */
					}
					//<WPA2-ENT> PMF 1 or 0: WPA2-EAP-CCMP
					else if (strstr(p, "WPA2-EAP-CCMP") != 0
						&& strstr(p, "EAP-SHA256") == 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA2_EAP; /* WPA2-ENT */
					}
					//<WPA1/2-ENT> PMF 0 	: WPA-EAP-, WPA2-EAP-
					else if (strstr(p, "WPA2-EAP") != 0
						&& strstr(p, "WPA-EAP") != 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA_AUTO_EAP; /* WPA1 WPA2-ENT */
					}
					//<WPA1-ENT> PMF 0	: WPA-EAP- : WPA1 Only
					else if (strstr(p, "WPA-EAP") != 0
						&& strstr(p, "WPA2-EAP") == 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA_EAP; /* WPA-ENT */
					}
					else
					{
						// Unknown
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA_AUTO_EAP; /* WPA1 WPA2-ENT */
					}
				}
        	} else
#endif // __SUPPORT_WPA_ENTERPRISE__
        	if (strstr(p, "WPA2-") != 0) {
#if defined (__SUPPORT_WPA3_PERSONAL__)
				// Start :__SUPPORT_WPA3_SAE__
				if (strstr(p, "+SAE") != 0) {
					scan_res_array[scan_res_num].security_type = eWiFiSecurityRSN_SAE; /* WPA2 / SAE */
				} else if (strstr(p, "-SAE") != 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecuritySAE; /* SAE */
				}
				else
				// End : __SUPPORT_WPA3_SAE__
				if (strstr(p, "+OWE") != 0) {
					scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA2; /* WPA2 / OWE */
				} else if (strstr(p, "-OWE") != 0) {
					scan_res_array[scan_res_num].security_type = eWiFiSecurityOWE; /* OWE */
				} else
#endif //__SUPPORT_WPA3_PERSONAL__
				if (strstr(p, "WPA-") != 0) {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA_AUTO; /* WPA / WPA2 */
				} else {
						scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA2; /* WPA2 */
				}
			} else {
				scan_res_array[scan_res_num].security_type = eWiFiSecurityWPA; /* WPA */
			}
        } else if (strstr(p, "WEP") != 0) {
			scan_res_array[scan_res_num].security_type = eWiFiSecurityWEP; /* WEP */
		} else {
			scan_res_array[scan_res_num].security_type = eWiFiSecurityOpen; /* OPEN */
		}

        p = strtok(NULL, "\n"); /* SSID */

        /* Discard hidden SSID. */
        if (p[0] == HIDDEN_SSID_DETECTION_CHAR) {
            continue;
        }

        memcpy(scan_res_array[scan_res_num].ssid, p, strlen(p));
        APRINTF("(%d) %s / %d / %d / %d \n", scan_res_num, scan_res_array[scan_res_num].ssid,
            scan_res_array[scan_res_num].security_type, scan_res_array[scan_res_num].signal_strength,
            scan_res_array[scan_res_num].freq);

        scan_res_num++;
    }

    if (scan_res_text) {
        app_common_free(scan_res_text);
    }

    for (int idx = 0; idx < scan_res_num; idx++) {
        if (_func != NULL) {
            _func(idx, scan_res_array[idx].ssid, scan_res_array[idx].security_type, scan_res_array[idx].signal_strength);
        } else {
            APRINTF("  ERROR no callback function  for AP LIST ");
        }
    }

    app_common_free(scan_res_array);

    return 1;

}


void sta_connect_on_concurrent_mode(char *input_ssid, char *input_psk)
{
	char cmdline[128];
	char *value_str;

	value_str = pvPortMalloc(128);
	memset(value_str, 0, 128);

	// If needed ..., disconnect Wi-Fi connection of STA
	da16x_cli_reply("remove_network 0", NULL, value_str);
	// Start new STA profile
	da16x_cli_reply("add_network 0", NULL, value_str);
	
	// Need to make string with SSID
	// set_network 0 ssid 'NEW_SSID'

	APRINTF("Concurrent SSID : %s PW : %s ",input_ssid,input_psk);

	
	memset(cmdline, 0, 128);
	sprintf(cmdline, "set_network 0 ssid '%s'", input_ssid);
	da16x_cli_reply(cmdline, NULL, value_str);

#if defined (__SUPPORT_WPA_ENTERPRISE__) || defined (__SUPPORT_WPA3_PERSONAL__)
#if defined (__SUPPORT_WPA_ENTERPRISE__)
	if (Security_Type >= eWiFiSecurityWPA_EAP && Security_Type < eWiFiSecurityWPA3_EAP_192B) {
		// EAP ID
		memset(cmdline, 0, 128);
		sprintf(cmdline, "set_network 0 identity '%s'", eap_ID);
		da16x_cli_reply(cmdline, NULL, value_str);

		// EAP PW
		memset(cmdline, 0, 128);
		sprintf(cmdline, "set_network 0 password '%s'", eap_PW);
		da16x_cli_reply(cmdline, NULL, value_str);

		// pairwiae: TKIP, CCMP, AUTO
//		da16x_cli_reply("set_network 0 pairwise TKIP CCMP", NULL, value_str);	// AUTO
		delete_tmp_nvram_env(NVR_KEY_ENC_TYPE_0);	// AUTO: TKIP CCMP

		// autho mode: WPA_EAP, WPA2_EAP, WPA_AUTO_EAP
		da16x_cli_reply("set_network 0 key_mgmt WPA-EAP", NULL, value_str);	 // WPA_EAP
//		da16x_cli_reply("set_network 0 proto WPA RSN", NULL, value_str); // WPA_AUTO_EAP
        delete_tmp_nvram_env(NVR_KEY_PROTO_0); // WPA_AUTO_EAP

		// pairwiae: TKIP, CCMP, AUTO
		da16x_cli_reply("set_network 0 pairwise TKIP CCMP", NULL, value_str);	// AUTO

		// auth type: eap: PEAP TTLS FAST, PEAP, TTLS, FAST, TLS
		if (eap_type == TYPE_EAP_PEAP) {
			da16x_cli_reply("set_network 0 eap PEAP", NULL, value_str);	// PEAP
		} else if (eap_type == TYPE_EAP_TTLS) {
			da16x_cli_reply("set_network 0 eap TTLS", NULL, value_str);	// TTLS
		} else if (eap_type == TYPE_EAP_FAST) {
			da16x_cli_reply("set_network 0 eap FAST", NULL, value_str);	// FAST
		} else if (eap_type == TYPE_EAP_TLS) {
			da16x_cli_reply("set_network 0 eap TLS", NULL, value_str);	// TLS: Not Support
		} else {
			da16x_cli_reply("set_network 0 eap PEAP TTLS FAST", NULL, value_str);	// PEAP TTLS FAST,  TYPE_EAP_DEAULT
		}

		// Erase old fast_pac, fast_pac_len for EAP-FAST
		delete_tmp_nvram_env("fast_pac");
		delete_tmp_nvram_env("fast_pac_len");

		// auth protocol: phase2: MSCHAPV2 GTC,MSCHAPV2, GTC
		if (eap_protocol == PROTO_EAP_MSCHAPV2) {
			da16x_cli_reply("set_network 0 phase2 'auth=MSCHAPV2'", NULL, value_str);	// MSCHAPV2
		} else if (eap_protocol == PROTO_EAP_GTC) {
			da16x_cli_reply("set_network 0 phase2 'auth=GTC'", NULL, value_str);	// GTC
		} else {
			da16x_cli_reply("set_network 0 phase2 'auth=MSCHAPV2 GTC'", NULL, value_str);	// MSCHAPV2 GTC
		}
	}
#endif // (__SUPPORT_WPA_ENTERPRISE__)
#if defined (__SUPPORT_WPA3_PERSONAL__)
	if (Security_Type == eWiFiSecurityOWE) {	///< WPA3 OWE
		da16x_cli_reply("set_network 0 key_mgmt OWE", NULL, value_str);
		delete_tmp_nvram_env("N0_wep_key0");
		delete_tmp_nvram_env("N0_wep_key1");
		delete_tmp_nvram_env("N0_wep_key2");
		delete_tmp_nvram_env("N0_wep_key3");
		delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);
	} else if (Security_Type == eWiFiSecuritySAE || Security_Type == eWiFiSecurityRSN_SAE) {
		if (Security_Type == eWiFiSecuritySAE) { ///< WPA3 SAE
			da16x_cli_reply("set_network 0 key_mgmt SAE", NULL, value_str);
		} else {  ///> WPA2 & WPA3 SAE //eWiFiSecurityRSN_SAE
			da16x_cli_reply("set_network 0 key_mgmt WPA-PSK SAE", NULL, value_str);
		}
		da16x_cli_reply("set_network 0 proto RSN", NULL, value_str);
		delete_tmp_nvram_env("N0_wep_key0");
		delete_tmp_nvram_env("N0_wep_key1");
		delete_tmp_nvram_env("N0_wep_key2");
		delete_tmp_nvram_env("N0_wep_key3");
		delete_tmp_nvram_env(NVR_KEY_WEPINDEX_0);

		memset(cmdline, 0, 128);
		sprintf(cmdline, "set_network 0 sae_password '%s'", input_psk);
		da16x_cli_reply(cmdline, NULL, value_str);
	}
#endif	// (__SUPPORT_WPA3_PERSONAL__)
	if(input_psk!=NULL && Security_Type <= eWiFiSecurityWPA_AUTO) {
		// Need to make string with PSK
		// set_network 0 psk 'NEW_PSK'
		memset(cmdline, 0, 128);
		sprintf(cmdline, "set_network 0 psk '%s'", input_psk);

		da16x_cli_reply(cmdline, NULL, value_str);
		// Set for WPA/WPA2 PSK mode
		da16x_cli_reply("set_network 0 key_mgmt WPA-PSK", NULL, value_str);

		/// Options : Don't need these fields mendatory... //////////////
		// Set Security protocol - Normally RSN for WPA2
		da16x_cli_reply("set_network 0 proto RSN", NULL, value_str); // WPA2
		// Set Authenticate type
		// "TKIP" / "CCMP" / "TKIP CCMP"
		da16x_cli_reply("set_network 0 pairwise CCMP", NULL, value_str);
		/////////////////////////////////////////////////////////////////
	}
#else
	if(input_psk!=NULL) {
		// Need to make string with PSK
		// set_network 0 psk 'NEW_PSK'
		memset(cmdline, 0, 128);
		sprintf(cmdline, "set_network 0 psk '%s'", input_psk);
		
		da16x_cli_reply(cmdline, NULL, value_str);
		// Set for WPA/WPA2 PSK mode
		da16x_cli_reply("set_network 0 key_mgmt WPA-PSK", NULL, value_str);


		/// Options : Don't need these fields mendatory... //////////////
		// Set Security protocol - Normally RSN for WPA2
		da16x_cli_reply("set_network 0 proto RSN", NULL, value_str); // WPA2
		// Set Authenticate type
		// "TKIP" / "CCMP" / "TKIP CCMP"
		da16x_cli_reply("set_network 0 pairwise CCMP", NULL, value_str);
		/////////////////////////////////////////////////////////////////
	}
#endif
	// Hidden SSID
	if (use_Hidden_SSID == 1)
		da16x_cli_reply("set_network 0 scan_ssid 1", NULL, value_str);

	// Save profile in NVRAM
	da16x_cli_reply("save_config 0", NULL, value_str);
	// Connect STA with profile
	da16x_cli_reply("select_network 0", NULL, value_str);

	vPortFree(value_str);
	

	
}

/**
 ****************************************************************************************
 * @brief reboot to STA mode 
 * @param[in] _ssid - ap's ssid info  
 * @param[in] _pw - ap's pw
 * @param[in] _ispw - set security mode or not
 * @param[in] _mode - dpm mode set or not
 * @param[in] _hidden - hidden ssid use flag
 * @return void
 ****************************************************************************************
 */
void app_reset_ap_to_station(char *_ssid, char *_pw, int32_t _security, uint8_t _mode, int32_t _hidden)
{
    app_prov_config_t config;

    if (_mode == 0) {
        APRINTF_E("After soon reboot to [ No DPM ] mode !!\n");
    } else if (_mode == 1) {
        APRINTF_E("After soon reboot to [ DPM ] mode !!\n");
    } else {
        APRINTF_E("NOT support reboot mode !!\n");
        return;
    }

    APRINTF("\n[CMD APP] STA_mode_reset....");

    memset(&config, 0, sizeof(app_prov_config_t));
    strcpy(config.ssid, _ssid);

    if (_security == eWiFiSecurityWEP) {
        config.auth_type = eWiFiSecurityWEP;			// WEP
        strcpy(config.psk, _pw);
    } else if (_security == eWiFiSecurityWPA) {
        config.auth_type = eWiFiSecurityWPA;			// WPA
        strcpy(config.psk, _pw);
    } else if (_security == eWiFiSecurityWPA2) {
        config.auth_type = eWiFiSecurityWPA2;			// WPA2
        strcpy(config.psk, _pw);
    } else if (_security == eWiFiSecurityWPA_AUTO) {
        config.auth_type = eWiFiSecurityWPA_AUTO;			// WPA_AUTO
        strcpy(config.psk, _pw);
    }
#if defined (__SUPPORT_WPA_ENTERPRISE__)
    else if (_security >= eWiFiSecurityWPA_EAP && _security < eWiFiSecurityWPA3_EAP_192B) { // WPA-Ent
            config.auth_type = _security;
            strcpy(config.ent_id, eap_ID);
            strcpy(config.ent_pw, eap_PW);
            config.ent_protocol = eap_protocol;
            config.ent_type = eap_type;
    }
#endif	// (__SUPPORT_WPA_ENTERPRISE__)
#if defined (__SUPPORT_WPA3_PERSONAL__)
    else if (_security >= eWiFiSecurityOWE && _security <= eWiFiSecurityRSN_SAE) {
    	config.auth_type = _security;
    	if (_security == eWiFiSecurityOWE) {
    		strcpy(config.psk, "");
    	} else {
    	    strcpy(config.psk, _pw);
    	}
    }
#endif //(__SUPPORT_WPA3_PERSONAL__)
    else {
        config.auth_type = eWiFiSecurityOpen;			// OPEN
        strcpy(config.psk, "");
    }

    config.auto_restart_flag = 1;		///<restart
    config.dpm_mode = _mode;
    config.sntp_flag = 0;
    config.hidden = _hidden;			// use Hidden SSID
    config.prov_type = Provision_Type;	// provision check for Platform

#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__)
	 atcmd_provstat(ATCMD_PROVISION_REBOOT_ACK);
#endif	// __SUPPORT_ATCMD__ && __PROVISION_ATCMD__

    app_reset_to_station_mode(1, &config);
}

/**
 ****************************************************************************************
 * @brief add provisioning mode
 * @param[in] _root Json object
 * @param[in] _mode provisioining mode Concurrent = 0 , soft AP = 1
 * @return  void
 ****************************************************************************************
 */
static void app_add_provisioning_mode(cJSON *_root, int32_t _mode)
{
    cJSON_AddNumberToObject(_root, "mode", _mode);
}

/**
 ****************************************************************************************
 * @brief add thing name 
 * @param[in] _root Json object
 * @param[in] _thingName things name for this device 
 * @return  void
 ****************************************************************************************
 */
static void app_add_thing_name(cJSON *_root, char *_thingName)
{
    cJSON_AddStringToObject(_root, "thingName", _thingName);
}

#if defined ( __SUPPORT_AZURE_IOT__ )
/**
 ****************************************************************************************
 * @brief add shared key(azure)
 * @param[in] _root Json object
 * @param[in] _thingName things name for this device 
 * @return  void
 ****************************************************************************************
 */
static void app_add_shared_key(cJSON * _root, char* _conStringKey)
{
	cJSON_AddStringToObject(_root, "azureConString", _conStringKey);
}
#endif	// __SUPPORT_AZURE_IOT__

/**
 ****************************************************************************************
 * @brief delete Json object for AP list
 * @param[in] Json object for AP list
 * @return  void
 ****************************************************************************************
 */
static void app_delete_ap_list_json(cJSON *_ap_list)
{
    char *buf;

    buf = cJSON_Print(_ap_list);
    ap_list_buffer = buf;
    cJSON_Delete(_ap_list);
}

/**
 ****************************************************************************************
 * @brief make AP list for adding Json object 
 * @param[in] _root Json object
 * @return  Json object for AP list
 ****************************************************************************************
 */
static cJSON* app_create_ap_list_json(cJSON *_root)
{
    cJSON *newAPList;

    newAPList = cJSON_CreateArray();
    cJSON_AddItemToObject(_root, "APList", newAPList);

    return newAPList;
}

/**
 ****************************************************************************************
 * @brief call back function for index , SSID, security mode and signal  when app_find_home_ap is calling
 * @param[out] _index current index
 * @param[out] _SSID found SSID 
 * @param[out] _secMode security mode status 
 * @param[out] _signal signal level
 * @return  void
 ****************************************************************************************
 */
static void app_make_ap_list_cb(int _index, char *_SSID, int _secMode, int _signal)
{
    cJSON *newAP;

    if (ap_list == NULL) {
        APRINTF_E("NO AP LIST  buffer\n");
        return;
    }

    cJSON_AddItemToArray(ap_list, newAP = cJSON_CreateObject());
    cJSON_AddItemToObject(newAP, "index", cJSON_CreateNumber(_index));
    cJSON_AddItemToObject(newAP, "SSID", cJSON_CreateString(_SSID));
    cJSON_AddItemToObject(newAP, "securityType", cJSON_CreateNumber(_secMode));
    cJSON_AddItemToObject(newAP, "signal", cJSON_CreateNumber(_signal));
}

/**
 ****************************************************************************************
 * @brief find  SSIDs around device and make Json format for AP list
 * @param[in] _opt scanning option
 * @param[in] _mode concurrent(0) or soft ap(1)
 * @return  void
 ****************************************************************************************
 */
static void app_get_ap_list_from_device(int _opt, int _mode)
{
    cJSON *apListRoot;

    apListRoot = cJSON_CreateObject();
    app_add_thing_name(apListRoot, (char*)getAppThingName());

#if defined(__SUPPORT_AZURE_IOT__)
	app_add_shared_key(apListRoot,(char*)getAppIotHubConnString()); 
#endif

    app_add_provisioning_mode(apListRoot, _mode);

    ap_list = app_create_ap_list_json(apListRoot);
    app_find_home_ap(_opt, app_make_ap_list_cb);
    app_delete_ap_list_json(apListRoot);
}

/**
 ****************************************************************************************
 * @brief make command and result  to Json format 
 * @param[in] _CMD command string 
 * @param[in] _r status for command string
 * @return  text for transfer
 ****************************************************************************************
 */
static char* app_make_json_cmd_result(char *_CMD, int32_t _r)
{
    cJSON *root;
    char *buf;

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, _CMD, _r);
    buf = cJSON_Print(root);
    cJSON_Delete(root);

    return buf;
}

/**
 ****************************************************************************************
 * @brief checking internet or network by using ping server
 * @param[in] ipaddrstr ping server address
 * @return  1 : ok	0 : internet fail 
 ****************************************************************************************
 */
extern UINT	da16x_ping_client(int iface,
							char *domain_str,
							unsigned long ipaddr,
							unsigned long *ipv6dst,
							unsigned long *ipv6src,
							int len,
							unsigned long count,
							int wait,
							int interval,
							int nodisplay,
							char *ping_result);


int32_t app_check_internet_with_ping(char *ipaddrstr)
{
	char result_str[128] = {0, };
	int32_t len = 0;
	int32_t wait = 0;
	int32_t interval = 0;
	int32_t ping_interface = 0; /* default wlan0 */
	int32_t transmitted, reply_count;
	uint32_t ipaddr = 0;
	uint32_t count = 0;
	uint32_t average, time_min, ping_max;
    

	// result string 
	transmitted = reply_count = 0;
	average = time_min = ping_max = 0;
	ipaddr = (uint32_t)iptolong(ipaddrstr);

	count = 4;         // only 2
	len = 32;          //default
	wait = 8000;      // default 4sec
	interval = 1000;  // interval default 1sec

	/* If station interface */
	ping_interface = 0;                    //WLAN0_IFACE;

	/* ping client api execution with nodisplay as 1 and getting the string of result */                  
	da16x_ping_client(ping_interface, NULL, ipaddr, NULL, NULL, len, count, wait,
						interval, 1, result_str);                             

	/* parsing the result string */
	sscanf(result_str, "%u,%u,%u,%u,%u", (unsigned int *)&transmitted , (unsigned int *)&reply_count , (unsigned int *)&average,
			(unsigned int *)&time_min, (unsigned int *)&ping_max);
     
	if(reply_count > 0) /* Success */
	{
		APRINTF("Ping reply is ok\n");
		return 1;
	}
     
	APRINTF("Ping reply is fail\n");

	return 0; 
}
/**
 ****************************************************************************************
 * @brief parsing data for received data from phone
 * @param[in] _recData  received data
 * @return  AP_CMD_TYPE
 ****************************************************************************************
 */
static AP_CMD_TYPE app_provisioning_json_parser(const char *_recData)
{

    AP_CMD_TYPE retStatus = CMD_ERROR;
    cJSON *json_recv_data = NULL;
    cJSON *child_object = NULL;

    json_recv_data = cJSON_Parse(_recData);
    child_object = cJSON_GetObjectItem(json_recv_data, "msgType");

    if (child_object != NULL) {
        APRINTF("MSG Type [%d] \n", child_object->valueint);

        if (child_object->valueint == SET_AP_SSID_PW) {
#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__)
			atcmd_provstat(ATCMD_PROVISION_SELECTED_AP_SUCCESS);
#endif
            APRINTF_I("[SET SSID , PW]\n");

            cJSON *child_ssid = cJSON_GetObjectItem(json_recv_data, "ssid");
            cJSON *child_pw = cJSON_GetObjectItem(json_recv_data, "pw");
            cJSON *child_url = cJSON_GetObjectItem(json_recv_data, "url");
            cJSON *child_hidden = cJSON_GetObjectItem(json_recv_data, "isHidden");
            cJSON *child_security = cJSON_GetObjectItem(json_recv_data, "securityType");

#if defined (__SUPPORT_WPA_ENTERPRISE__)
            cJSON *child_eaptype = cJSON_GetObjectItem(json_recv_data, "authType");
            cJSON *child_eapproto = cJSON_GetObjectItem(json_recv_data, "authProtocol");
            cJSON *child_eapID = cJSON_GetObjectItem(json_recv_data, "authID");
            cJSON *child_eapPW = cJSON_GetObjectItem(json_recv_data, "authPW");

			eap_ID = (char*)app_common_malloc(MAX_AP_SSID_SIZE);
			eap_PW = (char*)app_common_malloc(MAX_AP_PASSWORD_SIZE);
			memset(eap_ID, 0, MAX_AP_SSID_SIZE);
			memset(eap_PW, 0, MAX_AP_PASSWORD_SIZE);

            if (child_eapID != NULL) {
				strcpy(eap_ID, child_eapID->valuestring);
			}
            if (child_eapPW != NULL) {
				strcpy(eap_PW, child_eapPW->valuestring);
			}

            if (child_eaptype != NULL) {
				eap_type = child_eaptype->valueint;
			}

            if (child_eapproto != NULL) {
				eap_protocol = child_eapproto->valueint;
			}
            APRINTF(">>> eap ID: %s, eap PW: %s, eap type: %d, eap_protocol: %d \n",eap_ID, eap_PW, eap_type, eap_protocol);
#endif
            memset(new_SSID, 0, MAX_AP_SSID_SIZE);
            memset(new_pw, 0, MAX_AP_PASSWORD_SIZE);
            memset(server_URL, 0, MAX_CUSTOMER_SERVER_SIZE);
            new_pw_size = 0;
            new_SSID_size = 0;
            server_URL_size = 0;

            if (child_ssid != NULL) {
                APRINTF("[SSID] -> %s  size = %d\n", child_ssid->valuestring, strlen(child_ssid->valuestring));

                new_SSID_size = (int32_t)strlen(child_ssid->valuestring);
                strcpy(new_SSID, child_ssid->valuestring);
            }

            if (child_pw != NULL) {
                APRINTF("[PW] -> %s size = %d\n", child_pw->valuestring, strlen(child_pw->valuestring));
                new_pw_size = (int32_t)strlen(child_pw->valuestring);
                strcpy(new_pw, child_pw->valuestring);
            }

            if (child_url != NULL) {
                APRINTF("[SERVER URL] -> %s size = %d\n", child_url->valuestring, strlen(child_url->valuestring));
                server_URL_size = (int32_t)strlen(child_url->valuestring);
                strcpy(server_URL, child_url->valuestring);
            }

            /* Hidden SSID use set */
            if (child_hidden->valueint == 1) {
                APRINTF("[isHidden] -> %d\n", child_hidden->valueint);
                use_Hidden_SSID = 1;
            }

            /* Security Mode set */
            if (child_security->valueint >= 0) {
                APRINTF("[securityType] -> %d\n", child_security->valueint);
                Security_Type = child_security->valueint;
            }

            retStatus = SET_AP_SSID_PW;
        } else if (child_object->valueint == CONNECTED) {
            APRINTF_I("[CONNECTED]\n");
            retStatus = CONNECTED;
        } else if (child_object->valueint == REQ_HOMEAP_RESULT) {
            APRINTF_I("[REQ_HOMEAP_RESULT]\n");
            retStatus = REQ_HOMEAP_RESULT;
        } else if (child_object->valueint == REQ_RESCAN) {
            APRINTF_I("[REQ_RESCAN]\n");
            retStatus = REQ_RESCAN;
        } else if (child_object->valueint == REQ_REBOOT) {
#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__)
			atcmd_provstat(ATCMD_PROVISION_REBOOT_ACK);
#endif
            cJSON *child_finish = cJSON_GetObjectItem(json_recv_data, "finishCMD");
            if (child_finish != NULL) {
                APRINTF("Finish CMD  %d\n", child_finish->valueint);
            }

            APRINTF_I("[REQ_REBOOT]\n");
            retStatus = REQ_REBOOT;
        } else if (child_object->valueint == REQ_SET_DPM) {
            cJSON *sleepMode = cJSON_GetObjectItem(json_recv_data, "sleepMode");
            cJSON *useDPM = cJSON_GetObjectItem(json_recv_data, "useDPM");
            cJSON *dpmKeepAlive = cJSON_GetObjectItem(json_recv_data, "dpmKeepAlive");
            cJSON *userWakeup = cJSON_GetObjectItem(json_recv_data, "userWakeup");
            cJSON *timWakeup = cJSON_GetObjectItem(json_recv_data, "timWakeup");
            cJSON *rtcTimer = cJSON_GetObjectItem(json_recv_data, "rtcTimer");

            APRINTF_I("[SET Sleep mode , DTIM]\n");
            APRINTF("[SET Sleep mode , data]%s \n", _recData);

            if (sleepMode != NULL && useDPM != NULL && dpmKeepAlive != NULL && userWakeup != NULL && rtcTimer != NULL
                && timWakeup != NULL) {

                setSleepModeDtim(sleepMode->valueint, useDPM->valueint, rtcTimer->valueint, dpmKeepAlive->valueint,
                    userWakeup->valueint, timWakeup->valueint);

            } else {
                APRINTF_E("Set DPM value  parsing error \n");
            }

            retStatus = REQ_SET_DPM;
        } else if (child_object->valueint == REQ_SOCKET_TYPE) {
            cJSON *socketType = cJSON_GetObjectItem(json_recv_data, "SOCKET_TYPE");

            socket_app = (UINT8)socketType->valueint;
            APRINTF_I("[REQ_SOCKET_TYPE] %d\n", socket_app);
            retStatus = REQ_SOCKET_TYPE;
        }
    } else {
        APRINTF_E("LocalJSONParse  parsing error \n");
        retStatus = CMD_ERROR;
    }

    cJSON_Delete(json_recv_data);

    return retStatus;
}

/**
 ****************************************************************************************
 * @brief send message from TCP server thread to TCP client thread
 * @param[in] _proQData sending data
 * @return  void
 ****************************************************************************************
 */
static void app_send_provision_thread_msg(provisionData *_proQData)
{
    int status = 0;

    status = xQueueSend(provisionMsgQ, _proQData, portMAX_DELAY);
    if (status != pdPASS) {
        APRINTF("[%s] xQueueSend error !!! (%d)\n", __func__, status);
    }
}

/**
 ****************************************************************************************
 * @brief receive message from TCP server 
 * @return  receiving status
 ****************************************************************************************
 */
static int32_t app_recv_provision_thread_msg(void)
{
    int status = 0;
    provisionData proQData;

    status = xQueueReceive(provisionMsgQ, &proQData, portMAX_DELAY);
    if (status != pdPASS) {
        APRINTF("[%s] xQueueReceive error !!! (%d)\n", __func__, status);
        return -1;
    }

    return proQData.status;
}

/**
 ****************************************************************************************
 * @brief send TCP data to phone
 * @param[in] _data sending data
 * @param[in] _data sending data size
 * @return  void
 ****************************************************************************************
 */
static void app_provision_send_tcp_data(char *_data, int32_t _dataSize)
{
    int status = 0;

    app_mutex_lock(&provision_mutex);

    APRINTF("[Prov TCP Send] :::  size = %d \n", _dataSize);

    if (provision_TCP_csocket > 0) {
        status = send(provision_TCP_csocket, _data, (size_t )_dataSize, 0);
        if (status <= 0) {
            APRINTF("[%s:%d]failed to send packet(0x%02x)\n", __func__, __LINE__, status);
            close(provision_TCP_csocket);
        }
    } else {
        APRINTF("[%s:%d]client socket id invalid (0x%02x)\n", __func__, __LINE__, provision_TCP_csocket);
    }

    app_mutex_unlock(&provision_mutex);
}

/**
 ****************************************************************************************
 * @brief TLS provisioning data send
 * @param[in] config - tls server information
 * @param[in] data - send data
 * @param[in] dataSize - send data length
 * @return  result status
 ****************************************************************************************
 */
static int app_provision_send_tls_data(prov_tls_svr_conf_t *config, char *data, int32_t dataSize)
{
    int status;

    app_mutex_lock(&provision_mutex);

    while ((status = mbedtls_ssl_write(config->ssl_ctx, (const unsigned char*)data, (size_t)dataSize)) <= 0) {
        if ((status != MBEDTLS_ERR_SSL_WANT_READ) && (status != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            APRINTF_E("[%s:%d] failed to write ssl packet(0x%x)\n",
                __func__, __LINE__, -status);
        }
    }

    app_mutex_unlock(&provision_mutex);

    return status;

}

/**
 ****************************************************************************************
 * @brief send to Json format 
 * @param[in] _data sending data
 * @param[in] _type status or result
 * @return  1 on success
 ****************************************************************************************
 */
static int32_t app_send_json_data(char *_data, int32_t _type)
{
    char *dataBuf = NULL;

    dataBuf = app_make_json_cmd_result(_data, _type);
    if (dataBuf != NULL) {
        int32_t sizeSend = (int32_t)strlen(dataBuf);
        app_provision_send_tcp_data(dataBuf, sizeSend);
        app_common_free(dataBuf);
    } else {
        APRINTF_E("app_send_json_data error \n");
        return 0;
    }

    return 1;
}

/**
 ****************************************************************************************
 * @brief TLS data send to json format
 * @param[in] config - tls server information
 * @param[in] _data - send data
 * @param[in] _type - status or result
 * @return  1 success
 ****************************************************************************************
 */
static int32_t app_send_tls_json_data(prov_tls_svr_conf_t *config, char *_data, int32_t _type)
{
    char *dataBuf = NULL;

    dataBuf = app_make_json_cmd_result(_data, _type);
    if (dataBuf != NULL) {
        int32_t sizeSend = (int32_t)strlen(dataBuf);

        app_provision_send_tls_data(config, dataBuf, sizeSend);

        APRINTF("app_send_tls_json_data \n");
        app_common_free(dataBuf);
    } else {
        APRINTF_E("app_send_tls_json_data error \n");
        return 0;
    }

    return 1;
}

/**
 ****************************************************************************************
 * @brief provisioning socket type
 * @return  socket_type	0 :  TCP, 1 : TLS,  default is 1
 ****************************************************************************************
 */
static UINT8 get_provision_mode(void)
{
    return socket_dev;
}

/**
 ****************************************************************************************
 * @brief TLS provisioning api
 * @param[in] config - tls server information
 * @return  void
 ****************************************************************************************
 */
static void app_prov_run_tls_svr(prov_tls_svr_conf_t *config)
{
    int sys_wdog_id = -1;
    int status = ERR_OK;
    int recv_bytes = 0;
    unsigned char *buf = NULL;
    size_t buflen = (8 * 1024);

    mbedtls_net_context client_ctx;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    APRINTF("\r\n>>> Start Provisioning Server (TLS) ...\r\n", __func__);

    mbedtls_net_init(&client_ctx);

    // Allocate buffer
    buf = (unsigned char*)app_prov_calloc(1, buflen);
    if (!buf) {
        APRINTF("[%s] Failed to allocate buffer to receive\r\n", __func__);
        return;
    }

    // Init TLS server sample's configuration
    status = app_prov_tls_svr_init_config(config);
    if (status) {
        APRINTF("[%s] Failed to init sample tls server config(0x%x)\n", __func__, status);
        app_prov_free(buf);
        return;
    }

    // Init TCP socket
    status = app_prov_tls_svr_init_socket(config);
    if (status) {
        APRINTF("[%s] Failed to init tcp socket(0x%x)\n", __func__, status);
        app_prov_free(buf);
        return;
    }

    // Init TLS session
    status = app_prov_tls_svr_init_ssl(config);
    if (status) {
        APRINTF("[%s] Failed to init ssl(0x%x)\r\n", __func__, status);

        app_prov_tls_svr_deinit_socket(config);
        app_prov_free(buf);

        return;
    }

    // Setup TLS session
    status = app_prov_tls_svr_setup_ssl(config);
    if (status) {
        APRINTF("[%s] Failed to setup ssl(0x%x)\r\n", __func__, -status);

        app_prov_tls_svr_deinit_ssl(config);
        app_prov_tls_svr_deinit_socket(config);
        app_prov_free(buf);

        return;
    }

connect: mbedtls_net_free(&client_ctx);
    app_prov_tls_svr_shutdown_ssl(config);

    APRINTF_S("Wait Accept (TLS)...\n");

    da16x_sys_watchdog_notify(sys_wdog_id);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    // Wait until a client connects
    status = mbedtls_net_accept(&config->sock_ctx, &client_ctx, NULL, 0, NULL);
    if (status) {
        APRINTF("[%s] Failed to accept client connects(0x%x)\r\n", __func__, -status);
        goto connect;
    }

    // Set callbacks to write & read data.
    mbedtls_ssl_set_bio(config->ssl_ctx, &client_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

    // Handshake TLS session
    status = app_prov_tls_svr_do_handshake(config);
    if (status) {
        APRINTF("[%s] Failed to progress handshake(0x%x)\r\n", __func__, -status);
        goto connect;
    }

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    // Data Transmit:
    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        memset(buf, 0x00, buflen);

        status = mbedtls_ssl_read(config->ssl_ctx, buf, buflen);

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        if (status <= 0) {
            // For log
            switch (status) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
                continue;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                APRINTF("[%s]Connection was closed gracefully\r\n", __func__);
                break;

            case MBEDTLS_ERR_NET_CONN_RESET:
                APRINTF("[%s]Connection was reset by peer\r\n", __func__);
                break;

            default:
                APRINTF("[%s]Failed to read data(0x%x)\r\n", __func__, -status);
                break;
            }

            goto connect;
        }

        recv_bytes = status;

        if (recv_bytes > 0) {
            provisionData sampleData;
            sampleData.status = app_provisioning_json_parser((char const*)buf);

            if (check_connection_ap) {
                char *dataBuf = NULL;
                APRINTF_S("Now checking to connect Home AP...\n");

                dataBuf = app_make_json_cmd_result("RESULT_HOMEAP", 0);

                if (dataBuf != NULL) {
                    int32_t sizeSend = (int32_t)strlen(dataBuf);

                    app_provision_send_tls_data(config, dataBuf, sizeSend);
                    app_common_free(dataBuf);
                }
            } else {
                app_send_provision_thread_msg(&sampleData);
            }
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }

    // Socket terminate:
    mbedtls_net_free(&client_ctx);

    // Shutdown TLS session
    app_prov_tls_svr_shutdown_ssl(config);

    // Deinit TLS session
    app_prov_tls_svr_deinit_ssl(config);

    // Deinit TCP socket
    app_prov_tls_svr_deinit_socket(config);

    // Release memory
    if (buf) {
        app_prov_free(buf);
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    APRINTF("[%s] Terminated Provision Server (TLS) ...\r\n", __func__);

    return;
}

/**
 ****************************************************************************************
 * @brief thread for sending TCP/TLS data to phone
 * @param[in] _pMode - TLSInfo structure data
 * @return  void
 ****************************************************************************************
 */
static void app_provision_switch_client_thread(void *_pMode)
{
    int sys_wdog_id = -1;
    int32_t status = 0;
    int32_t homeAPConnectionResult = 0;

    TLSInfo *prov_mode = (TLSInfo*)_pMode;
    int32_t _provMode = prov_mode->mode;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    APRINTF("[%s] Create...(status=%d) [%d] \n", __func__, status, _provMode);

    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        vTaskDelay(10);
        status = app_recv_provision_thread_msg();

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        if (status != -1) {
            APRINTF("Recv MSG .. [%d] ", status);

            switch (status) {
            case CONNECTED: {
                if (ap_list_buffer) {
                    int32_t sizeSend = (int32_t)strlen(ap_list_buffer);

                    if (!get_provision_mode()) {
                        app_provision_send_tcp_data(ap_list_buffer, sizeSend);
                    } else {
                        app_provision_send_tls_data(prov_mode->config, ap_list_buffer, sizeSend);
                    }
                } else {
                    APRINTF("ERROR : SCAN LIST is null .... \n");
                }
            }
                break;

            case SET_AP_SSID_PW: {
                int rebootDPMMode = STATION_MODE_NO_DPM;   // default set is STATION_MODE_DPM

				tryConnectOnSTA =1;

                if (Provision_Type > SYSMODE_TEST) {		// for Platform
                    rebootDPMMode = STATION_MODE_DPM;
                }

                if (!get_provision_mode()) {
                    app_send_json_data("RESULT_REBOOT", 0);
                } else {
                    app_send_tls_json_data(prov_mode->config, "RESULT_REBOOT", 0);
                }

                vTaskDelay(50);

                if (app_get_DPM_set_value(TYPE_SLEEP_MODE) != 0) {
                    rebootDPMMode = app_get_DPM_set_value(TYPE_USE_DPM);
                    APRINTF("Reset DPM mode to %d \n", rebootDPMMode);
                }

			
					if(get_run_mode()==SYSMODE_STA_N_AP)
					{

							PRINTF("[Concurrent mode] \n");


						if (new_pw_size != 0) {
							if (new_SSID_size != 0) {
								sta_connect_on_concurrent_mode(new_SSID,new_pw);
							} else {
								APRINTF_E("ERROR : Something wrong.... SSID is null \n");		
							}
						} else {
							if (new_SSID_size != 0) {
								sta_connect_on_concurrent_mode(new_SSID,NULL);
							} else {
								APRINTF("ERROR : Something wrong.... SSID is null \n"); 
							}	
						}

					}else if(get_run_mode()==SYSMODE_AP_ONLY)
					{
					
							PRINTF("[Soft AP mode] \n");

							if (new_pw_size != 0) {
								if (new_SSID_size != 0) {
									app_reset_ap_to_station(new_SSID, new_pw, Security_Type,
															(uint8_t)rebootDPMMode, use_Hidden_SSID);
								} else {
									APRINTF_E("ERROR : Something wrong.... SSID is null \n");       
								}
							} else {
								if (new_SSID_size != 0) {
									app_reset_ap_to_station(new_SSID, new_pw, Security_Type,
															(uint8_t)rebootDPMMode, use_Hidden_SSID);
								} else {
									APRINTF("ERROR : Something wrong.... SSID is null \n"); 
								}   
							}
					}

				} break;

				case REQ_HOMEAP_RESULT:
				{

					if(tryConnectOnSTA==1)
					{

						PRINTF("Wating to complete AP connection ... \n");
						homeAPConnectionResult=10;
						if (!get_provision_mode()) {
							app_send_json_data("RESULT_HOMEAP",homeAPConnectionResult);
						} else {   
							app_send_tls_json_data(prov_mode->config, "RESULT_HOMEAP",homeAPConnectionResult);
						}

						break;
					}
					
					if(conSTAStatus!=1)
					{
						PRINTF("NO STA connection\n");
						homeAPConnectionResult = 2;
					}else
					{
						int32_t ping_rc =0;
					    char* pingAddr = "8.8.8.8";
						
						ping_rc = app_check_internet_with_ping(pingAddr);

						if(ping_rc == 0)
						{
							homeAPConnectionResult = 3;
						}else
						{
							homeAPConnectionResult = 1;
						}
					}
										
					if (!get_provision_mode()) {
						app_send_json_data("RESULT_HOMEAP",homeAPConnectionResult);
					} else {   
						app_send_tls_json_data(prov_mode->config, "RESULT_HOMEAP",homeAPConnectionResult);
					}
				} break;

            case REQ_RESCAN: {
                // need rescan
                int32_t sizeSend = 0;
                APRINTF("REQ reScan \n");

                if (ap_list_buffer != NULL) {
                    app_common_free(ap_list_buffer);
                    ap_list_buffer = NULL;
                }

                app_get_ap_list_from_device(1, _provMode);

                sizeSend = (int32_t)strlen(ap_list_buffer);

                if (!get_provision_mode()) {
                    app_provision_send_tcp_data(ap_list_buffer, sizeSend);
                } else {
                    app_provision_send_tls_data(prov_mode->config, ap_list_buffer, sizeSend);
                }
            }
                break;

            case REQ_REBOOT: {
                int rebootDPMMode = STATION_MODE_NO_DPM;   // default set is STATION_MODE_DPM

                if (Provision_Type > SYSMODE_TEST) {	// for Platform
                    rebootDPMMode = STATION_MODE_DPM;
                }

                if (!get_provision_mode()) {
                    app_send_json_data("RESULT_REBOOT", 0);
                } else {
                    app_send_tls_json_data(prov_mode->config, "RESULT_REBOOT", 0);
                }

                vTaskDelay(50);

                if (app_get_DPM_set_value(TYPE_SLEEP_MODE) != 0) {
                    rebootDPMMode = app_get_DPM_set_value(TYPE_USE_DPM);
                    APRINTF("Reset DPM mode to %d \n", rebootDPMMode);
                }

                if (new_pw_size != 0) {
                    if (new_SSID_size != 0) {
                        app_reset_ap_to_station(new_SSID, new_pw, Security_Type, (uint8_t)rebootDPMMode, use_Hidden_SSID);

                    } else {
                        APRINTF("ERROR : something wrong.... SSID null 1 \n");
                    }
                } else {
                    if (new_SSID_size != 0) {
                        app_reset_ap_to_station(new_SSID, new_pw, Security_Type, (uint8_t)rebootDPMMode, use_Hidden_SSID);
                    } else {
                        APRINTF("ERROR : something wrong.... SSID null 2 \n");
                    }
                }
            }
                break;

            case REQ_SOCKET_TYPE: {
                if (socket_app == get_provision_mode()) {
                    if (get_provision_mode()) {	// tls
                        app_send_tls_json_data(prov_mode->config, "SOCKET_TYPE", socket_app);
                    } else {	// tcp
                        app_send_json_data("SOCKET_TYPE", socket_app);
                    }
                } else if (socket_app != get_provision_mode()) {
                    if (socket_app) {
                        // swtiching to TLS socket from TCP mode
                        APRINTF("move to TLS\n");
                        app_send_json_data("SOCKET_TYPE", socket_app);
                    } else {
                        // switching to TCP socket from TLS mode
                        APRINTF("move to TCP\n");
                        app_send_tls_json_data(prov_mode->config, "SOCKET_TYPE", socket_app);
                    }

                    socket_dev = socket_app;
                } else {
                    APRINTF_E("\n[%s]SOCKET_TYPE UNDEFINE\n",__func__);
                }
            }
                break;

            case CMD_ERROR: {
                // RESULT_CMD_ERROR
            }
                break;

            default: {
            }
                break;
            }
        } else {
            APRINTF_E("\recv MSG error .... \n\n");
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
    }


    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);
}

/**
 ****************************************************************************************
 * @brief thread for receiving TLS data from phone
 * @param[in] _pMode - TLSInfo structure data
 * @return  void
 ****************************************************************************************
 */
static void app_provision_TLS_server_thread(void *_pMode)
{
    TLSInfo *prov_mode = (TLSInfo*)_pMode;

    APRINTF("[%s] Create TLS... \n", __func__);

    app_prov_run_tls_svr(prov_mode->config);
}

/**
 ****************************************************************************************
 * @brief thread for receiving TCP data from phone
 * @param[in] _provMode 0 - concurrent mode, 1 - ap mode
 * @return  void
 ****************************************************************************************
 */
static void app_provision_TCP_server_thread(void *_pMode)
{
    int sys_wdog_id = -1;
    int status;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addrlen;
    char data_buf[PROVISION_TCP_RX_BUF_SZ] = {0x00, };
    TLSInfo *prov_mode = (TLSInfo*)_pMode;

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    new_SSID = (char*)app_common_malloc(MAX_AP_SSID_SIZE);
    new_pw = (char*)app_common_malloc(MAX_AP_PASSWORD_SIZE);
    server_URL = (char*)app_common_malloc(MAX_CUSTOMER_SERVER_SIZE);

    APRINTF("[%s] Create ... \n", __func__);

    app_mutex_init(&provision_mutex, "provision_mutex");

    // create MSG Q  //////////////////////////////////////////////////////
    provisionMsgQ = xQueueCreate(PROVISION_MSG_CMD, sizeof(provisionData));

    if (provisionMsgQ == NULL) {
        APRINTF("[%s] Failed to create queue \"provisionMsgQ\" \n", __func__);
        da16x_sys_watchdog_register(sys_wdog_id);
        vTaskDelete(NULL);
        return;
    }

    // scan list update
    app_get_ap_list_from_device(1, prov_mode->mode);

    /////////////////////////////////////////////////////////////////////////////////

    provision_TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (provision_TCP_socket < 0) {
        APRINTF("[%s] Failed to assign listen socket\r\n", __func__);
        goto exit;
    }

    /* Listen */
    APRINTF("\n[%s] socket().. status=%d  \n ", __func__, provision_TCP_socket);

    memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TCP_PROVISION_PORT_NUM);

    status = bind(provision_TCP_socket, (struct sockaddr* )&server_addr, sizeof(server_addr));
    if (status < 0) {
        APRINTF("[%s] Failed to bind socket(%d)\r\n", __func__, status);
        goto exit;
    }

    status = listen(provision_TCP_socket, 1); //diafreertoswork: org. backlog # 4
    if (status < 0) {
        PRINTF("[%s] Failed to listen socket(%d)\r\n", __func__, status);
        goto exit;
    }

    while (pdTRUE) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        APRINTF_S("Wait Accept...\n");

        /* Accept */
        memset(&client_addr, 0x00, sizeof(client_addr));
        client_addrlen = sizeof(client_addr);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        provision_TCP_csocket = accept(provision_TCP_socket, (struct sockaddr* )&client_addr, &client_addrlen);
        if (provision_TCP_csocket < 0) {
            APRINTF("[%s] Failed to accept client connects\r\n", __func__);
            continue;
        }

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        APRINTF("[%s] Connected client(%d.%d.%d.%d:%d)\r\n", __func__, (ntohl(client_addr.sin_addr.s_addr) >> 24) & 0x0ff,
            (ntohl(client_addr.sin_addr.s_addr) >> 16) & 0x0ff, (ntohl(client_addr.sin_addr.s_addr) >> 8) & 0x0ff,
            (ntohl(client_addr.sin_addr.s_addr) ) & 0x0ff, ntohs(client_addr.sin_port));

        APRINTF_S("Accept OK\n");

        while (1) {
            memset(data_buf, 0, sizeof(data_buf));

            da16x_sys_watchdog_suspend(sys_wdog_id);

            status = recv(provision_TCP_csocket, data_buf, sizeof(data_buf), 0);

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

            da16x_sys_watchdog_suspend(sys_wdog_id);

            if (status > 0) {
                // parsing json
                uint32_t rx_bytes = (uint32_t)status;

                APRINTF("[%s] read ..  status = %d length = %d \n", __func__, status, rx_bytes);

                if (rx_bytes > 0) {
                    provisionData sampleData;
                    sampleData.status = app_provisioning_json_parser(data_buf);

                    if (check_connection_ap) {	//concurrent mode::not used
                        char *dataBuf = NULL;
                        APRINTF_S("Now checking to connect Home AP...\n");

                        dataBuf = app_make_json_cmd_result("RESULT_HOMEAP", 0);

                        if (dataBuf != NULL) {
                            int32_t sizeSend = (int32_t)strlen(dataBuf);

                            app_provision_send_tcp_data(dataBuf, sizeSend);
                            app_common_free(dataBuf);
                        }
                    } else {
                        app_send_provision_thread_msg(&sampleData);
                    }

                }
            } else if (status == 0) {
                OAL_MSLEEP(200);
                APRINTF_I("[%s] recv timeout...\n", __func__);
            } else {
                OAL_MSLEEP(200);
                APRINTF_S("dis connection...\n");
                close(provision_TCP_csocket);
                break;;
            }

            da16x_sys_watchdog_notify_and_resume(sys_wdog_id);
        }
    }

exit:

    close(provision_TCP_socket);
    close(provision_TCP_csocket);

    if (new_SSID) {
        app_common_free(new_SSID);
    }

    if (new_pw) {
        app_common_free(new_pw);
    }

    if (server_URL) {
        app_common_free(server_URL);
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);

    vTaskDelete(NULL);

    return;
}

/**
 ****************************************************************************************
 * @brief Entry function for APP provisioning
 * @param[in] _mode 
 *    Generic SDK:1,  
 *    Platform AWS: Generic:10, ATCMD:11, ...
 *    Platform AZURE: Generic:20, ATCMD:21, ...
 * @return  void
 ****************************************************************************************
 */
void app_start_provisioning(int32_t _mode)
{
    int status;
    TaskHandle_t TCPServerThreadPtr = NULL;
    TaskHandle_t TLSServerThreadPtr = NULL;
    TaskHandle_t PROVClientThreadPtr = NULL;

    TLSInfo *tlsinfo = NULL;

#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__)
    atcmd_provstat(ATCMD_PROVISION_START);
#endif	// __SUPPORT_ATCMD__ && __PROVISION_ATCMD__


	if(_mode == SYSMODE_AP_ONLY)
	{
		APRINTF_S("\n=======================================================\n");
	
		APRINTF_S("[Start Provisioning with TCP/TLS] .. Soft AP Mode \n");
	
		APRINTF_S("=======================================================\n");
	}else if (_mode == SYSMODE_STA_N_AP)
	{
		APRINTF_S("\n=======================================================\n");
	
		APRINTF_S("[Start Provisioning with TCP/TLS] .. Concurrent Mode(STA/AP) \n");
	
		APRINTF_S("=======================================================\n");
	}

    tlsinfo = (TLSInfo*)app_common_malloc(sizeof(TLSInfo));
    tlsinfo->mode = _mode;
    tlsinfo->config = &app_prov_tls_svr_config;

    Provision_Type = (int8_t)_mode;

    // TCP provisioning
    /* Create provision TCP thread on Soft-AP mode */
    status = xTaskCreate(app_provision_TCP_server_thread,
    APP_SOFTAP_PROV_NAME,
    APP_SOFTAP_PROV_STACK_SZ, (void*)tlsinfo,
    tskIDLE_PRIORITY + 2, &TCPServerThreadPtr);
    if (status != pdPASS) {
        APRINTF("[%s] Failed to create TCP svr thread\r\n", __func__);
    }

    // TLS provisioning
    /* Create provision TLS thread on Soft-AP mode */
    status = xTaskCreate(app_provision_TLS_server_thread,
    APP_SOFTAP_APROV_NAME,
    APP_SOFTAP_PROV_STACK_SZ, (void*)tlsinfo,
    tskIDLE_PRIORITY + 2, &TLSServerThreadPtr);
    if (status != pdPASS) {
        APRINTF("[%s] Failed to create TLS svr thread\r\n", __func__);
    }

    // Create client task
    status = xTaskCreate(app_provision_switch_client_thread, "ProvClient",
    APP_SOFTAP_PROV_STACK_SZ, (void*)tlsinfo,
    tskIDLE_PRIORITY + 3, &PROVClientThreadPtr);
    if (status != pdPASS) {
        APRINTF("[%s] Failed to create prov client thread\r\n", __func__);
    }
}

/**
 ****************************************************************************************
 * @brief SoftAP Provisioning application thread calling function 
 * @param[in] arg - transfer information
 * @return  void
 ****************************************************************************************
 */
void softap_provisioning(void *arg)
{
    int sys_wdog_id = -1;
    int sysmode;

    DA16X_UNUSED_ARG(arg);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);

    da16x_sys_watchdog_notify(sys_wdog_id);

#if defined (__SUPPORT_ATCMD__) && defined (__PROVISION_ATCMD__)
	atcmd_provstat(ATCMD_PROVISION_IDLE);
#endif	// __SUPPORT_ATCMD__ && __PROVISION_ATCMD__

SOFTAP_MODE : 

    da16x_sys_watchdog_suspend(sys_wdog_id);

	sysmode = getSysMode();
	if (SYSMODE_AP_ONLY == sysmode || SYSMODE_STA_N_AP == sysmode  ) {		
		app_start_provisioning(sysmode);		// support only AP_MODE
	} else {
		vTaskDelay(500);

		if (chk_network_ready(WLAN0_IFACE) != 1) {
			goto SOFTAP_MODE;
		}
	}

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    da16x_sys_watchdog_unregister(sys_wdog_id);

	vTaskDelete(NULL);
}

void AP_connected_notification_on_concurrent(int _index, char* _buf)
{
	DA16X_UNUSED_ARG(_buf);

	if(_index >=4)
	{
		return;
	}

	switch(_index)
		{
		case 0: conSTAStatus=1; tryConnectOnSTA=0 ; break;	
		case 1: conSTAStatus=0; tryConnectOnSTA=0 ;break;
		case 2: conAPStatus=1;  break;
		case 3: conAPStatus =0; break;
		default:
			PRINTF("AP_connected_notification_on_concurrent index error [%d]",_index);
			break;
		}
 
}
#endif 	//(__SUPPORT_WIFI_PROVISIONING__)

/* EOF */
