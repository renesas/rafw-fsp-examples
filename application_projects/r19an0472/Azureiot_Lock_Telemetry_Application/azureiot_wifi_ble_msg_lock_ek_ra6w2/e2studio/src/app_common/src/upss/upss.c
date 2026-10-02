/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
#include <stdbool.h>

#include "FreeRTOS.h"

#include "task.h"

#if defined(__SUPPORT_UPSS__)
#include "upss.h"
#endif

#if defined(__RTOS_PS__)
extern int rwnx_ops_pson(bool fastps);
extern int rwnx_ops_psoff();
#endif //__RTOS_PS__

int APP_Power_Save_Start(void)
{
    int rcode = 0;
#if defined(__RTOS_PS__)
    rcode = rwnx_ops_pson(0);
#endif
    return rcode;
}
int APP_Power_Save_Stop(void)
{
    int rcode = 0;
#if defined(__RTOS_PS__)
    rcode = rwnx_ops_psoff();
#endif //__RTOS_PS__
    return rcode;
}

void APP_Inrush_Suppress_Enable(void)
{
#if defined(__COINCELL_FEATURE__)
    //DC DC STR
    *((volatile unsigned int*)DC_DC_STR_REG) = DC_DC_STR_VALUE;

    vTaskDelay(2);

    *((volatile unsigned int*)DC_DC_STR_REG) = DC_DC_STR_READ_SET_VALUE;

    vTaskDelay(2);
#endif //__COINCELL_FEATURE__
}

void APP_Inrush_Suppress_Disable(void)
{
#if defined(__COINCELL_FEATURE__)
    //DC DC STR
    *((volatile unsigned int*)DC_DC_STR_REG) = DC_DC_STR_ORG_VALUE;

    vTaskDelay(2);

    *((volatile unsigned int*)DC_DC_STR_REG) = DC_DC_STR_READ_SET_ORG_VALUE;

    vTaskDelay(2);
#endif //__COINCELL_FEATURE__
}

unsigned int APP_Inrush_Reg_Read(void)
{
    unsigned int readReg = 0;

#if defined(__COINCELL_FEATURE__)
    readReg = *((volatile unsigned int*)(DC_DC_STR_REG));
#endif //__COINCELL_FEATURE__

    return readReg;
}
