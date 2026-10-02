/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
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
