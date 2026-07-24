/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "hal_data.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/*******************************************************************************************************************//**
 * hal_entry function
 **********************************************************************************************************************/
void hal_entry(void)
{
#if BSP_TZ_SECURE_BUILD
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * Warm start callback
 *
 * @param[in] event  BSP warm start event
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
    }

    if (BSP_WARM_START_POST_C == event)
    {
#if CFG_PMGR
        g_pmgr_w_ins.p_api->open(&g_pmgr_w_ctrl, &g_pmgr_w_cfg);
#endif

        IOPORT_CFG_OPEN(&IOPORT_CFG_CTRL, &IOPORT_CFG_NAME);
    }
}

#if BSP_TZ_SECURE_BUILD
FSP_CPP_HEADER

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable(void);

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable(void)
{
}

FSP_CPP_FOOTER
#endif
