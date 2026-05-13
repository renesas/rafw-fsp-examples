/***********************************************************************************************************************
 * File Name    : hal_entry.c
 * Description  : Entry point for the application.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/***********************************************************************************************************************
 * Function Name: hal_entry
 * Description  : Initial entry function. Called when RTOS is not used.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/***********************************************************************************************************************
 * Function Name: R_BSP_WarmStart
 * Description  : Called at various points during MCU warm start.
 * Arguments    : event - warm start event indicator
 * Return Value : None
 ***********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
        /* Reset event - no action required */
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins */
        IOPORT_CFG_OPEN(&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable(void);

/***********************************************************************************************************************
 * Function Name: template_nonsecure_callable
 * Description  : Required callable stub for TrustZone projects.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable(void)
{
}
FSP_CPP_FOOTER

#endif
