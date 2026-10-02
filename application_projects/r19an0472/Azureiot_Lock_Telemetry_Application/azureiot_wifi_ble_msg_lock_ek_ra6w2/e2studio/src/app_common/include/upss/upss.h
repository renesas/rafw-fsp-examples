/**********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *********************************************************************************************************************/
#ifndef __UPSS_H__
#define __UPSS_H__

#define DC_DC_STR_REG							(0x50091044)
#define DC_DC_STR_VALUE 				        (0x2c031710)
#define DC_DC_STR_READ_SET_VALUE 		        (0x0c031710)
#define DC_DC_STR_ORG_VALUE 					(0x20031710)
#define DC_DC_STR_READ_SET_ORG_VALUE 			(0x00031710)

/**
 * \brief  Starting power save mode on RTOS running 
 *
 * \note   After done AP connection, It should be called for power saving with UPSS feature
 *
 * \return 0 if successful
 */
int APP_Power_Save_Start(void);

/**
 * \brief  Stoping power save mode on RTOS running 
 *
 * \note   When needing scan a frequency or AP connection like going to sleep mode 2, It should be called
 *
 * \return 0 if successful
 */
int APP_Power_Save_Stop(void);

/**
 * \brief  Suppress in-rush current when boot or wakeup 
 *
 * \note   It should be called for suppressing in-rush current with COINCELL feature
 *
 * \return void
 */
void APP_Inrush_Suppress_Enable(void);

/**
 * \brief  Recover in-rush current setting when reboot_func() called 
 *
 * \note   It should be called with COINCELL feature
 *
 * \return void
 */
void APP_Inrush_Suppress_Disable(void);

/**
 * \brief  Read the current register value related in-rush
 *
 * \note   It should be called with COINCELL feature
 *
 * \return 4 Bytes register value
 */
unsigned int APP_Inrush_Reg_Read(void);

#endif //__UPSS_H__
