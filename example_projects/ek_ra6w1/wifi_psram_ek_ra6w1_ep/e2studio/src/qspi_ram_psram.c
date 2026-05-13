/***********************************************************************************************************************
 * File Name    : qspi_ram_psram.c
 * Description  : Contains data structures and functions used in qspi_ram_psram.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020-2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <common_utils.h>
#include <stdio.h>

#include "r_qspi_w.h"
#include "hal_data.h"

#include "FreeRTOS.h"
#include "task.h"

/* For logging */
#define EP_APP_VERSION      1.0
#define EP_APP_MODULE_NAME  "r_qspi_w:ram"
#define EP_APP_DESCRIPTION \
            "This example demonstrate the basic usage of QSPI PSRAM\n\r" \
            "The application is based on RA6W1 EVK with APS6404L-SQRH QSPI PSRAM"

#define PSRAM_TEST1 psram_full_range_wr_test
#define PSRAM_TEST2 psram_string_wr_test

void r_qspi_w_psram_basic_example(void);

#define SZ_1K      (1024)
#define SZ_1M      (SZ_1K * SZ_1K)
#define SZ_4K      (4 * 1024)
#define SZ_256K    (256 * SZ_1K)
#define SZ_100K    (100 * SZ_1K)
#define SZ_50K     (50 * SZ_1K)

const uint32_t test_size = 128 * SZ_1K;

/* Logging control */
#define SZ_LOG (SZ_1M)

/* Enable if we need custom initialization behavior */
#define QSPI_RAM_CUSTOM_INIT (1)

/* QSPI Commands */
#define QSPI_RAM_COMMAND_ENTER_QPI_MODE (0x38U)
#define QSPI_RAM_COMMAND_READ_ID        (0x9FU)

static uint32_t g_sz_sram = SZ_1M;

#if QSPI_RAM_CUSTOM_INIT
static struct
{
    uint32_t size;
    char *s_size;
} g_density[] =
{
    { (SZ_1M * 16),  "16MB"  },
    { (SZ_1M * 32),  "32MB"  },
    { (SZ_1M * 64),  "64MB"  },
    { (SZ_1M * 128), "128MB" },
    { (SZ_1M * 256), "256MB" }
};

/*******************************************************************************************************************//**
 *  @brief       Initialize PSRAM and enter in QPI mode
 **********************************************************************************************************************/
static fsp_err_t r_qspi_w_psram_example_init(void)
{
    uint8_t buf[10];
    fsp_err_t err;
    int i;
    uint8_t mfid, kgd, density;

    memset(buf, 0x0, sizeof(buf));
    buf[0] = QSPI_RAM_COMMAND_READ_ID;

    err = R_QSPI_W_DirectWrite(&g_qspi_ram0_ctrl, &buf[0], 4, true);
    assert(FSP_SUCCESS == err);

    memset(buf, 0x0, sizeof(buf));

    err = R_QSPI_W_DirectRead(&g_qspi_ram0_ctrl, &buf[0], 9);
    assert(FSP_SUCCESS == err);

    APP_PRINT_INFO("ID: ");
    for (i = 0; i < 9; i++)
    {
        APP_PRINT("0x%x ", buf[i]);
    }
    APP_PRINT("\n\r");

    mfid = buf[0];
    kgd = buf[1];
    density = buf[2] >> 5;

    APP_PRINT_INFO("ID INFO:\n\r"
                   "\tMF ID  : 0x%02X\n\r"
                   "\tKGD    : 0x%02X\n\r"
                   "\tDensity: 0x%02X (%s)\n\n\r",
                   mfid, kgd, density, g_density[density].s_size);

    if (kgd != 0x5D)
    {
        APP_PRINT_ERR("PSRAM Failed to detect, BAD KGD(Known-Good-Die) value(0x%02X)\n\r", kgd);
        return FSP_ERR_INVALID_DATA;
    }

    g_sz_sram = g_density[density].size;

    memset(buf, 0x0, sizeof(buf));
    buf[0] = QSPI_RAM_COMMAND_ENTER_QPI_MODE;

    err = R_QSPI_W_DirectWrite(&g_qspi_ram0_ctrl, &buf[0], 1, false);
    assert(FSP_SUCCESS == err);

    return err;
}
#endif

/*******************************************************************************************************************//**
 *  @brief       Write pattern data and Read it
 **********************************************************************************************************************/
static bool psram_full_range_wr_test(void)
{
    uint8_t *copy_block, *read_block;
    uint8_t *t_address = (uint8_t *)(QSPI_W_DEVICE_START_ADDRESS);
    uint32_t i, j, k;

    copy_block = pvPortMalloc(SZ_1K);
    if (!copy_block)
    {
        APP_PRINT_ERR("%s: pvPortMalloc failed for copy_block\n\r", __func__);
        return false;
    }

    read_block = pvPortMalloc(SZ_1K);
    if (!read_block)
    {
        APP_PRINT_ERR("%s: pvPortMalloc failed for read_block\n\r", __func__);
        vPortFree(copy_block);
        return false;
    }

    for (i = 0; i < SZ_1K; i++)
    {
        copy_block[i] = i % 256;
    }

    APP_PRINT_INFO("%s: Full Region r/w example\n\r", __func__);
    APP_PRINT_INFO("Clearing %u Bytes=%uKB=%uMB\n\r",
                   g_sz_sram,
                   g_sz_sram / SZ_1K,
                   g_sz_sram / SZ_1M);

    for (i = 0, j = 0; i < g_sz_sram; i += SZ_256K, j = i / SZ_1M + 1)
    {
        memset((void *)&t_address[i], 0xFE, SZ_256K);
        vTaskDelay(1);

        if ((i % SZ_LOG) == 0)
        {
            APP_PRINT_INFO("CLEAR: 1MB Block[%lu] OK, %lu%%\n\r",
                           j,
                           (((i + SZ_LOG) / SZ_1K * 100) / (g_sz_sram / SZ_1K)));
        }
    }

    APP_PRINT("\n\r");

    for (i = 0, j = 0; i < g_sz_sram; i += SZ_1K, j = i / SZ_1M + 1)
    {
        memcpy(t_address + i, copy_block, SZ_1K);

        if ((i % SZ_LOG) == 0)
        {
            APP_PRINT_INFO("WRITE: 1MB Block[%lu] OK, %lu%%\n\r",
                           j,
                           (((i + SZ_LOG) / SZ_1K * 100) / (g_sz_sram / SZ_1K)));

            vTaskDelay(1);
        }
    }

    APP_PRINT("\n\r");

    for (i = 0, j = 0; i < g_sz_sram; i += SZ_1K, j = i / SZ_1M + 1)
    {
        memset(read_block, 0xF3, SZ_1K);
        memcpy(read_block, t_address + i, SZ_1K);

        for (k = 0; k < SZ_1K; k++)
        {
            if (read_block[k] != (k % 256))
            {
                APP_PRINT_ERR("FAILED!: data mismatch %lu [%d != %lu]\n\r",
                              i + k,
                              t_address[i + k],
                              (k % 256));

                vPortFree(copy_block);
                vPortFree(read_block);
                return false;
            }
        }

        if ((i % SZ_LOG) == 0)
        {
            APP_PRINT_INFO("READ: 1MB Block[%lu] OK, %lu%%\n\r",
                           j,
                           (((i + SZ_LOG) / SZ_1K * 100) / (g_sz_sram / SZ_1K)));

            vTaskDelay(1);
        }
    }

    APP_PRINT("\n\r");

    vPortFree(copy_block);
    vPortFree(read_block);

    return true;
}

/*******************************************************************************************************************//**
 *  @brief       Write string data and Read it
 **********************************************************************************************************************/
static bool psram_string_wr_test(void)
{
    const char t_sting[] = "TEST sTrING@#4512";
    uint8_t *t_address = (uint8_t *)(QSPI_W_DEVICE_START_ADDRESS + 1024U);

    APP_PRINT_INFO("%s: String r/w example\n\r", __func__);

    strcpy((char *)t_address, t_sting);

    APP_PRINT_INFO("String: read after write: string @offset %p [%s]\n\r",
                   t_address, t_address);

    if (strcmp((const char *)t_address, t_sting))
        return false;

    memset(t_address, 0x00, SZ_1K);

    if (strcmp((const char *)t_address, t_sting) >= 0)
        return false;

    APP_PRINT_INFO("String: Read after erase: string @offset %p [%s]\n\r",
                   t_address, t_address);

    return true;
}

/*******************************************************************************************************************//**
 *  @brief       Test init API
 **********************************************************************************************************************/
void r_qspi_w_psram_basic_example(void)
{
    fsp_err_t err;
    TickType_t tick = 0;

    print_ep_info_banner(EP_APP_MODULE_NAME,
                         _STRINGFY(EP_APP_VERSION),
                         EP_APP_DESCRIPTION);

    err = R_QSPI_W_Open(&g_qspi_ram0_ctrl, &g_qspi_ram0_cfg);
    assert(FSP_SUCCESS == err);

#if QSPI_RAM_CUSTOM_INIT
    if (r_qspi_w_psram_example_init() != FSP_SUCCESS)
    {
        APP_PRINT_ERR("%s: PSRAM Init FAILED!\n\r", __func__);
        return;
    }
#endif

    tick = xTaskGetTickCount();

    if (PSRAM_TEST1())
    {
        APP_PRINT_INFO("\n\rPASS [%s]\n\n\r", _STRINGFY(PSRAM_TEST1));
    }
    else
    {
        APP_PRINT_ERR("\n\rFAIL [%s]\n\n\r", _STRINGFY(PSRAM_TEST1));
    }

    if (PSRAM_TEST2())
    {
        APP_PRINT_INFO("\n\rPASS [%s]\n\n\r", _STRINGFY(PSRAM_TEST2));
    }
    else
    {
        APP_PRINT_ERR("\n\rFAIL [%s]\n\n\r", _STRINGFY(PSRAM_TEST2));
    }

    APP_PRINT_INFO("\n\rTotal time=%lums\n\r",
                   portCONVERT_TICKS_2_MS(xTaskGetTickCount() - tick));
}
