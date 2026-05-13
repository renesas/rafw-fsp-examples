/**
 ****************************************************************************************
 *
 * @file upss.c
 *
 * @brief Defines upss function related APIs used to operate on UPSS module.
 *
 *  Copyright (c) 2026 Renesas Electronics. All rights reserved.
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
