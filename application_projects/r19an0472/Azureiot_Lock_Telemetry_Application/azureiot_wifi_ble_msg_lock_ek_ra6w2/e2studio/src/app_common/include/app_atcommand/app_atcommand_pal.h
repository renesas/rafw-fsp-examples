/**
 ****************************************************************************************
 *
 * @file app_atcommand_pal.h
 *
 * @brief adaptation layer header between platform and at-command driver
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */
#if defined (__SUPPORT_AWS_IOT__) || defined(__SUPPORT_AZURE_IOT__)
#ifndef __APP_ATCOMMAND_PAL_H__
#define __APP_ATCOMMAND_PAL_H__
#include "ota_update.h"
#include "app_common_support.h"
#include "app_atcommand.h"
#include "app_thing_manager.h"
#if defined(__SUPPORT_AWS_IOT__)
#include "core_mqtt_serializer.h"
#include "mqtt_pkcs11_demo_helpers.h"
#include "app_aws_user_conf.h"
#endif
#if defined(__SUPPORT_AZURE_IOT__)
#include "app_azure_user_conf.h"
#endif

#if defined (__SUPPORT_AWS_IOT__)
#define PLATFORM "AWS"
#elif defined (__SUPPORT_AZURE_IOT__)
#define PLATFORM "AZU"
#endif  /* __SUPPORT_AWS_IOT__ */

#define DEFAULT_PING_IP "8.8.8.8" /* dns.google */

#define APP_NVRAM_CONFIG_LPORT "APP_LPORT" ///< APP local port on NVRAM
#define APP_NVRAM_CONFIG_PTOPIC "APP_PUBTOPIC" ///< APP publishing topic on NVRAM
#define APP_NVRAM_CONFIG_STOPIC "APP_SUBTOPIC" ///< APP subcribing topic on NVRAM
#define APP_NVRAM_CONFIG_PING_CHECK "APP_PINGCHECK" ///< DPM running flag
#define APP_CONTROL_ATUPDATE_PREFIX     "update"                ///< APP updating current state

struct atcmd_nvparams {
    char *broker;
    uint32_t port;
    char *certCA;
    char *cert;
    char *key;
};

typedef enum _ATCMDStatus {
    ATCMD_Status_IDLE = -1,
    ATCMD_Status_factoryreset_done = 0,
    ATCMD_Status_boot_ready = 1,
    ATCMD_Status_need_configuration = 5,
    ATCMD_Status_AP_mode_start = 10,
    ATCMD_Status_network_OK = 15,
    ATCMD_Status_network_Fail = 16,
    ATCMD_Status_STA_start = 20,
    ATCMD_Status_STA_done = 25,
    ATCMD_Status_MCUOTA = 30,
    ATCMD_Status_Server_Connect_Success = 50,
    ATCMD_Status_Server_Connect_Fail = 51
} ATCMDScanResult;

typedef enum {
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
} JsonPrimitiveType;
typedef struct jsonStruct jsonStruct_t;
typedef void (*jsonStructCallback_t)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
struct jsonStruct {
    const char *pKey; /* JSON key */
    void *pData; /* pointer to the data (JSON value) */
    size_t dataLength; /* Length (in bytes) of pData */
    JsonPrimitiveType type; /* type of JSON */
    jsonStructCallback_t cb; /* callback to be executed on receiving the Key value pair */
};
typedef struct _shadowNode {
    struct _shadowNode *next;
    jsonStruct_t shadowData;
} shadowNode;

extern int32_t gShadowCount;
extern jsonStruct_t registeredShadow[];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief get default local port for network
 * @param[in] port local port
 ****************************************************************************************
 */
void pal_app_get_def_local_port(uint32_t *port);

/**
 ****************************************************************************************
 * @brief get root CA certification
 * @param[in] cert certification address
 * @return address of root CA
 ****************************************************************************************
 */
char* pal_app_get_cert_ca(char *cert);

/**
 ****************************************************************************************
 * @brief get at-command parameters for network information
 * @return 0 :success , 1 : error 
 ****************************************************************************************
 */
uint8_t pal_app_get_atcmd_parameter(void);

/**
 ****************************************************************************************
 * @brief network check by ping
 ****************************************************************************************
 */
void pal_app_check_ping(void);

/**
 ****************************************************************************************
 * @brief checking UART and at-command configuration
 ****************************************************************************************
 */
void pal_app_dpm_auto_start(void);

/**
 ****************************************************************************************
 * @brief initialize system for provisioning
 * @param[in] mode provisoning mode
 ****************************************************************************************
 */
void pal_app_start_provisioning(int32_t mode);

/**
 ****************************************************************************************
 * @brief set at-command status for platform
 * @param[in] _val status
 ****************************************************************************************
 */
void pal_app_at_command_status(ATCMDScanResult _val);

/**
 ****************************************************************************************
 * @brief set provisioning type of the application
 ****************************************************************************************
 */
void pal_app_set_prov_feature(void);

/**
 ****************************************************************************************
 * @brief checking whether the OTA is for MCU image
 * @param[in] _url url of OTA
 * @param[in] config configuration structure for OTA
 * @return 0 :Not MCU OTA , 1 : MCU OTA
 ****************************************************************************************
 */
#if defined(__SUPPORT_OTA__)
int8_t pal_app_check_is_update_mcu(char *_url, OTA_UPDATE_CONFIG *config);

/**
 ****************************************************************************************
 * @brief final work for OTA completion
 * @param[in] update_type OTA type
 * @param[in] status OTA status
 * @param[in] progress OTA progress
 * @param[in] renew not used
 * @return 0 :Not MCU OTA , 1 : MCU OTA
 ****************************************************************************************
 */
int8_t pal_app_notify_mcu_ota(ota_update_type update_type, uint32_t status, uint32_t progress, uint32_t renew);
#endif
/**
 ****************************************************************************************
 * @brief send command to MCU
 * @param[in] _rtmData rtm data
 * @return 0 :Not at-command , 1 : at-command process
 ****************************************************************************************
 */
uint8_t pal_app_controlCommand(app_dpm_info_rtm *_rtmData);

/**
 ****************************************************************************************
 * @brief update data from MCU
 * @param[in] _rtmData rtm data
 * @return 0 :Not at-command , 1 : at-command process
 ****************************************************************************************
 */
int8_t pal_app_atc_work(app_dpm_info_rtm *_rtmData);

/**
 ****************************************************************************************
 * @brief get default local port for network
 * @param[in] port local port
 * @return 0 :Not at-command , 1 : at-command process
 ****************************************************************************************
 */
int8_t pal_is_support_mcu_update(void);

#if defined (__SUPPORT_AWS_IOT__)
/**
 ****************************************************************************************
 * @brief get broker url
 * @param[in] broker default broker url from application
 * @return real broker url
 ****************************************************************************************
 */
char* pal_app_get_broker_endpoint(char *broker);

/**
 ****************************************************************************************
 * @brief get broker port.
 * @param[in] port default broker port from application
 * @return real broker port
 ****************************************************************************************
 */
uint32_t pal_app_get_broker_port(uint32_t port);

/**
 ****************************************************************************************
 * @brief get device cert.
 * @param[in] cert default device cert from application
 * @return real device cert
 ****************************************************************************************
 */
char* pal_app_get_cert_cert(char *cert);

/**
 ****************************************************************************************
 * @brief get device key
 * @param[in] key default device key from application
 * @return real device key
 ****************************************************************************************
 */
char* pal_app_get_cert_key(char *key);

/**
 ****************************************************************************************
 * @brief process event from AWS server
 * @param[in] payload event string from server
 * @param[in] len payload length
 * @param[in] connect is connect command
 * @param[in] control is control command
 * @param[in] updatesensor is updatesensor command
 * @return 0:Not at-command, 1:at-command process
 ****************************************************************************************
 */
int8_t pal_app_event_cb(char *payload, int32_t len, bool *connect, bool *control, bool *updatesensor);

/**
 ****************************************************************************************
 * @brief publish topic to broker
 * @param[in] _payload publish data
 ****************************************************************************************
 */
void pal_app_publish_user_contents(char *_payload);

/**
 ****************************************************************************************
 * @brief register subscribe topic
 * @param[in] port local port
 * @return 0:Not at-command, 1:register success, 2:register error
 ****************************************************************************************
 */
int8_t pal_app_reg_subscription_topic(void);
#endif  /* __SUPPORT_AWS_IOT__ */
#if defined(__SUPPORT_AZURE_IOT__)

/**
 ****************************************************************************************
 * @brief process event from AZURE server
 * @param[in] payload event string from server
 * @param[in] size payload length
 * @param[in] response response for server
 * @param[in] response_size response size
 * @param[in] result result of event processed for server
 * @param[in] connect_flag connect status
 * @return 0:Not at-command, 1:at-command process
 ****************************************************************************************
 */
int8_t pal_app_event_cb(const unsigned char *payload, size_t size, unsigned char **response, size_t *response_size,
    int32_t *result, bool *connect_flag);

/**
 ****************************************************************************************
 * @brief making Json for AZURE twin
 * @param[in] doorlock information of device to make Json
 * @return 0:Not at-command, others:Json data
 ****************************************************************************************
 */
char* palATDoorlockSerializeToJson(Doorlock *doorlock);
extern void app_set_control_flag(bool flag);
extern bool app_get_control_flag(void);
extern char* app_get_ota_result(void);
extern void app_send_twins(char *twins_what);
#endif  /* __SUPPORT_AZURE_IOT__ */
#if defined (__SUPPORT_AWS_IOT__)
extern void app_aws_dpm_app_connect(app_dpm_info_rtm *_rtmData);
extern MQTTStatus_t app_get_mqtt_status(void);
extern void app_set_mqtt_status(MQTTStatus_t rc);
extern MQTTContext_t* app_get_mqtt_context(void);
extern char* app_get_ota_url(void);
extern char* app_get_ota_result(void);
#endif  /* __SUPPORT_AWS_IOT__ */
extern void app_iot_at_command_status(ATCMDScanResult _val);
extern void app_start_provisioning(int32_t _mode);
#endif /* __APP_ATCOMMAND_PAL_H__ */
#endif  /* __SUPPORT_AWS_IOT__ || __SUPPORT_AZURE_IOT__ */
