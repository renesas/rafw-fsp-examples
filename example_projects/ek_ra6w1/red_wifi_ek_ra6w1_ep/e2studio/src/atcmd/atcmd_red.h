/***********************************************************************************************************************

* File Name    : atcmd_red.h

* Description  : Header file for AT command RED

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/


#ifndef RM_ATCMD_SECURE_STORAGE_PARSE_H
#define RM_ATCMD_SECURE_STORAGE_PARSE_H

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#include "rm_atcmd_w_core_common.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef enum _atcmd_provision_stat
{
    /// Provisioning not working or finish
    ATCMD_PROVISION_IDLE                            = 0,

    /// Provisioning start
    ATCMD_PROVISION_START                           = 1,

    /// Received AP info from mobile
    ATCMD_PROVISION_SELECTED_AP_SUCCESS             = 101,
    ATCMD_PROVISION_SELECTED_AP_FAIL                = 102,

    /// AP connection fail check SSID or PW
    ATCMD_PROVISION_WRONG_PW                        = 103,    // New add
    ATCMD_PROVISION_AP_FAIL                         = 105,

    /// Network checking status
    ATCMD_PROVISION_DNS_FAIL_SERVER_FAIL            = 106,
    ATCMD_PROVISION_DNS_FAIL_SERVER_OK              = 107,    // in case wrong DNS name
    ATCMD_PROVISION_NO_URL_PING_FAIL                = 108,
    ATCMD_PROVISION_NO_URL_PING_OK                  = 109,
    ATCMD_PROVISION_DNS_OK_PING_FAIL_N_SERVER_OK    = 110,    // in case AP gives wrong IP
    ATCMD_PROVISION_DNS_OK_PING_OK                  = 111,
    ATCMD_PROVISION_REBOOT_ACK                      = 112,
    ATCMD_PROVISION_DNS_OK_PING_N_SERVER_FAIL       = 113     // in case default ping fail

} atcmd_provision_stat;

/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
uint32_t RM_ATCMD_W_CORE_AWS_open(atcmd_w_ctrl_t *const p_atcmd_w_ctrl);
uint32_t RM_ATCMD_W_CORE_AWS_close(atcmd_w_ctrl_t *const p_atcmd_w_ctrl);
void RM_PL_PRINTF_ATCMD(char *p_str);
void atcmd_provstat(int status);
void add_red_atcmd(void);
#endif /* RM_ATCMD_W_CORE_SECURE_STORAGE_PARSE_H */
