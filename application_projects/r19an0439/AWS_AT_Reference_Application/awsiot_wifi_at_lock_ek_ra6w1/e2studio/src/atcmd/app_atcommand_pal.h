/***********************************************************************************************************************
 * File Name    : app_atcommand_pal.h
 * Description  : adaptation layer header between platform and at-command driver
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"

#ifndef __APP_ATCOMMAND_PAL_H__
#define __APP_ATCOMMAND_PAL_H__

#if defined(__SUPPORT_ATCMD__)
#include "app_aws_atcmd_parse.h"
#endif
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_common_support.h"
#include "app_sample_manager.h"
#include "core_mqtt_serializer.h"
#include "mqtt_pkcs11_demo_helpers.h"
#include "app_aws_user_conf.h"
#include "ota_update.h"

#define DEFAULT_PING_IP                             "8.8.8.8" /* dns.google */
#define APP_NVRAM_CONFIG_PING_CHECK                 "APP_PINGCHECK" ///< DPM running flag
#define APP_CONTROL_ATUPDATE_PREFIX                 "update"        ///< APP updating current state

struct atcmd_nvparams
{
    char *broker;
    uint32_t port;
    char *certCA;
    char *cert;
    char *key;
};

extern int32_t gShadowCount;
#if defined(__SUPPORT_ATCMD__)
extern st_atcmd_jsonStruct_t registeredShadow[];
#endif

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

void load_aws_credentials(void);
void free_aws_credentials(void);
char *aws_credentials_read(uint32_t addr);

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

#if defined(__SUPPORT_ATCMD__)
/**
 ****************************************************************************************
 * @brief set at-command status for platform
 * @param[in] _val status
 ****************************************************************************************
 */
void pal_app_at_command_status(atcmd_scan_result_t _val);
#endif

/**
 ****************************************************************************************
 * @brief set provisioning type of the application
 ****************************************************************************************
 */
void pal_app_set_prov_feature(void);

#if defined(__SUPPORT_OTA__)
/**
 ****************************************************************************************
 * @brief checking whether the OTA is for MCU image
 * @param[in] _url url of OTA
 * @param[in] config configuration structure for OTA
 * @return 0 :Not MCU OTA , 1 : MCU OTA
 ****************************************************************************************
 */
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
#endif  /* __SUPPORT_OTA__ */

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

/**
 ****************************************************************************************
 * @brief get broker url
 * @param[in] broker default broker url from application
 * @return real broker url
 ****************************************************************************************
 */
char *pal_app_get_broker_endpoint(char *broker);

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
extern void app_aws_dpm_app_connect(app_dpm_info_rtm *_rtmData);
extern MQTTStatus_t app_get_mqtt_status(void);
extern void app_set_mqtt_status(MQTTStatus_t rc);
extern MQTTContext_t* app_get_mqtt_context(void);
extern char* app_get_ota_url(void);
extern char* app_get_ota_result(void);
#if defined(__SUPPORT_ATCMD__)
extern void app_iot_at_command_status(atcmd_scan_result_t _val);
#endif
#endif /* __APP_ATCOMMAND_PAL_H__ */
