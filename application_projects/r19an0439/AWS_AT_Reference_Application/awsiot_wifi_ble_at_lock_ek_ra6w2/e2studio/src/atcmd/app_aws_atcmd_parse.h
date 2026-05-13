/***********************************************************************************************************************
 * File Name    : app_aws_atcmd_parse.h
 * Description  : Header file for AT command parser for AWS IoT
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* ${REA_DISCLAIMER_PLACEHOLDER} */

#ifndef RM_ATCMD_W_CORE_AWS_PARSE_H
#define RM_ATCMD_W_CORE_AWS_PARSE_H

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#if __has_include("rm_awsiot_w_cfg.h")
#include "rm_awsiot_w_cfg.h"
#endif

#ifdef __SUPPORT_AWS_IOT_W__
#include "sdk_defs.h"
#include "rm_atcmd_w_core_common.h"

/* Common macro for FSP header files.
 * There is also a corresponding FSP_FOOTER
 * macro at the end of this file. */
FSP_HEADER

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define PREFIX_AWS_CERT                             "AT+MCONFIG=CERT"
#define NV_BOARD_FEATURE                            "APP_BRD_FEATURE"
#define MAX_SHADOW_STRING                           (MAX_PKEY_LEN - 1)
#define MAX_SUBSCRIBE_COUNT                         10
#define MAX_SUBSCRIBE_STRING                        (MAX_PKEY_LEN - 1)
#define MAX_PKEY_LEN                                20
#define MAX_CMD_TO_MCU_PAYLOAD                      128
#define APP_NVRAM_CONFIG_DPM_AUTO                   "APP_DPMAUTO"

/* pin mux */
#define NV_PIN_AMUX                                 "NV_PIN_AMUX"
#define NV_PIN_BMUX                                 "NV_PIN_BMUX"
#define NV_PIN_CMUX                                 "NV_PIN_CMUX"
#define NV_PIN_DMUX                                 "NV_PIN_DMUX"
#define NV_PIN_EMUX                                 "NV_PIN_EMUX"
#define NV_PIN_FMUX                                 "NV_PIN_FMUX"
#define NV_PIN_HMUX                                 "NV_PIN_HMUX"
#define NV_PIN_UMUX                                 "NV_PIN_UMUX"
#define NV_MCU_UART_CFG                             "UART_CFG"

#define CONFIG_MQTT_TYPE_PUBLISH                    0
#define CONFIG_MQTT_TYPE_SHADOW                     1
#define CONFIG_MQTT_TYPE_SUBSCRIBE                  2
#define CONFIG_DATA_TYPE_INT                        0
#define CONFIG_DATA_TYPE_STR                        1
#define CONFIG_DATA_TYPE_FLOAT                      2

#define CMD_FATORY_RESET                            "FACTORY_RESET"
#define CMD_RESET_TO_AP                             "RESET_TO_AP"
#define CMD_MCU_MQTT                                "MCU_DATA"
#define CMD_GET_STATUS                              "GET_STATUS"
#define CMD_RESTART                                 "RESTART"
#define CMD_SERVER_DATA                             "SERVER_DATA"
#define CMD_TO_MCU                                  "CMD_TO_MCU"
#define NV_CONFIG_THING_ATTIRIBUTE                  "T_att"

#define TEMP_NV_NAME_SIZE                           256
#define MAX_ARGV_NUM                                50
#define TEMP_AT_DATA_SIZE                           128
#define MAX_PUBLISH_PAYLOAD_LEN                     512
#define MAX_SUBSCRIBE_TOPIC_LEN                     256
/* DPM parameter string from MCU set */
#define AT_DPM_SLEEP_MODE                           "SLEEP_MODE"
#define AT_DPM_USE_DPM                              "USE_DPM"
#define AT_DPM_RTC_TIME                             "RTC_TIME"
#define AT_DPM_KEEP_ALIVE                           "DPM_KEEP_ALIVE"
#define AT_DPM_USE_WAKE_UP                          "USE_WAKE_UP"
#define AT_DPM_TIM_WAKE_UP                          "TIM_WAKE_UP"

#define RETRY_ACK_CHECK_SKIP                        1
#define USE_FLEET_PROVISION                         "AWS_USE_FP"
#define NV_MCU_WAKEUP_PORT                          "APP_MCU_WU_PORT"
#define NV_MCU_WAKEUP_PIN                           "APP_MCU_WU_PIN"
#define APP_NVRAM_CONFIG_LPORT                      "APP_LPORT" ///< APP local port on NVRAM
#define APP_NVRAM_CONFIG_PTOPIC                     "APP_PUBTOPIC" ///< APP publishing topic on NVRAM
#define APP_NVRAM_CONFIG_STOPIC                     "APP_SUBTOPIC" ///< APP subcribing topic on NVRAM

#define AWS_CREDENTIALS_CA_ADDR                     (SF_USER_AREA + 0x1000)
#define AWS_CREDENTIALS_CERT_ADDR                   (SF_USER_AREA + 0x2000)
#define AWS_CREDENTIALS_KEY_ADDR                    (SF_USER_AREA + 0x3000)
#define MAX_SHADOW_COUNT                            10

#define PLATFORM                                    "AWS"

/* atoi error policy
 default leading "0" / "+" / "-0" are not allowed
*/
#define POL_1   1 
/* leading "+" / "-0" are not allowed */
#define POL_2   2
typedef struct
{
    UINT offset;
    UINT content_length;
    UINT received_length;
    UINT size;
    UINT crc;
    UINT imgcrc;
} ota_mcu_fw_stream_info_t;
#define OTA_MCU_FW_STREAM_HEADER_SIZE   sizeof(ota_mcu_fw_stream_info_t)

typedef enum _atcmd_provision_stat
{
    /// Provisioning not working or finish
    ATCMD_PROVISION_IDLE                            = 0,

    /// Provisioning start
    ATCMD_PROVISION_START                           = 1,

    /// Received AP info from mobile 
    ATCMD_PROVISION_SELECTED_AP_SUCCESS             = 101,
    ATCMD_PROVISION_SELECTED_AP_FAIL                = 102,

    /// AP connection fail check SSID or PW
    ATCMD_PROVISION_WRONG_PW                        = 103,    // New add
    ATCMD_PROVISION_AP_FAIL                         = 105,

    /// Network checking status 
    ATCMD_PROVISION_DNS_FAIL_SERVER_FAIL            = 106,
    ATCMD_PROVISION_DNS_FAIL_SERVER_OK              = 107,    // in case wrong DNS name
    ATCMD_PROVISION_NO_URL_PING_FAIL                = 108,
    ATCMD_PROVISION_NO_URL_PING_OK                  = 109,
    ATCMD_PROVISION_DNS_OK_PING_FAIL_N_SERVER_OK    = 110,    // in case AP gives wrong IP
    ATCMD_PROVISION_DNS_OK_PING_OK                  = 111,
    ATCMD_PROVISION_REBOOT_ACK                      = 112,
    ATCMD_PROVISION_DNS_OK_PING_N_SERVER_FAIL       = 113     // in case default ping fail
   
} atcmd_provision_stat;

typedef struct _atComdataConfig
{
    int filedNum;
    char *name;
    int type;
    int intVal;
    char *strVal;
    int mqtttype;
} atComdataConfig;

typedef enum atcmd_dpm_value_type
{
    ATCMD_TYPE_SLEEP_MODE = 0,                ///< Sleep mode
    ATCMD_TYPE_USE_DPM,                       ///< Use DPM or Not
    ATCMD_TYPE_RTC_TIME,                      ///< RTC Time
    ATCMD_TYPE_DPM_KEEP_ALIVE,                ///< DPM Keep Alive
    ATCMD_TYPE_USER_WAKE_UP,                  ///< User Wake Up Time
    ATCMD_TYPE_TIM_WAKE_UP,                   ///< Timer Wake Up Time
} atcmd_dpm_value_type_t;

typedef enum atcmd_scan_result
{
    ATCMD_STATUS_IDLE = -1,
    ATCMD_STATUS_FACTORY_RESET_DONE = 0,
    ATCMD_STATUS_BOOT_READY = 1,
    ATCMD_STATUS_NEED_CONFIGURATION = 5,
    ATCMD_STATUS_AP_MODE_START = 10,
    ATCMD_STATUS_NETWORK_OK = 15,
    ATCMD_STATUS_NETWORK_FAIL = 16,
    ATCMD_STATUS_STA_START = 20,
    ATCMD_STATUS_STA_DONE = 25,
    ATCMD_STATUS_MCUOTA = 30
} atcmd_scan_result_t;

typedef enum atcmd_json_primitive_type
{
    SHADOW_JSON_INT32,
    SHADOW_JSON_INT16,
    SHADOW_JSON_INT8,
    SHADOW_JSON_UINT32,
    SHADOW_JSON_UINT16,
    SHADOW_JSON_UINT8,
    SHADOW_JSON_FLOAT,
    SHADOW_JSON_DOUBLE,
    SHADOW_JSON_BOOL,
    SHADOW_JSON_STRING,
    SHADOW_JSON_OBJECT
} atcmd_json_primitive_type_t;

typedef struct st_atcmd_jsonStruct st_atcmd_jsonStruct_t;
typedef void (* jsonStructCallback_t)(const char * pJsonValueBuffer, uint32_t valueLength, st_atcmd_jsonStruct_t * pJsonStruct_t);
struct st_atcmd_jsonStruct
{
    const char * pKey; /* JSON key */
    void * pData; /* pointer to the data (JSON value) */
    size_t dataLength; /* Length (in bytes) of pData */
    atcmd_json_primitive_type_t type; /* type of JSON */
    jsonStructCallback_t cb; /* callback to be executed on receiving the Key value pair */
};

typedef struct st_atcmd_shadow_node
{
    struct st_atcmd_shadow_node * next;
    st_atcmd_jsonStruct_t shadowData;
} st_atcmd_shadow_node_t;

typedef void (* ATCmdPublishEventCallback_t)(char * payload);

/***********************************************************************************************************************
 * Function Prototypes
 **********************************************************************************************************************/
void add_aws_atcmd(void);
void register_atcmd_publish_user_app_callback(ATCmdPublishEventCallback_t callback);
fsp_err_t factory_user_nvram_reset_awsiot_at(void);
void waitIFReady(void);
void wait_MCU_resp(uint8_t owner);
uint8_t get_MCU_resp(void);
int checkconfigdataAtNVRAM(int num);
void atcmd_provstat(int status);
void atcmd_mcu_wakeup(uint8_t port, uint16_t pin);
UINT app_writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size);
void app_iot_at_command_status(atcmd_scan_result_t _val);
void app_iot_at_IF_ready_notification(int _val);
int app_iot_set_feature_parser(int argc, char *argv[]);
int app_iot_config_thing_parser(int argc, char *argv[]);
int app_iot_at_push_com_parser(int argc, char *argv[]);
int app_iot_at_response_ok_parser(int argc, char *argv[]);
atComdataConfig* app_get_at_config(int num);
void app_release_at_config(atComdataConfig *_config);
void app_atcmd_set_DPM_value(atcmd_dpm_value_type_t _type, int value);
uint8_t app_atcmd_mcu_wakeup_port(char *port_num);
uint16_t app_atcmd_mcu_wakeup_pin(char *pin_num);
int32_t app_at_feature_is_valid(void);
int32_t app_at_check_certi_info(void);
uint32_t app_at_init_shadow_items(void);
int32_t app_at_get_registered_subscribe_count(void);
char* app_at_get_cloud_cmd_payload(int32_t _index);
char* app_at_get_registered_subscribe(int32_t _index);
void app_at_set_cloud_cmd_payload(char *_payload, int32_t _needLen);
/***********************************************************************************************************************
 * AT Function Prototypes
 **********************************************************************************************************************/
uint32_t RM_ATCMD_W_CORE_AWS_register(atcmd_w_core_module_list_t * p_list);
uint32_t RM_ATCMD_W_CORE_AWS_deregister(atcmd_w_core_module_list_t * p_list);
uint32_t RM_ATCMD_W_CORE_AWS_open(atcmd_w_ctrl_t * const p_atcmd_w_ctrl);
uint32_t RM_ATCMD_W_CORE_AWS_close(atcmd_w_ctrl_t * const p_atcmd_w_ctrl);
void RM_PL_PRINTF_ATCMD(char *p_str);

/* Common macro for FSP header files.
 * There is also a corresponding FSP_HEADER
 * macro at the top of this file. */
FSP_FOOTER

#endif /* RM_ATCMD_W_CORE_AWS_PARSE_H */
#endif /* __SUPPORT_AWS_IOT_W__ */

