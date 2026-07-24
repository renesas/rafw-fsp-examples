/***********************************************************************************************************************
 * File Name    : utils_enum.h
 * Description  : For internal use, Contains macros, data structures and functions used  for testing
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef UTILS_ENUM_H_
#define UTILS_ENUM_H_

const char * BLE_STATUS_T(ble_status_t status);
const char * E_BLE_GAP_EVT_T(e_ble_gap_evt_t status);
const char * E_R_BLE_GATTC_EV_T(e_r_ble_gattc_evt_t status);
const char * E_R_BLE_GATTS_EVT_T(e_r_ble_gatts_evt_t status);
const char * E_R_BLE_VS_EVT_T(e_r_ble_vs_evt_t status);

const char * E_BLE_GAP_EVT_T (e_ble_gap_evt_t status)
{
    switch (status)
    {
        case BLE_GAP_EVENT_INVALID:
        {
            return "BLE_GAP_EVENT_INVALID";
        }

        case BLE_GAP_EVENT_STACK_ON:
        {
            return "BLE_GAP_EVENT_STACK_ON";
        }

        case BLE_GAP_EVENT_STACK_OFF:
        {
            return "BLE_GAP_EVENT_STACK_OFF";
        }

        case BLE_GAP_EVENT_LOC_VER_INFO:
        {
            return "BLE_GAP_EVENT_LOC_VER_INFO";
        }

        case BLE_GAP_EVENT_HW_ERR:
        {
            return "BLE_GAP_EVENT_HW_ERR";
        }

        case BLE_GAP_EVENT_CMD_ERR:
        {
            return "BLE_GAP_EVENT_CMD_ERR";
        }

        case BLE_GAP_EVENT_ADV_REPT_IND:
        {
            return "BLE_GAP_EVENT_ADV_REPT_IND";
        }

        case BLE_GAP_EVENT_ADV_PARAM_SET_COMP:
        {
            return "BLE_GAP_EVENT_ADV_PARAM_SET_COMP";
        }

        case BLE_GAP_EVENT_ADV_DATA_UPD_COMP:
        {
            return "BLE_GAP_EVENT_ADV_DATA_UPD_COMP";
        }

        case BLE_GAP_EVENT_ADV_ON:
        {
            return "BLE_GAP_EVENT_ADV_ON";
        }

        case BLE_GAP_EVENT_ADV_OFF:
        {
            return "BLE_GAP_EVENT_ADV_OFF";
        }

        case BLE_GAP_EVENT_PERD_ADV_PARAM_SET_COMP:
        {
            return "BLE_GAP_EVENT_PERD_ADV_PARAM_SET_COMP";
        }

        case BLE_GAP_EVENT_PERD_ADV_ON:
        {
            return "BLE_GAP_EVENT_PERD_ADV_ON";
        }

        case BLE_GAP_EVENT_PERD_ADV_OFF:
        {
            return "BLE_GAP_EVENT_PERD_ADV_OFF";
        }

        case BLE_GAP_EVENT_ADV_SET_REMOVE_COMP:
        {
            return "BLE_GAP_EVENT_ADV_SET_REMOVE_COMP";
        }

        case BLE_GAP_EVENT_SCAN_ON:
        {
            return "BLE_GAP_EVENT_SCAN_ON";
        }

        case BLE_GAP_EVENT_SCAN_OFF:
        {
            return "BLE_GAP_EVENT_SCAN_OFF";
        }

        case BLE_GAP_EVENT_SCAN_TO:
        {
            return "BLE_GAP_EVENT_SCAN_TO";
        }

        case BLE_GAP_EVENT_CREATE_CONN_COMP:
        {
            return "BLE_GAP_EVENT_CREATE_CONN_COMP";
        }

        case BLE_GAP_EVENT_CONN_IND:
        {
            return "BLE_GAP_EVENT_CONN_IND";
        }

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            return "BLE_GAP_EVENT_DISCONN_IND";
        }

        case BLE_GAP_EVENT_CONN_CANCEL_COMP:
        {
            return "BLE_GAP_EVENT_CONN_CANCEL_COMP";
        }

        case BLE_GAP_EVENT_WHITE_LIST_CONF_COMP:
        {
            return "BLE_GAP_EVENT_WHITE_LIST_CONF_COMP";
        }

        case BLE_GAP_EVENT_RAND_ADDR_SET_COMP:
        {
            return "BLE_GAP_EVENT_RAND_ADDR_SET_COMP";
        }

        case BLE_GAP_EVENT_CH_MAP_RD_COMP:
        {
            return "BLE_GAP_EVENT_CH_MAP_RD_COMP";
        }

        case BLE_GAP_EVENT_CH_MAP_SET_COMP:
        {
            return "BLE_GAP_EVENT_CH_MAP_SET_COMP";
        }

        case BLE_GAP_EVENT_RSSI_RD_COMP:
        {
            return "BLE_GAP_EVENT_RSSI_RD_COMP";
        }

        case BLE_GAP_EVENT_GET_REM_DEV_INFO:
        {
            return "BLE_GAP_EVENT_GET_REM_DEV_INFO";
        }

        case BLE_GAP_EVENT_CONN_PARAM_UPD_COMP:
        {
            return "BLE_GAP_EVENT_CONN_PARAM_UPD_COMP";
        }

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            return "BLE_GAP_EVENT_CONN_PARAM_UPD_REQ";
        }

        case BLE_GAP_EVENT_AUTH_PL_TO_EXPIRED:
        {
            return "BLE_GAP_EVENT_AUTH_PL_TO_EXPIRED";
        }

        case BLE_GAP_EVENT_SET_DATA_LEN_COMP:
        {
            return "BLE_GAP_EVENT_SET_DATA_LEN_COMP";
        }

        case BLE_GAP_EVENT_DATA_LEN_CHG:
        {
            return "BLE_GAP_EVENT_DATA_LEN_CHG";
        }

        case BLE_GAP_EVENT_RSLV_LIST_CONF_COMP:
        {
            return "BLE_GAP_EVENT_RSLV_LIST_CONF_COMP";
        }

        case BLE_GAP_EVENT_RPA_EN_COMP:
        {
            return "BLE_GAP_EVENT_RPA_EN_COMP";
        }

        case BLE_GAP_EVENT_SET_RPA_TO_COMP:
        {
            return "BLE_GAP_EVENT_SET_RPA_TO_COMP";
        }

        case BLE_GAP_EVENT_RD_RPA_COMP:
        {
            return "BLE_GAP_EVENT_RD_RPA_COMP";
        }

        case BLE_GAP_EVENT_PHY_UPD:
        {
            return "BLE_GAP_EVENT_PHY_UPD";
        }

        case BLE_GAP_EVENT_PHY_SET_COMP:
        {
            return "BLE_GAP_EVENT_PHY_SET_COMP";
        }

        case BLE_GAP_EVENT_DEF_PHY_SET_COMP:
        {
            return "BLE_GAP_EVENT_DEF_PHY_SET_COMP";
        }

        case BLE_GAP_EVENT_PHY_RD_COMP:
        {
            return "BLE_GAP_EVENT_PHY_RD_COMP";
        }

        case BLE_GAP_EVENT_SCAN_REQ_RECV:
        {
            return "BLE_GAP_EVENT_SCAN_REQ_RECV";
        }

        case BLE_GAP_EVENT_CREATE_SYNC_COMP:
        {
            return "BLE_GAP_EVENT_CREATE_SYNC_COMP";
        }

        case BLE_GAP_EVENT_SYNC_EST:
        {
            return "BLE_GAP_EVENT_SYNC_EST";
        }

        case BLE_GAP_EVENT_SYNC_TERM:
        {
            return "BLE_GAP_EVENT_SYNC_TERM";
        }

        case BLE_GAP_EVENT_SYNC_LOST:
        {
            return "BLE_GAP_EVENT_SYNC_LOST";
        }

        case BLE_GAP_EVENT_SYNC_CREATE_CANCEL_COMP:
        {
            return "BLE_GAP_EVENT_SYNC_CREATE_CANCEL_COMP";
        }

        case BLE_GAP_EVENT_PERD_LIST_CONF_COMP:
        {
            return "BLE_GAP_EVENT_PERD_LIST_CONF_COMP";
        }

        case BLE_GAP_EVENT_PRIV_MODE_SET_COMP:
        {
            return "BLE_GAP_EVENT_PRIV_MODE_SET_COMP";
        }

        case BLE_GAP_EVENT_PAIRING_REQ:
        {
            return "BLE_GAP_EVENT_PAIRING_REQ";
        }

        case BLE_GAP_EVENT_PASSKEY_ENTRY_REQ:
        {
            return "BLE_GAP_EVENT_PASSKEY_ENTRY_REQ";
        }

        case BLE_GAP_EVENT_PASSKEY_DISPLAY_REQ:
        {
            return "BLE_GAP_EVENT_PASSKEY_DISPLAY_REQ";
        }

        case BLE_GAP_EVENT_NUM_COMP_REQ:
        {
            return "BLE_GAP_EVENT_NUM_COMP_REQ";
        }

        case BLE_GAP_EVENT_KEY_PRESS_NTF:
        {
            return "BLE_GAP_EVENT_KEY_PRESS_NTF";
        }

        case BLE_GAP_EVENT_PAIRING_COMP:
        {
            return "BLE_GAP_EVENT_PAIRING_COMP";
        }

        case BLE_GAP_EVENT_ENC_CHG:
        {
            return "BLE_GAP_EVENT_ENC_CHG";
        }

        case BLE_GAP_EVENT_PEER_KEY_INFO:
        {
            return "BLE_GAP_EVENT_PEER_KEY_INFO";
        }

        case BLE_GAP_EVENT_EX_KEY_REQ:
        {
            return "BLE_GAP_EVENT_EX_KEY_REQ";
        }

        case BLE_GAP_EVENT_LTK_REQ:
        {
            return "BLE_GAP_EVENT_LTK_REQ";
        }

        case BLE_GAP_EVENT_LTK_RSP_COMP:
        {
            return "BLE_GAP_EVENT_LTK_RSP_COMP";
        }

        case BLE_GAP_EVENT_SC_OOB_CREATE_COMP:
        {
            return "BLE_GAP_EVENT_SC_OOB_CREATE_COMP";
        }

        case BLE_GAP_EVENT_CTE_CONN_REQ_FAILED:
        {
            return "BLE_GAP_EVENT_CTE_CONN_REQ_FAILED";
        }

        case BLE_GAP_EVENT_CTE_CONNLESS_REPT:
        {
            return "BLE_GAP_EVENT_CTE_CONNLESS_REPT";
        }

        case BLE_GAP_EVENT_CTE_CONN_REPT:
        {
            return "BLE_GAP_EVENT_CTE_CONN_REPT";
        }

        case BLE_GAP_EVENT_SUBRATE_CHG:
        {
            return "BLE_GAP_EVENT_SUBRATE_CHG";
        }

        case BLE_GAP_EVENT_PAST_RECV:
        {
            return "BLE_GAP_EVENT_PAST_RECV";
        }

        case BLE_GAP_EVENT_TX_POWER_REPT:
        {
            return "BLE_GAP_EVENT_TX_POWER_REPT";
        }

        case BLE_GAP_EVENT_PATH_LOSS_THR:
        {
            return "BLE_GAP_EVENT_PATH_LOSS_THR";
        }

        case BLE_GAP_EVENT_REQ_PEER_SCA_COMP:
        {
            return "BLE_GAP_EVENT_REQ_PEER_SCA_COMP";
        }

        case BLE_GAP_EVENT_CTE_SET_CONNLESS_PARAM_COMP:
        {
            return "BLE_GAP_EVENT_CTE_SET_CONNLESS_PARAM_COMP";
        }

        case BLE_GAP_EVENT_CTE_CONNLESS_TX_ON:
        {
            return "BLE_GAP_EVENT_CTE_CONNLESS_TX_ON";
        }

        case BLE_GAP_EVENT_CTE_CONNLESS_TX_OFF:
        {
            return "BLE_GAP_EVENT_CTE_CONNLESS_TX_OFF";
        }

        case BLE_GAP_EVENT_CTE_CONNLESS_RX_ON:
        {
            return "BLE_GAP_EVENT_CTE_CONNLESS_RX_ON";
        }

        case BLE_GAP_EVENT_CTE_CONNLESS_RX_OFF:
        {
            return "BLE_GAP_EVENT_CTE_CONNLESS_RX_OFF";
        }

        case BLE_GAP_EVENT_CTE_SET_CONN_PARAM_COMP:
        {
            return "BLE_GAP_EVENT_CTE_SET_CONN_PARAM_COMP";
        }

        case BLE_GAP_EVENT_CTE_SET_CONN_RSP_ON:
        {
            return "BLE_GAP_EVENT_CTE_SET_CONN_RSP_ON";
        }

        case BLE_GAP_EVENT_CTE_SET_CONN_RSP_OFF:
        {
            return "BLE_GAP_EVENT_CTE_SET_CONN_RSP_OFF";
        }

        case BLE_GAP_EVENT_CTE_CONN_REQ_ON:
        {
            return "BLE_GAP_EVENT_CTE_CONN_REQ_ON";
        }

        case BLE_GAP_EVENT_CTE_CONN_REQ_OFF:
        {
            return "BLE_GAP_EVENT_CTE_CONN_REQ_OFF";
        }

        case BLE_GAP_EVENT_SET_DEF_SUBRATE_COMP:
        {
            return "BLE_GAP_EVENT_SET_DEF_SUBRATE_COMP";
        }

        case BLE_GAP_EVENT_REQ_SUBRATE_COMP:
        {
            return "BLE_GAP_EVENT_REQ_SUBRATE_COMP";
        }

        case BLE_GAP_EVENT_PAST_START_COMP:
        {
            return "BLE_GAP_EVENT_PAST_START_COMP";
        }

        case BLE_GAP_EVENT_PAST_SET_PARAM_COMP:
        {
            return "BLE_GAP_EVENT_PAST_SET_PARAM_COMP";
        }

        case BLE_GAP_EVENT_PAST_SET_DEF_PARAM_COMP:
        {
            return "BLE_GAP_EVENT_PAST_SET_DEF_PARAM_COMP";
        }

        case BLE_GAP_EVENT_UPD_SCA_COMP:
        {
            return "BLE_GAP_EVENT_UPD_SCA_COMP";
        }

        case BLE_GAP_EVENT_READ_REMOTE_TX_POWER_COMP:
        {
            return "BLE_GAP_EVENT_READ_REMOTE_TX_POWER_COMP";
        }

        case BLE_GAP_EVENT_SET_PATHLOSS_REPT_PARAM_COMP:
        {
            return "BLE_GAP_EVENT_SET_PATHLOSS_REPT_PARAM_COMP";
        }

        case BLE_GAP_EVENT_PATHLOSS_REPT_ON:
        {
            return "BLE_GAP_EVENT_PATHLOSS_REPT_ON";
        }

        case BLE_GAP_EVENT_PATHLOSS_REPT_OFF:
        {
            return "BLE_GAP_EVENT_PATHLOSS_REPT_OFF";
        }

        case BLE_GAP_EVENT_LOCAL_TX_POWER_REPT_ON:
        {
            return "BLE_GAP_EVENT_LOCAL_TX_POWER_REPT_ON";
        }

        case BLE_GAP_EVENT_LOCAL_TX_POWER_REPT_OFF:
        {
            return "BLE_GAP_EVENT_LOCAL_TX_POWER_REPT_OFF";
        }

        case BLE_GAP_EVENT_REMOTE_TX_POWER_REPT_ON:
        {
            return "BLE_GAP_EVENT_REMOTE_TX_POWER_REPT_ON";
        }

        case BLE_GAP_EVENT_REMOTE_TX_POWER_REPT_OFF:
        {
            return "BLE_GAP_EVENT_REMOTE_TX_POWER_REPT_OFF";
        }

        case BLE_GAP_EVENT_SET_RPA_UPD_REASON_COMP:
        {
            return "BLE_GAP_EVENT_SET_RPA_UPD_REASON_COMP";
        }

        case BLE_GAP_EVENT_DTM_RX_TEST_COMP:
        {
            return "BLE_GAP_EVENT_DTM_RX_TEST_COMP";
        }

        case BLE_GAP_EVENT_DTM_TX_TEST_COMP:
        {
            return "BLE_GAP_EVENT_DTM_TX_TEST_COMP";
        }

        case BLE_GAP_EVENT_DTM_TEST_END_COMP:
        {
            return "BLE_GAP_EVENT_DTM_TEST_END_COMP";
        }

        case BLE_GAP_EVENT_ENHANCED_READ_TX_POWER_LEVEL_COMP:
        {
            return "BLE_GAP_EVENT_ENHANCED_READ_TX_POWER_LEVEL_COMP";
        }

        case BLE_GAP_EVENT_SET_HOST_FEAT_COMP:
        {
            return "BLE_GAP_EVENT_SET_HOST_FEAT_COMP";
        }

        default:

            return "UKNOWN";
    }
}

const char * BLE_STATUS_T (ble_status_t status)
{
    switch (status)
    {
        case BLE_SUCCESS:
        {
            return "BLE_SUCCESS";
        }

        case BLE_ERR_INVALID_PTR:
        {
            return "BLE_ERR_INVALID_PTR";
        }

        case BLE_ERR_INVALID_DATA:
        {
            return "BLE_ERR_INVALID_DATA";
        }

        case BLE_ERR_INVALID_ARG:
        {
            return "BLE_ERR_INVALID_ARG";
        }

        case BLE_ERR_INVALID_FUNC:
        {
            return "BLE_ERR_INVALID_FUNC";
        }

        case BLE_ERR_INVALID_CHAN:
        {
            return "BLE_ERR_INVALID_CHAN";
        }

        case BLE_ERR_INVALID_MODE:
        {
            return "BLE_ERR_INVALID_MODE";
        }

        case BLE_ERR_UNSUPPORTED:
        {
            return "BLE_ERR_UNSUPPORTED";
        }

        case BLE_ERR_INVALID_STATE:
        {
            return "BLE_ERR_INVALID_STATE";
        }

        case BLE_ERR_INVALID_OPERATION:
        {
            return "BLE_ERR_INVALID_OPERATION";
        }

        case BLE_ERR_ALREADY_IN_PROGRESS:
        {
            return "BLE_ERR_ALREADY_IN_PROGRESS";
        }

        case BLE_ERR_CONTEXT_FULL:
        {
            return "BLE_ERR_CONTEXT_FULL";
        }

        case BLE_ERR_MEM_ALLOC_FAILED:
        {
            return "BLE_ERR_MEM_ALLOC_FAILED";
        }

        case BLE_ERR_NOT_FOUND:
        {
            return "BLE_ERR_NOT_FOUND";
        }

        case BLE_ERR_INVALID_HDL:
        {
            return "BLE_ERR_INVALID_HDL";
        }

        case BLE_ERR_DISCONNECTED:
        {
            return "BLE_ERR_DISCONNECTED";
        }

        case BLE_ERR_LIMIT_EXCEEDED:
        {
            return "BLE_ERR_LIMIT_EXCEEDED";
        }

        case BLE_ERR_RSP_TIMEOUT:
        {
            return "BLE_ERR_RSP_TIMEOUT";
        }

        case BLE_ERR_NOT_YET_READY:
        {
            return "BLE_ERR_NOT_YET_READY";
        }

        case BLE_ERR_UNSPECIFIED:
        {
            return "BLE_ERR_UNSPECIFIED";
        }

        case BLE_ERR_ALREADY_INITIALIZED:
        {
            return "BLE_ERR_ALREADY_INITIALIZED";
        }

        case BLE_ERR_HC_UNKNOWN_HCI_CMD:
        {
            return "BLE_ERR_HC_UNKNOWN_HCI_CMD";
        }

        case BLE_ERR_HC_NO_CONN:
        {
            return "BLE_ERR_HC_NO_CONN";
        }

        case BLE_ERR_HC_HW_FAIL:
        {
            return "BLE_ERR_HC_HW_FAIL";
        }

        case BLE_ERR_HC_PAGE_TO:
        {
            return "BLE_ERR_HC_PAGE_TO";
        }

        case BLE_ERR_HC_AUTH_FAIL:
        {
            return "BLE_ERR_HC_AUTH_FAIL";
        }

        case BLE_ERR_HC_KEY_MISSING:
        {
            return "BLE_ERR_HC_KEY_MISSING";
        }

        case BLE_ERR_HC_MEM_FULL:
        {
            return "BLE_ERR_HC_MEM_FULL";
        }

        case BLE_ERR_HC_CONN_TO:
        {
            return "BLE_ERR_HC_CONN_TO";
        }

        case BLE_ERR_HC_MAX_NUM_OF_CONN:
        {
            return "BLE_ERR_HC_MAX_NUM_OF_CONN";
        }

        case BLE_ERR_HC_MAX_NUM_OF_SCO_CONN:
        {
            return "BLE_ERR_HC_MAX_NUM_OF_SCO_CONN";
        }

        case BLE_ERR_HC_ACL_CONN_ALREADY_EXISTS:
        {
            return "BLE_ERR_HC_ACL_CONN_ALREADY_EXISTS";
        }

        case BLE_ERR_HC_CMD_DISALLOWED:
        {
            return "BLE_ERR_HC_CMD_DISALLOWED";
        }

        case BLE_ERR_HC_HOST_REJ_LIMITED_RESRC:
        {
            return "BLE_ERR_HC_HOST_REJ_LIMITED_RESRC";
        }

        case BLE_ERR_HC_HOST_REJ_SEC_REASONS:
        {
            return "BLE_ERR_HC_HOST_REJ_SEC_REASONS";
        }

        case BLE_ERR_HC_HOST_REJ_PERSONAL_DEV:
        {
            return "BLE_ERR_HC_HOST_REJ_PERSONAL_DEV";
        }

        case BLE_ERR_HC_HOST_TO:
        {
            return "BLE_ERR_HC_HOST_TO";
        }

        case BLE_ERR_HC_UNSPRT_FEAT_OR_PARAM:
        {
            return "BLE_ERR_HC_UNSPRT_FEAT_OR_PARAM";
        }

        case BLE_ERR_HC_INVALID_HCI_CMD_PARAM:
        {
            return "BLE_ERR_HC_INVALID_HCI_CMD_PARAM";
        }

        case BLE_ERR_HC_OTHER_END_TERM_USER:
        {
            return "BLE_ERR_HC_OTHER_END_TERM_USER";
        }

        case BLE_ERR_HC_OTHER_END_TERM_LOW_RESRC:
        {
            return "BLE_ERR_HC_OTHER_END_TERM_LOW_RESRC";
        }

        case BLE_ERR_HC_OTHER_END_TERM_PW_OFF:
        {
            return "BLE_ERR_HC_OTHER_END_TERM_PW_OFF";
        }

        case BLE_ERR_HC_CONN_TERM_BY_LOCAL_HOST:
        {
            return "BLE_ERR_HC_CONN_TERM_BY_LOCAL_HOST";
        }

        case BLE_ERR_HC_REPEATED_ATTEMPTS:
        {
            return "BLE_ERR_HC_REPEATED_ATTEMPTS";
        }

        case BLE_ERR_HC_PAIRING_NOT_ALLOWED:
        {
            return "BLE_ERR_HC_PAIRING_NOT_ALLOWED";
        }

        case BLE_ERR_HC_UNKNOWN_LMP_PDU:
        {
            return "BLE_ERR_HC_UNKNOWN_LMP_PDU";
        }

        case BLE_ERR_HC_UNSPRT_REM_FEAT:
        {
            return "BLE_ERR_HC_UNSPRT_REM_FEAT";
        }

        case BLE_ERR_HC_SCO_OFFSET_REJ:
        {
            return "BLE_ERR_HC_SCO_OFFSET_REJ";
        }

        case BLE_ERR_HC_SCO_INTERVAL_REJ:
        {
            return "BLE_ERR_HC_SCO_INTERVAL_REJ";
        }

        case BLE_ERR_HC_SCO_AIR_MODE_REJ:
        {
            return "BLE_ERR_HC_SCO_AIR_MODE_REJ";
        }

        case BLE_ERR_HC_INVALID_LMP_PARAM:
        {
            return "BLE_ERR_HC_INVALID_LMP_PARAM";
        }

        case BLE_ERR_HC_UNSPECIFIED_ERR:
        {
            return "BLE_ERR_HC_UNSPECIFIED_ERR";
        }

        case BLE_ERR_HC_UNSPRT_LMP_PARAM_VAL:
        {
            return "BLE_ERR_HC_UNSPRT_LMP_PARAM_VAL";
        }

        case BLE_ERR_HC_ROLE_CHANGE_NOT_ALLOWED:
        {
            return "BLE_ERR_HC_ROLE_CHANGE_NOT_ALLOWED";
        }

        case BLE_ERR_HC_LMP_RSP_TO:
        {
            return "BLE_ERR_HC_LMP_RSP_TO";
        }

        case BLE_ERR_HC_LMP_ERR_TX_COLLISION:
        {
            return "BLE_ERR_HC_LMP_ERR_TX_COLLISION";
        }

        case BLE_ERR_HC_LMP_PDU_NOT_ALLOWED:
        {
            return "BLE_ERR_HC_LMP_PDU_NOT_ALLOWED";
        }

        case BLE_ERR_HC_ENC_MODE_NOT_ACCEPTABLE:
        {
            return "BLE_ERR_HC_ENC_MODE_NOT_ACCEPTABLE";
        }

        case BLE_ERR_HC_UNIT_KEY_USED:
        {
            return "BLE_ERR_HC_UNIT_KEY_USED";
        }

        case BLE_ERR_HC_QOS_IS_NOT_SPRT:
        {
            return "BLE_ERR_HC_QOS_IS_NOT_SPRT";
        }

        case BLE_ERR_HC_INSTANT_PASSED:
        {
            return "BLE_ERR_HC_INSTANT_PASSED";
        }

        case BLE_ERR_HC_PAIRING_UNIT_KEY_NOT_SPRT:
        {
            return "BLE_ERR_HC_PAIRING_UNIT_KEY_NOT_SPRT";
        }

        case BLE_ERR_HC_DIFF_TRANSACTION_COLLISION:
        {
            return "BLE_ERR_HC_DIFF_TRANSACTION_COLLISION";
        }

        case BLE_ERR_HC_QOS_UNACCEPTABLE_PARAM:
        {
            return "BLE_ERR_HC_QOS_UNACCEPTABLE_PARAM";
        }

        case BLE_ERR_HC_QOS_REJ:
        {
            return "BLE_ERR_HC_QOS_REJ";
        }

        case BLE_ERR_HC_CH_CLASSIFICATION_NOT_SPRT:
        {
            return "BLE_ERR_HC_CH_CLASSIFICATION_NOT_SPRT";
        }

        case BLE_ERR_HC_INSUFFICIENT_SEC:
        {
            return "BLE_ERR_HC_INSUFFICIENT_SEC";
        }

        case BLE_ERR_HC_PARAM_OUT_OF_MANDATORY_RANGE:
        {
            return "BLE_ERR_HC_PARAM_OUT_OF_MANDATORY_RANGE";
        }

        case BLE_ERR_HC_ROLE_SWITCH_PENDING:
        {
            return "BLE_ERR_HC_ROLE_SWITCH_PENDING";
        }

        case BLE_ERR_HC_RESERVED_SLOT_VIOLATION:
        {
            return "BLE_ERR_HC_RESERVED_SLOT_VIOLATION";
        }

        case BLE_ERR_HC_ROLE_SWITCH_FAIL:
        {
            return "BLE_ERR_HC_ROLE_SWITCH_FAIL";
        }

        case BLE_ERR_HC_EXT_INQUIRY_RSP_TOO_LARGE:
        {
            return "BLE_ERR_HC_EXT_INQUIRY_RSP_TOO_LARGE";
        }

        case BLE_ERR_HC_SSP_NOT_SPRT_BY_HOST:
        {
            return "BLE_ERR_HC_SSP_NOT_SPRT_BY_HOST";
        }

        case BLE_ERR_HC_HOST_BUSY_PAIRING:
        {
            return "BLE_ERR_HC_HOST_BUSY_PAIRING";
        }

        case BLE_ERR_HC_CONN_REJ_NO_SUIT_CH_FOUND:
        {
            return "BLE_ERR_HC_CONN_REJ_NO_SUIT_CH_FOUND";
        }

        case BLE_ERR_HC_CTRL_BUSY:
        {
            return "BLE_ERR_HC_CTRL_BUSY";
        }

        case BLE_ERR_HC_UNACCEPTEBALE_CONN_INTERVAL:
        {
            return "BLE_ERR_HC_UNACCEPTEBALE_CONN_INTERVAL";
        }

        case BLE_ERR_HC_ADV_TO:
        {
            return "BLE_ERR_HC_ADV_TO";
        }

        case BLE_ERR_HC_CONN_TREM_DUE_TO_MIC_FAIL:
        {
            return "BLE_ERR_HC_CONN_TREM_DUE_TO_MIC_FAIL";
        }

        case BLE_ERR_HC_CONN_FAIL_TO_BE_EST:
        {
            return "BLE_ERR_HC_CONN_FAIL_TO_BE_EST";
        }

        case BLE_ERR_HC_MAC_CONN_FAIL:
        {
            return "BLE_ERR_HC_MAC_CONN_FAIL";
        }

        case BLE_ERR_HC_COARSE_CLK_ADJUST_REJ:
        {
            return "BLE_ERR_HC_COARSE_CLK_ADJUST_REJ";
        }

        case BLE_ERR_HC_TYPE0_SUBMAP_NOT_DEFINED:
        {
            return "BLE_ERR_HC_TYPE0_SUBMAP_NOT_DEFINED";
        }

        case BLE_ERR_HC_UNKNOWN_ADV_ID:
        {
            return "BLE_ERR_HC_UNKNOWN_ADV_ID";
        }

        case BLE_ERR_HC_LIMIT_REACHED:
        {
            return "BLE_ERR_HC_LIMIT_REACHED";
        }

        case BLE_ERR_HC_OP_CANCELLED_BY_HOST:
        {
            return "BLE_ERR_HC_OP_CANCELLED_BY_HOST";
        }

        case BLE_ERR_SMP_LE_PASSKEY_ENTRY_FAIL:
        {
            return "BLE_ERR_SMP_LE_PASSKEY_ENTRY_FAIL";
        }

        case BLE_ERR_SMP_LE_OOB_DATA_NOT_AVAILABLE:
        {
            return "BLE_ERR_SMP_LE_OOB_DATA_NOT_AVAILABLE";
        }

        case BLE_ERR_SMP_LE_AUTH_REQ_NOT_MET:
        {
            return "BLE_ERR_SMP_LE_AUTH_REQ_NOT_MET";
        }

        case BLE_ERR_SMP_LE_CONFIRM_VAL_NOT_MATCH:
        {
            return "BLE_ERR_SMP_LE_CONFIRM_VAL_NOT_MATCH";
        }

        case BLE_ERR_SMP_LE_PAIRING_NOT_SPRT:
        {
            return "BLE_ERR_SMP_LE_PAIRING_NOT_SPRT";
        }

        case BLE_ERR_SMP_LE_INSUFFICIENT_ENC_KEY_SIZE:
        {
            return "BLE_ERR_SMP_LE_INSUFFICIENT_ENC_KEY_SIZE";
        }

        case BLE_ERR_SMP_LE_CMD_NOT_SPRT:
        {
            return "BLE_ERR_SMP_LE_CMD_NOT_SPRT";
        }

        case BLE_ERR_SMP_LE_UNSPECIFIED_REASON:
        {
            return "BLE_ERR_SMP_LE_UNSPECIFIED_REASON";
        }

        case BLE_ERR_SMP_LE_REPEATED_ATTEMPTS:
        {
            return "BLE_ERR_SMP_LE_REPEATED_ATTEMPTS";
        }

        case BLE_ERR_SMP_LE_INVALID_PARAM:
        {
            return "BLE_ERR_SMP_LE_INVALID_PARAM";
        }

        case BLE_ERR_SMP_LE_DHKEY_CHECK_FAIL:
        {
            return "BLE_ERR_SMP_LE_DHKEY_CHECK_FAIL";
        }

        case BLE_ERR_SMP_LE_NUM_COMP_FAIL:
        {
            return "BLE_ERR_SMP_LE_NUM_COMP_FAIL";
        }

        case BLE_ERR_SMP_LE_BREDR_PAIRING_IN_PROGRESS:
        {
            return "BLE_ERR_SMP_LE_BREDR_PAIRING_IN_PROGRESS";
        }

        case BLE_ERR_SMP_LE_CT_KEY_GEN_NOT_ALLOWED:
        {
            return "BLE_ERR_SMP_LE_CT_KEY_GEN_NOT_ALLOWED";
        }

        case BLE_ERR_SMP_LE_DISCONNECTED:
        {
            return "BLE_ERR_SMP_LE_DISCONNECTED";
        }

        case BLE_ERR_SMP_LE_TO:
        {
            return "BLE_ERR_SMP_LE_TO";
        }

        case BLE_ERR_SMP_LE_LOC_KEY_MISSING:
        {
            return "BLE_ERR_SMP_LE_LOC_KEY_MISSING";
        }

        case BLE_ERR_GATT_INVALID_HANDLE:
        {
            return "BLE_ERR_GATT_INVALID_HANDLE";
        }

        case BLE_ERR_GATT_READ_NOT_PERMITTED:
        {
            return "BLE_ERR_GATT_READ_NOT_PERMITTED";
        }

        case BLE_ERR_GATT_WRITE_NOT_PERMITTED:
        {
            return "BLE_ERR_GATT_WRITE_NOT_PERMITTED";
        }

        case BLE_ERR_GATT_INVALID_PDU:
        {
            return "BLE_ERR_GATT_INVALID_PDU";
        }

        case BLE_ERR_GATT_INSUFFICIENT_AUTHENTICATION:
        {
            return "BLE_ERR_GATT_INSUFFICIENT_AUTHENTICATION";
        }

        case BLE_ERR_GATT_REQUEST_NOT_SUPPORTED:
        {
            return "BLE_ERR_GATT_REQUEST_NOT_SUPPORTED";
        }

        case BLE_ERR_GATT_INVALID_OFFSET:
        {
            return "BLE_ERR_GATT_INVALID_OFFSET";
        }

        case BLE_ERR_GATT_INSUFFICIENT_AUTHORIZATION:
        {
            return "BLE_ERR_GATT_INSUFFICIENT_AUTHORIZATION";
        }

        case BLE_ERR_GATT_PREPARE_WRITE_QUEUE_FULL:
        {
            return "BLE_ERR_GATT_PREPARE_WRITE_QUEUE_FULL";
        }

        case BLE_ERR_GATT_ATTRIBUTE_NOT_FOUND:
        {
            return "BLE_ERR_GATT_ATTRIBUTE_NOT_FOUND";
        }

        case BLE_ERR_GATT_ATTRIBUTE_NOT_LONG:
        {
            return "BLE_ERR_GATT_ATTRIBUTE_NOT_LONG";
        }

        case BLE_ERR_GATT_INSUFFICIENT_ENC_KEY_SIZE:
        {
            return "BLE_ERR_GATT_INSUFFICIENT_ENC_KEY_SIZE";
        }

        case BLE_ERR_GATT_INVALID_ATTRIBUTE_LEN:
        {
            return "BLE_ERR_GATT_INVALID_ATTRIBUTE_LEN";
        }

        case BLE_ERR_GATT_UNLIKELY_ERROR:
        {
            return "BLE_ERR_GATT_UNLIKELY_ERROR";
        }

        case BLE_ERR_GATT_INSUFFICIENT_ENCRYPTION:
        {
            return "BLE_ERR_GATT_INSUFFICIENT_ENCRYPTION";
        }

        case BLE_ERR_GATT_UNSUPPORTED_GROUP_TYPE:
        {
            return "BLE_ERR_GATT_UNSUPPORTED_GROUP_TYPE";
        }

        case BLE_ERR_GATT_INSUFFICIENT_RESOURCES:
        {
            return "BLE_ERR_GATT_INSUFFICIENT_RESOURCES";
        }

        case BLE_ERR_GATT_WRITE_REQ_REJECTED:
        {
            return "BLE_ERR_GATT_WRITE_REQ_REJECTED";
        }

        case BLE_ERR_GATT_CCCD_IMPROPERLY_CFG:
        {
            return "BLE_ERR_GATT_CCCD_IMPROPERLY_CFG";
        }

        case BLE_ERR_GATT_PROC_ALREADY_IN_PROGRESS:
        {
            return "BLE_ERR_GATT_PROC_ALREADY_IN_PROGRESS";
        }

        case BLE_ERR_GATT_OUT_OF_RANGE:
        {
            return "BLE_ERR_GATT_OUT_OF_RANGE";
        }

        case BLE_ERR_L2CAP_PSM_NOT_SUPPORTED:
        {
            return "BLE_ERR_L2CAP_PSM_NOT_SUPPORTED";
        }

        case BLE_ERR_L2CAP_NO_RESOURCE:
        {
            return "BLE_ERR_L2CAP_NO_RESOURCE";
        }

        case BLE_ERR_L2CAP_INSUF_AUTHEN:
        {
            return "BLE_ERR_L2CAP_INSUF_AUTHEN";
        }

        case BLE_ERR_L2CAP_INSUF_AUTHOR:
        {
            return "BLE_ERR_L2CAP_INSUF_AUTHOR";
        }

        case BLE_ERR_L2CAP_INSUF_ENC_KEY_SIZE:
        {
            return "BLE_ERR_L2CAP_INSUF_ENC_KEY_SIZE";
        }

        case BLE_ERR_L2CAP_REFUSE_INSUF_ENC:
        {
            return "BLE_ERR_L2CAP_REFUSE_INSUF_ENC";
        }

        case BLE_ERR_L2CAP_REFUSE_INVALID_SCID:
        {
            return "BLE_ERR_L2CAP_REFUSE_INVALID_SCID";
        }

        case BLE_ERR_L2CAP_REFUSE_SCID_ALREADY_ALLOC:
        {
            return "BLE_ERR_L2CAP_REFUSE_SCID_ALREADY_ALLOC";
        }

        case BLE_ERR_L2CAP_REFUSE_UNACCEPTABLE_PARAM:
        {
            return "BLE_ERR_L2CAP_REFUSE_UNACCEPTABLE_PARAM";
        }

        default:

            return "UKNOWN";
    }
}

const char * E_R_BLE_GATTC_EV_T (e_r_ble_gattc_evt_t status)
{
    switch (status)
    {
        case BLE_GATTC_EVENT_ERROR_RSP:
        {
            return "BLE_GATTC_EVENT_ERROR_RSP";
        }

        case BLE_GATTC_EVENT_EX_MTU_RSP:
        {
            return "BLE_GATTC_EVENT_EX_MTU_RSP";
        }

        case BLE_GATTC_EVENT_CHAR_READ_BY_UUID_RSP:
        {
            return "BLE_GATTC_EVENT_CHAR_READ_BY_UUID_RSP";
        }

        case BLE_GATTC_EVENT_CHAR_READ_RSP:
        {
            return "BLE_GATTC_EVENT_CHAR_READ_RSP";
        }

        case BLE_GATTC_EVENT_CHAR_PART_READ_RSP:
        {
            return "BLE_GATTC_EVENT_CHAR_PART_READ_RSP";
        }

        case BLE_GATTC_EVENT_MULTI_CHAR_READ_RSP:
        {
            return "BLE_GATTC_EVENT_MULTI_CHAR_READ_RSP";
        }

        case BLE_GATTC_EVENT_CHAR_WRITE_RSP:
        {
            return "BLE_GATTC_EVENT_CHAR_WRITE_RSP";
        }

        case BLE_GATTC_EVENT_CHAR_PART_WRITE_RSP:
        {
            return "BLE_GATTC_EVENT_CHAR_PART_WRITE_RSP";
        }

        case BLE_GATTC_EVENT_HDL_VAL_NTF:
        {
            return "BLE_GATTC_EVENT_HDL_VAL_NTF";
        }

        case BLE_GATTC_EVENT_HDL_VAL_IND:
        {
            return "BLE_GATTC_EVENT_HDL_VAL_IND";
        }

        case BLE_GATTC_EVENT_CONN_IND:
        {
            return "BLE_GATTC_EVENT_CONN_IND";
        }

        case BLE_GATTC_EVENT_DISCONN_IND:
        {
            return "BLE_GATTC_EVENT_DISCONN_IND";
        }

        case BLE_GATTC_EVENT_PRIM_SERV_16_DISC_IND:
        {
            return "BLE_GATTC_EVENT_PRIM_SERV_16_DISC_IND";
        }

        case BLE_GATTC_EVENT_PRIM_SERV_128_DISC_IND:
        {
            return "BLE_GATTC_EVENT_PRIM_SERV_128_DISC_IND";
        }

        case BLE_GATTC_EVENT_ALL_PRIM_SERV_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_ALL_PRIM_SERV_DISC_COMP";
        }

        case BLE_GATTC_EVENT_PRIM_SERV_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_PRIM_SERV_DISC_COMP";
        }

        case BLE_GATTC_EVENT_SECOND_SERV_16_DISC_IND:
        {
            return "BLE_GATTC_EVENT_SECOND_SERV_16_DISC_IND";
        }

        case BLE_GATTC_EVENT_SECOND_SERV_128_DISC_IND:
        {
            return "BLE_GATTC_EVENT_SECOND_SERV_128_DISC_IND";
        }

        case BLE_GATTC_EVENT_ALL_SECOND_SERV_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_ALL_SECOND_SERV_DISC_COMP";
        }

        case BLE_GATTC_EVENT_INC_SERV_16_DISC_IND:
        {
            return "BLE_GATTC_EVENT_INC_SERV_16_DISC_IND";
        }

        case BLE_GATTC_EVENT_INC_SERV_128_DISC_IND:
        {
            return "BLE_GATTC_EVENT_INC_SERV_128_DISC_IND";
        }

        case BLE_GATTC_EVENT_INC_SERV_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_INC_SERV_DISC_COMP";
        }

        case BLE_GATTC_EVENT_CHAR_16_DISC_IND:
        {
            return "BLE_GATTC_EVENT_CHAR_16_DISC_IND";
        }

        case BLE_GATTC_EVENT_CHAR_128_DISC_IND:
        {
            return "BLE_GATTC_EVENT_CHAR_128_DISC_IND";
        }

        case BLE_GATTC_EVENT_ALL_CHAR_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_ALL_CHAR_DISC_COMP";
        }

        case BLE_GATTC_EVENT_CHAR_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_CHAR_DISC_COMP";
        }

        case BLE_GATTC_EVENT_CHAR_DESC_16_DISC_IND:
        {
            return "BLE_GATTC_EVENT_CHAR_DESC_16_DISC_IND";
        }

        case BLE_GATTC_EVENT_CHAR_DESC_128_DISC_IND:
        {
            return "BLE_GATTC_EVENT_CHAR_DESC_128_DISC_IND";
        }

        case BLE_GATTC_EVENT_ALL_CHAR_DESC_DISC_COMP:
        {
            return "BLE_GATTC_EVENT_ALL_CHAR_DESC_DISC_COMP";
        }

        case BLE_GATTC_EVENT_LONG_CHAR_READ_COMP:
        {
            return "BLE_GATTC_EVENT_LONG_CHAR_READ_COMP";
        }

        case BLE_GATTC_EVENT_LONG_CHAR_WRITE_COMP:
        {
            return "BLE_GATTC_EVENT_LONG_CHAR_WRITE_COMP";
        }

        case BLE_GATTC_EVENT_RELIABLE_WRITES_TX_COMP:
        {
            return "BLE_GATTC_EVENT_RELIABLE_WRITES_TX_COMP";
        }

        case BLE_GATTC_EVENT_RELIABLE_WRITES_COMP:
        {
            return "BLE_GATTC_EVENT_RELIABLE_WRITES_COMP";
        }

        case BLE_GATTC_EVENT_INVALID:
        {
            return "BLE_GATTC_EVENT_INVALID";
        }

        default:

            return "UKNOWN";
    }
}

const char * E_R_BLE_GATTS_EVT_T (e_r_ble_gatts_evt_t status)
{
    switch (status)
    {
        case BLE_GATTS_EVENT_EX_MTU_REQ:
        {
            return "BLE_GATTS_EVENT_EX_MTU_REQ";
        }

        case BLE_GATTS_EVENT_READ_BY_TYPE_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_READ_BY_TYPE_RSP_COMP";
        }

        case BLE_GATTS_EVENT_READ_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_READ_RSP_COMP";
        }

        case BLE_GATTS_EVENT_READ_BLOB_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_READ_BLOB_RSP_COMP";
        }

        case BLE_GATTS_EVENT_READ_MULTI_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_READ_MULTI_RSP_COMP";
        }

        case BLE_GATTS_EVENT_WRITE_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_WRITE_RSP_COMP";
        }

        case BLE_GATTS_EVENT_PREPARE_WRITE_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_PREPARE_WRITE_RSP_COMP";
        }

        case BLE_GATTS_EVENT_EXE_WRITE_RSP_COMP:
        {
            return "BLE_GATTS_EVENT_EXE_WRITE_RSP_COMP";
        }

        case BLE_GATTS_EVENT_HDL_VAL_CNF:
        {
            return "BLE_GATTS_EVENT_HDL_VAL_CNF";
        }

        case BLE_GATTS_EVENT_DB_ACCESS_IND:
        {
            return "BLE_GATTS_EVENT_DB_ACCESS_IND";
        }

        case BLE_GATTS_EVENT_CONN_IND:
        {
            return "BLE_GATTS_EVENT_CONN_IND";
        }

        case BLE_GATTS_EVENT_DISCONN_IND:
        {
            return "BLE_GATTS_EVENT_DISCONN_IND";
        }

        case BLE_GATTS_EVENT_INVALID:
        {
            return "BLE_GATTS_EVENT_INVALID";
        }

        default:

            return "UKNOWN";
    }
}

const char * E_R_BLE_VS_EVT_T (e_r_ble_vs_evt_t status)
{
    switch (status)
    {
        case BLE_VS_EVENT_SET_TX_POWER:
        {
            return "BLE_VS_EVENT_SET_TX_POWER";
        }

        case BLE_VS_EVENT_GET_TX_POWER:
        {
            return "BLE_VS_EVENT_GET_TX_POWER";
        }

        case BLE_VS_EVENT_TX_TEST_START:
        {
            return "BLE_VS_EVENT_TX_TEST_START";
        }

        case BLE_VS_EVENT_TX_TEST_TERM:
        {
            return "BLE_VS_EVENT_TX_TEST_TERM";
        }

        case BLE_VS_EVENT_RX_TEST_START:
        {
            return "BLE_VS_EVENT_RX_TEST_START";
        }

        case BLE_VS_EVENT_TEST_END:
        {
            return "BLE_VS_EVENT_TEST_END";
        }

        case BLE_VS_EVENT_SET_CODING_SCHEME_COMP:
        {
            return "BLE_VS_EVENT_SET_CODING_SCHEME_COMP";
        }

        case BLE_VS_EVENT_RF_CONTROL_COMP:
        {
            return "BLE_VS_EVENT_RF_CONTROL_COMP";
        }

        case BLE_VS_EVENT_SET_ADDR_COMP:
        {
            return "BLE_VS_EVENT_SET_ADDR_COMP";
        }

        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            return "BLE_VS_EVENT_GET_ADDR_COMP";
        }

        case BLE_VS_EVENT_GET_RAND:
        {
            return "BLE_VS_EVENT_GET_RAND";
        }

        case BLE_VS_EVENT_TX_FLOW_STATE_CHG:
        {
            return "BLE_VS_EVENT_TX_FLOW_STATE_CHG";
        }

        case BLE_VS_EVENT_FAIL_DETECT:
        {
            return "BLE_VS_EVENT_FAIL_DETECT";
        }

        case BLE_VS_EVENT_SET_SCAN_CH_MAP:
        {
            return "BLE_VS_EVENT_SET_SCAN_CH_MAP";
        }

        case BLE_VS_EVENT_GET_SCAN_CH_MAP:
        {
            return "BLE_VS_EVENT_GET_SCAN_CH_MAP";
        }

        case BLE_VS_EVENT_START_FW_UPDATE_COMP:
        {
            return "BLE_VS_EVENT_START_FW_UPDATE_COMP";
        }

        case BLE_VS_EVENT_SEND_FW_DATA_COMP:
        {
            return "BLE_VS_EVENT_SEND_FW_DATA_COMP";
        }

        case BLE_VS_EVENT_END_FW_UPDATE_COMP:
        {
            return "BLE_VS_EVENT_END_FW_UPDATE_COMP";
        }

        case BLE_VS_EVENT_GET_FW_VERSION_COMP:
        {
            return "BLE_VS_EVENT_GET_FW_VERSION_COMP";
        }

        case BLE_VS_EVENT_MODULE_READY_COMP:
        {
            return "BLE_VS_EVENT_MODULE_READY_COMP";
        }

        case BLE_VS_EVENT_OTA_START_NOTIFY:
        {
            return "BLE_VS_EVENT_OTA_START_NOTIFY";
        }

        case BLE_VS_EVENT_OTA_END_NOTIFY:
        {
            return "BLE_VS_EVENT_OTA_END_NOTIFY";
        }

        case BLE_VS_EVENT_OTA_ERROR_NOTIFY:
        {
            return "BLE_VS_EVENT_OTA_ERROR_NOTIFY";
        }

        case BLE_VS_EVENT_INVALID:
        {
            return "BLE_VS_EVENT_INVALID";
        }

        default:

            return "UKNOWN";
    }
}

#endif
