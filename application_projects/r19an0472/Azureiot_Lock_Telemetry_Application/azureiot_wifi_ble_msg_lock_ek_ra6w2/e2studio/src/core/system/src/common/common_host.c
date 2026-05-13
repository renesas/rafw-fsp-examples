/**
 ****************************************************************************************
 *
 * @file common_host.c
 *
 * @brief Host interface initialize functions
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


//#include "sdk_type.h"


#if defined ( XIP_CACHE_BOOT )

#include <stdio.h>
#include <string.h>

#include "command_host.h"
#include "da16x_system.h"
#include "target.h"
#include "user_dpm_api.h"
#include "util_api.h"
#include "da16x_sys_watchdog.h"
#include "da16x_sys_watchdog_ids.h"

#if !defined ( __SUPPORT_IPERF__ )
#define __IPERF_NETWORK_POOL__    /* iperf_packet_pool */
#endif /* !__SUPPORT_IPERF__ */

#if !defined ( __SUPPORT_SIGMA_TEST__ )

#include "atcmd.h"

//----------------------------------------------------------
// Features
//----------------------------------------------------------

#undef    DA16X_CON_SAFETY_SUPPORT

//----------------------------------------------------------
//    Definitions
//----------------------------------------------------------

#define COMMON_HOST_MAX_CMD_LEN            256
#define COMMON_HOST_MAX_HIS_NUM            3

#define PRINT_HOST        PRINTF
//----------------------------------------------------------


#if defined ( DA16X_CON_SAFETY_SUPPORT )
static TX_SEMAPHORE          _mutex_host_instance;
#endif    // DA16X_CON_SAFETY_SUPPORT

#define HOST_ILLEGAL_CMD          (0xffffffff)

#define HOST_MEM_WRITE_REQ        (0x80)
#define HOST_MEM_WRITE_RES        (0x81)
#define HOST_MEM_READ_REQ         (0x82)
#define HOST_MEM_READ_RES         (0x83)
#define HOST_MEM_WRITE            (0x84)
#define HOST_MEM_WRITE_DONE       (0x85)
#define HOST_MEM_READ             (0x86)
#define HOST_MEM_READ_DONE        (0x87)

unsigned int    *host_data_buffer = NULL;
unsigned int    *host_data_buffer_allocated = NULL;

enum
{
    STATE_WAIT_G_WR_REQ = 0,
    STATE_WAIT_G_WR_RES,
    STATE_WAIT_G_WR_SET_LENGTH,
    STATE_WAIT_G_WR_DATA,
    STATE_WAIT_G_RD_REQ,
    STATE_WAIT_G_RD_RES,
    STATE_WAIT_G_RD_SET_LENGTH,
    STATE_WAIT_G_RD_DATA
};

unsigned int    state_slave_proc      = STATE_WAIT_G_WR_REQ;
unsigned int    state_slave_proc_next = STATE_WAIT_G_WR_RES;
unsigned int    spi_hdr_mode = 8;

unsigned int    b_test_mode = FALSE;
unsigned int    g_test_length = 0;
unsigned int    g_test_time_start = 0;;
unsigned int    g_test_time = 0;;
int g_test_total_len = 0;

typedef struct _st_host_request {
    unsigned short    host_write_length;
    unsigned char    host_cmd;
    unsigned char    dummy;
} st_host_request;

typedef struct _st_host_response {
    unsigned int    buf_address;
    unsigned short    host_length;
    unsigned char    resp;
    unsigned char    padding_bytes;
} st_host_response;
//----------------------------------------------------------

#if defined ( __SUPPORT_ATCMD__ )
extern int get_run_mode(void);
#endif    // __SUPPORT_ATCMD__

extern unsigned long sys_jiffies(void);
extern void user_host_interface_init(void);
extern void host_process_AT_command(UCHAR *buf);

extern int g_is_atcmd_init;
extern int init_done_sent;
#if defined (__DPM_TEST_WITHOUT_MCU__)
extern int atcmd_dpm_ready;
#endif // (__DPM_TEST_WITHOUT_MCU__)

void Host_thread(void *pvParameters);

//----------------------------------------------------------
//
//
//----------------------------------------------------------
TaskHandle_t host_drv_task_handle;

#define HOST_DRV_STACK_SZ        ( 1024 * 2 )
#define HOST_BUF_SIZE            ( 2048 + 128 )

static char    host_buf[HOST_BUF_SIZE] = {1,};

char      host_hist_buf[COMMON_HOST_MAX_HIS_NUM][COMMON_HOST_MAX_CMD_LEN] ;

extern HANDLE uart1;
#ifdef __ATCMD_IF_UART2__
extern HANDLE uart2;
#endif

#define COMMAND_INT_ENABLE       (0x1<<15)
#define AT_CMD_INT_ENABLE        (0x1<<14)
#define PROCESS_END_INT_ENABLE   (0x1<<13)
#define PROTOCOL_MODE            (0x1<<12)
#define SPI_SLAVE_RESET          (0x1<<11)
#define SPI_SLAVE_MD_MISO        (0x1<<10)
#define SPI_SLAVE_SPEED          (0x1<<9)
#define SPI_SLAVE_ENDIAN_MD      (0x1<<8)
#define SPI_SLAVE_CPOL           (0x1<<7)
#define SPI_SLAVE_CPHASE         (0x1<<6)
#define SPI_SLAVE_CHIP_ID        (4)
#define SPI_SLAVE_D_BUS_WIDTH    (2)
#define SPI_SLAVE_A_BUS_WIDTH    (0)

#define SPI_READ                 (0x40)
#define SPI_WRITE                (0x00)
#define SPI_AINC                 (0x80)
#define SPI_COMMON_ADDR_EN       (0x20)
#define SPI_REF_LEN_EN           (0x10)

#define SPI_SLAVE_HDR8           (0x1000)

#define SLAVE_INT_STATUS_CMD     (0x1 << 15)
#define SLAVE_INT_STATUS_ATCMD   (0x1 << 14)
#define SLAVE_INT_STATUS_END     (0x1 << 13)

#define EXT_INTR_CONF            0x50001020 //
#define EXT_INTR_SET             0x50001024 //

enum {
    WAIT_CMD = 0,
    WAIT_WRITE_DATA,
    WAIT_READ_DATA
};


#define GPIO_DS_MASK_4           (0x300)
#define GPIO_DS_MASK_5           (0xc00)
#define GPIO_DS_MASK_6           (0x3000)
#define GPIO_DS_MASK_7           (0xc000)
#define GPIO_DS_MASK_8           (0x30000)
#define GPIO_DS_MASK_9           (0xc0000)

#define GPIO_DS_4MA_4            (0x200)
#define GPIO_DS_4MA_5            (0x800)
#define GPIO_DS_4MA_6            (0x2000)
#define GPIO_DS_4MA_7            (0x8000)
#define GPIO_DS_4MA_8            (0x20000)
#define GPIO_DS_4MA_9            (0x80000)



unsigned int    hostif = HOSTIF_SPI;
unsigned int    *AT_Cmd_Addr = NULL;
unsigned int    slave_interrupt_acc = 0;

QueueHandle_t slave_rx_int_evt;
QueueHandle_t slave_rx_end_int_evt; //APP_CORE
EventGroupHandle_t slave_cmd_int_evt;

void host_spi_4byte_mode(void)
{
    unsigned int     slave_control;

    slave_control = DA16X_SLAVECTRL->SPI_Slave_CR;
    DA16X_SLAVECTRL->SPI_Slave_CR = slave_control & (~SPI_SLAVE_HDR8);
    spi_hdr_mode = 4;
}

void host_spi_8byte_mode(void)
{
    unsigned int     slave_control;

    slave_control = DA16X_SLAVECTRL->SPI_Slave_CR;
    DA16X_SLAVECTRL->SPI_Slave_CR = slave_control | SPI_SLAVE_HDR8;
    spi_hdr_mode = 8;
}

void host_spi_slave_init(void)
{
    DA16X_SLAVECTRL->SPI_Slave_CR = 0xFD0B;     // 8-byte mode
    DA16X_SLAVECTRL->Base_Addr = 0x00005008;    // base 0x5000xxxx
    DA16X_DIOCFG->EXT_INTB_CTRL =  0xb;
}

void host_i2c_slave_init(void)
{
    DA16X_SLAVECTRL->SPI_Slave_CR = 0xe50b;     // Disable Slave SPI
    DA16X_DIOCFG->EXT_INTB_CTRL =  0xa;
}

unsigned int slave_int_mask = ( SLAVE_INT_STATUS_ATCMD | SLAVE_INT_STATUS_CMD | SLAVE_INT_STATUS_END);
void (*host_rx_callback)(UINT32 status) = NULL;

int    host_register_write_callback(void (* p_rx_callback_func)(UINT32 status))
{
    host_rx_callback = p_rx_callback_func;

    return 0;
}

int host_set_buffer(void *buffer)
{
    host_data_buffer_allocated = (unsigned int *)buffer;

    return 0;
}


#if defined ( __SUPPORT_ATCMD__ )
int host_response_esc(unsigned int buf_addr, unsigned int len, unsigned int resp, unsigned int padding_bytes)
{
    unsigned int mask_evt ;
    st_host_response host_resp;

    host_resp.buf_address = (unsigned int)buf_addr;
    host_resp.host_length = len;
    host_resp.resp = resp;
    host_resp.padding_bytes = padding_bytes ;
    DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(buf_addr)) >>
                                16; //Set Addr.High to Base Address Register
    DA16X_SLAVECTRL->Resp_Addr1 = *((UINT32 *)&host_resp.buf_address);
    DA16X_SLAVECTRL->Resp_Addr2 = *(((UINT32 *)&host_resp)+1);

    DA16X_DIOCFG->EXT_INTB_SET = 0x1; // Trigger Host Int (GPIO    )

/*
    uxBits = xEventGroupWaitBits(
            xEventGroup,        : The event group being tested.
            BIT_0 | BIT_4,      : The bits within the event group to wait for.
            pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
            pdFALSE,            : Don't wait for both bits, either bit will do.
            xTicksToWait )
*/
    //APP_CORE
    {
        xQueueReset(slave_rx_end_int_evt);
        xQueueReceive(slave_rx_end_int_evt, &mask_evt, 20);
    }
    if (!(mask_evt & slave_int_mask) )
	{
            PRINTF("[%s] Slave Int. Timeout esc\n", __func__);
            return -1;
	}

    return 0;
}
#endif    //    __SUPPORT_ATCMD__

int host_response(unsigned int buf_addr, unsigned int len, unsigned int resp, unsigned int padding_bytes)
{
    //APP_CORE
    unsigned int mask_evt = 0;
    volatile st_host_response host_resp;

    host_resp.buf_address = (unsigned int)buf_addr;
    host_resp.host_length = len;
    host_resp.resp = resp;
    host_resp.padding_bytes = padding_bytes ;
    DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(buf_addr)) >> 16; //Set Addr.High to Base Address Register
    DA16X_SLAVECTRL->Resp_Addr1 = *((UINT32 *)&host_resp.buf_address);
    DA16X_SLAVECTRL->Resp_Addr2 = *(((UINT32 *)&host_resp)+1);

    DA16X_DIOCFG->EXT_INTB_SET = 0x1; // Trigger Host Int (GPIO    )

/*
    uxBits = xEventGroupWaitBits(
            xEventGroup,        : The event group being tested.
            BIT_0 | BIT_4,      : The bits within the event group to wait for.
            pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
            pdFALSE,            : Don't wait for both bits, either bit will do.
            xTicksToWait )
*/
    //APP_CORE
    {
        xQueueReset(slave_rx_end_int_evt);
        xQueueReceive(slave_rx_end_int_evt, &mask_evt, 20);
    }
        if (!(mask_evt & slave_int_mask) ) {
            PRINTF("[%s] Slave Int. Timeout #1\n", __func__);
            return -1;
    }


/*
    uxBits = xEventGroupWaitBits(
            xEventGroup,        : The event group being tested.
            BIT_0 | BIT_4,      : The bits within the event group to wait for.
            pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
            pdFALSE,            : Don't wait for both bits, either bit will do.
            xTicksToWait )
*/
    //APP_CORE
    xQueueReceive(slave_rx_end_int_evt, &mask_evt,20);
    if (!(mask_evt & slave_int_mask) ) {
        PRINTF("[%s] Slave Int. Timeout #2\n", __func__);
        return -1;
    }

    return 0;
}

void host_process_general_command(void)
{
    unsigned int data;

    volatile st_host_request *host_req;
    volatile st_host_response host_resp;

    data = DA16X_SLAVECTRL->Cmd_Addr;

    if (data == HOST_ILLEGAL_CMD) {
        PRINT_HOST("!!!!! Illegal Status!!!!\r\n");
        return;
    }

    host_req = (st_host_request *)(&data);
    DA16X_SLAVECTRL->Cmd_Addr = HOST_ILLEGAL_CMD; // Clear Command Register

    switch (host_req->host_cmd) {
    case HOST_MEM_WRITE_REQ:
        host_data_buffer = host_data_buffer_allocated;  //(unsigned int*)0xd1200000;

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = host_req->host_write_length;
            host_resp.resp = HOST_MEM_WRITE_RES;
            host_resp.padding_bytes = 0;

            //Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_WRITE_RES;
            host_resp.padding_bytes = 0;
        }

        DA16X_SLAVECTRL->Resp_Addr1 = *((unsigned int *)(&host_resp));
        DA16X_SLAVECTRL->Resp_Addr2 = *(((unsigned int *)(&host_resp)) + 1);
        DA16X_DIOCFG->EXT_INTB_SET = 0x1;    // Trigger Host Int (GPIO)

        state_slave_proc = STATE_WAIT_G_WR_RES;

        if (host_resp.host_length >= 0x1000 && (spi_hdr_mode == 4)) {
            state_slave_proc_next = STATE_WAIT_G_WR_SET_LENGTH;
        } else {
            state_slave_proc_next = STATE_WAIT_G_WR_DATA;
        }

        break;

    case HOST_MEM_READ_REQ:
        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = host_req->host_write_length;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.padding_bytes = 0;

            //Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.padding_bytes = 0;
        }

        DA16X_SLAVECTRL->Resp_Addr1 = *((unsigned int *)(&host_resp));
        DA16X_SLAVECTRL->Resp_Addr2 = *(((unsigned int *)(&host_resp)) + 1);
        DA16X_DIOCFG->EXT_INTB_SET = 0x1;    // Trigger Host Int (GPIO)

        state_slave_proc = STATE_WAIT_G_RD_RES;

        if (host_resp.host_length >= 0x1000) {
            state_slave_proc_next = STATE_WAIT_G_RD_SET_LENGTH;
        } else {
            state_slave_proc_next = STATE_WAIT_G_RD_DATA;
        }

        break;
    }
}

void host_process_general_command_uart(st_host_request *host_req)
{
    volatile st_host_response host_resp;

    switch (host_req->host_cmd) {
    case HOST_MEM_WRITE_REQ:
        host_data_buffer = (unsigned int *)0xd1200000; //APP_MALLOC(host_req->host_write_length);

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = host_req->host_write_length;
            host_resp.resp = HOST_MEM_WRITE_RES;
            host_resp.padding_bytes = 0;
        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_WRITE_RES;
            host_resp.padding_bytes = 0;
        }

        UART_WRITE(uart1, (void *)&host_resp, sizeof(st_host_response));
        state_slave_proc = STATE_WAIT_G_WR_RES;
        state_slave_proc_next = STATE_WAIT_G_WR_DATA;
        break;

    case HOST_MEM_READ_REQ:
        host_data_buffer = host_data_buffer_allocated;

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = host_req->host_write_length;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.padding_bytes = 0;
        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.padding_bytes = 0;
        }

        UART_WRITE(uart1, (void *)&host_resp, sizeof(st_host_response));
        UART_WRITE(uart1, host_data_buffer + (sizeof(st_host_request) / 4), host_req->host_write_length);

        state_slave_proc = STATE_WAIT_G_RD_RES;
        state_slave_proc_next = STATE_WAIT_G_RD_DATA;
        break;

    case HOST_MEM_WRITE:
        host_data_buffer = (unsigned int *)0xd1200000;

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_WRITE_DONE;
            host_resp.padding_bytes = 0;

            UART_READ(uart1, host_data_buffer + (sizeof(st_host_request) / 4), host_req->host_write_length);
        } else {
            host_resp.buf_address = 0xffffffff;
            host_resp.host_length = 0;
            host_resp.resp = HOST_MEM_WRITE_DONE;
            host_resp.padding_bytes = 0;
        }

        UART_WRITE(uart1, (void *)&host_resp, sizeof(st_host_response));
        state_slave_proc = STATE_WAIT_G_RD_RES;
        state_slave_proc_next = STATE_WAIT_G_RD_DATA;

        break;
    }
}

void host_process_data(void)
{
    switch (state_slave_proc) {
    case STATE_WAIT_G_WR_RES:
        state_slave_proc = state_slave_proc_next;

        if (state_slave_proc_next == STATE_WAIT_G_WR_DATA) {
            //Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        }

        break;

    case STATE_WAIT_G_WR_SET_LENGTH:
        state_slave_proc = STATE_WAIT_G_WR_DATA;

        //Set Addr.High to Base Address Register
        DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        break;

    case STATE_WAIT_G_WR_DATA:
        if (host_rx_callback != NULL) {
            host_rx_callback(0);
        }

        break;

    case STATE_WAIT_G_RD_RES:
        state_slave_proc = state_slave_proc_next;

        if (state_slave_proc_next == STATE_WAIT_G_RD_DATA) {
            //Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        }

        break;

    case STATE_WAIT_G_RD_SET_LENGTH:
        state_slave_proc = STATE_WAIT_G_RD_DATA;
        //Set Addr.High to Base Address Register
        DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        break;

    case STATE_WAIT_G_RD_DATA:
        // for RW loop test
        if (host_data_buffer != NULL) {
            host_data_buffer = NULL;
        }

        if (b_test_mode) {
            g_test_total_len -= g_test_length;

            if (g_test_total_len <= 0) {
                g_test_time = sys_jiffies() - g_test_time_start;
                PRINT_HOST("Total Read Time = %d ticks\r\n", g_test_time);
                g_test_time_start = 0;
            }

            state_slave_proc = STATE_WAIT_G_RD_RES;
        }

    break;

    default:
        break;
    }
}

void host_process_command(void)
{
    int sys_wdog_id = da16x_sys_wdog_id_get_Host_drv();
    unsigned int mask_evt ;
    volatile st_host_response host_resp;

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    printf_with_run_time("[host_process_command] Waiting for host cmd \n");
#endif  /* __RUNTIME_CALCULATION__ */

    FPGA_DBG_TIGGER(0x0000);

    if (b_test_mode) {
        host_data_buffer = (unsigned int *) host_data_buffer_allocated;

        if (host_data_buffer != NULL) {
            host_resp.buf_address = (unsigned int)host_data_buffer;
            host_resp.host_length = g_test_length;
            host_resp.resp = HOST_MEM_READ_RES;
            host_resp.padding_bytes = 0;
            memcpy((void *)DA16X_SLAVECTRL->Resp_Addr1, (void *)&host_resp, sizeof(st_host_response));

            //Set Addr.High to Base Address Register
            DA16X_SLAVECTRL->Base_Addr = ((unsigned int)(host_data_buffer)) >> 16;
        }

        if (   (state_slave_proc_next != STATE_WAIT_G_RD_SET_LENGTH )
            && (state_slave_proc_next != STATE_WAIT_G_RD_DATA )) {
            state_slave_proc = STATE_WAIT_G_RD_RES;

            if (g_test_length >= 0x1000) {
                DA16X_SLAVECTRL->Length = g_test_length;
                state_slave_proc_next = STATE_WAIT_G_RD_SET_LENGTH;
            } else {
                state_slave_proc_next = STATE_WAIT_G_RD_DATA;
            }
        }

        if (g_test_total_len > 0) {
            DA16X_DIOCFG->EXT_INTB_SET = 0x1;    // Trigger Host Int (GPIO)
        }

    /*
     *  uxBits = xEventGroupWaitBits(
     *                    xEventGroup,        : The event group being tested.
     *                    BIT_0 | BIT_4,      : The bits within the event group to wait for.
     *                    pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
     *                    pdFALSE,            : Don't wait for both bits, either bit will do.
     *                    xTicksToWait )
     */
		xQueueReset(slave_rx_int_evt);
		xQueueReceive(slave_rx_int_evt, &mask_evt,100);

        if (!(mask_evt & slave_int_mask) ) {
            //Todo : Fill Code Here for Managing Abnormal Cases
            PRINTF("[%s] host_thread time-out\n", __func__);
            return;
        }

        if (g_test_time_start == 0) {
            g_test_time_start = sys_jiffies();
        }
    } else {
        da16x_sys_watchdog_notify(sys_wdog_id);

        da16x_sys_watchdog_suspend(sys_wdog_id);

        /*
         * uxBits = xEventGroupWaitBits(
         *                xEventGroup,        : The event group being tested.
         *                 BIT_0 | BIT_4,      : The bits within the event group to wait for.
         *                 pdTRUE,             : BIT_0 & BIT_4 should be cleared before returning.
         *                 pdFALSE,            : Don't wait for both bits, either bit will do.
         *                 xTicksToWait )
         */
#ifdef    __ATCMD_IF_SPI__
        mask_evt = xEventGroupWaitBits(slave_cmd_int_evt, slave_int_mask, pdTRUE, pdFALSE, portMAX_DELAY);
#else
        xQueueReceive(slave_rx_int_evt, &mask_evt,portMAX_DELAY);
#endif

        da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

        if (!(mask_evt & slave_int_mask) ) {
            //Todo : Fill Code Here for Managing Abnormal Cases
            return;
        }
    }

#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
    printf_with_run_time("[host_process_command] #3 : EVT_OK");
#endif  /* __RUNTIME_CALCULATION__ */

    FPGA_DBG_TIGGER(0x2222);

    da16x_sys_watchdog_suspend(sys_wdog_id);

    if (mask_evt & SLAVE_INT_STATUS_ATCMD) {
        host_process_AT_command((UCHAR *)AT_Cmd_Addr);
    } else if (mask_evt & SLAVE_INT_STATUS_CMD) {
        host_process_general_command();

    } else if (mask_evt & SLAVE_INT_STATUS_END) {
        host_process_data();
    }

    da16x_sys_watchdog_notify_and_resume(sys_wdog_id);

    slave_interrupt_acc++;
}

void host_interrupt_handler(void)
{
    FPGA_DBG_TIGGER(0x1111);

    BaseType_t xHigherPriorityTaskWoken, xResult;

    xHigherPriorityTaskWoken = pdFALSE;

    if (DA16X_SLAVECTRL->SPI_Intr_Status & (SLAVE_INT_STATUS_ATCMD | SLAVE_INT_STATUS_CMD)) {
        xResult = xEventGroupSetBitsFromISR(slave_cmd_int_evt, (DA16X_SLAVECTRL->SPI_Intr_Status), &xHigherPriorityTaskWoken);
    }
    else {
        xResult = pdTRUE;
    	uint32_t mask_evt = DA16X_SLAVECTRL->SPI_Intr_Status;
    	xQueueSendFromISR(slave_rx_int_evt, (void *)&mask_evt, &xHigherPriorityTaskWoken);
    }
	DA16X_SLAVECTRL->SPI_Intr_Status |= ( SLAVE_INT_STATUS_ATCMD | SLAVE_INT_STATUS_CMD |
										 SLAVE_INT_STATUS_END); //mask all
    if (xResult != pdFAIL ) {
        /*
         * If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         * switch should be requested.  The macro used is port specific and will
         * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         * the documentation page for the port being used.
         */
         portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }

}

#define CMD_ADDR    0x50080254

void host_callback_handler(UINT32 address)
{
    unsigned int  status = 0;

    if (address == CMD_ADDR) {
        status = SLAVE_INT_STATUS_CMD;
    } else if (address == (UINT)AT_Cmd_Addr) {
#if defined(__RUNTIME_CALCULATION__) && defined(XIP_CACHE_BOOT)
        printf_with_run_time("[host_callback_handler] EVT_SET ATCMD");
#endif  /* __RUNTIME_CALCULATION__ */
        status = SLAVE_INT_STATUS_ATCMD;
    } else {
        status = SLAVE_INT_STATUS_END;
    }

    BaseType_t xHigherPriorityTaskWoken, xResult;

    xHigherPriorityTaskWoken = pdFALSE;

    xResult = pdTRUE;
    //APP_CORE
    {
       if (status == SLAVE_INT_STATUS_END)
           xResult  = xQueueSendFromISR(slave_rx_end_int_evt, (void *)&status, &xHigherPriorityTaskWoken);
       else
           xResult  = xQueueSendFromISR(slave_rx_int_evt, (void *)&status, &xHigherPriorityTaskWoken);
    }
    if (xResult != pdFAIL) {
        /*
         * If xHigherPriorityTaskWoken is now set to pdTRUE then a context
         * switch should be requested.  The macro used is port specific and will
         * be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
         * the documentation page for the port being used.
         */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

void host_slave_interrupt(void)
{
    host_interrupt_handler();
}


#if defined (SRAM_PROTECT)    ///////////////////////////////////

#define SRAM_EXCEPT_RANGE_START_0    (DA16X_SYS_SECURE_BASE | 0x0520)
#define SRAM_EXCEPT_RANGE_END_0      (DA16X_SYS_SECURE_BASE | 0x0524)

#define SRAM_EXCEPT_RANGE_START_1    (DA16X_SYS_SECURE_BASE | 0x0528)
#define SRAM_EXCEPT_RANGE_END_1      (DA16X_SYS_SECURE_BASE | 0x052c)

#define SRAM_EXCEPT_READ             (0x01 << 0)
#define SRAM_EXCEPT_WRITE            (0x01 << 1)
#define SRAM_EXCEPT_ENABLE           (0x01 << 2)
#define SRAM_EXCEPT_FUNC_0           (DA16X_SYS_SECURE_BASE | 0x0550)
#define SRAM_EXCEPT_FUNC_1           (DA16X_SYS_SECURE_BASE | 0x0554)

#define SRAM_EXCEPT_ENABLE_ADDR      (DA16X_SYS_SECURE_BASE | 0x0504)
#define MEM_LONG_WRITE(addr, data)   *((volatile UINT *)addr) = data

static void host_sram_protect(UINT32 *buffer, UINT32 buffer_length, UINT32 *control, UINT32 control_length)
{
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_START_0, (UINT32)buffer);
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_END_0, (UINT32)buffer + buffer_length);

    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_START_1, (UINT32)control);
    MEM_LONG_WRITE(SRAM_EXCEPT_RANGE_END_1, (UINT32)control + control_length);

    MEM_LONG_WRITE(SRAM_EXCEPT_FUNC_0, SRAM_EXCEPT_READ | SRAM_EXCEPT_WRITE | SRAM_EXCEPT_ENABLE);
    MEM_LONG_WRITE(SRAM_EXCEPT_FUNC_1, SRAM_EXCEPT_READ | SRAM_EXCEPT_WRITE | SRAM_EXCEPT_ENABLE);

    MEM_LONG_WRITE(SRAM_EXCEPT_ENABLE_ADDR, 0x03);
}
#endif    // SRAM_PROTECT     ///////////////////////////////////

//extern OAL_EVENT_GROUP lmac_rx_evt;
void   host_init(void)
{
    int i;

    FPGA_DBG_TIGGER(0x0);

    //SPI Master Init - for Temporary loop back test

    user_host_interface_init();

    for (i = 0; i < COMMON_HOST_MAX_HIS_NUM; i++) {
        host_hist_buf[i][0] = '\0';
    }

    host_buf[0] = '\0';

    host_data_buffer_allocated = (UINT *)host_buf;

#if    defined (__ATCMD_IF_SDIO__) || defined (__ATCMD_IF_SPI__)
    AT_Cmd_Addr = (UINT *)APP_MALLOC(SLAVE_CMD_SIZE);

    if (AT_Cmd_Addr == NULL) {
        AT_Cmd_Addr = (UINT *)pvPortMalloc(SLAVE_CMD_SIZE);
    }

#else
    AT_Cmd_Addr = (UINT *)embhal_malloc(SLAVE_CMD_SIZE); //need to free @ deinit
#endif // (__ATCMD_IF_SDIO__) || (__ATCMD_IF_SPI__)

    PRINTF(" BUF Address = 0x%x\r\n",(unsigned int)host_data_buffer_allocated);
    PRINTF(" AT Address = 0x%x\r\n",(unsigned int)AT_Cmd_Addr);

    DA16X_SLAVECTRL->AT_CMD_Ref = (unsigned int)AT_Cmd_Addr;

    // Interrupt
    _sys_nvic_write(SPI_Slave_IRQn, (void *)host_slave_interrupt, 0x7);

#ifdef SRAM_PROTECT
    host_sram_protect(host_data_buffer_allocated, 4096, (UINT32 *)0x50080200, 0x7c);
#endif

	slave_rx_int_evt = xQueueCreate( 5, sizeof( unsigned long ) );
	xQueueReset(slave_rx_int_evt);
    //APP_CORE
    {
        slave_rx_end_int_evt = xQueueCreate( 5, sizeof( unsigned long ) );
        xQueueReset(slave_rx_end_int_evt);
    }
    PRINTF("[%s] slave_rx_int_evt addr = 0x%x\r\n",__func__, (unsigned int)slave_rx_int_evt);

    if (slave_rx_int_evt == NULL) {
        PRINTF("[%s] slave_rx_int_evt Event Flags Create Error!\n",__func__);
    }
    slave_cmd_int_evt = xEventGroupCreate();
    PRINTF("[%s] slave_cmd_int_evt addr = 0x%x\r\n",__func__, (unsigned int)slave_cmd_int_evt);

    if (slave_cmd_int_evt == NULL) {
        PRINTF("[%s] slave_cmd_int_evt Event Flags Create Error!\n",__func__);
    }

    //APP_CORE
{
        extern void app_iot_at_IF_ready_notification(int _val);
        app_iot_at_IF_ready_notification(1);
}

    if ( hostif == HOSTIF_SPI || hostif == HOSTIF_SDIO ) {
        g_is_atcmd_init = 1;
        
        if (!dpm_mode_is_wakeup()) {
#if defined ( __SUPPORT_ATCMD__ )
            if (!is_in_softap_acs_mode()) {
                PRINTF_ATCMD("\r\n+INIT:DONE,%d\r\n", get_run_mode());
                if (get_run_mode() == SYSMODE_AP_ONLY) {
                    set_INIT_DONE_msg_to_MCU_on_SoftAP_mode(pdTRUE);
                }
            }
#endif// (__SUPPORT_ATCMD__)
        } else {

#ifdef __DPM_WAKEUP_NOTICE_ADDITIONAL__
  #if defined ( __SUPPORT_ATCMD__ )
            PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
  #endif// (__SUPPORT_ATCMD__)

#else

  #if defined ( __SUPPORT_ATCMD__ )
            if (   (void *)atcmd_dpm_wakeup_status() != NULL
    #if defined (__SUPPORT_NOTIFY_RTC_WAKEUP__)
                && strcmp(atcmd_dpm_wakeup_status(), "POR") != 0
    #endif // __SUPPORT_NOTIFY_RTC_WAKEUP__
                )
            {
                PRINTF_ATCMD("\r\n+INIT:WAKEUP,%s\r\n", atcmd_dpm_wakeup_status());
            }
  #endif // (__SUPPORT_ATCMD__)
#endif // (__DPM_WAKEUP_NOTICE_ADDITIONAL__)
        }
        g_is_atcmd_init = 0;

#if defined (__DPM_TEST_WITHOUT_MCU__)
        atcmd_dpm_ready = pdTRUE;
#endif // (__DPM_TEST_WITHOUT_MCU__)

        init_done_sent = pdTRUE;
    }

    BaseType_t xResult;

    xResult = xTaskCreate(Host_thread,
                          "@Host_drv",
                          HOST_DRV_STACK_SZ,
                          (void *) NULL,
                          (OS_TASK_PRIORITY_USER + 6),
                          &host_drv_task_handle );
    if (xResult != pdPASS)
        PRINTF("[%s] Failed to created Host_drv create fail(%d) \r\n",__func__, xResult);
}

void Host_thread(void *pvParameters)
{
    int sys_wdog_id = DA16X_SYS_WDOG_ID_DEF_ID;

    DA16X_UNUSED_ARG(pvParameters);

    sys_wdog_id = da16x_sys_watchdog_register(pdFALSE);
    if (sys_wdog_id >= 0) {
        da16x_sys_wdog_id_set_Host_drv(sys_wdog_id);
    }

    // Execute
    while (1) {
        da16x_sys_watchdog_notify(sys_wdog_id);

        if (hostif == HOSTIF_UART) {
            ;
        } else {
            host_process_command();        //others
        }
    }

    da16x_sys_watchdog_unregister(sys_wdog_id);
}

#endif    // !__SUPPORT_SIGMA_TEST__
#endif    // XIP_CACHE_BOOT


/* EOF */
