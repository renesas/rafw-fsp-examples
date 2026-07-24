/***********************************************************************************************************************

* File Name    : logging.c

* Description  : Contains variables and functions used in logging.c.

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

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
