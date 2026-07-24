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
 * File Name: r_ble_WiFiProvisionings.c
 * Version : 1.0
 * Description : The source file for WiFiProvisioning service.
 **********************************************************************************************************************/

#include "r_ble_WiFiProvisionings.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

static st_ble_servs_info_t gs_servs_info;

/*----------------------------------------------------------------------------------------------------------------------
    WiFiCommand Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifiprovisionings_wificommand_char_user_description_t(st_ble_wifiprovisionings_wificommand_char_user_description_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiCommand Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifiprovisionings_wificommand_char_user_description_t(const st_ble_wifiprovisionings_wificommand_char_user_description_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiCommand Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wificommand_char_user_description = {
    .attr_hdl = BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifiprovisionings_wificommand_char_user_description_t),
    .desc_idx = BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_st_ble_wifiprovisionings_wificommand_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t)encode_st_ble_wifiprovisionings_wificommand_char_user_description_t,
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWificommandCharUserDescription(const st_ble_wifiprovisionings_wificommand_char_user_description_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wificommand_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWificommandCharUserDescription(st_ble_wifiprovisionings_wificommand_char_user_description_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wificommand_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFiCommand characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifiprovisionings_wificommand_t(st_ble_wifiprovisionings_wificommand_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiCommand characteristic value decode function. Do not edit comment generated here */
    if (p_gatt_value->value_len != 0) {
        memset(&p_app_value->value, 0, sizeof(p_app_value->value));
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);

        p_app_value->value[sizeof(p_app_value->value) - 1] = '\0';
    }
    /* End user code. Do not edit comment generated here */

    return BLE_SUCCESS;
}
static ble_status_t encode_st_ble_wifiprovisionings_wificommand_t(const st_ble_wifiprovisionings_wificommand_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiCommand characteristic value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiCommand characteristic descriptor definition */
static const st_ble_servs_desc_info_t *gspp_wificommand_descs[] = {
    &gs_wificommand_char_user_description,
};

/* WiFiCommand characteristic definition */
static const st_ble_servs_char_info_t gs_wificommand_char = {
    .start_hdl    = BLE_WIFIPROVISIONINGS_WIFICOMMAND_DECL_HDL,
    .end_hdl      = BLE_WIFIPROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFIPROVISIONINGS_WIFICOMMAND_IDX,
    .app_size     = sizeof(st_ble_wifiprovisionings_wificommand_t),
    .db_size      = BLE_WIFIPROVISIONINGS_WIFICOMMAND_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifiprovisionings_wificommand_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifiprovisionings_wificommand_t,
    .pp_descs     = gspp_wificommand_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wificommand_descs),
};

/*----------------------------------------------------------------------------------------------------------------------
    WiFiStatus Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
----------------------------------------------------------------------------------------------------------------------*/

static const st_ble_servs_desc_info_t gs_wifistatus_cli_cnfg = {
    .attr_hdl = BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
    .db_size  = BLE_WIFIPROVISIONINGS_WIFISTATUS_CLI_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t)encode_uint16_t,
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatusCliCnfg(uint16_t conn_hdl, const uint16_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifistatus_cli_cnfg, conn_hdl, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatusCliCnfg(uint16_t conn_hdl, uint16_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifistatus_cli_cnfg, conn_hdl, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFiStatus Characteristic User Description descriptor : Characteristic User Description
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifiprovisionings_wifistatus_char_user_description_t(st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifiprovisionings_wifistatus_char_user_description_t(const st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wifistatus_char_user_description = {
    .attr_hdl = BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifiprovisionings_wifistatus_char_user_description_t),
    .desc_idx = BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_st_ble_wifiprovisionings_wifistatus_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t)encode_st_ble_wifiprovisionings_wifistatus_char_user_description_t,
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatusCharUserDescription(const st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifistatus_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatusCharUserDescription(st_ble_wifiprovisionings_wifistatus_char_user_description_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifistatus_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFiStatus characteristic
----------------------------------------------------------------------------------------------------------------------*/

/* WiFiStatus characteristic descriptor definition */
static const st_ble_servs_desc_info_t *gspp_wifistatus_descs[] = {
    &gs_wifistatus_cli_cnfg,
    &gs_wifistatus_char_user_description,
};

/* WiFiStatus characteristic definition */
static const st_ble_servs_char_info_t gs_wifistatus_char = {
    .start_hdl    = BLE_WIFIPROVISIONINGS_WIFISTATUS_DECL_HDL,
    .end_hdl      = BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFIPROVISIONINGS_WIFISTATUS_IDX,
    .app_size     = sizeof(uint16_t),
    .db_size      = BLE_WIFIPROVISIONINGS_WIFISTATUS_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_uint16_t,
    .encode       = (ble_servs_attr_encode_t)encode_uint16_t,
    .pp_descs     = gspp_wifistatus_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wifistatus_descs),
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifistatus(const uint16_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifistatus_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifistatus(uint16_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifistatus_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_NotifyWifistatus(uint16_t conn_hdl, const uint16_t *p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_wifistatus_char, conn_hdl, (const void *)p_value, true);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFiOutput characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifiprovisionings_wifioutput_t(st_ble_wifiprovisionings_wifioutput_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiOutput characteristic value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifiprovisionings_wifioutput_t(
    const st_ble_wifiprovisionings_wifioutput_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    return BLE_SUCCESS;
}

/* WiFiOutput characteristic definition */
static const st_ble_servs_char_info_t gs_wifioutput_char = {
    .start_hdl    = BLE_WIFIPROVISIONINGS_WIFIOUTPUT_DECL_HDL,
    .end_hdl      = BLE_WIFIPROVISIONINGS_WIFIOUTPUT_VAL_HDL,
    .char_idx     = BLE_WIFIPROVISIONINGS_WIFIOUTPUT_IDX,
    .app_size     = sizeof(st_ble_wifiprovisionings_wifioutput_t),
    .db_size      = BLE_WIFIPROVISIONINGS_WIFIOUTPUT_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifiprovisionings_wifioutput_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifiprovisionings_wifioutput_t,
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifioutput(const st_ble_wifiprovisionings_wifioutput_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifioutput_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifioutput(st_ble_wifiprovisionings_wifioutput_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifioutput_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    WiFiProvisoning characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifiprovisionings_wifiprovisoning_t(st_ble_wifiprovisionings_wifiprovisoning_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiProvisoning characteristic value decode function. Do not edit comment generated here */
    if (p_gatt_value->value_len != 0) {
        memset(&p_app_value->value, 0, sizeof(p_app_value->value));
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);

        p_app_value->value[sizeof(p_app_value->value) - 1] = '\0';
    }
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifiprovisionings_wifiprovisoning_t(const st_ble_wifiprovisionings_wifiprovisoning_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiProvisoning characteristic value encode function. Do not edit comment generated here */
    strcpy(p_gatt_value->p_value, p_app_value->value);
    p_gatt_value->value_len = strlen(p_app_value->value);
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiProvisoning characteristic definition */
static const st_ble_servs_char_info_t gs_wifiprovisoning_char = {
    .start_hdl    = BLE_WIFIPROVISIONINGS_WIFIPROVISONING_DECL_HDL,
    .end_hdl      = BLE_WIFIPROVISIONINGS_WIFIPROVISONING_VAL_HDL,
    .char_idx     = BLE_WIFIPROVISIONINGS_WIFIPROVISONING_IDX,
    .app_size     = sizeof(st_ble_wifiprovisionings_wifiprovisoning_t),
    .db_size      = BLE_WIFIPROVISIONINGS_WIFIPROVISONING_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_wifiprovisionings_wifiprovisoning_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_wifiprovisionings_wifiprovisoning_t,
};

ble_status_t R_BLE_WIFIPROVISIONINGS_SetWifiprovisoning(const st_ble_wifiprovisionings_wifiprovisoning_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifiprovisoning_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_WIFIPROVISIONINGS_GetWifiprovisoning(st_ble_wifiprovisionings_wifiprovisoning_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifiprovisoning_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

#if defined (__SUPPORT_MATTER_IOT__)
/*----------------------------------------------------------------------------------------------------------------------
    Matter RX characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_matter_rx_t(st_ble_matter_rx_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Matter RX characteristic value decode function. Do not edit comment generated here */
    if (p_gatt_value->value_len != 0) {
        memset(&p_app_value->value, 0, sizeof(p_app_value->value));
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);
        p_app_value->len = p_gatt_value->value_len;
        p_app_value->value[sizeof(p_app_value->value) - 1] = '\0';
    }
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_matter_rx_t(const st_ble_matter_rx_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Matter RX characteristic value encode function. Do not edit comment generated here */
    strcpy(p_gatt_value->p_value, p_app_value->value);
    p_gatt_value->value_len = p_app_value->len;
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiProvisoning characteristic definition */
static const st_ble_servs_char_info_t gs_matter_rx_char = {
    .start_hdl    = BLE_MATTER_RX_DECL_HDL,
    .end_hdl      = BLE_MATTER_RX_VALUE_HDL,
    .char_idx     = BLE_MATTER_RX_IDX,
    .app_size     = sizeof(st_ble_matter_rx_t),
    .db_size      = BLE_MATTER_RX_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_matter_rx_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_matter_rx_t,
};

ble_status_t R_BLE_MATTER_SetRX(const st_ble_matter_rx_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_matter_rx_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_MATTER_GetRX(st_ble_matter_rx_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_matter_rx_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    Matter TX Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_matter_char_user_description_t(st_ble_matter_tx_cfg_user_description_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_matter_char_user_description_t(const st_ble_matter_tx_cfg_user_description_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_matter_char_user_description = {
    .attr_hdl = BLE_MATTER_TX_CNFG_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_matter_tx_cfg_user_description_t),
    .desc_idx = BLE_MATTER_TX_CNFG_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFIPROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_st_ble_matter_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t)encode_st_ble_matter_char_user_description_t,
};

ble_status_t R_BLE_MATTER_SET_CharUserDescription(const st_ble_matter_tx_cfg_user_description_t *p_value)
{
    printf("R_BLE_MATTER_CharUserDescription\n");
    return R_BLE_SERVS_SetDesc(&gs_matter_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_MATTER_GET_CharUserDescription(st_ble_matter_tx_cfg_user_description_t *p_value)
{
    printf("R_BLE_MATTER_CharUserDescription\n");
    return R_BLE_SERVS_GetDesc(&gs_matter_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

static const st_ble_servs_desc_info_t gs_matter_tx_cnfg = {
    .attr_hdl = BLE_MATTER_TX_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_MATTER_TX_CNFG_IDX,
    .db_size  = BLE_MATTER_TX_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t)decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t)encode_uint16_t,
};

ble_status_t R_BLE_Matter_SetTXCnfg(uint16_t conn_hdl, const uint16_t *p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_matter_tx_cnfg, conn_hdl, (const void *)p_value);
}

ble_status_t R_BLE_Matter_GetTXCnfg(uint16_t conn_hdl, uint16_t *p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_matter_tx_cnfg, conn_hdl, (void *)p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
    Matter TX characteristic
----------------------------------------------------------------------------------------------------------------------*/

/* WiFiStatus characteristic descriptor definition */
static const st_ble_servs_desc_info_t *gspp_matter_tx_descs[] = {
    &gs_matter_tx_cnfg,
    &gs_matter_char_user_description,
};

static ble_status_t decode_st_ble_matter_tx_t(st_ble_matter_tx_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Matter TX characteristic value decode function. Do not edit comment generated here */
    if (p_gatt_value->value_len != 0) {
        memcpy(&p_app_value->value, p_gatt_value->p_value, p_gatt_value->value_len);
        p_app_value->len = p_gatt_value->value_len;
        p_app_value->value[p_app_value->len] = '\0';
    }
    /* End user code. Do not edit comment generated here */
    // printf("decode_st_ble_matter_tx_t p_app_value->len %d\n", p_app_value->len);
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_matter_tx_t(const st_ble_matter_tx_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Matter TX characteristic value encode function. Do not edit comment generated here */
    memcpy(p_gatt_value->p_value, p_app_value->value, p_app_value->len);
    p_gatt_value->value_len = p_app_value->len;
    /* End user code. Do not edit comment generated here */
    // printf("encode_st_ble_matter_tx_t p_app_value->len %d\n", p_app_value->len);
    return BLE_SUCCESS;
}

/* WiFiStatus characteristic definition */
static const st_ble_servs_char_info_t gs_matter_tx_char = {
    .start_hdl    = BLE_MATTER_TX_DECL_HDL,
    .end_hdl      = BLE_MATTER_TX_CNFG_DESC_HDL,
    .char_idx     = BLE_MATTER_TX_IDX,
    .app_size     = sizeof(st_ble_matter_tx_t),
    .db_size      = BLE_MATTER_TX_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_matter_tx_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_matter_tx_t,
    .pp_descs     = gspp_matter_tx_descs,
    .num_of_descs = ARRAY_SIZE(gspp_matter_tx_descs),
};

ble_status_t R_BLE_MATTER_SetTX(const uint16_t *p_value)
{
    return R_BLE_SERVS_SetChar(&gs_matter_tx_char, BLE_GAP_INVALID_CONN_HDL, (const void *)p_value);
}

ble_status_t R_BLE_MATTER_GetTX(uint16_t *p_value)
{
    return R_BLE_SERVS_GetChar(&gs_matter_tx_char, BLE_GAP_INVALID_CONN_HDL, (void *)p_value);
}

ble_status_t R_BLE_MATTER_TX_Indicate(uint16_t conn_hdl, const void *p_value, uint16_t p_value_len)
{
    st_ble_matter_tx_t matter_value = {
      .len = p_value_len,
    };

    memcpy(matter_value.value, p_value, p_value_len);
    matter_value.value[p_value_len] = 0;

	return R_BLE_SERVS_SendHdlVal(&gs_matter_tx_char, conn_hdl, (const void *)&matter_value, false);
}
#endif	// __SUPPORT_MATTER_IOT__
/*----------------------------------------------------------------------------------------------------------------------
    WiFiProvisioning server
----------------------------------------------------------------------------------------------------------------------*/

/* WiFiProvisioning characteristics definition */
static const st_ble_servs_char_info_t *gspp_chars[] = {
    &gs_wificommand_char,
    &gs_wifistatus_char,
    &gs_wifioutput_char,
    &gs_wifiprovisoning_char,
#if defined (__SUPPORT_MATTER_IOT__)
	&gs_matter_rx_char,
	&gs_matter_tx_char,
#endif	// __SUPPORT_MATTER_IOT__
};

/* WiFiProvisioning service definition */
static st_ble_servs_info_t gs_servs_info = {
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_WIFIPROVISIONINGS_Init(ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}

