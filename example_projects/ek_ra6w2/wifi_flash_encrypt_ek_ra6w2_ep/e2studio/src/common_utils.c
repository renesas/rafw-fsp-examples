/***********************************************************************************************************************
 * File Name    : common_utils.c
 * Description  : Contains macros, data structures and functions used  common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "bsp_api.h"
#include "common_utils.h"

#define BANNER_1    "******************************************************************\n\r"
#define BANNER_2    "*   Renesas FSP Example Project for %s Module\n\r"
#define BANNER_3    "*   Example Project Version %s\n\r"
#define BANNER_4    "*   Flex Software Pack Version  %d.%d.%d\n\r"
#define BANNER_5    "******************************************************************\n\r"
#define BANNER_6    "Overview:\n\r%s\n\r"
#define BANNER_7    "Refer to readme.txt file for more details on Example Project and\n\r" \
                    "FSP User's Manual for more information about %s driver\n\n\r"

/*************************************************************
 * Wrapper for SEGGER_RTT_printf that supports larger output.
 * Uses vsnprintf into a local buffer (RTT_PRINT_BUF_SIZE),
 * then writes via SEGGER_RTT_Write in a retry loop so that
 * no data is silently dropped when the RTT up-buffer is full.
 *************************************************************/
int rtt_printf(unsigned int index, const char *fmt, ...)
{
    char buf[RTT_PRINT_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
        if ((unsigned int)len >= sizeof(buf))
        {
            len = (int)(sizeof(buf) - 1U);
        }

        /* Blocking write: retry until the host drains the ring buffer. */
        unsigned int written = 0;
        while (written < (unsigned int)len)
        {
            unsigned int n = SEGGER_RTT_Write(index,&buf[written],
                                              (unsigned int)len - written);
            written += n;
        }
    }
    return len;
}

/*******************************************************************************************************************//**
 *  @brief       Initialize PSRAM and enter in QPI mode
 *  @param[IN]   s_module/string, Name of the module
 *  @param[IN]   s_version/string, Version of example as a string
 *  @param[in]   s_info/string, brief description of the example application
 *  @retval      API status = fsp_err_t
 **********************************************************************************************************************/
void print_ep_info_banner(const char *s_module, const char *s_version, const char *s_info)
{
    fsp_pack_version_t version;

    /* version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the Console */
    APP_PRINT_INFO(BANNER_1);
    APP_PRINT_INFO(BANNER_2, s_module);
    APP_PRINT_INFO(BANNER_3, s_version);
    APP_PRINT_INFO(BANNER_4, version.version_id_b.major,
                   version.version_id_b.minor,
                   version.version_id_b.patch);
    APP_PRINT_INFO(BANNER_5);
    APP_PRINT_INFO(BANNER_6, s_info);
    APP_PRINT_INFO(BANNER_7, s_module);
}
