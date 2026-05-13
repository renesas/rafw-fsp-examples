/**
 ****************************************************************************************
 *
 * @file app_dpm_sample.c
 *
 * @brief Define the DPM sample running on Apps module
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

#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "app_provision.h"
#include "app_azure_user_conf.h"
#include "app_thing_manager.h"
#include "app_dpm_thread.h"
#include "app_dpm_interface.h"
#include "app_common_support.h"
#include "azure_iot_log.h"
#include "ota_update.h"
#include "json.h"
#include "net_common.h"

#include "iothub.h"
#include "iothub_device_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_security_factory.h"

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "parson.h"
//#include "../../../../app_common/include/app_atcommand/app_atcommand_pal.h"
#include "app_atcommand_pal.h"
#include "upss.h"
#include "rm_map_persistant_w.h"
#include "app_dpm_thread.h"
#include "ra6w1_platform_nvparam.h"

//#define SAMPLE_MQTT
#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#undef printf
//#define printf	PRINTF
#define SET_TRUSTED_CERT_IN_SAMPLES
#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
    #include "azure_prov_client/prov_transport_mqtt_client.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "iothubtransportmqtt_websockets.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS

#define SNTP_TRY_COUNT						15
#define MAX_URL_LEN							256

//azure [[:
#define AWS_OTA_RESULT_NG						1 ///< NG for OTA update
//azure]]

IOTHUB_DEVICE_CLIENT_HANDLE iotHubClientHandle;

static char conStrBuffer[250];

#if (ENABLE_AZURE_DPS_EXAMPLE == 1)
static char *nvConnectionStr = NULL;

static void registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void *user_context);
static void register_device_callback(PROV_DEVICE_RESULT register_result, const char *iothub_uri
    , const char *device_id, void *user_context);
static char *app_provisioning_device_service(void);
#endif
static void reportedStateCallback(int status_code, void *userContextCallback);
static void create_connection_string(char *strBuf, char *iothubURI, char *devID,
        char *key);

#if defined(__SUPPORT_OTA__)
static void setOtaFileUrl(bool desiredMode, int msgPayloadLen, void *msgPayload);
static UINT32 azure_ota_fw_update(char *_fw_url);
#endif

typedef struct CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char *iothub_uri;
    char *access_key_name;
    char *device_key;
    char *device_id;
    int registration_complete;
} CLIENT_SAMPLE_INFO;

static bool controlFlag = false;
static bool connectFlag = false;
static bool updateSensorFlag = false;
static int doorLock = Door_Idle; // 0 = idle, 1 = open, 2 = close, 3 = auto close by timer 10 sec.
static bool doorOpenFlag = false;
static bool doorBellFlag = false;
static int doorStateChange = 0;              // int 1 = changed, 0 = not changed
static int doorOpenMode = 0;                        // 0 = auto, 1 = manual
static char currentOTAResult[20] = { 0, };  // "OTA_OK", "OTA_NG", "OTA_UNKNOWN"

static char azure_file_version[20] = { 0, };
static char azure_file_url[MAX_URL_LEN] = { 0, };
static char ota_path_url_rtos[MAX_URL_LEN] = { 0, };
#if defined (__BLE_COMBO_REF__)
static char ota_path_url_ble[MAX_URL_LEN] = {0, };
extern int wifi_netif_status(int intf);
extern uint32_t wifi_svc_get_provisioning_flag(void);
#endif

static size_t g_message_count_send_confirmations = 0;

// names of optional custom properties that may be specified by the service to C2D messages
const char *applicationCustomPropertyKey_1 = "appCustomProperty_1";
const char *applicationCustomPropertyKey_2 = "appCustomProperty_2";

// The Azure IoT C SDK allows developers to acknowledge cloud-to-device messages
// (either sending DISPOSITION if using AMQP, or PUBACK if using MQTT)
// outside the following callback
// , in a separate function call.
// This would allow the user application to process the incoming message before acknowledging
// it to the Azure IoT Hub, since
// the callback below is supposed to return as fast as possible to unblock I/O.
// Please look for "IOTHUBMESSAGE_ASYNC_ACK" in iothub_ll_c2d_sample for more details.
static IOTHUBMESSAGE_DISPOSITION_RESULT receive_msg_callback(
        IOTHUB_MESSAGE_HANDLE message, void *user_context)
{
    (void) user_context;
    const char *messageId;
    const char *correlationId;
    const char *contentType;
    const char *contentEncoding;
    const char *messageCreationTimeUtc;
    const char *messageUserId;

    const char *applicationCustomPropertyValue1;
    const char *applicationCustomPropertyValue2;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<unavailable>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<unavailable>";
    }

    if ((contentType = IoTHubMessage_GetContentTypeSystemProperty(message))
            == NULL)
            {
        contentType = "<unavailable>";
    }

    if ((contentEncoding = IoTHubMessage_GetContentEncodingSystemProperty(
            message)) == NULL)
            {
        contentEncoding = "<unavailable>";
    }

    if ((messageCreationTimeUtc =
            IoTHubMessage_GetMessageCreationTimeUtcSystemProperty(message))
            == NULL)
            {
        messageCreationTimeUtc = "<unavailable>";
    }

    if ((messageUserId = IoTHubMessage_GetMessageUserIdSystemProperty(message))
            == NULL)
            {
        messageUserId = "<unavailable>";
    }

    if ((applicationCustomPropertyValue1 = IoTHubMessage_GetProperty(message,
            applicationCustomPropertyKey_1)) == NULL)
            {
        applicationCustomPropertyValue1 = "<unavailable>";
    }

    if ((applicationCustomPropertyValue2 = IoTHubMessage_GetProperty(message,
            applicationCustomPropertyKey_2)) == NULL)
            {
        applicationCustomPropertyValue2 = "<unavailable>";
    }

    IOTHUBMESSAGE_CONTENT_TYPE content_type = IoTHubMessage_GetContentType(
            message);
    if (content_type == IOTHUBMESSAGE_BYTEARRAY)
    {
        const unsigned char *buff_msg;
        size_t buff_len;

        if (IoTHubMessage_GetByteArray(message, &buff_msg, &buff_len)
                != IOTHUB_MESSAGE_OK)
                {
            (void) printf("Failure retrieving byte array message\r\n");
        } else
        {
            (void) printf("Received Binary message.\r\n  Data: <<<%.*s>>> & Size=%d\r\n"
                    , (int)buff_len, buff_msg, (int)buff_len);
        }
    } else
    {
        const char *string_msg = IoTHubMessage_GetString(message);
        if (string_msg == NULL)
        {
            (void) printf("Failure retrieving byte array message\r\n");
        } else
        {
            (void) printf("Received String Message.\r\n  Data: <<<%s>>>\r\n", string_msg);
        }
    }

    // printf("Message properties:\r\n  Message ID: %s\r\n  Correlation ID: %s\r\n"
    // 		"  ContentType: %s\r\n  ContentEncoding: %s\r\n"
    // 		"  messageCreationTimeUtc: %s\r\n  messageUserId: %s\r\n  %s: %s\r\n %s: %s\r\n",
    // 		messageId, correlationId, contentType, contentEncoding, messageCreationTimeUtc
    // 		, messageUserId, applicationCustomPropertyKey_1, applicationCustomPropertyValue1
    // 		, applicationCustomPropertyKey_2
    // 		, applicationCustomPropertyValue2);
//azure[[:
    if (RM_PMGR_W_dpm_is_wakeup() && app_dpm_get_recv_timeout_flag() == DPM_RCV_OK_CONNECT)
    {
//azure]]
        app_dpm_set_recv_timeout_flag(DPM_RCV_OK_SLEEP);
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

void app_set_control_flag(bool flag)
{
    controlFlag = flag;
    return;
}

bool app_get_control_flag(void)
{
    return controlFlag;
}

char* app_get_ota_result(void)
{
    return currentOTAResult;
}

void app_send_twins(char *twins_what)
{
    Doorlock doorlock_sample;
    char *reportedProperties;

    reportedProperties = palATDoorlockSerializeToJson(&doorlock_sample);
    printf("Report =>(%s) %s \r\n", twins_what, reportedProperties);
    (void) IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle,
            (const unsigned char*) reportedProperties,
            strlen(reportedProperties), reportedStateCallback, NULL);
}
static char* DoorlockSerializeToJson(Doorlock *doorlock)
{
    char *result;
    char *nvOtaFileVer;

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    memset(doorlock, 0x00, sizeof(Doorlock));

    // Only reported properties:
    doorlock->temperature = 23;
    (void) json_object_set_number(root_object, "temperature",
            doorlock->temperature);

    doorlock->battery = 100;
    (void) json_object_set_number(root_object, "battery", doorlock->battery);

    doorlock->doorStateChange = doorStateChange;
    (void) json_object_set_number(root_object, "doorStateChange",
            doorlock->doorStateChange);

    if (controlFlag == true)
    {
        if (doorLock == Door_Close || doorLock == Door_Open)
        {
            doorlock->openMethod = AZURE_RESPONSE_OPEN_APP;
        }
    } else
    {
        doorlock->openMethod = AZURE_RESPONSE_NOT_OPEN;
    }
    (void) json_object_set_string(root_object, "openMethod",
            doorlock->openMethod);

    doorlock->doorState = doorOpenFlag;
    (void) json_object_set_boolean(root_object, "doorState",
            doorlock->doorState);

    doorlock->doorBell = doorBellFlag;
    (void) json_object_set_boolean(root_object, "doorBell", doorlock->doorBell);

    doorlock->doorOpenMode = doorOpenMode;
    (void) json_object_set_number(root_object, "doorOpenMode",
            doorlock->doorOpenMode);

    doorlock->otaUpdate = (int) getOTAStat();
    (void) json_object_set_number(root_object, "OTAupdate",
            doorlock->otaUpdate);

    if (!strcmp(currentOTAResult, (char*) AZURE_UPDATE_OTA_OK))
    {
        doorlock->otaResult = AZURE_UPDATE_OTA_OK;
    } else if (!strcmp(currentOTAResult, (char*) AZURE_UPDATE_OTA_NG))
    {
        doorlock->otaResult = AZURE_UPDATE_OTA_NG;
    } else
    {
        doorlock->otaResult = AZURE_UPDATE_OTA_UNKNOWN;
    }
    (void) json_object_set_string(root_object, "OTAresult",
            doorlock->otaResult);

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_CURRENT_OTA_VERSION, &nvOtaFileVer);
    if (nvOtaFileVer == NULL)
    {
        doorlock->otaVersion = AZURE_REPORT_VERSION_NONE;
    } else
    {
        doorlock->otaVersion = nvOtaFileVer;
    }
    (void) json_object_set_string(root_object, "OTAversion",
            doorlock->otaVersion);

    result = json_serialize_to_string(root_value);

    json_value_free(root_value);

    if (doorStateChange)
    {
        doorStateChange = 0;
    }

    return result;
}

static int deviceMethodCallback(const char *method_name,
        const unsigned char *payload, size_t size, unsigned char **response,
        size_t *response_size, void *userContextCallback)
{
    (void) userContextCallback;
    (void) payload;
    (void) size;

    int result = 0;

    uint32_t resPayloadLen = 0;
    char resPayload[32] = { 0, };

    Doorlock doorlock_sample;
    char *reportedProperties;

    snprintf(resPayload, size, "%s", payload);
    resPayload[size] = 0;
    printf("method_name: %s, Command payload : %d, %s ", method_name, size,
            payload);
    memset(resPayload, 0x00, 32);

    if ((pal_app_event_cb(payload, size, response, response_size, (int32_t*)&result,
            &connectFlag)) != 0)
    {
        return result;
    } else
    {
        if (strncmp((char*) payload + 1, AZURE_CONTROL_OPEN,
                strlen(AZURE_CONTROL_OPEN)) == 0)
                {
            printf("\n[%s:%d]open comm\n",__func__,__LINE__);
            resPayloadLen = strlen(AZURE_RESPONSE_OPEN) + 2;
            snprintf(resPayload, resPayloadLen + 1, "\"%s\"",
                    AZURE_RESPONSE_OPEN);
            *response_size = strlen(resPayload);
            *response = malloc(*response_size);
            (void) memcpy(*response, resPayload, *response_size);
            result = 200;
            controlFlag = true;
            doorLock = Door_Open;
            doorStateChange = 1;
            doorOpenFlag = true;
        } else if (strncmp((char*) payload + 1, AZURE_CONTROL_CLOSE,
                strlen(AZURE_CONTROL_CLOSE)) == 0 /*&& controlFlag == false*/)
        {
            printf("\n[%s:%d]close comm\n",__func__,__LINE__);
            resPayloadLen = strlen(AZURE_RESPONSE_CLOSE) + 2;
            snprintf(resPayload, resPayloadLen + 1, "\"%s\"",
                    AZURE_RESPONSE_CLOSE);

            *response_size = strlen(resPayload);
            *response = malloc(*response_size);
            (void) memcpy(*response, resPayload, *response_size);
            result = 200;
            controlFlag = true;
            doorLock = Door_Close;
            doorStateChange = 1;
            doorOpenFlag = false;
        } else if (strncmp((char*) payload + 1, AZURE_CONTROL_OTA,
                strlen(AZURE_CONTROL_OTA)) == 0)
        {
            printf("confirmOTA comm");
            setOTAStat(OTA_STAT_JOB_CONFIRMED);

#ifdef RM_MAP_PERSISTANT_W
                if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                        AZURE_NVRAM_CONFIG_OTA_STATE, AZURE_OTA_STATE_UNKOWN))
#endif
                {
                printf("write AZURE_NVRAM_CONFIG_OTA_STATE failed\n");
                }
        } else if (strncmp((char*) payload + 1, AZURE_CONTROL_SENSOR,
                strlen(AZURE_CONTROL_SENSOR)) == 0)
        {
            printf("updateSensor comm");
            resPayloadLen = strlen(AZURE_RESPONSE_SENSOR_UPDATE) + 2;
            snprintf(resPayload, resPayloadLen + 1, "\"%s\"",
                    AZURE_RESPONSE_SENSOR_UPDATE);

            *response_size = strlen(resPayload);
            *response = malloc(*response_size);
            (void) memcpy(*response, resPayload, *response_size);
            result = 200;
            updateSensorFlag = true;
            doorLock = Door_Idle;
        } else if (strncmp((char*) payload + 1, AZURE_CONTROL_CONNECT,
                strlen(AZURE_CONTROL_CONNECT))
                == 0 /* && connectFlag == false */)
        {
            printf("connect comm \n");
            resPayloadLen = strlen(AZURE_RESPONSE_CONNECT) + 2;
            snprintf(resPayload, resPayloadLen + 1, "\"%s\"",
                    AZURE_RESPONSE_CONNECT);

            *response_size = strlen(resPayload);
            *response = malloc(*response_size);
            (void) memcpy(*response, resPayload, *response_size);
            result = 200;
            doorLock = Door_Idle;
            connectFlag = true;
        } else
        {
            printf("unsupported comm");
            const char deviceMethodResponse[] = "{ }";
            *response_size = sizeof(deviceMethodResponse) - 1;
            *response = malloc(*response_size);
            (void) memcpy(*response, deviceMethodResponse, *response_size);
            result = -1;
        }

        printf("\n[%s:%d]Command response : %.*s \r\n", __func__, __LINE__,*response_size, *response);

        if (result != -1)
        {
            reportedProperties = DoorlockSerializeToJson(&doorlock_sample);
            printf("[%s:%d]Report => %s \r\n", __func__, __LINE__,reportedProperties);
            (void) IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle,
                    (const unsigned char*) reportedProperties,
                    strlen(reportedProperties), reportedStateCallback, NULL);
            if (reportedProperties != NULL)
            {
                free(reportedProperties);
            }
        }
    }
    return result;
}

static void getCompleteDeviceTwinOnDemandCallback(
        DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char *payLoad,
        size_t size, void *userContextCallback)
{
    (void) update_state;
    (void) userContextCallback;

    // printf("GetTwinAsync result === \r\n%.*s\r\n", (int)size, payLoad);
}

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result,
        void *userContextCallback)
        {
    (void) userContextCallback;
    // When a message is sent this callback will get invoked
    g_message_count_send_confirmations++;
    (void) printf("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));

    app_dpm_put_to_sleep();
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE update_state,
        const unsigned char *payLoad, size_t size, void *userContextCallback)
{
    (void) update_state;
    (void) size;
    (void) userContextCallback;
    Doorlock doorlock_sample;
    char *reportedProperties;

    printf("deviceTwinCallback payload:\r\n%.*s\r\n\n", (int)size, payLoad);

#if defined(__SUPPORT_OTA__)
    setOtaFileUrl(FALSE, (int) size + 1, (void*) payLoad);
#endif	

    // Initialize the doorlock structure to avoid garbage data
    memset(&doorlock_sample, 0x00, sizeof(Doorlock)); //CH
    if ((reportedProperties = palATDoorlockSerializeToJson(&doorlock_sample))
            == 0)
        reportedProperties = DoorlockSerializeToJson(&doorlock_sample);

    printf("[%s:%d]Report => %s \r\n", __func__,__LINE__,reportedProperties);
    (void) IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle,
            (const unsigned char*) reportedProperties,
            strlen(reportedProperties), reportedStateCallback, NULL);
    if (reportedProperties != NULL)
    {
        free(reportedProperties);
    }
}

static void reportedStateCallback(int status_code, void *userContextCallback)
{
    (void) userContextCallback;
    printf("Device Twin reported properties update completed with result: %d\r\n", status_code);

#if defined(__SUPPORT_OTA__)
    if (getOTAStat() == OTA_STAT_JOB_CONFIRMED)
    {
        // reboot agaist insufficient memory	
        INT32 flag = 1;
        INT32 rcode = 0;
        IOT_INFO("URL for updating: \"%s\"", azure_file_url);
        IOT_INFO("save URL info & reboot for OTA");
        OAL_MSLEEP(30);

#ifdef RM_MAP_PERSISTANT_W
        rcode = RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                AZURE_NVRAM_CONFIG_OTA_FLAG, flag);
#endif
            OAL_MSLEEP(30);
        if (rcode)
        {
            IOT_ERROR("write NVRAM_CONFIG_OTA_FLAG failed => force to sleep");
            OAL_MSLEEP(30);
        }

        reboot_func(SYS_REBOOT_POR);

        /* Wait for system-reboot */
        while (1)
        {
            OAL_MSLEEP(100);
        }
    }
#endif		
    //azuredpmwork[[: ready to DPM sleep cause PUB ACK received from server
//azure[[:
    UINT8 flagRcvTimeout;
    flagRcvTimeout = app_dpm_get_recv_timeout_flag();
    if (RM_PMGR_W_dpm_is_enabled() && flagRcvTimeout == DPM_RCV_OK_CONNECT)
    {
//azure]]
        app_dpm_set_recv_pub_ack();
        app_dpm_set_recv_timeout_flag(DPM_RCV_OK_SLEEP);
//		printf("\n\t ***** ble_stop *****\t\na");
        if(!RM_PMGR_W_dpm_is_wakeup())
        {
             cmd_ble_stop_svc();
        }
    }
    //]]

}

void azure_dpm_app_init(void)
{
    INT32 rcode;
    char *nvVersionToBeApplied = NULL;

    /* add OTA result check */
    memset(currentOTAResult, 0, sizeof(currentOTAResult));
#ifdef RM_MAP_PERSISTANT_W
    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_RESULT, &rcode))
#endif
        {
        snprintf(currentOTAResult, strlen(AZURE_UPDATE_OTA_UNKNOWN) + 1, "%s",
                AZURE_UPDATE_OTA_UNKNOWN);
        printf("read AZURE_NVRAM_CONFIG_OTA_RESULT init status");
    } else
    {
        if (rcode == AZURE_OTA_RESULT_OK)
        {
            snprintf(currentOTAResult, strlen(AZURE_UPDATE_OTA_OK) + 1, "%s",
                    AZURE_UPDATE_OTA_OK);
            printf("AZURE_UPDATE_OTA_OK ");
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),ENV_GROUP_APPCFG,
                            AZURE_NVRAM_CONFIG_OTA_VERSION, &nvVersionToBeApplied);
#endif
                if (strlen(nvVersionToBeApplied > 0))
                {
#ifdef RM_MAP_PERSISTANT_W
                rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                        ENV_GROUP_APPCFG,
                        AZURE_NVRAM_CONFIG_CURRENT_OTA_VERSION,
                        nvVersionToBeApplied);
#endif
                OAL_MSLEEP(30);
                if (rcode)
                {
                    printf(
                            "write AZURE_NVRAM_CONFIG_OTA_VERSION failed => force to sleep");
                    OAL_MSLEEP(30);
                }
            }
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),ENV_GROUP_APPCFG,
                            AZURE_NVRAM_CONFIG_MCUOTA_VERSION, &nvVersionToBeApplied);
#endif
                if (strlen(nvVersionToBeApplied) > 0)
                {
#ifdef RM_MAP_PERSISTANT_W
                rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                        ENV_GROUP_APPCFG,
                        AZURE_NVRAM_CONFIG_CURRENT_MCUOTA_VERSION,
                        nvVersionToBeApplied);
#endif
                OAL_MSLEEP(30);
                if (rcode)
                {
                    printf(
                            "write AZURE_NVRAM_CONFIG_MCUOTA_VERSION failed => force to sleep");
                    OAL_MSLEEP(30);
                }
            }
        } else if (rcode == AZURE_OTA_RESULT_NG)
        {
            snprintf(currentOTAResult, strlen(AZURE_UPDATE_OTA_NG) + 1, "%s",
                    AZURE_UPDATE_OTA_NG);
#ifdef RM_MAP_PERSISTANT_W
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_STATE,
                    AZURE_OTA_STATE_READY))
#endif
                printf("write AZURE_NVRAM_CONFIG_OTA_STATE for retry failed");
            printf("AZURE_UPDATE_OTA_NG");
        } else
        {
            snprintf(currentOTAResult, strlen(AZURE_UPDATE_OTA_UNKNOWN) + 1,
                    "%s", AZURE_UPDATE_OTA_UNKNOWN);
            printf("AZURE_UPDATE_OTA_UNKNOWN");
        }
    }
#ifdef RM_MAP_PERSISTANT_W
    if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_RESULT,
            AZURE_OTA_RESULT_UNKNOWN))
#endif
        printf("write AZURE_NVRAM_CONFIG_OTA_RESULT failed");

    /* add OTA status check */
#ifdef RM_MAP_PERSISTANT_W
    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(),ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_STATE, &rcode))
#endif
    {
        setOTAStat(OTA_STAT_JOB_NONE);
        printf("read AZURE_NVRAM_CONFIG_OTA_STATE init status");
    } else
    {
        if (rcode == AZURE_OTA_STATE_READY)
        {
            setOTAStat(OTA_STAT_JOB_READY);
            printf("OTA_STAT_JOB_READY");
        } else
        {
            setOTAStat(OTA_STAT_JOB_NONE);
            printf("OTA_STAT_JOB_NONE");
        }
    }
}

void azure_dpm_app_connect(app_dpm_info_rtm *_rtmData, char *conn_string)
{
    (void) _rtmData;
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
        protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS

    app_dpm_set_skip_saving_serverip(FALSE);

    if (IoTHub_Init() != 0)
    {
        printf("Failed to initialize the platform.");
    } else
    {
        if ((iotHubClientHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(
                conn_string, protocol)) == NULL)
                {
            printf("ERROR: iotHubClientHandle is NULL!");
        } else
        {
            bool urlEncodeOn = true;
            (void) IoTHubDeviceClient_LL_SetOption(iotHubClientHandle,
                    OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

            if (IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, "TrustedCerts",
                    defaultROOT_CA_PEM) != IOTHUB_CLIENT_OK)
                printf("failure to set option \"TrustedCerts\"");
            else
                printf("Root CA registered");

        }
    }
}

void azure_dpm_app_boot(app_dpm_info_rtm *_rtmData)
{
    (void) _rtmData;
    IOTHUB_MESSAGE_HANDLE message_handle;
    float telemetry_temperature;
    float telemetry_humidity;
    const char *telemetry_scale = "Celsius";
    char telemetry_msg_buffer[80];

    // Setting message callback to get C2D messages
    (void) IoTHubDeviceClient_LL_SetMessageCallback(iotHubClientHandle,
            receive_msg_callback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetMessageCallback()...\r\n");

    // Construct the iothub message
    telemetry_temperature = 20.0f
            + ((float) rand() / (float) RAND_MAX) * 15.0f;
    telemetry_humidity = 60.0f
            + ((float) rand() / (float) RAND_MAX) * 20.0f;

    sprintf(telemetry_msg_buffer,
            "{\"temperature\":%.3f,\"humidity\":%.3f,\"scale\":\"%s\"}",
            telemetry_temperature, telemetry_humidity, telemetry_scale);

    message_handle = IoTHubMessage_CreateFromString(telemetry_msg_buffer);

    // Set Message property
    (void) IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
    (void) IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
    (void) IoTHubMessage_SetContentTypeSystemProperty(message_handle,
            "application%2fjson");
    (void) IoTHubMessage_SetContentEncodingSystemProperty(message_handle,
            "utf-8");

    // Add custom properties to message
    (void) IoTHubMessage_SetProperty(message_handle, "property_key",
            "property_value");

    (void) printf("\r\nSending message %d to IoTHub\r\nMessage: %s\r\n", 1, telemetry_msg_buffer);
    IoTHubDeviceClient_LL_SendEventAsync(iotHubClientHandle, message_handle,
            send_confirm_callback, NULL);

    //api_debug
    //normal scenario reached here
    // (void) IoTHubDeviceClient_LL_GetTwinAsync(iotHubClientHandle,
    // 		getCompleteDeviceTwinOnDemandCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_GetTwinAsync()...status = %d \r\n",IoTHubDeviceClient_LL_GetTwinAsync(iotHubClientHandle,
            getCompleteDeviceTwinOnDemandCallback, NULL));
    (void) printf("IoTHubDeviceClient_LL_SetDeviceMethodCallback()...status = %d...line = %d....\r\n",__LINE__,IoTHubDeviceClient_LL_SetDeviceMethodCallback(iotHubClientHandle,
            deviceMethodCallback, NULL));
    (void) printf("IoTHubDeviceClient_LL_SetDeviceTwinCallback()...status = %d\r\n",IoTHubDeviceClient_LL_SetDeviceTwinCallback(iotHubClientHandle,
            deviceTwinCallback, NULL));
    while(1)
    {
        IoTHubClientCore_LL_DoWork(iotHubClientHandle);
        vTaskDelay(10);
    }
    IoTHubMessage_Destroy(message_handle);
}

void azure_dpm_app_sensor_wakeup(app_dpm_info_rtm *_rtmData)
{
    (void) _rtmData;

    IOTHUB_MESSAGE_HANDLE message_handle;
    float telemetry_temperature;
    float telemetry_humidity;
    const char *telemetry_scale = "Celsius";
    char telemetry_msg_buffer[80];

    // Setting message callback to get C2D messages
    (void) IoTHubDeviceClient_LL_SetMessageCallback(iotHubClientHandle,
            receive_msg_callback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetMessageCallback()...\r\n");

    // Construct the iothub message
    telemetry_temperature = 20.0f
            + ((float) rand() / (float) RAND_MAX) * 15.0f;
    telemetry_humidity = 60.0f
            + ((float) rand() / (float) RAND_MAX) * 20.0f;

    sprintf(telemetry_msg_buffer,
            "{\"temperature\":%.3f,\"humidity\":%.3f,\"scale\":\"%s\"}",
            telemetry_temperature, telemetry_humidity, telemetry_scale);

    message_handle = IoTHubMessage_CreateFromString(telemetry_msg_buffer);

    // Set Message property
    (void) IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
    (void) IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
    (void) IoTHubMessage_SetContentTypeSystemProperty(message_handle,
            "application%2fjson");
    (void) IoTHubMessage_SetContentEncodingSystemProperty(message_handle,
            "utf-8");

    // Add custom properties to message
    (void) IoTHubMessage_SetProperty(message_handle, "property_key",
            "property_value");

    (void) printf("\r\nSending message %d to IoTHub\r\nMessage: %s\r\n", 1, telemetry_msg_buffer);
    IoTHubDeviceClient_LL_SendEventAsync(iotHubClientHandle, message_handle,
            send_confirm_callback, NULL);

    while(1)
    {
        IoTHubClientCore_LL_DoWork(iotHubClientHandle);
        vTaskDelay(10);
    }
    // The message is copied to the sdk so the we can destroy it
    IoTHubMessage_Destroy(message_handle);
}

void azure_dpm_app_timer_wakeup(app_dpm_info_rtm *_rtmData)
{
    (void) _rtmData;
    Doorlock doorlock_sample;
    char *reportedProperties;
    IOTHUB_MESSAGE_HANDLE message_handle;
    float telemetry_temperature;
    float telemetry_humidity;
    const char *telemetry_scale = "Celsius";
    char telemetry_msg_buffer[80];

    // Setting message callback to get C2D messages
    (void) IoTHubDeviceClient_LL_SetMessageCallback(iotHubClientHandle,
            receive_msg_callback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetMessageCallback()...\r\n");

    //		Construct the iothub message
    telemetry_temperature = 20.0f
            + ((float) rand() / (float) RAND_MAX) * 15.0f;
    telemetry_humidity = 60.0f
            + ((float) rand() / (float) RAND_MAX) * 20.0f;

    sprintf(telemetry_msg_buffer,
            "{\"temperature\":%.3f,\"humidity\":%.3f,\"scale\":\"%s\"}",
            telemetry_temperature, telemetry_humidity, telemetry_scale);

    message_handle = IoTHubMessage_CreateFromString(telemetry_msg_buffer);

    // for api_testing
    // const unsigned char payload[] = {0x4D, 0x61, 0x6E, 0x67, 0x6F, 0x65, 0x73};
    // message_handle = IoTHubMessage_CreateFromByteArray(payload, sizeof(payload));

    // Set Message property
    (void) IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
    (void) IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
    (void) IoTHubMessage_SetContentTypeSystemProperty(message_handle,
            "application%2fjson");
    (void) IoTHubMessage_SetContentEncodingSystemProperty(message_handle,
            "utf-8");

    if (IoTHubMessage_SetConnectionDeviceId(message_handle, APP_USER_MY_THING_NAME) != IOTHUB_MESSAGE_OK)
    {
        printf("Warning: failed to set connection-device-id property (continuing)\n");
    }

    // Add custom properties to message
    (void) IoTHubMessage_SetProperty(message_handle, "property_key",
            "property_value");

    (void) printf("\r\nSending message %d to IoTHub\r\nMessage: %s\r\n", 1, telemetry_msg_buffer);
    IoTHubDeviceClient_LL_SendEventAsync(iotHubClientHandle, message_handle,
            send_confirm_callback, NULL);


    (void) IoTHubDeviceClient_LL_GetTwinAsync(iotHubClientHandle,
            getCompleteDeviceTwinOnDemandCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_GetTwinAsync()...\r\n");

    if ((reportedProperties = palATDoorlockSerializeToJson(&doorlock_sample))
            == 0)
        reportedProperties = DoorlockSerializeToJson(&doorlock_sample);

    (void) printf("Report => %s \r\n", reportedProperties);
    //org
    //(void)IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle, (const unsigned char*)reportedProperties, strlen(reportedProperties), reportedStateCallback, NULL);
    //azuredpmwork[[: no callback for shortening running time
    (void) IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle,
            (const unsigned char*) reportedProperties,
            strlen(reportedProperties), NULL, NULL);
    //]]
    (void) printf("IoTHubDeviceClient_LL_SendReportedState()...\r\n");

    (void) IoTHubDeviceClient_LL_SetDeviceMethodCallback(iotHubClientHandle,
            deviceMethodCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetDeviceMethodCallback()...line = %d....\r\n",__LINE__);

    (void) IoTHubDeviceClient_LL_SetDeviceTwinCallback(iotHubClientHandle,
            deviceTwinCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetDeviceTwinCallback()...\r\n");
    
    while(1)
    {
        IoTHubClientCore_LL_DoWork(iotHubClientHandle);
        vTaskDelay(10);
    }
    IoTHubMessage_Destroy(message_handle);
    app_dpm_put_to_sleep();
}

void azure_dpm_app_recv(app_dpm_info_rtm *_rtmData)
{
    (void) _rtmData;
    (void) IoTHubDeviceClient_LL_SetDeviceMethodCallback(iotHubClientHandle,
            deviceMethodCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetDeviceMethodCallback()...line = %d....\r\n",__LINE__);

    //dpm_debug reached here
    (void) IoTHubDeviceClient_LL_GetTwinAsync(iotHubClientHandle,
            getCompleteDeviceTwinOnDemandCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_GetTwinAsync()...\r\n");


    (void) IoTHubDeviceClient_LL_SetDeviceTwinCallback(iotHubClientHandle,
            deviceTwinCallback, NULL);
    (void) printf("IoTHubDeviceClient_LL_SetDeviceTwinCallback()...\r\n");
    while(1)
    {
            IoTHubClientCore_LL_DoWork(iotHubClientHandle);
            vTaskDelay(10);
        }
}

void azure_nodpm_app_work(app_dpm_info_rtm *_rtmData)
{
    (void) _rtmData;
    Doorlock doorlock_sample;
    char *reportedProperties;

    memset(&doorlock_sample, 0x00, sizeof(Doorlock));//CH
    while (1)
    {
        if ((reportedProperties = palATDoorlockSerializeToJson(&doorlock_sample))
                == 0)
            reportedProperties = DoorlockSerializeToJson(&doorlock_sample);
        printf("reportedProperties : %s \n", reportedProperties);
        if (reportedProperties != NULL)
        {
                (void) IoTHubDeviceClient_LL_SendReportedState(iotHubClientHandle,
                        (const unsigned char*) reportedProperties,
                        strlen(reportedProperties), reportedStateCallback, NULL);
                // Free the allocated memory to prevent memory leak
                free(reportedProperties);
                reportedProperties = NULL;
        } else
        {
            printf("[%s:%d]Error: Failed to serialize doorlock to JSON\r\n", __func__, __LINE__);
        }
        vTaskDelay(3000); //sleep in 3000msec
    }

    IoTHubDeviceClient_LL_Destroy(iotHubClientHandle);
    IoTHub_Deinit();
}

void create_connection_string(char *strBuf, char *iothubURI, char *devID,
        char *key)
{
    memset(strBuf, 0x00, sizeof(strBuf));
    strcat(strBuf, (const char*) HOST_NAME_HEADER);
    strcat(strBuf, iothubURI);
    strcat(strBuf, ";");
    strcat(strBuf, (const char*) DEVICE_ID_HEADER);
    strcat(strBuf, devID);
    strcat(strBuf, ";");
    strcat(strBuf, (const char*) SHARED_KEY_HEADER);
    strcat(strBuf, key);

    printf("connection string : %s ", strBuf);
}

void DPM_App_Main(UINT32 _data, UINT32 _rtmData, DM_NOTI _status)
{
    (void) _data;
    app_dpm_info_rtm *rtmData = (app_dpm_info_rtm*) _rtmData;

    switch (_status)
    {
    case DM_INIT:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_INIT \n");
        pal_app_get_atcmd_parameter();
#if (ENABLE_AZURE_DPS_EXAMPLE == 1)
#ifdef RM_MAP_PERSISTANT_W
            //device provisioning service
        RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_NVRAM_DEVICE_CONNECTION_STRING, &nvConnectionStr);
#endif

            if(nvConnectionStr == NULL)
            {						
                nvConnectionStr = app_provisioning_device_service();
                if(nvConnectionStr == NULL)
                {					
                    printf("failed dps device registration ");
                }					
            }			
            printf("NVRAM DPS device connection string : %s ", nvConnectionStr);
#endif			
        azure_dpm_app_init();
#if defined(__SUPPORT_ATCMD__)
        pal_app_at_command_status(ATCMD_Status_STA_start);
#endif

    }
        break;
    case DM_NEED_CONNECTION:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_NEED_CONNECTION \n");

#if (ENABLE_AZURE_DPS_EXAMPLE == 1)
            azure_dpm_app_connect(rtmData, nvConnectionStr);
#else

        create_connection_string(conStrBuffer, getAppHostName(),
                getAppThingName(), getAppDevPrimaryKey());
        azure_dpm_app_connect(rtmData, conStrBuffer);
#endif			
    }
        break;
    case DM_CHECK_RECV:
    {

    }
        break;
    case DM_WAKEUP_RECV:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_WAKEUP_RECV \n");

        azure_dpm_app_recv(rtmData);
    }
        break;
    case DM_WAKEUP_SENSOR:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_WAKEUP_SENSOR \n");
        pal_app_atc_work(rtmData);
        azure_dpm_app_sensor_wakeup(rtmData);
    }
        break;
    case DM_WAKEUP_TIMER:	// if opened, timer check for close
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_WAKEUP_TIMER \n");
        pal_app_atc_work(rtmData);
        azure_dpm_app_timer_wakeup(rtmData);
    }
        break;
    case DM_WAKEUP_BOOT:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_WAKEUP_BOOT \n");
        pal_app_atc_work(rtmData);
        azure_dpm_app_boot(rtmData);
    }
        break;
    case DM_EXTERNAL_SENSOR:
    {
    }
        break;
    case DM_FINISH_DEVICE:
    {
        app_print_elapse_time_ms("[%s:%d]", __func__, __LINE__);
        IOT_INFO("DM_FINISH_DEVICE \n");
#if defined(__SUPPORT_ATCMD__)
        pal_app_at_command_status(ATCMD_Status_STA_done);
#endif

    }
        break;

    case DM_NO_DPM_MODE:
    {
        IOT_INFO("DM_NO_DPM_MODE \n");

        azure_nodpm_app_work(rtmData);

    }
        break;

    case DM_CHECK_DEVICE:
    {
        IOT_INFO("DM_CHECK_DEVICE \n");

        /* check communication with the peer server */
        //azure_dpm_app_timer_wakeup(rtmData);
        /* just sleep without resetting KA timer */
        goSleepAppDpmThread(0, DPM_RTC_NORMAL_MODE);

    }
        break;

    default:
    {
    }
        break;
    }

}

void azure_dpm_app_sample_thread(void)
{
    int sleepMode = 0;
    int rtcTime = 0;
    unsigned char useRTM = FALSE;
    INT32 rcode = 0;
    INT32 flagOTA = 0;
    char *tmp_url = NULL;
    UINT32 my_port = UNDEF_PORT;

    dpmAPPDataQ sampleData;

    //check OTA

#ifdef RM_MAP_PERSISTANT_W
    rcode = RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                     AWSIOT_CFG_OTA_FLAG, (int*)&flagOTA);
#endif
    if (!rcode && (flagOTA == 1))
    {
#ifdef RM_MAP_PERSISTANT_W
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                			AWSIOT_CFG_OTA_FLAG, AWS_OTA_RESULT_NG))
#endif
        {
            printf("write AZURE_NVRAM_CONFIG_OTA_RESULT failed\n");
        }
        vTaskDelay(1);
#ifdef RM_MAP_PERSISTANT_W
        RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                                AWSIOT_CFG_OTA_URL, &tmp_url);
#endif
        if (tmp_url)
        {
            flagOTA = 0;
#ifdef RM_MAP_PERSISTANT_W
             if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                                          AWSIOT_CFG_OTA_FLAG, flagOTA))
#endif
            {
                printf("write AZURE_NVRAM_CONFIG_OTA_FLAG failed");
            }
            vTaskDelay(1);
            strncpy(azure_file_url, (char const*) tmp_url, strlen(tmp_url));
#if defined(__SUPPORT_OTA__)
            azure_ota_fw_update(azure_file_url);
#endif
            return;
        } else
        {
            printf("OTA URL string invalid");
        }
    }

#if defined(__RUN_APP_SLEEP_2__)	
    sleepMode = SLEEP_MODE_2;
#else
    sleepMode = SLEEP_MODE_3;
#endif

    //check validation
    if (sleepMode > SLEEP_MODE_3 || sleepMode <= SLEEP_MODE_NONE)
    {
        sleepMode = SLEEP_MODE_3; //default		
        useRTM = TRUE;
    }

    if (rtcTime <= 0)
    {
        rtcTime = AZURE_CONFIG_KEEP_INTERVAL_DEF;
        if (sleepMode == SLEEP_MODE_2)
        {
            rtcTime = (int) (getAppSleep2Interval(
                    SLEEP_2_WAKEUP_TIME_INTERVAL_SEC) / 1000000);
        }
    }

    if (sleepMode == SLEEP_MODE_1)
    {
        if (SLEEP_1_USE_RTM != 0)
        {
            useRTM = TRUE;
        } else
        {
            useRTM = FALSE;
        }
    } else if (sleepMode == SLEEP_MODE_2)
    {
        if (SLEEP_2_USE_RTM != 0)
        {
            useRTM = TRUE;
        } else
        {
            useRTM = FALSE;
        }
    } else
    {
        useRTM = TRUE;
    }

    printf("sleepMode: %d, rtcTime: %d \n\n", sleepMode, rtcTime);

    setSleepMode((APPSleepMode) sleepMode, rtcTime, useRTM);
    my_port = app_dpm_get_client_socket_port();
    if (my_port == UNDEF_PORT)
        pal_app_get_def_local_port((uint32_t*) &my_port);

    dpmAppThreadCreate(APP_AZURE_TWIN, DPM_App_Main, 0,
            sizeof(app_dpm_info_rtm), (1024 * 8) / sizeof(StackType_t),
            AZURE_SUBTASK_PRIORITY + 1, my_port, rtcTime,
            DEFAULT_AZURE_DNS_ADDR, SECOND_AZURE_DNS_ADDR, SNTP_TRY_COUNT);
    sendDPMAppMSGQ(&sampleData);

    return;
}

void azure_twin_dpm_auto_start(void *arg)
{
    (void) arg;
    int sysmode;

#if defined (__BLE_COMBO_REF__)        // added for combo
    {
        int32_t status = pdPASS;
        int32_t cnt = 0;
        while (1)
        {
            if (check_net_init(WLAN0_IFACE) == pdPASS)
            {
                break;
            }
            vTaskDelay(50);
        }

        /* Waiting netif status */
        status = wifi_netif_status(WLAN0_IFACE);
        while (status == 0xFF || status != pdPASS || wifi_svc_get_provisioning_flag())
        {
            vTaskDelay(50);
            status = wifi_netif_status(WLAN0_IFACE);
            cnt++;
            if (cnt % 60 == 59)
            {
                APRINTF("wifi checking...(provision flag=%d)\n", wifi_svc_get_provisioning_flag());
            }
        }

        RTC_CLEAR_EXT_SIGNAL();        // clear wakeup source:: by Mike
    }
#endif        // __BLE_COMBO_REF__

    APRINTF_S("\n===========================================\n\n");
    pal_app_set_prov_feature();
    APRINTF("[ %s]\n\n", __func__);
    APRINTF("\nAPP_IOT on Station Mode for \"%s\" \n\n", getAppThingName());
    APRINTF_S("============================================\n\n");
    pal_app_dpm_auto_start();

    sysmode = get_sys_mode();
    if (sysmode == WIFI_DEVICE_MODE_EXT_AP || sysmode == WIFI_DEVICE_MODE_EXT_AP_STATION)
    {
        APRINTF("\nAZURE_IOT AP Mode %s \n\n", getAppThingName());

#if !defined (__BLE_COMBO_REF__)
        pal_app_start_provisioning(AZURE_MODE_GEN);
#endif     // (__BLE_COMBO_REF__)

        vTaskDelete(NULL);
        return;
    }
    pal_app_check_ping();
#if defined(__SUPPORT_ATCMD__)
    pal_app_at_command_status(ATCMD_Status_network_OK);
#endif
    azure_dpm_app_sample_thread();

    vTaskDelete(NULL);
}

// prov dev client sample
#if (ENABLE_AZURE_DPS_EXAMPLE == 1)
static void registration_status_callback(PROV_DEVICE_REG_STATUS reg_status, void *user_context)
{
    (void)user_context;
    LogInfo("Provisioning Status: %s", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char *iothub_uri
    , const char *device_id, void *user_context)
{
    char *sharedAccessKey;
    INT32 rcode = 0;

    if (user_context == NULL)
    {
        LogError("user_context is NULL");
    } else
    {
        CLIENT_SAMPLE_INFO *user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
            (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
            user_ctx->registration_complete = 1;

            create_connection_string(conStrBuffer, (char*)iothub_uri
                , (char*)device_id, getAppDevPrimaryKey());
#ifdef RM_MAP_PERSISTANT_W
            rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                   ENV_GROUP_APPCFG, APP_NVRAM_DEVICE_CONNECTION_STRING, conStrBuffer);
#endif
            OAL_MSLEEP(30);
            if (rcode)
            {
                LogError("APP_NVRAM_DEVICE_CONNECTION_STRING failed !!");
            }
        } else
        {
            LogError("Failure encountered on registration %s"
                , MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result) );
            user_ctx->registration_complete = 2;
        }
    }
}

static char *app_provisioning_device_service(void)
{
    //dpm_debug
    (void)IoTHub_Deinit();
    printf("\n\t**** Deinit Called *****\t\n");
    char *retConString;
    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport;
    PROV_DEVICE_LL_HANDLE prov_device_handle;

SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_SYMMETRIC_KEY;

    char *reg_device_id;
    char *shared_access_key;

    CLIENT_SAMPLE_INFO user_ctx;
    bool traceOn = false;

    memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

    /* Used to initialize IoTHub SDK subsystem */
    (void)IoTHub_Init();
    (void)prov_dev_security_init(hsm_type);

    LogInfo("app_provisioning_device_service");

#ifdef SAMPLE_MQTT
    prov_transport = Prov_Device_MQTT_Protocol;
#endif        // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    prov_transport = Prov_Device_MQTT_WS_Protocol;
#endif        // SAMPLE_MQTT_OVER_WEBSOCKETS

    // Set ini
    user_ctx.registration_complete = 0;
    user_ctx.sleep_time = 10;

    reg_device_id = getAppThingName();
    shared_access_key = getAppDevPrimaryKey();
    app_dpm_set_skip_saving_serverip(TRUE);

    // Set the symmetric key if using they auth type
    // If using DPS with an enrollment group, this must the the derived device key from
    // the DPS Primary Key
    // https://docs.microsoft.com/azure/iot-dps/concepts-symmetric-key-attestation?tabs=azure-cli#group-enrollments
    prov_dev_set_symmetric_key_info(reg_device_id, shared_access_key);

    if ((prov_device_handle = Prov_Device_LL_Create(GLOBAL_PROV_URI
        , ID_SCOPE, prov_transport)) == NULL)
        {
        (void)printf("failed calling Prov_Device_Create\r\n");
    } else
    {
        Prov_Device_LL_SetOption(prov_device_handle, PROV_OPTION_LOG_TRACE, &traceOn);

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate. This is only necessary on systems without
        // built in certificate stores.
        Prov_Device_LL_SetOption(prov_device_handle, OPTION_TRUSTED_CERT, defaultROOT_CA_PEM);
#endif        // SET_TRUSTED_CERT_IN_SAMPLES

        if (Prov_Device_LL_Register_Device(prov_device_handle, register_device_callback, &user_ctx
            , registration_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
            {
            (void)printf("failed calling Prov_Device_LL_Register_Device\r\n");
        } else
        {
            do
            {
                Prov_Device_LL_DoWork(prov_device_handle);
                ThreadAPI_Sleep(user_ctx.sleep_time);
            } while (user_ctx.registration_complete == 0);
        }
        Prov_Device_LL_Destroy(prov_device_handle);
    }

    if (user_ctx.registration_complete == 1)
    {
        retConString = conStrBuffer;
    } else
    {
        retConString = NULL;
    }

    free(user_ctx.iothub_uri);
    free(user_ctx.device_id);
    /* prov_dev_security_deinit(); */
    IoTHub_Deinit();

    return retConString;
}
#endif    /* ENABLE_AZURE_DPS_EXAMPLE */

#if defined(__SUPPORT_OTA__)
void setOtaFileUrl(bool desiredMode, int msgPayloadLen, void *msgPayload)
{
    cJSON *json_recv_data = NULL;
    cJSON *desired_object = NULL;
    cJSON *child_object = NULL;
    cJSON *child_version = NULL;
    cJSON *child_url = NULL;
    cJSON *mcu_version = NULL;
    char *nvCurFileVer = NULL;

    char resPayload[500] = { 0, };
    INT32 rcode = 0;

    pal_app_atc_work(NULL);
    snprintf(resPayload, (unsigned int) msgPayloadLen, "%s",
            (char*) msgPayload);

    json_recv_data = cJSON_Parse(resPayload);

    if (desiredMode == TRUE)
    {
        desired_object = cJSON_GetObjectItem(json_recv_data, "desired");

        if (desired_object != NULL)
            child_object = cJSON_GetObjectItem(desired_object, "fwupdate");
        else
            return;
    } else
    {
        child_object = cJSON_GetObjectItem(json_recv_data, "fwupdate");
    }
    if (child_object != NULL)
    {
        mcu_version = cJSON_GetObjectItem(child_object, "mcu_version");
        child_version = cJSON_GetObjectItem(child_object, "version");
        if ((child_version != NULL)
                && (strlen(child_version->valuestring) != 0))
                {
            memset(azure_file_version, 0x00, sizeof(azure_file_version));
            snprintf(azure_file_version, strlen(child_version->valuestring) + 1,
                    "%s", child_version->valuestring);

            LogInfo("OTA version : %s ", azure_file_version);
#ifdef RM_MAP_PERSISTANT_W

            rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                   ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_VERSION, azure_file_version);
#endif
            OAL_MSLEEP(30);
            if (rcode)
            {
                LogError(
                        "write AZURE_NVRAM_CONFIG_OTA_VERSION failed => force to sleep");
                OAL_MSLEEP(30);
            }
#ifdef RM_MAP_PERSISTANT_W
            RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_CURRENT_OTA_VERSION, &nvCurFileVer);
#endif

            if (nvCurFileVer == NULL)
            {
                LogInfo("OTA Current Version is NULL");
            } else
            {
                if (strcmp(nvCurFileVer, azure_file_version))
                {
                    LogInfo("version is different ota ready, new: %s, nv: %s ",
                            azure_file_version, nvCurFileVer);
                } else
                {
                    LogInfo("OTA Version is same, change version name !!!");
                    return;
                }
            }
        } else if ((mcu_version != NULL)
                && (strlen(mcu_version->valuestring) != 0)
                && pal_is_support_mcu_update())
                {
            if (strlen(mcu_version->valuestring) != 0)
            {
                memset(azure_file_version, 0x00, sizeof(azure_file_version));
                snprintf(azure_file_version,
                        strlen(mcu_version->valuestring) + 1, "%s",
                        mcu_version->valuestring);
                LogInfo("MCU OTA version : %s ", azure_file_version);
#ifdef RM_MAP_PERSISTANT_W
                rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                       ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_MCUOTA_VERSION, azure_file_version);
#endif
                if (rcode)
                    LogError(
                            "write AZURE_NVRAM_CONFIG_MCUOTA_VERSION failed => force to sleep");
#ifdef RM_MAP_PERSISTANT_W
                RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_CURRENT_MCUOTA_VERSION, &nvCurFileVer);
#endif

                if (nvCurFileVer == NULL)
                {
                    LogInfo("MCU OTA Current Version is NULL");
                } else
                {
                    if (strcmp(nvCurFileVer, azure_file_version))
                    {
                        LogInfo(
                                "MCU vesrion is different ota ready, new: %s, nv: %s ",
                                azure_file_version, nvCurFileVer);
                    } else
                    {
                        LogInfo(
                                "MCU OTA Version is same, change version name !!!");
                        return;
                    }
                }
            }
        } else
        {
            setOTAStat(OTA_STAT_JOB_NONE);
#ifdef RM_MAP_PERSISTANT_W
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                    AZURE_NVRAM_CONFIG_OTA_STATE, AZURE_OTA_STATE_UNKOWN))
#endif
                IOT_ERROR("write AZURE_NVRAM_CONFIG_OTA_STATE failed");
            return;
        }

        child_url = cJSON_GetObjectItem(child_object, "url");
        if (child_url != NULL)
        {
            if (strlen(child_url->valuestring) != 0)
                snprintf(azure_file_url, strlen(child_url->valuestring) + 1,
                        "%s", child_url->valuestring);

            LogInfo("file url : %s \r\n", azure_file_url);
#ifdef RM_MAP_PERSISTANT_W
            rcode = RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(),
                                                   ENV_GROUP_APPCFG, AZURE_NVRAM_CONFIG_OTA_URL, azure_file_url);
#endif
            OAL_MSLEEP(30);
            if (rcode)
            {
                LogError("write NVRAM_CONFIG_OTA_URL failed => force to sleep");
                OAL_MSLEEP(30);
            }

            setOTAStat(OTA_STAT_JOB_READY);
#ifdef RM_MAP_PERSISTANT_W
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                    AZURE_NVRAM_CONFIG_OTA_STATE, AZURE_OTA_STATE_READY))
#endif
                LogError("write AZURE_NVRAM_CONFIG_OTA_STATE failed");
        }
    }
}

static OTA_UPDATE_CONFIG _fw_conf = { 0, };
static OTA_UPDATE_CONFIG *fw_conf = (OTA_UPDATE_CONFIG*) &_fw_conf;

void app_ota_fw_download_complete_notify(ota_update_type update_type,
        UINT status, UINT progress)
        {
    if (pal_app_notify_mcu_ota(update_type, status, progress,
            fw_conf->auto_renew))
        return;

    switch (update_type)
    {
    case OTA_TYPE_RTOS:
        IOT_DEBUG("RTOS download finish. (0x%02x) \n\n", status)
        ;
// test code[[::check retry scenario
//            status = OTA_FAILED;
// ]]
        if ((status == OTA_SUCCESS))
        {
#if !defined(__BLE_COMBO_REF__)        // orig
            if (fw_conf->auto_renew == 0)
            {
                // update index here
                ota_update_start_renew(fw_conf);
            } else
            {
#ifdef RM_MAP_PERSISTANT_W
                if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                        AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_OK))
#endif
                {
                    IOT_ERROR("write AZURE_NVRAM_CONFIG_OTA_RESULT failed\n");
                }
                vTaskDelay(1);
                IOT_DEBUG("Succeeded to replace with new FW.");
            }
#else           // da16600work[[::
            IOT_DEBUG("next checking BLE f/w image...");
#endif          // ]] da16600work
        } else
        {
#ifdef RM_MAP_PERSISTANT_W
            if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_NG))
#endif
            {
                IOT_ERROR("write AZURE_NVRAM_CONFIG_OTA_RESULT failed\n");
            }
            vTaskDelay(1);
            IOT_DEBUG("Failed to replace with new FW. (Err : 0x%02x)", status);
            if (fw_conf->auto_renew == 0)
            {
                IOT_DEBUG(">>> System Reboot !!!");
                reboot_func(SYS_REBOOT);
            }
        }
        break;

#if defined(__BLE_COMBO_REF__)        // da16600work[[::
    case OTA_TYPE_MCU_FW:
        IOT_DEBUG("BLE download finish. (0x%02x) \n\n", status);
// test code[[::check retry scenario
//              status = OTA_FAILED;
// ]]
        if ((status == OTA_SUCCESS))
        {
            if (fw_conf->auto_renew == 0)
            {
                // update index here
                ota_update_start_renew(fw_conf);
            } else
            {
#ifdef RM_MAP_PERSISTANT_W
            	if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
            			AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_OK))
#endif
            	{
                    IOT_ERROR("write AWS_NVRAM_CONFIG_OTA_RESULT failed\n");
                }
                vTaskDelay(1);
                IOT_DEBUG("Succeeded to replace with new FW.");
            }
        } else
        {
#ifdef RM_MAP_PERSISTANT_W
        	if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
        			AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_NG))
#endif
        	{
                IOT_ERROR("write AWS_NVRAM_CONFIG_OTA_RESULT failed\n");
            }
            vTaskDelay(1);
            IOT_DEBUG("Failed to replace with new FW. (Err : 0x%02x)", status);
            if (fw_conf->auto_renew == 0)
            {
                IOT_DEBUG(">>> System Reboot !!!");
                reboot_func(SYS_REBOOT);
            }
        }
        break;
#endif        // ]]da16600work
    default:
        break;
    }

    /* work todo */
}

void app_ota_fw_renew_notify(UINT status)
{
    if (status == OTA_SUCCESS)
    {
#ifdef RM_MAP_PERSISTANT_W
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                    AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_OK))
#endif
        {
            IOT_ERROR("write AZURE_NVRAM_CONFIG_OTA_RESULT failed\n");
        }
        vTaskDelay(1);
        IOT_DEBUG("Succeeded to replace with new FW.");
    } else
    {
#ifdef RM_MAP_PERSISTANT_W
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                AZURE_NVRAM_CONFIG_OTA_RESULT, AZURE_OTA_RESULT_NG))
#endif
        {
            IOT_ERROR("write AZURE_NVRAM_CONFIG_OTA_RESULT failed\n");
        }
        vTaskDelay(1);
        IOT_DEBUG("Failed to replace with new FW. (Err : 0x%02x)", status);
        IOT_DEBUG(">>> System Reboot !!!");
        reboot_func(SYS_REBOOT);
    }
    /* work todo */
}

UINT32 azure_ota_fw_update(char *_fw_url)
{
    UINT32 status = OTA_SUCCESS;

    memset(fw_conf, 0x00, sizeof(OTA_UPDATE_CONFIG));
    if (!pal_app_check_is_update_mcu(_fw_url, fw_conf))
    {
        /* Setting the type to be updated */

        fw_conf->update_type = OTA_TYPE_RTOS;
        snprintf(ota_path_url_rtos, MAX_URL_LEN, "%s/%s", _fw_url, RTOS_NAME);
#if (SDK_MAJOR == 3) && (SDK_MINOR == 2) && (SDK_REVISION >= 5)		
        memcpy(fw_conf->url, ota_path_url_rtos, strlen(ota_path_url_rtos));
        IOT_DEBUG(" RTOS url %s", fw_conf->url);
#else
        memcpy(fw_conf->url, ota_path_url_rtos, strlen(ota_path_url_rtos));
        IOT_DEBUG(" RTOS url %s", fw_conf->url);
#endif		

#if defined (__BLE_COMBO_REF__)        // org
        snprintf(ota_path_url_ble, MAX_URL_LEN, "%s/%s", _fw_url, BLE_NAME);
#if (SDK_MAJOR == 3) && (SDK_MINOR == 2) && (SDK_REVISION >= 5)
        fw_conf->update_type = OTA_TYPE_BLE_COMBO;
        memcpy(fw_conf->url_ble_fw, ota_path_url_ble, strlen(ota_path_url_ble));
#else
        fw_conf->uri_other_fw = ota_path_url_ble;
#endif
        IOT_DEBUG(" BLE url %s", ota_path_url_ble);
#endif        // __BLE_COMBO_REF__
    }

    /* FW renew (reboot) after successful download */
#if (SDK_MAJOR == 3) && (SDK_MINOR == 2) && (SDK_REVISION >= 5)
    fw_conf->auto_renew = 1;
#else
    fw_conf->auto_renew = 0;
#endif

    /* Call when download is complete (success or fail) */
    fw_conf->download_notify = app_ota_fw_download_complete_notify;

    /* Called when the renew(change boot index) is complete
     (If it succeeds, it will reboot automatically.) */
    fw_conf->renew_notify = app_ota_fw_renew_notify;

    /* get ip address in advance */
    // test code
    /*
     char hostName[256] = {0, };
     size_t uri_len = strlen(_fw_url);
     status = app_parse_uri((unsigned char *)_fw_url, uri_len, hostName, 800);
     IOT_DEBUG("OTA hostname = \"%s\", rc=%d\n", hostName, status);
     */
    status = ota_update_start_download(fw_conf);
    if (status)
    {
        IOT_ERROR("OTA update start fail (0x%02x)\n", status);
    }

    return status;
}
#endif    /* __SUPPORT_OTA__ */
