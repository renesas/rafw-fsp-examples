/***********************************************************************************************************************
 * File Name    : app_aws_atcmd_parse.h
 * Description  : Header file for AT command parser for AWS IoT
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* ${REA_DISCLAIMER_PLACEHOLDER} */

#ifndef RM_ATCMD_W_CORE_AWS_PARSE_H
#define RM_ATCMD_W_CORE_AWS_PARSE_H

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#include "bsp_api.h"
#if CFG_WIFI
#include "rm_wifi.h"
#if (ATCMD_IF_SUPPORT == 1)
#include "sdk_defs.h"
#include "rm_atcmd_w_core_common.h"

/* Common macro for FSP header files.
 * There is also a corresponding FSP_FOOTER
 * macro at the end of this file. */
FSP_HEADER

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define MAX_ARGV_NUM                                5
#define AWS_NVRAM_CFG_PORT                          "PORT" ///< APP local port on NVRAM
#define AWS_NVRAM_CFG_DEV_ID                        "DEVICE_ID"
#define AWS_NVRAM_CFG_FP_TMPL_NAME                  "FP_TMPL_NAME"
#define AWS_NVRAM_CFG_BROKER_URL                    "AWS_BROKER"
/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
void add_aws_atcmd(void);
int app_iot_set_feature_parser(int argc, char *argv[]);
/***********************************************************************************************************************
 * AT Function Prototypes
 **********************************************************************************************************************/
uint32_t RM_ATCMD_W_CORE_AWS_register(atcmd_w_core_module_list_t *p_list);
uint32_t RM_ATCMD_W_CORE_AWS_deregister(atcmd_w_core_module_list_t *p_list);
uint32_t RM_ATCMD_W_CORE_AWS_open(atcmd_w_ctrl_t *const p_atcmd_w_ctrl);
uint32_t RM_ATCMD_W_CORE_AWS_close(atcmd_w_ctrl_t *const p_atcmd_w_ctrl);

/* Common macro for FSP header files.
 * There is also a corresponding FSP_HEADER
 * macro at the top of this file. */
FSP_FOOTER

#endif /* __SUPPORT_AWS_IOT_W__ */
#endif /* CFG_WIFI */
#endif /* RM_ATCMD_W_CORE_AWS_PARSE_H */

