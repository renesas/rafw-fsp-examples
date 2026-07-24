/***********************************************************************************************************************
* File Name    : common_utils.h
* Description  : Contains macros, data structures and functions used common to the EP
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef COMMON_UTILS_H_
#define COMMON_UTILS_H_

/* generic headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hal_data.h"

/* SEGGER RTT and error related headers */

#define BIT_SHIFT_8  (8u)
#define SIZE_64      (64u)

#define LVL_ERR      (1u)       /* error conditions   */

#define RA_SDIO_VER_MAJOR      "1"
#define RA_SDIO_VER_MINOR      "10"
#define RA_SDIO_VER_IS_REL     (1)  /* REL:1 / DBG:0 */
#if (RA_SDIO_VER_IS_REL == 1)
#define RA_SDIO_VER_TYPE       "REL"
#else
#define RA_SDIO_VER_TYPE       "DBG"
#endif
#define RA_SDIO_TICKET_ID      

#define RESET_VALUE             (0x00)
#define KIT_NAME                "EK-RA6M4"
#define EP_VERSION              (RA_SDIO_VER_MAJOR "." RA_SDIO_VER_MINOR "_" RA_SDIO_VER_TYPE)
#define MCU_NAME                "RA6M4"
#define ATCMD_TRANSPORT         "SDIO"
#define MODULE_NAME             (MCU_NAME "_" ATCMD_TRANSPORT)

#define BUFFER_LINE_LENGTH (512)

typedef unsigned int        bool_t;
typedef int (*test_fn)(void);

#endif /* COMMON_UTILS_H_ */
