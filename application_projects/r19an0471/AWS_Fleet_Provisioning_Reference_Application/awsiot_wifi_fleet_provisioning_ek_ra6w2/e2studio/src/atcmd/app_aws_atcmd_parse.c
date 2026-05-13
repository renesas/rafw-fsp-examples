/***********************************************************************************************************************
 * File Name    : app_aws_atcmd_parse.c
 * Description  : AT command parser for AWS IoT
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* ${REA_DISCLAIMER_PLACEHOLDER} */
/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
#if CFG_WIFI
#include "rm_wifi.h"
#if (ATCMD_IF_SUPPORT == 1)
#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdlib.h>
#include "bsp_io.h"
#include "common_data.h"
#include "app_aws_atcmd_parse.h"
#include "app_dpm_thread.h"
#include "rm_atcmd_w_core_err_code.h"
#include "rm_atcmd_w_core.h"
#include "rm_atcmd_w_app.h"
#include "rm_atcmd_transport_uart_w.h"
/* MQTT library includes. */
#include "core_mqtt.h"
#include "app_aws_user_conf.h"
#include "app_dpm_interface.h"
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_cert.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define RM_ATCMD_W_CORE_AWS_ATCMD_CODE(atcmd)         "AT+" # atcmd
#define RM_ATCMD_W_CORE_AWS_ATCMD_CB(atcmd) \
	uint32_t RM_ATCMD_W_CORE_AWS_ ## atcmd ## _cmd_cb(atcmd_w_ctrl_t * const p_atcmd_w_ctrl, int argc, char *argv[])
#define RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(atcmd)  \
    const char *RM_ATCMD_W_CORE_AWS_ ## atcmd ## _format_cb(void)
#define RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(atcmd)   \
    const char *RM_ATCMD_W_CORE_AWS_ ## atcmd ## _brief_cb(void)

#define RM_ATCMD_W_CORE_AWS_ATCMD_CB_P(atcmd)         RM_ATCMD_W_CORE_AWS_ ## atcmd ## _cmd_cb
#define RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB_P(atcmd)  RM_ATCMD_W_CORE_AWS_ ## atcmd ## _format_cb
#define RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB_P(atcmd)   RM_ATCMD_W_CORE_AWS_ ## atcmd ## _brief_cb

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
RM_ATCMD_W_CORE_AWS_ATCMD_CB(AWS);
RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(AWS);
RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(AWS);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
atcmd_w_core_module_t atcmd_w_core_aws_module[] =
{
    {
        RM_ATCMD_W_CORE_AWS_ATCMD_CODE(AWS),
        ATCMD_W_TYPE_A,
        5,
        0,
        RM_ATCMD_W_CORE_AWS_ATCMD_CB_P(AWS),
        RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB_P(AWS),
        RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB_P(AWS)
    },
    {
        NULL,
        ATCMD_W_TYPE_MAX,
        0,
        0,
        NULL,
        NULL,
        NULL
    },
};

atcmd_w_ctrl_t * gp_pl_at_ctrl = NULL;
/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

int app_iot_set_feature_parser(int argc, char *argv[])
{
    fsp_err_atcmd_err_code errorCode = FSP_ERR_AT_CMD_ERR_CMD_OK;

    RA6W1_UNUSED_ARG(argc);

    printf("APP SET %s \n", argv[2]);
    if (strcmp(argv[0], "help") == 0)
    {
    }
    else if (strcmp(argv[1], AWS_NVRAM_CFG_DEV_ID) == 0)
    {
        /* thing name */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_FLEET_PROVISIONING_DEVICE_ID, argv[2]);
    }
    else if (strcmp(argv[1], AWS_NVRAM_CFG_FP_TMPL_NAME) == 0)
    {
        /* thing name */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_FLEET_PROVISIONING_TEMPLATE_NAME, argv[2]);
    }
    else if (strcmp(argv[1], AWS_NVRAM_CFG_BROKER_URL) == 0)
    {
        /* Broker URL */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_BROKER_URL, argv[2]);
    }
    else if (strcmp(argv[1], AWS_NVRAM_CFG_PORT) == 0)
    {
        /* local port */
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                      AWSIOT_CFG_PORT, atoi(argv[2]));
    }
    else
    {
        printf("ERROR!! wrong name [%s]\n", argv[1]);
        errorCode = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    return errorCode;
}

/***********************************************************************************************************************
 * AT Functions
 **********************************************************************************************************************/
uint32_t RM_ATCMD_W_CORE_AWS_register(atcmd_w_core_module_list_t * p_list)
{
#if (AT_CORE_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
#endif

    if (p_list->module_cnt > ATCMD_W_LIST_MAX_CNT)
    {
        return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
    }

    if (p_list->unfixed_module_cnt > ATCMD_W_LIST_MAX_CNT)
    {
        return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
    }

    if (rm_atcmd_w_core_register_module_node(p_list, atcmd_w_core_aws_module) == FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_AWS_deregister(atcmd_w_core_module_list_t * p_list)
{
#if (AT_CORE_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
#endif

    rm_atcmd_w_core_deregister(p_list, atcmd_w_core_aws_module);

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_AWS_open(atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    gp_pl_at_ctrl = p_atcmd_w_ctrl;
    return err;
}

uint32_t RM_ATCMD_W_CORE_AWS_close(atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;

    RA6W1_UNUSED_ARG(p_atcmd_w_ctrl);
    gp_pl_at_ctrl = NULL;

    return err;
}

RM_ATCMD_W_CORE_AWS_ATCMD_CB(AWS)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    int i;
    int k = 0;
    char *params[MAX_ARGV_NUM] = { 0, };

    RA6W1_UNUSED_ARG(p_atcmd_w_ctrl);

    printf("\n======================================================= \n");
    printf("argc num = %d \n", argc);
    for (i = 0; i < MAX_ARGV_NUM; i++)
    {
        if (argv[i] == NULL)
            break;

        printf("argv[%d]: %s\n", i, argv[i]);
    }
    printf("\n======================================================= \n");

    if (argc > 2)
    {
        k = 1;
        for (int j = 0; j < i - 1; j++)
        {
            params[j] = argv[k];
            k++;
        }
    }
    else
    {
        params[k] = (char *)strtok(argv[1], " ");

        while (params[k] != NULL)
        {
            k++;
            params[k] = strtok(NULL, " ");
        }
    }

    if (strncmp(argv[1], "SET", 3) == 0)
        return app_iot_set_feature_parser(k, params);
    else
        printf("[APP-IOT] Not support or wrong commmand \n");

    return err;
}

RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(AWS)
{
    const char * p_usage = "<parameter>,[<size>],<value>";
    return p_usage;
}

RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(AWS)
{
    const char * p_descrption = "Set AWS parameters";
    return p_descrption;
}

void add_aws_atcmd(void)
{
    set_user_app_atcmd_open_callback(RM_ATCMD_W_CORE_AWS_open);
    rm_atcmd_w_core_user_command_register(atcmd_w_core_aws_module);
    set_user_app_atcmd_close_callback(RM_ATCMD_W_CORE_AWS_close);
}

#endif /* __SUPPORT_AWS_IOT_W__ */
#endif /* CFG_WIFI */
