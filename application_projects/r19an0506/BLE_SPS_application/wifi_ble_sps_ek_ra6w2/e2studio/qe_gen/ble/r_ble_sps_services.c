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
 * File Name: r_ble_sps_services.c
 * Version : 1.0
 * Description : The source file for SPS Service service.
 **********************************************************************************************************************/

#include "r_ble_sps_services.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

static st_ble_servs_info_t gs_servs_info;

/*----------------------------------------------------------------------------------------------------------------------
 *  Server TX Data Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 * ----------------------------------------------------------------------------------------------------------------------*/

static const st_ble_servs_desc_info_t gs_sps_server_tx_data_cli_cnfg =
{
    .attr_hdl = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_IDX,
    .db_size  = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CLI_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t) encode_uint16_t,
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_server_tx_dataCliCnfg (uint16_t conn_hdl, const uint16_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_sps_server_tx_data_cli_cnfg, conn_hdl, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_server_tx_dataCliCnfg (uint16_t conn_hdl, uint16_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_sps_server_tx_data_cli_cnfg, conn_hdl, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Server TX Data Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_sps_services_sps_server_tx_data_char_user_desc_t (
    st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_app_value,
    const st_ble_gatt_value_t                               * p_gatt_value)
{
    /* Start user code for Server TX Data Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_sps_services_sps_server_tx_data_char_user_desc_t (
    const st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_app_value,
    st_ble_gatt_value_t                                           * p_gatt_value)
{
    /* Start user code for Server TX Data Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_sps_server_tx_data_char_user_desc =
{
    .attr_hdl = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_DESC_HDL,
    .app_size = sizeof(st_ble_sps_services_sps_server_tx_data_char_user_desc_t),
    .desc_idx = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_IDX,
    .db_size  = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_st_ble_sps_services_sps_server_tx_data_char_user_desc_t,
    .encode   = (ble_servs_attr_encode_t) encode_st_ble_sps_services_sps_server_tx_data_char_user_desc_t,
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_server_tx_dataCharUserDesc (
    const st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_sps_server_tx_data_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_server_tx_dataCharUserDesc (
    st_ble_sps_services_sps_server_tx_data_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_sps_server_tx_data_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Server TX Data characteristic : SPS Server TX Data
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_sps_services_sps_server_tx_data_t (
    st_ble_sps_services_sps_server_tx_data_t * p_app_value,
    const st_ble_gatt_value_t                * p_gatt_value)
{
    /* Start user code for Server TX Data characteristic value decode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_sps_services_sps_server_tx_data_t (
    const st_ble_sps_services_sps_server_tx_data_t * p_app_value,
    st_ble_gatt_value_t                            * p_gatt_value)
{
    /* Start user code for Server TX Data characteristic value encode function. Do not edit comment generated here */
    if (0 != p_app_value->tx_len)
    {
        memset(p_gatt_value->p_value, 0x0, p_gatt_value->value_len);
        memcpy(p_gatt_value->p_value, p_app_value->tx_data, p_app_value->tx_len);

        /* Preserve the original size of the data. */
        p_gatt_value->value_len = p_app_value->tx_len;
    }
    else
    {
        p_gatt_value->value_len = 0;
    }

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* Server TX Data characteristic descriptor definition */
static const st_ble_servs_desc_info_t * gspp_sps_server_tx_data_descs[] =
{
    &gs_sps_server_tx_data_cli_cnfg,
    &gs_sps_server_tx_data_char_user_desc,
};

/* Server TX Data characteristic definition */
static const st_ble_servs_char_info_t gs_sps_server_tx_data_char =
{
    .start_hdl    = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_DECL_HDL,
    .end_hdl      = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_CHAR_USER_DESC_DESC_HDL,
    .char_idx     = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_IDX,
    .app_size     = sizeof(st_ble_sps_services_sps_server_tx_data_t),
    .db_size      = BLE_SPS_SERVICES_SPS_SERVER_TX_DATA_LEN,
    .decode       = (ble_servs_attr_decode_t) decode_st_ble_sps_services_sps_server_tx_data_t,
    .encode       = (ble_servs_attr_encode_t) encode_st_ble_sps_services_sps_server_tx_data_t,
    .pp_descs     = gspp_sps_server_tx_data_descs,
    .num_of_descs = ARRAY_SIZE(gspp_sps_server_tx_data_descs),
};

ble_status_t R_BLE_SPS_SERVICES_NotifySps_server_tx_data (uint16_t                                         conn_hdl,
                                                          const st_ble_sps_services_sps_server_tx_data_t * p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_sps_server_tx_data_char, conn_hdl, (const void *) p_value, true);
}

ble_status_t R_BLE_SPS_SERVICES_IndicateSps_server_tx_data (uint16_t                                         conn_hdl,
                                                            const st_ble_sps_services_sps_server_tx_data_t * p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_sps_server_tx_data_char, conn_hdl, (const void *) p_value, false);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Server RX Data Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_sps_services_sps_server_rx_data_char_user_desc_t (
    st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_app_value,
    const st_ble_gatt_value_t                               * p_gatt_value)
{
    /* Start user code for Server RX Data Characteristic User Description descriptor value decode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_sps_services_sps_server_rx_data_char_user_desc_t (
    const st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_app_value,
    st_ble_gatt_value_t                                           * p_gatt_value)
{
    /* Start user code for Server RX Data Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_sps_server_rx_data_char_user_desc =
{
    .attr_hdl = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_DESC_HDL,
    .app_size = sizeof(st_ble_sps_services_sps_server_rx_data_char_user_desc_t),
    .desc_idx = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_IDX,
    .db_size  = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_st_ble_sps_services_sps_server_rx_data_char_user_desc_t,
    .encode   = (ble_servs_attr_encode_t) encode_st_ble_sps_services_sps_server_rx_data_char_user_desc_t,
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_server_rx_dataCharUserDesc (
    const st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_sps_server_rx_data_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_server_rx_dataCharUserDesc (
    st_ble_sps_services_sps_server_rx_data_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_sps_server_rx_data_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Server RX Data characteristic : SPS Server RX Data
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_sps_services_sps_server_rx_data_t (
    st_ble_sps_services_sps_server_rx_data_t * p_app_value,
    const st_ble_gatt_value_t                * p_gatt_value)
{
    /* Start user code for Server RX Data characteristic value decode function. Do not edit comment generated here */

    if (0 != p_gatt_value->value_len)
    {
        memset(p_app_value->rx_data, 0x0, sizeof(p_app_value->rx_data));
        memcpy(p_app_value->rx_data, p_gatt_value->p_value, p_gatt_value->value_len);

        /* Preserve the original size of the data. */
        p_app_value->rx_len = p_gatt_value->value_len;
    }
    else
    {
        p_app_value->rx_len = 0;
    }

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_sps_services_sps_server_rx_data_t (
    const st_ble_sps_services_sps_server_rx_data_t * p_app_value,
    st_ble_gatt_value_t                            * p_gatt_value)
{
    /* Start user code for Server RX Data characteristic value encode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* Server RX Data characteristic descriptor definition */
static const st_ble_servs_desc_info_t * gspp_sps_server_rx_data_descs[] =
{
    &gs_sps_server_rx_data_char_user_desc,
};

/* Server RX Data characteristic definition */
static const st_ble_servs_char_info_t gs_sps_server_rx_data_char =
{
    .start_hdl    = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_DECL_HDL,
    .end_hdl      = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_CHAR_USER_DESC_DESC_HDL,
    .char_idx     = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_IDX,
    .app_size     = sizeof(st_ble_sps_services_sps_server_rx_data_t),
    .db_size      = BLE_SPS_SERVICES_SPS_SERVER_RX_DATA_LEN,
    .decode       = (ble_servs_attr_decode_t) decode_st_ble_sps_services_sps_server_rx_data_t,
    .encode       = (ble_servs_attr_encode_t) encode_st_ble_sps_services_sps_server_rx_data_t,
    .pp_descs     = gspp_sps_server_rx_data_descs,
    .num_of_descs = ARRAY_SIZE(gspp_sps_server_rx_data_descs),
};

/*----------------------------------------------------------------------------------------------------------------------
 *  Flow Control Client Characteristic Configuration descriptor : Client Characteristic Configuration Descriptor
 * ----------------------------------------------------------------------------------------------------------------------*/

static const st_ble_servs_desc_info_t gs_sps_flow_ctrl_cli_cnfg =
{
    .attr_hdl = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_DESC_HDL,
    .app_size = sizeof(uint16_t),
    .desc_idx = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_IDX,
    .db_size  = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CLI_CNFG_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_uint16_t,
    .encode   = (ble_servs_attr_encode_t) encode_uint16_t,
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrlCliCnfg (uint16_t conn_hdl, const uint16_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_sps_flow_ctrl_cli_cnfg, conn_hdl, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrlCliCnfg (uint16_t conn_hdl, uint16_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_sps_flow_ctrl_cli_cnfg, conn_hdl, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Flow Control Characteristic User Description descriptor : Characteristic User Description
 * ----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_sps_services_sps_flow_ctrl_char_user_desc_t (
    st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_app_value,
    const st_ble_gatt_value_t                          * p_gatt_value)
{
    /* Start user code for Flow Control Characteristic User Description descriptor value decode function. Do not edit comment generated here */

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_sps_services_sps_flow_ctrl_char_user_desc_t (
    const st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_app_value,
    st_ble_gatt_value_t                                      * p_gatt_value)
{
    /* Start user code for Flow Control Characteristic User Description descriptor value encode function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_app_value);

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static const st_ble_servs_desc_info_t gs_sps_flow_ctrl_char_user_desc =
{
    .attr_hdl = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_DESC_HDL,
    .app_size = sizeof(st_ble_sps_services_sps_flow_ctrl_char_user_desc_t),
    .desc_idx = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_IDX,
    .db_size  = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_LEN,
    .decode   = (ble_servs_attr_decode_t) decode_st_ble_sps_services_sps_flow_ctrl_char_user_desc_t,
    .encode   = (ble_servs_attr_encode_t) encode_st_ble_sps_services_sps_flow_ctrl_char_user_desc_t,
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrlCharUserDesc (
    const st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_SetDesc(&gs_sps_flow_ctrl_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrlCharUserDesc (
    st_ble_sps_services_sps_flow_ctrl_char_user_desc_t * p_value)
{
    return R_BLE_SERVS_GetDesc(&gs_sps_flow_ctrl_char_user_desc, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Flow Control characteristic : SPS Flow Control
 * ----------------------------------------------------------------------------------------------------------------------*/

/* Flow Control characteristic descriptor definition */
static const st_ble_servs_desc_info_t * gspp_sps_flow_ctrl_descs[] =
{
    &gs_sps_flow_ctrl_cli_cnfg,
    &gs_sps_flow_ctrl_char_user_desc,
};

/* Flow Control characteristic definition */
static const st_ble_servs_char_info_t gs_sps_flow_ctrl_char =
{
    .start_hdl    = BLE_SPS_SERVICES_SPS_FLOW_CTRL_DECL_HDL,
    .end_hdl      = BLE_SPS_SERVICES_SPS_FLOW_CTRL_CHAR_USER_DESC_DESC_HDL,
    .char_idx     = BLE_SPS_SERVICES_SPS_FLOW_CTRL_IDX,
    .app_size     = sizeof(uint8_t),
    .db_size      = BLE_SPS_SERVICES_SPS_FLOW_CTRL_LEN,
    .decode       = (ble_servs_attr_decode_t) decode_uint8_t,
    .encode       = (ble_servs_attr_encode_t) encode_uint8_t,
    .pp_descs     = gspp_sps_flow_ctrl_descs,
    .num_of_descs = ARRAY_SIZE(gspp_sps_flow_ctrl_descs),
};

ble_status_t R_BLE_SPS_SERVICES_SetSps_flow_ctrl (const uint8_t * p_value)
{
    return R_BLE_SERVS_SetChar(&gs_sps_flow_ctrl_char, BLE_GAP_INVALID_CONN_HDL, (const void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_GetSps_flow_ctrl (uint8_t * p_value)
{
    return R_BLE_SERVS_GetChar(&gs_sps_flow_ctrl_char, BLE_GAP_INVALID_CONN_HDL, (void *) p_value);
}

ble_status_t R_BLE_SPS_SERVICES_NotifySps_flow_ctrl (uint16_t conn_hdl, const uint8_t * p_value)
{
    return R_BLE_SERVS_SendHdlVal(&gs_sps_flow_ctrl_char, conn_hdl, (const void *) p_value, true);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  SPS Service server
 * ----------------------------------------------------------------------------------------------------------------------*/

/* SPS Service characteristics definition */
static const st_ble_servs_char_info_t * gspp_chars[] =
{
    &gs_sps_server_tx_data_char,
    &gs_sps_server_rx_data_char,
    &gs_sps_flow_ctrl_char,
};

/* SPS Service service definition */
static st_ble_servs_info_t gs_servs_info =
{
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_SPS_SERVICES_Init (ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}
