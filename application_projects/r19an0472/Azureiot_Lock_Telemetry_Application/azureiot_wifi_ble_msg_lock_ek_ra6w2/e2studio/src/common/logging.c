#include <stdlib.h>

#include "logging.h"

static LOG_LEVEL_t log_level_value = LOG_LEVEL_INFO;

LOG_LEVEL_t log_level_get() {
    return log_level_value;
}

void log_level_set(LOG_LEVEL_t level) {
    log_level_value = level;
}
