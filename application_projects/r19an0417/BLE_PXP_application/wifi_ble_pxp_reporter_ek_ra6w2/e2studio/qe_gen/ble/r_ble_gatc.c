/*
 * Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * File Name: r_ble_<built-in method lower of str object at 0x00000256AEB48FB0>.c
 * Version : 1.0
 * Description : The source file for GATT Service client.
 **********************************************************************************************************************/
#include "r_ble_gatc.h"
#include "profile_cmn/r_ble_servc_if.h"

static st_ble_servc_info_t gs_client_info;

/*----------------------------------------------------------------------------------------------------------------------
 *  Service Changed Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 * ----------------------------------------------------------------------------------------------------------------------*/

/* Client Characteristic Configuration characteristic 128 bit UUID */
/* Service Changed characteristic descriptors attribute handles */
static uint16_t gs_serv_chged_cli_cnfg_desc_hdls[BLE_SERVC_MAX_NUM_OF_SAVED];

static const st_ble_servc_desc_info_t gs_serv_chged_cli_cnfg =
{
    .uuid_16     = BLE_GATC_SERV_CHGED_CLI_CNFG_UUID,
    .uuid_type   = BLE_GATT_16_BIT_UUID_FORMAT,
    .app_size    = sizeof(uint16_t),
    .db_size     = BLE_GATC_SERV_CHGED_CLI_CNFG_LEN,
    .desc_idx    = BLE_GATC_SERV_CHGED_CLI_CNFG_IDX,
    .p_attr_hdls = gs_serv_chged_cli_cnfg_desc_hdls,
    .decode      = (ble_servc_attr_decode_t) decode_uint16_t,
    .encode      = (ble_servc_attr_encode_t) encode_uint16_t,
};

ble_status_t R_BLE_GATC_WriteServChgedCliCnfg (uint16_t conn_hdl, const uint16_t * p_value) // @suppress("API function naming")
{
    return R_BLE_SERVC_WriteDesc(&gs_serv_chged_cli_cnfg, conn_hdl, p_value);
}

ble_status_t R_BLE_GATC_ReadServChgedCliCnfg (uint16_t conn_hdl) // @suppress("API function naming")
{
    return R_BLE_SERVC_ReadDesc(&gs_serv_chged_cli_cnfg, conn_hdl);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Service Changed Characteristic : The Service Changed characteristic is a control-point attribute that shall be used to indicate to connected devices that services have changed.
 * ----------------------------------------------------------------------------------------------------------------------*/

/* Service Changed characteristic 128 bit UUID */

/* Service Changed characteristic attribute handles */
static st_ble_gatt_hdl_range_t gs_serv_chged_char_ranges[BLE_SERVC_MAX_NUM_OF_SAVED];

static ble_status_t decode_st_ble_gatc_serv_chged_t (st_ble_gatc_serv_chged_t  * p_app_value,
                                                     const st_ble_gatt_value_t * p_gatt_value)
{
    /* Start user code for Service Changed characteristic value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_gatc_serv_chged_t (const st_ble_gatc_serv_chged_t * p_app_value,
                                                     st_ble_gatt_value_t            * p_gatt_value)
{
    /* Start user code for Service Changed characteristic value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* Service Changed characteristic descriptor definition */
static const st_ble_servc_desc_info_t * gspp_serv_chged_descs[] =
{
    &gs_serv_chged_cli_cnfg,
};

/* Service Changed characteristic definition */
static const st_ble_servc_char_info_t gs_serv_chged_char =
{
    .uuid_16      = BLE_GATC_SERV_CHGED_UUID,
    .uuid_type    = BLE_GATT_16_BIT_UUID_FORMAT,
    .app_size     = sizeof(st_ble_gatc_serv_chged_t),
    .db_size      = BLE_GATC_SERV_CHGED_LEN,
    .char_idx     = BLE_GATC_SERV_CHGED_IDX,
    .p_attr_hdls  = gs_serv_chged_char_ranges,
    .decode       = (ble_servc_attr_decode_t) decode_st_ble_gatc_serv_chged_t,
    .encode       = (ble_servc_attr_encode_t) encode_st_ble_gatc_serv_chged_t,
    .num_of_descs = ARRAY_SIZE(gspp_serv_chged_descs),
    .pp_descs     = gspp_serv_chged_descs,
};

void R_BLE_GATC_GetServChgedAttrHdl (const st_ble_dev_addr_t * p_addr, st_ble_gatc_serv_chged_attr_hdl_t * p_hdl)
{
    uint8_t conn_idx;

    conn_idx = R_BLE_SERVC_GetConnIdx(p_addr);

    p_hdl->range             = gs_serv_chged_char_ranges[conn_idx];
    p_hdl->cli_cnfg_desc_hdl = gs_serv_chged_cli_cnfg_desc_hdls[conn_idx];
}

/*----------------------------------------------------------------------------------------------------------------------
 *  GATT Service client
 * ----------------------------------------------------------------------------------------------------------------------*/

/* GATT Service client attribute handles */
static st_ble_gatt_hdl_range_t gs_gatc_ranges[BLE_SERVC_MAX_NUM_OF_SAVED];

const st_ble_servc_char_info_t * gspp_gatc_chars[] =
{
    &gs_serv_chged_char,
};

static st_ble_servc_info_t gs_client_info =
{
    .pp_chars     = gspp_gatc_chars,
    .num_of_chars = ARRAY_SIZE(gspp_gatc_chars),
    .p_attr_hdls  = gs_gatc_ranges,
};

ble_status_t R_BLE_GATC_Init (ble_servc_app_cb_t cb) // @suppress("API function naming")
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_client_info.cb = cb;

    return R_BLE_SERVC_RegisterClient(&gs_client_info);
}

void R_BLE_GATC_ServDiscCb (uint16_t conn_hdl, uint8_t serv_idx, uint16_t type, void * p_param) // @suppress("API function naming")
{
    R_BLE_SERVC_ServDiscCb(&gs_client_info, conn_hdl, serv_idx, type, p_param);
}

void R_BLE_GATC_GetServAttrHdl (const st_ble_dev_addr_t * p_addr, st_ble_gatt_hdl_range_t * p_hdl)
{
    uint8_t conn_idx;

    conn_idx = R_BLE_SERVC_GetConnIdx(p_addr);

    *p_hdl = gs_gatc_ranges[conn_idx];
}
