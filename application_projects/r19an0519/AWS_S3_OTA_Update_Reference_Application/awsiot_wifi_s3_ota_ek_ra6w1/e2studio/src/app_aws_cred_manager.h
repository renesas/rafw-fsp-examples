/***********************************************************************************************************************
 * File Name    : app_aws_cred_manager.h
 * Description  : Header file for holding the declarations of functions in app_aws_cred_manager.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"

#ifndef __APP_AWS_CRED_MANAGER_H__
#define __APP_AWS_CRED_MANAGER_H__

#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_dpm_sample.h"
#include "core_mqtt_serializer.h"
#include "app_aws_user_conf.h"
#include "ota_update.h"
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
#define APP_MALLOC                                      pvPortMalloc
#define APP_FREE                                        vPortFree

void load_aws_credentials(void);
void free_aws_credentials(void);
char *aws_credentials_read(uint32_t addr);
#endif /* __APP_AWS_CRED_MANAGER_H__ */
