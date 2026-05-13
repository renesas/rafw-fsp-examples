/**
 ****************************************************************************************
 *
 * @file app_dpm_wrapper.c
 *
 * @brief Define DPM related APIs used to operate device on platform.
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

#ifdef __cplusplus
extern "C" {
#endif

#include "user_dpm.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#endif
#include "util_api.h"
#include "azure_iot_log.h"
#include "app_dpm_interface.h"
#include "common_utils.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"

#include "app_azure_user_conf.h"
#define malloc	pvPortMalloc
#define free	vPortFree
#if defined(__SUPPORT_ATCMD__) /* AZURE + ATCMD	*/
#include "../../src/app_common/include/app_atcommand/app_atcommand.h"
#endif

char azure_dpm_recv_timeout_alt_flag = 0; // flag for setting recv timeout on DPM mode
char flagReconnected = 0; // reconnection flag on DPM mode
char m_is_subscribed = 0; // flag for subscribing dummy on stack
UINT16 secondsKeepalive = 0; // wakup timer interval for KA (seconds)
UINT32 m_local_port = UNDEF_PORT; // local port

bool skip_save_dns_ip_flag = FALSE; // flag for dns query skip
UINT8 flagSleepDPM = 0;
static char azure_rtm_name[20] = AZURE_RTM_NAME;
char flagRegisteredThing = 0;
static char pubSendFlag = 0;
static UINT8 waitNextJobFlag = 0;
UINT8 flagRcvTimeout = 0; // flag for setting recv timeout on DPM mode
UINT8 flagUnknownUC = 0; // flag for Unknown UC on DPM wakeup
static APPSleepMode appSleepMode = SLEEP_MODE_3;
static UINT32 countReconTry = 0;

UINT8 app_is_reconnected(void);
UINT16 app_get_ka_value(void);
//azure[[::
dpmAppThreadInfo *pAppDpmThread = NULL;
dpmAppThreadInfo * app_get_thread_info(void)
{
    return pAppDpmThread;
}
//azure]]
UINT32 app_dpm_info_release(void) {
	UINT32 status = 0;

    if (RM_PMGR_W_dpm_is_enabled()) {
        status = RM_PMGR_W_user_rtm_free(AZURE_RTM_NAME);
        if (status) {
            IOT_ERROR("failed to release dpm info in rtm(0x%02x)", status);
        }
    }

    return status;
}

app_dpm_info_rtm* app_dpm_info_get(char *_name) {
	UINT32 status = 0;
	const ULONG wait_option = 100;

	app_dpm_info_rtm *data = NULL;
	UINT32 len = 0;

	if (_name == NULL) {
		len = RM_PMGR_W_user_rtm_get(azure_rtm_name, (unsigned char**) &data);
	} else {
		len = RM_PMGR_W_user_rtm_get(_name, (unsigned char**) &data);
	}

	if (len == 0) {
		if (_name == NULL) {
			status = RM_PMGR_W_user_rtm_pool_alloc(azure_rtm_name, (void**) &data,
					sizeof(app_dpm_info_rtm), wait_option);
		} else {
			status = RM_PMGR_W_user_rtm_pool_alloc(_name, (void**) &data,
					sizeof(app_dpm_info_rtm), wait_option);
		}

		if (status) {
			IOT_ERROR("failed to allocate app_dpm info in rtm(0x%02x)", status);
			return NULL;
		}

		memset(data, 0x00, sizeof(app_dpm_info_rtm));

	} else if (len != sizeof(app_dpm_info_rtm)) {
		IOT_ERROR("invalid size(%ld)", len);
		return NULL;
	}

	if (_name != NULL) {
		memset(azure_rtm_name, 0, sizeof(azure_rtm_name));
		strcpy(azure_rtm_name, _name);
	}
	//IOT_INFO("\"%s\" got dpm data size (%ld)\n", azure_rtm_name, sizeof(app_dpm_info_rtm));

	return data;
}

void app_dpm_set_sleep_flag(UINT8 _flag) {
	flagSleepDPM = _flag;
}

UINT8 app_dpm_get_sleep_flag(void) {
	return flagSleepDPM;
}

IoT_Error_t app_dpm_wakeup_init(void) {
//	if (dpm_mode_is_enabled()) {
#if PMGR
		RM_PMGR_W_dpm_wakeup_done(APP_AZURE_TWIN);
		 bool is_sleep3_wakeup = RM_PMGR_W_dpm_is_wakeup() || RM_PMGR_W_IsSleep3Wakeup();
		 if (is_sleep3_wakeup) {
			 dpm_app_data_rcv_ready_set(APP_AZURE_TWIN);
		 }
#endif
	return SUCCESS;
}

IoT_Error_t app_dpm_goto_sleep(void) {
	if (RM_PMGR_W_dpm_is_enabled()) {
		UINT16 intervalSec = app_get_ka_value();
		IOT_DEBUG(" \"%s\" goto DPM SLEEP...", APP_AZURE_TWIN);
		//DEFAULT_MQTT_KEEPALIVE - from iothubtransport_mqtt_common.c
		if (intervalSec < 60) {
			IOT_ERROR(
					"KA timer interval very short(sec)! - rearranged to 30secs");
			intervalSec = 30;
		} else {
			intervalSec = intervalSec - 20;
		}
		app_dpm_keepalive_timer_register(intervalSec);
		dpm_app_sleep_ready_set(APP_AZURE_TWIN);
	}

	return SUCCESS;
}

IoT_Error_t app_dpm_exit_sleep(void) {
	if (dpm_mode_is_enabled()) {
		//IOT_DEBUG(" \"%s\" exit DPM SLEEP...", APP_AZURE_TWIN);
		//vTaskDelay(1);
		dpm_app_sleep_ready_clear(APP_AZURE_TWIN);
	}

	return SUCCESS;
}

//Don't release rtm memory. it's called by app_dpm_keepalive_timer_register() func
UINT32 app_dpm_keepalive_timer_unregister(void) {
	if (RM_PMGR_W_dpm_is_enabled()) {
		app_dpm_info_rtm *data = app_dpm_info_get(NULL);
		if (data && data->tid) {
			rtc_cancel_timer(data->tid);
			data->tid = 0;
		}
	}

	return ER_SUCCESS;
}
//azure[[::
#if CFG_PMGR
static void aws_app_timer_cb(char *timer_name)
{
    APRINTF_I("AWS App Timer(%s)\n", timer_name);

    RM_PMGR_W_add_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_SLEEP_PROHIBITED);

    return ;
}
#endif
//azure]]
UINT32 app_dpm_keepalive_timer_register(int _intervalSec) {
	UINT32 status = 0;
	app_dpm_info_rtm *data = NULL;
//azure[[:
    char dpmTimerJobName[20] = {0, };
    char dpmTimerName[20] = {0, };
    UINT32 interval;
    INT32 tmpInterval;
//azure]]
	if (RM_PMGR_W_dpm_is_enabled()) {
		IOT_DEBUG("passed interval(sec) (=%d)", _intervalSec);
		if (_intervalSec <= 0) {
			return ER_INVALID_PARAMETERS;
		}

		//clr_dpm_sleep(APP_AZURE_TWIN);

		data = app_dpm_info_get(NULL);
		if (data) {
			if (data->tid) {
				//unregister previous timer
				status = app_dpm_keepalive_timer_unregister();
				if (status) {
					IOT_ERROR("failed to unreigster timer(0x%02x)", status);
					goto end_of_func;
				}

				data->tid = APP_DPM_TIMER_ID;
			}

			data->interval = _intervalSec;
//azure [[:
			interval = app_get_ka_value();
			tmpInterval = interval;

			sprintf(dpmTimerJobName, "%s", APP_AZURE_TWIN);
			sprintf(dpmTimerName, "%sN", APP_AZURE_TWIN);
			data->tid = RM_PMGR_W_dpm_timer_create(dpmTimerJobName, dpmTimerName, aws_app_timer_cb, tmpInterval*1000, 0);
//azure]]
			//register timer
			//data->tid = rtc_register_timer(data->interval*1000,
			//APP_AZURE_TWIN, data->tid, 0, //0: one-shot, 1: periodically
				//	NULL); //azure CHECK commented ?

			if (data->tid < 0) {
				IOT_ERROR("failed to register timer: interval = %d, tid = %d",
						data->interval, data->tid);

				data->tid = APP_DPM_TIMER_ID;

				status = ER_NOT_SUCCESSFUL;
				goto end_of_func;
			}
		} else {
			IOT_ERROR("failed to get sub info from rtm");

			status = ER_NOT_SUCCESSFUL;
			goto end_of_func;
		}
	}

	end_of_func:

	if (RM_PMGR_W_dpm_is_enabled()) {
		//set_dpm_sleep(APP_AZURE_TWIN);
	}

	return status;
}

UINT32 app_dpm_set_client_socket_port(UINT32 _port)
{
    UINT32 status = ER_SUCCESS;
    app_dpm_info_rtm *data = NULL;

    if (RM_PMGR_W_dpm_is_enabled()) {
        data = app_dpm_info_get(NULL);
        if (data) {
            data->bindPort = (int)_port;
        } else {
            status = ER_NO_MEMORY;
            IOT_ERROR("[set] rtm binded port: null");
        }
    }

    return status;
}

UINT32 app_dpm_get_client_socket_port(void) {
	if (RM_PMGR_W_dpm_is_enabled()) {
		app_dpm_info_rtm *data = app_dpm_info_get(NULL);
		if (data) {
			return data->bindPort;
		} else {
			IOT_ERROR("[get] rtm binded port: null");
		}
	}

	return ER_SUCCESS; //return port 0 
}

//azuredpmwork[[
//static UINT32 countReconTry = 0;
static char reportSendFlag = 0;
static char pubAckFlag = 0;

char app_dpm_is_received_pub_ack(void) {
	return pubAckFlag;
}

void app_dpm_set_recv_pub_ack(void) {
	pubAckFlag = 1;
}

char app_dpm_get_report_send_flag(void) {
	return reportSendFlag;
}

void app_dpm_set_report_send_flag(void) {
	reportSendFlag = 1;
}

UINT32 app_get_random_local_port(void) {
	static UINT32 appDummyCount = 0;
	UINT32 port;

	generate_port: ++appDummyCount;

	port = 30000 + ((rand() * xTaskGetTickCount()) % 10000) + appDummyCount;

	if (appDummyCount > 25000) {
		appDummyCount = 0;
		goto generate_port;
	}

	APRINTF("port re-generated: #%d, retry count: %d\n", port,
			countReconTry + 1);
	countReconTry++;

	return port;
}

UINT32 app_get_local_port(void) {
	UINT32 port;

	port = AZURE_MQTT_PORT;
	countReconTry++;

	return port;
}

UINT32 app_get_count_of_reconnection(void) {
	return countReconTry;
}

UINT32 app_socket_get_local_port(void) {
	return m_local_port;
}

void app_socket_set_port_n_filter(UINT32 _defLocalPort) {
	uint32_t localPort = app_dpm_get_client_socket_port();

    if (app_is_reconnected()) {
        if (localPort != UNDEF_PORT) {
            //delete local port
            RM_WIFI_dpm_tcp_port_delete((uint16_t)localPort);
        }
        //generate ramdom port
        localPort = app_get_local_port();
        if (!app_dpm_set_client_socket_port(localPort)) {
            APRINTF("saving binded port num=%lu\n", localPort);
        } else {
            APRINTF_E("saving binded port num=%lu: error\n", localPort);
        }
        localPort = app_dpm_get_client_socket_port();
    }

    if (localPort == UNDEF_PORT) {
        localPort = _defLocalPort;
    }

	if (RM_PMGR_W_dpm_is_enabled()) {
		uint32_t status = ER_SUCCESS;
		status = RM_PMGR_W_dpm_job_name_is_set(APP_AZURE_TWIN);
		if (status == DPM_NOT_REGISTERED) {
			APRINTF(
					"\n %s is not registered. Will be registered with %d port\n",
					APP_AZURE_TWIN, localPort);

			RM_PMGR_W_dpm_job_name_set(APP_AZURE_TWIN, localPort);
			app_dpm_set_client_socket_port(localPort);
		} else {
			UINT32 save_port = app_dpm_get_client_socket_port();

			if (save_port != UNDEF_PORT) {
				char *reg_name = RM_PMGR_W_dpm_port_is_set(save_port);

				if (strncmp(reg_name, APP_AZURE_TWIN, strlen(APP_AZURE_TWIN))) {
					RM_PMGR_W_dpm_job_name_clear(APP_AZURE_TWIN);
					RM_PMGR_W_dpm_job_name_set(APP_AZURE_TWIN, save_port);

					APRINTF_S("\n\"%s\" re-registered to port #%d for DPM process\n", APP_AZURE_TWIN, save_port);
				} else {
					APRINTF(
							"\n\"%s\" already registered to port #%d for DPM process\n",
							APP_AZURE_TWIN, save_port);
				}
			} else {
				char *reg_name = RM_PMGR_W_dpm_port_is_set(localPort);
				if (strncmp(reg_name, APP_AZURE_TWIN, strlen(APP_AZURE_TWIN))) {
					APRINTF_E("\nport #%d registered to \"%s\" (wrong).\n", localPort, reg_name);
				} else {
					APRINTF(
							"\n\"%s\" already registered to port #%d for DPM process\n",
							APP_AZURE_TWIN, localPort);
				}
			}
		}
		RM_PMGR_W_dpm_job_name_set(APP_AZURE_TWIN, localPort);

		app_dpm_set_client_socket_port(localPort);
		/* Set tcp client' binding port to filter in DPM */
		RM_WIFI_dpm_tcp_port_filter_set((uint16_t) localPort);

		//call this API at early time when application thread start
		//app_dpm_wakeup_init();
	}
	m_local_port = _defLocalPort;
}

UINT8 app_is_reconnected(void) {
	return flagReconnected;
}

void app_set_reconnect_flag(UINT8 _value) {
	flagReconnected = _value;
}

UINT8 app_dpm_get_recv_timeout_flag(void) {
	return azure_dpm_recv_timeout_alt_flag;
}

void app_dpm_set_recv_timeout_flag(UINT8 _value) {
	azure_dpm_recv_timeout_alt_flag = _value;
	if (_value == DPM_RCV_OK_SLEEP) {
		app_print_elapse_time_ms("[%s:%d] DPM_RCV_OK_SLEEP set", __func__,
				__LINE__);
	}
}

//dpm_debug
void app_dpm_put_to_sleep(){
	RM_PMGR_W_remove_sleep_constraint(RM_PMGR_W_get_ctrl(), PMGR_CONSTRAINT_POWER_RAM);
}

void app_set_ka_value(UINT16 _kaVal) {
	secondsKeepalive = _kaVal;
	app_print_elapse_time_ms("[%s:%d] KA set to %d", __func__, __LINE__,
			secondsKeepalive);
}

UINT16 app_get_ka_value(void) {
	return secondsKeepalive;
}

UINT32 app_dpm_set_subscribe_info(UINT16 _packetIDSub, UINT8 _qosSubCount,
		UINT32 *_qosSubReturn) {
	UINT32 status = ER_SUCCESS;
	app_dpm_info_rtm *data = NULL;

	if (RM_PMGR_W_dpm_is_enabled()) {
		data = app_dpm_info_get(NULL);
		if (data) {
			int i;
			data->packetIdSub = _packetIDSub;
			data->qosSubCount = _qosSubCount;
			memset(data->qosSubReturn, 0, sizeof(data->qosSubReturn));
			if (data->qosSubCount
					> sizeof(data->qosSubReturn)|| _qosSubReturn == NULL) {
				IOT_ERROR(
						"[set] passed parameter _qosSubCount out of range or _qosSubReturn NULL");
				data->packetIdSub = 0;
				data->qosSubCount = 0;
				return ER_ARGUMENT;
			}
			for (i = 0; i < data->qosSubCount; i++) {
				data->qosSubReturn[i] = _qosSubReturn[i];
			}
		} else {
			status = ER_NO_MEMORY;
			IOT_ERROR("[set] rtm data: null");
		}
	}

	return status;
}

UINT32* app_dpm_get_subscribe_info(UINT16 *_packetIDSub, UINT8 *_qosSubCount) {
	if (RM_PMGR_W_dpm_is_enabled()) {

app_dpm_info_rtm *data = app_dpm_info_get(NULL);
		if (data) {
			*_packetIDSub = data->packetIdSub;
			*_qosSubCount = data->qosSubCount;
			return data->qosSubReturn;
		} else {
			IOT_ERROR("[get] rtm data: null");
			*_packetIDSub = 0;
			*_qosSubCount = 0;
			return NULL;
		}
	}
	return NULL;
}

void app_dpm_set_rcv_ready(void) {
	RM_PMGR_W_dpm_rcv_ready_set(APP_AZURE_TWIN);
	m_is_subscribed = 1;
	app_print_elapse_time_ms("[%s:%d] DPM rcv ready & suscription completed",
			__func__, __LINE__);
}

UINT8 app_dpm_is_subscribed(void) {
	return m_is_subscribed;
}
//]]

IoT_Error_t azure_tls_save(const char *name, mbedtls_ssl_context *sslCtx) {
	INT32 ret = 0;

     if (RM_PMGR_W_dpm_is_enabled()) {
        if ((strlen(name) == 0) || (strlen(name) > REG_NAME_DPM_MAX_LEN)) {
            APRINTF_E("[%s:%d] user rtm name's length has to be less than %d\n", __func__, __LINE__, REG_NAME_DPM_MAX_LEN);
            return FAILURE;
        }

        ret = RM_PMGR_W_dpm_tls_session_set(name, sslCtx);
        if (ret) {
            APRINTF_E("[%s:%d] Failed to save tls session(0x%x)\n",__func__, __LINE__, -ret);
            return FAILURE;
        }
    }

    return SUCCESS;
}

IoT_Error_t azure_tls_restore(const char *name, mbedtls_ssl_context *sslCtx) {
	INT32 ret = 0;

    if (RM_PMGR_W_dpm_is_enabled()) {
        if ((strlen(name) == 0) || (strlen(name) > REG_NAME_DPM_MAX_LEN)) {
            APRINTF_E("[%s:%d] user rtm name's length has to be less than %d\n", __func__, __LINE__, REG_NAME_DPM_MAX_LEN);
            return FAILURE;
        }

        ret = RM_PMGR_W_dpm_tls_session_get(name, sslCtx);
        if (ret == ER_NOT_FOUND) {
            APRINTF_E("[%s:%d] Not found(%s)\n", __func__, __LINE__, name);
            return NETWORK_SSL_UNKNOWN_ERROR;
        } else if (ret != 0) {
            if (ret == ER_INVALID_PARAMETERS) {
                //APRINTF("%s:%d ----ssl_ctx=0x%x, ssl_ctx->state=%d\n", __func__, __LINE__, tlsDataParams->ssl_ctx, tlsDataParams->ssl_ctx->state);
                if (sslCtx) {
                    if (sslCtx->MBEDTLS_PRIVATE(state) == MBEDTLS_SSL_HANDSHAKE_OVER) {
                        goto NEXT_STEP;
                    }
                }
            }

            APRINTF_E("[%s:%d] Failed to restore tls session(0x%x)\n", __func__, __LINE__, ret);
            return FAILURE;
        } else {
           // APRINTF("[%s:%d] restore tls session OK, state = %d\n", __func__, __LINE__, sslCtx->state);
        }

        NEXT_STEP: if (RM_PMGR_W_dpm_is_wakeup()) {
            //RM_PMGR_W_dpm_rcv_ready_set(APP_AZURE_TWIN);

			/*
			 if (WAKEUP_SOURCE_FN() == WAKEUP_SENSOR_EXT_SIGNAL ||WAKEUP_TYPE_FN() != DPM_PACKET_WAKEUP)
			 {
			 if (WAKEUP_SOURCE_FN() == WAKEUP_SENSOR_EXT_SIGNAL)
			 {
			 APRINTF("[%s] sensor wakeup mode\n", __func__);
			 }
			 else
			 {
			 APRINTF("[%s] not the mode of receiving packet\n", __func__);
			 }
			 return NULL_VALUE_ERROR;
			 }
			 */
			return SUCCESS;
		} else {
			return FAILURE;
		}
	} else {
		return SUCCESS;
	}
}

IoT_Error_t azure_tls_clear(const char *name) {
	INT32 ret = 0;

	if ((strlen(name) == 0) || (strlen(name) > REG_NAME_DPM_MAX_LEN)) {
		APRINTF_E("[%s:%d] user rtm name's length has to be less than %d\n",
				__func__, __LINE__, REG_NAME_DPM_MAX_LEN);
		return FAILURE;
	}

	ret = RM_PMGR_W_dpm_tls_session_clear(name);
	if (ret) {
		APRINTF_E("[%s:%d] Failed to clear tls session(0x%x)\n", __func__, __LINE__, ret);
		return FAILURE;
	}

	return SUCCESS;
}

void app_dpm_set_skip_saving_serverip(bool skip_flag) {
	skip_save_dns_ip_flag = skip_flag;
}

bool app_dpm_get_skip_saving_serverip(void) {
	return skip_save_dns_ip_flag;
}

#ifdef __cplusplus
}
#endif

