/***********************************************************************************************************************

* File Name    : atcmd_red.c

* Description  : AT command Red functions

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/


/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "rm_atcmd_w_cfg.h"

#include <stdlib.h>
#include "atcmd_red.h"
#include "rm_atcmd_w_api.h"
#include "rm_atcmd_w_core.h"
#include "secure_asset/secure_storage.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define RM_ATCMD_BASIC_CODE(atcmd)      # atcmd
#define RM_ATCMD_RED_CODE(atcmd)         "AT+" # atcmd

#define RM_ATCMD_RED_CB(atcmd) \
    uint32_t RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _cmd_cb(atcmd_w_ctrl_t *const p_at_ctrl, int argc, const char *argv[])
#define RM_ATCMD_RED_FORMAT_CB(atcmd)  \
    const char *RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _format_cb(void)
#define RM_ATCMD_RED_BRIEF_CB(atcmd)   \
    const char *RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _brief_cb(void)

#define RM_ATCMD_RED_CB_P(atcmd)         RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _cmd_cb
#define RM_ATCMD_RED_FORMAT_CB_P(atcmd)  RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _format_cb
#define RM_ATCMD_RED_BRIEF_CB_P(atcmd)   RM_ATCMD_W_CORE_SECURE_STORAGE_ ## atcmd ## _brief_cb

#define RM_ATCMD_W_CORE_SECURE_STORAGE_DEBUG(fmt, ...)
#define RM_ATCMD_W_CORE_SECURE_STORAGE_ERROR(fmt, ...)

extern void red_ota_get_param(uint8_t *mode, uint32_t *expire, char *url);
extern fsp_err_t red_ota_set_param(uint8_t mode, uint32_t expire, char *url);

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
RM_ATCMD_RED_CB(USERASSETSTORE);
RM_ATCMD_RED_FORMAT_CB(USERASSETSTORE);
RM_ATCMD_RED_BRIEF_CB(USERASSETSTORE);

RM_ATCMD_RED_CB(USERASSETLOAD);
RM_ATCMD_RED_FORMAT_CB(USERASSETLOAD);
RM_ATCMD_RED_BRIEF_CB(USERASSETLOAD);

RM_ATCMD_RED_CB(PRODASSETSTORE);
RM_ATCMD_RED_FORMAT_CB(PRODASSETSTORE);
RM_ATCMD_RED_BRIEF_CB(PRODASSETSTORE);

RM_ATCMD_RED_CB(PRODASSETLOAD);
RM_ATCMD_RED_FORMAT_CB(PRODASSETLOAD);
RM_ATCMD_RED_BRIEF_CB(PRODASSETLOAD);

/* User AT commands are low priority, so ATF cannot be overridden. Therefore, this command is used instead of ATF for RED. */
RM_ATCMD_RED_CB(REDF);
RM_ATCMD_RED_FORMAT_CB(REDF);
RM_ATCMD_RED_BRIEF_CB(REDF);

RM_ATCMD_RED_CB(OTAMODE);
RM_ATCMD_RED_FORMAT_CB(OTAMODE);
RM_ATCMD_RED_BRIEF_CB(OTAMODE);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
const atcmd_w_core_module_t at_red_module[] =
{
     {
        RM_ATCMD_RED_CODE(USERASSETSTORE),
        ATCMD_W_TYPE_A,
        4,  // ssid, password, key
        0,
        RM_ATCMD_RED_CB_P(USERASSETSTORE),
        RM_ATCMD_RED_FORMAT_CB_P(USERASSETSTORE),
        RM_ATCMD_RED_BRIEF_CB_P(USERASSETSTORE)
    },
    {
       RM_ATCMD_RED_CODE(USERASSETLOAD),
        ATCMD_W_TYPE_A,
        0,  // No arguments needed
        0,
        RM_ATCMD_RED_CB_P(USERASSETLOAD),
        RM_ATCMD_RED_FORMAT_CB_P(USERASSETLOAD),
        RM_ATCMD_RED_BRIEF_CB_P(USERASSETLOAD)
    },
     {
        RM_ATCMD_RED_CODE(PRODASSETSTORE),
        ATCMD_W_TYPE_A,
        5,  // ssid, password, key, type
        0,
        RM_ATCMD_RED_CB_P(PRODASSETSTORE),
        RM_ATCMD_RED_FORMAT_CB_P(PRODASSETSTORE),
        RM_ATCMD_RED_BRIEF_CB_P(PRODASSETSTORE)
    },
    {
        RM_ATCMD_RED_CODE(PRODASSETLOAD),
        ATCMD_W_TYPE_A,
        2,  // type
        0,
        RM_ATCMD_RED_CB_P(PRODASSETLOAD),
        RM_ATCMD_RED_FORMAT_CB_P(PRODASSETLOAD),
        RM_ATCMD_RED_BRIEF_CB_P(PRODASSETLOAD)
    },
    {
        RM_ATCMD_RED_CODE(REDF),
        ATCMD_W_TYPE_A,
        0,
        0,
        RM_ATCMD_RED_CB_P(REDF),
        RM_ATCMD_RED_FORMAT_CB_P(REDF),
        RM_ATCMD_RED_BRIEF_CB_P(REDF),
    },
    {
        RM_ATCMD_RED_CODE(OTAMODE),
        ATCMD_W_TYPE_A,
        3,
        0,
        RM_ATCMD_RED_CB_P(OTAMODE),
        RM_ATCMD_RED_FORMAT_CB_P(OTAMODE),
        RM_ATCMD_RED_BRIEF_CB_P(OTAMODE)
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

atcmd_w_ctrl_t *gp_pl_at_ctrl = NULL;
atcmd_provision_stat Prov_LastStat = ATCMD_PROVISION_IDLE;

RM_ATCMD_RED_CB(USERASSETSTORE)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);

    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    ap_info_t ap_info;
    key_info_t key_info;
    char result_str[64] = {
        0,
    };

    /* Check arguments */
    if (argc != 4)
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    /* Check argument lengths */
    if ((strlen(argv[1]) >= sizeof(ap_info.ssid)) || (strlen(argv[2]) >= sizeof(ap_info.password)) ||
        (strlen(argv[3]) >= sizeof(key_info.key)))
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    memset(&ap_info, 0, sizeof(ap_info_t));
    memset(&key_info, 0, sizeof(key_info_t));

    memcpy(ap_info.ssid, argv[1], strlen(argv[1]));
    memcpy(ap_info.password, argv[2], strlen(argv[2]));
    memcpy(key_info.key, argv[3], strlen(argv[3]));

    if (R_RED_SecureAssetStore(SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)) != FSP_SUCCESS ||
        R_RED_SecureAssetStore(SECURE_ASSET_AT_KEY, (uint8_t *) &key_info, sizeof(key_info_t)) != FSP_SUCCESS)
    {
        err = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        goto end;
    }

end:
    sprintf(result_str, "\r\n+USERASSETSTORE:%d\r\n", err);
    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t *) result_str, strlen(result_str));
    return err;
}

RM_ATCMD_RED_CB(USERASSETLOAD)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);
    FSP_PARAMETER_NOT_USED(argv);

    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    ap_info_t ap_info;
    key_info_t key_info;
    char *result_str;
    bool status = true;

    result_str = malloc(256);
    if (result_str == NULL)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    /* Check arguments */
    if (argc != 1)
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    memset(&ap_info, 0, sizeof(ap_info_t));
    memset(&key_info, 0, sizeof(key_info_t));

    if (R_RED_SecureAssetLoad(SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)) != FSP_SUCCESS)
    {
        err    = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        status = false;
    }

    if (R_RED_SecureAssetLoad(SECURE_ASSET_AT_KEY, (uint8_t *) &key_info, sizeof(key_info_t)) != FSP_SUCCESS)
    {
        err    = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        status = false;
    }

    if (status)
    {
        sprintf(result_str,
                "\r\n+USERASSETLOAD:SSID=%s,PWD=%s,KEY=%s\r\n",
                ap_info.ssid,
                ap_info.password,
                key_info.key);
    }
    else
    {
        sprintf(result_str, "\r\n+USERASSETLOAD:%d\r\n", err);
    }

end:
    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t *) result_str, strlen(result_str));
    free(result_str);
    return err;
}

RM_ATCMD_RED_FORMAT_CB(USERASSETSTORE)
{
    return "<ssid>,<password>,<key>";
}

RM_ATCMD_RED_BRIEF_CB(USERASSETSTORE)
{
    return "Store AP info and key to secure storage for user";
}

RM_ATCMD_RED_FORMAT_CB(USERASSETLOAD)
{
    return "";  // No arguments needed
}

RM_ATCMD_RED_BRIEF_CB(USERASSETLOAD)
{
    return "Load AP info and key from secure storag for user";
}

RM_ATCMD_RED_CB(PRODASSETSTORE)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    ap_info_t ap_info;
    key_info_t key_info;
    uint32_t type = SECURE_ASSET_FACTORY_STORAGE_OTP;
    char result_str[64] = {0, };

    /* Check arguments */
    if (argc == 5)
    {
        if (rm_atcmd_w_core_common_htoi_custom(argv[4]) == 0)
        {
            type = SECURE_ASSET_FACTORY_STORAGE_OTP;
        }
        else if (rm_atcmd_w_core_common_htoi_custom(argv[4]) == 1)
        {
            type = SECURE_ASSET_FACTORY_STORAGE_FLASH;
        }
        else
        {
            type = SECURE_ASSET_FACTORY_STORAGE_OTP;
        }
    }
    else if (argc != 4)
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }
    printf("str leng: %zu, %zu, %zu\n", strlen(argv[1]), strlen(argv[2]), strlen(argv[3]));

    /* Check argument lengths */
    if ((strlen(argv[1]) > sizeof(ap_info.ssid)) || (strlen(argv[2]) > sizeof(ap_info.password)) ||
        (strlen(argv[3]) > sizeof(key_info.key)))
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    memset(&ap_info, 0, sizeof(ap_info_t));
    memset(&key_info, 0, sizeof(key_info_t));

    memcpy(ap_info.ssid, argv[1], strlen(argv[1]));
    memcpy(ap_info.password, argv[2], strlen(argv[2]));
    memcpy(key_info.key, argv[3], strlen(argv[3]));

    if (R_RED_SecureAssetProdStore(type,
                                   SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)) != FSP_SUCCESS ||
        R_RED_SecureAssetProdStore(type,
                                   SECURE_ASSET_AT_KEY, (uint8_t *) &key_info, sizeof(key_info_t)) != FSP_SUCCESS)
    {
        err = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        goto end;
    }

end:
    sprintf(result_str, "\r\n+PRODASSETSTORE:%d\r\n", err);
    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t *) result_str, strlen(result_str));
    return err;
}

RM_ATCMD_RED_CB(PRODASSETLOAD)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    ap_info_t ap_info;
    key_info_t key_info;
    uint32_t type = SECURE_ASSET_FACTORY_STORAGE_OTP;
    char *result_str;
    bool status = true;

    result_str = malloc(256);
    if (result_str == NULL)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    /* Check arguments */
    if (argc == 2)
    {
        if (rm_atcmd_w_core_common_htoi_custom(argv[1]) == 0)
        {
            type = SECURE_ASSET_FACTORY_STORAGE_OTP;
        }
        else if (rm_atcmd_w_core_common_htoi_custom(argv[1]) == 1)
        {
            type = SECURE_ASSET_FACTORY_STORAGE_FLASH;
        }
        else
        {
            type = SECURE_ASSET_FACTORY_STORAGE_OTP;
        }
    }
    else if (argc != 1)
    {
        err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        goto end;
    }

    memset(&ap_info, 0, sizeof(ap_info_t));
    memset(&key_info, 0, sizeof(key_info_t));

    if (R_RED_SecureAssetProdLoad(type,
                                  SECURE_ASSET_APP_INFO, (uint8_t *) &ap_info, sizeof(ap_info_t)) != FSP_SUCCESS)
    {
        err    = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        status = false;
    }

    if (R_RED_SecureAssetProdLoad(type,
                                  SECURE_ASSET_AT_KEY, (uint8_t *) &key_info, sizeof(key_info_t)) != FSP_SUCCESS)
    {
        err    = FSP_ERR_AT_CMD_ERR_PERI_XXX;
        status = false;
    }

    if (status)
    {
        sprintf(result_str,
                "\r\n+PRODASSETLOAD:SSID=%s,PWD=%s,KEY=%s\r\n",
                ap_info.ssid,
                ap_info.password,
                key_info.key);
    }
    else
    {
        sprintf(result_str, "\r\n+PRODASSETLOAD:%d\r\n", err);
    }

end:
    RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t *) result_str, strlen(result_str));
    free(result_str);
    return err;
}

RM_ATCMD_RED_FORMAT_CB(PRODASSETSTORE)
{
    return "<ssid>,<password>,<key>[,<type>]";
}

RM_ATCMD_RED_BRIEF_CB(PRODASSETSTORE)
{
    return "Store AP info and key to secure storage for PLT";
}

RM_ATCMD_RED_FORMAT_CB(PRODASSETLOAD)
{
    return "type";  // No arguments needed
}

RM_ATCMD_RED_BRIEF_CB(PRODASSETLOAD)
{
    return "Load AP info and key from secure storage for PLT";
}

RM_ATCMD_RED_CB(REDF)
{
	FSP_PARAMETER_NOT_USED(p_at_ctrl);
	FSP_PARAMETER_NOT_USED(argc);
	FSP_PARAMETER_NOT_USED(argv);

    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    printf("user factory reset\n");
    R_RED_SecureAssetDelete();
    factory_reset(1);
    return err;
}

RM_ATCMD_RED_FORMAT_CB(REDF)
{
    const char *p_usage = "";
    return p_usage;
}

RM_ATCMD_RED_BRIEF_CB(REDF)
{
    const char *p_description = "Restore to Factory mode (NVRAM clean)";
    return p_description;
}

RM_ATCMD_RED_CB(OTAMODE)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    uint8_t mode = 0;
    uint32_t period = 0;
    char url[256];

    red_ota_get_param(&mode, &period, url);

    if (argc == 1)
    {
        char result_str[256] = {0, };
        int result_len = sprintf(result_str, "%s:%u, %u, %s\r\n",
                             rm_atcmd_w_core_common_strupr(argv[0] + 2),
                             mode, period, url);
        RM_ATCMD_W_CORE_Write(p_at_ctrl, (uint8_t *) result_str, result_len);
    }
    else if (argc == 2)
    {
        mode = atoi(argv[1]);
        red_ota_set_param(mode, period, url);
    }
    else if (argc == 3)
    {
        mode = atoi(argv[1]);
        period = atoi(argv[2]);
        red_ota_set_param(mode, period, url);
    }
    else if (argc == 4)
    {
        mode = atoi(argv[1]);
        period = atoi(argv[2]);
        strcpy(url, argv[3]);
        red_ota_set_param(mode, period, url);
    } 
    else
    {
        err = FSP_ERR_AT_CMD_ERR_UNKNOWN;
    }
    return err;
}

RM_ATCMD_RED_FORMAT_CB(OTAMODE)
{
    const char *p_usage = "<mode>,<period_sec>,<host url>";
    return p_usage;
}

RM_ATCMD_RED_BRIEF_CB(OTAMODE)
{
    const char *p_descrption = "Set OTA Manual or Auto Mode";
    return p_descrption;
}

uint32_t RM_ATCMD_W_CORE_RED_open(atcmd_w_ctrl_t *const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    gp_pl_at_ctrl = p_atcmd_w_ctrl;
    return err;
}

uint32_t RM_ATCMD_W_CORE_RED_close(atcmd_w_ctrl_t *const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    gp_pl_at_ctrl = NULL;
    return err;
}

void RM_PL_PRINTF_ATCMD(char *p_str)
{
    if (gp_pl_at_ctrl == NULL)
    {
        printf("gp_pl_at_ctrl Failed in RM_PL_PRINTF_ATCMD\n");
        return;
    }
    RM_ATCMD_W_CORE_Write(gp_pl_at_ctrl, (uint8_t *)p_str, strlen(p_str));
    return;
}

void atcmd_provstat(int status)
{
    char tBuf[128] = {0, };

    Prov_LastStat = status;

    sprintf(tBuf,"+ATPROV=STATUS %d\n",status);
    RM_PL_PRINTF_ATCMD(tBuf);
    printf(tBuf);
}

void add_red_atcmd(void)
{
    set_user_app_atcmd_open_callback(RM_ATCMD_W_CORE_RED_open);
    rm_atcmd_w_core_user_command_register(at_red_module);
    set_user_app_atcmd_close_callback(RM_ATCMD_W_CORE_RED_close);
}
