/***********************************************************************************************************************
 * File Name    : app_topic_name_manager.c
 * Description  : Define topic related APIs used to operate device on AWS platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "FreeRTOS.h"
#include "custom_config_sdk.h"
/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#if defined(__SUPPORT_AWS_IOT_W__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_def.h"
#include "common_utils.h"
#include "app_sample_manager.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_dpm_thread.h"
#include "app_aws_user_conf.h"
#include "aws_iot_error.h"
#include "app_topic_name_manager.h"
#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#endif

#define PRE_FIXED_AWS_MY_THING_NAME AWS_IOT_THING_NAME
#define AWS_CONNECT_CHECK_PUB_PRE "/DeviceConnect"
#define AWS_CONTROL_COMMAND_SUB_PRE "/AppControl"
#define AWS_CONTROL_COMMAND_PUB_PRE "/DeviceControl"

char *aws_thing_name = NULL;
char *aws_connect_check_pub_name = NULL;
char *aws_control_command_sub_name = NULL;
char *aws_control_command_pub_name = NULL;

void makeAWSTopicName(void)
{
    char *nvramName = NULL;
    size_t nvramLen;
    size_t check_pub_pre_length = strlen(AWS_CONNECT_CHECK_PUB_PRE);
    size_t com_sub_pre_length = strlen(AWS_CONTROL_COMMAND_SUB_PRE);
    size_t com_pub_pre_length = strlen(AWS_CONTROL_COMMAND_PUB_PRE);

#if defined(__SUPPORT_ATCMD__)
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_THINGNAME, &nvramName);
#else
#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_CONFIG_THINGNAME, &nvramName);
#else
    nvramName = read_nvram_appcfg_string(APP_CONFIG_THINGNAME);
#endif 
#endif 

    IOT_INFO("[Make AWS-Thing-Name]")
    if (nvramName == NULL)
    {
        size_t prefixNameLen = strlen(PRE_FIXED_AWS_MY_THING_NAME);

        if (aws_thing_name != NULL)
        {
            app_common_free(aws_thing_name);
            aws_thing_name = NULL;
        }
        aws_thing_name = (char*)app_common_malloc(prefixNameLen + 1);
        memset(aws_thing_name, 0x00, prefixNameLen + 1);
        memcpy(aws_thing_name, PRE_FIXED_AWS_MY_THING_NAME, prefixNameLen);
        /* adding string */

        IOT_INFO("[Built-name] AWS Thing name : [%s] (len=%d)", aws_thing_name, prefixNameLen)

        if (aws_connect_check_pub_name != NULL)
        {
            app_common_free(aws_connect_check_pub_name);
            aws_connect_check_pub_name = NULL;
        }
        aws_connect_check_pub_name = (char*)app_common_malloc(prefixNameLen + check_pub_pre_length + 1);

        memset(aws_connect_check_pub_name, 0x00, prefixNameLen + check_pub_pre_length + 1);
        if (aws_control_command_sub_name != NULL)
        {
            app_common_free(aws_control_command_sub_name);
            aws_control_command_sub_name = NULL;
        }
        aws_control_command_sub_name = (char*)app_common_malloc(prefixNameLen + com_sub_pre_length + 1);
        memset(aws_control_command_sub_name, 0x00, prefixNameLen + com_sub_pre_length + 1);

        if (aws_control_command_pub_name != NULL)
        {
            app_common_free(aws_control_command_pub_name);
            aws_control_command_pub_name = NULL;
        }        
        aws_control_command_pub_name = (char*)app_common_malloc(prefixNameLen + com_pub_pre_length + 1);
        memset(aws_control_command_pub_name, 0x00, prefixNameLen + com_pub_pre_length + 1);

        memcpy(aws_connect_check_pub_name, PRE_FIXED_AWS_MY_THING_NAME, prefixNameLen);
        memcpy(aws_connect_check_pub_name + prefixNameLen, AWS_CONNECT_CHECK_PUB_PRE, check_pub_pre_length);
        memcpy(aws_control_command_sub_name, PRE_FIXED_AWS_MY_THING_NAME, prefixNameLen);
        memcpy(aws_control_command_sub_name + prefixNameLen, AWS_CONTROL_COMMAND_SUB_PRE, com_sub_pre_length);
        memcpy(aws_control_command_pub_name, PRE_FIXED_AWS_MY_THING_NAME, prefixNameLen);
        memcpy(aws_control_command_pub_name + prefixNameLen, AWS_CONTROL_COMMAND_PUB_PRE, com_pub_pre_length);
        IOT_INFO("[Built-name] [%s][%s][%s]", aws_connect_check_pub_name, aws_control_command_sub_name,
                 aws_control_command_pub_name)
    }
    else
    {
        nvramLen = strlen(nvramName);

        if (aws_thing_name != NULL)
        {
            app_common_free(aws_thing_name);
            aws_thing_name = NULL;
        }
        aws_thing_name = (char*)app_common_malloc(nvramLen + 1);
        memset(aws_thing_name, 0x00, nvramLen + 1);
        strncpy(aws_thing_name, nvramName, nvramLen);
        /* adding string */

        IOT_INFO("[NVRAM] AWS Thing name : [%s] (len=%d)", aws_thing_name, nvramLen)

        if (aws_connect_check_pub_name != NULL)
        {
            app_common_free(aws_connect_check_pub_name);
            aws_connect_check_pub_name = NULL;
        }
        aws_connect_check_pub_name = (char*)app_common_malloc(nvramLen + check_pub_pre_length + 1);
        memset(aws_connect_check_pub_name, 0x00, nvramLen + check_pub_pre_length + 1);

        if (aws_control_command_sub_name != NULL)
        {
            app_common_free(aws_control_command_sub_name);
            aws_control_command_sub_name = NULL;
        }
        aws_control_command_sub_name = (char*)app_common_malloc(nvramLen + com_sub_pre_length + 1);
        memset(aws_control_command_sub_name, 0x00, nvramLen + com_sub_pre_length + 1);

        if (aws_control_command_pub_name != NULL)
        {
            app_common_free(aws_control_command_pub_name);
            aws_control_command_pub_name = NULL;
        }
        aws_control_command_pub_name = (char*)app_common_malloc(nvramLen + com_pub_pre_length + 1);
        memset(aws_control_command_pub_name, 0x00, nvramLen + com_pub_pre_length + 1);
        memcpy(aws_connect_check_pub_name, aws_thing_name, nvramLen);
        memcpy(aws_connect_check_pub_name + nvramLen, AWS_CONNECT_CHECK_PUB_PRE, check_pub_pre_length);
        memcpy(aws_control_command_sub_name, aws_thing_name, nvramLen);
        memcpy(aws_control_command_sub_name + nvramLen, AWS_CONTROL_COMMAND_SUB_PRE, com_sub_pre_length);
        memcpy(aws_control_command_pub_name, aws_thing_name, nvramLen);
        memcpy(aws_control_command_pub_name + nvramLen, AWS_CONTROL_COMMAND_PUB_PRE, com_pub_pre_length);
        IOT_INFO("[NVRAM] [%s][%s][%s] ", aws_connect_check_pub_name, aws_control_command_sub_name,
                 aws_control_command_pub_name)
    }
}

char* getAWSCheckPubName(void)
{
    if (aws_connect_check_pub_name == NULL)
        makeAWSTopicName();

    return aws_connect_check_pub_name;
}

char* getAWSCommandSubName(void)
{
    if (aws_control_command_sub_name == NULL)
        makeAWSTopicName();

    return aws_control_command_sub_name;
}

char* getAWSCommandPubName(void)
{
    if (aws_control_command_pub_name == NULL)
        makeAWSTopicName();

    return aws_control_command_pub_name;
}

#endif
