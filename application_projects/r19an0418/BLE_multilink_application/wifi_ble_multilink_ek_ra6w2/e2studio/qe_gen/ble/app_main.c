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
 * Copyright (C) 2019-2025 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

/******************************************************************************
 * File Name    : app_main.c
 * Tool-Chain   : e2Studio
 * Description  : This is a application file for peripheral role.
 *******************************************************************************/

/******************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *******************************************************************************/
#include <string.h>
#include "r_ble_api.h"
#include "rm_ble_abs.h"
#include "rm_ble_abs_api.h"
#include "discovery/r_ble_disc.h"
#include "gatt_db.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "profile_cmn/r_ble_servc_if.h"
#include "hal_data.h"

/* This code is needed for using FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
 #include "FreeRTOS.h"
 #include "task.h"
 #include "event_groups.h"
 #define BLE_EVENT_PATTERN    (0x0A0A)
EventGroupHandle_t g_ble_event_group_handle;
#endif
#include "r_ble_gapc.h"
#include "r_ble_gaps.h"
#include "r_ble_gats.h"
#include "r_ble_mlss.h"

/******************************************************************************
 * User file includes
 *******************************************************************************/

/* Start user code for file includes. Do not edit comment generated here */
#include "r_ble_gtl.h"
#include "common_utils.h"
#include "qe_ble_profile.h"
#include "ble_thread.h"
#include "app_lib/r_ble_ml.h"

#include "app_lib/utils_enum.h"

#include "ra6w1_platform_nvparam.h"
#ifdef RM_MAP_PERSISTANT_W
 #include "rm_map_persistant_w.h"
#else
 #include "rm_vee_flash_w_rrq_nvram.h"
#endif

#include "cli_ble_queue.h"

/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG                      "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE    (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN       (245)
#define BLE_GATTS_QUEUE_NUM              (1)

/******************************************************************************
 * User macro definitions
 *******************************************************************************/

/* Start user code for macro definitions. Do not edit comment generated here */
#define BLE_RF_FORCE_EN                  (1)
#define CNT_MAX                          (50)

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated function prototype declarations
 *******************************************************************************/

/* Internal functions */
void         gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data);
void         gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t * p_data);
void         gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t * p_data);
void         vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t * p_data);
void         disc_comp_cb(uint16_t conn_hdl);
ble_status_t ble_init(void);
void         app_main(void);

/******************************************************************************
 * User function prototype declarations
 *******************************************************************************/

/* Start user code for function prototype declarations. Do not edit comment generated here */
static void         ble_set_bd_addr(const char * bd_addr_string);
static const char * ble_address_to_string(const uint8_t * address);
static ble_status_t ble_uninit(void);

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated global variables
 *******************************************************************************/

/* GAP Service UUID */
static uint8_t GAPC_UUID[] = {0x00, 0x18};

/* Service discovery parameters */
static st_ble_disc_entry_t gs_disc_entries[] =
{
    {
        .p_uuid    = GAPC_UUID,
        .uuid_type = BLE_GATT_16_BIT_UUID_FORMAT,
        .serv_cb   = R_BLE_GAPC_ServDiscCb,
    },
};

/* Advertising Data */
static uint8_t gs_advertising_data[] =
{
    /* Flags */
    0x02,                                                                                                       /**< Data Size */
    0x01,                                                                                                       /**< Data Type */
    (0x06),                                                                                                     /**< Data Value */

    /* Complete Local Name */
    0x13,                                                                                                       /**< Data Size */
    0x09,                                                                                                       /**< Data Type */
    0x52, 0x65, 0x6e, 0x65, 0x73, 0x61, 0x73, 0x20, 0x4d, 0x75, 0x6c, 0x74, 0x69, 0x2d, 0x6c, 0x69, 0x6e, 0x6b, /**< Data Value */
};

/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Complete Local Name */
    0x13,                                                                                                       /**< Data Size */
    0x09,                                                                                                       /**< Data Type */
    0x52, 0x65, 0x6e, 0x65, 0x73, 0x61, 0x73, 0x20, 0x4d, 0x75, 0x6c, 0x74, 0x69, 0x2d, 0x6c, 0x69, 0x6e, 0x6b, /**< Data Value */

    /* Manufacturer Specific Data */
    0x06,                                                                                                       /**< Data Size */
    0xff,                                                                                                       /**< Data Type */
    0x55, 0x55, 0x55, 0x55, 0x55,                                                                               /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
    .p_peer_address             = NULL,                                                        ///< Peer address.
    .fast_advertising_interval  = 0x00000030,                                                  ///< Fast advertising interval. 30.0(ms)
    .fast_advertising_period    = 0x0BB8,                                                      ///< Fast advertising period. 30,000(ms)
    .slow_advertising_interval  = 0x000000A0,                                                  ///< Slow advertising interval. 100.0(ms)
    .slow_advertising_period    = 0x0000,                                                      ///< Slow advertising period.
    .p_advertising_data         = gs_advertising_data,                                         ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
    .advertising_data_length    = ARRAY_SIZE(gs_advertising_data),                             ///< Advertising data length (in bytes).
    .p_scan_response_data       = gs_scan_response_data,                                       ///< Scan response data. If p_scan_response_data is specified as NULL, scan response data is not set.
    .scan_response_data_length  = ARRAY_SIZE(gs_scan_response_data),                           ///< Scan response data length (in bytes).
    .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY,                        ///< Advertising Filter Policy.
    .advertising_channel_map    = (BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39), ///< Channel Map.
    .own_bluetooth_address_type = BLE_GAP_ADDR_RAND,                                           ///< Own Bluetooth address type.
    .own_bluetooth_address      = {0},
};

/* GATT server callback parameters */
ble_abs_gatt_server_callback_set_t gs_abs_gatts_cb_param[] =
{
    {
        .gatt_server_callback_function = gatts_cb,
        .gatt_server_callback_priority = 1,
    },
    {
        .gatt_server_callback_function = NULL,
    }
};

/* GATT client callback parameters */
ble_abs_gatt_client_callback_set_t gs_abs_gattc_cb_param[] =
{
    {
        .gatt_client_callback_function = gattc_cb,
        .gatt_client_callback_priority = 1,
    },
    {
        .gatt_client_callback_function = NULL,
    }
};

/* GATT server Prepare Write Queue parameters */
static st_ble_gatt_queue_elm_t gs_queue_elms[BLE_GATTS_QUEUE_ELEMENTS_SIZE];
static uint8_t                 gs_buffer[BLE_GATTS_QUEUE_BUFFER_LEN];
static st_ble_gatt_pre_queue_t gs_queue[BLE_GATTS_QUEUE_NUM] =
{
    {
        .p_buf_start = gs_buffer,
        .buffer_len  = BLE_GATTS_QUEUE_BUFFER_LEN,
        .p_queue     = gs_queue_elms,
        .queue_size  = BLE_GATTS_QUEUE_ELEMENTS_SIZE,
    }
};

/* Connection handle */
uint16_t g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

/******************************************************************************
 * User global variables
 *******************************************************************************/

/* Start user code for global variables. Do not edit comment generated here */

/* Advertisement handle */
uint8_t g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

typedef enum e_mfr_app_state
{
    MFR_APP_STATE_STOPPED,
    MFR_APP_STATE_STOPPING,
    MFR_APP_STATE_RUNNING,
} mfr_app_state_t;

mfr_app_state_t g_app_state = MFR_APP_STATE_RUNNING; /* Application starts with running state */

/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Generated function definitions
 *******************************************************************************/

/******************************************************************************
 * Function Name: gap_cb
 * Description  : Callback function for GAP API.
 * Arguments    : uint16_t type -
 *                  Event type of GAP API.
 *              : ble_status_t result -
 *                  Event result of GAP API.
 *              : st_ble_vs_evt_data_t *p_data -
 *                  Event parameters of GAP API.
 * Return Value : none
 ******************************************************************************/
void gap_cb (uint16_t type, ble_status_t result, st_ble_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP callback function common process. Do not edit comment generated here */
    APP_PRINT("[%s]\r\n", E_BLE_GAP_EVT_T(type));
    if (R_BLE_ML_AbsGapCb(type, result, p_data))
    {

        /* event handled by Multi-link application */
        return;
    }

    switch (type)
    {
        case BLE_GAP_EVENT_LOC_VER_INFO:
        {
            /* fix to local identity address not being set (resulting in GAP resolved providing
             * incorrect private address */
            R_BLE_GAP_SetLocIdInfo((st_ble_dev_addr_t *) (&g_ble_abs0_ctrl.loc_bd_addr), g_ble_abs0_ctrl.local_irk);
            break;
        }

        default:
        {
            break;
        }
    }

/* End user code. Do not edit comment generated here */

    switch (type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            /* Get BD address for Advertising */
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);
            break;
        }

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t * p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *) p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
            }
            else
            {
                /* Restart advertising when connection failed */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }

            break;
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Restart advertising when disconnected */
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            /* Send connection update response with value received on connection update request */
            st_ble_gap_conn_upd_req_evt_t * p_conn_upd_req_evt_param =
                (st_ble_gap_conn_upd_req_evt_t *) p_data->p_param;

            st_ble_gap_conn_param_t conn_updt_param =
            {
                .conn_intv_min = p_conn_upd_req_evt_param->conn_intv_min,
                .conn_intv_max = p_conn_upd_req_evt_param->conn_intv_max,
                .conn_latency  = p_conn_upd_req_evt_param->conn_latency,
                .sup_to        = p_conn_upd_req_evt_param->sup_to,
            };

            R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
                              BLE_GAP_CONN_UPD_MODE_RSP,
                              BLE_GAP_CONN_UPD_ACCEPT,
                              &conn_updt_param);
            break;
        }

/* Hint: Add cases of GAP event macros defined as BLE_GAP_XXX */
/* Start user code for GAP callback function event process. Do not edit comment generated here */
        case BLE_GAP_EVENT_ADV_ON:
        {
            st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;
            APP_PRINT_INFO("Legacy advertising started\n");
            break;
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
            APP_PRINT_INFO("Legacy advertising stopped\n");
            break;
        }

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            st_ble_gap_pairing_req_evt_t * p_pair_req_param = (st_ble_gap_pairing_req_evt_t *) p_data->p_param;
            R_BLE_GAP_ReplyPairing(p_pair_req_param->conn_hdl, BLE_GAP_PAIRING_ACCEPT);
            break;
        }

        case BLE_GAP_EVENT_NUM_COMP_REQ:
        {
            st_ble_gap_num_comp_evt_t * p_numcmp_evt_param = (st_ble_gap_num_comp_evt_t *) p_data->p_param;

            APP_PRINT_INFO("Numeric Comparison: %06d \r\n", p_numcmp_evt_param->numeric);

            /* DO THE COMPARISON, APPLICATION JOB */
            R_BLE_GAP_ReplyNumComp(p_numcmp_evt_param->conn_hdl, BLE_GAP_PAIRING_ACCEPT);
            break;
        }

        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gatts_cb
 * Description  : Callback function for GATT Server API.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Server API.
 *              : ble_status_t result -
 *                  Event result of GATT Server API.
 *              : st_ble_gatts_evt_data_t *p_data -
 *                  Event parameters of GATT Server API.
 * Return Value : none
 ******************************************************************************/
void gatts_cb (uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_GattsCb(type, result, p_data);
    switch (type)
    {
/* Hint: Add cases of GATT Server event macros defined as BLE_GATTS_XXX */
/* Start user code for GATT Server callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gattc_cb
 * Description  : Callback function for GATT Client API.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Client API.
 *              : ble_status_t result -
 *                  Event result of GATT Client API.
 *              : st_ble_gattc_evt_data_t *p_data -
 *                  Event parameters of GATT Client API.
 * Return Value : none
 ******************************************************************************/
void gattc_cb (uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVC_GattcCb(type, result, p_data);
    switch (type)
    {
        case BLE_GATTC_EVENT_CONN_IND:
        {
            /* Start discovery operation after connection established */
            R_BLE_DISC_Start(p_data->conn_hdl, gs_disc_entries, ARRAY_SIZE(gs_disc_entries), disc_comp_cb);
            break;
        }

/* Hint: Add cases of GATT Client event macros defined as BLE_GATTC_XXX */
/* Start user code for GATT Client callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: vs_cb
 * Description  : Callback function for Vendor Specific API.
 * Arguments    : uint16_t type -
 *                  Event type of Vendor Specific API.
 *              : ble_status_t result -
 *                  Event result of Vendor Specific API.
 *              : st_ble_vs_evt_data_t *p_data -
 *                  Event parameters of Vendor Specific API.
 * Return Value : none
 ******************************************************************************/
void vs_cb (uint16_t type, ble_status_t result, st_ble_vs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for vender specific callback function common process. Do not edit comment generated here */
    if (BLE_VS_EVENT_GET_ADDR_COMP == type)
    {
        st_ble_vs_get_bd_addr_comp_evt_t * p_get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
        AUTOTEST_PRINT("BD address from device: %s\n", ble_address_to_string(p_get_address->addr.addr));
    }

/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_VsCb(type, result, p_data);
    switch (type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *) p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            break;
        }

/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gaps_cb
 * Description  : Callback function for GAP Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of GAP Service server feature.
 *              : ble_status_t result -
 *                  Event result of GAP Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of GAP Service server feature.
 * Return Value : none
 ******************************************************************************/
static void gaps_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_data);
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GAP Service server events defined in e_ble_gaps_event_t */
/* Start user code for GAP Service Server callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gats_cb
 * Description  : Callback function for GATT Service server feature.
 * Arguments    : uint16_t type -
 *                  Event type of GATT Service server feature.
 *              : ble_status_t result -
 *                  Event result of GATT Service server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of GATT Service server feature.
 * Return Value : none
 ******************************************************************************/
static void gats_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Service Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_data);
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GATT Service server events defined in e_ble_gats_event_t */
/* Start user code for GATT Service Server callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: mlss_cb
 * Description  : Callback function for Multi-Link server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Multi-Link server feature.
 *              : ble_status_t result -
 *                  Event result of Multi-Link server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of Multi-Link server feature.
 * Return Value : none
 ******************************************************************************/
static void mlss_cb (uint16_t type, ble_status_t result, st_ble_servs_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Multi-Link Server callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of Multi-Link server events defined in e_ble_mlss_event_t */
/* Start user code for Multi-Link Server callback function event process. Do not edit comment generated here */
        case BLE_MLSS_EVENT_PERIPH_ADDR_WRITE_REQ:
        case BLE_MLSS_EVENT_PERIPH_ADDR_WRITE_CMD:
        {
            uint8_t * p_addr;
            p_addr = (uint8_t *) p_data->p_param;
            fsp_err_t err;
            APP_PRINT("mlss_cb param_len: %d\r\n", p_data->param_len);
            gs_conn_bd_addr.type = p_addr[0];
            memcpy(gs_conn_bd_addr.addr, &(p_addr[1]), BLE_BD_ADDR_LEN);
            APP_PRINT("mlss_cb addr: %x %x %x %x %x %x\r\n",
                      gs_conn_bd_addr.addr[0],
                      gs_conn_bd_addr.addr[1],
                      gs_conn_bd_addr.addr[2],
                      gs_conn_bd_addr.addr[3],
                      gs_conn_bd_addr.addr[4],
                      gs_conn_bd_addr.addr[5]);

            err = RM_BLE_ABS_CreateConnection(&g_ble_abs0_ctrl, &gs_conn_param);

            if (err != FSP_SUCCESS)
            {
                APP_PRINT("Err %d\r\n", err);
            }

            break;
        }

        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: gapc_cb
 * Description  : Callback function for GAP Service client feature.
 * Arguments    : uint16_t type -
 *                  Event type of GAP Service client feature.
 *              : ble_status_t result -
 *                  Event result of GAP Service client feature.
 *              : st_ble_servc_evt_data_t *p_data -
 *                  Event parameters of GAP Service client feature.
 * Return Value : none
 ******************************************************************************/
static void gapc_cb (uint16_t type, ble_status_t result, st_ble_servc_evt_data_t * p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP Service Client callback function common process. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(p_data);
    FSP_PARAMETER_NOT_USED(result);

/* End user code. Do not edit comment generated here */

    switch (type)
    {
/* Hint: Add cases of GAP Service client events defined in e_ble_gapc_event_t */
/* Start user code for GAP Service Client callback function event process. Do not edit comment generated here */
        default:
        {
            break;
        }

/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: disc_comp_cb
 * Description  : Callback function for Service Discovery.
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void disc_comp_cb (uint16_t conn_hdl)
{
/* Hint: Input process such as GATT operation */
/* Start user code for Discovery Complete callback function. Do not edit comment generated here */
    FSP_PARAMETER_NOT_USED(conn_hdl);

/* End user code. Do not edit comment generated here */
}

/******************************************************************************
 * Function Name: ble_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : none
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
ble_status_t ble_init (void)
{
    ble_status_t status;
    fsp_err_t    err;

    /* Initialize BLE */
    err = RM_BLE_ABS_Open(&g_ble_abs0_ctrl, &g_ble_abs0_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* Initialize GATT Database */
    status = R_BLE_GATTS_SetDbInst(NULL);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT server */
    status = R_BLE_SERVS_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /*Initialize GATT client */
    status = R_BLE_SERVC_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Set Prepare Write Queue */
    R_BLE_GATTS_SetPrepareQueue(gs_queue, BLE_GATTS_QUEUE_NUM);

    /* Initialize GATT Discovery Library */
    status = R_BLE_DISC_Init();
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GAP Service server API */
    status = R_BLE_GAPS_Init(gaps_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT Service server API */
    status = R_BLE_GATS_Init(gats_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize Multi-Link server API */
    status = R_BLE_MLSS_Init(mlss_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GAP Service client API */
    status = R_BLE_GAPC_Init(gapc_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    return status;
}

/******************************************************************************
 * Function Name: app_main
 * Description  : Application main function with main loop
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void app_main (void)
{
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)

    /* Create Event Group */
    g_ble_event_group_handle = xEventGroupCreate();
    assert(g_ble_event_group_handle);
#endif

    /* Initialize BLE and profiles */
    ble_init();

/* Hint: Input process that should be done before main loop such as calling initial function or variable definitions */
/* Start user code for process before main loop. Do not edit comment generated here */
#if BLE_RF_FORCE_EN

    /* Set RF switch to BLE */
    CRG_TOP->CLK_AMBA_REG_b.OQSPI_GPIO_MODE = 1;
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_01, BSP_IO_LEVEL_HIGH);
#endif

    APP_PRINT_INFO("Multi-link application is starting...\n");

    ble_status_t ret;
    char       * p_bd_addr = NULL;

    cli_ble_queue_init();

/* End user code. Do not edit comment generated here */

    /* main loop */
    while (1)
    {
        /* Process BLE Event */
        R_BLE_Execute();

/* When this BLE application works on the FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
        if (0 != R_BLE_IsTaskFree())
        {
            /* If the BLE Task has no operation to be processed, it transits block state until the event from RF transciever occurs. */
            xEventGroupWaitBits(g_ble_event_group_handle,
                                (EventBits_t) BLE_EVENT_PATTERN,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);
        }
#endif

/* Hint: Input process that should be done during main loop such as calling processing functions */
/* Start user code for process during main loop. Do not edit comment generated here */
        cli_ble_msg_t msg;

        if (cli_ble_queue_dequeue(&msg))
        {
            switch (msg.type)
            {
                case CLI_BLE_MSG_TYPE_SVC_START:
                {
                    /* Started already */
                    if (MFR_APP_STATE_RUNNING == g_app_state)
                    {
                        break;
                    }

#ifndef RM_MAP_PERSISTANT_W
                    p_bd_addr = (uint8_t *) read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                    ENV_GROUP_BLECFG,
                                                    "PUBLIC_BD_ADDR",
                                                    (char *) &p_bd_addr);
#endif
                    if (p_bd_addr)
                    {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    g_app_state = MFR_APP_STATE_RUNNING;
                    break;
                }

                case CLI_BLE_MSG_TYPE_SVC_STOP:
                {
                    /* Already not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    g_app_state = MFR_APP_STATE_STOPPING;

                    /* Cleaning up the connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        /* 0x13: Remote User Terminated Connection. */
                        ret = R_BLE_GAP_Disconnect(g_conn_hdl, 0x13);
                        if (BLE_SUCCESS == ret)
                        {
                            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
                        }
                    }

                    /* Cleaning up the advertisement */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        ret = R_BLE_GAP_StopAdv(g_adv_hdl);
                        if (BLE_SUCCESS != ret)
                        {
                            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_ADV_START:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cannot advertise while in active connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        break;
                    }

                    /* We are already advertising */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        break;
                    }

                    ret = RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
                    assert(BLE_SUCCESS == ret);
                    break;
                }

                case CLI_BLE_MSG_TYPE_ADV_STOP:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cleaning up the advertisement */
                    if (BLE_GAP_INVALID_ADV_HDL != g_adv_hdl)
                    {
                        ret = R_BLE_GAP_StopAdv(g_adv_hdl);
                        if (BLE_SUCCESS != ret)
                        {
                            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_DISCONNECT:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    /* Cleaning up the connection */
                    if (BLE_GAP_INVALID_CONN_HDL != g_conn_hdl)
                    {
                        /* 0x13: Remote User Terminated Connection. */
                        ret = R_BLE_GAP_Disconnect(g_conn_hdl, 0x13);
                        if (BLE_SUCCESS == ret)
                        {
                            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
                        }
                    }

                    break;
                }

                case CLI_BLE_MSG_TYPE_BD_ADDR_GET:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    ret = R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_PUBLIC);
                    assert(BLE_SUCCESS == ret);
                    break;
                }

                default:
                {
                    printf("Received CLI msg of type (%d) which was not handled.\n", msg.type);
                }
            }
        }

        if ((MFR_APP_STATE_STOPPING == g_app_state) &&
            (BLE_GAP_INVALID_CONN_HDL == g_conn_hdl) &&
            (BLE_GAP_INVALID_ADV_HDL == g_adv_hdl))
        {
            ret = ble_uninit();
            assert(BLE_SUCCESS == ret);

            g_app_state = MFR_APP_STATE_STOPPED;
        }
        while(MFR_APP_STATE_STOPPED == g_app_state)
        {
            if (cli_ble_queue_dequeue(&msg) &&
                CLI_BLE_MSG_TYPE_SVC_START == msg.type)
            {
#ifndef RM_MAP_PERSISTANT_W
                p_bd_addr = (uint8_t *)read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                ENV_GROUP_BLECFG,
                                                "PUBLIC_BD_ADDR",
                                                (char *)&p_bd_addr);
#endif
                if (p_bd_addr)
                {
                    ble_set_bd_addr(p_bd_addr);
                }

                ret = ble_init();
                assert(BLE_SUCCESS == ret);

                g_app_state = MFR_APP_STATE_RUNNING;
            }
            else
            {
                vTaskDelay(10);
            }
        }

/* End user code. Do not edit comment generated here */
    }

/* Hint: Input process that should be done after main loop such as calling closing functions */
/* Start user code for process after main loop. Do not edit comment generated here */
    cli_ble_queue_uninit();

/* End user code. Do not edit comment generated here */

    /* Terminate BLE */
    RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 * User function definitions
 *******************************************************************************/

/* Start user code for function definitions. Do not edit comment generated here */
static void ble_set_bd_addr (const char * bd_addr_string)
{
    if (!bd_addr_string)
    {
        return;
    }

    size_t bd_addr_len = strlen(bd_addr_string);
    char   tmp_bd_string[bd_addr_len + 1];
    memset(tmp_bd_string, 0, bd_addr_len + 1);
    memcpy(tmp_bd_string, bd_addr_string, bd_addr_len);
    st_ble_dev_addr_t bd_addr = {.type = BLE_GAP_ADDR_PUBLIC, .addr = {0}};
    char            * saveptr;
    char            * p_tp = strtok_r(tmp_bd_string, ":", &saveptr);
    unsigned int      p    = BLE_BD_ADDR_LEN;
    while ((NULL != p_tp) && (p > 0))
    {
        bd_addr.addr[p - 1] = (char) strtol(p_tp, NULL, 16);
        p--;
        p_tp = strtok_r(NULL, ":", &saveptr);
    }

    ble_status_t ret = R_BLE_VS_SetBdAddr(BLE_VS_ADDR_AREA_REG, &bd_addr);
    assert(BLE_SUCCESS == ret);
}

void cust_conf_ble (void)
{
    char * p_bd_addr = NULL;
#ifndef RM_MAP_PERSISTANT_W
    p_bd_addr = read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
    fsp_err_t err = 0;
    uint32_t  cnt = 0;

    /* Wait until RM_MAP_PERSISTANT_W_Open is called from WiFi thread */
    while (cnt++ < CNT_MAX)
    {
        err = RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                              ENV_GROUP_BLECFG,
                                              "PUBLIC_BD_ADDR",
                                              &p_bd_addr);
        if ((err == FSP_SUCCESS) || (err == FSP_ERR_NOT_FOUND))
        {
            break;
        }

        vTaskDelay(portCONVERT_MS_2_TICKS(100));
    }
    assert((FSP_SUCCESS == err || FSP_ERR_NOT_FOUND == err));
#endif

    if (p_bd_addr)
    {
        ble_set_bd_addr(p_bd_addr);
    }
}

/******************************************************************************
 * Function Name: ble_address_to_string
 * Description  : Converts a BLE address into a colon-separated string format
 * Arguments    : const uint8_t *address - Pointer to BLE address array
 * Return Value : const char* - String representation of BLE address
 ******************************************************************************/
static const char * ble_address_to_string (const uint8_t * address)
{
    static char buf[18];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", address[5], address[4], address[3], address[2], address[1],
            address[0]);

    return buf;
}

/******************************************************************************
 * Function Name: ble_uninit
 * Description  : Shuts down the BLE stack and closes the BLE ABS instance.
 * Arguments    : none
 * Return Value : ble_status_t - BLE_SUCCESS on success or error code on failure
 ******************************************************************************/
static ble_status_t ble_uninit (void)
{

    /* Terminate BLE */
    return RM_BLE_ABS_Close(&g_ble_abs0_ctrl);;
}

/* End user code. Do not edit comment generated here */
