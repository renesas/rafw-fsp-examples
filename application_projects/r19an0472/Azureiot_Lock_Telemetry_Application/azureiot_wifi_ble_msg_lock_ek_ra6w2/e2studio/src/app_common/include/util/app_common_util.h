/**
 ****************************************************************************************
 *
 * @file app_common_util.h
 *
 * @brief common util for app
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

#include "oal.h"

#if !defined(_APP_COMMON_UTIL_H_)
#define _APP_COMMON_UTIL_H_

#if !defined(SYS_ASSERT)
#define SYS_ASSERT	configASSERT
#endif

/*! type define app mutex*/
typedef SemaphoreHandle_t app_mutex;

#if !defined(OAL_MSLEEP)
#define	OAL_MSLEEP(wtime)		{							\
			portTickType xFlashRate, xLastFlashTime;		\
			uint32_t mtime = wtime;							\
			if (wtime < 10) mtime = 1;						\
			xFlashRate = mtime/portTICK_RATE_MS;			\
			xLastFlashTime = xTaskGetTickCount();			\
			vTaskDelayUntil( &xLastFlashTime, xFlashRate );	\
}
#endif

/*! wrapping system sleep */
#if !defined(sleep)
#define sleep OAL_MSLEEP
#endif

/*! print function for APP debugging */
#if !defined(APRINTF)
#define APRINTF(...) PRINTF(__VA_ARGS__)
//#define APRINTF(...)
#endif
#if !defined(APRINTF_Y)
#define APRINTF_Y	APRINTF
#endif
#if !defined(APRINTF_I)
#define APRINTF_I	APRINTF
#endif
#if !defined(APRINTF_S)
#define APRINTF_S	APRINTF
#endif
#if !defined(APRINTF_E)
#define APRINTF_E	APRINTF
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ***************************************************************************************
 * @brief Hex value print for debugging
 * @param[in] _data Data address to print
 * @param[in] _length Data size
 * @return void
 ****************************************************************************************
 */
void app_view_hex(char *_data, int32_t _length);

/**
 ***************************************************************************************
 * @brief Hex value print for debugging
 * @param[in] _data Data address to print
 * @param[in] _length Data size
 * @param[in] _function Function name
 * @return void
 ****************************************************************************************
 */
void app_view_hex_func_name(char *_data, int32_t _length, const char *_function);

/**
 ***************************************************************************************
 * @brief Check passed time 
 * @param[in] formatted string for debug
 * @return void
 ****************************************************************************************
 */
void app_print_elapse_time_ms(const char *fmt, ...);

/**
 ***************************************************************************************
 * @brief Check current memory heap info
 * @param[in] formatted string for debug
 * @return void
 ****************************************************************************************
 */
void app_print_elapse_memory_info(const char *fmt, ...);

/**
 ***************************************************************************************
 * @brief mutex init
 * @param[in] _mutex  mutex structure address
 * @param[in] _function function name
 * @return void
 ****************************************************************************************
 */
void app_mutex_init(app_mutex *_mutex, char *_name_ptr);

/**
 ***************************************************************************************
 * @brief mutex lock
 * @param[in] _mutex  mutex structure address
 * @param[in] _function function name
 * @return 1 :success , other : error 
 ****************************************************************************************
 */
int32_t app_mutex_lock(app_mutex *_muxte);

/**
 ***************************************************************************************
 * @brief mutex unlock
 * @param[in] _mutex  mutex structure address
 * @param[in] _function function name
 * @return 1 :success , other : error 
 ****************************************************************************************
 */
int32_t app_mutex_unlock(app_mutex *_muxte);

/**
 ***************************************************************************************
 * @brief mutex delete
 * @param[in] _mutex  mutex structure address
 * @param[in] _function function name
 * @return 1 :success , other : error 
 ****************************************************************************************
 */
int32_t app_mutex_delete(app_mutex *_muxte);

#endif	// _APP_COMMON_UTIL_H_

/* EOF */
