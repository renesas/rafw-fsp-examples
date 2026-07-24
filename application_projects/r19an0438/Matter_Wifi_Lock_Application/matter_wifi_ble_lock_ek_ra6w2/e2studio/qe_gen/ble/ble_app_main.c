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
* File Name    : ble_app_main.c
* Device(s)    : RA4W1
* Tool-Chain   : e2Studio
* Description  : This is a application file for peripheral role.
*******************************************************************************/

/******************************************************************************
 Includes   <System Includes> , "Project Includes"
*******************************************************************************/
#include <string.h>
#include <assert.h>

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
#include "r_ble_WiFiProvisionings.h"
//#include "ble_thread.h"

#include "logging.h"
#include "provisioning.h"

#if defined (__SUPPORT_MATTER_IOT__)
#include "ble_app_main.h"
#include "rnDeviceWrapAPIs.h"
#endif 	//__SUPPORT_MATTER_IOT__

/******************************************************************************
 User file includes
*******************************************************************************/
/* Start user code for file includes. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

#define BLE_LOG_TAG "app_main"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE       (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN          (245)
#define BLE_GATTS_QUEUE_NUM                 (1)

/******************************************************************************
 User macro definitions
*******************************************************************************/
/* Start user code for macro definitions. Do not edit comment generated here */
#define BLE_FAST_ADVERTISEMENT              (1)
#define BLE_MAX_PACKET_SIZE                 (220)
#define BLE_RF_FORCE_EN                     (1)
#define BLE_OUTPUT_BUF_LEN                  (64)
#define BLE_LOG_LEVEL                       (LOG_LEVEL_DEBUG)
/* End user code. Do not edit comment generated here */

/******************************************************************************
 Generated function prototype declarations
*******************************************************************************/
/* Internal functions */
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data);
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data);
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data);
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data);
//ble_status_t ble_init(void);  //  org change it for Matter
void app_main(void);

static TaskHandle_t ble_app_main_hdler = NULL;
/******************************************************************************
 User function prototype declarations
*******************************************************************************/
/* Start user code for function prototype declarations. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

uint8_t           g_advertising_handle;

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
};

uint8_t g_scan_response_data[] =
{
    /* Complete Local Name */
    11,
    0x08,
    'R', 'R', 'Q', '-', 'D', 'E', 'V', 'I', 'C', 'E',
    6,
    0xff,                              // NOLINT(readability-magic-numbers)
    0x55, 0x55, 0x55, 0x55, 0x55       // NOLINT(readability-magic-numbers)
};

ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter =
{
   .p_peer_address             = NULL,                ///< Peer address.
   .slow_advertising_interval  = 0x00000640,          ///< Slow advertising interval. 1,000.0(ms)
   .slow_advertising_period    = 0x0000,              ///< Slow advertising period.
   .p_advertising_data         = gs_advertising_data, ///< Advertising data. If p_advertising_data is specified as NULL, advertising data is not set.
   .advertising_data_length    = ARRAY_SIZE(gs_advertising_data), ///< Advertising data length (in bytes).
   .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY, ///< Advertising Filter Policy.
//   .advertising_channel_map    = ( BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39 ), ///< Channel Map.  // org
   .advertising_channel_map    = BLE_GAP_ADV_CH_ALL,	///< Channel Map. for Matter
   .own_bluetooth_address_type = BLE_GAP_ADDR_RAND,   ///< Own Bluetooth address type.	// org
//   .own_bluetooth_address_type = BLE_GAP_ADDR_PUBLIC,
   .own_bluetooth_address      = { 0 },

   .p_scan_response_data       = g_scan_response_data,
   .scan_response_data_length  = (sizeof(g_scan_response_data) / sizeof(g_scan_response_data[0])),

#if BLE_FAST_ADVERTISEMENT
   .fast_advertising_interval  = 0x00000030, ///< Fast advertising interval. 30.0(ms)
   .fast_advertising_period    = 0x0BB8,     ///< Fast advertising period. 30,000(ms)
#endif
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

/* Connection handle */
uint16_t g_conn_hdl;

/******************************************************************************
 User global variables
*******************************************************************************/
/* Start user code for global variables. Do not edit comment generated here */

extern char wifi_networks[];
static char output_buf[BLE_OUTPUT_BUF_LEN];

typedef struct {
  uint16_t remain;
  uint16_t total;
} prov_header_t;

static bool flag_start = false;
static prov_header_t ble_prov_hdr;

static const char* msg2string(uint16_t msg_type) {
#define MSG_2_STRING(msg) case msg: return #msg;

    switch (msg_type) {
        /* WiFiCommand */
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ                        )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_COMP                       )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_REQ  )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_WRITE_COMP )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_CHAR_USER_DESCRIPTION_READ_REQ   )

        /* WiFiStatus */
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_READ_REQ                          )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_HDL_VAL_CNF                       )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_REQ                )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_WRITE_COMP               )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CLI_CNFG_READ_REQ                 )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_REQ   )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_WRITE_COMP  )
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFISTATUS_CHAR_USER_DESCRIPTION_READ_REQ    )

        /* WiFiOutput */
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFIOUTPUT_READ_REQ                          )

        /* WiFiProvisoning */
        MSG_2_STRING( BLE_WIFIPROVISIONINGS_EVENT_WIFIPROVISONING_READ_REQ                     )

#if defined (__SUPPORT_MATTER_IOT__)
		/* Matter RX */
		MSG_2_STRING( BLE_MATTER_EVENT_RX_READ_REQ 											   )
		MSG_2_STRING( BLE_MATTER_EVENT_RX_WRITE_REQ											   )
        MSG_2_STRING( BLE_MATTER_EVENT_RX_WRITE_COMP                                           )
		/* Matter TX */
		MSG_2_STRING( BLE_MATTER_EVENT_TX_WRITE_REQ 										   )
        MSG_2_STRING( BLE_MATTER_EVENT_TX_WRITE_COMP 										   )
        MSG_2_STRING( BLE_MATTER_EVENT_TX_WRITE_HDL_VAL_CNF                                    )
		MSG_2_STRING( BLE_MATTER_EVENT_TX_CHAR_USER_DESCRIPTION_WRITE_REQ 					   )
        MSG_2_STRING( BLE_MATTER_EVENT_TX_CHAR_USER_DESCRIPTION_WRITE_COMP 					   )       
#endif

        default: return "Unknown";
    }
#undef MSG_2_STRING
}

static const char* WiFiErrCode2string(WIFIReturnCode_t rc) {
#define ERRCODE_2_STRING(err_code) case err_code: return #err_code;

    switch (rc) {
        ERRCODE_2_STRING( eWiFiSuccess      )
        ERRCODE_2_STRING( eWiFiFailure      )
        ERRCODE_2_STRING( eWiFiTimeout      )
        ERRCODE_2_STRING( eWiFiNotSupported )

        default: return "Unknown";
    }
#undef ERRCODE_2_STRING
}

static const char* WiFiSecType2string(WIFISecurity_t sec_type) {
#define SECTYPE_2_STRING(type) case type: return #type;

    switch (sec_type) {
        SECTYPE_2_STRING( eWiFiSecurityOpen         )
        SECTYPE_2_STRING( eWiFiSecurityWEP          )
        SECTYPE_2_STRING( eWiFiSecurityWPA          )
        SECTYPE_2_STRING( eWiFiSecurityWPA2         )
        SECTYPE_2_STRING( eWiFiSecurityWPA2_ent     )
        SECTYPE_2_STRING( eWiFiSecurityWPA3         )
        SECTYPE_2_STRING( eWiFiSecurityNotSupported )

        default: return "Unknown";
    }
#undef SECTYPE_2_STRING
}
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
	ble_status_t sta;

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
        	    // RENES_LOG("BLE_GAP_EVENT_CONN_IND g_conn_hdl %d\n", g_conn_hdl);
#if defined (__SUPPORT_MATTER_IOT__)
                evt_ble_matter.conn_handle = g_conn_hdl;
                evt_ble_matter.bType = BT_EVT_DEVICE_CONNECT;
                ble_comm_event(&evt_ble_matter);        
#endif /* __SUPPORT_MATTER_IOT__ */
                sta = R_BLE_GTL_GATTC_ReqExMtu(0, 0xc0);
                RENES_LOG("handle: %d, ReqExMtu stat: %u", g_conn_hdl, sta);
        #if defined (__SUPPORT_MATTER_IOT__)
                evt_ble_matter.bType = BT_EVT_DEVICE_UPDATE_MTU;
                evt_ble_matter.gatt.mtu = 0xc0;//g_dev_params.mtu_size;
                ble_comm_event(&evt_ble_matter);
        #endif //(__SUPPORT_MATTER_IOT__)                

                if (sta == BLE_SUCCESS)
                {
                    st_ble_gap_conn_upd_req_evt_t *p_conn_upd_req_evt_param = (st_ble_gap_conn_upd_req_evt_t *)p_data->p_param;

                    st_ble_gap_conn_param_t conn_updt_param = {
                        .conn_intv_min = (uint16_t)(12/1.25),
                        .conn_intv_max = (uint16_t)(12/1.25),
                        .conn_latency  = (uint16_t)0x0000,
                        .sup_to        = (uint16_t)(10000/10),		// 10sec
                        .min_ce_length = 0xFFFF,
                        .max_ce_length = 0xFFFF,
       			    };

       			    R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
       							  BLE_GAP_CONN_UPD_MODE_RSP,
       							  BLE_GAP_CONN_UPD_ACCEPT,
       							  &conn_updt_param);
                }
            }
            else
            {
                /* Restart advertising when connection failed */
//                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);	// org
            }

        } break;

        case BLE_GAP_EVENT_DISCONN_IND:
        {
        	RENES_LOG("BLE_GAP_EVENT_DISCONN_IND\n");
            /* Restart advertising when disconnected */
//            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
//            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);	// org
//            RENES_LOG("DISCONN and restart with BLE_GAP_INVALID_CONN_HDL\n");
#if defined (__SUPPORT_MATTER_IOT__)
            evt_ble_matter.bType = BT_EVT_DEVICE_DISCONNECTED;
            evt_ble_matter.conn_handle = g_conn_hdl;
            evt_ble_matter.disconn_reason = 0x13;	//BLE_ABS_TEST_DISCOONNECT_REASON Usually, set 0x13 which indicates that a user disconnects the link in r_ble_api.h
            ble_comm_event(&evt_ble_matter);
#endif /* __SUPPORT_MATTER_IOT__ */
        } break;

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
        	// RENES_LOG("BLE_GAP_EVENT_CONN_PARAM_UPD_REQ\n");
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
        case BLE_GAP_EVENT_CONN_PARAM_UPD_COMP:
        {
        	// RENES_LOG("BLE_GAP_EVENT_CONN_PARAM_UPD_COMP\n");
           	st_ble_gap_conn_upd_req_evt_t *p_conn_upd_req_evt_param = (st_ble_gap_conn_upd_req_evt_t *)p_data->p_param;

            RENES_LOG("interval min: %u, interval maxn: %u, latency: %u, sup_to: %d", 
                p_conn_upd_req_evt_param->conn_intv_min, p_conn_upd_req_evt_param->conn_intv_max, p_conn_upd_req_evt_param->conn_latency,p_conn_upd_req_evt_param->sup_to);


        } break;
        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
        	// RENES_LOG("BLE_GAP_EVENT_DATA_LEN_CHG\n");
        	st_ble_gap_data_len_chg_evt_t *p_gap_data_len_chg_evt_param = (st_ble_gap_data_len_chg_evt_t *)p_data->p_param;
			uint16_t tx_octets = p_gap_data_len_chg_evt_param->tx_octets;
			uint16_t tx_time = p_gap_data_len_chg_evt_param->tx_time;
			uint16_t rx_octets = p_gap_data_len_chg_evt_param->rx_octets;
			uint16_t rx_time = p_gap_data_len_chg_evt_param->rx_time;
//			printf("Data length changed \n\tConnection index: %X\n", g_conn_hdl);
			printf("\tRX_Octets: %d\tTX_Octets: %d\n", rx_octets, tx_octets);
//			printf("\tRX time: %d\tTX time: %d\n\n", rx_time, tx_time);

        } break;
        case BLE_GAP_EVENT_ENC_CHG:
        {
            RENES_LOG("BLE_GAP_EVENT_ENC_CHG");
        } break;
        case BLE_GAP_EVENT_LTK_REQ:
        {
            RENES_LOG("BLE_GAP_EVENT_LTK_REQ");
        } break;
        case BLE_GAP_EVENT_PEER_KEY_INFO:
        {
            RENES_LOG("BLE_GAP_EVENT_PEER_KEY_INFO");
        } break;
        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            RENES_LOG("BLE_GAP_EVENT_PAIRING_COMP");
        } break;
        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            RENES_LOG("BLE_GAP_EVENT_PAIRING_REQ");
        } break;
        case BLE_GAP_EVENT_EX_KEY_REQ:
        {
            RENES_LOG("BLE_GAP_EVENT_EX_KEY_REQ");
        } break;
        case BLE_GAP_EVENT_NUM_COMP_REQ:
        {
            RENES_LOG("BLE_GAP_EVENT_NUM_COMP_REQ");
        } break;
        case BLE_GAP_EVENT_PASSKEY_ENTRY_REQ:
        {
            RENES_LOG("BLE_GAP_EVENT_PASSKEY_ENTRY_REQ");
        } break;
/* Hint: Add cases of GAP event macros defined as BLE_GAP_XXX */
/* Start user code for GAP callback function event process. Do not edit comment generated here */
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

    // RENES_LOG(">>> gatts_cb BLE_GATTS_EVENT type: %u \n", type);
    switch(type)
    {
        case BLE_GATTS_EVENT_WRITE_RSP_COMP:
        {
            RENES_LOG("BLE_GATTS_EVENT_WRITE_RSP_COMP\n");
        } break;
        case BLE_GATTS_EVENT_DB_ACCESS_IND:
        {
            RENES_LOG("BLE_GATTS_EVENT_DB_ACCESS_IND");
        } break;        
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
      case BLE_GATTC_EVENT_CONN_IND:
//      {
//		RENES_LOG("BLE_GATTC_EVENT_CONN_IND \n");
//      } break;
      case BLE_GATTC_EVENT_EX_MTU_RSP: {
        st_ble_gattc_ex_mtu_rsp_evt_t * p_ex_mtu_rsp;
        p_ex_mtu_rsp = (st_ble_gattc_ex_mtu_rsp_evt_t *)p_data->p_param;
        LOG_DEBUG("--> BLE_GATTC_EVENT_EX_MTU_RSP: %d\n", p_ex_mtu_rsp->mtu);


      } break;
      case BLE_GATTC_EVENT_HDL_VAL_IND:
      {

      } break;
      case BLE_GATTC_EVENT_DISCONN_IND:
      {
    	  RENES_LOG(">>> BLE_GATTC_EVENT_DISCONN_IND\n");
      } break;
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
//	fsp_err_t         err     = FSP_SUCCESS;

    R_BLE_SERVS_VsCb(type, result, p_data);
    switch(type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
        	RENES_LOG("BLE_VS_EVENT_GET_ADDR_COMP\n");
            /* Start advertising when BD address is ready */
            st_ble_vs_get_bd_addr_comp_evt_t * get_address = (st_ble_vs_get_bd_addr_comp_evt_t *)p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, get_address->addr.addr, BLE_BD_ADDR_LEN);
//            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);	// org

#if defined (__SUPPORT_MATTER_IOT__)
		    evt_ble_matter.bType = BT_EVT_DEVICE_READY;
		    ble_comm_event(&evt_ble_matter);
#endif	// __SUPPORT_MATTER_IOT__
        } break;

/* Hint: Add cases of vender specific event macros defined as BLE_VS_XXX */
/* Start user code for vender specific callback function event process. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
    }
}

/******************************************************************************
 * Function Name: WiFiProvisionings_cb
 * Description  : Callback function for WiFiProvisioning server feature.
 * Arguments    : uint16_t type -
 *                  Event type of WiFiProvisioning server feature.
 *              : ble_status_t result -
 *                  Event result of WiFiProvisioning server feature.
 *              : st_ble_servs_evt_data_t *p_data -
 *                  Event parameters of WiFiProvisioning server feature.
 * Return Value : none
 ******************************************************************************/
static void WiFiProvisionings_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data) {
    LOG_TRACE("Type: 0x%x, Msg: %s\n", type, msg2string(type));

    if (result == BLE_SUCCESS) {
        switch (type) {
            case BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ: {
                st_ble_wifiprovisionings_wificommand_t *event_data =
                  (st_ble_wifiprovisionings_wificommand_t *) p_data->p_param;

                provisioning_cmd_dispatch(event_data->value);

                LOG_INFO("BLE_WIFIPROVISIONINGS_EVENT_WIFICOMMAND_WRITE_REQ\n\n");
            } break;

            case BLE_WIFIPROVISIONINGS_EVENT_WIFIOUTPUT_READ_REQ: {
                st_ble_wifiprovisionings_wifioutput_t *event_data =
                  (st_ble_wifiprovisionings_wifioutput_t *) p_data->p_param;

                memset(output_buf, 0, sizeof(output_buf));

                if (flag_start == false) {
                    uint32_t total = strlen(wifi_networks);

                    ble_prov_hdr.remain = total;
                    ble_prov_hdr.total  = total;

                    flag_start = true;

                    LOG_INFO("To be sent: %s\n\n", wifi_networks);
                }

                uint32_t to_be_sent = MIN(
                    MIN(ble_prov_hdr.remain, ble_prov_hdr.total),
                    MIN(sizeof(output_buf) - sizeof(ble_prov_hdr), BLE_MAX_PACKET_SIZE)
                );

                uint32_t offset = ble_prov_hdr.total - ble_prov_hdr.remain;
                ble_prov_hdr.remain = ble_prov_hdr.remain - to_be_sent;

                strncpy(output_buf + sizeof(ble_prov_hdr), wifi_networks + offset, to_be_sent);
                memcpy(output_buf, &ble_prov_hdr, sizeof(ble_prov_hdr));

                LOG_INFO("Sent(%d:%d): %s\n", to_be_sent, offset, output_buf + sizeof(ble_prov_hdr));

                st_ble_gatt_value_t value = {
                  .value_len = to_be_sent + sizeof(ble_prov_hdr),
                  .p_value = output_buf,
                };

                uint16_t attr_hdl = BLE_WIFIPROVISIONINGS_WIFIOUTPUT_VAL_HDL;
                int rc = R_BLE_GTL_GATTS_ReadCfm(p_data->conn_hdl, attr_hdl, &value);
                LOG_INFO("ReadCfm RC: 0x%x\n", rc);

                if (ble_prov_hdr.remain == 0) {
                    /* Disconnect / timeout, the flag should be cleared */
                    flag_start = false;
                }
            } break;

            case BLE_WIFIPROVISIONINGS_EVENT_WIFIPROVISONING_READ_REQ: {
                st_ble_wifiprovisionings_wifiprovisoning_t *event_data =
                  (st_ble_wifiprovisionings_wifiprovisoning_t *) p_data->p_param;
                LOG_INFO("BLE_WIFIPROVISIONINGS_EVENT_WIFIPROVISONING_READ_REQ\n\n");
            } break;

            case BLE_MATTER_EVENT_RX_READ_REQ: {
                st_ble_matter_rx_t *event_data =
                  (st_ble_matter_rx_t *) p_data->p_param;
                LOG_INFO("BLE_MATTER_EVENT_RX_READ_REQ\n\n");
            } break;
            case BLE_MATTER_EVENT_RX_WRITE_REQ: {
				st_ble_matter_rx_t *event_data =  (st_ble_matter_rx_t *) p_data->p_param;

#if defined(__SUPPORT_MATTER_IOT__)
				evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_RX;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
				ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
} break;
            case BLE_MATTER_EVENT_RX_WRITE_COMP:
            {
            	st_ble_matter_tx_t *event_data = (st_ble_matter_tx_t *) p_data->p_param;

#if defined(__SUPPORT_MATTER_IOT__)
            	evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_CCCD;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
				ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
            } break;
            case BLE_MATTER_EVENT_TX_WRITE_REQ: {
				st_ble_matter_rx_t *event_data =  (st_ble_matter_rx_t *) p_data->p_param;

                LOG_INFO("BLE_MATTER_EVENT_TX_WRITE_REQ\n\n");

#if defined(__SUPPORT_MATTER_IOT__)
				evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_CCCD;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
				ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
            } break;
            case BLE_MATTER_EVENT_TX_WRITE_COMP: {
				st_ble_matter_rx_t *event_data =  (st_ble_matter_rx_t *) p_data->p_param;

                LOG_INFO("BLE_MATTER_EVENT_TX_WRITE_COMP\n\n");

#if defined(__SUPPORT_MATTER_IOT__)
				evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_RX;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
				ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
			} break;
            case BLE_MATTER_EVENT_TX_CHAR_USER_DESCRIPTION_WRITE_REQ:
            {
            	st_ble_matter_tx_t *event_data = (st_ble_matter_tx_t *) p_data->p_param;

#if defined(__SUPPORT_MATTER_IOT__)
            	evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_CCCD;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
				ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
} break;
            case BLE_MATTER_EVENT_TX_CHAR_USER_DESCRIPTION_WRITE_COMP:
            {
            	st_ble_matter_tx_t *event_data = (st_ble_matter_tx_t *) p_data->p_param;
            	// LOG_INFO("BLE_MATTER_EVENT_TX_CHAR_USER_DESCRIPTION_WRITE_COMP: %s(%u)\n", event_data->value, event_data->len);
#if defined(__SUPPORT_MATTER_IOT__)
            	evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_CCCD;
				evt_ble_matter.gatt.data = (unsigned char *)event_data->value;
				evt_ble_matter.gatt.len = event_data->len;
                ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
            } break;            
            case BLE_MATTER_EVENT_TX_WRITE_HDL_VAL_CNF:
            {
                LOG_INFO("BLE_MATTER_EVENT_TX_WRITE_HDL_VAL_CNF\n\n");
#if defined(__SUPPORT_MATTER_IOT__)
                evt_ble_matter.bType = BT_EVT_DEVICE_WRITE_CONFIRM;
                ble_comm_event(&evt_ble_matter);
#endif //(__SUPPORT_MATTER_IOT__)
            } break;
           
            default: break;
        }
    }
}

#if defined (__SUPPORT_MATTER_IOT__)

/******************************************************************************
 * Function Name: ble_profile_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : none
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
//ble_status_t ble_init(void)	// org
ble_status_t ble_profile_init(void)
{
    ble_status_t status;
    fsp_err_t err;

    LOG_INFO(">>> ble_profile_init\n");

    /* Initialize BLE */
    err = RM_BLE_ABS_Open(&g_ble_abs0_ctrl, &g_ble_abs0_cfg);
    if (FSP_SUCCESS != err)
    {
        LOG_INFO(">>> RM_BLE_ABS_Open fail %u\n", err);
        return err;
    }

    /* Initialize GATT Database */
    status = R_BLE_GATTS_SetDbInst(NULL);
    if (BLE_SUCCESS != status)
    {
        LOG_INFO(">>> R_BLE_GATTS_SetDbInst err\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT server */
    status = R_BLE_SERVS_Init();
    if (BLE_SUCCESS != status)
    {
        LOG_INFO(">>> R_BLE_SERVS_Init err\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /*Initialize GATT client */
    status = R_BLE_SERVC_Init();
    if (BLE_SUCCESS != status)
    {
        LOG_INFO(">>> R_BLE_SERVC_Init err\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Set Prepare Write Queue */
    R_BLE_GATTS_SetPrepareQueue(gs_queue, BLE_GATTS_QUEUE_NUM);  

    /* Initialize WiFiProvisioning server API */
    status = R_BLE_WIFIPROVISIONINGS_Init(WiFiProvisionings_cb);
    if (BLE_SUCCESS != status)
    {
        LOG_INFO(">>> R_BLE_WIFIPROVISIONINGS_Init err\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    return status;
}

/******************************************************************************
 * Function Name: ble_adv_start
 * Description  : BLE advertising start.
 * Arguments    : None
 * Return Value : None
  ******************************************************************************/
void ble_adv_start(void)
{
	fsp_err_t         err     = FSP_SUCCESS;

	// RENES_LOG(">>> ble_adv_start\n");

	g_ble_advertising_parameter.fast_advertising_interval = (uint32_t) ( ble_advertise_matter.interval_min/0.625 );
	g_ble_advertising_parameter.slow_advertising_interval = (uint32_t) ( ble_advertise_matter.interval_max/0.625 );

	g_ble_advertising_parameter.advertising_data_length = ble_advertise_matter.advertise_data_len;
	g_ble_advertising_parameter.p_advertising_data = ble_advertise_matter.advertise_data;

	g_ble_advertising_parameter.scan_response_data_length = ble_advertise_matter.scan_response_data_len;
	g_ble_advertising_parameter.p_scan_response_data = ble_advertise_matter.scan_response_data;

	RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
	if (err != FSP_SUCCESS)
	{
		RENES_LOG("StartLegacyAdvertising fail %lu\n", err);
	}
}

/******************************************************************************
 * Function Name: ble_adv_stop
 * Description  : BLE advertising stop.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void ble_adv_stop(void)
{
	// RENES_LOG(">>> ble_adv_stop\n");
	R_BLE_GAP_StopAdv(g_advertising_handle);
}

/******************************************************************************
 * Function Name: ble_close_connection
 * Description  : BLE close connection.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void ble_close_connection(void)
{
	// RENES_LOG(">>> ble_close_connection\n");
	/* Close BLE driver */
	RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 * Function Name: ble_gattc_indicate_send
 * Description  : BLE change value indicate send.
 * Arguments    : 	char * send_data
 * 					char data_len
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
//uint8_t ble_gattc_indicate_send(uint8_t * send_data, uint8_t data_len)
unsigned char ble_gattc_indicate_send(unsigned char *send_data, unsigned short data_len)
{
	uint8_t *data_value = NULL;

	data_value = (uint8_t *)pvPortMalloc(data_len + 1);
	if (data_value == NULL)
	{
		RENES_LOG("Malloc fail\n");
		return BLE_ERR_MEM_ALLOC_FAILED;
	}

	memset(data_value, 0, data_len + 1);
	memcpy(data_value, send_data, data_len);

	// RENES_LOG(">>> [ble_gattc_indicate_send] g_conn_hdl[%d], data[%d]: %s\n", g_conn_hdl, data_len, data_value);
	// RENES_LOG(">>> [ble_gattc_indicate_send] data: [0x%x][0x%x][0x%x][0x%x][0x%x][0x%x][%d]\n"
    //     , data_value[0], data_value[1], data_value[2], data_value[3], data_value[4], data_value[5], data_len);
	R_BLE_MATTER_TX_Indicate(g_conn_hdl, (const void*) data_value, data_len);

	vPortFree(data_value);

	return BLE_SUCCESS;
}
#endif	// __SUPPORT_MATTER_IOT__

//static int reboot(const prov_cmd_reboot_t* params) {	// org
static int reboot_by_ble(const prov_cmd_reboot_t* params) {
    return R_BSP_WarmStart(BSP_WARM_START_RESET);
}
//static int wifi_scan(const prov_cmd_scan_t* params, prov_scanned_networks_t** networks) {	// org
static int wifi_scan_by_ble(const prov_cmd_scan_t* params, prov_scanned_networks_t** networks) {
#define MAX_WIFI_SCAN_RESULTS 20

    LOG_TRACE("\n");

    static WIFIScanResult_t scan_data[MAX_WIFI_SCAN_RESULTS];
    memset(scan_data, 0, sizeof(scan_data));

    WIFIReturnCode_t rc = WIFI_Scan(&scan_data[0], MAX_WIFI_SCAN_RESULTS);
    if (rc == eWiFiSuccess) {
        uint32_t num_of_found_networks = 0;

        for (int i = 0; i < MAX_WIFI_SCAN_RESULTS; i++) {
            if (scan_data[i].ucSSIDLength != 0) {
                num_of_found_networks++;
            } else {
                break;
            }
        }

        prov_scanned_networks_t* networks_db = malloc(
            sizeof(prov_scanned_networks_t) + (num_of_found_networks * sizeof(prov_ap_entry_t)));
        assert(networks_db);

        networks_db->num_of_networks = 0;

        LOG_INFO("Found %d networks:\n", num_of_found_networks);
        for (int i = 0, j = 0; i < num_of_found_networks; i++) {

            if (scan_data[i].xSecurity >= eWiFiSecurityNotSupported) {
                LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %s [SKIP]\n",
                    scan_data[i].ucSSIDLength, scan_data[i].ucSSID, scan_data[i].cRSSI,
                    scan_data[i].ucChannel, WiFiSecType2string(scan_data[i].xSecurity));
                continue;
            }

            prov_ap_entry_t entry = {
                .rssi          = scan_data[i].cRSSI,
                .channel       = scan_data[i].ucChannel,
                .security_type = scan_data[i].xSecurity,
            };

            strncpy(entry.ssid, scan_data[i].ucSSID,
                MAX(sizeof(entry.ssid), scan_data[i].ucSSIDLength));

            networks_db->networks[j++] = entry;
            networks_db->num_of_networks++;

            LOG_INFO("SSID: %20.*s, RSSI: %4d, Channel: %4u, Security: %s\n",
                scan_data[i].ucSSIDLength, scan_data[i].ucSSID, scan_data[i].cRSSI,
                scan_data[i].ucChannel, WiFiSecType2string(scan_data[i].xSecurity));
        }
        LOG_INFO("----------------------\n\n");

        *networks = networks_db;
    } else {
        LOG_ERROR("Unable to scan WiFi networks: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;

#undef MAX_WIFI_SCAN_RESULTS
}

static void wifi_network_params_print(const WIFINetworkParams_t* params) {
    LOG_INFO("WIFINetworkParams_t: {\n"
             "  - ucSSID:           %.*s\n"
             "  - ucSSIDLength:     %d\n"
             "  - cPassphrase:      %.*s\n"
             "  - ucLength:         %d\n"
             "  - xSecurity:        %s\n"
             "  - ucDefWEPKeyIndex: %d\n"
             "  - ucChannel:        %d\n"
             "}\n", sizeof(params->ucSSID), params->ucSSID, params->ucSSIDLength,
                    sizeof(params->xPassword.xWPA.cPassphrase),
                    params->xPassword.xWPA.cPassphrase, params->xPassword.xWPA.ucLength,
                    WiFiSecType2string(params->xSecurity), params->ucDefaultWEPKeyIndex, params->ucChannel
    );
}

//static int wifi_connect(const prov_cmd_select_ap_t* params) {	// org
static int wifi_connect_by_ble(const prov_cmd_select_ap_t* params) {
    LOG_TRACE("\n");

    WIFINetworkParams_t net_params = {
        .ucChannel = 4,
        .xSecurity = (WIFISecurity_t) params->security_type,
    };

    uint32_t ssid_len = strnlen(params->ssid, sizeof(params->ssid));
    uint32_t pass_len = strnlen(params->password, sizeof(params->password));

    net_params.ucSSID[0] = '\'';
    net_params.xPassword.xWPA.cPassphrase[0] = '\'';

    strncpy(&net_params.ucSSID[1], params->ssid, sizeof(net_params.ucSSID) - 1);
    strncpy(&net_params.xPassword.xWPA.cPassphrase[1], params->password,
        sizeof(net_params.xPassword.xWPA.cPassphrase) - 1);

    uint32_t ssid_len_1 = strnlen(net_params.ucSSID, sizeof(net_params.ucSSID) - 2);
    uint32_t pass_len_1 = strnlen(net_params.xPassword.xWPA.cPassphrase,
        sizeof(net_params.xPassword.xWPA.cPassphrase) - 2);

    net_params.ucSSID[ssid_len_1] = '\'';
    net_params.xPassword.xWPA.cPassphrase[pass_len_1] = '\'';

    net_params.ucSSIDLength = ssid_len;
    net_params.xPassword.xWPA.ucLength = pass_len;

    wifi_network_params_print(&net_params);

    WIFIReturnCode_t rc = WIFI_ConnectAP(&net_params);
    if (rc != eWiFiSuccess) {
        LOG_ERROR("Unable to connect to WiFi network: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;
}
//static int wifi_disconnect(const prov_cmd_disconnect_t* params) {		// org
static int wifi_disconnect_by_ble(const prov_cmd_disconnect_t* params) {
    LOG_TRACE("\n");

    WIFIReturnCode_t rc = WIFI_Disconnect();
    if (rc != eWiFiSuccess) {
        LOG_ERROR("Unable to disconnect from WiFi network: %s (%d)\n", WiFiErrCode2string(rc), rc);
    }

    return rc;
}

//static int wifi_ping(const prov_cmd_network_info_t* params) {	// org
static int wifi_ping_by_ble(const prov_cmd_network_info_t* params) {
    char ip_buff[20] = {};

    LOG_TRACE("Ping %s...\n", ip_to_string(params->ping_ip, ip_buff, sizeof(ip_buff)));

    uint8_t ip_addr[4] = {0, 0, 0, 0};
    memcpy(ip_addr, &params->ping_ip, sizeof(ip_addr));

    WIFIReturnCode_t rc = WIFI_Ping(ip_addr, 3, 500);
    if (rc != eWiFiSuccess) {

        LOG_ERROR("Unable to ping %s: %s (%d)\n",
            ip_to_string(params->ping_ip, ip_buff, sizeof(ip_buff)), WiFiErrCode2string(rc), rc);
    }

    return rc;
}

/******************************************************************************
 * Function Name: app_main
 * Description  : Application main function with main loop
 * Arguments    : none
 * Return Value : none
 ******************************************************************************/
//void app_main(void)	// org
void ble_app_main(void * arg)
{
	FSP_PARAMETER_NOT_USED(arg);

#if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
    /* Create Event Group */
    g_ble_event_group_handle = xEventGroupCreate();
    assert(g_ble_event_group_handle);
#endif

    log_level_set(BLE_LOG_LEVEL);

#if BLE_RF_FORCE_EN
    /* Set RF switch to BLE */
    CRG_TOP->CLK_AMBA_REG_b.OQSPI_GPIO_MODE = 1;
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_00, BSP_IO_LEVEL_LOW);
    R_BSP_PinWrite(BSP_IO_PORT_01_PIN_01, BSP_IO_LEVEL_HIGH);
#endif

    LOG_INFO("Provisioning application is starting...\n");

    provisioning_cb_t prov_params = {
      .reboot = reboot_by_ble,
      .wifi_scan = wifi_scan_by_ble,
      .wifi_connect = wifi_connect_by_ble,
      .wifi_disconnect = wifi_disconnect_by_ble,
      .wifi_ping = wifi_ping_by_ble,
    };

    provisioning_init(&prov_params);

    /* Initialize BLE and profiles call it  from rnDeviceWrapAPIs for Matter*/
//    ble_status_t rc = ble_init();
    ble_status_t rc = ble_profile_init();
    if (rc != FSP_SUCCESS) {
        LOG_ERROR("Func: %s, line: %d, rc: %u\n", __FUNCTION__, __LINE__, rc);
        //assert(0);
    }

    LOG_INFO("Please run 'provisioning.py' script\n");

/* Hint: Input process that should be done before main loop such as calling initial function or variable definitions */
/* Start user code for process before main loop. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

    /* main loop */
    while (1)
    {
        /* Process BLE Event */
        R_BLE_Execute();

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
/* End user code. Do not edit comment generated here */

    /* Terminate BLE */
    RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/******************************************************************************
 User function definitions
*******************************************************************************/
/* Start user code for function definitions. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/******************************************************************************
 * Function Name: app_start_ble_main
 * Description  : Create BLE main thread.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void app_start_ble_main(void)
{
	 uint8_t status = BLE_SUCCESS;

	 if (ble_app_main_hdler)
	 {
		 vTaskDelete(ble_app_main_hdler);
		 ble_app_main_hdler = NULL;
	 }

	 status = xTaskCreate(ble_app_main,
			 "BLE_AppMain_Threda",
			 8912/4,			// stack
			 NULL,
			 1,					// priority
			 &ble_app_main_hdler);

	 if (status != pdPASS)
	{
		LOG_INFO("[app_start_ble_main] ERROR(0x%0x)\n", status);
		if (ble_app_main_hdler)
		{
			vPortFree(ble_app_main_hdler);
		}
	}
}

void app_stop_ble_main(void)
{
    if (ble_app_main_hdler)
    {
        vTaskDelete(ble_app_main_hdler);
        ble_app_main_hdler = NULL;
    }
}
