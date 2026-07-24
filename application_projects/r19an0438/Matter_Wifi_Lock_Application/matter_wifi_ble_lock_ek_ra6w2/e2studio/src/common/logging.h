/***********************************************************************************************************************

* File Name    : logging.h

* Description  : Contains variables and functions used in logging.h.

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/
#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>

#define LOG_COLOR_OUTPUT 1

#if LOG_COLOR_OUTPUT
# define COLLOR_ERROR "\x1b[31m"
# define COLLOR_WARN  "\x1b[1m"
# define COLLOR_INFO  "\x1b[36m"
# define COLLOR_TRACE "\x1b[4m"
# define COLLOR_DEBUG "\x1b[35m"
# define COLLOR_RESET "\x1b[0m"
#else
# define COLLOR_ERROR
# define COLLOR_WARN
# define COLLOR_INFO
# define COLLOR_TRACE
# define COLLOR_DEBUG
# define COLLOR_RESET
#endif

#define LOG_ERROR(msg, ...) { if (log_level_get() >= LOG_LEVEL_ERROR) printf(COLLOR_ERROR msg COLLOR_RESET, ##__VA_ARGS__); }
#define LOG_WARN( msg, ...) { if (log_level_get() >= LOG_LEVEL_WARN ) printf(COLLOR_WARN  msg COLLOR_RESET, ##__VA_ARGS__); }
#define LOG_INFO( msg, ...) { if (log_level_get() >= LOG_LEVEL_INFO ) printf(COLLOR_INFO  msg COLLOR_RESET, ##__VA_ARGS__); }

#define LOG_TRACE(msg, ...) {             \
  if (log_level_get() >= LOG_LEVEL_TRACE) \
  printf(COLLOR_TRACE "Func: %s, line: %d:" COLLOR_RESET " " msg, __FUNCTION__, __LINE__, ##__VA_ARGS__); }

#define LOG_DEBUG(msg, ...) { if (log_level_get() >= LOG_LEVEL_DEBUG) printf(COLLOR_DEBUG msg COLLOR_RESET, ##__VA_ARGS__); }

typedef enum
{
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN ,
    LOG_LEVEL_INFO ,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
} LOG_LEVEL_t;

LOG_LEVEL_t log_level_get();
void log_level_set(LOG_LEVEL_t level);

#endif // __LOGGING_H__

