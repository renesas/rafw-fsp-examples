/**
 ****************************************************************************************
 *
 * @file app_common_memory.c
 *
 * @brief Defines memory related APIs used to operate on Apps module.
 *
 *	Copyright (c) 2026 Renesas Electronics. All rights reserved.
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


#if defined (__SUPPORT_WIFI_PROVISIONING__)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "da16x_system.h"
#include "app_common_memory.h"
#include "app_common_util.h"

#if defined(__CHECK_MEMORY_USE__)

/*! stack buffer size for memory usage checking  */
#define MAX_MEMORY_CHECK 300

/*! structure for memory usage checking   */
typedef struct _memory_check_t {
    uint32_t size;
    int32_t addr;
} memory_check_t;

/*! memory used count    */
uint32_t used_memory = 0;
uint32_t malloc_count = 0;
memory_check_t memory_check[MAX_MEMORY_CHECK] = {0, };
#endif /* __CHECK_MEMORY_USE__ */

SemaphoreHandle_t memory_mutex;

int32_t mutex_init = 0;

void* app_internal_malloc(unsigned int _size)
{
    void *ret = NULL;

    ret = (void*)APP_MALLOC(_size);
    if (ret != NULL) {
        memset(ret, 0, _size);
        return ret;
    } else {
        APRINTF_E("------------ app_internal_malloc fail  ----------------\n ");
        SYS_ASSERT(0);
    }

    return ret;
}

void* app_malloc_debug(unsigned int _size)
{
    void *ret = NULL;

    ret = (void*)APP_MALLOC(_size);
    if (ret == NULL) {
        APRINTF_E("MEMORY_ALLOC_FAIL");
        SYS_ASSERT(0);
    }

    return ret;
}

void* app_malloc_check(unsigned int _size, const char *_func, int _line)
{
#if defined(__CHECK_MEMORY_USE__)

    if (mutex_init == 0) {
        app_mutex_init(&memory_mutex, "memory_mutex");
        mutex_init = 1;
    }
    app_mutex_lock(&memory_mutex);

    for (int i = 0; i < MAX_MEMORY_CHECK; i++) {

        if (memory_check[i].size == 0) {

            void *result = APP_MALLOC(_size);

            memory_check[i].size = _size;
            memory_check[i].addr = (int32_t)result;
            used_memory += memory_check[i].size;
            malloc_count++;

            if (119 == _size) {
                APRINTF("|Malloc");
            }

            app_mutex_unlock(&memory_mutex);

            return result;
        }

    }

    app_mutex_unlock(&memory_mutex);

    SYS_ASSERT(0);

#else

    DA16X_UNUSED_ARG(_size);
    DA16X_UNUSED_ARG(_func);
    DA16X_UNUSED_ARG(_line);

#endif // __CHECK_MEMORY_USE__

    return NULL;
}

void app_free_check(void *_ptr, const char *_func, int _line)
{
#if defined(__CHECK_MEMORY_USE__)

    if (_ptr == NULL) {
        APRINTF("|Free| Found  NULL addr = %x [%s : %d] \n", (int32_t )_ptr, _func, _line);
        return;
    }

    if (mutex_init == 0) {
        app_mutex_init(&memory_mutex, "memory_mutex");
        mutex_init = 1;
    }

    app_mutex_lock(&memory_mutex);

    for (int i = 0; i < MAX_MEMORY_CHECK; i++) {
        if (memory_check[i].addr == (int32_t)_ptr) {
            used_memory -= memory_check[i].size;
            malloc_count--;

            APP_FREE(_ptr);

            _ptr = NULL;
            memory_check[i].addr = 0;
            memory_check[i].size = 0;
            app_mutex_unlock(&memory_mutex);
            return;
        }

    }

    APRINTF("|Free| ERROR NO Addr [%d] Used =  %u , addr = %x [%s : %d]\n", malloc_count, used_memory, (int32_t )_ptr, _func,
        _line);
    app_mutex_unlock(&memory_mutex);
#else
    DA16X_UNUSED_ARG(_ptr);
    DA16X_UNUSED_ARG(_func);
    DA16X_UNUSED_ARG(_line);
#endif // __CHECK_MEMORY_USE__
}

void app_show_current_memory(void)
{
#if defined(__CHECK_MEMORY_USE__)
    app_mutex_lock(&memory_mutex);

    int32_t totalCount = 0;

    APRINTF("\app_show_current_memory  ========================================\n\n");

    for (int32_t i = 0; i < MAX_MEMORY_CHECK; i++) {
        if ((memory_check[i].size != 0) && (memory_check[i].addr != 0)) {
            APRINTF("|MEMORY|[%d]  addr = %x size %d \n", i, (int32_t )memory_check[i].addr, memory_check[i].size);
            totalCount++;
        }
    }

    APRINTF("\n\n totalCount = %d size =%d ================================\n\n", totalCount, used_memory);

    app_mutex_unlock(&memory_mutex);
#endif	// __CHECK_MEMORY_USE__
}

#endif	// __SUPPORT_WIFI_PROVISIONING__

/* EOF */
