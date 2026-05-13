/**
 ****************************************************************************************
 *
 * @file app_atcommand.c
 *
 * @brief The at command interface code of Apps at-command
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

#include "FreeRTOS.h"
#include "task.h"

#if defined (__SUPPORT_ATCMD__) && (defined (__SUPPORT_AZURE_IOT__) || defined (__SUPPORT_AWS_IOT__))

#include "nvedit.h"
#include "environ.h"
#include "sys_feature.h"
#include "da16x_system.h"
#include "command.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_ping.h"
#include "da16x_cert.h"

#include "common_utils.h"
#include "command_net.h"
#include "app_provision.h"
#include "app_common_util.h"

#include "app_atcommand.h"
#include "app_atcommand_pal.h"
#include "atcmd.h"

#include "app_thing_manager.h"
#include "common_config.h"

extern int read_nvram_int(const char *name, int *_val);
extern char* read_nvram_string(const char *name);
extern int write_nvram_int(const char *name, int val);
extern int write_nvram_string(const char *name, const char *val);

extern void PRINTF_ATCMD(const char *fmt, ...);

#if defined (__BLE_COMBO_REF__)
extern UINT factory_reset_default(int reboot_flag);
#endif
extern int atcmd_uartconf(int argc, char *argv[]);

shadowNode *shadowNodehead = NULL;
jsonStruct_t registeredShadow[MAX_SHADOW_COUNT] = {0, };
char cmdToMCUPayload[MAX_CMD_TO_MCU_PAYLOAD] = {0, };

void check_current_heap(void);
char registeredSubsItems[MAX_SUBSCRIBE_COUNT][MAX_SUBSCRIBE_STRING] = {0, };
int32_t paramInteger[MAX_SHADOW_COUNT] = {0, };
char paramString[MAX_SHADOW_COUNT][MAX_SHADOW_STRING] = {0, };
float paramFloat[MAX_SHADOW_COUNT] = {0, };
int32_t paramIntegerCnt = 0;
int32_t paramStringCnt = 0;
int32_t paramFloatCnt = 0;
int32_t gShadowCount = 0;
int32_t gSubscribeCount = 0;
uint8_t flagATOK = 1;
static const uint16_t RO_fast_crc_nbit_LUT[4][16] = { {swap16(0x0000), swap16(0x3331), swap16(0x6662), swap16(0x5553), swap16(
    0xccc4), swap16(0xfff5), swap16(0xaaa6), swap16(0x9997), swap16(0x89a9), swap16(0xba98), swap16(0xefcb), swap16(0xdcfa),
    swap16(0x456d), swap16(0x765c), swap16(0x230f), swap16(0x103e)}, {swap16(0x0000), swap16(0x0373), swap16(0x06e6), swap16(
    0x0595), swap16(0x0dcc), swap16(0x0ebf), swap16(0x0b2a), swap16(0x0859), swap16(0x1b98), swap16(0x18eb), swap16(0x1d7e),
    swap16(0x1e0d), swap16(0x1654), swap16(0x1527), swap16(0x10b2), swap16(0x13c1)}, {swap16(0x0000), swap16(0x1021), swap16(
    0x2042), swap16(0x3063), swap16(0x4084), swap16(0x50a5), swap16(0x60c6), swap16(0x70e7), swap16(0x8108), swap16(0x9129),
    swap16(0xa14a), swap16(0xb16b), swap16(0xc18c), swap16(0xd1ad), swap16(0xe1ce), swap16(0xf1ef)}, {swap16(0x0000), swap16(
    0x1231), swap16(0x2462), swap16(0x3653), swap16(0x48c4), swap16(0x5af5), swap16(0x6ca6), swap16(0x7e97), swap16(0x9188),
    swap16(0x83b9), swap16(0xb5ea), swap16(0xa7db), swap16(0xd94c), swap16(0xcb7d), swap16(0xfd2e), swap16(0xef1f)}};

extern int8_t mcu_wakeup_port;
extern uint16_t mcu_wakeup_pin;

static int readyIF = 0;

ATCMDScanResult latestStatus = ATCMD_Status_IDLE;

HANDLE atcmd_uart_ctrl;
extern void wait_MCU_resp(uint8_t owner);
extern uint8_t get_MCU_resp(void);

void l_write_nvram_string(const char *name, const char *val)
{
    int result = 0;

    result = write_nvram_string(name, val);
    if (result != 0) {
        APRINTF_E("ERROR!! fail to write at NVRAM  . \n");
    }
}

void l_write_nvram_int(const char *name, int val)
{
    int result = 0;

    result = write_nvram_int(name, val);
    if (result != 0) {
        APRINTF_E("ERROR!! fail to write at NVRAM  . \n");
    }
}

char* app_get_string_nvram(const char *name)
{
    return read_nvram_string(name);
}

int app_get_int_nvram(const char *name, int *_val)
{
    return read_nvram_int(name, _val);
}

void APP_PRINTF_ATCMD(char *_payload, uint32_t _option)
{
    int32_t cntTenSleep;
    int32_t cntResend = 1;

    if (_option == RETRY_ACK_CHECK_SKIP) {
        PRINTF_ATCMD(_payload);
    } else {
        flagATOK = 0;
RE_SEND: if ((mcu_wakeup_port >= 0) && (mcu_wakeup_pin > 0x00))
            atcmd_mcu_wakeup((uint8_t)mcu_wakeup_port, mcu_wakeup_pin);
        PRINTF_ATCMD(_payload);
        cntTenSleep = 1;

        while (!flagATOK) {
            vTaskDelay(5);
            if (cntResend > 10) {
                APRINTF_E("%s:%d - forced to break\n", __func__, __LINE__);
                break;
            }
            if (cntTenSleep++ >= MAX_RESEND_SLEEP_CNT) {
                APRINTF_E("%s:%d - resent count =%d\n", __func__, __LINE__, cntResend++);
                goto RE_SEND;
            }
        }
        cntResend = 0;
    }
}

int checkconfigdataAtNVRAM(int num)
{
    char *nvramName = NULL;
    char tempNVname[TEMP_NV_NAME_SIZE] = {0, };

    da16x_sprintf(tempNVname, "%s_%d_0", NV_CONFIG_THING_ATTIRIBUTE, num);
    nvramName = read_nvram_string(tempNVname);

    if (nvramName == NULL)
        return 0;
    else
        return 1;
}

atComdataConfig* app_get_at_config(int num)
{
    atComdataConfig *retData = NULL;
    char tempNVname[TEMP_NV_NAME_SIZE] = {0, };
    int attNum = 0;
    int rc = 0;
    int filedNumber = 0;
    int filedtype = 0;
    int filedmqtttype = 0;

    if (checkconfigdataAtNVRAM(num) == 0) {
        return NULL;
    }

    retData = (atComdataConfig*)pvPortMalloc(sizeof(atComdataConfig));

    if (retData) {
        da16x_sprintf(tempNVname, "%s_%d_%d", NV_CONFIG_THING_ATTIRIBUTE, num, attNum);
        attNum++;
        rc = app_get_int_nvram(tempNVname, &filedNumber);
        if (!rc)
            retData->filedNum = filedNumber;
        else
            goto INVALID_VAL;

        da16x_sprintf(tempNVname, "%s_%d_%d", NV_CONFIG_THING_ATTIRIBUTE, num, attNum);
        attNum++;
        retData->name = app_get_string_nvram(tempNVname);
        if (retData->name == NULL)
            goto INVALID_VAL;

        da16x_sprintf(tempNVname, "%s_%d_%d", NV_CONFIG_THING_ATTIRIBUTE, num, attNum);
        attNum++;
        rc = app_get_int_nvram(tempNVname, &filedtype);
        if (!rc)
            retData->type = filedtype;
        else
            goto INVALID_VAL;

        da16x_sprintf(tempNVname, "%s_%d_%d", NV_CONFIG_THING_ATTIRIBUTE, num, attNum);
        attNum++;
        rc = app_get_int_nvram(tempNVname, &filedmqtttype);
        if (!rc)
            retData->mqtttype = filedmqtttype;
        else
            goto INVALID_VAL;
    } else {
        APRINTF_E("%s> malloc failed\n", __func__);
    }

    return retData;
INVALID_VAL: if (retData != NULL)
        vPortFree(retData);
    return NULL;
}

void app_release_at_config(atComdataConfig *_config)
{
    if (_config != NULL) {
        vPortFree(_config);
        _config = NULL;
    }
}

int app_iot_set_feature_parser(int argc, char *argv[])
{
    atcmd_error_code errorCode = AT_CMD_ERR_CMD_OK;
    APRINTF_I("APP SET %s \n", argv[2]);
    if (strcmp(argv[0], "help") == 0) {
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_THINGNAME) == 0) { /* thing name */
        l_write_nvram_string(APP_NVRAM_CONFIG_THINGNAME, argv[2]);
#if defined ( __SUPPORT_AWS_IOT__ )
    } else if (strcmp(argv[1], USE_FLEET_PROVISION) == 0) { /* Fleet Provisioning */
        l_write_nvram_string(USE_FLEET_PROVISION, argv[2]);
    } else if (strcmp(argv[1], AWS_NVRAM_CONFIG_BROKER_URL) == 0) { /* Broker URL */
        l_write_nvram_string(AWS_NVRAM_CONFIG_BROKER_URL, argv[2]);
#elif defined ( __SUPPORT_AZURE_IOT__ )
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_DEV_PRIMARY_KEY) == 0) { /* Primary Key */
        l_write_nvram_string(APP_NVRAM_CONFIG_DEV_PRIMARY_KEY, argv[2]);
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_HOST_NAME) == 0) { /* Host name */
        l_write_nvram_string(APP_NVRAM_CONFIG_HOST_NAME, argv[2]);
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_IOTHUB_CONN_STRING) == 0) { /* Connection String */
        l_write_nvram_string(APP_NVRAM_CONFIG_IOTHUB_CONN_STRING, argv[2]);
#endif
    } else if (strcmp(argv[1], NV_BOARD_FEATURE) == 0) { /* feature and pin mux */
        l_write_nvram_string(NV_BOARD_FEATURE, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_AMUX) == 0) {
        l_write_nvram_string(NV_PIN_AMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_BMUX) == 0) {
        l_write_nvram_string(NV_PIN_BMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_CMUX) == 0) {
        l_write_nvram_string(NV_PIN_CMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_DMUX) == 0) {
        l_write_nvram_string(NV_PIN_DMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_EMUX) == 0) {
        l_write_nvram_string(NV_PIN_EMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_FMUX) == 0) {
        l_write_nvram_string(NV_PIN_FMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_HMUX) == 0) {
        l_write_nvram_string(NV_PIN_HMUX, argv[2]);
    } else if (strcmp(argv[1], NV_PIN_UMUX) == 0) {
        l_write_nvram_string(NV_PIN_UMUX, argv[2]);
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_LPORT) == 0) { /* local port */
        l_write_nvram_int(APP_NVRAM_CONFIG_LPORT, atoi(argv[2]));
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_STOPIC) == 0) { /* subscribing topic */
        l_write_nvram_string(APP_NVRAM_CONFIG_STOPIC, argv[2]);
    } else if (strcmp(argv[1], APP_NVRAM_CONFIG_PTOPIC) == 0) { /* publishing topic */
        l_write_nvram_string(APP_NVRAM_CONFIG_PTOPIC, argv[2]);
    } else if (strcmp(argv[1], AT_DPM_SLEEP_MODE) == 0) { /* sleep mode */
        app_atcmd_set_DPM_value(TYPE_SLEEP_MODE, atoi(argv[2]));
    } else if (strcmp(argv[1], AT_DPM_USE_DPM) == 0) { /* DPM use */
        app_atcmd_set_DPM_value(TYPE_USE_DPM, atoi(argv[2]));
    } else if (strcmp(argv[1], AT_DPM_RTC_TIME) == 0) { /* keepalive wakeup time */
        app_atcmd_set_DPM_value(TYPE_RTC_TIME, atoi(argv[2]));
    } else if (strcmp(argv[1], AT_DPM_KEEP_ALIVE) == 0) { /* dpm keepalive time */
        app_atcmd_set_DPM_value(TYPE_DPM_KEEP_ALIVE, atoi(argv[2]));
    } else if (strcmp(argv[1], AT_DPM_USE_WAKE_UP) == 0) { /* user wakeup time */
        app_atcmd_set_DPM_value(TYPE_USER_WAKE_UP, atoi(argv[2]));
    } else if (strcmp(argv[1], AT_DPM_TIM_WAKE_UP) == 0) { /* TIM wakeup time */
        app_atcmd_set_DPM_value(TYPE_TIM_WAKE_UP, atoi(argv[2]));
    } else if (strcmp(argv[1], NV_MCU_WAKEUP_PORT) == 0) { /* mcu wake up gpio_unit_A or C */
        l_write_nvram_string(NV_MCU_WAKEUP_PORT, argv[2]);
    } else if (strcmp(argv[1], NV_MCU_WAKEUP_PIN) == 0) { /* mcu wake up gpio_pin0~11 */
        l_write_nvram_string(NV_MCU_WAKEUP_PIN, argv[2]);
    } else if (strcmp(argv[1], NV_MCU_UART_CFG) == 0) { /* UART1 baudrate: SET UART_CFG 115200 */
        /* l_write_nvram_int(NV_MCU_UART_CFG, atoi(argv[2])); */
        int32_t rcode;
        rcode = atcmd_uartconf(argc - 1, &argv[1]);
        if (rcode != 0) {
            APRINTF_E("ERROR!! uart configuration(rc=%d)\n", rcode);
            errorCode = AT_CMD_ERR_WRONG_ARGUMENTS;
        }
    } else {
        APRINTF_E("ERROR!! wrong name \n");
        errorCode = AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    /* add */
#if defined(AT_ACK_CHECK)
    if (errorCode == AT_CMD_ERR_CMD_OK)
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */

    return errorCode;
}

int app_iot_config_thing_parser(int argc, char *argv[])
{
    char tempNVname[TEMP_NV_NAME_SIZE] = {0, };
    atcmd_error_code errorCode = AT_CMD_ERR_CMD_OK;
    int attNum = 0;

    if (argc < 5) {
        errorCode = AT_CMD_ERR_WRONG_ARGUMENTS;
        APRINTF_E("Error! Need  more data. [Number][name][vlaue-type][mqtt-type]\n");
        return errorCode;
    }
    APRINTF_I("\n=======================================================\n");

    da16x_sprintf(tempNVname, "%s_%s_%d", NV_CONFIG_THING_ATTIRIBUTE, argv[1], attNum);
    attNum++;
    l_write_nvram_int(tempNVname, atoi(argv[1]));
    APRINTF_I("Att[%d] number   : %d\n", atoi(argv[1]), atoi(argv[1]));

    da16x_sprintf(tempNVname, "%s_%s_%d", NV_CONFIG_THING_ATTIRIBUTE, argv[1], attNum);
    attNum++;
    l_write_nvram_string(tempNVname, argv[2]);
    APRINTF_I("Att[%d] name     : %s\n", atoi(argv[1]), argv[2]);

    da16x_sprintf(tempNVname, "%s_%s_%d", NV_CONFIG_THING_ATTIRIBUTE, argv[1], attNum);
    attNum++;
    l_write_nvram_int(tempNVname, atoi(argv[3]));
    APRINTF_I("Att[%d] data type: %d\n", atoi(argv[1]), atoi(argv[3]));

    da16x_sprintf(tempNVname, "%s_%s_%d", NV_CONFIG_THING_ATTIRIBUTE, argv[1], attNum);
    attNum++;
    l_write_nvram_int(tempNVname, atoi(argv[4]));
    APRINTF_I("Att[%d] MQTT type: %d\n", atoi(argv[1]), atoi(argv[4]));

    APRINTF_I("=======================================================\n\n");

#if defined(AT_ACK_CHECK)
    if (errorCode == AT_CMD_ERR_CMD_OK) {
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
    }
#endif  /* AT_ACK_CHECK */

    return errorCode;
}

int app_iot_at_response_ok_parser(int argc, char *argv[])
{
    atcmd_error_code errorCode = AT_CMD_ERR_CMD_OK;

    if (argc < 1) {
        errorCode = AT_CMD_ERR_INSUFFICENT_ARGS;
        APRINTF_E("Error! Need more data\n");
        return errorCode;
    }

    if ((strncmp(argv[0], "OK_OTA", 5) == 0)) {
        if ((strncmp(argv[0], "OK_OTA_FAIL", 10) == 0)) {
            if (get_MCU_resp() == 3) {
                wait_MCU_resp(99);
            }
        } else {
            if (get_MCU_resp() == 3) {
                wait_MCU_resp(0);
            }
        }
    } else if ((strncmp(argv[0], "OK", 2) == 0)) {
        /* check payload length */
        flagATOK = 1;
    }

    return errorCode;
}

int app_iot_at_push_com_parser(int argc, char *argv[])
{
    char *tmp_read = NULL;
    char readit[32] = {0, };
    char payload[TEMP_AT_DATA_SIZE] = {0, };
    int i, j;
    int cmdNum;
    atComdataConfig *retData = NULL;
    atcmd_error_code errorCode = AT_CMD_ERR_CMD_OK;
    uint8_t mcu_res = 0; /* publish : 1, shadow = 2 */
#if defined ( __SUPPORT_AZURE_IOT__ )
    struct atcmd_twin_params *atParams;
#endif

    tmp_read = app_get_string_nvram(NV_BOARD_FEATURE);
    strncpy(readit, tmp_read, strlen(tmp_read));
    APRINTF_I("\nboard feature : %s\n", readit);

    if (argc < 2) {
        errorCode = AT_CMD_ERR_INSUFFICENT_ARGS;
        APRINTF_E("Error! Need more data. [number]\n");
        return errorCode;
    }

    if (strcmp(argv[0], "help") == 0) {
    } else if (strcmp(argv[1], CMD_FATORY_RESET) == 0) { /* factroyreset */
        APRINTF_I(" CMD_FATORY_RESET \n");
#if defined(AT_ACK_CHECK)
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */
#if !defined (__BLE_COMBO_REF__)
        app_reboot_ap_mode(SYSMODE_AP_ONLY, 1, 1); /* factory reset */
#else
        factory_reset_default(1); /* for da16600 */
#endif
    } else if (strcmp(argv[1], CMD_RESET_TO_AP) == 0) {
        APRINTF_I(" TO AP resetMode\n");
#if defined(AT_ACK_CHECK)
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */
#if !defined (__BLE_COMBO_REF__)
        if (dpm_mode_is_enabled()) {
            dpm_mode_disable();
            /* save provisioning flag for DPM mode */
            write_nvram_int(APP_NVRAM_CONFIG_DPM_AUTO, 1);
        }

        app_reboot_ap_mode(SYSMODE_AP_ONLY, 1, 0); /* reset to AP */
#else
        factory_reset_default(1); /* for da16600 */
#endif
    } else if (strcmp(argv[1], CMD_GET_STATUS) == 0) { /* factroyreset */
        app_iot_at_command_status(latestStatus);
    } else if (strcmp(argv[1], CMD_RESTART) == 0) { /* reboot */
#if defined ( __SUPPORT_AWS_IOT__ )
        APRINTF_E("AWS_IOT Reboot ... \n");
#elif defined ( __SUPPORT_AZURE_IOT__ )
        APRINTF_E("AZURE_IOT Reboot ... \n");
#endif
        vTaskDelay(10);
#if defined(AT_ACK_CHECK)
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */
        reboot_func(SYS_REBOOT_POR);
    } else if (strcmp(argv[1], CMD_MCU_MQTT) == 0) {
#if defined(AT_ACK_CHECK)
        /* APRINTF("%s:%d \n", __func__, __LINE__); */
        APP_PRINTF_ATCMD(CMD_RESPONSE_OK_STR, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */
        if (argc < 5) {
            errorCode = AT_CMD_ERR_INSUFFICENT_ARGS;
            APRINTF_E("Error! Need more data. [number][name][value]\n");
            return errorCode;
        }

        /* CMD[0] MCU_DATA[1] 4[2] battery[3] 2700[4] */
        int topicCount = 0;

        for (i = 2; i < 100; i++) {
            if (argv[i] == NULL) {
                topicCount = topicCount / 3;
                APRINTF_I("topicCount : %d\n", topicCount);
                break;
            }

            topicCount++;
        }

        for (i = 0; i < topicCount; i++) {
            cmdNum = atoi(argv[2 + (i * 3)]);
            retData = app_get_at_config(cmdNum);
            APRINTF_I("\n\nCount : %d, cmdNum = %d\n", i, cmdNum);

            if (retData) {
                if (retData->filedNum == cmdNum) {
                    APRINTF_I("mqtttype = %d\n", retData->mqtttype);
                    if (retData->mqtttype == CONFIG_MQTT_TYPE_SHADOW) {
                        /* find same pkey */
                        for (j = 0; j < MAX_SHADOW_STRING; j++) {
                            if (strcmp(registeredShadow[j].pKey, retData->name) == 0) {
                                APRINTF_I("index(=%d) matched\n", j);
                                break;
                            }
                        }

                        if (j == MAX_SHADOW_STRING) {
                            APRINTF_I("matched index not found\n");
                            continue;
                        }

                        APRINTF_I("data type(shadow) = %d\n", retData->type);
                        mcu_res = 2;

                        if (retData->type == CONFIG_DATA_TYPE_INT) {
                            APRINTF_I("call update sensor(need to be set variable): %s = %d\n", retData->name,
                                atoi(argv[4 + (i * 3)]));
                            *(int*)registeredShadow[j].pData = atoi(argv[4 + (i * 3)]);
                        } else if (retData->type == CONFIG_DATA_TYPE_STR) { /* atcmd::todo -> test needed */
                            APRINTF_I("call update sensor(need to be set variable): %s = %s\n", retData->name, argv[4 + (i * 3)]);
                            memcpy(registeredShadow[j].pData, argv[4 + (i * 3)], strlen(argv[4 + (i * 3)]) + 1);
                        } else if (retData->type == CONFIG_DATA_TYPE_FLOAT) {
                            APRINTF_I("call update sensor(need to be set variable): %s = %.6f\n", retData->name,
                                atof(argv[4 + (i * 3)]));
                            *(float*)registeredShadow[j].pData = (float)atof(argv[4 + (i * 3)]);
                        } else {
                            APRINTF_I("%s's type(=%d) invalid\n", retData->name, retData->type);
                            errorCode = AT_CMD_ERR_NOT_SUPPORTED;
                            goto QUIT;
                        }
                    } else if (retData->mqtttype == CONFIG_MQTT_TYPE_PUBLISH) { /* support only 1 item */
                        APRINTF_I("data type(publish)=%d\n", retData->type);
                        mcu_res = 1;
                        if (retData->type == CONFIG_DATA_TYPE_INT) {
                            da16x_sprintf(payload, "%s %s %d", argv[2], argv[3], atoi(argv[4]));
                            APRINTF_I("call publish: %s\n", payload);
                        } else if (retData->type == CONFIG_DATA_TYPE_STR) {
                            da16x_sprintf(payload, "%s %s %s", argv[2], argv[3], argv[4]);
                            APRINTF_I("call publish: %s\n", payload);
                        } else if (retData->type == CONFIG_DATA_TYPE_FLOAT) {
                            da16x_sprintf(payload, "%s %s %.6f", argv[2], argv[3], atof(argv[4]));
                            APRINTF_I("call publish: %s\n", payload);
                        } else {
                            APRINTF_I("%s's type(=%d) invalid\n", retData->name, retData->type);
                            errorCode = AT_CMD_ERR_NOT_SUPPORTED;
                            goto QUIT;
                        }
#if defined ( __SUPPORT_AWS_IOT__ )
                        pal_app_publish_user_contents(payload);
#elif defined ( __SUPPORT_AZURE_IOT__ )
                        atParams = app_at_get_twins();
                        if (strcmp(argv[2], "1") == 0) {
                            if (strcmp(argv[3], "mcu_door") == 0) {
                                if (strcmp(argv[4], "opened") == 0) {
                                    atParams->doorState = 1;
                                    atParams->doorlock_state = 1; /* Door_Open */
                                    atParams->doorStateChange = 1;
                                    APRINTF_I("call mcu_door opened set\n");
                                } else if (strcmp(argv[4], "closed") == 0) {
                                    atParams->doorState = 0;
                                    atParams->doorlock_state = 2; /* Door_Close */
                                    atParams->doorStateChange = 1;
                                    APRINTF_I("call mcu_door closed set\n");
                                }
                            }
                        }
                        app_at_set_twins(atParams);
                        if (app_get_control_flag())
                            app_at_set_resp_string(payload, strlen(payload));
#endif
                    } else {
                        APRINTF_E("%s's mqtttype(=%d) invalid\n", retData->name, retData->mqtttype);
                        errorCode = AT_CMD_ERR_NOT_SUPPORTED;
                        goto QUIT;
                    }
                } else {
                    APRINTF_E("field number differet from passed numer\n");
                    errorCode = AT_CMD_ERR_NOT_SUPPORTED;
                    goto QUIT;
                }
            } else {
                APRINTF_E("no config data in nvram\n");
                errorCode = AT_CMD_ERR_NOT_SUPPORTED;
                goto QUIT;
            }
QUIT:      if (retData != NULL) {
                app_release_at_config(retData);
            }
        }
    }

    if (get_MCU_resp() == mcu_res) {
        wait_MCU_resp(0);
    }
    return errorCode;
}

void waitIFReady()
{
    while (readyIF == 0) {
        vTaskDelay(10);
        APRINTF_I(".");
    }
}

int isReadyIF()
{
    return readyIF;
}

void app_iot_set_feature(int argc, char *argv[])
{
    app_iot_set_feature_parser(argc, argv);
}

void app_iot_config_thing(int argc, char *argv[])
{
    app_iot_config_thing_parser(argc, argv);
}

void app_iot_at_push_com(int argc, char *argv[])
{
    app_iot_at_push_com_parser(argc, argv);
}

void app_iot_at_response_ok(int argc, char *argv[])
{
    app_iot_at_response_ok_parser(argc, argv);
}

void app_iot_at_command_status(ATCMDScanResult _val)
{
    char tmp[128] = {0, };

    waitIFReady();

    latestStatus = _val;
    if ((mcu_wakeup_port >= 0) && (mcu_wakeup_pin > 0x00)) {
        APRINTF_I("\r\n>>latestStatus : %d\r\n", _val);
        atcmd_mcu_wakeup((uint8_t)mcu_wakeup_port, mcu_wakeup_pin);
        vTaskDelay(5);
    }

    memset(tmp, 0, sizeof(tmp));
    da16x_sprintf(tmp, "\r\n+%sIOT=status %d\r\n", PLATFORM, _val);
#if defined(AT_ACK_CHECK)
    APP_PRINTF_ATCMD(tmp, RETRY_ACK_CHECK);
#else
    APP_PRINTF_ATCMD(tmp, RETRY_ACK_CHECK_SKIP);
#endif  /* AT_ACK_CHECK */
    vTaskDelay(10);
}

void app_iot_at_IF_ready_notification(int _val)
{
    APRINTF_I("[AT Interface ready notification]\n");
    readyIF = _val;
}

int app_iot_at_command(int argc, char *argv[])
{
    int i;
    int k = 0;
    char *params[MAX_ARGV_NUM] = {0, };

    APRINTF_I("\n======================================================= \n" , argc);
    APRINTF_I("argc num = %d \n", argc);
    for (i = 0; i < MAX_ARGV_NUM; i++) {
        if (argv[i] == NULL)
            break;

        APRINTF_I("argv[%d]: %s\n", i, argv[i]);
    }
    APRINTF_I("=======================================================\n");

    if (argc > 2) {
        k = 1;
        for (int j = 0; j < i - 1; j++) {
            params[j] = argv[k];
            k++;
        }
    } else {
        params[k] = strtok(argv[1], " ");

        while (params[k] != NULL) {
            /* APRINTF("%s\n", params[k]); */
            k++;
            params[k] = strtok(NULL, " ");
        }
    }

    if (strncmp(argv[1], "SET", 3) == 0)
        return app_iot_set_feature_parser(k, params);
    else if (strncmp(argv[1], "CFG", 3) == 0)
        return app_iot_config_thing_parser(k, params);
    else if (strncmp(argv[1], "CMD", 3) == 0)
        return app_iot_at_push_com_parser(k, params);
    else if (strncmp(argv[1], "OK", 2) == 0)
        return app_iot_at_response_ok_parser(k, params);
    else
        APRINTF_E("[APP-IOT] Not support or wrong commmand \n");

    return 0;
}

/* atcmd */
int32_t app_at_get_registered_subscribe_count(void)
{
    return gSubscribeCount;
}

void app_at_set_cloud_cmd_payload(char *_payload, int32_t _needLen)
{
    if (_payload && _needLen < MAX_CMD_TO_MCU_PAYLOAD && strlen(_payload) >= (unsigned int)_needLen) {
        memset(cmdToMCUPayload, 0, sizeof(cmdToMCUPayload));
        strncpy(cmdToMCUPayload, _payload, _needLen);
    }
}

char* app_at_get_cloud_cmd_payload(int32_t _index)
{
    if (_index < MAX_SUBSCRIBE_COUNT) {
        if (strlen(registeredSubsItems[_index]) > 0) {
            char *foundStr = NULL;
            foundStr = strstr(cmdToMCUPayload, registeredSubsItems[_index]);
            if (foundStr)
                return cmdToMCUPayload;
            else
                return NULL;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

char* app_at_get_registered_subscribe(int32_t _index)
{
    if (_index < MAX_SUBSCRIBE_COUNT)
        return registeredSubsItems[_index];
    else
        return NULL;
}

uint32_t app_at_init_shadow_items(void)
{
    int32_t i = 0;
    int32_t countItem = 0;
    atComdataConfig *item = NULL;

    gSubscribeCount = 0;

    for (i = 0; i < 100; i++) {
        item = app_get_at_config(i);
        if (item != NULL && item->filedNum == i) {
            if (item->mqtttype == CONFIG_MQTT_TYPE_SHADOW) {
                shadowNode *newNode = pvPortMalloc(sizeof(shadowNode));

                if (countItem >= MAX_SHADOW_COUNT) {
                    APRINTF_I("shadow count(=%d) over max count(=%d)\n", countItem, MAX_SHADOW_COUNT);
                    if (newNode != NULL)
                        vPortFree(newNode);
                    if (item)
                        app_release_at_config(item);
                    continue;
                }

                if (newNode) {
                    newNode->shadowData.cb = NULL;
                    APRINTF_I("newNode index=%d\n", i);
                    newNode->shadowData.pKey = item->name;

                    if (item->type == CONFIG_DATA_TYPE_INT) {
                        newNode->shadowData.type = SHADOW_JSON_INT32;
                        newNode->shadowData.pData = &paramInteger[paramIntegerCnt];
                        newNode->shadowData.dataLength = sizeof(int32_t);
                        *(int*)newNode->shadowData.pData = 2700;
                        paramIntegerCnt++;
                    } else if (item->type == CONFIG_DATA_TYPE_STR) {
                        newNode->shadowData.type = SHADOW_JSON_STRING;
                        newNode->shadowData.pData = &paramString[paramStringCnt];
                        newNode->shadowData.dataLength = sizeof(paramString[0]);
                        memset((char*)newNode->shadowData.pData, 0, sizeof(paramString[0]));
                        da16x_sprintf((char*)newNode->shadowData.pData, "%s", "test");
                        paramStringCnt++;
                    } else if (item->type == CONFIG_DATA_TYPE_FLOAT) {
                        newNode->shadowData.type = SHADOW_JSON_FLOAT;
                        newNode->shadowData.pData = &paramFloat[paramFloatCnt];
                        newNode->shadowData.dataLength = sizeof(float);
                        *(float*)newNode->shadowData.pData = 16.5;
                        paramFloatCnt++;
                    } else {
                        APRINTF_E("newNode type invalid\n");
                        if (newNode != NULL)
                            vPortFree(newNode);
                        if (item)
                            app_release_at_config(item);
                        break;
                    }
                    if (shadowNodehead == NULL) {
                        shadowNodehead = newNode;
                    } else {
                        newNode->next = shadowNodehead;
                        shadowNodehead = newNode;
                    }
                    countItem++;
                } else {
                    APRINTF_E("newNode malloc failed\n");
                    if (item)
                        app_release_at_config(item);
                    break;
                }
            } else if (item->mqtttype == CONFIG_MQTT_TYPE_SUBSCRIBE) {
                int32_t len;
                if (gSubscribeCount >= MAX_SUBSCRIBE_COUNT) {
                    APRINTF_E("subscribe count(=%d) over max count(=%d)\n", gSubscribeCount, MAX_SUBSCRIBE_COUNT);
                    if (item)
                        app_release_at_config(item);
                    continue;
                }
                len = strlen(item->name);
                if (len <= 0 || len >= MAX_SUBSCRIBE_STRING) {
                    APRINTF_E("subscribe name length invalid(=%d) (0 < len < %d)\n", len, MAX_SUBSCRIBE_STRING);
                    if (item) {
                        app_release_at_config(item);
                        continue;
                    }
                } else {
                    APRINTF("subscribe index=%d, name=%s\n", i, item->name);
                }
                memset(registeredSubsItems[gSubscribeCount], 0, sizeof(registeredSubsItems[0]));
                strncpy(registeredSubsItems[gSubscribeCount], item->name, len);
                gSubscribeCount++;
            }
        }
        if (item)
            app_release_at_config(item);
    }

    APRINTF_I("shadow item count = %d, (integer#=%d, string#=%d, float#=%d)\n", countItem, paramIntegerCnt, paramStringCnt,
        paramFloatCnt);

    if (countItem > 0) {
        gShadowCount = countItem;
        i = 0;
        shadowNode *currentShadow = shadowNodehead;

        while ((currentShadow != NULL) && (countItem > 0)) {
            APRINTF_I("current shadowConut = %d\n", countItem);
            if (currentShadow->shadowData.type == SHADOW_JSON_INT32)
                APRINTF_I("pkey=%s, pdata=%d\n", currentShadow->shadowData.pKey, *(int32_t* )currentShadow->shadowData.pData);
            else if (currentShadow->shadowData.type == SHADOW_JSON_STRING)
                APRINTF_I("pkey=%s, pdata=%s\n", currentShadow->shadowData.pKey, (char* )currentShadow->shadowData.pData);
            else if (currentShadow->shadowData.type == SHADOW_JSON_FLOAT)
                APRINTF_I("pkey=%s, pdata=%.6f\n", currentShadow->shadowData.pKey, *(float* )currentShadow->shadowData.pData);

            memcpy(&registeredShadow[i], &currentShadow->shadowData, sizeof(jsonStruct_t));
            currentShadow = currentShadow->next;
            countItem--;
            i++;
        }
    }

    return gShadowCount;
}

int32_t app_at_check_certi_info(void)
{
    int32_t ret = 1;
    char *temp_cert = NULL;
    int format = DA16X_CERT_FORMAT_NONE;
    size_t readLen = CERT_MAX_LENGTH;

    temp_cert = pvPortMalloc(CERT_MAX_LENGTH);
    if (!temp_cert) {
        APRINTF_E("%s:%d ---- malloc failed\n", __func__, __LINE__);
        return ret;
    }
    da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CA_CERT, &format, (unsigned char*)temp_cert, &readLen);
    APRINTF_I("\nRoot CA: %s\n", ((uint8_t )temp_cert[0] != 0xFF) ? "O" : "X");
    if ((uint8_t)temp_cert[0] == 0xFF)
        ret = 0;

#if defined(__SUPPORT_AWS_IOT__)
    if (!getUseFPstatus()) {
        readLen = CERT_MAX_LENGTH;
        da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_CERT, &format, (unsigned char*)temp_cert, &readLen);
        APRINTF_I("Certificate: %s\n", ((uint8_t )temp_cert[0] != 0xFF) ? "O" : "X");
        if ((uint8_t)temp_cert[0] == 0xFF)
            ret = 0;
        readLen = CERT_MAX_LENGTH;
        da16x_cert_read(DA16X_CERT_MODULE_MQTT, DA16X_CERT_TYPE_PRIVATE_KEY, &format, (unsigned char*)temp_cert, &readLen);
        APRINTF_I("Private Key: %s\n\n", ((uint8_t )temp_cert[0] != 0xFF) ? "O" : "X");
        if ((uint8_t)temp_cert[0] == 0xFF)
            ret = 0;
    }
#endif

    if (temp_cert != NULL)
        vPortFree(temp_cert);

    if (ret == 0) {
        APRINTF_E("Not ready Cert for APP service ...\n\n");
        return 0;
    }

    return ret;
}

int32_t app_at_feature_is_valid(void)
{
    char tempNVname[TEMP_NV_NAME_SIZE] = {0, };
    int readInt = 0;
    char *readStr = NULL;

    /* check certification key */
    if (app_at_check_certi_info() == 0)
        return 0;

#if defined(__SUPPORT_AWS_IOT__)
    if (!getUseFPstatus()) {
        /* check thing name */
        da16x_sprintf(tempNVname, "%s", APP_NVRAM_CONFIG_THINGNAME);
        readStr = app_get_string_nvram(tempNVname);
        if (readStr == NULL) {
            APRINTF_E("nvram read string(thingname) error\n");
            return 0;
        }
    }
    /* check broker url */
    da16x_sprintf(tempNVname, "%s", AWS_NVRAM_CONFIG_BROKER_URL);
    readStr = app_get_string_nvram(tempNVname);
    if (readStr == NULL) {
        APRINTF_E("nvram read string(broker) error\n");
        return 0;
    }
#elif defined(__SUPPORT_AZURE_IOT__)
    /* check thing name */
    da16x_sprintf(tempNVname, "%s", APP_NVRAM_CONFIG_THINGNAME);
    readStr = app_get_string_nvram(tempNVname);
    if (readStr == NULL) {
        APRINTF_E("nvram read string(thingname) error\n");
        return 0;
    }

    /* check primary key */
    da16x_sprintf(tempNVname, "%s", APP_NVRAM_CONFIG_DEV_PRIMARY_KEY);
    readStr = app_get_string_nvram(tempNVname);
    if (readStr == NULL) {
        APRINTF_E("nvram read string(primary key) error\n");
        return 0;
    }

    /* check thing name */
    da16x_sprintf(tempNVname, "%s", APP_NVRAM_CONFIG_HOST_NAME);
    readStr = app_get_string_nvram(tempNVname);
    if (readStr == NULL) {
        APRINTF_E("nvram read string(host name) error\n");
        return 0;
    }

    /* check primary key */
    da16x_sprintf(tempNVname, "%s", APP_NVRAM_CONFIG_IOTHUB_CONN_STRING);
    readStr = app_get_string_nvram(tempNVname);
    if (readStr == NULL) {
        APRINTF_E("nvram read string(connection string) error\n");
        return 0;
    }
#endif

    /* check shadow configuration */
    readInt = app_at_init_shadow_items();
    if (readInt == 0) {
        APRINTF_E("nvram read shadow config error\n");
        return 0;
    }
    return 1;
}

void atcmd_mcu_wakeup(uint8_t port, uint16_t pin)
{
    uint16_t data;

    atcmd_uart_ctrl = GPIO_CREATE(port);
    GPIO_INIT(atcmd_uart_ctrl);

    GPIO_IOCTL(atcmd_uart_ctrl, GPIO_SET_OUTPUT, &pin);

    data = pin;
    GPIO_WRITE(atcmd_uart_ctrl, pin, &data, sizeof(uint16_t));

    data = 0;
    GPIO_WRITE(atcmd_uart_ctrl, pin, &data, sizeof(uint16_t));

    GPIO_CLOSE(atcmd_uart_ctrl);
}

void check_current_heap(void)
{
    extern void heapInfo(void);
    heapInfo();
    da16x_printenv(ENVIRON_DEVICE, "app");
}

/**
 ****************************************************************************************
 * @brief Set Sleep mode and DTIM valuet 
 * @param[in] _CMD command string 
 * @param[in] _r status for command string
 * @return  text for transfer
 ****************************************************************************************
 */
void app_atcmd_set_DPM_value(GetValueType _type, int value)
{
    if (_type == TYPE_SLEEP_MODE) {
        APRINTF_I("Set Sleep mode : %d\n", value);
        if (write_nvram_int(SLEEP_MODE_FOR_NARAM, value) != 0) {
            APRINTF_E("\r\nSet Sleep mode Err\r\n");
        }
    } else if (_type == TYPE_USE_DPM) {
        APRINTF_I("Set to use DPM : %d\n", value);
        if (write_nvram_int(USE_DPM_FOR_NARAM, value) != 0)
            APRINTF_E("\r\nSet USE_DPM_FOR_NARAM Err\r\n");
    } else if (_type == TYPE_DPM_KEEP_ALIVE) {
        APRINTF_I("Set DPM Keep alive time : %d\n", value);
        if (write_nvram_int(NVR_KEY_DPM_KEEPALIVE_TIME, value) != 0)
            APRINTF_E("\r\nSet DPM Keep alive time Err\r\n");
    } else if (_type == TYPE_USER_WAKE_UP) {
        APRINTF_I("Set user wake-up time : %d\n", value);
        if (write_nvram_int(NVR_KEY_DPM_USER_WAKEUP_TIME, value) != 0)
            APRINTF_E("\r\nSet user wake-up time Err\r\n");
    } else if (_type == TYPE_TIM_WAKE_UP) {
        APRINTF_I("Set tim wake-up time : %d\n", value);
        if (write_nvram_int(NVR_KEY_DPM_TIM_WAKEUP_TIME, value) != 0)
            APRINTF_E("\r\nSet tim wake-up time Err\r\n");
    } else if (_type == TYPE_RTC_TIME) {
        APRINTF_I("Set RTC time : %d\n", value);
        if (write_nvram_int(SLEEP_MODE2_RTC_TIME, value) != 0)
            APRINTF_E("\r\nSet RTC time Err\r\n");
    } else {
        APRINTF_E("app_set_DPM_value error [Undfined type]\n");
    }
}

void app_atcmd_setSleepModeDtim(int _sleepMode, int _useDpmSet, int _rtcTime, int _dpmKeepAlive, int _userWakeup,
    int _timWakeup)
{
    if (_sleepMode >= 1 && _sleepMode <= 3) {
        app_atcmd_set_DPM_value(TYPE_SLEEP_MODE, _sleepMode);
        app_atcmd_set_DPM_value(TYPE_USE_DPM, _useDpmSet);
        app_atcmd_set_DPM_value(TYPE_RTC_TIME, _rtcTime);
        app_atcmd_set_DPM_value(TYPE_DPM_KEEP_ALIVE, _dpmKeepAlive);
        app_atcmd_set_DPM_value(TYPE_USER_WAKE_UP, _userWakeup);
        app_atcmd_set_DPM_value(TYPE_TIM_WAKE_UP, _timWakeup);
    } else {
        APRINTF_E("Unsupport Sleep mode error \n");
        return;
    }
}

static uint16_t fast_crc_nbit_lookup(const void *data, int length, uint16_t CrcLUT[4][16], uint16_t previousCrc16)
{
    uint16_t crc = swap16(previousCrc16);
    const uint16_t *current = (const uint16_t*)data;

    while (length > 1) {
        uint16_t one = *current++ ^ (crc);

        crc = CrcLUT[0][(one >> 0) & 0x0f] ^ CrcLUT[1][(one >> 4) & 0x0f] ^ CrcLUT[2][(one >> 8) & 0x0f]
            ^ CrcLUT[3][(one >> 12) & 0x0f];
        length -= 2;
    }

    if (length > 0) {
        uint16_t one = *current;

        one = ((one ^ crc) << 8);
        crc = crc >> 8;

        crc = crc ^ CrcLUT[0][(one >> 0) & 0x0f] ^ CrcLUT[1][(one >> 4) & 0x0f] ^ CrcLUT[2][(one >> 8) & 0x0f]
            ^ CrcLUT[3][(one >> 12) & 0x0f];
    }

    return swap16(crc);
}

static uint16_t swcrc16(uint8_t *data, uint16_t length, uint16_t prevCrc16)
{
    uint16_t crcdata;
    crcdata = fast_crc_nbit_lookup(data, length, (uint16_t (*)[16])RO_fast_crc_nbit_LUT, prevCrc16);
    return crcdata;
}

UINT app_writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size)
{
    UINT8 *imgbuffer;
    UINT32 imgsize;
    UINT32 malloc_size;
    uint32_t wait_cnt = 0;
    uint8_t wait_res = 0;

    static ota_mcu_fw_stream_info_t imginfo;
    malloc_size = size + OTA_MCU_FW_STREAM_HEADER_SIZE + 12;
    imgbuffer = (UINT8*)pvPortMalloc(malloc_size);
    if (imgbuffer != NULL) {
        da16x_sprintf((char*)imgbuffer, "\r\n+%sOTA=", PLATFORM);
        imgsize = strlen((char*)imgbuffer);
        memcpy(&imgbuffer[imgsize + OTA_MCU_FW_STREAM_HEADER_SIZE], srcMemAddr, size);
        imginfo.offset = offset;
        imginfo.content_length = tot_len;
        imginfo.received_length = r_len;
        imginfo.size = size;
        imginfo.crc = swcrc16(&imgbuffer[imgsize + OTA_MCU_FW_STREAM_HEADER_SIZE], size, 0);
        if (offset == 0)
            imginfo.imgcrc = 0;
        imginfo.imgcrc = swcrc16(&imgbuffer[imgsize + OTA_MCU_FW_STREAM_HEADER_SIZE], size, imginfo.imgcrc);
        memcpy(&imgbuffer[imgsize], &imginfo, OTA_MCU_FW_STREAM_HEADER_SIZE);
        imgsize += OTA_MCU_FW_STREAM_HEADER_SIZE + size;
        memcpy(&imgbuffer[imgsize], "\r\n", 2);
        imgsize += 2;
        waitIFReady();
        if ((mcu_wakeup_port >= 0) && (mcu_wakeup_pin > 0x00)) {
            atcmd_mcu_wakeup((uint8_t)mcu_wakeup_port, mcu_wakeup_pin);
            if (offset == 0)
                vTaskDelay(1);
        }

        wait_MCU_resp(3);
        PUTS_ATCMD((char*)imgbuffer, imgsize);

        while ((wait_res = get_MCU_resp()) == 3) {
            /* wait here */
            if (wait_cnt > 500) {
                APRINTF_E("\r\n [%s] break loop out(=%d) \r\n", __func__, wait_cnt);
                vPortFree(imgbuffer);
                return 0;
            }
            vTaskDelay(1);
            wait_cnt++;
        }

        if (wait_res == 99) {
            APRINTF_E("\r\n [%s] OTA Fail \r\n", __func__);
            vPortFree(imgbuffer);
            return 0;
        }

        vPortFree(imgbuffer);
    } else {
        return 0;
    }
    return size;
}
#endif  /* (__SUPPORT_ATCMD__) && ((__SUPPORT_AZURE_IOT__) || (__SUPPORT_AWS_IOT__)) */
