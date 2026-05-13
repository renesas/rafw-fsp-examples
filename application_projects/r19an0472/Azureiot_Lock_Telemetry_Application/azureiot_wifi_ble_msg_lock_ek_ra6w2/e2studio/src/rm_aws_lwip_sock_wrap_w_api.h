/*
 * rm_aws_lwip_sock_wrap_w_api.h
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

#ifndef IOT_APP_SUPPORT_SOCKET_WRAPPER_INTERFACE_H_
#define IOT_APP_SUPPORT_SOCKET_WRAPPER_INTERFACE_H_
#include <stdbool.h>
#include "app_dpm_thread.h"
#include "sdk_defs.h"

/** Used to indicate receiving timeout on DPM mode */
#define DPM_RCV_NO_CONNECT	0 ///< before connection with server
#define DPM_RCV_OK_CONNECT	1 ///< connection checked
#define DPM_RCV_OK_SLEEP	2 ///< ready to sleep

#define WAKEUP_SOURCE_FN      RM_PMGR_W_dpm_wakeup_src_get

typedef enum {
    PROP_OP_GET,
    PROP_OP_SET,
	PROP_OP_DEL,
} property_op_t;


typedef enum {
    PROP_SVR_IP,
    PROP_SNTP,
    PROP_DNS_IP,
} property_id_t;

typedef struct {
    property_id_t id;
    property_op_t op;
    void *value; // pointer to value (can be int*, float*, struct*, etc.)
} property_msg_t;


typedef void (*app_dpm_timer_callback)(UINT8 _resetTimer, APPTimerMode _mode);
typedef void (*exit_sleep_thread_callback)(void);

/// internal used RTM structure
typedef struct _InternalRTM {
    /// timer id
    INT32 tid;
    /// timer timeout (by seconds)
    INT32 interval;
    /// timer purpose - 0: normal, 1~9: abnormal
    UINT8 mode;
} InternalRTM;

/// structure for DPM App thread
typedef struct _dpmAppThreadInfo {
    TaskHandle_t thread;

    UINT32 entryInput;
    UINT32 pAppData;
    UINT32 RTMDataSize;
    UINT32 internalStatus;
    UINT32 wakeUpTime;
    UINT32 dpmPort;

    char *threadName;
    char *DPMRegeditName;
    char *internalRTMName;
    char *externalRTMName;
    char *DNSAddr;
    char *secDNSAddr;

    UINT32 externalRTM;
    InternalRTM *internalRTM;
    INT32 currentState;

    bool DpmMode;
    bool DpmWakeUp;

    UINT32 sntpCount;
    app_dpm_timer_callback dpm_timer_callback;
    exit_sleep_thread_callback exit_dpm_sleep_cb;
} dpmAppThreadInfo;

extern dpmAppThreadInfo *pAppDpmThread;

typedef void *(*persistant_storage_read_cb_t)(property_id_t id);
typedef int   (*persistant_storage_write_cb_t)(property_id_t id, const void *value);

#endif /* IOT_APP_SUPPORT_SOCKET_WRAPPER_INTERFACE_H_ */
