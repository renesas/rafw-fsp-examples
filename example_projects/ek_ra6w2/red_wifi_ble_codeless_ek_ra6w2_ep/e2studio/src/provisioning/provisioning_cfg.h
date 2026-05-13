/***********************************************************************************************************************

* File Name    : provisioning_cfg.h

* Description  : Provisioning config declaration

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#ifndef PROVISIONING_W_CFG_H
#define PROVISIONING_W_CFG_H

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#ifndef WLAN0_IFACE
#define WLAN0_IFACE                     (0)
#endif

#ifndef WLAN1_IFACE
#define WLAN1_IFACE                     (1)
#endif

#ifndef ENV_SYS_MODE
#define ENV_SYS_MODE                "SYSMODE"
#endif

#ifndef DFLT_NETMODE_1
#define DFLT_NETMODE_1                      DEFAULT_NETMODE_WLAN1
#endif

#ifndef DHCPCLIENT
#define DHCPCLIENT          1
#endif

#ifndef STATIC_IP
#define STATIC_IP           2
#endif

#ifndef DEFAULT_NETMODE_WLAN1
#define DEFAULT_NETMODE_WLAN1       STATIC_IP
#endif

#ifndef PROVISIONING_W_CFG_PARAM_CHECKING_ENABLE
 #define PROVISIONING_W_CFG_PARAM_CHECKING_ENABLE    (1)
#endif

#ifndef PROVISIONING_W_WATCHDOG_SERVICE_ENABLE
 #define PROVISIONING_W_WATCHDOG_SERVICE_ENABLE      (PROVISIONING_W_CFG_WATCHDOG_SERVICE_ENABLE)
#endif

#ifndef SCAN_RESULT_MAX
 #define SCAN_RESULT_MAX                             (30) // SHOULD BE OUT
#endif

#ifndef PROVISIONING_TYPE
#if (ATCMD_IF_SUPPORT)
 #define __PROVISION_ATCMD__
 #define PROVISIONING_TYPE                         (11) //default: 1, aws-iot: 10
#else
 #define PROVISIONING_TYPE                         (10) //default: 1, aws-iot-w: 10
#endif
#endif

#if 1 == AWS_PROVISIONING_TEST_ENABLE
 #include "awsiot_w_cfg.h"
 #define PROVISIONING_W_APP_THING_NAME AWS_IOT_THING_NAME
#else
#ifndef PROVISIONING_APP_THING_NAME
 #define PROVISIONING_APP_THING_NAME    "RRQ61xxx"
#endif
#endif

#endif
