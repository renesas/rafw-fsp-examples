/***********************************************************************************************************************
 * File Name    : app_dpm_thread.h
 * Description  : Header file to support the source file app_dpm_thread.c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#if !defined(_APP_DPM_THREAD_H_)
#define _APP_DPM_THREAD_H_

#include "rm_aws_lwip_sock_wrap_w_api.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 * @brief Sets the sleep mode and it's facotors on sleep mode 1/2/3(DPM)
 *
 * @param [in] _mode The value of one of the APPSleepMode enums
 * @param [in] _sec The value of wakeup timer interval by seconds
 * @param [in] _retention 0 or 1 only used in case of sleep mode 1/2
 *
 * @return void
 *
 */
void setSleepMode(APPSleepMode _mode, int _sec, unsigned char _retention);

/**
 * @brief Gets the current sleep mode
 *
 * @return APPSleepMode type value
 *
 */
APPSleepMode getSleepMode(void);

#endif /* _APP_DPM_THREAD_H_ */

