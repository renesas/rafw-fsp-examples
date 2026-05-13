/***********************************************************************************************************************
 * File Name    : app_sample_manager.c
 * Description  : Make the device thingname for AWS platform also provide fleet provisioning status.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdarg.h"
#include "common_def.h"
#include "common_utils.h"
#include "rm_vee_flash_w_rrq_nvram.h"
/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#include "app_sample_manager.h"
#include "fleet_provisioning.h"
#include "app_dpm_thread.h"
#include "rm_cert.h"
#include "demo_config.h"
#if defined(RM_MAP_PERSISTANT_W)
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#endif

#undef	TIME_CHECK_DBG //enable or disable of awsiot_app_print_elapse_time_ms()

#if defined ( __SUPPORT_AWS_IOT_W__ )
#include "iface_defs.h"
#endif // ( __SUPPORT_AWS_IOT_W__ ) 

char *aws_root_ca = NULL;
char *aws_init_cert = NULL;
char *aws_init_key = NULL;
char *aws_uniq_cert = NULL;
char *aws_uniq_key = NULL;

static char *getMemoryString(const char *string_data)
{
    size_t strLen;
    char *strTemp;

    strLen = strlen(string_data);
    strTemp = (char *)pvPortMalloc(strLen + 1);
    if (strTemp != NULL)
    {
        memset(strTemp, 0x00, (strLen + 1));
        strcpy(strTemp, string_data);
    }

    return strTemp;
}

char* getAppThingName(void)
{
    char *nvramName = NULL;
    char *app_thing_name = NULL;

#if defined(__SUPPORT_ATCMD__)
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_THINGNAME, &nvramName);
#else
#ifdef RM_MAP_PERSISTANT_W
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, APP_CONFIG_THINGNAME, &nvramName);
#else
    nvramName = read_nvram_appcfg_string(APP_CONFIG_THINGNAME);
#endif 
#endif 

    if (nvramName == NULL)
    {
        app_thing_name = getMemoryString((const char*)AWS_IOT_THING_NAME);
    }

    return app_thing_name;
}

void load_aws_credentials(void)
{
    size_t aws_root_ca_len = CERT_MAX_LENGTH;
    size_t aws_init_cert_len = CERT_MAX_LENGTH;
    size_t aws_init_key_len = CERT_MAX_LENGTH;
    size_t aws_uniq_cert_len = CERT_MAX_LENGTH;
    size_t aws_uniq_key_len = CERT_MAX_LENGTH;
    rm_cert_format_t format = RM_CERT_FORMAT_PEM;

    if (aws_root_ca == NULL)
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_CA_CERT))
        {
            printf("Found and loading AWS RootCA from NVRAM\n\r");
            aws_root_ca = APP_MALLOC(CERT_MAX_LENGTH);
            memset(aws_root_ca, 0x00, CERT_MAX_LENGTH);
            RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_CA_ADDR),
                         RM_CERT_GetType(SF_TLS_CERT_AWS_CA_ADDR), &format,
                         (unsigned char *)aws_root_ca, &aws_root_ca_len);
        }
        else
        {
            printf("No RootCA in NVRAM\n\r");
        }
    }
    if (aws_init_cert == NULL)
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_INITIAL_CERT))
        {
            printf("Found and loading AWS Initial Certificate from NVRAM\n\r");
            aws_init_cert = APP_MALLOC(CERT_MAX_LENGTH);
            memset(aws_init_cert, 0x00, CERT_MAX_LENGTH);
            RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR),
                         RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR), &format,
                         (unsigned char *)aws_init_cert, &aws_init_cert_len);
        }
        else
        {
            printf("No AWS Initial Certificate in NVRAM\n\r");
        }
    }
    if (aws_init_key == NULL)
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_INITIAL_PRIV_KEY))
        {
            printf("Found and loading AWS Initial Private Key from NVRAM\n\r");
            aws_init_key = APP_MALLOC(CERT_MAX_LENGTH);
            memset(aws_init_key, 0x00, CERT_MAX_LENGTH);
            RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR),
                         RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR), &format,
                         (unsigned char *)aws_init_key, &aws_init_key_len);
        }
        else
        {
            printf("No AWS Initial Private Key in NVRAM\n\r");
        }
    }
    if (aws_uniq_cert == NULL)
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_UNIQUE_CERT))
        {
            printf("Found and loading AWS Unique Certificate from NVRAM\n\r");
            aws_uniq_cert = APP_MALLOC(CERT_MAX_LENGTH);
            memset(aws_uniq_cert, 0x00, CERT_MAX_LENGTH);
            RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_UNIQUE_CERT_ADDR),
                         RM_CERT_GetType(SF_TLS_CERT_AWS_UNIQUE_CERT_ADDR), &format,
                         (unsigned char *)aws_uniq_cert, &aws_uniq_cert_len);
        }
        else
        {
            printf("No AWS Unique Certificate in NVRAM\n\r");
        }
    }
    if (aws_uniq_key == NULL)
    {
        if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_UNIQUE_PRIV_KEY))
        {
            printf("Found and loading AWS Unique Private Key from NVRAM\n\r");
            aws_uniq_key = APP_MALLOC(CERT_MAX_LENGTH);
            memset(aws_uniq_key, 0x00, CERT_MAX_LENGTH);
            RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_UNIQUE_PRIV_KEY_ADDR),
                         RM_CERT_GetType(SF_TLS_CERT_AWS_UNIQUE_PRIV_KEY_ADDR), &format,
                         (unsigned char *)aws_uniq_key, &aws_uniq_key_len);
        }
        else
        {
            printf("No AWS Unique Private Key in NVRAM\n\r");
        }
    }
}

void free_aws_credentials(void)
{
    if (aws_root_ca)
    {
        APP_FREE(aws_root_ca);
        aws_root_ca = NULL;
    }
    if (aws_init_cert)
    {
        APP_FREE(aws_init_cert);
        aws_init_cert = NULL;
    }
    if (aws_init_key)
    {
        APP_FREE(aws_init_key);
        aws_init_key = NULL;
    }
    if (aws_uniq_cert)
    {
        APP_FREE(aws_uniq_cert);
        aws_uniq_cert = NULL;
    }
    if (aws_uniq_key)
    {
        APP_FREE(aws_uniq_key);
        aws_uniq_key = NULL;
    }
}

char *aws_credentials_read(uint32_t addr)
{
    if ((aws_root_ca == NULL) || (aws_init_cert == NULL) || (aws_init_key == NULL) ||
        (aws_uniq_cert == NULL) || (aws_uniq_key == NULL))
    {
        load_aws_credentials();
    }

    switch(addr)
    {
    case SF_TLS_CERT_AWS_CA_ADDR:
        if (aws_root_ca)
        {
            return aws_root_ca;
        }
        break;
    case SF_TLS_CERT_AWS_INITIAL_CERT_ADDR:
        if (aws_init_cert)
        {
            return aws_init_cert;
        }
        break;
    case SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR:
        if (aws_init_key)
        {
            return aws_init_key;
        }
        break;
    case SF_TLS_CERT_AWS_UNIQUE_CERT_ADDR:
        if (aws_uniq_cert)
        {
            return aws_uniq_cert;
        }
        break;
    case SF_TLS_CERT_AWS_UNIQUE_PRIV_KEY_ADDR:
        if (aws_uniq_key)
        {
            return aws_uniq_key;
        }
        break;
    default:
        printf("Wrong input for getting aws credentials\n\r");

        return NULL;
    }

    printf("Could not get the aws credentials\n\r");

    return NULL;
}

char *get_fleet_provisioning_client_identifier(void)
{
    char *dev_id = NULL;
    char *client_id = NULL;
    const char *prefix = "client";
    size_t client_id_len;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_FLEET_PROVISIONING_DEVICE_ID, &dev_id);
    if (dev_id == NULL)
    {
        printf("Failed to read fleet provisioning device id from NVRAM\n\r");

        return NULL;
    }

    client_id_len = strlen(prefix) + strlen(dev_id) + 1;
    client_id = malloc(client_id_len);
    if (client_id == NULL)
    {
        printf("Memory allocation failed for client identifier\n\r");

        return NULL;
    }
    memset(client_id, 0, client_id_len);
    sprintf(client_id, "%s%s", prefix, dev_id);
    printf("Client Identifier: %s\n\r", client_id);

    return client_id;
}

char *get_fleet_provisioning_csr_subject_name(void)
{
    char *dev_id = NULL;
    char *csr_sub_name = NULL;
    const char *prefix = "CN=";
    size_t csr_sub_name_len;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_FLEET_PROVISIONING_DEVICE_ID, &dev_id);
    if (dev_id == NULL)
    {
        printf("Failed to read fleet provisioning device id from NVRAM\n\r");

        return NULL;
    }

    csr_sub_name_len = strlen(prefix) + strlen(dev_id) + 1;
    csr_sub_name = pvPortMalloc(csr_sub_name_len);
    if (csr_sub_name == NULL)
    {
        printf("Memory allocation failed for CSR subject name\n\r");

        return NULL;
    }
    memset(csr_sub_name, 0, csr_sub_name_len);
    sprintf(csr_sub_name, "%s%s", prefix, dev_id);
    printf("CSR Subject Name: %s\n\r", csr_sub_name);

    return csr_sub_name;
}

static char *get_fleet_provisioning_template_name(void)
{
    char *fp_tmpl_name = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_FLEET_PROVISIONING_TEMPLATE_NAME, &fp_tmpl_name);
    if (fp_tmpl_name)
    {
        printf("Fleet Provisioning Template Name: %s\n\r", fp_tmpl_name);

        return fp_tmpl_name;
    }
    else
    {
        printf("Using default Fleet Provisioning Template Name: %s\n\r", democonfigPROVISIONING_TEMPLATE_NAME);

        return democonfigPROVISIONING_TEMPLATE_NAME;
    }
}

char *get_aws_broker_url(void)
{
    char *broker_url = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_BROKER_URL, &broker_url);
    if (broker_url)
    {
        printf("AWS Broker URL: %s\n\r", broker_url);

        return broker_url;
    }
    else
    {
        printf("Using default AWS Broker URL: %s\n\r", democonfigMQTT_BROKER_ENDPOINT);

        return democonfigMQTT_BROKER_ENDPOINT;
    }
}

int get_aws_mqtt_broker_port(void)
{
    int port = 0;

    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                 AWSIOT_CFG_PORT, &port);
    if (port > 0)
    {
        printf("AWS MQTT Port: %d\n\r", port);

        return port;
    }
    else
    {
        printf("Using default AWS MQTT Port: %d\n\r", democonfigMQTT_BROKER_PORT);

        return democonfigMQTT_BROKER_PORT;
    }
}

char *get_fleet_provisioning_topics(uint8_t topic_type)
{
    char *topic = NULL;
    size_t topic_len;
    char *fp_tmpl_name = NULL;

    fp_tmpl_name = get_fleet_provisioning_template_name();

    switch(topic_type)
    {
        case E_FP_JSON_REGISTER_PUBLISH_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_JSON_FORMAT) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_JSON_FORMAT);
            }
            break;
        case E_FP_JSON_REGISTER_ACCEPTED_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_JSON_FORMAT) +
                        strlen(FP_API_ACCEPTED_SUFFIX) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_JSON_FORMAT, FP_API_ACCEPTED_SUFFIX);
            }
            break;
        case E_FP_JSON_REGISTER_REJECTED_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_JSON_FORMAT) +
                        strlen(FP_API_REJECTED_SUFFIX) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_JSON_FORMAT, FP_API_REJECTED_SUFFIX);
            }
            break;
        case E_FP_CBOR_REGISTER_PUBLISH_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_CBOR_FORMAT) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_CBOR_FORMAT);
            }
            break;
        case E_FP_CBOR_REGISTER_ACCEPTED_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_CBOR_FORMAT) +
                        strlen(FP_API_ACCEPTED_SUFFIX) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_CBOR_FORMAT, FP_API_ACCEPTED_SUFFIX);
            }
            break;
        case E_FP_CBOR_REGISTER_REJECTED_TOPIC:
            topic_len = strlen(FP_REGISTER_API_PREFIX) +
                        strlen(fp_tmpl_name) +
                        strlen(FP_REGISTER_API_BRIDGE) +
                        strlen(FP_API_CBOR_FORMAT) +
                        strlen(FP_API_REJECTED_SUFFIX) + 1;
            topic = malloc(topic_len);
            if (topic == NULL)
            {
                printf("Memory allocation for fleet provisioning template topic failed\n\r");
            }
            else
            {
                memset(topic, 0, topic_len);
                sprintf(topic, "%s%s%s%s%s", FP_REGISTER_API_PREFIX, fp_tmpl_name,
                        FP_REGISTER_API_BRIDGE, FP_API_CBOR_FORMAT, FP_API_REJECTED_SUFFIX);
            }
            break;
        default:
            printf("Invalid topic type\n\r");
         break;
    }

    if (topic)
    {
        printf("Fleet Provisioning Topic generated: %s\n\r", topic);
    }

    return topic;
}
bool validate_fleet_provisioning_nvram_config(void)
{
    char *dev_id = NULL;
    char *fp_tmpl_name = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_FLEET_PROVISIONING_DEVICE_ID, &dev_id);
    if (dev_id == NULL)
    {
        printf("Fleet Provisioning Device ID not found in nvram\n\r");

        return false;
    }

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_FLEET_PROVISIONING_TEMPLATE_NAME, &fp_tmpl_name);
    if (fp_tmpl_name == NULL)
    {
        printf("Fleet Provisioning Template Name not found in nvram\n\r");

        return false;
    }

    printf("Fleet Provisioning Device ID: %s\n"
           "Fleet Provisioning Template Name: %s\n", dev_id, fp_tmpl_name);

    return true;
}
