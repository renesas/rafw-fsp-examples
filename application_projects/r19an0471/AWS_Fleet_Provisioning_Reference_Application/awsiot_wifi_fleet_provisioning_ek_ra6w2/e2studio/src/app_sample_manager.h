/***********************************************************************************************************************
 * File Name    : app_sample_manager.h
 * Description  : Network related defines & declarations used to operate device in DPM mode on platform.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "stdbool.h"
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"

#if !defined(_APP_SAMPLE_Manager_H_)
#define _APP_SAMPLE_Manager_H_

/*! type define app mutex*/
typedef SemaphoreHandle_t app_mutex;

#if defined ( __SUPPORT_AWS_IOT_W__ ) 
/**
 ***************************************************************************************
 * @brief Fleet Provisioning ThingID define
 ****************************************************************************************
 */
#define FP_DEMO_ID_SUFFIX                           "Renesas_DoorLockID"
#endif  // ( __SUPPORT_AWS_IOT_W__ ) 

/*! memory malloc API for APP  */
#define app_common_malloc(x) pvPortMalloc(x)
/*! memory free API for APP  */
#define app_common_free(x) vPortFree(x)

enum e_fp_topics
{
    E_FP_JSON_REGISTER_PUBLISH_TOPIC,
    E_FP_JSON_REGISTER_ACCEPTED_TOPIC,
    E_FP_JSON_REGISTER_REJECTED_TOPIC,
    E_FP_CBOR_REGISTER_PUBLISH_TOPIC,
    E_FP_CBOR_REGISTER_ACCEPTED_TOPIC,
    E_FP_CBOR_REGISTER_REJECTED_TOPIC
};

/**
 ****************************************************************************************
 * @brief get app thing name for AWS or Azure sever
 * @param[in] void
 * @return thing name string
 ****************************************************************************************
 */
char* getAppThingName(void);
void app_common_set_time_sync_from_normal_sntp(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval);
void app_common_set_time_sync_from_fast_sntp_with_nvram(UINT32 _sntpCount, UINT8 _sleepMode, UINT32 _sleepInterval,
                                                        unsigned long long added_sleep_dur);
void set_persistant_storage_cb(void);
int32_t app_common_goto_sleep_1_or_2(APPSleepMode _sleepMode, UINT8 _useRTM, INT32 _wakeupIntervalSec, UINT8 _timeSave);
void aws_shadow_dpm_auto_start(void *arg);
void load_aws_credentials(void);
void free_aws_credentials(void);
char *aws_credentials_read(uint32_t addr);
int prvFleetProvisioningTask(void);
void connect_to_aws_cloud(void);
char *get_fleet_provisioning_client_identifier(void);
char *get_fleet_provisioning_csr_subject_name(void);
char *get_aws_broker_url(void);
int get_aws_mqtt_broker_port(void);
char *get_fleet_provisioning_topics(uint8_t topic_type);
bool validate_fleet_provisioning_nvram_config(void);

#endif /* _APP_SAMPLE_Manager_H_ */

/* EOF */
