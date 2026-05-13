/**
 ****************************************************************************************
 *
 * @file app_common_util.c
 *
 * @brief Define the common utility for apps module
 *
 * Copyright (c) 2026 Renesas Electronics. All rights reserved.
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

#include "app_common_memory.h"
#include "app_common_util.h"
#include "sdk_defs.h"
#include <stdarg.h>

#undef	TIME_CHECK_DBG //enable or disable of app_print_elapse_time_ms()

/*! getting current time after booting   */
extern unsigned long long get_fci_dpm_curtime();

/*! timestamp at last time   */
uint32_t previous_ts = 0;

/*! timestamp at first time   */
uint32_t first_ts = 0;

/*! heap size at last time   */
uint32_t previous_heap_size = 0;

/*! heap size at first time   */
uint32_t first_heap_size = 0;

#if defined (__SUPPORT_WIFI_PROVISIONING__)

void app_mutex_init(app_mutex *_mutex, char *_name_ptr)
{
    char result[128];

    sprintf(result, "app_mutex_%lu", (long unsigned int)_mutex);

    if (_name_ptr == NULL) {
        _name_ptr = result;
    }

    *_mutex = xSemaphoreCreateMutex();
    if (*_mutex == NULL) {
        PRINTF("[%s] tx_xmit_lock Semaphore Create Error!\n", __func__);
        SYS_ASSERT(0);
    }

}

int32_t app_mutex_lock(app_mutex *_mutex)
{
    long status;

    status = xSemaphoreTake(*_mutex, (TickType_t)portMAX_DELAY);
    if (status != pdTRUE) {
        SYS_ASSERT(0);
        return 0;
    }

    return 1;
}

int32_t app_mutex_unlock(app_mutex *_mutex)
{
    long status;

    status = xSemaphoreGive(*_mutex);
    if (status != pdTRUE) {
        SYS_ASSERT(0);
        return 0;
    }

    return 1;
}

int32_t app_mutex_delete(app_mutex *_mutex)
{
    vSemaphoreDelete(*_mutex);

    return 1;
}

#endif //(__SUPPORT_WIFI_PROVISIONING__)
//azure[[:
static UINT32 app_get_ms(void)
{
    uint32_t ulTimeMs = 0UL;
    TickType_t xTickCount = 0;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = pdTICKS_TO_MS( xTickCount );

    return ulTimeMs;
}
//]]
void app_print_elapse_time_ms(const char *fmt, ...)
{
    UINT32 currentTS = app_get_ms();
#if defined(TIME_CHECK_DBG)
    UINT32 followingTS = 0;
#endif  // TIME_CHECK_DBG
    va_list ap;
    char buf[160];

    if (first_ts == 0) {
        first_ts = currentTS;
    }

#if defined(TIME_CHECK_DBG)
    if (previous_ts != 0) {
        followingTS = currentTS - previous_ts;
    }
#endif  // TIME_CHECK_DBG

    if (fmt == NULL) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), (const char*)fmt, ap);
    va_end(ap);

#if defined(TIME_CHECK_DBG)
    APRINTF("\n%s ==> elapsed time: %u ms,  total time: %u ms \n", buf, 
            followingTS, currentTS - first_ts);
#endif

    previous_ts = currentTS;
}

void app_print_elapse_memory_info(const char *fmt, ...)
{
    UINT32 currentHeapSize = xPortGetFreeHeapSize();
    UINT32 followingHeapSize = 0;
    char plusFlag = 0;
    va_list ap;
    char buf[160];

    if (first_heap_size == 0) {
        first_heap_size = currentHeapSize;
    }

    if (previous_heap_size != 0) {
        if (previous_heap_size < currentHeapSize) {
            followingHeapSize = currentHeapSize - previous_heap_size;
            plusFlag = 1;
        } else if (previous_heap_size > currentHeapSize) {
            followingHeapSize = previous_heap_size - currentHeapSize;
            plusFlag = -1;
        } else {
            plusFlag = 0;
        }
    }

    va_start(ap, fmt);
    da16x_vsnprintf(buf, sizeof(buf), 0, (const char*)fmt, ap);
    va_end(ap);

    APRINTF("\n%s current heap size(=%d): %d %s\n", buf, currentHeapSize, followingHeapSize,
        plusFlag ? (plusFlag == 1 ? "increased" : "decreased") : "not changed");

    previous_heap_size = currentHeapSize;
}

void app_view_hex(char *_data, int32_t _length)
{
    for (int j = 0; j < _length; j++) {
        APRINTF("0x%02x,", _data[j] & 0xFF);
    }

    APRINTF(" length = %d \n", _length);
}

void app_view_hex_func_name(char *_data, int32_t _length, const char *_function)
{
    if (_data == NULL) {
        APRINTF("View hex Error. data is Null [%s]\n", _function);
        return;
    }

    if (_length <= 0) {
        APRINTF("View hex Error size [%s]  size = %d \n", _function, _length);
        return;
    }

    APRINTF("[%s] ==> View Hex value <==  string  :  ( %s )  , length = %d \n", _function, _data, _length);

    for (int j = 0; j < _length; j++) {
        if (j > 0 && j % 16 == 0) {
            APRINTF("\n\r");
        }

        APRINTF("0x%02x,", _data[j] & 0xFF);
    }

    APRINTF("\n");
}

/* EOF */
