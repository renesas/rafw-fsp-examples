/***********************************************************************************************************************
* File Name    : at_cmd.h
* Description  : AT command declarations and definitions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AT_CMD_H
#define __AT_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#if (SUPPORT_AWS_APP == 1) /* 0:AWS, 1:AZURE, 2:Matter */
#define PLATFORM_ID 0
#elif (SUPPORT_AZURE_APP == 1)
#define PLATFORM_ID 1
#elif (SUPPORT_MATTER_APP == 1)
#define PLATFORM_ID 2
#else
#define PLATFORM_ID 0
#endif

#if (PLATFORM_ID == 0)
#define PLATFORM "AWS"
#define AWS_USE_FLEET_PROV 0 /* Pleet Provisioning, Not Used */
#elif (PLATFORM_ID == 1)
#define PLATFORM "AZU"
#elif (PLATFORM_ID == 2)
#define PLATFORM "MAT"
#endif
#define THROUGHPUT_TESTMODE

#define PLATFORM_INTERFACE 0 /* 0:UART, 1:SPI */
#define DELIMIT                 ((char *) " ")
#define DELIMIT_EQ              ((char *) "=")
#define DELIMIT_COMMA           ((char *) ",")
#define DELIMIT_COLON           ((char *) ":")

#define STR_AWS                 ((char *) "+"PLATFORM"IOT")
#define STR_OTA                 ((char *) "+"PLATFORM"OTA")
#define STR_TCPSD               ((char *) "+TRDTS")
#define STR_TCPCD               ((char *) "+TRDTC")
#define STR_BAUD                ((char *) "+BAUDRATE")
#define STR_OK                  ((char *) "OK")
#define STR_ERR                 ((char *) "ERROR")
#define STR_PLUS                ('+')

#define TX_PAYLOAD_MAX_SIZE     1024 * 2
#define UART1_GET_BUF_SIZE      64 * 2
#define UART1_OTA_BUF_SIZE      (4 * 1024 + 48)
#define MAX_PARAMS              19

/* To be implemented for robust communication with wifi-module */
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
/* #define AT_ACK_CHECK */
#if defined(AT_ACK_CHECK)
#define RES_OK                  "AT+"PLATFORM" OK"
#endif /* AT_ACK_CHECK */
#define RES_OTA_OK              "AT+"PLATFORM" OK_OTA"
#define RES_OTA_FAIL            "AT+"PLATFORM" OK_OTA_FAIL"
#define RETRY_ACK_CHECK         0
#define RETRY_ACK_CHECK_SKIP    1

/* new configuration */
#define REQ_STATUS              "AT+"PLATFORM" CMD GET_STATUS"
#define REQ_FACTORY             "AT+"PLATFORM" CMD FACTORY_RESET"
#define REQ_APMODE              "AT+"PLATFORM" CMD RESET_TO_AP"
#define REQ_RESTART             "AT+"PLATFORM" CMD RESTART"

#define REQ_DOOR_OPENED         "AT+"PLATFORM" CMD MCU_DATA 1 mcu_door opened"
#define REQ_DOOR_CLOSED         "AT+"PLATFORM" CMD MCU_DATA 1 mcu_door closed"
#define REQ_WINDOW_OPENED       "AT+"PLATFORM" CMD MCU_DATA 3 mcu_window opened"
#define REQ_WINDOW_CLOSED       "AT+"PLATFORM" CMD MCU_DATA 3 mcu_window closed"
#if (PLATFORM_ID == 0)
#define REQ_SHADOW_UPDATED      "AT+"PLATFORM" CMD MCU_DATA 9 mcu_shadow updated"
#else
#define REQ_SHADOW_UPDATED      "AT+"PLATFORM" CMD MCU_DATA 6 mcu_shadow updated"
#endif

#else
/* #define AT_ACK_CHECK */
#if defined(AT_ACK_CHECK)
#define RES_OK                  "\r\nAT+"PLATFORM" OK\r\n"
#endif /* AT_ACK_CHECK */
#define RES_OTA_OK                  "\r\nAT+"PLATFORM" OK_OTA\r\n"
#define RES_OTA_FAIL                "\r\nAT+"PLATFORM" OK_OTA_FAIL\r\n"
#define RETRY_ACK_CHECK         0
#define RETRY_ACK_CHECK_SKIP    1

/* new configuration */
#define REQ_STATUS              "\r\nAT+"PLATFORM" CMD GET_STATUS\r\n"
#define REQ_FACTORY             "\r\nAT+"PLATFORM" CMD FACTORY_RESET\r\n"
#define REQ_APMODE              "\r\nAT+"PLATFORM" CMD RESET_TO_AP\r\n"
#define REQ_RESTART             "\r\nAT+"PLATFORM" CMD RESTART\r\n"

#define REQ_DOOR_OPENED         "\r\nAT+"PLATFORM" CMD MCU_DATA 1 mcu_door opened\r\n"
#define REQ_DOOR_CLOSED         "\r\nAT+"PLATFORM" CMD MCU_DATA 1 mcu_door closed\r\n"
#define REQ_WINDOW_OPENED       "\r\nAT+"PLATFORM" CMD MCU_DATA 3 mcu_window opened\r\n"
#define REQ_WINDOW_CLOSED       "\r\nAT+"PLATFORM" CMD MCU_DATA 3 mcu_window closed\r\n"
#if (PLATFORM_ID == 0)
#define REQ_SHADOW_UPDATED      "\r\nAT+"PLATFORM" CMD MCU_DATA 9 mcu_shadow updated\r\n"
#else
#define REQ_SHADOW_UPDATED      "\r\nAT+"PLATFORM" CMD MCU_DATA 6 mcu_shadow updated\r\n"
#endif
#endif
/* send wait ok until 500ms, receive send ok routine add */
#define RESPONSE_CLOSE_FALSE    "false"       ///< Azure app response publishing for 'closed'
#define RESPONSE_OPEN_TRUE      "true"        ///< Azure app response publishing for 'opened'
#define RESPONSE_CLOSE          "closed"      ///< Azure app response publishing for 'closed'
#define RESPONSE_OPEN           "opened"      ///< Azure app response publishing for 'opened'
#define RESPONSE_OPEN_APP       "app"         ///< open response by APP
#define RESPONSE_OPEN_MCU       "mcu"         ///< open response by MCU 
#define RESPONSE_NOT_OPEN       "none"        ///< not open response (checked by sensor)

#define UART_BAUDRATE_IF        115200
#define UART_BAUDRATE_OTA       921600
/* da16200 config data */
#define MAX_BUF_SIZE                8192 + 512

#if 1 /* org */
#define MAX_CFG_NUM             30
#else /* elliottest::maximum test */
#define MAX_CFG_NUM             44
#endif

typedef int (*command_function_t)(int argc, char *argv[]);

typedef struct {
    /* The command name matched at the command line. */
    char *name;

    /* Function that runs the command. */
    command_function_t command;

    /* Minimum number of arguments. */
    int arg_count;

    /* String describing argument format used
     * by the generic help generator function. */
    char *format;

    /* Brief description of the command */
    char *brief;
} command_t;

/* Error Response Codes */
typedef enum _atcmd_error_code {
    /* No error (without Debugging messages) */
    ERR_CMD_OK_WO_PRINT = 1,
    /* No error */
    ERR_CMD_OK = 0,
    /* Unknown command */
    ERR_UNKNOWN_CMD = -1,
    /* Insufficient parameter */
    ERR_INSUFFICENT_ARGS = -2,
    /* Too many parameters */
    ERR_TOO_MANY_ARGS = -3,
    /* Wrong parameter value */
    ERR_WRONG_ARGUMENTS = -4,
    /* Unsupported function */
    ERR_NOT_SUPPORTED = -5,
    /* Unknown error */
    ERR_UNKNOWN = -99
} atcmd_error_code;

typedef enum _DeviceStatus {
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
    ATCMD_Status_init_stat = 98,
    ATCMD_Status_check_clear = 99
} DeviceStatus;

typedef struct {
    uint32_t offset;
    uint32_t content_length;
    uint32_t received_length;
    uint32_t size;
    uint32_t crc;
    uint32_t imgcrc;
} ota_mcu_fw_stream_info_t;

typedef enum _IDX_ATCMD {
    ATCMD_NONE,
    ATCMD_ESC,
    ATCMD_COMMON,
    ATCMD_TRSSLWR,
    ATCMD_TRSSLCERTSTORE,
    ATCMD_MATTER,
    ATCMD_MAX
} IDX_ATCMD;

#define OTA_MCU_FW_STREAM_HEADER_SIZE sizeof(ota_mcu_fw_stream_info_t)
// Hex Dump print //
#define OUTPUT_ASCII_ONLY 0
#define OUTPUT_HEXA_ONLY 1
#define OUTPUT_HEXA_ASCII 2

#define BIT_STATUS      ( 1 << 0 )
#define BIT_OK          ( 1 << 1 )
#define BIT_ERR         ( 1 << 2 )
#define BIT_BAUD        ( 1 << 3 )
#define SET_BIT(x, bit)      (x = (uint32_t)(x | (uint32_t)bit))
#define CLR_BIT(x, bit)      (x = (uint32_t)(x & (~(uint32_t)bit)))

extern uint32_t g_app_at_start;

extern int get_status_ready;
extern uint32_t revTotalLen;
extern uint32_t thr_test_rdy;
extern uint32_t start_tick, last_tick;
extern uint8_t isEscRes;

extern void PRINTF(char *fmt, ...);
extern void PRINTF_ATCMD(char *fmt, ...);
extern void PRINTF_ATCMD_PUT(char *str, uint32_t len);
extern uint32_t PRINTF_ATCMD_WAITRES(uint32_t res_bit, uint32_t timeout_ms);

extern void wake_up_wifi(void);
atcmd_error_code command_parser(char *line);
atcmd_error_code atcmd_config_set(void);
atcmd_error_code atcmd_certificate(uint8_t idx);

void set_dev_status(int val);
int get_device_status(void);
int Send_to_DA16200(uint8_t *sData, uint16_t size, uint8_t retry_skip);
int get_send_ok(void);
void blink_auto_LED(uint8_t on);
void set_uart_baudrate(uint32_t baud);
void hexa_dump_print(uint8_t *title, const void *buf, size_t len, char direction, char output_fmt);
void ath_timer_reset(void);
#ifdef __cplusplus
}
#endif

#endif /* __AT_CMD_H */
