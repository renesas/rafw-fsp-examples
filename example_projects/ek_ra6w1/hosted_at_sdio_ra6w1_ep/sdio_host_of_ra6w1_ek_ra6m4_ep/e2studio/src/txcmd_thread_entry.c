/***********************************************************************************************************************
* File Name    : txcmd_thread_entry.c
* Description  : TxCmd thread entry function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

#include "txcmd_thread.h"
#include "common_support.h"
#include "common_init.h"
#include "usb_console.h"
#include "at_cmd.h"

#define MAX_MSG_SIZE            MAX_BUF_SIZE

#if (SUPPORT_SPI == 1)
#if (CHIP_RRQ61x == 1)
#define WRITE_REQ_HEADER_SIZE           4
#else
#define WRITE_REQ_HEADER_SIZE           8
#endif
#elif (SUPPORT_UART == 1) || (SUPPORT_SDIO == 1)
#define WRITE_REQ_HEADER_SIZE           0
#endif

#if (USE_UART_PRINTF == 1)
#define UART_BUFFER_SIZE        1024
#define MAX_RX_UARTQ_SIZE       200

volatile bool transmitComplete;
static uint8_t rxdata = 0;

//queue
uint32_t rxUartQfront=0;
uint32_t rxUartQrear=0;
uint8_t rxUartDataQ[MAX_RX_UARTQ_SIZE];

void uart0_print_init(void)
{
    R_SCI_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    setvbuf(stdout, NULL, _IONBF, UART_BUFFER_SIZE);
}

int _write(int file, char * buffer, int count)
{
    (void)file;
    int tx_count;
    int i;

    for (i = 0, tx_count = 0; i < count; i++, tx_count++) {
        transmitComplete = false;
        R_SCI_UART_Write(&g_uart0_ctrl, (uint8_t const *)(buffer + i), 1);
        while (!transmitComplete){};
    }

    return tx_count;
}

void user_uart0_callback(uart_callback_args_t *p_args)
{
    uint32_t tmp;

    switch(p_args->event) {
        case UART_EVENT_TX_COMPLETE:
            transmitComplete = true;
            break;

        case UART_EVENT_RX_CHAR:
            rxdata = (uint8_t)p_args->data;
            if ((rxUartQrear + 1) % MAX_RX_UARTQ_SIZE == rxUartQfront) {
                printf("queue overflow \n");
            }

            rxUartDataQ[rxUartQrear] = rxdata;

            tmp = ++rxUartQrear;
            rxUartQrear = tmp % MAX_RX_UARTQ_SIZE;
            break;
        default:
        break;
    }
}


/* TxCmd Thread entry function */
/* pvParameters contains TaskHandle_t */
void txcmd_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    uint32_t tmp;

    int argc = 0;
    char *argv[10];
    char rxIndex = 0;
    char RxCmdBuffer[MAX_MSG_SIZE];

    uart0_print_init();

    while (1) {
        if (rxUartQfront == rxUartQrear) {
            //(" Empty Queue \n");
        } else {
            //Clear Rx_Buffer before receiving new data
            if (rxIndex == 0) {
                for (int i = 0; i < MAX_MSG_SIZE; i++)
                RxCmdBuffer[i] = 0;
            }

            // if \r\n ==> pass
            if (!(rxUartDataQ[rxUartQfront] == '\r' || rxUartDataQ[rxUartQfront] == '\n' || rxUartDataQ[rxUartQfront] == '\b' || rxUartDataQ[rxUartQfront] == 27 || rxUartDataQ[rxUartQfront] == 91))
                RxCmdBuffer[rxIndex] = rxUartDataQ[rxUartQfront];

            char temp[2] = "";

            temp[0] = rxUartDataQ[rxUartQfront];

            for (int i = 0; i < strlen(temp); i++) {
                printf("%c", temp[i]);
            }

            char del[3] = {' ', '\b', 0};

            if (rxUartDataQ[rxUartQfront] == '\b') {
                if (rxIndex > 0) {
                    RxCmdBuffer[rxIndex] = '\0';
                    rxIndex--;
                    for (int i = 0; i < strlen(del); i++) {
                        printf("%c", del[i]);
                    }
                }
            } else {
                rxIndex++;
            }

            if (rxIndex > MAX_MSG_SIZE) {
                rxIndex = 0;
                printf("Buffer is Full!!! Clear Buffer\r\n");
            }

            if (rxUartDataQ[rxUartQfront] == '\r' || rxUartDataQ[rxUartQfront] == '\n') {
                rxIndex = 0;

                RemoveCRLF(RxCmdBuffer);

                if (strlen(RxCmdBuffer) > 0) {
                    argc = make_argv(RxCmdBuffer, sizeof(argv) / sizeof(argv[0]), argv);
                    if (argc != 0) {
                        AnalyzeCommand(argc, argv);
                    }
                }
            }

            tmp = ++rxUartQfront;
            rxUartQfront = tmp % MAX_RX_UARTQ_SIZE; 
        }
        vTaskDelay(10);
    }
}
//uint32_t fw_update_on;
#else
extern bool b_usb_configured;
extern uint8_t s_response[];
extern fsp_err_t print_to_console(char *p_data);
extern int check_end(char *buf, uint32_t offset);

/* TxCmd Thread entry function */
/* pvParameters contains TaskHandle_t */
char RxCmdBuffer[MAX_MSG_SIZE];
uint32_t fw_update_on;
#define STR_TRSSLCERTSTORE "AT+TRSSLCERTSTORE"
#define STR_TRSSLWR "AT+TRSSLWR"
#define STR_MATTER "matter"
#define REMOTE_ECHO_ENABLE (1)

int check_end(char *buf, uint32_t offset)
{
    if ((offset >= 2) && (buf[0] == 0x1b) && ((buf[1] == 'c') || (buf[1] == 'C'))) {
        if  (buf[offset - 1] == 0x03) {
            //PRINTF("\r\n[\r\nESC: (%d) %s\r\n]\r\n", strlen(buf), buf);
            buf[offset - 1] = 0x00;
            return ATCMD_ESC;
        } else {
            return ATCMD_NONE;
        }
    }

    if (buf[0] == 0x1b) {
        if (buf[offset - 1] == '\r' || buf[offset - 1] == '\n') {
            //PRINTF("\r\n[\r\nESC: (%d) %s\r\n]\r\n", strlen(buf), buf);
            buf[offset - 1] = 0x00;
            return ATCMD_ESC;
        } else {
            return ATCMD_NONE;
        }
    }

    if ((offset >= strlen(STR_TRSSLCERTSTORE))
        && (strncasecmp(buf, STR_TRSSLCERTSTORE, strlen(STR_TRSSLCERTSTORE)) == 0)) {
        if  (buf[offset - 1] == 0x03) {
            //PRINTF("\r\n[\r\nATCMD_TRSSLCERTSTORE: (%d) %s\r\n]\r\n", strlen(buf), buf);
            buf[offset - 1] = 0x00;
            return ATCMD_TRSSLCERTSTORE;
        } else {
            return ATCMD_NONE;
        }
    }

    if ((offset >= strlen(STR_TRSSLWR))
        && (strncasecmp(buf, STR_TRSSLWR, strlen(STR_TRSSLWR)) == 0)) {
        if  (buf[offset - 1] == 0x03) {
            //PRINTF("\r\n[\r\nATCMD_TRSSLWR: (%d) %s\r\n]\r\n", strlen(buf), buf);
            buf[offset - 1] = 0x00;
            return ATCMD_TRSSLWR;
        } else {
            return ATCMD_NONE;
        }
    }

    if ((offset >= strlen(STR_MATTER))
        && (strncasecmp(buf, STR_MATTER, strlen(STR_MATTER)) == 0)) {
        if (buf[offset - 1] == '\r' || buf[offset - 1] == '\n') {
            buf[offset - 1] = 0x00;
            return ATCMD_MATTER;
        } else {
            return ATCMD_NONE;
        }
    }

    if (buf[offset - 1] == '\r' || buf[offset - 1] == '\n')
        return ATCMD_COMMON;
    return ATCMD_NONE;
}

void txcmd_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    EventBits_t uxBits;
    char revPacket[2];
    int argc = 0;
    int cnt = 0;

    uint32_t rxIndex = 0;
    char *argv[10];

    while (1) {
        if (true == b_usb_configured) {
            uxBits = xEventGroupWaitBits(g_update_console_event, STATUS_CONSOLE_RX_EVENT, pdTRUE, pdFALSE, portMAX_DELAY);
            if ((uxBits & (STATUS_CONSOLE_RX_EVENT)) == (STATUS_CONSOLE_RX_EVENT)) {
                if (fw_update_on) {
                    if (((uint32_t)strlen((const char *)s_response)) != 0)  {
                        uint32_t len = 0;
                        char *ptr = NULL;
                        char *stop = NULL;

                        if ((strncasecmp((char *)s_response, "AT+TX_SIZE=", 11) == 0) ||
                            (strncasecmp((char *)s_response, "TX_SIZE=", 8) == 0)) {
                            ptr = (char *)strstr((char *)s_response, "TX_SIZE=");
                            ptr += 8;
                            len = (uint32_t)strtol(ptr, &stop, 10);
                            len += (uint32_t)(13 + (stop - ptr));
                            if (strncasecmp((char *)s_response, "TX_SIZE=", 8) == 0)
                                len -= 3;
                            if (s_response[len - 1] == '\r' || s_response[len - 1] == '\n') {
                                s_response[len - 1] = 0;
                                memcpy((char *)(RxCmdBuffer + WRITE_REQ_HEADER_SIZE), s_response, len);
                                PRINTF_ATCMD_PUT(RxCmdBuffer, len);
                                s_response[0] = 0;
                            }
                        } else {
                            memcpy(RxCmdBuffer, s_response, strlen((char *)s_response));
                            RemoveCRLF(RxCmdBuffer);
                            if (strlen(RxCmdBuffer) > 0) {
                                argc = make_argv(RxCmdBuffer, sizeof(argv) / sizeof(argv[0]), argv);
                                if (argc != 0) {
                                    AnalyzeCommand(argc, argv);
                                }
                            }
                        }
                    }
                } else {
                    memset(revPacket, 0x00, 2);
                    if (rxIndex == 0) {
                        memset(RxCmdBuffer, 0x00, MAX_MSG_SIZE);
                    }
                    cnt = 0;
                    while (1) {
                        if (s_response[cnt] == 0) {
                            s_response[0] = 0;
                            break;
                        }
                        if (s_response[cnt] != '\b') {
                            revPacket[0] = RxCmdBuffer[rxIndex] = (char)s_response[cnt];
                        }
#if (REMOTE_ECHO_ENABLE == 1)
                        PRINTF("%s", revPacket);
                        char del[4] = {'\b',' ','\b', 0};
#endif
                        if (s_response[cnt] == '\b') {
                            if (rxIndex > 0) {
                                RxCmdBuffer[rxIndex] = '\0';
                                rxIndex--;
#if (REMOTE_ECHO_ENABLE == 1)
                                PRINTF("%s", del);
#else
                                print_to_console(" \b");
#endif
                            }
                        } else {
                            rxIndex++;
                        }
                        cnt++;
                    }

                    if (rxIndex > MAX_MSG_SIZE) {
                        rxIndex = 0;
                        print_to_console("Buffer is Full!!! Clear Buffer\r\n");
                    }

                    switch (check_end(RxCmdBuffer, rxIndex))
                    {
                        case ATCMD_NONE:
                        {
                            break;
                        }
                        case ATCMD_COMMON:
                        {
                            rxIndex = 0;
                            //RemoveCRLF(RxCmdBuffer);
                            argc = make_argv(RxCmdBuffer, sizeof(argv) / sizeof(argv[0]), argv);
                            if (argc != 0) {
                                AnalyzeCommand(argc, argv);
                            }
                            break;
                        }
                        case ATCMD_ESC:
                        case ATCMD_TRSSLWR:
                        case ATCMD_TRSSLCERTSTORE:
                        case ATCMD_MATTER:
                        {
                            rxIndex = 0;
                            argc = 1;
                            argv[0] = RxCmdBuffer;
                            if (argc != 0) {
                                AnalyzeCommand(argc, argv);
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }
        } else {
            vTaskDelay(1);
        }
    }
}
#endif
