/***********************************************************************************************************************
 * File Name    : app_dpm_thread.c
 * Description  : Define the DPM thread running on Apps module.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "bsp_api.h"
#include "projdefs.h"
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include <stdio.h>
#include "oal.h"
#include "net_common.h"
#include "net_sntp_client.h"
#include "net_dns_client.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "iface_defs.h"
#include "nvedit.h"
#include "sys_feature.h"
#include "common_def.h"
#include "common_utils.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"
#include "user_dpm.h"
#if CFG_PMGR
#include "rm_pmgr_w_instance.h"
#include "r_pm_if.h"
#endif
#include "rm_awsiot_w_cfg.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_dpm_thread.h"
#include "app_aws_user_conf.h"
#include "app_dpm_sample.h"

static APPSleepMode appSleepMode = SLEEP_MODE_3;
static unsigned char sleepRTMMem = 0;
static int appUsecTimer = 0;

void setSleepMode(APPSleepMode _mode, int _sec, unsigned char _retention)
{
    appSleepMode = _mode;
    appUsecTimer = _sec;
    sleepRTMMem = _retention;

    appSetSleepMode(_mode);
    APRINTF_S("\n===========================================\n\n");
    if (_mode == SLEEP_MODE_NONE)
    {
        APRINTF_E("[ Set valid sleep mode !!! ] \n\n");
    }
    else
    {
        APRINTF("[ Set Sleep mode : %d , RTC Wakeup time (sec) : %d ,useRTM : %d ] \n\n", _mode, appUsecTimer, sleepRTMMem);
    }
    APRINTF_S("============================================\n\n");
}

APPSleepMode getSleepMode(void)
{
    return appSleepMode;
}
