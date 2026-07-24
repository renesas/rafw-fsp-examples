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

/******************************************************************************
* File Name    : app_main.c
* Device(s)    : RA4W1
* Tool-Chain   : e2Studio
* Description  : This is a application file for peripheral role.
*******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
*******************************************************************************/
#include <string.h>
#include "r_ble_api.h"
#include "rm_ble_abs.h"
#include "rm_ble_abs_api.h"
#include "gatt_db.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "profile_cmn/r_ble_servc_if.h"
#include "hal_data.h"

/* This code is needed for using FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#define BLE_EVENT_PATTERN   (0x0A0A)
EventGroupHandle_t  g_ble_event_group_handle;
#endif
#include "r_ble_di2s.h"

/******************************************************************************
 User file includes
*******************************************************************************/
/* Start user code for file includes. Do not edit comment generated here */
#include <stdio.h>
#include "ble_thread.h"
#include "cli_ble_queue.h"

#ifdef RM_MAP_PERSISTANT_W
#include "rm_map_persistant_w.h"
#include dg_configADNVPARAM_PROJ_FILE
#else
#include "rm_vee_flash_w_rrq_nvram.h"
#endif

/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE       (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN          (245)
#define BLE_GATTS_QUEUE_NUM                 (1)

/******************************************************************************
 User macro definitions
*******************************************************************************/
/* Start user code for macro definitions. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated function prototype declarations
*******************************************************************************/
/* Internal functions */
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data);
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data);
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data);
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data);
ble_status_t ble_init(void);
void app_main(void);

/******************************************************************************
 User function prototype declarations
*******************************************************************************/
/* Start user code for function prototype declarations. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated global variables
*******************************************************************************/
/* Advertising Data */
static uint8_t gs_advertising_data[] =
{
    /* Flags */
    0x02, /**< Data Size */
    0x01, /**< Data Type */
    ( 0x06 ), /**< Data Value */
    /* Complete Local Name */
    0x17, /**< Data Size */
    0x09, /**< Data Type */
    0x76, 0x6e, 0x64, 0x6d, 0x5f, 0x77, 0x6c, 0x61, 0x6e, 0x5f, 0x62, 0x6c, 0x65, 0x5f, 0x63, 0x6f, 0x65, 0x78, 0x5f, 0x61, 0x70, 0x70, /**< Data Value */
};
/* Scan Response Data */
static uint8_t gs_scan_response_data[] =
{
    /* Complete Local Name */
    0x17, /**< Data Size */
    0x09, /**< Data Type */
    0x76, 0x6e, 0x64, 0x6d, 0x5f, 0x77, 0x6c, 0x61, 0x6e, 0x5f, 0x62, 0x6c, 0x65, 0x5f, 0x63, 0x6f, 0x65, 0x78, 0x5f, 0x61, 0x70, 0x70, /**< Data Value */
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
 .p_peer_address             = NULL,       ///< Peer address.
 .slow_advertising_interval  = 0x000000A0, ///< Slow advertising interval. 100.0(ms)
 .slow_advertising_period    = 0x0000,     ///< Slow advertising period.
 .p_advertising_data         = gs_advertising_data,             ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
 .advertising_data_length    = ARRAY_SIZE(gs_advertising_data), ///< Advertising data length (in bytes).
 .p_scan_response_data       = gs_scan_response_data,             ///< Scan response data. If p_scan_response_data is specified as NULL, scan response data is not set.
 .scan_response_data_length  = ARRAY_SIZE(gs_scan_response_data), ///< Scan response data length (in bytes).
 .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY, ///< Advertising Filter Policy.
 .advertising_channel_map    = ( BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39 ), ///< Channel Map.
 .own_bluetooth_address_type = BLE_GAP_ADDR_RAND, ///< Own Bluetooth address type.
 .own_bluetooth_address      = { 0 },
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
static st_ble_gatt_queue_elm_t  gs_queue_elms[BLE_GATTS_QUEUE_ELEMENTS_SIZE];
static uint8_t gs_buffer[BLE_GATTS_QUEUE_BUFFER_LEN];
static st_ble_gatt_pre_queue_t gs_queue[BLE_GATTS_QUEUE_NUM] = {
    {
        .p_buf_start = gs_buffer,
        .buffer_len  = BLE_GATTS_QUEUE_BUFFER_LEN,
        .p_queue     = gs_queue_elms,
        .queue_size  = BLE_GATTS_QUEUE_ELEMENTS_SIZE,
    }
};

typedef enum e_mfr_app_state
{
    MFR_APP_STATE_STOPPED,
    MFR_APP_STATE_STOPPING,
    MFR_APP_STATE_RUNNING,
} mfr_app_state_t;

/* Connection handle */
volatile uint16_t g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

/* Advertisement handle */
volatile uint8_t g_adv_hdl   = BLE_GAP_INVALID_ADV_HDL;

mfr_app_state_t g_app_state = MFR_APP_STATE_RUNNING; /* Application starts with running state */

/******************************************************************************
 User global variables
*******************************************************************************/
/* Start user code for global variables. Do not edit comment generated here */
uint8_t remote_addr[BLE_BD_ADDR_LEN];
uint8_t remote_addr_type;
char remote_addr_type_str[16];
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated function definitions
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
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GAP callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch(type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            /* Get BD address for Advertising */
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);
        } break;

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                /* Store connection handle */
                st_ble_gap_conn_evt_t *p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *)p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
                memcpy(&remote_addr[0], &p_gap_conn_evt_param->remote_addr[0], BLE_BD_ADDR_LEN);
                remote_addr_type = p_gap_conn_evt_param->remote_addr_type;
                switch (remote_addr_type)
                {
                case 0x00:
                    memcpy(&remote_addr_type_str[0], "public", 7);
                    break;
                case 0x01:
                    memcpy(&remote_addr_type_str[0], "random", 7);
                    break;
                case 0x02:
                    memcpy(&remote_addr_type_str[0], "public identity", 16);
                    break;
                case 0x03:
                    memcpy(&remote_addr_type_str[0], "random identity", 16);
                    break;
                default:
                    memcpy(&remote_addr_type_str[0], " ", 2);
                    break;
                }
                printf("Device connected\n");
                printf("\tConnection index: %X\n", g_conn_hdl);
                printf("\tAddress: %s %02X:%02X:%02X:%02X:%02X:%02X\n\n", remote_addr_type_str, remote_addr[5], remote_addr[4], remote_addr[3], remote_addr[2], remote_addr[1], remote_addr[0]);
            }
            else
            {
                /* Restart advertising when connection failed */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }
        } break;

        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
            st_ble_gap_data_len_chg_evt_t *p_gap_data_len_chg_evt_param = (st_ble_gap_data_len_chg_evt_t *)p_data->p_param;
            uint16_t tx_octets = p_gap_data_len_chg_evt_param->tx_octets;
            uint16_t tx_time = p_gap_data_len_chg_evt_param->tx_time;
            uint16_t rx_octets = p_gap_data_len_chg_evt_param->rx_octets;
            uint16_t rx_time = p_gap_data_len_chg_evt_param->rx_time;
            printf("Data length changed \n\tConnection index: %X\n", g_conn_hdl);
            printf("\tMaximum RX data length: %d\n\tMaximum RX time: %d\n", rx_octets, rx_time);
            printf("\tMaximum TX data length: %d\n\tMaximum TX time: %d\n\n", tx_octets, tx_time);
        } break;
        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            st_ble_gap_pairing_req_evt_t *p_gap_pairing_req_evt_param = (st_ble_gap_pairing_req_evt_t *)p_data->p_param;
            st_ble_gap_auth_info_t auth_info = p_gap_pairing_req_evt_param->auth_info;
            uint8_t bonding = auth_info.bonding;
            printf("Pair request\n");
            printf("\tConnection index: %X\n", g_conn_hdl);
            printf("\tBond: %s\n\n", bonding ? "true" : "false");
        } break;
        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            st_ble_gap_pairing_info_evt_t *p_gap_pairing_info_evt_param = (st_ble_gap_pairing_info_evt_t *)p_data->p_param;
            st_ble_gap_auth_info_t auth_info = p_gap_pairing_info_evt_param->auth_info;
            uint8_t bonding = auth_info.bonding;
            printf("Pair completed\n");
            printf("\tConnection index: %X\n", g_conn_hdl);
            printf("\tBond: %s\n\n", bonding ? "true" : "false");
        } break;
        case BLE_GAP_EVENT_ENC_CHG:
        {
            st_ble_gap_enc_chg_evt_t *p_gap_enc_chg_evt_param = (st_ble_gap_enc_chg_evt_t *)p_data->p_param;
            uint8_t enc_status = p_gap_enc_chg_evt_param->enc_status;
            printf("Security level changed\n");
            printf("\tConnection index: %X\n", g_conn_hdl);
            printf("\tSecurity level: %d\n\n", enc_status);
        } break;
        case BLE_GAP_EVENT_DISCONN_IND:
        {
            /* Restart advertising when disconnected */
            st_ble_gap_disconn_evt_t *p_gap_disconn_evt_param = (st_ble_gap_disconn_evt_t *)p_data->p_param;
            uint8_t disconn_reason = p_gap_disconn_evt_param->reason;
            printf("Device disconnected\n");
            printf("\tConnection index: %X\n", g_conn_hdl);
            printf("\tBD address of disconnected device: %s, %02X:%02X:%02X:%02X:%02X:%02X\n", remote_addr_type_str, remote_addr[5], remote_addr[4], remote_addr[3], remote_addr[2], remote_addr[1], remote_addr[0]);
            printf("\tReason of disconnection: 0x%X\n\n", disconn_reason);
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;

            if (MFR_APP_STATE_RUNNING == g_app_state)
            {
                /* We aren't disconnecting due to closing of the service - restore the advertisement. */
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }
        } break;

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            /* Send connection update response with value received on connection update request */
            st_ble_gap_conn_upd_req_evt_t *p_conn_upd_req_evt_param = (st_ble_gap_conn_upd_req_evt_t *)p_data->p_param;

            st_ble_gap_conn_param_t conn_updt_param = {
                .conn_intv_min = p_conn_upd_req_evt_param->conn_intv_min,
                .conn_intv_max = p_conn_upd_req_evt_param->conn_intv_max,
                .conn_latency  = p_conn_upd_req_evt_param->conn_latency,
                .sup_to        = p_conn_upd_req_evt_param->sup_to,
            };

            R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
                              BLE_GAP_CONN_UPD_MODE_RSP,
                              BLE_GAP_CONN_UPD_ACCEPT,
                              &conn_updt_param);
        } break;

/* Hint: Add cases of GAP event macros defined as BLE_GAP_XXX */
/* Start user code for GAP callback function event process. Do not edit comment generated here */
        case BLE_GAP_EVENT_ADV_ON:
        {
            st_ble_gap_adv_set_evt_t * p_gap_adv_set_evt_param = (st_ble_gap_adv_set_evt_t *) p_data->p_param;
            g_adv_hdl = p_gap_adv_set_evt_param->adv_hdl;

            printf("Legacy advertising started\n");
        } break;

        case BLE_GAP_EVENT_ADV_OFF:
        {
            g_adv_hdl = BLE_GAP_INVALID_ADV_HDL;

            printf("Legacy advertising stopped\n");
        } break;
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
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVS_GattsCb(type, result, p_data);
    switch(type)
    {
/* Hint: Add cases of GATT Server event macros defined as BLE_GATTS_XXX */
/* Start user code for GATT Server callback function event process. Do not edit comment generated here */
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
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for GATT Client callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    R_BLE_SERVC_GattcCb(type, result, p_data);
    switch(type)
    {

/* Hint: Add cases of GATT Client event macros defined as BLE_GATTC_XXX */
/* Start user code for GATT Client callback function event process. Do not edit comment generated here */
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
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for vender specific callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    
    R_BLE_SERVS_VsCb(type, result, p_data);
    switch(type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *)p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
            printf("BD address from device: %02x:%02x:%02x:%02x:%02x:%02x\n", get_address->addr.addr[5], get_address->addr.addr[4], 
                                                                               get_address->addr.addr[3], get_address->addr.addr[2],
                                                                               get_address->addr.addr[1], get_address->addr.addr[0]);
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
        } break;

/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: di2s_cb
 * Description  : Callback function for Device Information Service2 server feature.
 * Arguments    : uint16_t type -
 *                  Event type of Device Information Service2 server feature.
 *              : ble_status_t result -
 *                  Event result of Device Information Service2 server feature.
 *              : st_ble_servs_evt_data_t *p_data - 
 *                  Event parameters of Device Information Service2 server feature.
 * Return Value : none
 ******************************************************************************/
static void di2s_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data)
{
/* Hint: Input common process of callback function such as variable definitions */
/* Start user code for Device Information Service2 Server callback function common process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    switch(type)
    {
/* Hint: Add cases of Device Information Service2 server events defined in e_ble_di2s_event_t */
/* Start user code for Device Information Service2 Server callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }    
}
/******************************************************************************
 * Function Name: ble_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : none
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
ble_status_t ble_init(void)
{
    ble_status_t status;
    fsp_err_t err;

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

    /* Initialize Device Information Service2 server API */
    status = R_BLE_DI2S_Init(di2s_cb);
    if (BLE_SUCCESS != status)
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    return status;
}

ble_status_t ble_uninit(void)
{
    /* Terminate BLE */
    return RM_BLE_ABS_Close(&g_ble_abs0_ctrl);;
}

static void ble_set_bd_addr(const char *bd_addr_string)
{
    if (!bd_addr_string)
        return;

    size_t bd_addr_len = strlen(bd_addr_string);
    char tmp_bd_string[bd_addr_len + 1];

    memset(tmp_bd_string, 0, bd_addr_len + 1);
    memcpy(tmp_bd_string, bd_addr_string, bd_addr_len);

    st_ble_dev_addr_t bd_addr = {.type = BLE_GAP_ADDR_PUBLIC, .addr = {0} };

    char *saveptr;
    char *p_tp = strtok_r(tmp_bd_string, ":", &saveptr);

    unsigned int p = BLE_BD_ADDR_LEN - 1;

    while ((p_tp != NULL) && (p >= 0)) {
        bd_addr.addr[p] = (char)strtol(p_tp, NULL, 16);
        p--;

        p_tp = strtok_r(NULL, ":", &saveptr);
    }

    ble_status_t ret = R_BLE_VS_SetBdAddr(BLE_VS_ADDR_AREA_REG, &bd_addr);
    assert(BLE_SUCCESS == ret);
}

/******************************************************************************
 * Function Name: app_main
 * Description  : Application main function with main loop
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
void app_main(void)
{
    ble_status_t ret;  
#if CFG_PMGR
    extern int RM_PMGR_W_dpm_is_wakeup(void);
    extern bool RM_PMGR_W_IsSleep3Wakeup(void);
    int dpm_ble_hibernate = 0;
    bool is_sleep3_wakeup = RM_PMGR_W_dpm_is_wakeup() || RM_PMGR_W_IsSleep3Wakeup();
#endif CFG_PMGR
    uint32_t timeout;

#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
    /* Create Event Group */
    g_ble_event_group_handle = xEventGroupCreate();
    assert(g_ble_event_group_handle);
#endif

    /* Initialize BLE and profiles */

    char *p_bd_addr = NULL;
#ifndef RM_MAP_PERSISTANT_W
    p_bd_addr = read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, "PUBLIC_BD_ADDR", &p_bd_addr);

#endif
#if CFG_PMGR
    if (is_sleep3_wakeup == false)
#endif
    {
        if (p_bd_addr) {
            ble_set_bd_addr(p_bd_addr);
        }

        ret = ble_init();
        assert(BLE_SUCCESS == ret);

        ret = R_BLE_GTL_VS_SetSleepModeExt(R_BLE_GTL_SLEEP_MODE_EXTENDED, R_BLE_GTL_SLEEP_RAM_MODE_ON, 1 << DA1453x_GTL_CTS_PIN, 0);
        assert(BLE_SUCCESS == ret);
    }
/* Hint: Input process that should be done before main loop such as calling initial function or variable definitions */
/* Start user code for process before main loop. Do not edit comment generated here */
    cli_ble_queue_init();

#if CFG_PMGR
#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_WIFIPROFILE,
        WIFI_PROFILE_DPM_BLE_HIBERNATE, &dpm_ble_hibernate);
#else
    dpm_ble_hibernate = MODE_DISABLE; /* dpm_ble_hibernate (combo mode) disabled by default */
#endif
    if(dpm_ble_hibernate == MODE_ENABLE)
    {
        if (is_sleep3_wakeup)
        {
            g_app_state = MFR_APP_STATE_STOPPED;
        }
        else
        {
            cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_SVC_STOP };

            cli_ble_queue_enqueue(&msg);
        }
    }
#endif
/* End user code. Do not edit comment generated here */
    /* main loop */
    while (1)
    {
        cli_ble_msg_t msg;
        
        timeout = (g_app_state == MFR_APP_STATE_RUNNING) ? 0 : portMAX_DELAY;

        if (cli_ble_queue_dequeue(&msg, timeout))
        {
            switch (msg.type)
            {
                case CLI_BLE_MSG_TYPE_SVC_START:
                {
                    /* Started already */
                    if (MFR_APP_STATE_STOPPED != g_app_state)
                    {
                        break;
                    }

#ifndef RM_MAP_PERSISTANT_W
                    p_bd_addr = (uint8_t *)read_nvram_blecfg_string("PUBLIC_BD_ADDR");
#else
                    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, "PUBLIC_BD_ADDR", (char *)&p_bd_addr);
#endif
                    if (p_bd_addr) {
                        ble_set_bd_addr(p_bd_addr);
                    }

                    ret = ble_init();
                    assert(BLE_SUCCESS == ret);

                    ret = R_BLE_GTL_VS_SetSleepModeExt(R_BLE_GTL_SLEEP_MODE_EXTENDED, R_BLE_GTL_SLEEP_RAM_MODE_ON, 1 << DA1453x_GTL_CTS_PIN, 0);
                    assert(BLE_SUCCESS == ret);

                    g_app_state = MFR_APP_STATE_RUNNING;
                } break;

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
                        if (BLE_SUCCESS != ret)
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
                } break;

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
                } break;

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
                } break;

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
                        if (BLE_SUCCESS != ret)
                        {
                            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
                        }
                    }
                } break;

                case CLI_BLE_MSG_TYPE_BD_ADDR_GET:
                {
                    /* Not running */
                    if (MFR_APP_STATE_RUNNING != g_app_state)
                    {
                        break;
                    }

                    ret = R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_PUBLIC);
                    assert (BLE_SUCCESS == ret);

                } break;

                default:
                {
                    printf("Received CLI msg of type (%d) which was not handled.\n", msg.type);
                }
            }
        }

        if (MFR_APP_STATE_STOPPING == g_app_state &&
            BLE_GAP_INVALID_CONN_HDL == g_conn_hdl &&
            BLE_GAP_INVALID_ADV_HDL == g_adv_hdl)
        {
            ret = ble_uninit();
            assert(BLE_SUCCESS == ret);

            g_app_state = MFR_APP_STATE_STOPPED;
            continue;
        }

        if (MFR_APP_STATE_STOPPED != g_app_state)
        {
            /* Process BLE Event */
            ret = R_BLE_Execute();
            assert(BLE_SUCCESS == ret);
        }
        else
        {
            continue;
        }

/* When this BLE application works on the FreeRTOS */
#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
        if(0 != R_BLE_IsTaskFree())
        {
            /* If the BLE Task has no operation to be processed, it transits block state until the event from RF transciever occurs. */
            xEventGroupWaitBits(g_ble_event_group_handle,
                                (EventBits_t)BLE_EVENT_PATTERN,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);
        }
#endif

/* Hint: Input process that should be done during main loop such as calling processing functions */
/* Start user code for process during main loop. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }

/* Hint: Input process that should be done after main loop such as calling closing functions */
/* Start user code for process after main loop. Do not edit comment generated here */
    cli_ble_queue_uninit();
/* End user code. Do not edit comment generated here */

    ret = ble_uninit();
    assert(BLE_SUCCESS == ret);
}

/******************************************************************************
 User function definitions
*******************************************************************************/
/* Start user code for function definitions. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
