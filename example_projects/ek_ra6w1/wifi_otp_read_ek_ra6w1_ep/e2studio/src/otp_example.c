/***********************************************************************************************************************
* File Name    : otp_example.c
* Description  : Contains functions for OTP initialization and reading MAC from OTP.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020-2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "otp_example.h"
#include "hal_data.h"
#include "common_utils.h"
#include <stdio.h>
#include <string.h>

/* Marker for MAC section */
#define OTP_MAC_SECTION_MARKER  (0xE9000221U)

#define OTP_INVALID_WORD 0xFFFFFFFFU

static bool g_otp_initialized = false;

/*--------------------------------------------------
 * Helper: Reverse bits in a byte
 *--------------------------------------------------*/
static uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

/*--------------------------------------------------
 * OTP Init
 *--------------------------------------------------*/
otp_example_result_t otp_example_init(void)
{
    if (!g_otp_initialized)
    {
        bsp_otp_init();
        g_otp_initialized = true;
    }

    return OTP_EXAMPLE_SUCCESS;
}

/*--------------------------------------------------
 * Read single word
 *--------------------------------------------------*/
uint32_t otp_example_read_word(uint32_t offset)
{
    if (offset >= BSP_OTP_CELL_NUM)
    {
        return OTP_INVALID_WORD;
    }

    return bsp_otp_word_read(offset);
}

/*--------------------------------------------------
 * Read MAC from OTP
 *--------------------------------------------------*/
otp_example_result_t otp_example_read_mac(uint8_t *mac_addr)
{
    uint32_t i;
    uint32_t mac_low;
    uint32_t mac_high;
    uint8_t raw[OTP_EXAMPLE_MAC_SIZE];
    int j;

    if (!mac_addr)
    {
        return OTP_EXAMPLE_ERROR_INVALID_OFFSET;
    }

    for (i = OTP_OFFSET_CS_SCRIPT;
         i < (OTP_OFFSET_CS_SCRIPT + OTP_SIZE_CS_SCRIPT);
         i++)
    {
        if (bsp_otp_word_read(i) == OTP_MAC_SECTION_MARKER)
        {
            mac_low = bsp_otp_word_read(i + 1);
            mac_high = bsp_otp_word_read(i + 2);
            raw[0] = (uint8_t)(mac_low);
            raw[1] = (uint8_t)(mac_low >> 8);
            raw[2] = (uint8_t)(mac_low >> 16);
            raw[3] = (uint8_t)(mac_low >> 24);
            raw[4] = (uint8_t)(mac_high);
            raw[5] = (uint8_t)(mac_high >> 8);
            /* Reverse bits + reverse byte order */
            for (j = 0; j < OTP_EXAMPLE_MAC_SIZE; j++)
            {
                mac_addr[j] = reverse_bits(raw[OTP_EXAMPLE_MAC_SIZE - 1 - j]);
            }

            return OTP_EXAMPLE_SUCCESS;
        }
    }

    return OTP_EXAMPLE_ERROR_READ_FAILED;
}

/*--------------------------------------------------
 * Print MAC
 *--------------------------------------------------*/
static void otp_example_print_mac(uint8_t *mac_addr)
{
    printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);
}

/*--------------------------------------------------
 * Demo
 *--------------------------------------------------*/
otp_example_result_t otp_example_run_read_demo(void)
{
    uint8_t mac_addr[OTP_EXAMPLE_MAC_SIZE];

    printf("\r\n");
    printf("========================================\r\n");
    printf("   OTP Read Example Application \r\n");
    printf("========================================\r\n");

    printf("\r\nInitializing OTP controller...\r\n");

    if (otp_example_init() != OTP_EXAMPLE_SUCCESS)
    {
        printf("ERROR: OTP init failed\r\n");
        return OTP_EXAMPLE_ERROR_INIT_FAILED;
    }

    printf("  OTP initialized OK\r\n");

    printf("\r\nReading the MAC Address \r\n \r\n");

    if (otp_example_read_mac(mac_addr) == OTP_EXAMPLE_SUCCESS)
    {
        otp_example_print_mac(mac_addr);
    }
    else
    {
        printf("MAC not found\r\n");
    }

    otp_example_close();

    printf("\r\n========================================\r\n");
    printf("        OTP Read Complete      \r\n");
    printf("========================================\r\n");

    return OTP_EXAMPLE_SUCCESS;
}

/*--------------------------------------------------
 * Close
 *--------------------------------------------------*/
void otp_example_close(void)
{
    if (g_otp_initialized)
    {
        bsp_otp_close();
        g_otp_initialized = false;
    }
}
