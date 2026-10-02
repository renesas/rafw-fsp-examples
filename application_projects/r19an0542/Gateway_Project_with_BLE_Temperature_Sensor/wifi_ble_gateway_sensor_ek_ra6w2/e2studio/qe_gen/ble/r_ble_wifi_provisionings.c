/*
 * Copyright (c) 2019 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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
 *  WiFiCommand Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wificommand_char_user_description_t (
    st_ble_wifi_provisionings_wificommand_char_user_description_t * p_app_value,
    const st_ble_gatt_value_t                                     * p_gatt_value)
{
    /* Start user code for WiFiCommand Characteristic User Description descriptor value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wificommand_char_user_description_t (
    const st_ble_wifi_provisionings_wificommand_char_user_description_t * p_app_value,
    st_ble_gatt_value_t                                                 * p_gatt_value)
{
    /* Start user code for WiFiCommand Characteristic User Description descriptor value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wificommand_char_user_description =
{
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifi_provisionings_wificommand_char_user_description_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_wificommand_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_wificommand_char_user_description_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWificommandCharUserDescription (
    const st_ble_wifi_provisionings_wificommand_char_user_description_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wificommand_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWificommandCharUserDescription (
    st_ble_wifi_provisionings_wificommand_char_user_description_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wificommand_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiCommand characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wificommand_t (
    st_ble_wifi_provisionings_wificommand_t * p_app_value,
    const st_ble_gatt_value_t               * p_gatt_value)
{
    /* Start user code for WiFiCommand characteristic value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wificommand_t (
    const st_ble_wifi_provisionings_wificommand_t * p_app_value,
    st_ble_gatt_value_t                           * p_gatt_value)
{
    /* Start user code for WiFiCommand characteristic value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiCommand characteristic descriptor definition */
static const st_ble_servs_desc_info_t * gspp_wificommand_descs[] =
{
    &gs_wificommand_char_user_description,
};

/* WiFiCommand characteristic definition */
static const st_ble_servs_char_info_t gs_wificommand_char =
{
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_IDX,
    .app_size     = sizeof(st_ble_wifi_provisionings_wificommand_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFICOMMAND_LEN,
    .decode       = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_wificommand_t,
    .encode       = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_wificommand_t,
    .pp_descs     = gspp_wificommand_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wificommand_descs),
};

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiStatus Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 * ----------------------------------------------------------------------------------------------------------------------*/

static const st_ble_servs_desc_info_t gs_wifistatus_cli_cnfg =
{
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CLI_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t) encode_uint16_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatusCliCnfg (uint16_t conn_hdl, const uint16_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifistatus_cli_cnfg, conn_hdl, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatusCliCnfg (uint16_t conn_hdl, uint16_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifistatus_cli_cnfg, conn_hdl, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiStatus Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifistatus_char_user_description_t (
    st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_app_value,
    const st_ble_gatt_value_t                                    * p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifistatus_char_user_description_t (
    const st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_app_value,
    st_ble_gatt_value_t                                                * p_gatt_value)
{
    /* Start user code for WiFiStatus Characteristic User Description descriptor value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_wifistatus_char_user_description =
{
    .attr_hdl = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .app_size = sizeof(st_ble_wifi_provisionings_wifistatus_char_user_description_t),
    .desc_idx = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_IDX,
    .db_size  = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_wifistatus_char_user_description_t,
    .encode   = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_wifistatus_char_user_description_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatusCharUserDescription (
    const st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_wifistatus_char_user_description, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatusCharUserDescription (
    st_ble_wifi_provisionings_wifistatus_char_user_description_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_wifistatus_char_user_description, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiStatus characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

/* WiFiStatus characteristic descriptor definition */
static const st_ble_servs_desc_info_t * gspp_wifistatus_descs[] =
{
    &gs_wifistatus_cli_cnfg,
    &gs_wifistatus_char_user_description,
};

/* WiFiStatus characteristic definition */
static const st_ble_servs_char_info_t gs_wifistatus_char =
{
    .start_hdl    = BLE_WIFI_PROVISIONINGS_WIFISTATUS_DECL_HDL,
    .end_hdl      = BLE_WIFI_PROVISIONINGS_WIFISTATUS_CHAR_USER_DESCRIPTION_DESC_HDL,
    .char_idx     = BLE_WIFI_PROVISIONINGS_WIFISTATUS_IDX,
    .app_size     = sizeof(uint16_t),
    .db_size      = BLE_WIFI_PROVISIONINGS_WIFISTATUS_LEN,
    .decode       = (ble_servs_attr_decode_t) decode_uint16_t,
    .encode       = (ble_servs_attr_encode_t) encode_uint16_t,
    .pp_descs     = gspp_wifistatus_descs,
    .num_of_descs = ARRAY_SIZE(gspp_wifistatus_descs),
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifistatus (const uint16_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifistatus_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifistatus (uint16_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifistatus_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_NotifyWifistatus (uint16_t conn_hdl, const uint16_t * p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_wifistatus_char, conn_hdl, (const void *) p_value, true);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiOutput characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifioutput_t (st_ble_wifi_provisionings_wifioutput_t * p_app_value,
                                                                   const st_ble_gatt_value_t              * p_gatt_value)
{
    /* Start user code for WiFiOutput characteristic value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifioutput_t (
    const st_ble_wifi_provisionings_wifioutput_t * p_app_value,
    st_ble_gatt_value_t                          * p_gatt_value)
{
    /* Start user code for WiFiOutput characteristic value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiOutput characteristic definition */
static const st_ble_servs_char_info_t gs_wifioutput_char =
{
    .start_hdl = BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_DECL_HDL,
    .end_hdl   = BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_VAL_HDL,
    .char_idx  = BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_IDX,
    .app_size  = sizeof(st_ble_wifi_provisionings_wifioutput_t),
    .db_size   = BLE_WIFI_PROVISIONINGS_WIFIOUTPUT_LEN,
    .decode    = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_wifioutput_t,
    .encode    = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_wifioutput_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifioutput (const st_ble_wifi_provisionings_wifioutput_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifioutput_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifioutput (st_ble_wifi_provisionings_wifioutput_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifioutput_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  WiFiProvisioning characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_wifiprovisioning_t (
    st_ble_wifi_provisionings_wifiprovisioning_t * p_app_value,
    const st_ble_gatt_value_t                    * p_gatt_value)
{
    /* Start user code for WiFiProvisioning characteristic value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_wifiprovisioning_t (
    const st_ble_wifi_provisionings_wifiprovisioning_t * p_app_value,
    st_ble_gatt_value_t                                * p_gatt_value)
{
    /* Start user code for WiFiProvisioning characteristic value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* WiFiProvisioning characteristic definition */
static const st_ble_servs_char_info_t gs_wifiprovisioning_char =
{
    .start_hdl = BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_DECL_HDL,
    .end_hdl   = BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_VAL_HDL,
    .char_idx  = BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_IDX,
    .app_size  = sizeof(st_ble_wifi_provisionings_wifiprovisioning_t),
    .db_size   = BLE_WIFI_PROVISIONINGS_WIFIPROVISIONING_LEN,
    .decode    = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_wifiprovisioning_t,
    .encode    = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_wifiprovisioning_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetWifiprovisioning (const st_ble_wifi_provisionings_wifiprovisioning_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_wifiprovisioning_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetWifiprovisioning (st_ble_wifi_provisionings_wifiprovisioning_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_wifiprovisioning_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  ThingName characteristic
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_wifi_provisionings_thingname_t (st_ble_wifi_provisionings_thingname_t * p_app_value,
                                                                  const st_ble_gatt_value_t             * p_gatt_value)
{
    /* Start user code for ThingName characteristic value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_wifi_provisionings_thingname_t (
    const st_ble_wifi_provisionings_thingname_t * p_app_value,
    st_ble_gatt_value_t                         * p_gatt_value)
{
    /* Start user code for ThingName characteristic value encode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* ThingName characteristic definition */
static const st_ble_servs_char_info_t gs_thingname_char =
{
    .start_hdl = BLE_WIFI_PROVISIONINGS_THINGNAME_DECL_HDL,
    .end_hdl   = BLE_WIFI_PROVISIONINGS_THINGNAME_VAL_HDL,
    .char_idx  = BLE_WIFI_PROVISIONINGS_THINGNAME_IDX,
    .app_size  = sizeof(st_ble_wifi_provisionings_thingname_t),
    .db_size   = BLE_WIFI_PROVISIONINGS_THINGNAME_LEN,
    .decode    = (ble_servs_attr_decode_t) decode_st_ble_wifi_provisionings_thingname_t,
    .encode    = (ble_servs_attr_encode_t) encode_st_ble_wifi_provisionings_thingname_t,
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_SetThingname (const st_ble_wifi_provisionings_thingname_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_thingname_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_WIFI_PROVISIONINGS_GetThingname (st_ble_wifi_provisionings_thingname_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_thingname_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  wifi_provisioning server
 * ----------------------------------------------------------------------------------------------------------------------*/

/* wifi_provisioning characteristics definition */
static const st_ble_servs_char_info_t * gspp_chars[] =
{
    &gs_wificommand_char,
    &gs_wifistatus_char,
    &gs_wifioutput_char,
    &gs_wifiprovisioning_char,
    &gs_thingname_char,
};

/* wifi_provisioning service definition */
static st_ble_servs_info_t gs_servs_info =
{
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_WIFI_PROVISIONINGS_Init (ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}
