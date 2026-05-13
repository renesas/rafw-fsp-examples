/***********************************************************************************************************************
 * File Name    : app_dpm_sample.h
 * Description  : Header file for supporting the source file, app_dpm_sample.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "stdbool.h"
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"

#if !defined(_APP_DPM_SAMPLE_H_)
#define _APP_DPM_SAMPLE_H_

/*! print function for APP debugging */
#if !defined(APRINTF)
#define APRINTF(...)                        printf(__VA_ARGS__)
#endif

#if !defined(APRINTF_Y)
#define APRINTF_Y                           APRINTF
#endif
#if !defined(APRINTF_I)
#define APRINTF_I                           APRINTF
#endif
#if !defined(APRINTF_S)
#define APRINTF_S                           APRINTF
#endif
#if !defined(APRINTF_E)
#define APRINTF_E                           APRINTF
#endif

/* add OTA related state */
#define AWS_OTA_RESULT_OK                   0 ///< OK for OTA update
#define AWS_OTA_RESULT_NG                   1 ///< NG for OTA update
#define AWS_OTA_RESULT_UNKNOWN              0xFF ///< unknown state for OTA update (default)

#define AWS_OTA_STATE_READY                 1 ///<
#define AWS_OTA_STATE_UNKOWN                0xFF ///<

void aws_s3_ota_app_start(void *arg);
char* app_get_ota_url(void);
char* app_get_ota_result(void);

#endif /* _APP_DPM_SAMPLE_H_ */

/* EOF */
