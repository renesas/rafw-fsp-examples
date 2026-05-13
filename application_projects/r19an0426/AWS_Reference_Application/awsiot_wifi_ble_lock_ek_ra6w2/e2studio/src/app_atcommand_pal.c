/***********************************************************************************************************************
 * File Name    : app_atcommand_pal.c
 * Description  : adaptation layer between platform and at-command driver
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* Referring Feature for AWS-IOT-W */
#include "rm_awsiot_w_cfg.h"
#if defined (__SUPPORT_AWS_IOT_W__)
#include "FreeRTOS.h"
#include "custom_config_sdk.h"
#include "nvedit.h"
#include "sys_feature.h"
#include "app_sample_manager.h"
#include "app_dpm_thread.h"
#include "app_atcommand_pal.h"
#include "rm_aws_lwip_sock_wrap_w_api.h"
#include "app_aws_user_conf.h"
#include "app_awsiot_user_config.h"
#include "shadow.h"
#include "ip_addr.h"
#include "ota_update.h"

#if defined(__SUPPORT_ATCMD__)
#include "app_aws_atcmd_parse.h"
#endif

#include "rm_cert.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_map_persistant_w.h"

#ifndef APP_UNUSED_ARG
#define APP_UNUSED_ARG(x)                   (void)x
#endif

#if defined(__SUPPORT_ATCMD__)
#define Cmd_Idle                            0xFF
#define MAX_RESPONSE_WAIT_COUNT             600
#define DEFAULT_SHADOW_NAME                 SHADOW_NAME_CLASSIC
#define SHADOW_NAME_LENGTH                  ((uint16_t)(sizeof(DEFAULT_SHADOW_NAME) - 1))
#define MAX_PUBLISH_PAYLOAD_LEN             512
#define MAX_SUBSCRIBE_TOPIC_LEN             256

struct atcmd_nvparams atnvParam;
static int8_t userCommand = Cmd_Idle;
int8_t set_atnvParam = 0;
int8_t mcu_wakeup_port = -1;
uint16_t mcu_wakeup_pin = 0;
static char atBuf[256] = { 0, };
static char payload_buf[MAX_PUBLISH_PAYLOAD_LEN] = { 0, };
static char topic_buf[MAX_SUBSCRIBE_TOPIC_LEN] = { 0, };

extern st_atcmd_jsonStruct_t registeredShadow[MAX_SHADOW_COUNT];
extern int32_t gShadowCount;
#endif  /* __SUPPORT_ATCMD__ */

#if defined (__SUPPORT_ATCMD__)
static void pal_app_making_shadow_payload(char *payload, uint32_t lpayload);
extern unsigned int ra6w1_ping_client(int, char*, unsigned long, unsigned long*,
                                      unsigned long*, int, uint64_t, int, int, int, char*);
#endif  /* __SUPPORT_ATCMD__ */
extern bool reset(int flag);
extern void wait_MCU_resp(uint8_t owner);
extern uint8_t get_MCU_resp(void);

char *aws_root_ca = NULL;
char *aws_client_cert = NULL;
char *aws_priv_key = NULL;

void pal_app_get_def_local_port(uint32_t *port)
{
#if defined(__SUPPORT_ATCMD__)
    uint32_t my_local_port = 0;

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                     AWSIOT_CFG_LPORT, (int*)&my_local_port))
    {
        *port = my_local_port = AWS_MQTT_PORT;
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_LPORT, my_local_port))
            printf("write local port nvram: failed\n");
    }
    else
    {
        *port = my_local_port;
    }
#else
    *port = AWS_MQTT_PORT;
    RA6W1_UNUSED_ARG(port);
#endif  /* __SUPPORT_ATCMD__ */
}

void load_aws_credentials(void)
{
    size_t aws_root_ca_len = CERT_MAX_LENGTH;
    size_t aws_client_cert_len = CERT_MAX_LENGTH;
    size_t aws_priv_key_len = CERT_MAX_LENGTH;
    rm_cert_format_t format = RM_CERT_FORMAT_PEM;

    if (aws_root_ca == NULL)
    {
        aws_root_ca = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_root_ca, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_CA_ADDR),
        RM_CERT_GetType(SF_TLS_CERT_AWS_CA_ADDR), &format,
                        (unsigned char *)aws_root_ca, &aws_root_ca_len);
    }
    if (aws_client_cert == NULL)
    {
        aws_client_cert = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_client_cert, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR), &format,
                     (unsigned char *)aws_client_cert, &aws_client_cert_len);
    }
    if (aws_priv_key == NULL)
    {
        aws_priv_key = APP_MALLOC(CERT_MAX_LENGTH);
        memset(aws_priv_key, 0x00, CERT_MAX_LENGTH);
        RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR),
                     RM_CERT_GetType(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR), &format,
                     (unsigned char *)aws_priv_key, &aws_priv_key_len);
    }
}

void free_aws_credentials(void)
{
    if (aws_root_ca)
    {
        APP_FREE(aws_root_ca);
        aws_root_ca = NULL;
    }
    if (aws_client_cert)
    {
        APP_FREE(aws_client_cert);
        aws_client_cert = NULL;
    }
    if (aws_priv_key)
    {
        APP_FREE(aws_priv_key);
        aws_priv_key = NULL;
    }
}

char *aws_credentials_read(uint32_t addr)
{
    if ((aws_root_ca == NULL) || (aws_client_cert == NULL) || (aws_priv_key == NULL))
    {
        free_aws_credentials();
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
        if (aws_client_cert)
        {
            return aws_client_cert;
        }
        break;
    case SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR:
        if (aws_priv_key)
        {
            return aws_priv_key;
        }
        break;
    default:
        printf("Wrong input for getting aws credentials\n\r");
        return NULL;
    }

    printf("Could not get the aws credentials\n\r");
    return NULL;
}

uint8_t pal_app_get_atcmd_parameter(void)
{
#if defined(__SUPPORT_ATCMD__)
    char *nvp;
    uint32_t port;
    rm_cert_format_t format = RM_CERT_FORMAT_PEM;
    size_t outlen = CERT_MAX_LENGTH;

    if (set_atnvParam)
    {
        return 1;
    }
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_BROKER_URL, &nvp);

    if (nvp != NULL)
    {
        printf("pal_app_get_atcmd_parameter BROKER(%s) : (%d)\n", nvp, strlen(nvp));
        atnvParam.broker = APP_MALLOC(strlen(nvp) + 1);
        if (atnvParam.broker == NULL)
        {
            printf("broker malloc: failed\n");
            goto init_error;
        }
        memset(atnvParam.broker, 0x00, strlen(nvp) + 1);
        memcpy(atnvParam.broker, nvp, strlen(nvp));
    }
    else
    {
        atnvParam.broker = APP_MALLOC(strlen(democonfigMQTT_BROKER_ENDPOINT) + 1);
        if (atnvParam.broker == NULL)
        {
            printf("broker malloc: failed\n");
            goto init_error;
        }
        memset(atnvParam.broker, 0x00, strlen(democonfigMQTT_BROKER_ENDPOINT) + 1);
        memcpy(atnvParam.broker, democonfigMQTT_BROKER_ENDPOINT,
        strlen(democonfigMQTT_BROKER_ENDPOINT));
    }

    if (RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                     AWSIOT_CFG_PORT, (int*)&port))
    {
        port = AWS_CONFIG_PORT_DEF;
        /* [aws work] */
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_PORT, port))
        {
            printf(" write aws broker port: failed\n");
            goto init_error;
        }
    }
    atnvParam.port = port;
    atnvParam.certCA = APP_MALLOC(CERT_MAX_LENGTH);
    if (atnvParam.certCA == NULL)
    {
        APP_FREE(atnvParam.broker);
        goto init_error;
    }

    memset(atnvParam.certCA, 0x00, CERT_MAX_LENGTH);
    RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_CA_ADDR),
                 RM_CERT_TYPE_CA_CERT, &format, (unsigned char *)atnvParam.certCA, &outlen);
    atnvParam.cert = APP_MALLOC(CERT_MAX_LENGTH);
    if (atnvParam.cert == NULL)
    {
        APP_FREE(atnvParam.broker);
        APP_FREE(atnvParam.certCA);
        goto init_error;
    }
    atnvParam.key = APP_MALLOC(CERT_MAX_LENGTH);
    if (atnvParam.key == NULL)
    {
        APP_FREE(atnvParam.broker);
        APP_FREE(atnvParam.certCA);
        APP_FREE(atnvParam.cert);
        printf("NV key malloc: failed\n");
        goto init_error;
    }
    memset(atnvParam.cert, 0x00, CERT_MAX_LENGTH);
    memset(atnvParam.key, 0x00, CERT_MAX_LENGTH);
    outlen = CERT_MAX_LENGTH;
    RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR),
                 RM_CERT_TYPE_INITIAL_CERT, &format, (unsigned char *)atnvParam.cert, &outlen);
    outlen = CERT_MAX_LENGTH;
    RM_CERT_Read(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR),
                 RM_CERT_TYPE_INITIAL_PRIV_KEY, &format, (unsigned char *)atnvParam.key, &outlen);
    register_atcmd_publish_user_app_callback(pal_app_publish_user_contents);
    set_atnvParam = 1;

    return 0;

init_error:
    printf("AT initialization failed\n");

    return 1;
#else
    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

/**
 ****************************************************************************************
 * @brief Network connection check with ping
 * @param[in] ipaddrstr ping ip address
 * @return  void
 ****************************************************************************************
 */

#if defined(__SUPPORT_ATCMD__)
static int8_t pal_app_check_internet_with_ping(char *ipaddrstr)
{
    UINT8 result_str[128] = { 0, };
    int result = 0;
    INT32 len = 0;
    INT32 wait = 0;
    INT32 interval = 0;
    INT32 ping_interface = 0; /* default wlan0 */
    UINT32 ipaddr = 0;
    ip_addr_t tmp_addr;
    UINT32 count = 0;
    UINT32 average, time_min, ping_max, transmitted, reply_count;

    /* result string */
    transmitted = reply_count = 0;
    average = time_min = ping_max = 0;
    result = ipaddr_aton(ipaddrstr, &tmp_addr);

    if (result == 0)
        return 0;

    ipaddr = lwip_htonl(ip4_addr_get_u32(ip_2_ip4(&tmp_addr)));
    count = 4; /* only 2 */
    len = 32; /* default */
    wait = 8000; /* default 4sec */
    interval = 1000; /* interval default 1sec */
    /* If station interface */
    ping_interface = 0; /* WLAN0_IFACE; */
    /* ping client api execution with nodisplay as 1 and getting the string of result */
    ra6w1_ping_client(ping_interface, NULL, ipaddr, NULL, NULL, len, count, wait,
                      interval, 1, (char*)result_str);
    /* parsing the result string */
    sscanf((char*)result_str, "%u,%u,%u,%u,%u", &transmitted, &reply_count, &average,
           &time_min, &ping_max);

    if (reply_count > 0)
    { /* Success */
        printf("Ping reply is ok\n");

        return 1;
    }
    printf("Ping reply is fail\n");

    return 0;
}
#endif

void pal_app_check_ping(void)
{
#if defined (__SUPPORT_ATCMD__)
    int32_t pingCheck;
    /* Check Ping */
    RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                 AWSIOT_CFG_PING_CHECK, (int *)&pingCheck);

    if (pingCheck != 1)
    {
        if (pal_app_check_internet_with_ping(DEFAULT_PING_IP) == 1)
        {
            printf("[ Ping-check] OK \n\n");
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_PING_CHECK, 1);
        }
        else
        {
            printf("[ Ping-check] fail \n\n");
            /* need factory reset */
            app_iot_at_command_status(ATCMD_STATUS_NETWORK_FAIL);
            /* Wait for system-reboot */
            while (1)
            {
                vTaskDelay(portCONVERT_MS_2_TICKS(100));
            }
        }
    }
#endif
    return;
}

void pal_app_dpm_auto_start(void)
{
#if defined(__SUPPORT_ATCMD__)
    int sysmode;
    char *tmp_read = NULL;
    char user[32] = { 0, };
    int32_t dpm_auto;

    /* MCU wake up port and pin check */
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_MCU_WAKEUP_PORT, &tmp_read);
    if (tmp_read != NULL)
    {
        strncpy(user, tmp_read, strlen(tmp_read));
        mcu_wakeup_port = (int8_t)app_atcmd_mcu_wakeup_port((char*)user);
    }

    memset(user, 0, 32);
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_MCU_WAKEUP_PIN, &tmp_read);

    if (tmp_read != NULL)
    {
        strncpy(user, tmp_read, strlen(tmp_read));
        mcu_wakeup_pin = app_atcmd_mcu_wakeup_pin((char*)user);
    }

    printf("\r\n[%s] mcu_wakeup_port=%d, mcu_wakeup_pin=0x%x \n\n",
           __func__, mcu_wakeup_port, mcu_wakeup_pin);

    /* AWS + ATCMD, default set */
#if defined(__SUPPORT_ATCMD__WORK)
    if (mcu_wakeup_port < 0 || mcu_wakeup_pin <= 0)
    {
        if (mcu_wakeup_port < 0)
            mcu_wakeup_port = GPIO_UNIT_A;
        if (mcu_wakeup_pin <= 0)
            mcu_wakeup_pin = GPIO_PIN11;
        printf("default set to mcu_wakeup_port=%d, mcu_wakeup_pin=0x%x \n\n", mcu_wakeup_port, mcu_wakeup_pin);
    }
#endif
    /* check whether configuration value OK or not */
    if (!app_at_feature_is_valid())
    {
        APRINTF_E("\ninvalid APP feature...can't start APP Platform thread...check again\n\n");
        if (RM_PMGR_W_dpm_is_enabled())
        {
            RM_PMGR_W_dpm_disable();
            /* save provisioning flag for DPM mode */
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_DPM_AUTO, 1);
            printf("\nDPM mode is disabled\n");
        }

        app_iot_at_command_status(ATCMD_STATUS_NEED_CONFIGURATION);
        vTaskDelete(NULL);

        return;
    }
    else
    {
        sysmode = get_run_mode();
        if (sysmode != WIFI_DEVICE_MODE_EXT_AP)
        {
            if (!RM_PMGR_W_dpm_is_enabled())
            {
                RM_MAP_PERSISTANT_W_Read_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                             AWSIOT_CFG_DPM_AUTO, (int *)&dpm_auto);
                if (dpm_auto == 1)
                {
#if defined(__SUPPORT_ATCMD__)
                    RM_PMGR_W_dpm_enable();
#endif
                    reset(0);

                    /* Wait for system-reboot */
                    while (1)
                        vTaskDelay(portCONVERT_MS_2_TICKS(100));
                }
            }
        }
    }
#endif  /* __SUPPORT_ATCMD__ */

    return;
}

void pal_app_start_provisioning(int32_t mode)
{
    RA6W1_UNUSED_ARG(mode);

    return;
}

#if defined(__SUPPORT_ATCMD__)
void pal_app_at_command_status(atcmd_scan_result_t _val)
{
    app_iot_at_command_status(_val);
}
#endif  /* __SUPPORT_ATCMD__ */

void pal_app_set_prov_feature(void)
{
#if defined (__SUPPORT_ATCMD__WORK)
    set_prov_feature(APP_AT_CMD);
    APRINTF_I("[ APP-IOT AT-COMMAND ] \n\n");
#elif defined(__SUPPORT_SENSOR_REF__)
    APRINTF_S("[ APP-IOT Sensors ] \n\n");
    set_prov_feature(APP_SENSOR);
#else
    APRINTF_S("[ APP-IOT Doorlock ] \n\n");
#endif  /* __SUPPORT_ATCMD__ */

    return;
}

#if defined(__SUPPORT_OTA__)
int8_t pal_app_check_is_update_mcu(char *_url, OTA_UPDATE_CONFIG *config)
{
#if defined (__SUPPORT_ATCMD__)
    char *img_name = NULL;
    char *str;

    memset(atBuf, 0x00, 128);
    memcpy(atBuf, _url, strlen(_url));
    str = strtok(atBuf, (char*)"/");

    do
    {
        if (str == NULL)
        {
            printf("OTA url is NULL break \n");
            break;
        }
        img_name = str;
    }
    while ((str = strtok(NULL, (char*)"/")) != NULL);

    if (strncasecmp(img_name, "MCU", 3) == 0)
    {
        config->update_type = OTA_TYPE_MCU_FW_STREAM;
        config->download_sflash_addr = 0;
        pal_app_at_command_status(ATCMD_STATUS_MCUOTA);
        memcpy(config->url, _url, strlen(_url));
        printf(" %s url %s\n", "MCU_Stream", config->url);

        return 1;
    }
    else
    {
        return 0;
    }
#else
    RA6W1_UNUSED_ARG(_url);
    RA6W1_UNUSED_ARG(config);

    return 0;
#endif
}

int8_t pal_app_notify_mcu_ota(ota_update_type update_type, uint32_t status, uint32_t progress, uint32_t renew)
{
    APP_UNUSED_ARG(progress);

#if defined (__SUPPORT_ATCMD__)
    if (update_type != OTA_TYPE_MCU_FW_STREAM)
        return 0;

    if ((status == OTA_SUCCESS))
    {
        pal_app_at_command_status(ATCMD_STATUS_STA_DONE);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_OTA_RESULT, AWS_OTA_RESULT_OK))
            printf("write AWS_NVRAM_CONFIG_OTA_RESULT failed\n");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
        printf("Succeeded to replace with new MCU FW.\n");
        reboot_func(SYS_REBOOT);
    }
    else
    {
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_OTA_RESULT, AWS_OTA_RESULT_NG))
            printf("write AWS_NVRAM_CONFIG_OTA_RESULT failed\n");
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
        printf("Failed to replace with new FW. (Err : 0x%02lx)\n", status);
        if (renew == 0)
        {
            printf(">>> System Reboot !!!\n");
            reboot_func(SYS_REBOOT);
        }
    }

    return 1;
#else
    APP_UNUSED_ARG(update_type);
    APP_UNUSED_ARG(status);
    APP_UNUSED_ARG(renew);

    return 0;
#endif
}
#endif  // __SUPPORT_OTA__

uint8_t pal_app_controlCommand(app_dpm_info_rtm *_rtmData)
{
#if defined (__SUPPORT_ATCMD__)
    int32_t subsCnt;
    uint32_t wait_cnt = 0;
    char *atSubsCmd = NULL;
    APP_UNUSED_ARG(_rtmData);

    subsCnt = app_at_get_registered_subscribe_count();
    if ((subsCnt > 0) && (subsCnt > userCommand))
    {
        atSubsCmd = app_at_get_cloud_cmd_payload(userCommand);
        printf("\r\nsend \"%s %s\" to MCU\r\n", CMD_SERVER_DATA, atSubsCmd);
        wait_MCU_resp(1);
        if ((mcu_wakeup_port >= 0) && (mcu_wakeup_pin > 0x00))
            atcmd_mcu_wakeup(mcu_wakeup_port, mcu_wakeup_pin);

        memset(atBuf, 0, sizeof(atBuf));
        sprintf(atBuf, "\r\n+%sIOT=%s %s\r\n", PLATFORM, CMD_SERVER_DATA, atSubsCmd);
        vTaskDelay(portCONVERT_MS_2_TICKS(50));
        RM_PL_PRINTF_ATCMD(atBuf);
        while (get_MCU_resp() == 1)
        {
            if (wait_cnt > MAX_RESPONSE_WAIT_COUNT)
            {
                printf("\r\n [%s] break loop out(=%lu) \r\n", __func__, wait_cnt);
                break;
            }
            vTaskDelay(portCONVERT_MS_2_TICKS(10));
            wait_cnt++;
        }
    }

    return 1;
#else
    APP_UNUSED_ARG(_rtmData);

    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

int8_t pal_app_atc_work(app_dpm_info_rtm *_rtmData)
{
#if defined (__SUPPORT_ATCMD__)
    MQTTStatus_t rc = app_get_mqtt_status();
    uint32_t wait_cnt = 0;
    BaseType_t xStatus = pdPASS;
    app_dpm_info_rtm *data = (app_dpm_info_rtm*)_rtmData;
    DL_dpm_info_rtm *DL_data = NULL;
    char *ota_url = NULL;

    if (rc != MQTTSuccess)
        xStatus = pdFAIL;

    DL_data = DL_dpm_info_get(DL_DATA_RTM_NAME);
    printf("*************************************************************************"
        "**************\n");
    printf("\r\nsend \"%s %s\" to MCU\r\n", CMD_TO_MCU, APP_CONTROL_ATUPDATE_PREFIX);
    wait_MCU_resp(2); // shadow response wait
    if ((mcu_wakeup_port >= 0) && (mcu_wakeup_pin > 0x00))
        atcmd_mcu_wakeup((uint8_t)mcu_wakeup_port, mcu_wakeup_pin);

    memset(atBuf, 0, sizeof(atBuf));
    sprintf(atBuf, "\r\n+AWSIOT=%s %s\r\n", CMD_TO_MCU, APP_CONTROL_ATUPDATE_PREFIX);
    vTaskDelay(portCONVERT_MS_2_TICKS(50));
    RM_PL_PRINTF_ATCMD(atBuf);
    while (get_MCU_resp() == 2)
    {
        if (wait_cnt > MAX_RESPONSE_WAIT_COUNT)
        {
            printf("\r\n [%s] break loop out(=%lu) \r\n", __func__, wait_cnt);
            break;
        }
        vTaskDelay(portCONVERT_MS_2_TICKS(10));
        wait_cnt++;
    }
    printf("*************************************************************************"
        "**************\n");

    if (sizeof(DEFAULT_SHADOW_NAME) > 1)
        sprintf(topic_buf, "%s%s%s%s%s%s", SHADOW_PREFIX, getAppThingName(),
                SHADOW_NAMED_ROOT, DEFAULT_SHADOW_NAME, SHADOW_OP_UPDATE, "");
    else
        sprintf(topic_buf, "%s%s%s%s%s", SHADOW_PREFIX, getAppThingName(),
                SHADOW_CLASSIC_ROOT, SHADOW_OP_UPDATE, "");

    pal_app_making_shadow_payload(payload_buf, sizeof(payload_buf));
    xStatus = xPublishToTopic(app_get_mqtt_context(),
                              topic_buf,
                              strlen(topic_buf),
                              payload_buf,
                              strlen(payload_buf));
    if (xStatus != pdPASS)
    {
        printf("publish (shadow sensor update) NG\n");
        rc = MQTTSendFailed;
    }
    else
    {
        printf("publish (shadow sensor update) OK -  payload: \"%s\"\n", payload_buf);
    }

    if (data && (xStatus == pdPASS))
    {
        ota_url = app_get_ota_url();

        data->FOTAStat = (uint32_t)getOTAStat();
        if (getOTAStat() == (uint32_t)OTA_STAT_JOB_READY && strlen(ota_url) > 0)
            sprintf((char*)data->FOTAUrl, "%s", ota_url);
        else if (getOTAStat() == (INT32)OTA_STAT_JOB_CONFIRMED && strlen(ota_url) > 0)
            memset(data->FOTAUrl, 0, sizeof(data->FOTAUrl));

        printf("last user Timer ID = %d\n", data->tid);
        printf("last doorOpenFlag state: \"%s\"\n", DL_data->doorOpen ? "true" : "false");
        printf("last FOTA Stat: %d\n", getOTAStat());
        /* check printable code */
        if ((UINT8)ota_url[0] > 0x7F)
            printf("last FOTA Url: \"???\"\n");
        else
            printf("last FOTA Url: \"%s\"\n", ota_url);
    }

    if (rc != MQTTSuccess)
    {
        printf("[%s] reconnecting...\n", __func__);
        app_aws_dpm_app_connect(data);
    }

    return 1;
#else
    APP_UNUSED_ARG(_rtmData);

    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

int8_t pal_is_support_mcu_update(void)
{
#if defined (__SUPPORT_ATCMD__)
    return 1;
#else
    return 0;
#endif
}

char* pal_app_get_broker_endpoint(char *broker)
{
#if defined(__SUPPORT_ATCMD__)
    APP_UNUSED_ARG(broker);
    pal_app_get_atcmd_parameter();

    return atnvParam.broker;
#else
    return broker;
#endif  /* __SUPPORT_ATCMD__ */
}

uint32_t pal_app_get_broker_port(uint32_t port)
{
#if defined(__SUPPORT_ATCMD__)
    APP_UNUSED_ARG(port);
    pal_app_get_atcmd_parameter();

    return atnvParam.port;
#else
    return port;
#endif  /* __SUPPORT_ATCMD__ */
}

int8_t pal_app_event_cb(char *payload, int32_t len, bool *connect, bool *control,
                        bool *updatesensor)
{
#if defined (__SUPPORT_ATCMD__)
    int32_t subItemCnt = 0;
    int i;
    char *atRegisteredSubscribe = NULL;
    char *foundStr = NULL;

    printf("pal_app_event_cb %s\n", payload);
    if (strncmp(payload, AWS_CONTROL_OTA, strlen(AWS_CONTROL_OTA)) == 0)
    {
        printf("confirmOTA comm\n");
        setOTAStat(OTA_STAT_JOB_CONFIRMED);
        userCommand = Cmd_Idle;
    }
    else if (strncmp(payload, AWS_CONTROL_CONNECT, strlen(AWS_CONTROL_CONNECT)) == 0 && *connect == false)
    {
        printf("connect comm\n");
        *connect = true;
        userCommand = Cmd_Idle;
    }
    else if (strncmp(payload, AWS_CONTROL_SENSOR, strlen(AWS_CONTROL_SENSOR)) == 0)
    {
        printf("updateSensor comm\n");
        *updatesensor = true;
        userCommand = Cmd_Idle;
    }
    else
    {
        subItemCnt = app_at_get_registered_subscribe_count();
        if (subItemCnt > 0)
        {
            for (i = 0; i < subItemCnt; i++)
            {
                foundStr = NULL;
                atRegisteredSubscribe = app_at_get_registered_subscribe(i);

                if (strlen(atRegisteredSubscribe) > 0)
                {
                    foundStr = strstr(payload, atRegisteredSubscribe);
                    if (foundStr && (*control == false))
                    {
                        printf("dynamic subscription command (\"%s\")\n", atRegisteredSubscribe);
                        app_at_set_cloud_cmd_payload(payload, len);
                        *control = true;
                        userCommand = i;
                        break;
                    }
                }
            }
        }
    }

    return 1;
#else
    APP_UNUSED_ARG(payload);
    APP_UNUSED_ARG(len);
    APP_UNUSED_ARG(connect);
    APP_UNUSED_ARG(control);
    APP_UNUSED_ARG(updatesensor);

    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

void pal_app_publish_user_contents(char *_payload)
{
#if defined (__SUPPORT_ATCMD__)
    MQTTStatus_t rc = app_get_mqtt_status();
    BaseType_t xStatus = pdPASS;
    char *pub_topic;
    int k = 0;
    char *params[10] = { 0, };

    memset(payload_buf, 0, sizeof(payload_buf));
    memset(topic_buf, 0, sizeof(topic_buf));
    memcpy(payload_buf, _payload, strlen(_payload));

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_PTOPIC, &pub_topic);
    sprintf(topic_buf, "%s/%s", getAppThingName(), pub_topic);
    params[k] = strtok(payload_buf, " ");

    while (params[k] != NULL)
    {
        k++;
        params[k] = strtok(NULL, " ");
    }

    xStatus = xPublishToTopic(app_get_mqtt_context(),
                              topic_buf, strlen(topic_buf),
                              _payload, strlen(_payload));

    if (xStatus != pdPASS)
    {
        printf("publish (contents update) NG - topic : %s payload: %s\n", topic_buf, _payload);
        rc = MQTTSendFailed;
    }
    else
    {
        printf("publish (contents update) OK - topic : %s payload: %s\n", topic_buf, _payload);
        rc = MQTTSuccess;
    }

    app_set_mqtt_status(rc);
#else
    APP_UNUSED_ARG(_payload);
#endif
    return;
}

int8_t pal_app_reg_subscription_topic(void)
{
#if defined (__SUPPORT_ATCMD__)
    BaseType_t xStatus = pdPASS;
    char *sub_topic;

    memset(topic_buf, 0, sizeof(topic_buf));
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_STOPIC, &sub_topic);
    sprintf(topic_buf, "%s/%s", getAppThingName(), sub_topic);
    xStatus = xSubscribeToTopic(app_get_mqtt_context(), topic_buf, strlen(topic_buf));
    if (xStatus == pdPASS)
        return 1;
    else
        return 2;
#else
    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

#if defined (__SUPPORT_ATCMD__)
static void pal_app_making_shadow_payload(char *payload, uint32_t lpayload)
{
    int8_t i;
    char *ptr = payload;

    memset(ptr, 0x00, lpayload);
    sprintf(ptr, "{\"state\":{\"reported\":{");
    ptr = payload + strlen(payload);
    for (i = 0; i < gShadowCount; i++)
    {
        if (registeredShadow[i].type == SHADOW_JSON_INT32)
        {
            if (i != 0)
            {
                sprintf(ptr, ",");
                ptr = payload + strlen(payload);
            }
            sprintf(ptr, "\"%s\":%d", registeredShadow[i].pKey,
                    *(int*)registeredShadow[i].pData);
            ptr = payload + strlen(payload);
        }
        else if (registeredShadow[i].type == SHADOW_JSON_STRING)
        {
            if (i != 0)
            {
                sprintf(ptr, ",");
                ptr = payload + strlen(payload);
            }
            sprintf(ptr, "\"%s\":\"%s\"", registeredShadow[i].pKey,
                    (char*)registeredShadow[i].pData);
            ptr = payload + strlen(payload);
        }
        else if (registeredShadow[i].type == SHADOW_JSON_FLOAT)
        {
            if (i != 0)
            {
                sprintf(ptr, ",");
                ptr = payload + strlen(payload);
            }
            sprintf(ptr, "\"%s\":%d.%06d", registeredShadow[i].pKey,
                    ((int)*(float *)registeredShadow[i].pData),
                    (int)((float)((float)*(float *)registeredShadow[i].pData - (int)*(float *)registeredShadow[i].pData) * 1000) % 1000);
            ptr = payload + strlen(payload);
        }
        else
        {
            printf("registerShadow Wrong data (%s)\n", registeredShadow[i].pKey);
        }
    }
    sprintf(ptr, ",");
    ptr = payload + strlen(payload);
    sprintf(ptr, "\"OTAupdate\":%d,", getOTAStat());
    ptr = payload + strlen(payload);
    sprintf(ptr, "\"OTAresult\":\"%s\"", app_get_ota_result());
    ptr = payload + strlen(payload);
    sprintf(ptr, "}},\"clientToken\":\"%s-0\"}", getAppThingName());

    return;
}
#endif

#endif  /* __SUPPORT_AWS_IOT_W__ */
