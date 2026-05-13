/**
 ****************************************************************************************
 *
 * @file app_atcommand.h
 *
 * @brief The structure information of Apps at command
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

#if !defined(_APP_ATCOMMAND_H_)
#define _APP_ATCOMMAND_H_

#include "app_provision.h"
#include "app_atcommand_pal.h"

#define RETRY_ACK_CHECK             0
#define RETRY_ACK_CHECK_SKIP        1

/* board feature */
#define FEATURE_EVK                 "EVK"
#define FEATURE_DOORLOCK            "DOORLOCK_REF"
#define FEATURE_SENSOR              "SENSOR_REF"
#define FEATURE_SENSOR_DOORLOCK     "SENSOR_DOORLOCK"
#define FEATURE_USER                "USER"
/* board feature set : EVK, SENSOR_REF, DOORLOCK_REF, SENSOR_DOORLOCK, USER_OWN */
#define NV_BOARD_FEATURE            "APP_BOARD_FEATURE"
/* pin mux */
#define NV_PIN_AMUX                 "NV_PIN_AMUX"
#define NV_PIN_BMUX                 "NV_PIN_BMUX"
#define NV_PIN_CMUX                 "NV_PIN_CMUX"
#define NV_PIN_DMUX                 "NV_PIN_DMUX"
#define NV_PIN_EMUX                 "NV_PIN_EMUX"
#define NV_PIN_FMUX                 "NV_PIN_FMUX"
#define NV_PIN_HMUX                 "NV_PIN_HMUX"
#define NV_PIN_UMUX                 "NV_PIN_UMUX"

#define NV_MCU_WAKEUP_PORT          "APP_MCU_WKAEUP_PORT"
#define NV_MCU_WAKEUP_PIN           "APP_MCU_WKAEUP_PIN"
#define NV_MCU_UART_CFG             "UART_CFG"

#define CONFIG_MQTT_TYPE_PUBLISH    0
#define CONFIG_MQTT_TYPE_SHADOW     1
#define CONFIG_MQTT_TYPE_SUBSCRIBE  2
#define CONFIG_DATA_TYPE_INT        0
#define CONFIG_DATA_TYPE_STR        1
#define CONFIG_DATA_TYPE_FLOAT      2

#define CMD_FATORY_RESET            "FACTORY_RESET"
#define CMD_RESET_TO_AP             "RESET_TO_AP"
#define CMD_START_SOFTAP            "SOFTAP_START"
#define CMD_START_STATION           "STA_START"
#define CMD_GET_STATUS              "GET_STATUS"
#define CMD_RESTART                 "RESTART"
#define CMD_MCU_MQTT                "MCU_DATA"
#define CMD_SERVER_DATA             "SERVER_DATA"
#define CMD_TO_MCU                  "CMD_TO_MCU"
#define CMD_RESPONSE_OK_STR         "\r\n+"PLATFORM"IOT OK\r\n"
#define NV_CONFIG_THING_ATTIRIBUTE  "T_att"

#define TEMP_NV_NAME_SIZE   256
#define MAX_ARGV_NUM        50
#define TEMP_AT_DATA_SIZE   128

/* DPM parameter string from MCU set */

#define AT_DPM_SLEEP_MODE                   "SLEEP_MODE"
#define AT_DPM_USE_DPM                      "USE_DPM"
#define AT_DPM_RTC_TIME                     "RTC_TIME"
#define AT_DPM_KEEP_ALIVE                   "DPM_KEEP_ALIVE"
#define AT_DPM_USE_WAKE_UP                  "USE_WAKE_UP"
#define AT_DPM_TIM_WAKE_UP                  "TIM_WAKE_UP"

/* Command */
#define MAX_PKEY_LEN            20
#define MAX_SHADOW_COUNT        10
#define MAX_SHADOW_STRING       (MAX_PKEY_LEN - 1)
#define MAX_SUBSCRIBE_COUNT     10
#define MAX_SUBSCRIBE_STRING    (MAX_PKEY_LEN - 1)
#define MAX_CMD_TO_MCU_PAYLOAD  128

#define MAX_RESEND_SLEEP_CNT    10
#define swap16(x)   (((x) & 0xff) << 8) | (((x) & 0xff00) >> 8)

typedef struct _atComdataConfig {
    int filedNum;
    char *name;
    int type;
    int intVal;
    char *strVal;
    int mqtttype;
} atComdataConfig;

#if defined (__SUPPORT_AZURE_IOT__)
struct atcmd_twin_params {
    int temperature;
    int battery;
    int doorStateChange;
    int doorlock_state;
    int doorState;
};
#endif  /* __SUPPORT_AZURE_IOT__ */

typedef enum {
    EVK = 0,
    DOORLOCK_REF,
    SENSOR_REF,
    SENSOR_DOORLOCK,
    USER_REF
} BOARD_FEATURES;

typedef struct {
    UINT offset;
    UINT content_length;
    UINT received_length;
    UINT size;
    UINT crc;
    UINT imgcrc;
} ota_mcu_fw_stream_info_t;
#define OTA_MCU_FW_STREAM_HEADER_SIZE   sizeof(ota_mcu_fw_stream_info_t)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief get attribut for at-command
 * @param[in] num attribute number
 * @return information of the attiribute
 ****************************************************************************************
 */
atComdataConfig* app_get_at_config(int num);

/**
 ****************************************************************************************
 * @brief free heap when used get attribute
 * @param[in] _config attribute address
 ****************************************************************************************
 */
void app_release_at_config(atComdataConfig *_config);

/**
 ****************************************************************************************
 * @brief read nvram for string
 * @param[in] name nvram item name
 * @return nvram string
 ****************************************************************************************
 */
char* app_get_string_nvram(const char *name);

/**
 ****************************************************************************************
 * @brief read nvram for integer
 * @param[in] name nvram item name
 * @param[in] _val read value
 * @return 0 :success , -1 : error 
 ****************************************************************************************
 */
int app_get_int_nvram(const char *name, int *_val);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxA
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxA(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxB
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxB(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxC
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxC(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxD
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxD(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxE
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxE(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxF
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxF(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxH
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxH(char *pin_config);

/**
 ****************************************************************************************
 * @brief setting pinmux for muxU
 * @param[in] pin_config mux value string for pinmux
 * @return 0 :success
 ****************************************************************************************
 */
int app_atcmd_user_pinmuxU(char *pin_config);

/**
 ****************************************************************************************
 * @brief set supported board feature
 * @param[in] feature supported feature
 ****************************************************************************************
 */
void app_atcmd_set_features(BOARD_FEATURES feature);

/**
 ****************************************************************************************
 * @brief get supported board feature
 * @return supported feature
 ****************************************************************************************
 */
BOARD_FEATURES app_atcmd_get_features(void);

/**
 ****************************************************************************************
 * @brief check whether at configuration is normal
 * @return 0 :NOK , 1 : OK 
 ****************************************************************************************
 */
int32_t app_at_feature_is_valid(void);

/**
 ****************************************************************************************
 * @brief get subscribe count in at-command items
 * @return count
 ****************************************************************************************
 */
int32_t app_at_get_registered_subscribe_count(void);

/**
 ****************************************************************************************
 * @brief get subscribe item registered
 * @param[in] _index item index
 * @return name of the item
 ****************************************************************************************
 */
char* app_at_get_registered_subscribe(int32_t _index);

/**
 ****************************************************************************************
 * @brief get command name subscribed
 * @param[in] _index command index
 * @return name of the command item
 ****************************************************************************************
 */
char* app_at_get_cloud_cmd_payload(int32_t _index);

/**
 ****************************************************************************************
 * @brief set command subscribed to process
 * @param[in] _payload command payload
 * @param[in] _needLen command payload length
 ****************************************************************************************
 */
void app_at_set_cloud_cmd_payload(char *_payload, int32_t _needLen);

/**
 ****************************************************************************************
 * @brief sending at-command to MCU
 * @param[in] _payload data
 * @param[in] _option retry option
 ****************************************************************************************
 */
void APP_PRINTF_ATCMD(char *_payload, uint32_t _option);

/**
 ****************************************************************************************
 * @brief get GPIO port for MCU wakeup pin
 * @param[in] port_num string of port number saved in nvram
 * @return GPIO port number
 ****************************************************************************************
 */
uint8_t app_atcmd_mcu_wakeup_port(char *port_num);

/**
 ****************************************************************************************
 * @brief get GPIO pin number for MCU wakeup pin
 * @param[in] pin_num string of pin number saved in nvram
 * @return GPIO pin number
 ****************************************************************************************
 */
uint16_t app_atcmd_mcu_wakeup_pin(char *pin_num);

/**
 ****************************************************************************************
 * @brief trigerring MCU wakeup pin to wakeup MCU
 * @param[in] port wakeup port number
 * @param[in] pin wakeup pin number
 ****************************************************************************************
 */
void atcmd_mcu_wakeup(uint8_t port, uint16_t pin);

/**
 ****************************************************************************************
 * @brief checking whether uart is ready to interface with MCU
 * @return ready status
 ****************************************************************************************
 */
int isReadyIF(void);

/**
 ****************************************************************************************
 * @brief set configuration regarding DPM
 * @param[in] _type configuration type
 * @param[in] value value
 ****************************************************************************************
 */
void app_atcmd_set_DPM_value(GetValueType _type, int value);

/**
 ****************************************************************************************
 * @brief set sleep configuration
 * @param[in] _sleepMode sleep mode
 * @param[in] _useDpmSet whether use dpm mode or not
 * @param[in] _rtcTime sleep time for sleep2
 * @param[in] _dpmKeepAlive keep alive time
 * @param[in] _userWakeup user wakeup time
 * @param[in] _timWakeup TIM wakeup time
 ****************************************************************************************
 */
void app_atcmd_setSleepModeDtim(int _sleepMode, int _useDpmSet, int _rtcTime, int _dpmKeepAlive, int _userWakeup,
    int _timWakeup);
extern void PRINTF_ATCMD(const char *fmt, ...);
#if defined (__SUPPORT_AZURE_IOT__)
extern struct atcmd_twin_params* app_at_get_twins(void);
extern void app_at_set_twins(struct atcmd_twin_params *param);
extern void app_at_set_resp_string(char *new_string, uint32_t string_size);
#endif  /* __SUPPORT_AZURE_IOT__ */
extern UINT app_writeDataToMCU(UINT offset, UINT tot_len, UINT r_len, UINT *srcMemAddr, UINT size);
#endif
