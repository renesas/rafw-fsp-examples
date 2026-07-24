/***********************************************************************************************************************
* File Name    : sdio_cmd.c
* Description  : SDIO command processing functions.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#if (SUPPORT_SDIO == 1)
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "common_utils.h"
#include "sdio_cmd.h"
#include "sdio_drv.h"

#include "FreeRTOS.h"
#include "semphr.h"
//#include "SEGGER_RTT.h"
#include "at_cmd.h"

#define DATA_ALLIGNED_SIZE              4
/* Max wait count for time-out operation */
#define MAX_COUNT 10000 // estimated 10ms
/* Min wait count for time-out operation */
#define MIN_COUNT (0)

#define ATREF_ADDR                      (0x50080264)
#define RESP_ADDR                       (0x50080258)
#define READ_RES_TOTAL_LENGTH           8  // READ_RES_INFO_SIZE
#define ESC_BUFF_OK                     (0x00000000)
#define MAX_ESC_CMD_LEN                 (4096)
#define SDIO_TX_PRIO                    (4)
#define SDIO_LOOP_TIMEOUT               (20000000)
#define WAIT_READ_RESP_DELAY_US         (60)

typedef struct {
    uint8_t data[MAX_BUF_SIZE] BSP_ALIGN_VARIABLE(4);
    uint32_t size;
} SdioTxPacket;

typedef struct {
    uint32_t pkt_size;
    uint32_t pkt_count;
    char mode[2]; // "M1" or "M2"
} SdioTxJob;

extern uint32_t g_app_at_start;
extern uint32_t fw_update_on;
extern uint32_t g_sdio_reinitialize_needs;
extern MODE_FLAG mode_flag;

static uint8_t tx_ready = 0;

static char gWBuffer_Space[MAX_BUF_SIZE] BSP_ALIGN_VARIABLE(4) = {0};
static char gWBuffer[MAX_BUF_SIZE] BSP_ALIGN_VARIABLE(4) = {0};
static char gRBuffer[MAX_BUF_SIZE] BSP_ALIGN_VARIABLE(4) = {0};

static uint32_t at_addr = 0;
static uint32_t pre_recv_len = 0;

static QueueHandle_t sdioTxJobQueue;
static SemaphoreHandle_t escRespSemaphore;
static SdioTxPacket gs_packet;

/* Master Transfer Event completion flag */
static volatile spi_event_t g_master_event_flag;
static volatile uint32_t g_wait_count = MAX_COUNT;

void SdioTxWorkerTask(void *pvParameters);

void SDIO_cmd_init(void)
{
    thr_test_rdy = 0;
    memset(&gs_packet, 0x00, sizeof(SdioTxPacket));
    sdioTxJobQueue = xQueueCreate(5, sizeof(SdioTxJob));

    escRespSemaphore = xSemaphoreCreateBinary();
    if (escRespSemaphore == NULL) {
        PRINTF("Failed to create ESC response semaphore\r\n");
        return;
    }

    xSemaphoreTake(escRespSemaphore, 0);

    xTaskCreate(SdioTxWorkerTask, "SdioTxWorker", 2048, NULL, tskIDLE_PRIORITY + SDIO_TX_PRIO, NULL);
}

static fsp_err_t SDIO_Write_To_DA16xxx_Throughput(uint8_t *buf, uint32_t len)
{
#if DEBUG_XTRA
    PRINTF("SDIO_Write_To_DA16xxx_Throughput\r\n");
#endif
    fsp_err_t ret = FSP_SUCCESS;

    ret = sdio_write_ext((uint8_t *)buf, at_addr, len);
    if (FSP_SUCCESS != ret)
        PRINTF("sdio_write_ext failed\r\n");
    return ret;
}

fsp_err_t SDIO_Write_To_DA16xxx(uint8_t *buf, uint32_t len)
{
    //uint8_t *buf2 = "AT+SDKVER\r\n";
#if DEBUG_XTRA
    PRINTF("SDIO_Write_To_DA16xxx\r\n");
#endif
    //len = 11;
    fsp_err_t ret = FSP_SUCCESS;
    //PRINTF("AT REF wADDR : 0x%x\r\n", at_addr);
#if 0
    if (at_addr == 0) {
        ret = sdio_read_ext_1((uint8_t *)&at_addr, ATREF_ADDR, 4);
        if (ret) {
            PRINTF("Failed Getting at address_ 0x%x %s\r\n", ret, buf);
            return ret;
        }
        PRINTF("AT REF wADDR : 0x%x\r\n", at_addr);
    }
#endif
    if (buf != (uint8_t *)gWBuffer) {
        memset(gWBuffer, '\0', sizeof(gWBuffer));
        memcpy(gWBuffer, buf, len);
#if 0
        gWBuffer[len++] = '\r';
        gWBuffer[len++] = '\n';
#endif
        uint32_t alignedsize = (len + 3U) & ~3U;
        memset(&gWBuffer[len], '\0', (alignedsize - len));
    }
    if (strncmp((char *)&gWBuffer[0], "AT", 2) == 0 ||
            strncmp((char *)&gWBuffer[0], "at", 2) == 0) {
        if ((len % DATA_ALLIGNED_SIZE) == 0)
            len += DATA_ALLIGNED_SIZE;
        len = ((len + DATA_ALLIGNED_SIZE - 1) / DATA_ALLIGNED_SIZE) * DATA_ALLIGNED_SIZE;
    }

#if DEBUG_XTRA
    PRINTF("\r\ngWBuffer :%s len: %d\r\n",gWBuffer,len);
#endif
    ret = sdio_write_ext((uint8_t *)gWBuffer, at_addr, len);
    if (FSP_SUCCESS != ret)
        PRINTF("sdio_write_ext failed\r\n");
    return ret;
}

fsp_err_t SDIO_Read_From_DA16xxx(uint8_t *buf, uint32_t len)
{
    fsp_err_t ret = FSP_SUCCESS;
    if (at_addr == 0) {
        ret = sdio_read_ext((uint8_t *)&at_addr, ATREF_ADDR, 4);
        if (ret) {
            PRINTF("Failed Getting at address 0x%x\r\n", ret);
            return ret;
        }
        PRINTF("AT REF rADDR : 0x%x\r\n", at_addr);
    }

    ret = sdio_read_ext(buf, at_addr, len);

    return ret;
}

#define READ_RES_HEADER_SIZE            8
fsp_err_t Received_SDIO_Data_From_DA16xxx(void)
{
    fsp_err_t ret = FSP_SUCCESS;
    uint32_t resp_addr = 0;
    uint16_t resp_length = 0;
    uint8_t revRequesetInfo[READ_RES_TOTAL_LENGTH] = {0};

    ret = sdio_read_ext(revRequesetInfo, RESP_ADDR, READ_RES_TOTAL_LENGTH);
    if (ret != FSP_SUCCESS) {
#if DEBUG_XTRA
        PRINTF("\r\n %s:sdio_read_ext failed\r\n", __func__);
#endif
        return FSP_ERR_INTERNAL;
    }

#if 0
    resp_addr = *(uint32_t *)revRequesetInfo;
    resp_length = (*((uint32_t *)revRequesetInfo + 1)) & 0xffff;
#endif
#if 0
    PRINTF("h:%02x %02x %02x %02x %02x %02x %02x %02x \r\n", revRequesetInfo[0],
                                                             revRequesetInfo[1],
                                                             revRequesetInfo[2],
                                                             revRequesetInfo[3],
                                                             revRequesetInfo[4],
                                                             revRequesetInfo[5],
                                                             revRequesetInfo[6],
                                                             revRequesetInfo[7]);
#endif

#if 1
    resp_addr = (uint32_t)(revRequesetInfo[0]
                                      | (revRequesetInfo[1] << 8)
                                      | (revRequesetInfo[2] << 16)
                                      | (revRequesetInfo[3] << 24));
    resp_length = (uint16_t)(revRequesetInfo[4]
                                      | (revRequesetInfo[5] << 8));
#endif
#if DEBUG_XTRA
    PRINTF("\r\n resp_addr:%d resp_length:%d resp code :%d \r\n", resp_addr, resp_length, revRequesetInfo[6]);
#endif
    if (resp_addr == 0xffffffff && revRequesetInfo[6] == 0x20) {
        PRINTF("resp_addr = 0xffffffff\r\n");
        xSemaphoreGive(escRespSemaphore);
        return ret;
    }

    R_BSP_SoftwareDelay(WAIT_READ_RESP_DELAY_US, BSP_DELAY_UNITS_MICROSECONDS);
#if 0
    ret = sdio_read_ext((uint8_t *)gRBuffer, resp_addr, resp_length);
    PRINTF("\r\n gRBuffer :%s \r\n", gRBuffer);
    if (ret != FSP_SUCCESS) {
        return FSP_ERR_INTERNAL;
    }
#endif

#if 1
   if(pre_recv_len > resp_length)
   {
       memset(&gRBuffer[resp_length], 0x00, pre_recv_len - resp_length);
   }
#else
    memset(gRBuffer, 0x00, sizeof(gRBuffer));
#endif

    pre_recv_len = resp_length;

    ret = sdio_read_ext((uint8_t *)gRBuffer, 0, resp_length);
#if DEBUG_XTRA
       PRINTF("\r\n gRBuffer :%s \r\n", gRBuffer);
#endif
    //PRINTF("\r\n gRBuffer :%s resp_length: %d \r\n", gRBuffer, resp_length);
    //PRINTF("\r\n recv d \r\n");

    if (strstr (gRBuffer, "OK") && tx_ready == 1)
    {
        xSemaphoreGive(escRespSemaphore);
        //PRINTF("tx OK \r\n");
        return ret;
    }


    if (strstr (gRBuffer, "+PMGR:1"))
    {
        PRINTF("RA6Wx device went to sleep. Needs SDIO reinitialisation \r\n");
        g_sdio_reinitialize_needs = 1;
    }

#if 1
    if (resp_addr != ESC_BUFF_OK && tx_ready == 1) {
        xSemaphoreGive(escRespSemaphore);
        return ret;
    }
#endif

    if (ret != FSP_SUCCESS)
    {
        return FSP_ERR_INTERNAL;
    }
    if (thr_test_rdy)
    {
        if (revTotalLen == 0)
            ath_timer_reset();
        revTotalLen += resp_length;
        last_tick = xTaskGetTickCount();
    } else if (g_app_at_start) {
        command_parser((char *)gRBuffer);
    } else {
        //command_parser((char *)gSpirBuffer);
        gRBuffer[resp_length] = 0;
        if (resp_length < MAX_BUF_SIZE) {
            PRINTF("%s\r\n", gRBuffer);
        }
        else
            PRINTF("\r\nAT-CMD Res Size Err *** %d %s\r\n", resp_length,gRBuffer);
    }
    return ret;
}

unsigned char toint(char c)
{
    if ( c >= '0' && c <= '9' ) return(unsigned char)(c-'0');
    else if ( c >= 'a' && c <= 'f' ) return(unsigned char)(c-'a'+10);
    else if ( c >= 'A' && c <= 'F' ) return(unsigned char)(c-'A'+10);
    else return(0);
}

unsigned int htoi(char *s)
{
    unsigned int sum = 0;

    while (*s)
    {
        sum = sum * 16 + toint(*s++);
    }
    return(sum);
}


static inline uint32_t to_be32(uint32_t x)
{
    return ((x & 0x000000FFU) << 24) |
           ((x & 0x0000FF00U) << 8)  |
           ((x & 0x00FF0000U) >> 8)  |
           ((x & 0xFF000000U) >> 24);
}


void SdioTxWorkerTask(void *pvParameters)
{
    (void) pvParameters;

    SdioTxJob job;

    while (1) {
        if (xQueueReceive(sdioTxJobQueue, &job, portMAX_DELAY) == pdPASS) {
            TickType_t start_tick_sdio = xTaskGetTickCount();

            uint32_t total_kbytes_sent = 0;
            uint32_t bytes_remainder = 0;

            uint8_t data = 0x30;
            uint32_t pkt_count = job.pkt_count;
            char *payload = (char *) gs_packet.data;
            payload[0] = 0x1b;
            uint32_t seq_counter = 1;

            if(strncmp(job.mode, "M1", 2) == 0)
            {
                sprintf(&payload[1], "%s,%lu,0,0,", job.mode, job.pkt_size);
            }
            else if (strncmp(job.mode, "M2", 2) == 0)
            {
                sprintf(&payload[1], "%s,%lu,0,0,", job.mode, job.pkt_size);
            }

            uint32_t header_len = strlen(payload);
            memset(&payload[header_len], data, job.pkt_size);
            gs_packet.size = header_len + job.pkt_size;

            for (uint32_t i = 0; i < job.pkt_count; i++) {
                /* Add sequence number for iperf udp */
                if (job.pkt_size >= 4U) {
                    uint32_t seq_be = to_be32(seq_counter);
                    memcpy(&payload[header_len], &seq_be, sizeof(seq_be));
                    if(seq_counter == 0xFFFFFFFF)
                        seq_counter = 1;
                    else
                        seq_counter++;
                }

                uint32_t loop_timeout = SDIO_LOOP_TIMEOUT;
                tx_ready = 1;
                xSemaphoreTake(escRespSemaphore, 0);
                SDIO_Write_To_DA16xxx_Throughput(gs_packet.data, gs_packet.size);
                bytes_remainder += gs_packet.size;
                if (bytes_remainder >= 1024) {
                    total_kbytes_sent += bytes_remainder / 1024;
                    bytes_remainder = bytes_remainder % 1024;
                }

                if (xSemaphoreTake(escRespSemaphore, pdMS_TO_TICKS(20000)) != pdTRUE) {
                    PRINTF("\r\nESC timeout Error Exit\r\n");
                    tx_ready=0;
                    return;
                }

                tx_ready=0;
                pkt_count--;
                if (loop_timeout == 0) {
                    PRINTF("\r\nESC timeout Error Exit\r\n");
                    return;
                }
            }

            TickType_t spend = xTaskGetTickCount() - start_tick_sdio;
            uint32_t spend_ms = (spend * 1000) / configTICK_RATE_HZ;

            if (spend_ms == 0) {
                spend_ms = 1;
            }

            uint32_t kbytes_per_sec_whole = (total_kbytes_sent * 1000) / spend_ms;
            uint32_t kbytes_per_sec_frac = ((total_kbytes_sent * 1000) % spend_ms) * 100 / spend_ms;

            uint32_t kbps = kbytes_per_sec_whole * 8;
            uint32_t mbps_whole = kbps / 1000;
            uint32_t mbps_frac = (kbps % 1000) / 10;

            PRINTF("\r\n====================== TCP TX Report ======================\r\n");
            PRINTF(">>> Total Sent %d.%dKB\r\n", total_kbytes_sent, (bytes_remainder * 1000) / 1024);
            PRINTF(">>> Send Time %d.%d seconds\r\n", spend_ms / 1000, spend_ms % 1000);
            PRINTF(">>> Throughput %d.%dKB/s (%d.%d Mbps)\r\n",
                   kbytes_per_sec_whole, kbytes_per_sec_frac,
                   mbps_whole, mbps_frac);
            PRINTF("===========================================================\r\n");
        }
    }
}

void Transfer_ATCmd_To_SDIO(int argc, char *argv[])
{
    uint32_t cmdLen;

    if ((strcasecmp(argv[0], "atapp") == 0)) {
        g_app_at_start = 1;
    } else if (strncasecmp(argv[0], "esc", 3) == 0 && argc > 2) {
        // esc 4096(pkt_size) 1000(pkt_count)

        SdioTxJob job;
        job.pkt_size = (uint32_t)atoi(argv[1]);
        job.pkt_count = (uint32_t)atoi(argv[2]);

        if (strncasecmp(argv[0], "escu", 4) == 0) {
            strcpy(job.mode, "M2");
        } else {
            strcpy(job.mode, "M1");
        }

        if (xQueueSend(sdioTxJobQueue, &job, portMAX_DELAY) != pdPASS) {
            PRINTF("Failed to queue SDIO TX job\r\n");
            return;
        }

        PRINTF("\r\n============== TCP Send Throughput Test Start==============\r\n");
    } else if (strncasecmp(argv[0], "athstart", 8) == 0) {
        revTotalLen = 0;
        thr_test_rdy = 1;
        PRINTF("\r\n=============TCP Receive Throughput Test Start=============\r\n");
        PRINTF(">>>>> Ready to Receiving\r\n");
    } else if (strncasecmp(argv[0], "athstop", 6) == 0) {
        uint32_t spend = last_tick - start_tick;
        uint32_t bytepms = revTotalLen / spend;
        uint32_t byteps = bytepms * 1000;
        uint32_t kbytepsh = byteps / 1024;
        PRINTF("\r\n====================== TCP RX Report ======================\r\n");
        PRINTF(">>>>> Receiving Len %d.%dKB\r\n", revTotalLen / 1024, (revTotalLen % 1024) * 1000 / 1024);
        PRINTF(">>>>> Receiving Time %d.%d seconds\r\n", spend / 1000, spend % 1000);
        PRINTF(">>>>> Receiving THR %d.%dKB/S (%d.%d Mbps)\r\n"
            , kbytepsh, byteps % 1024
            , (kbytepsh * 8) / 1024 , (kbytepsh * 8) % 1024);
        PRINTF("===========================================================\r\n");
        thr_test_rdy = 0;
    } else if ((strncasecmp(argv[0], "atotamodeon", 11) == 0)) {
        if (g_app_at_start) {
            fw_update_on = 2;
            g_app_at_start = 0;
        } else {
            fw_update_on = 1;
        }
        //wake_up_wifi();
        //PRINTF_ATCMD("AT+CLRDPMSLPEXT\r\n");
        PRINTF("FWMODEON\r\n");
    } else if ((strncasecmp(argv[0], "atotamodeoff", 12) == 0)) {
        if (fw_update_on == 2) {
            g_app_at_start = 1;
        }
        fw_update_on = 0;
        //PRINTF_ATCMD("AT+SETDPMSLPEXT\r\n");
        vTaskDelay(100);
        PRINTF("FWMODEOFF\r\n");
    } else if ((strncasecmp(argv[0], "init", 4) == 0)) {
        /* get status */
        bool sdhi_init = false;
        sdhi_init = get_sdhi_init_status();
        if(false == sdhi_init)
        {
            init_drv_sdio();
        }
        else
        {
            PRINTF("SDHI has already been initialized.\r\n");
        }

    } else if ((strncasecmp(argv[0], "write", 5) == 0)) {
        //ATCMD_ADDR
        if (argc >= 3) {
            uint32_t len, addr;

            memset(gWBuffer, 0x00, sizeof(gWBuffer));
            for (unsigned int i = 0; i < sizeof(gWBuffer); i++) {
                gWBuffer[i] = (char)(i & 0xff);
            }

            len = (uint32_t)atoi(argv[1]);
            addr = (uint32_t)htoi(argv[2]);
            sdio_write_ext((uint8_t *)gWBuffer, addr, len);
        } else {
            PRINTF("Wrong Write Param.....[write len addr]\r\n");        
        }
    } else if ((strncasecmp(argv[0], "read", 4) == 0)) {
        if (argc >= 3) {
            uint32_t len, addr;

            memset(gRBuffer, 0x00, sizeof(gRBuffer));
            len = (uint32_t)atoi(argv[1]);
            addr = (uint32_t)htoi(argv[2]);
            sdio_read_ext((uint8_t *)gRBuffer, addr, len);
            PRINTF("SDIO Read : ");
            for (unsigned int i = 0; i < len; i++) {
                PRINTF(" [0x%x]", gRBuffer[i] & 0xff);
                if ((i > 0) && ((i % 16) == 0))
                    PRINTF("\r\n");
            }
            PRINTF("\r\n");
        } else {
            PRINTF("Wrong Read Param.....[read len addr]\r\n");        
        }
    } else {
        // AT+TRTS=5001 for RX
        // AT+TRTC=192.168.1.3,5001 for TX
        cmdLen = 0;
        memset(gWBuffer_Space, 0x00, sizeof(gWBuffer_Space));
        for (unsigned int i=0; i < (unsigned int)argc; i++) {
            strcat(gWBuffer_Space, argv[i]);
            if (i < ((unsigned int)argc -1))
                strcat(gWBuffer_Space, " ");
        }
        cmdLen = strlen(gWBuffer_Space);
        SDIO_Write_To_DA16xxx((uint8_t *)gWBuffer_Space, cmdLen);

        //PRINTF("\r\n[AT] %s\r\n", argv[0]);
    }

    return;
}
#endif
