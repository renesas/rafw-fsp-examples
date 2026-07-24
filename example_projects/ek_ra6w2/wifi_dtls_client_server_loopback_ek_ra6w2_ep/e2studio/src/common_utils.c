/***********************************************************************************************************************
 * File Name    : common_utils.c
 * Description  : Contains macros, data structures and functions used common to the EP
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#include "bsp_api.h"
#include "common_utils.h"

#define BANNER_1    "******************************************************************\n\r"
#define BANNER_2    "*   Renesas FSP Example Project for %s Module\n\r"
#define BANNER_3    "*   Example Project Version %s\n\r"
#define BANNER_4    "*   Flex Software Pack Version %d.%d.%d\n\r"
#define BANNER_5    "******************************************************************\n\r"
#define BANNER_6    "Refer to readme.txt file for more details on Example Project and " \
                    "FSP User's Manual for more information about %s driver\n\n\r"

/*******************************************************************************************************************//**
 * @brief       Print Example Project banner information
 * @param[in]   s_module   Module name
 * @param[in]   s_version  Example version
 * @param[in]   s_info     Example description
 * @retval      None
 **********************************************************************************************************************/
void print_ep_info_banner(const char *s_module, const char *s_version, const char *s_info)
{
    FSP_PARAMETER_NOT_USED(s_info);

    fsp_pack_version_t version;

    R_FSP_VersionGet(&version);

    APP_PRINT(BANNER_1);
    APP_PRINT(BANNER_2, s_module);
    APP_PRINT(BANNER_3, s_version);
    APP_PRINT(BANNER_4,
              version.version_id_b.major,
              version.version_id_b.minor,
              version.version_id_b.patch);
    APP_PRINT(BANNER_5);
    APP_PRINT(BANNER_6, s_module);
}
