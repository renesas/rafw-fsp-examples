/***********************************************************************************************************************
* File Name    : sdio_drv.c
* Description  : SDIO driver implementation.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if (SUPPORT_SDIO == 1)
#include "sdio_drv.h"
#include "sdio_cmd.h"

#define SDIO_RETRY_COUNT     (10000U)

volatile uint32_t g_sdio_transfer_done;
uint32_t g_sdio_reinitialize_needs;
uint32_t g_sdio_first_write = 1;

//extern uint32_t g_sdio_trans_err;

fsp_err_t sdio_read_ext_1(uint8_t *p_data, uint32_t address, uint32_t length)
{
    (void) address;

    fsp_err_t ret = FSP_SUCCESS;
#if 0
    uint8_t data;
#endif
    uint32_t retry = 0;
#if DEBUG_XTRA
PRINTF("\r\n %s: \r\n",__func__);
#endif
    if (!g_sdmmc0_ctrl.initialized) {
        g_sdio_reinitialize_needs = 1;
        PRINTF("SDIO is not initialized\r\nINITIALIZATION START!!\r\n");
        return FSP_ERR_NOT_INITIALIZED;
    }
#if 0
    for (int i = 0; i < 4; i++) {
        data = (address >> (i * 8)) & 0xff;
        g_sdio_transfer_done = 0;
        ret = sdio_writeb(&data, 1, (uint32_t)(0x44 + i), SDMMC_IO_WRITE_MODE_NO_READ);
        if (FSP_SUCCESS != ret)
        {
            PRINTF("%s:sdio_writeb failed\r\n",__func__);
        }
    }
#endif
    g_sdio_transfer_done = 0;
#if 1
    g_sdio_transfer_done = 0;
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                        , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_FIXED);
        if (FSP_SUCCESS != ret)
            {
               PRINTF("\r\n %s: readIoExt failed\r\n",__func__);
            }
#endif
#if 0
    if (length < 512) {
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_INCREMENT);
        if (FSP_SUCCESS != ret)
                {
                    PRINTF("%s:readIoExt failed\r\n",__func__);
                }
    } else {
        uint32_t blocksize = length / 512;
        if (length % 512)
            blocksize++;
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &blocksize
                , SDMMC_IO_MODE_TRANSFER_BLOCK, SDMMC_IO_ADDRESS_MODE_INCREMENT);
    }
#endif
    while (g_sdio_transfer_done != 1) {
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        retry++;
        if (retry > SDIO_RETRY_COUNT) {
#if DEBUG_XTRA
            PRINTF("SDIO read callback timeout_1 0x%x\r\n", g_sdio_transfer_done);
#endif
            g_sdio_reinitialize_needs = 1;
            break;
        }
    };
#if DEBUG_XTRA
    PRINTF("\r\n %s: g_sdio_transfer_done\r\n",__func__);
#endif
    return ret;
}

fsp_err_t sdio_read_ext(uint8_t *p_data, uint32_t address, uint32_t length)
{
    (void) address;

    fsp_err_t ret = FSP_SUCCESS;
#if 0
    uint8_t data;
#endif
    uint32_t retry = 0;
#if DEBUG_XTRA
PRINTF("\r\n %s: \r\n",__func__);
#endif
    if (!g_sdmmc0_ctrl.initialized) {
        g_sdio_reinitialize_needs = 1;
        PRINTF("SDIO is not initialized\r\nINITIALIZATION START!!\r\n");
        return FSP_ERR_NOT_INITIALIZED;
    }
#if 0
	for (int i = 0; i < 4; i++) {
		data = (address >> (i * 8)) & 0xff;
        g_sdio_transfer_done = 0;
        ret = sdio_writeb(&data, 1, (uint32_t)(0x44 + i), SDMMC_IO_WRITE_MODE_NO_READ);
        if (FSP_SUCCESS != ret)
        {
            PRINTF("%s:sdio_writeb failed\r\n",__func__);
        }
	}
#endif
    g_sdio_transfer_done = 0;
#if 0
    g_sdio_transfer_done = 0;
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                        , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_FIXED);
        if (FSP_SUCCESS != ret)
            {
               PRINTF("\r\n %s: readIoExt failed\r\n",__func__);
            }
#endif
#if 1
    if (length < 512) {
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_FIXED);
        if (FSP_SUCCESS != ret)
                {
                    PRINTF("%s:readIoExt failed\r\n",__func__);
                }
    } else {
		uint32_t blocksize = length / 512;
#if 1
		if (length % 512)
            blocksize++;
#endif
		ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &blocksize
                , SDMMC_IO_MODE_TRANSFER_BLOCK, SDMMC_IO_ADDRESS_MODE_FIXED);
    }
#endif
    while (g_sdio_transfer_done != 1) {
        R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
        retry++;
        if (retry > SDIO_RETRY_COUNT) {
#if DEBUG_XTRA
            PRINTF("SDIO read callback timeout_1 0x%x\r\n", g_sdio_transfer_done);
#endif
            PRINTF("SDIO read callback timeout_1 0x%x\r\n", g_sdio_transfer_done);
            g_sdio_reinitialize_needs = 1;
            break;
        }
    };
#if DEBUG_XTRA
    PRINTF("\r\n %s: g_sdio_transfer_done\r\n",__func__);
#endif

    return ret;
}

fsp_err_t sdio_read_data_ext(uint8_t *p_data, uint32_t address, uint32_t length)
{
    (void) address;

    fsp_err_t ret = FSP_SUCCESS;
#if 0
    uint8_t data;
#endif
    uint32_t retry = 0;

    if (!g_sdmmc0_ctrl.initialized) {
        g_sdio_reinitialize_needs = 1;
        PRINTF("SDIO is not initialized\r\nINITIALIZATION START!!\r\n");
        return FSP_ERR_NOT_INITIALIZED;
    }
#if 0
    for (int i = 0; i < 4; i++) {
        data = (address >> (i * 8)) & 0xff;
        g_sdio_transfer_done = 0;
        ret = sdio_writeb(&data, 1, (uint32_t)(0x44 + i), SDMMC_IO_WRITE_MODE_NO_READ);
    }
#endif
    g_sdio_transfer_done = 0;
    ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                    , SDMMC_IO_MODE_TRANSFER_BLOCK, SDMMC_IO_ADDRESS_MODE_FIXED);
#if 0
    if (length < 512) {
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &length
                , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_INCREMENT);
    } else {
        uint32_t blocksize = length / 512;
        if (length % 512)
            blocksize++;
        ret = g_sdmmc_on_sdhi.readIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, &blocksize
                , SDMMC_IO_MODE_TRANSFER_BLOCK, SDMMC_IO_ADDRESS_MODE_INCREMENT);
    }
#endif
    while (g_sdio_transfer_done != 1) {
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        retry++;
        if (retry > SDIO_RETRY_COUNT) {
            PRINTF("SDIO read callback timeout_2 0x%x\r\n", g_sdio_transfer_done);
            g_sdio_reinitialize_needs = 1;
            break;
        }
    };

    return ret;
}

fsp_err_t sdio_write_ext(uint8_t *p_data, uint32_t address, uint32_t length)
{
    (void) address;

    fsp_err_t ret = FSP_SUCCESS;
#if 0
    uint8_t data;
#endif
    uint32_t retry = 0;

    if (!g_sdmmc0_ctrl.initialized) {
        g_sdio_reinitialize_needs = 1;
        PRINTF("SDIO is not initialized\r\nINITIALIZATION START!!\r\n");
        return FSP_ERR_NOT_INITIALIZED;
    }

#if 1
    if(g_sdio_first_write == 0)
    {
        while (g_sdio_transfer_done != 1) {
            R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
            retry++;
            if (retry > SDIO_RETRY_COUNT) {
                PRINTF("SDIO write callback timeout 0x%x\r\n", g_sdio_transfer_done);
                g_sdio_reinitialize_needs = 1;
                break;
            }
        };
    }
#endif

#if 0
	for (int i = 0; i < 4; i++) {
		data = (address >> (i * 8)) & 0xff;
        g_sdio_transfer_done = 0;
        PRINTF("\r\n %s: data: %s\r\n",__func__,data);
        ret = sdio_writeb(&data, 1, (uint32_t)(0x44 + i), SDMMC_IO_WRITE_MODE_NO_READ);
	}
#endif
    g_sdio_transfer_done = 0;
    if (length < 512) {
#if DEBUG_XTRA
        PRINTF("\r\n %s: p_data: %s\r\n",__func__,p_data);
#endif
#if 0
        ret = g_sdmmc_on_sdhi.writeIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, length
                , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_INCREMENT);
#endif
        ret = g_sdmmc_on_sdhi.writeIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, length
                        , SDMMC_IO_MODE_TRANSFER_BYTE, SDMMC_IO_ADDRESS_MODE_FIXED);
    } else {
		uint32_t blocksize = length / 512;
		if (length % 512)
            blocksize++;
        ret = g_sdmmc_on_sdhi.writeIoExt(&g_sdmmc0_ctrl, p_data, 1, 0, blocksize
                , SDMMC_IO_MODE_TRANSFER_BLOCK, SDMMC_IO_ADDRESS_MODE_FIXED);
    }

    g_sdio_first_write = 0;
#if 0
    while (g_sdio_transfer_done != 1) {
        R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MICROSECONDS);
        retry++;
        if (retry > 10000) {
            PRINTF("SDIO write callback timeout 0x%x\r\n", g_sdio_transfer_done);
            g_sdio_reinitialize_needs = 1;
            break;
        }
    };
#endif
#if DEBUG_XTRA
    PRINTF("\r\n g_sdio_transfer_done\r\n");
#endif
#if 0
    if(g_sdio_transfer_done != 1)
        PRINTF("intr 0x%x\r\n", g_sdio_trans_err);
#endif

    return ret;
}

fsp_err_t sdio_readb(uint8_t *p_data, uint32_t function, uint32_t address)
{
    fsp_err_t ret = FSP_SUCCESS;
    ret = g_sdmmc_on_sdhi.readIo(&g_sdmmc0_ctrl, p_data, function, address);
    return ret;
}

fsp_err_t sdio_writeb(uint8_t *p_data, uint32_t function, uint32_t address, sdmmc_io_write_mode_t read_after_write)
{
    fsp_err_t ret = FSP_SUCCESS;
    ret = g_sdmmc_on_sdhi.writeIo(&g_sdmmc0_ctrl, p_data, function, address, read_after_write);
    return ret;
}

fsp_err_t sdio_initialize(void)
{
    #undef SDIO_DEBUG
	uint32_t function, address;
    uint8_t data;
	uint8_t func_num = 1;
    fsp_err_t err = FSP_SUCCESS;

	address = SDIO_CCCR_CAPS;
	function = 0;
	err = sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
    PRINTF("caps : %02X\r\n", data);
#endif

	address = SDIO_CCCR_IOEx;
	err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("CCCR_IOEx Data : %d\r\n", data);
#endif

	data |= (uint8_t)(1 << func_num);
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("CCCR_IOEx Data : %d\r\n", data);
#endif

	address = SDIO_CCCR_IORx;
	int i = 0;
    for (i = 0; i < 10; i ++)
    {
        err |= sdio_readb(&data, function, address);
        if (data & (1 << func_num))
            break;
    }

	//set 4-bit mode only
	address = SDIO_CCCR_IF;
    err |= sdio_readb(&data, function, address);

	data = SDIO_BUS_WIDTH_4BIT | 0x80;
	address = SDIO_CCCR_IF;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("sdio 4bit mode cccr 0x%02X\r\n", data);
#endif

	function = 1;
	address = 0x38;
	data = 0x01;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

	//Set blk size 512
	function = 0;
	address = SDIO_CCCR_BLKSIZE + 0x100;
	data = (BLOCK_SIZE) & 0xff;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

	address = SDIO_CCCR_BLKSIZE + 0x100 + 1;
	data = (BLOCK_SIZE >> 8) & 0xff;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

	function = 0;
	address = SDIO_CCCR_BLKSIZE + 0x100;
    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("block size 0x110 0x%02X\r\n", data);
#endif

	address = SDIO_CCCR_BLKSIZE + 0x100 + 1;
    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("block size 0x111 0x%02X\r\n", data);
#endif

	//check CSA
	address = 0x10c;
    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("0x10c : 0x%02X\r\n", data);
#endif
	if (data != 0)
	{
		data = 0;
        err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);
	}
	address = 0x10d;
    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("0x10d : 0x%02X\r\n", data);
#endif
	if (data != 0)
	{
		data = 0;
        err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);
	}

	//set high clock
	address = 0x13;
    err |= sdio_readb(&data, function, address);

	data |= 0x02;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);

    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
	PRINTF("set high clock 0x13 : 0x%02X\r\n", data);
#endif
	PRINTF("SDIO INITIALIZE COMPLETE!\r\n");
	address = SDIO_CCCR_IENx;
	function = 0;
	data = 2;
	err |= sdio_writeb(&data, function, address, SDMMC_IO_WRITE_MODE_NO_READ);
#ifdef SDIO_DEBUG
	PRINTF("writeb err=0x%02X\r\n",err);
#endif
	data = 0;
    err |= sdio_readb(&data, function, address);
#ifdef SDIO_DEBUG
    PRINTF("readb err=0x%02X\r\n",err);
#endif
#ifdef SDIO_DEBUG
	PRINTF("SDIO CCCR_IENx = 0x%02X\r\n", data);
#endif

	return err;
}


void user_sdio_transfer_callback(sdmmc_callback_args_t *p_args)
{
    if (p_args->event & SDMMC_EVENT_TRANSFER_COMPLETE) {
        g_sdio_transfer_done = 1;
    } else if (p_args->event & SDMMC_EVENT_TRANSFER_ERROR) {
        g_sdio_transfer_done = p_args->event;
        //PRINTF("err:0x%08x\n", g_sdio_trans_err);
    } else
        g_sdio_transfer_done = p_args->event;

    //PRINTF("err:0x%08x\n", g_sdio_trans_err);

}

bool get_sdhi_init_status(void)
{
    sdmmc_status_t sdhi_status = {0};

    g_sdmmc_on_sdhi.statusGet(&g_sdmmc0_ctrl, &sdhi_status);

    return sdhi_status.initialized;
}


fsp_err_t init_drv_sdio(void)
{
    fsp_err_t err;

    if (g_sdmmc0_ctrl.open != 0) {
        err = g_sdmmc_on_sdhi.close(&g_sdmmc0_ctrl);
#if DEBUG_XTRA
        PRINTF("SDIO Close\r\n");
#endif
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    }

    err = g_sdmmc_on_sdhi.open(&g_sdmmc0_ctrl, &g_sdmmc0_cfg);
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
    if (err != FSP_SUCCESS) {
        PRINTF("SDIO Open Fail \r\n");
        return FSP_SUCCESS;
    }

    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);

    err = g_sdmmc_on_sdhi.mediaInit(&g_sdmmc0_ctrl, NULL);
    if (err != FSP_SUCCESS) {
        PRINTF("SDIO mediaInit Fail \r\n");
        return FSP_SUCCESS;
    }
    PRINTF("SDIO Open Success \r\n");

    g_sdio_first_write = 1;

    err = g_sdmmc_on_sdhi.callbackSet(&g_sdmmc0_ctrl, user_sdio_transfer_callback, NULL, NULL);
    if (err != FSP_SUCCESS) {
        PRINTF("SDIO set callback Fail \r\n");
        return FSP_SUCCESS;
    }

    err = sdio_initialize();
    if (err != FSP_SUCCESS) {
        PRINTF("SDIO initialize Fail \r\n");
        return FSP_SUCCESS;
    }

    SDIO_cmd_init();

	return FSP_SUCCESS;
}
#endif
