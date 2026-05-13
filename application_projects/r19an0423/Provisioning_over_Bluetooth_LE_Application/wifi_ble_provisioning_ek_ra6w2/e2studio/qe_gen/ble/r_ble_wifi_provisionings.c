/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2019-2020 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
 * File Name: r_ble_wifi_provisionings.c
 * Version : 1.0
 * Description : The source file for wifi_provisioning service.
 **********************************************************************************************************************/

#include "r_ble_wifi_provisionings.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

static st_ble_servs_info_t gs_servs_info;

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Command Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifi_command_char_user_description_t(st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Command Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifi_command_char_user_description_t(const st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Command Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wifi_command_char_user_description = {
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifi_provisionings_wifi_command_char_user_description_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_st_ble_wifi_provisionings_wifi_command_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t)encode_st_ble_wifi_provisionings_wifi_command_char_user_description_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiCommandCharUserDescription(const st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifi_command_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiCommandCharUserDescription(st_ble_wifi_provisionings_wifi_command_char_user_description_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifi_command_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Command characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifi_command_t(st_ble_wifi_provisionings_wifi_command_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Command characteristic value decode function. Do not edit comment generated here */
    if (0 != p_gatt_value->value_len)
    {
        memset(&p_app_value->value, 0, sizeof(p_app_value->value));
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);
        p_app_value->value[sizeof(p_app_value->value) - 1] = '\0';
    }
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifi_command_t(const st_ble_wifi_provisionings_wifi_command_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Command characteristic value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFi Command characteristic descriptor definition */
static const st_ble_servs_desc_info_t *gspp_wifi_command_descs[] = { 
    &gs_wifi_command_char_user_description,
};

/* WiFi Command characteristic definition */
static const st_ble_servs_char_info_t gs_wifi_command_char = {
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_IDX,
    .app_size     = sizeof(st_ble_wifi_provisionings_wifi_command_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFI_COMMAND_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifi_provisionings_wifi_command_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifi_provisionings_wifi_command_t,
    .pp_descs     = gspp_wifi_command_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wifi_command_descs),
};

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Status Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
----------------------------------------------------------------------------------------------------------------------*/

static const st_ble_servs_desc_info_t gs_wifi_status_cli_cnfg = {
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CLI_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t)encode_uint16_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatusCliCnfg(uint16_t conn_hdl, const uint16_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifi_status_cli_cnfg, conn_hdl, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatusCliCnfg(uint16_t conn_hdl, uint16_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifi_status_cli_cnfg, conn_hdl, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Status Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifi_status_char_user_description_t(st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Status Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifi_status_char_user_description_t(const st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Status Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wifi_status_char_user_description = {
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifi_provisionings_wifi_status_char_user_description_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_st_ble_wifi_provisionings_wifi_status_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t)encode_st_ble_wifi_provisionings_wifi_status_char_user_description_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatusCharUserDescription(const st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifi_status_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatusCharUserDescription(st_ble_wifi_provisionings_wifi_status_char_user_description_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifi_status_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Status characteristic
----------------------------------------------------------------------------------------------------------------------*/

/* WiFi Status characteristic descriptor definition */
static const st_ble_servs_desc_info_t *gspp_wifi_status_descs[] = { 
    &gs_wifi_status_cli_cnfg, 
    &gs_wifi_status_char_user_description,
};

/* WiFi Status characteristic definition */
static const st_ble_servs_char_info_t gs_wifi_status_char = {
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_IDX,
    .app_size     = sizeof(uint16_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFI_STATUS_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_uint16_t,
    .encode       = (ble_servs_attr_encode_t)encode_uint16_t,
    .pp_descs     = gspp_wifi_status_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wifi_status_descs),
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiStatus(const uint16_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifi_status_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiStatus(uint16_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifi_status_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_NotifyWifiStatus(uint16_t conn_hdl, const uint16_t *p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_wifi_status_char, conn_hdl, (const void *)p_value, true);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Output characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifi_output_t(st_ble_wifi_provisionings_wifi_output_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Output characteristic value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifi_output_t(const st_ble_wifi_provisionings_wifi_output_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Output characteristic value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFi Output characteristic definition */
static const st_ble_servs_char_info_t gs_wifi_output_char = {
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_VAL_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_IDX,
    .app_size     = sizeof(st_ble_wifi_provisionings_wifi_output_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFI_OUTPUT_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifi_provisionings_wifi_output_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifi_provisionings_wifi_output_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiOutput(const st_ble_wifi_provisionings_wifi_output_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifi_output_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiOutput(st_ble_wifi_provisionings_wifi_output_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifi_output_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFi Provisioning characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifi_provisioning_t(st_ble_wifi_provisionings_wifi_provisioning_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Provisioning characteristic value decode function. Do not edit comment generated here */
    if (0 != p_gatt_value->value_len)
    {
        memset(&p_app_value->value, 0, sizeof(p_app_value->value));
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);

        p_app_value->value[sizeof(p_app_value->value) - 1] = '\0';
    }
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifi_provisioning_t(const st_ble_wifi_provisionings_wifi_provisioning_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFi Provisioning characteristic value encode function. Do not edit comment generated here */
    strcpy(p_gatt_value->p_value, p_app_value->value);
    p_gatt_value->value_len = strlen(p_app_value->value);
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFi Provisioning characteristic definition */
static const st_ble_servs_char_info_t gs_wifi_provisioning_char = {
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_VAL_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_IDX,
    .app_size     = sizeof(st_ble_wifi_provisionings_wifi_provisioning_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFI_PROVISIONING_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifi_provisionings_wifi_provisioning_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifi_provisionings_wifi_provisioning_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiProvisioning(const st_ble_wifi_provisionings_wifi_provisioning_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifi_provisioning_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiProvisioning(st_ble_wifi_provisionings_wifi_provisioning_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifi_provisioning_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    wifi_provisioning server
----------------------------------------------------------------------------------------------------------------------*/

/* wifi_provisioning characteristics definition */
static const st_ble_servs_char_info_t *gspp_chars[] = {
    &gs_wifi_command_char,
    &gs_wifi_status_char,
    &gs_wifi_output_char,
    &gs_wifi_provisioning_char,
};

/* wifi_provisioning service definition */
static st_ble_servs_info_t gs_servs_info = {
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_Init(ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}
