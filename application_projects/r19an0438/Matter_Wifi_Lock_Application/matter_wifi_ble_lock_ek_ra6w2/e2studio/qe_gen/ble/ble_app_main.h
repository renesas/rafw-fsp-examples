/*
 * ble_app_main.h
 *
 *  Created on: Feb 3, 2025
 *      Author: justinsongzj
 */
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef BLE_APP_MAIN_H_
#define BLE_APP_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "r_ble_api.h"

/******************************************************************************
 * Function Name: app_start_ble_main
 * Description  : Create BLE main thread.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void app_start_ble_main(void);

/******************************************************************************
 * Function Name: app_stop_ble_main
 * Description  : Delete BLE main thread.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void app_stop_ble_main(void);

/******************************************************************************
 * Function Name: ble_profile_init
 * Description  : Initialize BLE and profiles.
 * Arguments    : None
 * Return Value : BLE_SUCCESS - SUCCESS
 *                BLE_ERR_INVALID_OPERATION -
 *                    Failed to initialize BLE or profiles.
 ******************************************************************************/
ble_status_t ble_profile_init(void);

/******************************************************************************
 * Function Name: ble_adv_start
 * Description  : BLE advertising start.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void ble_adv_start(void);

/******************************************************************************
 * Function Name: ble_adv_stop
 * Description  : BLE advertising stop.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void ble_adv_stop(void);

/******************************************************************************
 * Function Name: ble_close_connection
 * Description  : BLE close connection.
 * Arguments    : None
 * Return Value : None
 ******************************************************************************/
void ble_close_connection(void);

#ifdef __cplusplus
}
#endif
#endif /* BLE_APP_MAIN_H_ */
