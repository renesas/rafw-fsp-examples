/***********************************************************************************************************************
 * File Name    : logging.c
 * Description  : Defines common logging-level control functions used across the application.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include <stdlib.h>

#include "logging.h"

static LOG_LEVEL_t log_level_value = LOG_LEVEL_INFO;

LOG_LEVEL_t log_level_get()
{
    return log_level_value;
}

void log_level_set(LOG_LEVEL_t level)
{
    log_level_value = level;
}
