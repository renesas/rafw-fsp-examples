/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
#if !defined(_APP_COMMON_MEMORY_H_)
#define _APP_COMMON_MEMORY_H_

//#include "driver.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief memory allocation for debugging
 * @param[in] allocation size
 * @param[in] function name for debugging
 * @param[in] function line for debugging
 * @return memory allocated address
 ****************************************************************************************
 */
void* app_malloc_check(unsigned int _size, const char *_func, int _line);

/**
 ****************************************************************************************
 * @brief memory free for debugging
 * @param[in] address to free
 * @param[in] function name for debugging
 * @param[in] function line for debugging
 * @return void
 ****************************************************************************************
 */
void app_free_check(void *_ptr, const char *_func, int _line);

/**
 ****************************************************************************************
 * @brief show current memory usage for debugging
 * @return void
 ****************************************************************************************
 */
void app_show_current_memory(void);

/**
 ****************************************************************************************
 * @brief memory allocation without memset 0 for debuging at be failed
 * @param[in] allocation size
 * @return memory allocated address
 ****************************************************************************************
 */
void* app_malloc_debug(unsigned int _size);

/**
 ****************************************************************************************
 * @brief memory allocation with memset 0
 * @param[in] allocation size
 * @return memory allocated address
 ****************************************************************************************
 */
void* app_internal_malloc(unsigned int _size);

/**
 ****************************************************************************************
 * @brief memory allocation with memset 0
 * @param[in] allocation size
 * @return memory allocated address
 ****************************************************************************************
 */

/*! for checking memory usage  */
#undef __CHECK_MEMORY_USE__

#if !defined(__CHECK_MEMORY_USE__)
/*! for using APP memory as like "APP_MALLOC " */
#undef __USING_APP_MEMORY__
#endif

#if defined(__CHECK_MEMORY_USE__)
#define app_common_malloc(x) app_malloc_check(x,__func__,__LINE__)
#define app_common_free(x) app_free_check(x,__func__,__LINE__)

#else

#if defined(__USING_APP_MEMORY__)

#define app_common_safe_free(a) if(a!=NULL){APP_FREE(a); a=NULL;}
#define app_common_malloc(x) app_internal_malloc(x)
#define app_common_free(x) app_common_safe_free(x)

#else
/*! memory malloc API for APP  */
#define app_common_malloc(x) APP_MALLOC(x)
/*! memory free API for APP  */
#define app_common_free(x) APP_FREE(x)

#endif /* __USING_APP_MEMORY__ */
#endif /* __CHECK_MEMORY_USE__ */

#endif /* _APP_COMMON_MEMORY_H_ */

/* EOF */
