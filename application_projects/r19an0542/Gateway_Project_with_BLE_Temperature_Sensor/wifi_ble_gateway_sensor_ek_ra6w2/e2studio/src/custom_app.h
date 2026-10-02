/***********************************************************************************************************************
 * File Name    : custom_app.h
 * Description  : Public interface for BLE advertisement parsing and MQTT client initialisation.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#ifndef CUSTOM_APP_H_
#define CUSTOM_APP_H_

#include "rm_ble_abs_api.h"

void advRptParse(void * pdata);
void vd_ble_start_scan(void);
void mqtt_initialize(void);

#endif /* CUSTOM_APP_H_ */
