/***********************************************************************************************************************
* File Name    : sdio_drv.h
* Description  : SDIO driver function declarations.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef _SDIO_DRV_H_
#define _SDIO_DRV_H_

#if (SUPPORT_SDIO == 1)
#include "main_thread.h"

#define SDIO_CCCR_CAPS		0x08
#define SDIO_CCCR_IOEx		0x02
#define SDIO_CCCR_IORx		0x03
#define SDIO_CCCR_IF		0x07	/* bus interface controls */
#define SDIO_BUS_WIDTH_1BIT	0x00
#define SDIO_BUS_WIDTH_4BIT	0x02
#define SDIO_CCCR_BLKSIZE	0x10
#define SDIO_CCCR_IENx		0x04	/* Function/Master Interrupt Enable */
#define BLOCK_SIZE          512

extern volatile uint32_t g_sdio_transfer_done;
extern void PRINTF(char *fmt, ...);

fsp_err_t sdio_read_ext(uint8_t *p_data, uint32_t address, uint32_t length);
fsp_err_t sdio_write_ext(uint8_t *p_data, uint32_t address, uint32_t length);
fsp_err_t sdio_readb(uint8_t *p_data, uint32_t function, uint32_t address);
fsp_err_t sdio_writeb(uint8_t *p_data, uint32_t function, uint32_t address, sdmmc_io_write_mode_t read_after_write);
fsp_err_t sdio_read_ext_1(uint8_t *p_data, uint32_t address, uint32_t length);
fsp_err_t sdio_read_data_ext(uint8_t *p_data, uint32_t address, uint32_t length);
fsp_err_t init_drv_sdio(void);
fsp_err_t sdio_initialize(void);
bool get_sdhi_init_status(void);
#endif /* SUPPORT_SDIO */
#endif /* _SDIO_DRV_H_ */
