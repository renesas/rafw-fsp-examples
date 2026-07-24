/***********************************************************************************************************************
* File Name    : sdio_cmd.h
* Description  : SDIO command function declarations.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#ifndef _SDIO_CMD_H_
#define _SDIO_CMD_H_

#include "bsp_api.h"
#if (SUPPORT_SDIO == 1)
unsigned char toint(char c);
unsigned int htoi(char *s);

typedef enum _MODE_FLAG {
    START_MODE = 0,
    SET_MODE,
    RUN_MODE,
    READ_MODE,
    AP_MODE,
    COMMISSIONING_MODE
} MODE_FLAG;

fsp_err_t SDIO_Write_To_DA16xxx(uint8_t *buf, uint32_t len);
fsp_err_t SDIO_Read_From_DA16xxx(uint8_t *buf, uint32_t len);
fsp_err_t Received_SDIO_Data_From_DA16xxx(void);
void Transfer_ATCmd_To_SDIO(int argc, char *argv[]);
void SDIO_cmd_init(void);
#endif
#endif /* _SPI_CMD_H_ */

