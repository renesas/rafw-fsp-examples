/***********************************************************************************************************************
 * File Name    : app_aws_atcmd_parse.c
 * Description  : AT command parser for AWS IoT
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

/* ${REA_DISCLAIMER_PLACEHOLDER} */
/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
#if CFG_WIFI
#if __has_include("rm_awsiot_w_cfg.h")
#include "rm_awsiot_w_cfg.h"
#endif

#ifdef __SUPPORT_AWS_IOT_W__
#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdlib.h>
#include "bsp_io.h"
#include "common_data.h"
#include "app_aws_atcmd_parse.h"
#include "app_dpm_thread.h"
#include "rm_atcmd_w_core_err_code.h"
#include "rm_atcmd_w_core.h"
#include "rm_atcmd_w_app.h"
#include "rm_atcmd_transport_uart_w.h"
/* MQTT library includes. */
#include "core_mqtt.h"
#include "app_aws_user_conf.h"
#include "app_dpm_interface.h"
#include "rm_map_persistant_w.h"
#include "ra6w1_platform_nvparam.h"
#include "rm_cert.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define MCU_PIN_PORT_LENGTH                           32
#define ATCMD_STATUS_SEND_LENGTH                      128

#define RM_ATCMD_W_CORE_AWS_ATCMD_CODE(atcmd)         "AT+" # atcmd
#define RM_ATCMD_W_CORE_AWS_ATCMD_CB(atcmd) \
    uint32_t RM_ATCMD_W_CORE_AWS_ ## atcmd ## _cmd_cb(atcmd_w_ctrl_t * const p_atcmd_w_ctrl, int argc, char *argv[])
#define RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(atcmd)  \
    const char *RM_ATCMD_W_CORE_AWS_ ## atcmd ## _format_cb(void)
#define RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(atcmd)   \
    const char *RM_ATCMD_W_CORE_AWS_ ## atcmd ## _brief_cb(void)
#define RM_ATCMD_W_CORE_AWS_UNFIXED_ATCMD_CB(atcmd) \
    uint32_t RM_ATCMD_W_CORE_AWS_ ## atcmd ## _cmd_cb(atcmd_w_ctrl_t * const p_atcmd_w_ctrl, uint8_t * p_in, size_t inlen)

#define RM_ATCMD_W_CORE_AWS_ATCMD_CB_P(atcmd)         RM_ATCMD_W_CORE_AWS_ ## atcmd ## _cmd_cb
#define RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB_P(atcmd)  RM_ATCMD_W_CORE_AWS_ ## atcmd ## _format_cb
#define RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB_P(atcmd)   RM_ATCMD_W_CORE_AWS_ ## atcmd ## _brief_cb
#define RM_ATCMD_W_CORE_AWS_DEBUG(fmt, ...)           //printf("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)
#define RM_ATCMD_W_CORE_AWS_ERROR(fmt, ...)           //printf("[%s:%d]" fmt, __func__, __LINE__, ##__VA_ARGS__)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
RM_ATCMD_W_CORE_AWS_ATCMD_CB(AWS);
RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(AWS);
RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(AWS);

RM_ATCMD_W_CORE_AWS_UNFIXED_ATCMD_CB(CERT);
RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(CERT);
RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(CERT);
/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
atcmd_w_core_module_t atcmd_w_core_aws_module[] =
{
    {
        RM_ATCMD_W_CORE_AWS_ATCMD_CODE(AWS),
        ATCMD_W_TYPE_A,
        5,
        0,
        RM_ATCMD_W_CORE_AWS_ATCMD_CB_P(AWS),
        RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB_P(AWS),
        RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB_P(AWS)
    },
    {
        NULL,
        ATCMD_W_TYPE_MAX,
        0,
        0,
        NULL,
        NULL,
        NULL
    },
};

atcmd_w_core_unfixed_module_t atcmd_w_core_aws_unfixed_module[] =
{
    {
        { AT_CMD_ESC_KEY_CHAR, 'C', 0x00, },
        2,
        RM_ATCMD_W_CORE_AWS_ATCMD_CB_P(CERT),
        RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB_P(CERT),
        RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB_P(CERT)
    },
    {
        "",
        0,
        NULL,
        NULL,
        NULL
    },
};

static ATCmdPublishEventCallback_t atcmd_publish_user_app_callback = NULL;
atcmd_w_ctrl_t * gp_pl_at_ctrl = NULL;
static int readyIF = 0;
atcmd_scan_result_t latestStatus = ATCMD_STATUS_IDLE;
uint8_t flagATOK = 1;
int8_t set_own = 0;
int32_t gSubscribeCount = 0;
st_atcmd_shadow_node_t *shadowNodehead = NULL;
char registeredSubsItems[MAX_SUBSCRIBE_COUNT][MAX_SUBSCRIBE_STRING] = {0, };
int32_t paramInteger[MAX_SHADOW_COUNT] = {0, };
char paramString[MAX_SHADOW_COUNT][MAX_SHADOW_STRING] = {0, };
float paramFloat[MAX_SHADOW_COUNT] = {0, };
int paramIntegerCnt = 0;
int paramStringCnt = 0;
int paramFloatCnt = 0;
int32_t gShadowCount = 0;
char cmdToMCUPayload[MAX_CMD_TO_MCU_PAYLOAD] = {0, };
st_atcmd_jsonStruct_t registeredShadow[MAX_SHADOW_COUNT] = {0, };
int8_t atcmd_mcu_wakeup_port = -1;
uint16_t atcmd_mcu_wakeup_pin = 0;
atcmd_provision_stat Prov_LastStat = ATCMD_PROVISION_IDLE;

static const char * awsThingAttributes[] = {
    AWSIOT_CFG_THING_ATTIRIBUTE_0,
    AWSIOT_CFG_THING_ATTIRIBUTE_1,
    AWSIOT_CFG_THING_ATTIRIBUTE_2,
    AWSIOT_CFG_THING_ATTIRIBUTE_3,
    AWSIOT_CFG_THING_ATTIRIBUTE_4,
    AWSIOT_CFG_THING_ATTIRIBUTE_5,
    AWSIOT_CFG_THING_ATTIRIBUTE_6,
    AWSIOT_CFG_THING_ATTIRIBUTE_7,
    AWSIOT_CFG_THING_ATTIRIBUTE_8,
    AWSIOT_CFG_THING_ATTIRIBUTE_9
};

#define swap16(x)   (((x) & 0xff) << 8) | (((x) & 0xff00) >> 8)
EventGroupHandle_t pl_ota_evt;
#define PL_OTA_STATUS_EVT_OK    (0x1 << 0)
#define PL_OTA_STATUS_EVT_NOK   (0x1 << 1)
static const uint16_t RO_fast_crc_nbit_LUT[4][16] = {
    {
        swap16(0x0000), swap16(0x3331), swap16(0x6662), swap16(0x5553),
        swap16(0xccc4), swap16(0xfff5), swap16(0xaaa6), swap16(0x9997),
        swap16(0x89a9), swap16(0xba98), swap16(0xefcb), swap16(0xdcfa),
        swap16(0x456d), swap16(0x765c), swap16(0x230f), swap16(0x103e)
    },
    {
        swap16(0x0000), swap16(0x0373), swap16(0x06e6), swap16(0x0595),
        swap16(0x0dcc), swap16(0x0ebf), swap16(0x0b2a), swap16(0x0859),
        swap16(0x1b98), swap16(0x18eb), swap16(0x1d7e), swap16(0x1e0d),
        swap16(0x1654), swap16(0x1527), swap16(0x10b2), swap16(0x13c1)
    },
    {
        swap16(0x0000), swap16(0x1021), swap16(0x2042), swap16(0x3063),
        swap16(0x4084), swap16(0x50a5), swap16(0x60c6), swap16(0x70e7),
        swap16(0x8108), swap16(0x9129), swap16(0xa14a), swap16(0xb16b),
        swap16(0xc18c), swap16(0xd1ad), swap16(0xe1ce), swap16(0xf1ef)
    },
    {
        swap16(0x0000), swap16(0x1231), swap16(0x2462), swap16(0x3653),
        swap16(0x48c4), swap16(0x5af5), swap16(0x6ca6), swap16(0x7e97),
        swap16(0x9188), swap16(0x83b9), swap16(0xb5ea), swap16(0xa7db),
        swap16(0xd94c), swap16(0xcb7d), swap16(0xfd2e), swap16(0xef1f)
    }
};

/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/
extern bool reset(int flag);
extern void app_set_mqtt_status(MQTTStatus_t rc);
extern MQTTStatus_t app_get_mqtt_status(void);

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
void register_atcmd_publish_user_app_callback(ATCmdPublishEventCallback_t callback)
{
    atcmd_publish_user_app_callback = callback;
}

static char *getStringRetain(const char *string_data)
{
    size_t strLen;
    char *strTemp;

    strLen = strlen(string_data);
    strTemp = (char *)pvPortMalloc(strLen + 1);
    strncpy(strTemp, string_data, strLen);
    strTemp[strLen] = 0;

    return strTemp;
}

void waitIFReady(void)
{
    while (readyIF == 0) {
        vTaskDelay(portCONVERT_MS_2_TICKS(100));
        printf(".");
    }
}

void wait_MCU_resp(uint8_t owner)
{
#if defined(__SUPPORT_ATCMD__)
    set_own = owner;
#else
    RA6W1_UNUSED_ARG(owner);
#endif  /* __SUPPORT_ATCMD__ */
}

uint8_t get_MCU_resp(void)
{
#if defined(__SUPPORT_ATCMD__)
    return set_own;
#else
    return 0;
#endif  /* __SUPPORT_ATCMD__ */
}

int checkconfigdataAtNVRAM(int num)
{
    char * nvramName = NULL;

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    awsThingAttributes[num], &nvramName);

    if (nvramName == NULL)
        return 0;
    else
        return 1;
}

void atcmd_provstat(int status)
{
    char tBuf[128] = {0, };

    Prov_LastStat = status;

    sprintf(tBuf,"+ATPROV=STATUS %d\n",status);
    RM_PL_PRINTF_ATCMD(tBuf);
    printf(tBuf);
}

void atcmd_mcu_wakeup(uint8_t port, uint16_t pin)
{
    bsp_io_port_pin_t l_pin = (((port << 8) & 0xff00) | (pin & 0xff));

    R_GPIO_W_PinCfg(&g_gpio_w_ctrl, l_pin
        , ((uint32_t) GPIO_W_PERIPHERAL_GPIO
        | (uint32_t) GPIO_W_CFG_PORT_DIRECTION_OUTPUT
        | (uint32_t) GPIO_W_CFG_PORT_OUTPUT_HIGH));
    vTaskDelay(1);
    R_GPIO_W_PinWrite(&g_gpio_w_ctrl, l_pin, BSP_IO_LEVEL_LOW);
}

static uint16_t fast_crc_nbit_lookup(const void *data, int length, uint16_t CrcLUT[4][16], uint16_t previousCrc16)
{
    uint16_t crc = swap16(previousCrc16);
    const uint16_t *current = (const uint16_t*)data;

    while (length > 1)
    {
        uint16_t one = *current++ ^ (crc);

        crc = CrcLUT[0][(one >> 0) & 0x0f] ^ CrcLUT[1][(one >> 4) & 0x0f] ^ CrcLUT[2][(one >> 8) & 0x0f]
            ^ CrcLUT[3][(one >> 12) & 0x0f];
        length -= 2;
    }

    if (length > 0)
    {
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
    unsigned int mask_evt;
    unsigned int evt_mask = PL_OTA_STATUS_EVT_OK | PL_OTA_STATUS_EVT_NOK;
    static ota_mcu_fw_stream_info_t imginfo;

    if (pl_ota_evt == NULL)
    {
        pl_ota_evt = xEventGroupCreate();
        if (pl_ota_evt == NULL)
        {
            printf("[%s] pl_ota_evt Event Flags Create Error!\n",__func__);

            return 0;
        }
    }
    malloc_size = size + OTA_MCU_FW_STREAM_HEADER_SIZE + 17;
    imgbuffer = (UINT8*)pvPortMalloc(malloc_size);
    if (imgbuffer != NULL)
    {
        sprintf((char*)imgbuffer, "\r\n+MATOTA=");
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

#if ATCMD_TRANSPORT_UART    //need to avoid crlf filter
        ((uart_transport_instance_ctrl_t *)((at_core_instance_ctrl_t *)gp_pl_at_ctrl)->p_transport_instance->p_ctrl)->addcrlf = 0;
        RM_ATCMD_W_CORE_Write(gp_pl_at_ctrl, (uint8_t *)imgbuffer, imgsize);
        ((uart_transport_instance_ctrl_t *)((at_core_instance_ctrl_t *)gp_pl_at_ctrl)->p_transport_instance->p_ctrl)->addcrlf = 1;
#else
        RM_ATCMD_W_CORE_Write(gp_pl_at_ctrl, (uint8_t *)imgbuffer, imgsize);
#endif
        mask_evt = xEventGroupWaitBits(pl_ota_evt, evt_mask, pdTRUE, pdFALSE, 1000);

        if (!(mask_evt & PL_OTA_STATUS_EVT_OK))
        {
            printf("[%s] OTA res fail 0x%x 0x%x\n", __func__, mask_evt, evt_mask);
            vPortFree(imgbuffer);

            return 0;
        }

        vPortFree(imgbuffer);
    }
    else
    {
        printf("[%s] Malloc fail!\n",__func__);
        return 0;
    }

    return size;
}

void app_iot_at_command_status(atcmd_scan_result_t _val)
{
    char tmp[ATCMD_STATUS_SEND_LENGTH] = {0, };
    char *tmp_read = NULL;
    char user[MCU_PIN_PORT_LENGTH] = { 0, };

    waitIFReady();

    latestStatus = _val;
    /* MCU wake up port and pin check */
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_MCU_WAKEUP_PORT, &tmp_read);
    if (tmp_read != NULL)
    {
        strncpy(user, tmp_read, strlen(tmp_read));
        atcmd_mcu_wakeup_port = (int8_t)app_atcmd_mcu_wakeup_port((char*)user);
    }

    memset(user, 0, MCU_PIN_PORT_LENGTH);
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG, AWSIOT_CFG_MCU_WAKEUP_PIN, &tmp_read);

    if (tmp_read != NULL)
    {
        strncpy(user, tmp_read, strlen(tmp_read));
        atcmd_mcu_wakeup_pin = app_atcmd_mcu_wakeup_pin((char*)user);
    }

    if ((atcmd_mcu_wakeup_port >= 0) && (atcmd_mcu_wakeup_pin > 0x00))
    {
        printf("\r\n>>latestStatus : %d\r\n", _val);
        atcmd_mcu_wakeup((uint8_t)atcmd_mcu_wakeup_port, atcmd_mcu_wakeup_pin);
        vTaskDelay(portCONVERT_MS_2_TICKS(50));
    }

    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "\r\n+%sIOT=status %d\r\n", PLATFORM, _val);
    RM_PL_PRINTF_ATCMD(tmp);
    vTaskDelay(portCONVERT_MS_2_TICKS(100));
}

void app_iot_at_IF_ready_notification(int _val)
{
    printf("[AT Interface ready notification]\n");
    readyIF = _val;
}

int app_iot_set_feature_parser(int argc, char *argv[])
{
    fsp_err_atcmd_err_code errorCode = FSP_ERR_AT_CMD_ERR_CMD_OK;

    RA6W1_UNUSED_ARG(argc);

    printf("APP SET %s \n", argv[2]);
    if (strcmp(argv[0], "help") == 0)
    {
    }
    else if (strcmp(argv[1], APP_CONFIG_THINGNAME) == 0)
    {
        /* thing name */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_THINGNAME, argv[2]);
    }
    else if (strcmp(argv[1], AWS_NVRAM_CONFIG_BROKER_URL) == 0)
    {
        /* Broker URL */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_BROKER_URL, argv[2]);
    }
    else if (strcmp(argv[1], NV_BOARD_FEATURE) == 0)
    {
        /* feature and pin mux */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_BOARD_FEATURE, argv[2]);
    }
    else if (strcmp(argv[1], APP_NVRAM_CONFIG_LPORT) == 0)
    {
        /* local port */
        RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                      AWSIOT_CFG_LPORT, atoi(argv[2]));
    }
    else if (strcmp(argv[1], APP_NVRAM_CONFIG_STOPIC) == 0)
    {
        /* subscribing topic */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_STOPIC, argv[2]);
    }
    else if (strcmp(argv[1], APP_NVRAM_CONFIG_PTOPIC) == 0)
    {
        /* publishing topic */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_PTOPIC, argv[2]);
    }
    else if (strcmp(argv[1], AT_DPM_SLEEP_MODE) == 0)
    {
        /* sleep mode */
        app_atcmd_set_DPM_value(ATCMD_TYPE_SLEEP_MODE, atoi(argv[2]));
    }
    else if (strcmp(argv[1], AT_DPM_USE_DPM) == 0)
    {
        /* DPM use */
        app_atcmd_set_DPM_value(ATCMD_TYPE_USE_DPM, atoi(argv[2]));
    }
    else if (strcmp(argv[1], AT_DPM_RTC_TIME) == 0)
    {
        /* keepalive wakeup time */
        app_atcmd_set_DPM_value(ATCMD_TYPE_RTC_TIME, atoi(argv[2]));
    }
    else if (strcmp(argv[1], AT_DPM_KEEP_ALIVE) == 0)
    {
        /* dpm keepalive time */
        app_atcmd_set_DPM_value(ATCMD_TYPE_DPM_KEEP_ALIVE, atoi(argv[2]));
    }
    else if (strcmp(argv[1], AT_DPM_USE_WAKE_UP) == 0)
    {
        /* user wakeup time */
        app_atcmd_set_DPM_value(ATCMD_TYPE_USER_WAKE_UP, atoi(argv[2]));
    }
    else if (strcmp(argv[1], AT_DPM_TIM_WAKE_UP) == 0)
    {
        /* TIM wakeup time */
        app_atcmd_set_DPM_value(ATCMD_TYPE_TIM_WAKE_UP, atoi(argv[2]));
    }
    else if (strcmp(argv[1], NV_MCU_WAKEUP_PORT) == 0)
    {
        /* mcu wake up gpio_unit_A or C */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_MCU_WAKEUP_PORT, argv[2]);
    }
    else if (strcmp(argv[1], NV_MCU_WAKEUP_PIN) == 0)
    {
        /* mcu wake up gpio_pin0~11 */
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         AWSIOT_CFG_MCU_WAKEUP_PIN, argv[2]);
    }
    else if (strcmp(argv[1], NV_MCU_UART_CFG) == 0)
    {
        /* UART1 baudrate: SET UART_CFG 115200 */
        printf("ERROR!! uart configuration\n");
        errorCode = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
    }
    else
    {
        printf("ERROR!! wrong name [%s]\n", argv[1]);
        errorCode = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
    }

    return errorCode;
}

int app_iot_config_thing_parser(int argc, char *argv[])
{
    char thing_att[NVEE_LEN_THING_ATTRIBUTE] = {0};
    fsp_err_atcmd_err_code errorCode = FSP_ERR_AT_CMD_ERR_CMD_OK;
    int attNum = 0;
    int recv_attnum = 0;

    if (argc < 5)
    {
        errorCode = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
        printf("Error! Need  more data. [Number][name][vlaue-type][mqtt-type]\n");

        return errorCode;
    }

    recv_attnum = atoi(argv[1]);
    if (recv_attnum < MAX_THING_ATT)
    {
        printf("=======================================================\n\n");
        printf("Att[%d] number   : %d\n", recv_attnum, recv_attnum);
        printf("Att[%d] name     : %s\n", recv_attnum, argv[2]);
        printf("Att[%d] data type: %d\n", recv_attnum, atoi(argv[3]));
        printf("Att[%d] MQTT type: %d\n", recv_attnum, atoi(argv[4]));
        printf("=======================================================\n\n");

        sprintf(thing_att, "%s %s %s %s", argv[1], argv[2], argv[3], argv[4]);
        attNum = atoi(argv[1]);
        RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                         awsThingAttributes[attNum], thing_att);
    }
    else
    {
        printf("Error! Maximum %d attirbutes are allowed.\n", MAX_THING_ATT);
    }

    return errorCode;
}

int app_iot_at_push_com_parser(int argc, char *argv[])
{
    char *tmp_read = NULL;
    char readit[32] = { 0, };
    char payload[TEMP_AT_DATA_SIZE] = { 0, };
    int i, j;
    int cmdNum;
    atComdataConfig *retData = NULL;
    fsp_err_atcmd_err_code errorCode = FSP_ERR_AT_CMD_ERR_CMD_OK;
    uint8_t mcu_res = 0; /* publish : 1, shadow = 2 */
#if defined ( __SUPPORT_AZURE_IOT__ )
    struct atcmd_twin_params *atParams;
#endif

    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_BOARD_FEATURE, &tmp_read);
    if (tmp_read != NULL)
    {
        strncpy(readit, tmp_read, strlen(tmp_read));
        printf("\nboard feature(%d) : %s\n", strlen(tmp_read), readit);
    }

    if (argc < 2)
    {
        errorCode = FSP_ERR_AT_CMD_ERR_INSUFFICIENT_ARGS;
        printf("Error! Need more data. [number]\n");
        return errorCode;
    }

    if (strcmp(argv[0], "help") == 0)
    {
    }
    else if (strcmp(argv[1], CMD_FATORY_RESET) == 0)
    {
        /* factroyreset */
        printf(" CMD_FATORY_RESET \n");
#if !defined (__BLE_COMBO_REF__)
#else
        factory_reset_default(1);
#endif
    }
    else if (strcmp(argv[1], CMD_RESET_TO_AP) == 0)
    {
        printf(" TO AP resetMode\n");
#if !defined (__BLE_COMBO_REF__)
        if (RM_PMGR_W_dpm_is_enabled())
        {
            RM_PMGR_W_dpm_disable();
            /* save provisioning flag for DPM mode */
            RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_DPM_AUTO, 1);
        }

#else
        factory_reset_default(1);
#endif
    }
    else if (strcmp(argv[1], CMD_GET_STATUS) == 0)
    {
        /* factroyreset */
        app_iot_at_command_status(latestStatus);
    }
    else if (strcmp(argv[1], CMD_RESTART) == 0)
    {
        /* reboot */
#if defined ( __SUPPORT_AWS_IOT_W__ )
        printf("AWS_IOT Reboot ... \n");
#elif defined ( __SUPPORT_AZURE_IOT__ )
        printf("AZURE_IOT Reboot ... \n");
#endif
        vTaskDelay(10);
        reset(0);
    }
    else if (strcmp(argv[1], CMD_MCU_MQTT) == 0)
    {
        if (argc < 5)
        {
            errorCode = FSP_ERR_AT_CMD_ERR_INSUFFICIENT_ARGS;
            printf("Error! Need more data. [number][name][value]\n");
            return errorCode;
        }

        /* CMD[0] MCU_DATA[1] 4[2] battery[3] 2700[4] */
        int topicCount = 0;

        for (i = 2; i < 100; i++)
        {
            if (argv[i] == NULL)
            {
                topicCount = topicCount / 3;
                printf("topicCount : %d\n", topicCount);
                break;
            }

            topicCount++;
        }

        for (i = 0; i < topicCount; i++)
        {
            cmdNum = atoi(argv[2 + (i * 3)]);
            retData = app_get_at_config(cmdNum);
            printf("\n\nCount : %d, cmdNum = %d\n", i, cmdNum);

            if (retData)
            {
                if (retData->filedNum == cmdNum)
                {
                    printf("mqtttype = %d\n", retData->mqtttype);
                    if (retData->mqtttype == CONFIG_MQTT_TYPE_SHADOW)
                    {
                        /* find same pkey */
                        for (j = 0; j < MAX_SHADOW_COUNT; j++)
                        {
                            if (strcmp(registeredShadow[j].pKey, retData->name) == 0)
                            {
                                printf("index(=%d) matched\n", j);
                                break;
                            }
                        }

                        if (j == MAX_SHADOW_COUNT)
                        {
                            printf("matched index not found\n");
                            continue;
                        }

                        printf("data type(shadow) = %d\n", retData->type);
                        mcu_res = 2;

                        if (retData->type == CONFIG_DATA_TYPE_INT)
                        {
                            printf("call update sensor(need to be set variable): %s = %d\n",
                                retData->name, atoi(argv[4 + (i * 3)]));
                            *(int*)registeredShadow[j].pData = atoi(argv[4 + (i * 3)]);
                        }
                        else if (retData->type == CONFIG_DATA_TYPE_STR)
                        {
                            printf("call update sensor(need to be set variable): %s = %s\n",
                                retData->name, argv[4 + (i * 3)]);
                            memcpy(registeredShadow[j].pData, argv[4 + (i * 3)],
                                strlen(argv[4 + (i * 3)]) + 1);
                        }
                        else if (retData->type == CONFIG_DATA_TYPE_FLOAT)
                        {
                            *(float*)registeredShadow[j].pData = (float)atof(argv[4 + (i * 3)]);
                            printf("call update sensor(need to be set variable): %s = %d.%06d\n",
                                retData->name, ((int)*(float *)registeredShadow[j].pData)
                                , (int)((float)((float)*(float *)registeredShadow[j].pData - (int)*(float *)registeredShadow[j].pData) * 1000) % 1000);
                        }
                        else
                        {
                            printf("%s's type(=%d) invalid\n", retData->name, retData->type);
                            errorCode = FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
                            goto QUIT;
                        }
                    }
                    else if (retData->mqtttype == CONFIG_MQTT_TYPE_PUBLISH)
                    {
                        /* support only 1 item */
                        printf("data type(publish)=%d\n", retData->type);
                        mcu_res = 1;
                        if (retData->type == CONFIG_DATA_TYPE_INT)
                        {
                            sprintf(payload, "%s %s %d", argv[2], argv[3], atoi(argv[4]));
                            printf("call publish: %s\n", payload);
                        }
                        else if (retData->type == CONFIG_DATA_TYPE_STR)
                        {
                            sprintf(payload, "%s %s %s", argv[2], argv[3], argv[4]);
                            printf("call publish: %s\n", payload);
                        }
                        else if (retData->type == CONFIG_DATA_TYPE_FLOAT)
                        {
                            sprintf(payload, "%s %s %.6f", argv[2], argv[3], atof(argv[4]));
                            printf("call publish: %s\n", payload);
                        }
                        else
                        {
                            printf("%s's type(=%d) invalid\n", retData->name, retData->type);
                            errorCode = FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
                            goto QUIT;
                        }
#if defined ( __SUPPORT_AWS_IOT_W__ )
                        if (atcmd_publish_user_app_callback != NULL)
                            atcmd_publish_user_app_callback(payload);
#elif defined ( __SUPPORT_AZURE_IOT__ )
                        atParams = app_at_get_twins();
                        if (strcmp(argv[2], "1") == 0)
                        {
                            if (strcmp(argv[3], "mcu_door") == 0)
                            {
                                if (strcmp(argv[4], "opened") == 0)
                                {
                                    atParams->doorState = 1;
                                    atParams->doorlock_state = 1; /* Door_Open */
                                    atParams->doorStateChange = 1;
                                    printf("call mcu_door opened set\n");
                                }
                                else if (strcmp(argv[4], "closed") == 0)
                                {
                                    atParams->doorState = 0;
                                    atParams->doorlock_state = 2; /* Door_Close */
                                    atParams->doorStateChange = 1;
                                    printf("call mcu_door closed set\n");
                                }
                            }
                        }
                        app_at_set_twins(atParams);
                        if (app_get_control_flag())
                            app_at_set_resp_string(payload, strlen(payload));
#endif
                    }
                    else
                    {
                        printf("%s's mqtttype(=%d) invalid\n", retData->name, retData->mqtttype);
                        errorCode = FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
                        goto QUIT;
                    }
                }
                else
                {
                    printf("field number differet from passed numer\n");
                    errorCode = FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
                    goto QUIT;
                }
            }
            else
            {
                printf("no config data in nvram\n");
                errorCode = FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
                goto QUIT;
            }
QUIT:
            if (retData != NULL)
            {
                app_release_at_config(retData);
            }
        }
    }

    if (get_MCU_resp() == mcu_res)
    {
        wait_MCU_resp(0);
        printf("\r\nrelease response\r\n\n");
    }

    return errorCode;
}

int app_iot_at_response_ok_parser(int argc, char *argv[])
{
    fsp_err_atcmd_err_code errorCode = FSP_ERR_AT_CMD_ERR_CMD_OK;

    if (argc < 1)
    {
        errorCode = FSP_ERR_AT_CMD_ERR_INSUFFICIENT_ARGS;
        printf("Error! Need more data\n");
        return errorCode;
    }

    if ((strncmp(argv[0], "OK_OTA", 5) == 0))
    {
        if ((strncmp(argv[0], "OK_OTA_FAIL", 10) == 0))
        {
            if (get_MCU_resp() == 3)
            {
                wait_MCU_resp(99);
            }
        }
        else
        {
            if (get_MCU_resp() == 3)
            {
                wait_MCU_resp(0);
            }
        }
    }
    else if ((strncmp(argv[0], "OK", 2) == 0))
    {
        /* check payload length */
        flagATOK = 1;
    }

    return errorCode;
}
atComdataConfig * app_get_at_config(int num)
{
    static atComdataConfig * retData = NULL;
    char att_read[NVEE_LEN_THING_ATTRIBUTE] = {0};
    char * nvram_read = NULL;
    int32_t filedNumber = 0;
    int32_t filedtype = 0;
    int32_t filedmqtttype = 0;
    char * filedname = NULL;

    if (checkconfigdataAtNVRAM(num) == 0)
    {
        return NULL;
    }
    retData = (atComdataConfig *) pvPortMalloc(sizeof(atComdataConfig));

    if (retData)
    {
            RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                            awsThingAttributes[num], &nvram_read);
            if (nvram_read != NULL)
            {
                strncpy(att_read, nvram_read, NVEE_LEN_THING_ATTRIBUTE - 1);
                att_read[NVEE_LEN_THING_ATTRIBUTE - 1] = '\0';  // Ensure null-termination
            }
            else
            {
                goto INVALID_VAL;
            }

            if (att_read[0] != '\0')
            {
                char * token = strtok(att_read, " ");
                int tokenCount = 0;

                while (token != NULL)
                {
                    switch (tokenCount)
                    {
                        case 0:
                            filedNumber = atoi(token);
                            retData->filedNum = filedNumber;
                        break;
                        case 1:
                            filedname = token;
                            retData->name = getStringRetain(filedname);
                        break;
                        case 2:
                            filedtype = atoi(token);
                            retData->type = filedtype;
                        break;
                        case 3:
                            filedmqtttype = atoi(token);
                            retData->mqtttype = filedmqtttype;
                        break;
                        default:
                        break;
                    }
                    token = strtok(NULL, " ");
                    tokenCount++;
                }
            }
    }
    else
    {
        printf("%s> malloc failed\n", __func__);
    }

    return retData;
INVALID_VAL:
    if (retData != NULL)
        vPortFree(retData);

    return NULL;
}

void app_release_at_config(atComdataConfig *_config)
{
    if (_config->name != NULL)
    {
        vPortFree(_config->name);
        _config->name = NULL;
    }

    if (_config != NULL)
    {
        vPortFree(_config);
        _config = NULL;
    }
}

void app_atcmd_set_DPM_value(atcmd_dpm_value_type_t _type, int value)
{
    if (_type == ATCMD_TYPE_SLEEP_MODE)
    {
        printf("Set Sleep mode : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_DPM_SLEEP_MODE, value) != 0)
        {
            printf("\r\nSet Sleep mode Err\r\n");
        }
    }
    else if (_type == ATCMD_TYPE_USE_DPM)
    {
        printf("Set to use DPM : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_USE_DPM, value) != 0)
            printf("\r\nSet USE_DPM_FOR_NVRAM Err\r\n");
    }
    else if (_type == ATCMD_TYPE_DPM_KEEP_ALIVE)
    {
        printf("Set DPM Keep alive time : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_DPM_KEEP_ALIVE_TIME, value) != 0)
            printf("\r\nSet DPM Keep alive time Err\r\n");
    }
    else if (_type == ATCMD_TYPE_USER_WAKE_UP)
    {
        printf("Set user wake-up time : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_USER_WAKE_UP_TIME, value) != 0)
            printf("\r\nSet user wake-up time Err\r\n");
    }
    else if (_type == ATCMD_TYPE_TIM_WAKE_UP)
    {
        printf("Set tim wake-up time : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          NVR_KEY_DPM_TIM_WAKEUP_TIME, value) != 0)
            printf("\r\nSet tim wake-up time Err\r\n");
    }
    else if (_type == ATCMD_TYPE_RTC_TIME)
    {
        printf("Set RTC time : %d\n", value);
        if (RM_MAP_PERSISTANT_W_Write_INT(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                          AWSIOT_CFG_RTC_TIME, value) != 0)
            printf("\r\nSet RTC time Err\r\n");
    }
    else
    {
        printf("app_set_DPM_value error [Undfined type]\n");
    }
}

uint8_t app_atcmd_mcu_wakeup_port(char *port_num)
{
    uint8_t gpio_port = BSP_IO_PORT_00;

    if (strcmp(port_num, "GPIO_PORT_0") == 0)
        gpio_port = BSP_IO_PORT_00;
    else if (strcmp(port_num, "GPIO_PORT_1") == 0)
        gpio_port = BSP_IO_PORT_01;
    else if (strcmp(port_num, "GPIO_PORT_2") == 0)
        gpio_port = BSP_IO_PORT_02;

    atcmd_mcu_wakeup_port = gpio_port;

    return gpio_port;
}

uint16_t app_atcmd_mcu_wakeup_pin(char *pin_num)
{
    uint16_t gpio_pin = 0;
    char ch = pin_num[8];

    if ((ch >= 0x30) && (ch < 0x39))
    {
        gpio_pin = ch - 0x30;
        ch = pin_num[9];
        if ((ch >= 0x30) && (ch < 0x39))
        {
            gpio_pin = gpio_pin * 10;
            gpio_pin += ch - 0x30;
        }
    }
    else
    {
        printf("mcu_wakeup_pin err %s", pin_num);
    }

    atcmd_mcu_wakeup_pin = gpio_pin;
    return gpio_pin;
}

int32_t app_at_feature_is_valid(void)
{
    int readInt = 0;
    char *readStr = NULL;

    /* check certification key */
    if (app_at_check_certi_info() == 0)
        return 0;

    /* check thing name */
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_THINGNAME, &readStr);
    if (readStr == NULL)
    {
        printf("nvram read string(thingname) error\n");

        return 0;
    }
    /* check broker url */
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_APPCFG,
                                    AWSIOT_CFG_BROKER_URL, &readStr);
    if (readStr == NULL)
    {
        printf("nvram read string(broker) error\n");

        return 0;
    }
    /* check shadow configuration */
    readInt = app_at_init_shadow_items();
    if (readInt == 0)
    {
        printf("nvram read shadow config error\n");

        return 0;
    }

    return 1;
}

int32_t app_at_check_certi_info(void)
{
    int32_t ret = 1;

    if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_CA_CERT))
    {
        printf("\nRoot CA: O");
    }
    else
    {
        printf("\nRoot CA: X");
        ret = 0;
    }

    if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_INITIAL_CERT))
    {
        printf("\nCertificate: O");
    }
    else
    {
        printf("\nCertificate: X");
        ret = 0;
    }

    if (RM_CERT_IsExistCert(RM_CERT_MODULE_AWS, RM_CERT_TYPE_INITIAL_PRIV_KEY))
    {
        printf("\nPrivate Key: O");
    }
    else
    {
        printf("\nPrivate Key: X");
        ret = 0;
    }

    if (ret == 0)
    {
        printf("Not ready Cert for APP service ...\n\n");

        return 0;
    }

    return ret;
}

uint32_t app_at_init_shadow_items(void)
{
    int i = 0;
    int countItem = 0;
    atComdataConfig *item = NULL;
    int32_t len;
    st_atcmd_shadow_node_t * currentShadow;
    st_atcmd_shadow_node_t * newNode;

    gSubscribeCount = 0;

    for (i = 0; i < MAX_THING_ATT; i++)
    {
        item = app_get_at_config(i);
        if (item != NULL && item->filedNum == i)
        {
            if (item->mqtttype == CONFIG_MQTT_TYPE_SHADOW)
            {
                newNode = pvPortMalloc(sizeof(st_atcmd_shadow_node_t));

                if (countItem >= MAX_SHADOW_COUNT)
                {
                    printf("shadow count(=%d) over max count(=%d)\n", countItem, MAX_SHADOW_COUNT);
                    if (newNode != NULL)
                        vPortFree(newNode);
                    if (item)
                        app_release_at_config(item);
                    continue;
                }

                if (newNode)
                {
                    newNode->shadowData.cb = NULL;
                    printf("newNode index=%d\n", i);
                    if (item->type == CONFIG_DATA_TYPE_INT)
                    {
                        newNode->shadowData.type = SHADOW_JSON_INT32;
                        newNode->shadowData.pData = &paramInteger[paramIntegerCnt];
                        newNode->shadowData.dataLength = sizeof(int32_t);
                        *(int*)newNode->shadowData.pData = 2700;
                        paramIntegerCnt++;
                    }
                    else if (item->type == CONFIG_DATA_TYPE_STR)
                    {
                        newNode->shadowData.type = SHADOW_JSON_STRING;
                        newNode->shadowData.pData = &paramString[paramStringCnt];
                        newNode->shadowData.dataLength = sizeof(paramString[0]);
                        memset((char*)newNode->shadowData.pData, 0, sizeof(paramString[0]));
                        sprintf((char*)newNode->shadowData.pData, "%s", "test");
                        paramStringCnt++;
                    }
                    else if (item->type == CONFIG_DATA_TYPE_FLOAT)
                    {
                        newNode->shadowData.type = SHADOW_JSON_FLOAT;
                        newNode->shadowData.pData = &paramFloat[paramFloatCnt];
                        newNode->shadowData.dataLength = sizeof(float);
                        paramFloat[paramFloatCnt] = 16.5;
                        paramFloatCnt++;
                    }
                    else
                    {
                        printf("newNode type invalid\n");
                        if (newNode != NULL)
                            vPortFree(newNode);
                        if (item)
                            app_release_at_config(item);
                        break;
                    }
                    newNode->shadowData.pKey = getStringRetain(item->name);
                    if (shadowNodehead == NULL)
                    {
                        shadowNodehead = newNode;
                    }
                    else
                    {
                        newNode->next = shadowNodehead;
                        shadowNodehead = newNode;
                    }
                    countItem++;
                }
                else
                {
                    printf("newNode malloc failed\n");
                    if (item)
                        app_release_at_config(item);
                    break;
                }
            }
            else if (item->mqtttype == CONFIG_MQTT_TYPE_SUBSCRIBE)
            {
                if (gSubscribeCount >= MAX_SUBSCRIBE_COUNT)
                {
                    printf("subscribe count(=%ld) over max count(=%d)\n", gSubscribeCount, MAX_SUBSCRIBE_COUNT);
                    if (item)
                        app_release_at_config(item);
                    continue;
                }
                len = strlen(item->name);
                if (len <= 0 || len >= MAX_SUBSCRIBE_STRING)
                {
                    printf("subscribe name length invalid(=%ld) (0 < len < %d)\n", len, MAX_SUBSCRIBE_STRING);
                    if (item)
                    {
                        app_release_at_config(item);
                        continue;
                    }
                }
                else
                {
                    printf("subscribe index=%d, name=%s\n", i, item->name);
                }
                memset(registeredSubsItems[gSubscribeCount], 0, sizeof(registeredSubsItems[0]));
                strncpy(registeredSubsItems[gSubscribeCount], item->name, len);
                gSubscribeCount++;
            }
        }
        if (item)
            app_release_at_config(item);
    }

    printf("shadow item count = %d, (integer#=%d, string#=%d, float#=%d)\n", countItem, paramIntegerCnt, paramStringCnt,
           paramFloatCnt);

    if (countItem > 0)
    {
        gShadowCount = countItem;
        i = 0;
        currentShadow = shadowNodehead;

        while ((currentShadow != NULL) && (countItem > 0))
        {
            printf("current shadowCount = %d\n", countItem);
            if (currentShadow->shadowData.type == SHADOW_JSON_INT32)
                printf("pkey=%s, pdata=%ld\n", currentShadow->shadowData.pKey, *(int32_t *)currentShadow->shadowData.pData);
            else if (currentShadow->shadowData.type == SHADOW_JSON_STRING)
                printf("pkey=%s, pdata=%s\n", currentShadow->shadowData.pKey, (char *)currentShadow->shadowData.pData);
            else if (currentShadow->shadowData.type == SHADOW_JSON_FLOAT)
                printf("pkey=%s, pdata=%d.%06d\n", currentShadow->shadowData.pKey,
                       ((int)*(float *)currentShadow->shadowData.pData),
                       (int)((float)((float)*(float *)currentShadow->shadowData.pData - (int)*(float *)currentShadow->shadowData.pData) * 1000) % 1000);

            memcpy(&registeredShadow[i], &currentShadow->shadowData, sizeof(st_atcmd_jsonStruct_t));
            currentShadow = currentShadow->next;
            countItem--;
            i++;
        }
    }

    return gShadowCount;
}

int32_t app_at_get_registered_subscribe_count(void)
{
    return gSubscribeCount;
}

char* app_at_get_cloud_cmd_payload(int32_t _index)
{
    char *foundStr = NULL;

    if (_index < MAX_SUBSCRIBE_COUNT)
    {
        if (strlen(registeredSubsItems[_index]) > 0)
        {
            foundStr = strstr(cmdToMCUPayload, registeredSubsItems[_index]);
            if (foundStr)
                return cmdToMCUPayload;
            else
                return NULL;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
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

void app_at_set_cloud_cmd_payload(char *_payload, int32_t _needLen)
{
    if (_payload && _needLen < MAX_CMD_TO_MCU_PAYLOAD && strlen(_payload) >= (unsigned int)_needLen)
    {
        memset(cmdToMCUPayload, 0, sizeof(cmdToMCUPayload));
        strncpy(cmdToMCUPayload, _payload, _needLen);
    }
}

/***********************************************************************************************************************
 * AT Functions
 **********************************************************************************************************************/
uint32_t RM_ATCMD_W_CORE_AWS_register(atcmd_w_core_module_list_t * p_list)
{
#if (AT_CORE_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
#endif

    if (p_list->module_cnt > ATCMD_W_LIST_MAX_CNT)
    {
        return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
    }

    if (p_list->unfixed_module_cnt > ATCMD_W_LIST_MAX_CNT)
    {
        return FSP_ERR_AT_CMD_ERR_NOT_SUPPORTED;
    }

    if (rm_atcmd_w_core_register_module_node(p_list, atcmd_w_core_aws_module) == FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    if (rm_atcmd_w_core_register_unfixed_module_node(p_list, atcmd_w_core_aws_unfixed_module) == FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
    {
        return FSP_ERR_AT_CMD_ERR_MEM_ALLOC;
    }

    app_iot_at_IF_ready_notification(1);

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_AWS_deregister(atcmd_w_core_module_list_t * p_list)
{
#if (AT_CORE_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
#endif

    rm_atcmd_w_core_deregister(p_list, atcmd_w_core_aws_module);
    rm_atcmd_w_core_unfixed_deregister(p_list, atcmd_w_core_aws_unfixed_module);

    return FSP_ERR_AT_CMD_ERR_CMD_OK;
}

uint32_t RM_ATCMD_W_CORE_AWS_open(atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    gp_pl_at_ctrl = p_atcmd_w_ctrl;
    return err;
}

uint32_t RM_ATCMD_W_CORE_AWS_close(atcmd_w_ctrl_t * const p_atcmd_w_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;

    RA6W1_UNUSED_ARG(p_atcmd_w_ctrl);
    gp_pl_at_ctrl = NULL;

    return err;
}

void RM_PL_PRINTF_ATCMD(char *p_str)
{
    if (gp_pl_at_ctrl == NULL)
    {
        printf("gp_pl_at_ctrl Failed in RM_PL_PRINTF_ATCMD\n");

        return;
    }
    RM_ATCMD_W_CORE_Write(gp_pl_at_ctrl, (uint8_t *)p_str, strlen(p_str));

    return;
}
RM_ATCMD_W_CORE_AWS_ATCMD_CB(AWS)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    int i;
    int k = 0;
    char *params[MAX_ARGV_NUM] = { 0, };

    RA6W1_UNUSED_ARG(p_atcmd_w_ctrl);

    printf("\n======================================================= \n");
    printf("argc num = %d \n", argc);
    for (i = 0; i < MAX_ARGV_NUM; i++)
    {
        if (argv[i] == NULL)
            break;

        printf("argv[%d]: %s\n", i, argv[i]);
    }
    printf("\n======================================================= \n");

    if (argc > 2)
    {
        k = 1;
        for (int j = 0; j < i - 1; j++)
        {
            params[j] = argv[k];
            k++;
        }
    }
    else
    {
        params[k] = (char *)strtok(argv[1], " ");

        while (params[k] != NULL)
        {
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
        printf("[APP-IOT] Not support or wrong commmand \n");

    return err;
}

RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(AWS)
{
    const char * p_usage = "<parameter>,[<size>],<value>";
    return p_usage;
}

RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(AWS)
{
    const char * p_descrption = "Set AWS parameters";
    return p_descrption;
}

RM_ATCMD_W_CORE_AWS_UNFIXED_ATCMD_CB(CERT)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK;
    fsp_err_t fsp_err = FSP_SUCCESS;
    const int max_atcmd_len = CERT_MAX_LENGTH;
    int cdata_idx = 0;
    char ch = 0;
    int cert_id = 0;
    unsigned char * p_cert = NULL;
    char resp_str[32] = {0x00, };

    RA6W1_UNUSED_ARG(p_in);
    RA6W1_UNUSED_ARG(inlen);

    /* [ESC, 0x1b]C[Index],<content>[EXT, 0x03]*/
    /* Input index */
    fsp_err = RM_ATCMD_W_CORE_Read(p_atcmd_w_ctrl, (uint8_t *)&ch, sizeof(char));
    if ((ch < 0x30) || (ch > 0x39))
    {
        err = FSP_ERR_AT_CMD_ERR_INSUFFICIENT_ARGS;
        goto end;
    }
    cert_id = ch - 0x30;

    /* Input comma(,) */
    fsp_err = RM_ATCMD_W_CORE_Read(p_atcmd_w_ctrl, (uint8_t *)&ch, sizeof(char));
    if ((fsp_err != FSP_SUCCESS) || (ch != ','))
    {
        err = FSP_ERR_AT_CMD_ERR_INSUFFICIENT_ARGS;
        goto end;
    }

    p_cert = pvPortMalloc(CERT_MAX_LENGTH);
    if (!p_cert)
    {
        printf("%s:%d ---- malloc failed\n", __func__, __LINE__);
        goto end;
    }

    while (1)
    {
        fsp_err = RM_ATCMD_W_CORE_Read(p_atcmd_w_ctrl, (uint8_t *)&ch, sizeof(char));

        if (fsp_err == FSP_SUCCESS)
        {
            if (cdata_idx >= (max_atcmd_len - 1))
            {
                cdata_idx = max_atcmd_len - 2;
            }

            if (ch == '\b')          /* backspace */
            {
                cdata_idx--;
                p_cert[cdata_idx] = '\0';
            }
            else if (ch == 0x0d)
            {
                p_cert[cdata_idx++] = 0x0a;
            }
            else if (ch == 0x03 || ch == 0x1a)
            {
                p_cert[cdata_idx++] = '\0';
                break;
            }
            else
            {
                p_cert[cdata_idx++] = ch;
            }
        }
        else
        {
            err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
            break;
        }
    }

    if (err == FSP_ERR_AT_CMD_ERR_CMD_OK)
    {
        /* Store certificate */
        switch (cert_id)
        {
            case 0:
                /* CA */
                if (RM_CERT_Write(RM_CERT_GetModule(SF_TLS_CERT_AWS_CA_ADDR),
                                  RM_CERT_TYPE_CA_CERT, RM_CERT_FORMAT_PEM, p_cert, cdata_idx))
                {
                    err = FSP_ERR_AT_CMD_ERR_SFLASH_WRITE;
                }
            break;
            case 1:
                /* Certificate */
                if (RM_CERT_Write(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_CERT_ADDR),
                                  RM_CERT_TYPE_INITIAL_CERT, RM_CERT_FORMAT_PEM, p_cert, cdata_idx))
                {
                    err = FSP_ERR_AT_CMD_ERR_SFLASH_WRITE;
                }
            break;
            case 2:
                /* Key */
                if (RM_CERT_Write(RM_CERT_GetModule(SF_TLS_CERT_AWS_INITIAL_PRIV_KEY_ADDR),
                                  RM_CERT_TYPE_INITIAL_PRIV_KEY, RM_CERT_FORMAT_PEM, p_cert, cdata_idx))
                {
                    err = FSP_ERR_AT_CMD_ERR_SFLASH_WRITE;
                }
            break;
            default:
                err = FSP_ERR_AT_CMD_ERR_WRONG_ARGUMENTS;
            break;
        }
    }

end:

    if (err == FSP_ERR_AT_CMD_ERR_CMD_OK)
    {
        ATCMD_ESC_OK_STR(resp_str);
    }
    else
    {
        ATCMD_ESC_ERROR_STR(resp_str, err);
    }

    RM_ATCMD_W_CORE_Write(p_atcmd_w_ctrl, (uint8_t *)resp_str, strlen(resp_str));

    if (p_cert)
    {
        vPortFree(p_cert);
        p_cert = NULL;
    }

    return err;
}

RM_ATCMD_W_CORE_AWS_ATCMD_FORMAT_CB(CERT)
{
    const char * p_usage = "";

    return p_usage;
}

RM_ATCMD_W_CORE_AWS_ATCMD_BRIEF_CB(CERT)
{
    const char * p_description = "";

    return p_description;
}

void add_aws_atcmd(void)
{
    set_user_app_atcmd_open_callback(RM_ATCMD_W_CORE_AWS_open);
    rm_atcmd_w_core_user_command_register(atcmd_w_core_aws_module);
    rm_atcmd_w_core_user_command_unfixed_register(atcmd_w_core_aws_unfixed_module);
    set_user_app_atcmd_close_callback(RM_ATCMD_W_CORE_AWS_close);
    app_iot_at_IF_ready_notification(1);
}

#endif /* __SUPPORT_AWS_IOT_W__ */
#endif /* CFG_WIFI */
