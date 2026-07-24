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
 * File Name: r_ble_mlss.c
 * Version : 1.0
 * Description : The source file for Multi-Link service.
 **********************************************************************************************************************/

#include "r_ble_mlss.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "gatt_db.h"

static st_ble_servs_info_t gs_servs_info;

/*----------------------------------------------------------------------------------------------------------------------
    Peripheral Address characteristic
----------------------------------------------------------------------------------------------------------------------*/

static ble_status_t decode_st_ble_mlss_periph_addr_t(st_ble_mlss_periph_addr_t *p_app_value, const st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Peripheral Address characteristic value decode function. Do not edit comment generated here */

    uint32_t pos = 0;

    for (uint32_t i = 0; i < 7; i++)
    {
        BT_UNPACK_LE_1_BYTE(&p_app_value->periph_addr[i], &p_gatt_value->p_value[pos]);
        pos += 1;
    }

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

static ble_status_t encode_st_ble_mlss_periph_addr_t(const st_ble_mlss_periph_addr_t *p_app_value, st_ble_gatt_value_t *p_gatt_value)
{
    /* Start user code for Peripheral Address characteristic value encode function. Do not edit comment generated here */
    uint32_t pos = 0;

    for (uint32_t i = 0; i < 7; i++)
    {
        BT_PACK_LE_1_BYTE(&p_gatt_value->p_value[pos], &p_app_value->periph_addr[i]);
        pos += 1;
    }

    p_gatt_value->value_len = (uint16_t) pos;

    /* End user code. Do not edit comment generated here */
    return BLE_SUCCESS;
}

/* Peripheral Address characteristic definition */
static const st_ble_servs_char_info_t gs_periph_addr_char = {
    .start_hdl    = BLE_MLSS_PERIPH_ADDR_DECL_HDL,
    .end_hdl      = BLE_MLSS_PERIPH_ADDR_VAL_HDL,
    .char_idx     = BLE_MLSS_PERIPH_ADDR_IDX,
    .app_size     = sizeof(st_ble_mlss_periph_addr_t),
    .db_size      = BLE_MLSS_PERIPH_ADDR_LEN,
    .decode       = (ble_servs_attr_decode_t)decode_st_ble_mlss_periph_addr_t,
    .encode       = (ble_servs_attr_encode_t)encode_st_ble_mlss_periph_addr_t,
};

/*----------------------------------------------------------------------------------------------------------------------
    Multi-Link server
----------------------------------------------------------------------------------------------------------------------*/

/* Multi-Link characteristics definition */
static const st_ble_servs_char_info_t *gspp_chars[] = {
    &gs_periph_addr_char,
};

/* Multi-Link service definition */
static st_ble_servs_info_t gs_servs_info = {
    .pp_chars     = gspp_chars,
    .num_of_chars = ARRAY_SIZE(gspp_chars),
};

ble_status_t R_BLE_MLSS_Init(ble_servs_app_cb_t cb)
{
    if (NULL == cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    gs_servs_info.cb = cb;

    return R_BLE_SERVS_RegisterServer(&gs_servs_info);
}
