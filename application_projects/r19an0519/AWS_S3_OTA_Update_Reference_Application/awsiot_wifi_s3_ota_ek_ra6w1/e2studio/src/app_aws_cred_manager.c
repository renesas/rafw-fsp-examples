/***********************************************************************************************************************
 * File Name    : app_aws_cred_manager.c
 * Description  : File manages the AWS credentials like rootca, certificate and key for the application.  
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#if defined (__SUPPORT_AWS_IOT_W__)
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "nvedit.h"
#include "sys_feature.h"
#include "app_dpm_sample.h"
#include "app_dpm_thread.h"
#include "app_aws_cred_manager.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_aws_user_conf.h"
#include "shadow.h"
#include "ota_update.h"
#if defined(__SUPPORT_ATCMD__)
#include "app_aws_atcmd_parse.h"
#endif
#include "rm_cert.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"

#ifndef APP_UNUSED_ARG
#define APP_UNUSED_ARG(x)                   (void)x
#endif

char *aws_root_ca = NULL;
char *aws_client_cert = NULL;
char *aws_priv_key = NULL;

void load_aws_credentials(void)
{
    size_t aws_root_ca_len = CERT_MAX_LENGTH;
    size_t aws_client_cert_len = CERT_MAX_LENGTH;
    size_t aws_priv_key_len = CERT_MAX_LENGTH;
    rm_cert_format_t format = RM_CERT_FORMAT_PEM;

    if (aws_root_ca == NULL)
    {
        aws_root_ca = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_root_ca, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_CA_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_AWS_CA_ADDR), &format,
                     (unsigned char *)aws_root_ca, &aws_root_ca_len);

    }
    if (aws_client_cert == NULL)
    {
        aws_client_cert = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_client_cert, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR), &format,
                     (unsigned char *)aws_client_cert, &aws_client_cert_len);
    }
    if (aws_priv_key == NULL)
    {
        aws_priv_key = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_priv_key, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR), &format,
                     (unsigned char *)aws_priv_key, &aws_priv_key_len);
    }
}

void free_aws_credentials(void)
{
    if (aws_root_ca)
    {
        APP_FREE(aws_root_ca);
        aws_root_ca = NULL;
    }
    if (aws_client_cert)
    {
        APP_FREE(aws_client_cert);
        aws_client_cert = NULL;
    }
    if (aws_priv_key)
    {
        APP_FREE(aws_priv_key);
        aws_priv_key = NULL;
    }
}

char *aws_credentials_read(uint32_t addr)
{
    if ((aws_root_ca == NULL) || (aws_client_cert == NULL) || (aws_priv_key == NULL))
    {
        free_aws_credentials();
        load_aws_credentials();
    }

    switch(addr)
    {
    case SF_TLS_CERT_AWS_CA_ADDR:
        if (aws_root_ca)
        {
            return aws_root_ca;
        }
        break;
    case SF_TLS_CERT_AWS_INITIAL_CERT_ADDR:
        if (aws_client_cert)
        {
            return aws_client_cert;
        }
        break;
    case SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR:
        if (aws_priv_key)
        {
            return aws_priv_key;
        }
        break;
    default:
        printf("Wrong input for getting aws credentials\n\r");

        return NULL;
    }

    printf("Could not get the aws credentials\n\r");

    return NULL;
}
#endif  /* __SUPPORT_AWS_IOT_W__ */
