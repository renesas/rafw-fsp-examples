/***********************************************************************************************************************
 * File Name    : i2c_sensor.h
 * Description  : Contains data structures and functions used in i2c_sensor.h/c
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/


#ifndef I2C_SENSOR_H_
#define I2C_SENSOR_H_
#include "bsp_api.h"
#include "common_utils.h"

#define ONE_BYTE                0x01
#define TWO_BYTE                0x02

#define MEASURE_PAYLOAD_SIZE    0x03        //measurement enable data length
#define ACCELERO_DELAY          0xC8
#define SENSOR_READ_DELAY       0x03
#define ENABLE_BIT              0x08
#define DATA_REGISTERS          0x03

/* Accelerometer internal register whichever consumed here */
#define DEVICE_ID_REG           0xD0
#define DEVICE_SIGNATURE        0x60
#define DEVICE_ALT_ADDR         0x77    /* BME280 alternate address when SDO=VCC */
#define POWER_CTL_REG           0x2D
#define AXIS_DATA               0x32
#define TEMP_REG                0xFA
#define SOFTRESET               0xE0
#define REGISTER_CONTROL        0xF4
#define REGISTER_DIG_T1         0x88
#define REGISTER_DIG_T2         0x8A
#define REGISTER_DIG_T3         0x8C

/* Humidity calibration registers */
#define REGISTER_DIG_H1         0xA1    /* unsigned, 1 byte */
#define REGISTER_DIG_H2         0xE1    /* signed 16-bit, little-endian */
#define REGISTER_DIG_H3         0xE3    /* unsigned, 1 byte */
#define REGISTER_DIG_H4         0xE4    /* signed 12-bit: [11:4]=0xE4, [3:0]=0xE5[3:0] */
#define REGISTER_DIG_H5         0xE5    /* signed 12-bit: [11:4]=0xE6, [3:0]=0xE5[7:4] */
#define REGISTER_DIG_H6         0xE7    /* signed, 1 byte */

/* Humidity control and raw data registers */
#define REGISTER_CTRL_HUM       0xF2    /* osrs_h — must be written BEFORE ctrl_meas */
#define REGISTER_HUM_MSB        0xFD    /* humidity raw MSB */

#define SENSOR_DATA_SIZE        0x03
#define RESET_VALUE             (0x00)

/*
 * function declarations
 */
fsp_err_t init_sensor(void);
void deinit_sensor(void);
fsp_err_t read_sensor_data(uint8_t *xyz_data);
uint16_t read_reg_data(uint8_t addr);
fsp_err_t read_humidity_raw(uint16_t *raw_h);



#endif /* I2C_SENSOR_H_ */
