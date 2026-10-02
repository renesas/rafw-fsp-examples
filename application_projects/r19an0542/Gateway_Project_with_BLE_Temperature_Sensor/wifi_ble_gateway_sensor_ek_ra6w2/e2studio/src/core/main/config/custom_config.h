/**
 ****************************************************************************************
 *
 * @file custom_config.h
 *
 * @brief Board Support Package. User Configuration file.
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

#ifndef CUSTOM_CONFIG__H_
#define CUSTOM_CONFIG__H_

/* Use SEGGER"s RTT(Real Time Terminal) */
#define CONFIG_RTT
#if defined(__SUPPORT_AWS_IOT__) || defined(__SUPPORT_AZURE_IOT__) //APP_CORE::for expanding heap
#undef CONFIG_RTT
#endif


/* ----------------------------- Segger System View configuration ------------------------------- */

/**
 * \brief Segger's System View
 *
 * When enabled the application should also call SEGGER_SYSVIEW_Conf() to enable system monitoring.
 * configTOTAL_HEAP_SIZE should be increased by dg_configSYSTEMVIEW_STACK_OVERHEAD bytes for each system task.
 * For example, if there are 8 system tasks configTOTAL_HEAP_SIZE should be increased by
 * (8 * dg_configSYSTEMVIEW_STACK_OVERHEAD) bytes.
 *
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app,}
 *
 */
#ifndef dg_configSYSTEMVIEW
#define dg_configSYSTEMVIEW                     (0)
#endif

#endif /* CUSTOM_CONFIG__H_ */

/* EOF */
