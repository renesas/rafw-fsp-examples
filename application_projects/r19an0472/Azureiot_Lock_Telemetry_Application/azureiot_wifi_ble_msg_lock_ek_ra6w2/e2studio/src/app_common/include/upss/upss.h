/*******************************************************************************
 *
 * @file upss.h
 *
 * @brief upss header file
 *
 * @brief Configuration for SDK Version
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
 *******************************************************************************/

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
