/*
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * Includes
 ***********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "rm_atcmd_w_cfg.h"
#if (ATCMD_DA14XXX_CODELESS == 1)

 #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)
  #include "rm_pmgr_api.h"

  #define PMGR_CONSTRAINT_BLE_CONNECTIVITY    PMGR_CONSTRAINT_SLEEP_PROHIBITED

  #define PMGR_ADD_SLEEP_CONSTRAINT()       RM_PMGR_W_add_sleep_constraint(&g_pmgr_w_ctrl, \
                                                                           PMGR_CONSTRAINT_BLE_CONNECTIVITY)
  #define PMGR_REMOVE_SLEEP_CONSTRAINT()    RM_PMGR_W_remove_sleep_constraint(&g_pmgr_w_ctrl, \
                                                                              PMGR_CONSTRAINT_BLE_CONNECTIVITY)
 #endif

 #include "rm_atcmd_w_core_err_code.h"
 #include "rm_atcmd_w_core.h"

 #include "r_uart_api.h"
 #include "r_uart_w.h"

 #include "common_data.h"
 #include "rm_atcmd_w_core_da14xxx_parse.h"
 #include "rm_atcmd_w_da14xxx_image.h"

/* This code is needed for using FreeRTOS */
 #if (BSP_CFG_RTOS == 2 || BSP_CFG_RTOS_USED == 1)
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
 #else
  #error "This moduless is supposed to run under FreeRTOS"
 #endif

/* Board specific configuration. */
 #define RM_ATCMD_W_DA14XXX_UART_CHANNEL               (2)
 #define BSP_VECTOR_UART_W2_IRQ                        ((IRQn_Type) UART3_IRQn)

/***********************************************************************************************************************
 * Defines
 **********************************************************************************************************************/

 #define RM_ATCMD_W_DA14XXX_ATCMD_AT                   "AT\r"
 #define RM_ATCMD_W_DA14XXX_ATCMD_OK                   "OK\r"
 #define RM_ATCMD_W_DA14XXX_ATCMD_REBOOT               "ATR\r"
 #define RM_ATCMD_W_DA14XXX_EVENT_READY                "+READY\n"
 #define RM_ATCMD_W_DA14XXX_ATCMD_TIMEOUT              "\r\nERROR: DA14XXX timeout\r\n"
 #define RM_ATCMD_W_DA14XXX_ATCMD_OVERFLOW             "\r\nERROR: DA14XXX Rx buffer overflow\r\n"

 #define RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ           (3)
 #define ATCMD_W_DISPATCH                              (11)

/* UART boot protocol message types */
 #define RM_ATCMD_W_DA14XXX_BOOT_STX                   0x02
 #define RM_ATCMD_W_DA14XXX_BOOT_SOH                   0x01
 #define RM_ATCMD_W_DA14XXX_BOOT_ACK                   0x06
 #define RM_ATCMD_W_DA14XXX_BOOT_NACK                  0x15

 #define RM_ATCMD_W_DA14XXX_MSG_TX_TIMEOUT_MS          5000
 #define RM_ATCMD_W_DA14XXX_MSG_TX_BLOCK_TIMEOUT_MS    5000
 #define RM_ATCMD_W_CORE_DA14XXX_CMD_TIMEOUT_MS        8100 /* 8 s (AT+GAPSCAN) + 100 ms */
 #define RM_ATCMD_W_CORE_DA14XXX_BINACK_TIMEOUT_MS     100  /* 100 ms - wait for binary mode exit confirmation */

 #define RM_ATCMD_W_DA14XXX_RESET_PULSE_MS             (200)
 #define RM_ATCMD_W_DA14XXX_WAKE_UP_PULSE_MS           (100)
 #define RM_ATCMD_W_DA14XXX_RESET_PIN                  (BSP_IO_PORT_01_PIN_10)
 #define RM_ATCMD_W_DA14XXX_POR_RESET_PIN              (BSP_IO_PORT_01_PIN_14)
 #define RM_ATCMD_W_DA14XXX_WAKE_UP_PIN                (BSP_IO_PORT_01_PIN_13)

 #define RM_ATCMD_W_DA14XXX_TASK_STACK_SIZE            (1024 / sizeof(StackType_t))
 #define RM_ATCMD_W_DA14XXX_OPEN                       (0x41544441) /* ATDA */

typedef uint16_t da14xxx_status_t;

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_CODE(atcmd)    "AT"  # atcmd
 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_CODE(atcmd)          "AT+" # atcmd

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(atcmd) \
    uint32_t RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _cmd_cb(atcmd_w_ctrl_t * const p_at_ctrl, int argc, char * argv[])
 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(atcmd) \
    const char * RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _format_cb(void)
 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(atcmd) \
    const char * RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _brief_cb(void)

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB_P(atcmd)           RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _cmd_cb
 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB_P(atcmd)    RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _format_cb
 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB_P(atcmd)     RM_ATCMD_W_CORE_DA14xxx_ ## atcmd ## _brief_cb

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(atcmd) \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(atcmd);            \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(atcmd);     \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(atcmd)

/***********************************************************************************************************************
 * Enumerations
 **********************************************************************************************************************/

typedef enum
{
    ATCMD_RESP_MESSAGE,                // regular response message processing
    ATCMD_RESP_CR,                     // CR received
    ATCMD_RESP_LF,                     // RF received
    ATCMD_RESP_POSSIBLE_OK_O,          // maybe 'O' from 'OK'
    ATCMD_RESP_POSSIBLE_OK_K,          // maybe 'K' from 'OK'
    ATCMD_RESP_POSSIBLE_ERROR_E,       // maybe 'E' from 'ERROR'
    ATCMD_RESP_POSSIBLE_ERROR_R1,      // maybe 1st 'R' from 'ERROR'
    ATCMD_RESP_POSSIBLE_ERROR_R2,      // maybe 2nd 'R' from 'ERROR'
    ATCMD_RESP_POSSIBLE_ERROR_O,       // maybe 'O' from 'ERROR'
    ATCMD_RESP_POSSIBLE_ERROR_R3,      // maybe 3rd 'R' from 'ERROR'
    ATCMD_RESP_CMD_END_CR,             // CR at the end of command
    ATCMD_RESP_ERROR_EXT,              // error message
    ATCMD_EVENT_READY_R,               // +READY R
    ATCMD_EVENT_READY_E,               // +READY E
    ATCMD_EVENT_READY_A,               // +READY A
    ATCMD_EVENT_READY_D,               // +READY D
    ATCMD_EVENT_READY_Y,               // +READY Y
    ATCMD_EVENT_READY_CR,              // +READY CR
    ATCMD_EVENT_AWAKE_A,               // +AWAKE A
    ATCMD_EVENT_AWAKE_W,               // +AWAKE W
    ATCMD_EVENT_AWAKE_A2,              // +AWAKE A2
    ATCMD_EVENT_AWAKE_K,               // +AWAKE K
    ATCMD_EVENT_AWAKE_E,               // +AWAKE E
    ATCMD_EVENT_AWAKE_CR,              // +AWAKE CR
    ATCMD_EVENT_BINREQACK_B,           // +BINREQACK B
    ATCMD_EVENT_BINREQACK_I,           // +BINREQACK I
    ATCMD_EVENT_BINREQACK_N,           // +BINREQACK N
    ATCMD_EVENT_BINREQACK_R,           // +BINREQACK R
    ATCMD_EVENT_BINREQACK_E,           // +BINREQACK E
    ATCMD_EVENT_BINREQACK_Q,           // +BINREQACK Q
    ATCMD_EVENT_BINREQACK_A,           // +BINREQACK A
    ATCMD_EVENT_BINREQACK_C,           // +BINREQACK C
    ATCMD_EVENT_BINREQACK_K,           // +BINREQACK K
    ATCMD_EVENT_BINREQACK_CR,          // +BINREQACK CR
    ATCMD_RESP_UNDEFINED,
    ATCMD_RESP_MAX_NUM = ATCMD_RESP_UNDEFINED
} atcmd_resp_parser_state_t;

typedef void (* atcmd_parser_rx_handler_t)(uint8_t rxd_byte);

/***********************************************************************************************************************
 * Extern variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Local function prototypes
 **********************************************************************************************************************/

static void                   da14xxx_idle_parse_rx_char(uint8_t rxd_byte);
static void                   da14xxx_response_parse_rx_char(uint8_t rxd_byte);
static void                   da14xxx_event_parse_rx_char(uint8_t rxd_byte);
static fsp_err_atcmd_err_code rm_atcmd_w_da14xxx_proc_cmd_raw(atcmd_w_ctrl_t * const p_at_ctrl,
                                                              const char           * cmdstr,
                                                              uint32_t               cmdlen);

/* AT Command handlers */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(I);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(R);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BDADDR);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(PRINT);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(PIN);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(ADVSTART);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(ADVSTOP);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(ADVDATA);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(ADVRESP);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(CENTRAL);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(PERIPHERAL);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(GAPSTATUS);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(GAPSCAN);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(GAPCONNECT);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(GAPDISCONNECT);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(CLRBNDE);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(CHGBNDP);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(IEBNDE);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(RSSI);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(SEC);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(ENPRMD);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(DISPRMD);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(GETPRMD);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINREQ);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINREQACK);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINREQEXIT);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINREQEXITACK);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINRESUME);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(BINESC);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(SLEEP);
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CALLBACKS(DEVNAME);

/***********************************************************************************************************************
 * Static Private Variables
 **********************************************************************************************************************/

typedef struct __attribute__((packed))
{
    uint8_t  soh;
    uint16_t length;
}
rm_atcmd_w_da14xxx_boot_header_t;

/* Sleep modes supported by DA14XXX */
typedef enum
{
    SLEEP_MODE_NO_SLEEP,               /* Disable sleep */
    SLEEP_MODE_EXTENDED_SLEEP,         /* Extended sleep */
    SLEEP_MODE_DEEP_SLEEP,             /* Deep sleep */
    SLEEP_MODE_HIBERNATION,            /* Hibernation */
    SLEEP_MODE_INVAL
} sleep_mode_t;

/* AT command execution and event status */
typedef enum
{
    ATCMD_IDLE,                        /* No commands in progress */
    ATCMD_IN_PROGRESS,                 /* Command execution in progress */
    ATCMD_OK,                          /* Command completed successfully */
    ATCMD_ERROR,                       /* Command completed with errror status */
} atcmd_status_t;

/* AT events */
typedef enum
{
    ATEVT_NO_EVENT,
    ATEVT_READY,
    ATEVT_AWAKE,
    ATEVT_BINREQACK,
} atcmd_event_t;

typedef struct
{
    atcmd_resp_parser_state_t state;
    uint32_t idx;
} rm_atcmd_w_core_da14xxx_rx_parser_t;

static struct
{
    atcmd_status_t            cmd_status;
    atcmd_event_t             event;
    atcmd_parser_rx_handler_t rx_handler;
    struct
    {
        volatile bool     overflow;
        volatile uint32_t wr_idx;
        volatile uint32_t pc_idx;
        uint32_t          rd_idx;
        uint8_t           data[RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX];
    } rx;
    rm_atcmd_w_core_da14xxx_rx_parser_t resp;
    rm_atcmd_w_core_da14xxx_rx_parser_t evt;
    bool         disable_output;
    bool         is_ready;
    bool         is_binary;
    sleep_mode_t sleep_mode;
    struct
    {
        union
        {
            uint32_t u32;
            uint8_t  u8[4];
        }        seq;
        uint32_t pre_delay;
        uint32_t post_delay;
    }        esc_seq;
    uint32_t open;
} g_da14xxx_atcmd =
{
    .rx_handler         = da14xxx_idle_parse_rx_char,
    .is_ready           = false,
    .is_binary          = false,
    .esc_seq.seq.u32    = 0x2b2b2b,    // default escape sequence: '+++'
    .esc_seq.pre_delay  = 1000,        // default delay on line before escape sequence, 1000 ms
    .esc_seq.post_delay = 1000,        // default delay on line after escape sequence, 1000 ms
};

/* Set this flag when booting DA1453x from host. It informs UART receive interrupt handler that incoming bytes are not
 * AT messages and should instead be handled by boot loader function. */
static volatile bool g_booting = false;

/* When booting DA1453x from host MCU received bytes are stored in this variable and are processed by the boot loader function */
static volatile uint8_t g_rx_boot_byte          = 0;
static volatile uint8_t g_rx_boot_byte_received = 0;

/* UART for module/host communications */

static SemaphoreHandle_t g_tx_semaphore;
static StaticSemaphore_t g_tx_semaphore_data;
static SemaphoreHandle_t g_rx_semaphore;
static StaticSemaphore_t g_rx_semaphore_data;
static TaskHandle_t      g_rx_dispatch_task;
static StaticTask_t      g_rx_dispatch_task_data;
static StackType_t       g_rx_dispatch_task_stack[RM_ATCMD_W_DA14XXX_TASK_STACK_SIZE];

void RM_ATCMD_W_DA14XXX_UartCallback(uart_callback_args_t * p_args);

uart_w_instance_ctrl_t g_uart_ctrl;

uart_w_baud_setting_t g_baud_setting =
{
    .int_baud = 21,
    .fra_baud = 45,
};

/** UART extended configuration for UART_W HAL driver */
const uart_w_extended_cfg_t g_uart_cfg_extend =
{
    .flow_control     = UART_W_AUTO_FLOW_CONTROL_ENABLED,
    .loop_back_enable = UART_W_LOOP_BACK_DISABLE,
    .fifo_enable      = UART_W_FIFO_ENABLE,
    .p_baud_setting   = &g_baud_setting,
};

/** UART interface configuration */
const uart_cfg_t g_uart_cfg =
{
    .channel       = RM_ATCMD_W_DA14XXX_UART_CHANNEL,

    .data_bits     = UART_W_DATA_BITS_8,
    .parity        = UART_PARITY_OFF,
    .stop_bits     = UART_STOP_BITS_1,
    .p_callback    = RM_ATCMD_W_DA14XXX_UartCallback,
    .p_context     = NULL,
    .p_extend      = &g_uart_cfg_extend,
    .p_transfer_tx = NULL,
    .p_transfer_rx = NULL,
    .rxi_ipl       = (12),
    .rxi_irq       = BSP_VECTOR_UART_W2_IRQ,
};

static void da14xxx_atcmd_set_parser (atcmd_parser_rx_handler_t rx_handler)
{
    static atcmd_parser_rx_handler_t prev_handler = da14xxx_idle_parse_rx_char;

    if (rx_handler == NULL)
    {
        g_da14xxx_atcmd.rx_handler = prev_handler;

        return;
    }

    /* AT command processing starts when event is processing */
    if ((g_da14xxx_atcmd.rx_handler == da14xxx_event_parse_rx_char) &&
        (rx_handler == da14xxx_response_parse_rx_char))
    {
        prev_handler = da14xxx_response_parse_rx_char;
    }
    else
    {
        prev_handler               = g_da14xxx_atcmd.rx_handler;
        g_da14xxx_atcmd.rx_handler = rx_handler;
    }
}

static void da14xxx_idle_parse_rx_char (uint8_t rxd_byte)
{
    static atcmd_resp_parser_state_t state = ATCMD_RESP_MESSAGE;

    /* Ignore zero byte in IDLE */
    if (rxd_byte == '\0')
    {
        return;
    }

    switch (state)
    {
        case ATCMD_RESP_CR:
        {
            if (rxd_byte == '\n')
            {
                state = ATCMD_RESP_LF;
            }
            else
            {
                state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_LF:
        {
            if (rxd_byte == '+')
            {
                /* Event received, switch to event parser */
                g_da14xxx_atcmd.evt.idx = g_da14xxx_atcmd.rx.rd_idx--;
                da14xxx_atcmd_set_parser(da14xxx_event_parse_rx_char);
                state = ATCMD_RESP_MESSAGE;
            }
            else if (rxd_byte == '\r')
            {
                state = ATCMD_RESP_CR;
            }
            else
            {
                state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        /* Response or event processing stage - looking for CR */
        case ATCMD_RESP_MESSAGE:
        default:
        {
            if (rxd_byte == '\r')
            {
                state = ATCMD_RESP_CR;
            }

            break;
        }
    }
}

static void da14xxx_response_parse_rx_char (uint8_t rxd_byte)
{
    static atcmd_status_t                 status = ATCMD_IN_PROGRESS;
    rm_atcmd_w_core_da14xxx_rx_parser_t * parser = &g_da14xxx_atcmd.resp;

    switch (parser->state)
    {
        case ATCMD_RESP_CR:
        {
            if (rxd_byte == '\n')
            {
                parser->state = ATCMD_RESP_LF;
            }
            else if (rxd_byte != '\r')
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_LF:
        {
            /* There can be 3 options: OK, ERROR or message continued */
            switch (rxd_byte)
            {
                case 'O':
                {
                    parser->state = ATCMD_RESP_POSSIBLE_OK_O;
                    break;
                }

                case 'E':
                {
                    parser->state = ATCMD_RESP_POSSIBLE_ERROR_E;
                    break;
                }

                case '+':
                {
                    g_da14xxx_atcmd.evt.idx = g_da14xxx_atcmd.rx.rd_idx--;
                    da14xxx_atcmd_set_parser(da14xxx_event_parse_rx_char);
                    parser->state = ATCMD_RESP_MESSAGE;

                    return;
                }

                case '\r':
                {
                    parser->state = ATCMD_RESP_CR;
                    break;
                }

                default:
                {
                    parser->state = ATCMD_RESP_MESSAGE;
                    break;
                }
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_OK_O:
        {
            if (rxd_byte == 'K')
            {
                parser->state = ATCMD_RESP_POSSIBLE_OK_K;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_OK_K:
        {
            if (rxd_byte == '\r')
            {
                parser->state = ATCMD_RESP_CMD_END_CR;
                status        = ATCMD_OK;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_ERROR_E:
        {
            if (rxd_byte == 'R')
            {
                parser->state = ATCMD_RESP_POSSIBLE_ERROR_R1;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_ERROR_R1:
        {
            if (rxd_byte == 'R')
            {
                parser->state = ATCMD_RESP_POSSIBLE_ERROR_R2;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_ERROR_R2:
        {
            if (rxd_byte == 'O')
            {
                parser->state = ATCMD_RESP_POSSIBLE_ERROR_O;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_ERROR_O:
        {
            if (rxd_byte == 'R')
            {
                parser->state = ATCMD_RESP_POSSIBLE_ERROR_R3;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_POSSIBLE_ERROR_R3:
        {
            if (rxd_byte == '\r')
            {
                /* Error without error code received - listen for unsolicited events*/
                parser->state = ATCMD_RESP_CMD_END_CR;
                status        = ATCMD_ERROR;
            }
            else if (rxd_byte == ':')
            {
                parser->state = ATCMD_RESP_ERROR_EXT;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_RESP_ERROR_EXT:
        {
            if (rxd_byte == '\r')
            {
                /* Error with error code received */
                parser->state = ATCMD_RESP_CMD_END_CR;
            }

            break;
        }

        case ATCMD_RESP_CMD_END_CR:
        {
            if (rxd_byte == '\n')
            {
                da14xxx_atcmd_set_parser(da14xxx_idle_parse_rx_char);
                g_da14xxx_atcmd.cmd_status = status;
                status = ATCMD_IN_PROGRESS;
            }

            parser->state = ATCMD_RESP_MESSAGE;
            break;
        }

        /* Response or event processing stage - looking for CR */
        case ATCMD_RESP_MESSAGE:
        default:
        {
            if (rxd_byte == '\r')
            {
                parser->state = ATCMD_RESP_CR;
            }

            break;
        }
    }
}

static void da14xxx_event_parse_rx_char (uint8_t rxd_byte)
{
    static atcmd_event_t event = ATEVT_NO_EVENT;
    rm_atcmd_w_core_da14xxx_rx_parser_t * parser = &g_da14xxx_atcmd.evt;

    switch (parser->state)
    {
        case ATCMD_RESP_CR:
        {
            if (rxd_byte == '\n')
            {
                da14xxx_atcmd_set_parser(NULL);
            }

            parser->state = ATCMD_RESP_MESSAGE;
            break;
        }

        /* Response or event processing stage - looking for CR */
        case ATCMD_RESP_MESSAGE:
        default:
        {
            if (rxd_byte == '\r')
            {
                parser->state = ATCMD_RESP_CR;
            }
            else if (rxd_byte == 'R')
            {
                parser->state = ATCMD_EVENT_READY_R;
            }
            else if (rxd_byte == 'A')
            {
                parser->state = ATCMD_EVENT_AWAKE_A;
            }
            else if (rxd_byte == 'B')
            {
                parser->state = ATCMD_EVENT_BINREQACK_B;
            }

            break;
        }

        case ATCMD_EVENT_READY_R:
        {
            if (rxd_byte == 'E')
            {
                parser->state = ATCMD_EVENT_READY_E;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_READY_E:
        {
            if (rxd_byte == 'A')
            {
                parser->state = ATCMD_EVENT_READY_A;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_READY_A:
        {
            if (rxd_byte == 'D')
            {
                parser->state = ATCMD_EVENT_READY_D;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_READY_D:
        {
            if (rxd_byte == 'Y')
            {
                parser->state = ATCMD_EVENT_READY_Y;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_READY_Y:
        {
            if (rxd_byte == '\r')
            {
                event         = ATEVT_READY;
                parser->state = ATCMD_EVENT_READY_CR;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_READY_CR:
        {
            if (rxd_byte == '\n')
            {
                da14xxx_atcmd_set_parser(NULL);
                g_da14xxx_atcmd.event = event;
            }

            event         = ATEVT_NO_EVENT;
            parser->state = ATCMD_RESP_MESSAGE;
            break;
        }

        case ATCMD_EVENT_AWAKE_A:
        {
            if (rxd_byte == 'W')
            {
                parser->state = ATCMD_EVENT_AWAKE_W;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_AWAKE_W:
        {
            if (rxd_byte == 'A')
            {
                parser->state = ATCMD_EVENT_AWAKE_A2;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_AWAKE_A2:
        {
            if (rxd_byte == 'K')
            {
                parser->state = ATCMD_EVENT_AWAKE_K;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_AWAKE_K:
        {
            if (rxd_byte == 'E')
            {
                parser->state = ATCMD_EVENT_AWAKE_E;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_AWAKE_E:
        {
            if (rxd_byte == '\r')
            {
                event         = ATEVT_AWAKE;
                parser->state = ATCMD_EVENT_AWAKE_CR;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_AWAKE_CR:
        {
            if (rxd_byte == '\n')
            {
                da14xxx_atcmd_set_parser(NULL);
                g_da14xxx_atcmd.event = event;
            }

            event         = ATEVT_NO_EVENT;
            parser->state = ATCMD_RESP_MESSAGE;
            break;
        }

        case ATCMD_EVENT_BINREQACK_B:
        {
            if (rxd_byte == 'I')
            {
                parser->state = ATCMD_EVENT_BINREQACK_I;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_I:
        {
            if (rxd_byte == 'N')
            {
                parser->state = ATCMD_EVENT_BINREQACK_N;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_N:
        {
            if (rxd_byte == 'R')
            {
                parser->state = ATCMD_EVENT_BINREQACK_R;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_R:
        {
            if (rxd_byte == 'E')
            {
                parser->state = ATCMD_EVENT_BINREQACK_E;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_E:
        {
            if (rxd_byte == 'Q')
            {
                parser->state = ATCMD_EVENT_BINREQACK_Q;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_Q:
        {
            if (rxd_byte == 'A')
            {
                parser->state = ATCMD_EVENT_BINREQACK_A;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_A:
        {
            if (rxd_byte == 'C')
            {
                parser->state = ATCMD_EVENT_BINREQACK_C;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_C:
        {
            if (rxd_byte == 'K')
            {
                parser->state = ATCMD_EVENT_BINREQACK_K;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_K:
        {
            if (rxd_byte == '\r')
            {
                parser->state = ATCMD_EVENT_BINREQACK_CR;
                event         = ATEVT_BINREQACK;
            }
            else
            {
                parser->state = ATCMD_RESP_MESSAGE;
            }

            break;
        }

        case ATCMD_EVENT_BINREQACK_CR:
        {
            if (rxd_byte == '\n')
            {
                da14xxx_atcmd_set_parser(NULL);
                g_da14xxx_atcmd.event     = event;
                g_da14xxx_atcmd.is_binary = true;
            }

            event         = ATEVT_NO_EVENT;
            parser->state = ATCMD_RESP_MESSAGE;
            break;
        }
    }
}

static bool da14xxx_binary_check_exit_condition (uint8_t byte, bool h2d)
{
    static uint32_t h2d_seq_idx = 0;
    static uint32_t d2h_seq_idx = 0;
    static uint32_t h2d_cfm_idx = 0;
    static uint32_t d2h_cfm_idx = 0;
    if (h2d)
    {
        if (d2h_seq_idx >= RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ)
        {
            /* Escape sequence detected in d2h direction, look for confirmation */

            if (!((h2d_cfm_idx == 0) && (byte == '\r')) &&
                (RM_ATCMD_W_DA14XXX_ATCMD_AT[h2d_cfm_idx++] != byte))
            {
                h2d_cfm_idx = 0;
                d2h_seq_idx = 0;
            }

            if (h2d_cfm_idx == sizeof(RM_ATCMD_W_DA14XXX_ATCMD_AT) - 1)
            {
                goto exit_binary;
            }
        }
        else
        {
            /* Look for escape sequence in h2d direction */
            if (h2d_seq_idx == RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ)
            {
                h2d_seq_idx = 0;
            }

            if (g_da14xxx_atcmd.esc_seq.seq.u8[h2d_seq_idx++] != byte)
            {
                h2d_seq_idx = 0;
            }
        }
    }
    else
    {
        if (d2h_seq_idx)
        {
            if (d2h_cfm_idx || ((byte != '\r') && (byte != '\n')))
            {
                /* Escape sequence detected in d2h direction, look for confirmation */
                if (RM_ATCMD_W_DA14XXX_ATCMD_OK[d2h_cfm_idx++] != byte)
                {
                    d2h_cfm_idx = 0;
                    if (byte != g_da14xxx_atcmd.esc_seq.seq.u8[d2h_seq_idx - 1])
                    {
                        d2h_seq_idx = 0;
                    }
                }

                if (d2h_cfm_idx == sizeof(RM_ATCMD_W_DA14XXX_ATCMD_OK) - 1)
                {
                    goto exit_binary;
                }
            }
        }
        else
        {
            /* Escape sequence repeats continuously, don't reset when detected once */
            if (g_da14xxx_atcmd.esc_seq.seq.u8[d2h_seq_idx++ % RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ] != byte)
            {
                d2h_seq_idx = 0;
            }
        }
    }

    return false;
exit_binary:
    h2d_seq_idx = 0;
    h2d_cfm_idx = 0;
    d2h_seq_idx = 0;
    d2h_cfm_idx = 0;

    return true;
}

/* Switch internal binary mode exit condition handler state to wait for DA14531 confirmation */
static void da14xxx_binary_switch_exit_condition_to_wait_for_confirmation (void)
{
    /* Imitate escape sequence from the host */
    da14xxx_binary_check_exit_condition('\r', true);
    for (uint32_t i = 0; i < RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ; i++)
    {
        da14xxx_binary_check_exit_condition(g_da14xxx_atcmd.esc_seq.seq.u8[i], true);
    }
}

void RM_ATCMD_W_DA14XXX_UartCallback (uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_RX_CHAR:
        {
            uint8_t data_byte = (uint8_t) p_args->data;

            if (g_booting == true)
            {
                /* In process of booting DA1453x so store received byte for processing
                 * by image loading function. OK to overwrite previously received data,
                 * we are only interested in the last received byte. */
                g_rx_boot_byte          = data_byte;
                g_rx_boot_byte_received = 1;
            }
            else
            {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;

                if (g_da14xxx_atcmd.rx.overflow)
                {

                    /* Drop all data until overflow is handled */
                    return;
                }

                /* Store character to the buffer and check for overflow */
                g_da14xxx_atcmd.rx.data[g_da14xxx_atcmd.rx.wr_idx++] = data_byte;
                g_da14xxx_atcmd.rx.wr_idx %= RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX;
                if (g_da14xxx_atcmd.rx.wr_idx == g_da14xxx_atcmd.rx.pc_idx)
                {
                    g_da14xxx_atcmd.rx.overflow = true;
                }

                if (pdTRUE == xSemaphoreGiveFromISR(g_rx_semaphore, &xHigherPriorityTaskWoken))
                {
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }

            break;
        }

        case UART_EVENT_TX_COMPLETE:
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;

            if (xPortIsInsideInterrupt())
            {
                if (pdTRUE == xSemaphoreGiveFromISR(g_tx_semaphore, &xHigherPriorityTaskWoken))
                {
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
            else
            {
                xSemaphoreGive(g_tx_semaphore);
            }

            break;
        }

        default:
        {
            break;
        }
    }
}

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_REG(cmd, n_args) \
    {                                                         \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_CODE(cmd),        \
        ATCMD_W_TYPE_A,                                       \
        (n_args),                                             \
        0,                                                    \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB_P(cmd),              \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB_P(cmd),       \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB_P(cmd)         \
    }

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(cmd, n_args) \
    {                                                   \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_CODE(cmd),        \
        ATCMD_W_TYPE_A,                                 \
        (n_args),                                       \
        0,                                              \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB_P(cmd),        \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB_P(cmd), \
        RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB_P(cmd)   \
    }

const atcmd_w_core_module_t at_core_da14xxx_module[] =
{
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_REG(I,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_REG(R,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BDADDR,        1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(PRINT,         2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(PIN,           2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(ADVSTART,      2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(ADVSTOP,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(ADVDATA,       2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(ADVRESP,       2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(CENTRAL,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(PERIPHERAL,    1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(GAPSTATUS,     1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(GAPSCAN,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(GAPCONNECT,    3),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(GAPDISCONNECT, 1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(CLRBNDE,       2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(CHGBNDP,       3),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(IEBNDE,        3),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(RSSI,          1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(SEC,           2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(ENPRMD,        1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(DISPRMD,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(GETPRMD,       1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINREQ,        1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINREQACK,     1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINREQEXIT,    1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINREQEXITACK, 1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINRESUME,     1),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(BINESC,        4),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(PRINT,         2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(SLEEP,         2),
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_REG(DEVNAME,       2),

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

/*******************************************************************************************************************//**
 *  Generate a blocking delay.
 *
 * @param[in]  ms       Length of delay in milliseconds
 **********************************************************************************************************************/
static void rm_atcmd_w_da14xxx_delay (uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline void rm_atcmd_w_da14xxx_pin_restore_defaults (bsp_io_port_pin_t pin)
{
    /* Restore pin initial status back to default value */
    for (int i = 0; i < IOPORT_CFG_NAME.number_of_pins; i++)
    {
        if (IOPORT_CFG_NAME.p_pin_cfg_data[i].pin == pin)
        {
            R_BSP_PinCfg(pin, IOPORT_CFG_NAME.p_pin_cfg_data[i].pin_cfg);
            break;
        }
    }
}

static void rm_atcmd_w_da14xxx_hw_reset (void)
{
    R_BSP_PinAccessEnable();

    R_BSP_PinCfg(RM_ATCMD_W_DA14XXX_POR_RESET_PIN, BSP_IO_DIRECTION_OUTPUT);

    /* 1-st stage: bring POR to active mode. */
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_POR_RESET_PIN, BSP_IO_LEVEL_HIGH);
    rm_atcmd_w_da14xxx_delay(RM_ATCMD_W_DA14XXX_RESET_PULSE_MS / 2);

    /* 2-nd stage: bring bootloader reset pin to active mode (high). */
    R_BSP_PinCfg(RM_ATCMD_W_DA14XXX_RESET_PIN, BSP_IO_DIRECTION_OUTPUT);
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_RESET_PIN, BSP_IO_LEVEL_HIGH);
    rm_atcmd_w_da14xxx_delay(RM_ATCMD_W_DA14XXX_RESET_PULSE_MS / 2);

    /* 3-rd stage: set POR inactive. */
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_POR_RESET_PIN, BSP_IO_LEVEL_LOW);

    /* 4-th stage: restore original GPIO state. */
    rm_atcmd_w_da14xxx_pin_restore_defaults(RM_ATCMD_W_DA14XXX_POR_RESET_PIN);
    rm_atcmd_w_da14xxx_pin_restore_defaults(RM_ATCMD_W_DA14XXX_RESET_PIN);

    R_BSP_PinAccessDisable();
}

static void rm_atcmd_w_da14xxx_prepare_hw_wake_up (void)
{
    R_BSP_PinAccessEnable();

    R_BSP_PinCfg(RM_ATCMD_W_DA14XXX_WAKE_UP_PIN, BSP_IO_DIRECTION_OUTPUT);
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_WAKE_UP_PIN, BSP_IO_LEVEL_LOW);

    R_BSP_PinAccessDisable();
}

static fsp_err_atcmd_err_code rm_atcmd_w_da14xxx_hw_wake_up (atcmd_w_ctrl_t * const p_at_ctrl)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_TIMEOUT;
    g_da14xxx_atcmd.is_ready = false;

    R_BSP_PinAccessEnable();

    /* Generate an active high wake up */
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_WAKE_UP_PIN, BSP_IO_LEVEL_HIGH);
    rm_atcmd_w_da14xxx_delay(RM_ATCMD_W_DA14XXX_WAKE_UP_PULSE_MS);
    R_BSP_PinWrite(RM_ATCMD_W_DA14XXX_WAKE_UP_PIN, BSP_IO_LEVEL_LOW);

    /* Restore Wake Up pin initial status */
    rm_atcmd_w_da14xxx_pin_restore_defaults(RM_ATCMD_W_DA14XXX_WAKE_UP_PIN);
    R_BSP_PinAccessDisable();

    /* Wait for +AWAKE notification */
    uint32_t timeout = (uint32_t) xTaskGetTickCount() + pdMS_TO_TICKS(RM_ATCMD_W_CORE_DA14XXX_CMD_TIMEOUT_MS);
    do
    {
        RM_ATCMD_W_CORE_DA14xxx_ProcEvents(p_at_ctrl);
        if (g_da14xxx_atcmd.is_ready)
        {
            g_da14xxx_atcmd.is_ready = false;
            err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
            break;
        }

        rm_atcmd_w_da14xxx_delay(3);
    } while (timeout > (uint32_t) xTaskGetTickCount());

    if (err == FSP_ERR_AT_CMD_ERR_TIMEOUT)
    {
        da14xxx_atcmd_set_parser(da14xxx_idle_parse_rx_char);
    }

    return err;
}

/*******************************************************************************************************************//**
 *  Transmit a sequence of bytes via the appropriate transport layer.
 *
 * @param[in]  p_data                   Pointer to data to be transmitted
 * @param[in]  len                      Number of bytes to be transmitted
 *
 * @retval FSP_SUCCESS                  Message transmitted successfully
 * @retval FSP_ERR_TIMEOUT              Transport layer failed to transmit message
 **********************************************************************************************************************/
static fsp_err_t rm_atcmd_w_da14xxx_transmit (const uint8_t * p_data, uint32_t len)
{
    if (FSP_SUCCESS == R_UART_W_Write(&g_uart_ctrl, p_data, len))
    {
        /* Wait for transmit to complete */
        if (pdTRUE != xSemaphoreTake(g_tx_semaphore, RM_ATCMD_W_DA14XXX_MSG_TX_BLOCK_TIMEOUT_MS))
        {
            return FSP_ERR_TIMEOUT;
        }
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 *  Calculate CRC of image selected at build time.
 *
 * @retval CRC of image.
 **********************************************************************************************************************/
static uint8_t rm_atcmd_w_da14xxx_image_get_crc (void)
{
    uint8_t  crc = 0;
    uint16_t i;

    for (i = 0; i < g_da14xxx_codeless_image_size; i++)
    {
        crc ^= g_da14xxx_codeless_image[i];
    }

    return crc;
}

/*******************************************************************************************************************//**
 *  Load an image into the DA1453x. DA1453x will automatically start executing the image once loading is complete.
 *
 * @retval FSP_SUCCESS           Image loading successful.
 * @retval FSP_ERR_TIMEOUT       Image loading failed due to timeout.
 **********************************************************************************************************************/
static fsp_err_t rm_atcmd_w_da14xxx_load_image (void)
{
    fsp_err_t         status  = FSP_ERR_TIMEOUT;
    uint8_t           rx_crc  = 0;
    volatile uint32_t timeout = 250;
    uint8_t           nof_stx = 0;

    /* Inform UART ISR how to handle received data */
    g_booting = true;

    /* Reset DA1453x */
    rm_atcmd_w_da14xxx_hw_reset();

    /* Wait for DA1453x bootloader to transmit STX */

    /* We wait for 2 STX in case that there is a 2ry BL in flash.
     * In this case the 1st STX is sent by the boot-rom and the second one
     * from the 2ry BL
     */
    g_rx_boot_byte = 0;
    while ((RM_ATCMD_W_DA14XXX_BOOT_STX != g_rx_boot_byte) && timeout)
    {
        rm_atcmd_w_da14xxx_delay(10);
        timeout -= 10;
    }

    if (timeout > 0)
    {
        nof_stx = 1;
    }

 #if (ATCMD_DA14XXX_CODELESS_SIGNED_IMAGE == 1)
    timeout        = 250;
    g_rx_boot_byte = 0;
    while ((RM_ATCMD_W_DA14XXX_BOOT_STX != g_rx_boot_byte) && timeout)
    {
        rm_atcmd_w_da14xxx_delay(10);
        timeout -= 10;
    }

    if (timeout > 0)
    {
        nof_stx++;
    }
 #endif

    if (0 == nof_stx)
    {
        return status;
    }

    rm_atcmd_w_da14xxx_boot_header_t header;

    header.soh    = RM_ATCMD_W_DA14XXX_BOOT_SOH;
    header.length = g_da14xxx_codeless_image_size;

    /* Send header information (SOH and image length in bytes) */
    if (FSP_SUCCESS == rm_atcmd_w_da14xxx_transmit((uint8_t *) &header, sizeof(header)))
    {
        /* Give DA1453x time to transmit response */
        rm_atcmd_w_da14xxx_delay(10);
        if (RM_ATCMD_W_DA14XXX_BOOT_ACK == g_rx_boot_byte)
        {
            /* Transmit image to DA1453x */
            if (FSP_SUCCESS == rm_atcmd_w_da14xxx_transmit(g_da14xxx_codeless_image, g_da14xxx_codeless_image_size))
            {
                /* Wait for CRC from DA1453x */

 #if (ATCMD_DA14XXX_CODELESS_SIGNED_IMAGE == 1)
                rm_atcmd_w_da14xxx_delay(100);

                /* Check CRC, in case of encrypted/signed image the CRC might take up to 2sec to be sent */
                g_rx_boot_byte_received = 0;
                timeout                 = 2500;
                while (timeout && (g_rx_boot_byte_received == 0))
                {
                    rm_atcmd_w_da14xxx_delay(10);
                    timeout -= 10;
                }

 #else
                rm_atcmd_w_da14xxx_delay(10);
 #endif

                /* Check CRC */
                rx_crc = g_rx_boot_byte;
                if (rx_crc == rm_atcmd_w_da14xxx_image_get_crc())
                {
                    uint8_t ack = RM_ATCMD_W_DA14XXX_BOOT_ACK;
                    if (FSP_SUCCESS == rm_atcmd_w_da14xxx_transmit(&ack, sizeof(ack)))
                    {
                        status = FSP_SUCCESS;
                    }
                }
            }
        }
    }

    g_booting = false;

    return status;
}

static void rm_atcmd_w_da14xxx_rx_dispatch_thread_func (void * p_param)
{
    atcmd_w_ctrl_t * const p_at_ctrl = p_param;

    while (1)
    {
        /* Wait for the external event */
        xSemaphoreTake(g_rx_semaphore, portMAX_DELAY);

        if (RM_ATCMD_W_DA14XXX_OPEN != g_da14xxx_atcmd.open)
        {
            return;                    /* Gracefully shutdown the thread. */
        }

        if (RM_ATCMD_W_CORE_DA14xxx_IsBinaryMode(p_at_ctrl))
        {
            /* Process data received from DA14XXX */
            RM_ATCMD_W_CORE_DA14xxx_BinaryRead(p_at_ctrl);
        }
        else
        {
            /* Process DA14XXX events, errors are notified internally */
            RM_ATCMD_W_CORE_DA14xxx_ProcEvents(p_at_ctrl);
        }
    }
}

/***********************************************************************************************************************
 * Public Functions Implementation
 **********************************************************************************************************************/

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_Register (atcmd_w_core_module_list_t * p_list)
{
 #if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
 #endif

    /* Register DA14xxx AT subsystem */
    if (p_list->module_cnt >= ATCMD_W_LIST_MAX_CNT)
    {
        return FSP_ERR_UNSUPPORTED;
    }

    if (rm_atcmd_w_core_register_module_node(p_list, at_core_da14xxx_module) == FSP_ERR_AT_CMD_ERR_MEM_ALLOC)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    return FSP_SUCCESS;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_Unregister (atcmd_w_core_module_list_t * p_list)
{
 #if (ATCMD_W_CFG_PARAM_CHECKING_ENABLE)
    FSP_ASSERT(p_list);
 #endif

    rm_atcmd_w_core_deregister(p_list, at_core_da14xxx_module);

    return FSP_SUCCESS;
}

static fsp_err_atcmd_err_code rm_atcmd_w_da14xxx_reboot (atcmd_w_ctrl_t * const p_at_ctrl)
{
    fsp_err_t              fsp_err;
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_TIMEOUT;
    g_da14xxx_atcmd.is_ready = false;

    /* HW reset and load codeless FW image */
    fsp_err = rm_atcmd_w_da14xxx_load_image();
    if (fsp_err != FSP_SUCCESS)
    {
        return FSP_ERR_AT_CMD_ERR_TIMEOUT;
    }

    /* Wait for +READY notification */
    uint32_t timeout = (uint32_t) xTaskGetTickCount() + pdMS_TO_TICKS(RM_ATCMD_W_CORE_DA14XXX_CMD_TIMEOUT_MS);
    do
    {
        RM_ATCMD_W_CORE_DA14xxx_ProcEvents(p_at_ctrl);
        if (g_da14xxx_atcmd.is_ready)
        {
            g_da14xxx_atcmd.is_ready = false;
            err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
            break;
        }

        rm_atcmd_w_da14xxx_delay(3);
    } while (timeout > (uint32_t) xTaskGetTickCount());

    if (err == FSP_ERR_AT_CMD_ERR_TIMEOUT)
    {
        da14xxx_atcmd_set_parser(da14xxx_idle_parse_rx_char);
    }

    return err;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_Open (atcmd_w_ctrl_t * const p_at_ctrl)
{
    fsp_err_t err = FSP_SUCCESS;

    /**
     * Open AT connection to the DA14xxx:
     * 1. Open serial connection
     * 2. Reset DA14xxx
     * 3. Load codeless image to it
     */
    g_tx_semaphore     = xSemaphoreCreateBinaryStatic(&g_tx_semaphore_data);
    g_rx_semaphore     = xSemaphoreCreateBinaryStatic(&g_rx_semaphore_data);
    g_rx_dispatch_task = xTaskCreateStatic(rm_atcmd_w_da14xxx_rx_dispatch_thread_func,
                                           "ATCMD_DA14XX_RX_DISPATCH",
                                           RM_ATCMD_W_DA14XXX_TASK_STACK_SIZE,
                                           p_at_ctrl,
                                           OS_TASK_PRIORITY_LOWEST + ATCMD_W_DISPATCH,
                                           g_rx_dispatch_task_stack,
                                           &g_rx_dispatch_task_data);

    /* Configure UART for TIN to DA14531 communications */

    err = R_UART_W_BaudCalculate(115200, &g_baud_setting);
    if (FSP_SUCCESS != err)
    {
        goto RM_ATCMD_W_CORE_DA14xxx_Open_error;
    }

    err = R_UART_W_Open(&g_uart_ctrl, &g_uart_cfg);
    if (err != FSP_SUCCESS)
    {
        goto RM_ATCMD_W_CORE_DA14xxx_Open_error;
    }

    err = rm_atcmd_w_da14xxx_load_image();
    if (err != FSP_SUCCESS)
    {
        goto RM_ATCMD_W_CORE_DA14xxx_Open_error;
    }

 #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)
    err = PMGR_ADD_SLEEP_CONSTRAINT();
    if (err != FSP_SUCCESS)
    {
        goto RM_ATCMD_W_CORE_DA14xxx_Open_error;
    }
 #endif

    g_da14xxx_atcmd.open = RM_ATCMD_W_DA14XXX_OPEN;

    return FSP_SUCCESS;

RM_ATCMD_W_CORE_DA14xxx_Open_error:

    R_UART_W_Close(&g_uart_ctrl);

    vTaskDelete(g_rx_dispatch_task);

    xSemaphoreGive(g_rx_semaphore);

    vSemaphoreDelete(g_tx_semaphore);
    vSemaphoreDelete(g_rx_semaphore);

    return err;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_Close (atcmd_w_ctrl_t * const p_at_ctrl)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);

    g_da14xxx_atcmd.open = 0;

 #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)
    PMGR_REMOVE_SLEEP_CONSTRAINT();
 #endif

    R_UART_W_Close(&g_uart_ctrl);

    vTaskDelete(g_rx_dispatch_task);

    xSemaphoreGive(g_rx_semaphore);

    vSemaphoreDelete(g_tx_semaphore);
    vSemaphoreDelete(g_rx_semaphore);

    return FSP_SUCCESS;
}

static fsp_err_t rm_atcmd_w_da14xxx_send_resp_if_avail (atcmd_w_ctrl_t * const p_at_ctrl)
{
    if (g_da14xxx_atcmd.is_binary)
    {
        /* Process input data here in binary mode only as ProcEvents is not used */
        g_da14xxx_atcmd.rx.rd_idx = g_da14xxx_atcmd.rx.wr_idx;
    }

    /* In case of overflow notify the host and reset internals */
    if (g_da14xxx_atcmd.rx.overflow)
    {
        RM_ATCMD_W_CORE_Write(p_at_ctrl, (const uint8_t *) RM_ATCMD_W_DA14XXX_ATCMD_OVERFLOW,
                              sizeof(RM_ATCMD_W_DA14XXX_ATCMD_OVERFLOW) - 1);

        g_da14xxx_atcmd.rx.rd_idx   = 0;
        g_da14xxx_atcmd.rx.wr_idx   = 0;
        g_da14xxx_atcmd.rx.pc_idx   = 0;
        g_da14xxx_atcmd.rx.overflow = false;

        return FSP_ERR_OVERFLOW;
    }

    /* Send available data to the host */
    if (g_da14xxx_atcmd.rx.pc_idx != g_da14xxx_atcmd.rx.rd_idx)
    {
        /* Unsent data available */
        if (!g_da14xxx_atcmd.disable_output)
        {
            if (g_da14xxx_atcmd.rx.rd_idx > g_da14xxx_atcmd.rx.pc_idx)
            {
                RM_ATCMD_W_CORE_Write(p_at_ctrl,
                                      (const uint8_t *) &g_da14xxx_atcmd.rx.data[g_da14xxx_atcmd.rx.pc_idx],
                                      g_da14xxx_atcmd.rx.rd_idx - g_da14xxx_atcmd.rx.pc_idx);
            }
            else
            {
                RM_ATCMD_W_CORE_Write(p_at_ctrl,
                                      (const uint8_t *) &g_da14xxx_atcmd.rx.data[g_da14xxx_atcmd.rx.pc_idx],
                                      RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX - g_da14xxx_atcmd.rx.pc_idx);
                if (g_da14xxx_atcmd.rx.rd_idx != 0)
                {
                    RM_ATCMD_W_CORE_Write(p_at_ctrl,
                                          (const uint8_t *) &g_da14xxx_atcmd.rx.data[0],
                                          g_da14xxx_atcmd.rx.rd_idx);
                }
            }
        }

        /* In binary mode look for exit conditions */
        if (g_da14xxx_atcmd.is_binary)
        {
            /* Parse only 1st bytes for exit confirmation */
            uint32_t size = g_da14xxx_atcmd.rx.rd_idx - g_da14xxx_atcmd.rx.pc_idx;
            if (g_da14xxx_atcmd.rx.pc_idx > g_da14xxx_atcmd.rx.rd_idx)
            {
                size = g_da14xxx_atcmd.rx.pc_idx + g_da14xxx_atcmd.rx.pc_idx + RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX;
            }

            for (uint32_t idx = g_da14xxx_atcmd.rx.pc_idx; size > 0; size--)
            {
                if (da14xxx_binary_check_exit_condition(g_da14xxx_atcmd.rx.data[idx++], false))
                {
                    g_da14xxx_atcmd.is_binary = false;
                    g_da14xxx_atcmd.rx.pc_idx = idx;

                    return FSP_SUCCESS;
                }

                idx %= RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX;
            }
        }

        g_da14xxx_atcmd.rx.pc_idx = g_da14xxx_atcmd.rx.rd_idx;

        return FSP_SUCCESS;
    }

    return FSP_ERR_BUFFER_EMPTY;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_ProcEvents (void * const p_ctrl)
{
    if (!p_ctrl)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* Process all symbols received on DA14xxx Rx */
    uint32_t wr_idx = g_da14xxx_atcmd.rx.wr_idx;
    while (g_da14xxx_atcmd.rx.rd_idx != wr_idx)
    {
        g_da14xxx_atcmd.rx_handler(g_da14xxx_atcmd.rx.data[g_da14xxx_atcmd.rx.rd_idx++]);
        g_da14xxx_atcmd.rx.rd_idx %= RM_ATCMD_W_CORE_DA14XXX_RX_DATA_LEN_MAX;

        /* Process detected events immediately, actualy do nothing */
        if (g_da14xxx_atcmd.event != ATEVT_NO_EVENT)
        {
            if ((g_da14xxx_atcmd.event == ATEVT_READY) || (g_da14xxx_atcmd.event == ATEVT_AWAKE))
            {
                g_da14xxx_atcmd.is_ready = true;
            }

            g_da14xxx_atcmd.event = ATEVT_NO_EVENT;
        }
    }

    return rm_atcmd_w_da14xxx_send_resp_if_avail(p_ctrl);
}

static fsp_err_atcmd_err_code rm_atcmd_w_da14xxx_proc_cmd_raw (atcmd_w_ctrl_t * const p_at_ctrl,
                                                               const char           * cmdstr,
                                                               uint32_t               cmdlen)
{
    fsp_err_atcmd_err_code err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;

    sleep_mode_t sleep_mode = g_da14xxx_atcmd.sleep_mode;
    if (sleep_mode >= SLEEP_MODE_EXTENDED_SLEEP)
    {
        g_da14xxx_atcmd.sleep_mode = SLEEP_MODE_NO_SLEEP;

        /* Restore UART connection */
        if (R_UART_W_Open(&g_uart_ctrl, &g_uart_cfg) != FSP_SUCCESS)
        {
            err = FSP_ERR_AT_CMD_ERR_UART_INTERFACE;
            goto exit;
        }

        if (sleep_mode == SLEEP_MODE_EXTENDED_SLEEP)
        {
            err = rm_atcmd_w_da14xxx_hw_wake_up(p_at_ctrl);
            if (err != FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT)
            {
                goto exit;
            }
        }
        else
        {
 #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)
            if (PMGR_ADD_SLEEP_CONSTRAINT() != FSP_SUCCESS)
            {
                return FSP_ERR_AT_CMD_ERR_UNKNOWN;
            }
 #endif

            /* Reset and upload FW */
            err = rm_atcmd_w_da14xxx_reboot(p_at_ctrl);
            if (err != FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT)
            {
                goto exit;
            }
        }
    }

    /* Set parser to waiting for response state */
    da14xxx_atcmd_set_parser(da14xxx_response_parse_rx_char);

    /* Send command to DA14XXX */
    rm_atcmd_w_da14xxx_transmit((uint8_t *) cmdstr, cmdlen);

    /* Wait for DA14XXX response */
    uint32_t timeout = (uint32_t) xTaskGetTickCount() + pdMS_TO_TICKS(RM_ATCMD_W_CORE_DA14XXX_CMD_TIMEOUT_MS);
    do
    {
        if (RM_ATCMD_W_CORE_DA14xxx_ProcEvents(p_at_ctrl) == FSP_SUCCESS)
        {
            /* Reset timeout, data transmition is in progress */
            timeout = (uint32_t) xTaskGetTickCount() + pdMS_TO_TICKS(RM_ATCMD_W_CORE_DA14XXX_CMD_TIMEOUT_MS);
        }

        if (g_da14xxx_atcmd.cmd_status > ATCMD_IN_PROGRESS)
        {
            if (g_da14xxx_atcmd.cmd_status != ATCMD_OK)
            {
                err = FSP_ERR_AT_CMD_ERR_BLE_XXX;
            }

            g_da14xxx_atcmd.cmd_status = ATCMD_IDLE;
            goto exit;
        }

        /* TODO: need to add WDT service reset here to avoid WDT Fault when set delay less than 3ms */
        rm_atcmd_w_da14xxx_delay(3);
    } while (timeout > (uint32_t) xTaskGetTickCount());

    da14xxx_atcmd_set_parser(da14xxx_idle_parse_rx_char);
    RM_ATCMD_W_CORE_Write(p_at_ctrl, (const uint8_t *) RM_ATCMD_W_DA14XXX_ATCMD_TIMEOUT,
                          sizeof(RM_ATCMD_W_DA14XXX_ATCMD_TIMEOUT) - 1);

    err = FSP_ERR_AT_CMD_ERR_TIMEOUT;

exit:

    return err;
}

static fsp_err_atcmd_err_code rm_atcmd_w_da14xxx_proc_cmd (atcmd_w_ctrl_t * const p_at_ctrl,
                                                           const char           * cmd,
                                                           int32_t                argc,
                                                           const char           * argv[])
{
    static char cmdstr[ATCMD_W_RESP_LEN_MAX];
    uint32_t    cmdlen = 0;

    bsp_safe_strcpy(&cmdstr[cmdlen], cmd, ATCMD_W_RESP_LEN_MAX - cmdlen);
    cmdlen += strlen(cmd);

    if (argc > 1)
    {
        cmdstr[cmdlen++] = '=';
        for (int i = 1; i < argc; i++)
        {
            uint32_t arglen = strlen(argv[i]);
            strncpy(&cmdstr[cmdlen], argv[i], arglen);
            cmdlen += arglen;

            /* It's assumed that AT command length is checked by AT core */
            if (i + 1 < argc)
            {
                cmdstr[cmdlen++] = ',';
            }
        }
    }

    /* Add CR at the end of command */
    cmdstr[cmdlen++] = '\r';

    return rm_atcmd_w_da14xxx_proc_cmd_raw(p_at_ctrl, cmdstr, cmdlen);
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_ForceExitBinary (void * const p_ctrl)
{
    fsp_err_t err     = FSP_ERR_OVERFLOW;
    uint32_t  retries = 3;

    FSP_PARAMETER_NOT_USED(p_ctrl);

    /* TODO: find a way to know about actual transfer completion instead of delay */
    rm_atcmd_w_da14xxx_delay(400);

    g_da14xxx_atcmd.disable_output = true;

    while (retries-- > 0)
    {
        /* Send escape sequence */
        rm_atcmd_w_da14xxx_delay(g_da14xxx_atcmd.esc_seq.pre_delay);
        rm_atcmd_w_da14xxx_transmit(g_da14xxx_atcmd.esc_seq.seq.u8, RM_ATCMD_W_DA14XXX_BINMOD_ESCSEQ_SZ);
        rm_atcmd_w_da14xxx_delay(g_da14xxx_atcmd.esc_seq.post_delay);

        /* Imitate escape sequence from the host */
        da14xxx_binary_switch_exit_condition_to_wait_for_confirmation();

        /* Wait for binary mode exit confirmation from the DA14xxx */
        uint32_t timeout = (uint32_t) xTaskGetTickCount() + pdMS_TO_TICKS(RM_ATCMD_W_CORE_DA14XXX_BINACK_TIMEOUT_MS);
        do
        {
            /* Process response from the DA14xxx and check for exit confirmation */
            rm_atcmd_w_da14xxx_send_resp_if_avail(p_ctrl);
            rm_atcmd_w_da14xxx_delay(5);
        } while (timeout > xTaskGetTickCount() && g_da14xxx_atcmd.is_binary);

        if (!g_da14xxx_atcmd.is_binary)
        {
            err = FSP_SUCCESS;
            break;
        }
    }

    g_da14xxx_atcmd.disable_output = false;

    return err;
}

/* DA14XXX AT Command handlers */

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_HANDLER(cmd)                                                           \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(cmd)                                                                           \
    {                                                                                                               \
        fsp_err_atcmd_err_code rc = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT" # cmd, argc, (const char **) argv); \
        if (rc == FSP_ERR_AT_CMD_ERR_BLE_XXX)                                                                       \
        {                                                                                                           \
            rc = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;                                                                \
        }                                                                                                           \
        return rc;                                                                                                  \
    }                                                                                                               \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(cmd)                                                                    \
    {                                                                                                               \
        return "TODO: redirect to DA14xxx";                                                                         \
    }                                                                                                               \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(cmd)                                                                     \
    {                                                                                                               \
        return "TODO: redirect to DA14xxx";                                                                         \
    }

 #define RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(cmd)                                                                  \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(cmd)                                                                            \
    {                                                                                                                \
        fsp_err_atcmd_err_code rc = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT+" # cmd, argc, (const char **) argv); \
        if (rc == FSP_ERR_AT_CMD_ERR_BLE_XXX)                                                                        \
        {                                                                                                            \
            rc = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;                                                                 \
        }                                                                                                            \
        return rc;                                                                                                   \
    }                                                                                                                \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(cmd)                                                                     \
    {                                                                                                                \
        return "TODO: redirect to DA14xxx";                                                                          \
    }                                                                                                                \
    RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(cmd)                                                                      \
    {                                                                                                                \
        return "TODO: redirect to DA14xxx";                                                                          \
    }

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BASIC_HANDLER(I)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(BDADDR)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(PRINT)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(PIN)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(ADVSTART)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(ADVSTOP)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(ADVDATA)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(ADVRESP)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(CENTRAL)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(PERIPHERAL)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(GAPSTATUS)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(GAPSCAN)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(GAPCONNECT)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(GAPDISCONNECT)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(CLRBNDE)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(CHGBNDP)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(IEBNDE)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(RSSI)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(SEC)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(ENPRMD)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(DISPRMD)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(GETPRMD)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(BINREQ)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(BINREQEXIT)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(BINREQEXITACK)
RM_ATCMD_W_CORE_DA14XXX_ATCMD_HANDLER(DEVNAME)

/* Reset command requires special processing */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(R)
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    /* Activate command parser */
    da14xxx_atcmd_set_parser(da14xxx_response_parse_rx_char);

    /* Send reset command to DA14XXX */
    rm_atcmd_w_da14xxx_transmit((uint8_t *) RM_ATCMD_W_DA14XXX_ATCMD_REBOOT,
                                sizeof(RM_ATCMD_W_DA14XXX_ATCMD_REBOOT) - 1);
    rm_atcmd_w_da14xxx_delay(3);

    /* Load codeless image */
    return rm_atcmd_w_da14xxx_reboot(p_at_ctrl);
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(R)
{
    return "ATR";
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(R)
{
    return "ATR";
}

/* Binary mode support */

/* AT+BINRECKACK - need to track it to know when to enter binary mode */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(BINREQACK)
{
    /* Disable echo in order to easily detect exit condition */

    /* Send it as a regular command, but on successfull completion switch to binary mode */
    fsp_err_atcmd_err_code err = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT+BINREQACK", argc, (const char **) argv);
    if (FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT == err)
    {
        g_da14xxx_atcmd.is_binary = true;
    }
    else if (FSP_ERR_AT_CMD_ERR_BLE_XXX == err)
    {
        err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

    return err;
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(BINREQACK)
{
    return "AT+BINREQACK";
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(BINREQACK)
{
    return "AT+BINREQACK";
}

/* AT+BINESC - need to track it to know which escape sequence to use */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(BINESC)
{
    /* Send it a a regular command, but also store params here to sniff binary traffic */
    fsp_err_atcmd_err_code err = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT+BINESC", argc, (const char **) argv);
    if (FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT == err)
    {
        /* Command executed successfully at DA14XXX: don't need to validate arguments */
        g_da14xxx_atcmd.esc_seq.seq.u32    = atoi(argv[2]);
        g_da14xxx_atcmd.esc_seq.pre_delay  = atoi(argv[1]);
        g_da14xxx_atcmd.esc_seq.post_delay = atoi(argv[3]);
    }
    else if (FSP_ERR_AT_CMD_ERR_BLE_XXX == err)
    {
        err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

    return err;
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(BINESC)
{
    return "AT+BINESC";
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(BINESC)
{
    return "AT+BINESC";
}

/* AT+BINRESUME - need to check command execution status to switch to binary mode */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(BINRESUME)
{
    /* Send it a a regular command, but also store params here to sniff binary traffic */
    fsp_err_atcmd_err_code err = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT+BINRESUME", argc, (const char **) argv);
    if (FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT == err)
    {
        /* Command executed successfully, activate binary mode */
        g_da14xxx_atcmd.is_binary = true;
    }
    else if (FSP_ERR_AT_CMD_ERR_BLE_XXX == err)
    {
        err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

    return err;
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(BINRESUME)
{
    return "AT+BINRESUME";
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(BINRESUME)
{
    return "AT+BINRESUME";
}

/* AT+SLEEP - should be handled in different way */
RM_ATCMD_W_CORE_DA14XXX_ATCMD_CB(SLEEP)
{
    /* Send it a a regular command and set appropriate state in order to resume later */
    fsp_err_atcmd_err_code err = rm_atcmd_w_da14xxx_proc_cmd(p_at_ctrl, "AT+SLEEP", argc, (const char **) argv);
    if (FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT == err)
    {
        /* Convert sleep mode to integer and store it */
        sleep_mode_t sleep_mode = argv[1][0] - '0';
        if ((sleep_mode > SLEEP_MODE_NO_SLEEP) && (sleep_mode < SLEEP_MODE_INVAL))
        {
            fsp_err_t rc;
            rc = R_UART_W_Close(&g_uart_ctrl);
            if (sleep_mode == SLEEP_MODE_EXTENDED_SLEEP)
            {
                rm_atcmd_w_da14xxx_prepare_hw_wake_up();
            }

            if (rc != FSP_SUCCESS)
            {
                return FSP_ERR_AT_CMD_ERR_UART_INTERFACE;
            }

 #if (ATCMD_PMGR_SUPPORT_ENABLE == 1)
            if (sleep_mode >= SLEEP_MODE_DEEP_SLEEP)
            {
                rc = PMGR_REMOVE_SLEEP_CONSTRAINT();
            }

            if (rc != FSP_SUCCESS)
            {
                return FSP_ERR_AT_CMD_ERR_UNKNOWN;
            }
 #endif
            g_da14xxx_atcmd.sleep_mode = sleep_mode;
        }
    }
    else if (FSP_ERR_AT_CMD_ERR_BLE_XXX == err)
    {
        err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

    return err;
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_FORMAT_CB(SLEEP)
{
    return "AT+SLEEP";
}

RM_ATCMD_W_CORE_DA14XXX_ATCMD_BRIEF_CB(SLEEP)
{
    return "AT+SLEEP";
}

bool RM_ATCMD_W_CORE_DA14xxx_IsBinaryMode (void * const p_at_ctrl)
{
    FSP_PARAMETER_NOT_USED(p_at_ctrl);

    return g_da14xxx_atcmd.is_binary;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_BinaryWrite (void * const p_ctrl, const uint8_t * const data, uint32_t size)
{
    fsp_err_t err;
    FSP_PARAMETER_NOT_USED(p_ctrl);

    err = rm_atcmd_w_da14xxx_transmit(data, size);
    if (err != FSP_SUCCESS)
    {
        return err;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        if (da14xxx_binary_check_exit_condition(data[i], true))
        {
            g_da14xxx_atcmd.is_binary = false;

            return FSP_SUCCESS;
        }
    }

    return FSP_SUCCESS;
}

fsp_err_t RM_ATCMD_W_CORE_DA14xxx_BinaryRead (void * const p_ctrl)
{
    return rm_atcmd_w_da14xxx_send_resp_if_avail(p_ctrl);
}

/* All remote commands are bypassed without special preprocessing */
fsp_err_atcmd_err_code RM_ATCMD_W_CORE_DA14xxx_ProcRemoteCmd (atcmd_w_ctrl_t * const p_at_ctrl,
                                                              const char           * cmd,
                                                              uint32_t               size)
{
    fsp_err_atcmd_err_code err = rm_atcmd_w_da14xxx_proc_cmd_raw(p_at_ctrl, cmd, size);
    if (FSP_ERR_AT_CMD_ERR_BLE_XXX == err)
    {
        err = FSP_ERR_AT_CMD_ERR_CMD_OK_WO_PRINT;
    }

    return err;
}

#endif                                 /* ATCMD_DA14XXX_CODELESS == 1 */
