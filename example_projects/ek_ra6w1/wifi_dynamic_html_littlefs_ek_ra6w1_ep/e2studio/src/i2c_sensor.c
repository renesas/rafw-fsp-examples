/***********************************************************************************************************************
 * File Name    : i2c_sensor.c
 * Description  : Contains data structures and functions used in i2c_sensor.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "i2c_sensor.h"
#include "r_i2c_master_api.h"
#include "r_i2c_master_w.h"
#include "hal_data.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile i2c_master_event_t i2c_event = (i2c_master_event_t)RESET_VALUE;

static fsp_err_t get_device_id(uint8_t *dev_id);
static fsp_err_t validate_i2c_event(void);

fsp_err_t init_sensor(void)
{
    uint8_t device_id     = RESET_VALUE;
    fsp_err_t err         = FSP_SUCCESS;
    uint8_t reset_data[2] = { SOFTRESET, 0xB6 };
    uint8_t ctrl_data[2]  = { REGISTER_CONTROL, 0x3F };

    err = R_I2C_MASTER_W_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
    if (FSP_SUCCESS != err)
        return err;

    err = get_device_id(&device_id);
    if (FSP_ERR_TRANSFER_ABORTED == err)
    {
        APP_PRINT_INFO("[sensor] 0x76 NACK — probing alternate address 0x77 (SDO=VCC)\n");
        R_I2C_MASTER_W_SlaveAddressSet(&g_i2c_master0_ctrl, DEVICE_ALT_ADDR,
                                       I2C_MASTER_ADDR_MODE_7BIT);
        device_id = RESET_VALUE;
        err = get_device_id(&device_id);
    }
    APP_PRINT_INFO("[sensor] get_device_id ret=%d  device_id=0x%02X (expect 0x%02X)\n",
                  (int)err, device_id, DEVICE_SIGNATURE);

    if (FSP_SUCCESS != err)
    {
        APP_PRINT_ERR("[sensor] get_device_id failed: %d\n", (int)err);
        R_I2C_MASTER_W_Close(&g_i2c_master0_ctrl);
        return err;
    }
    if (DEVICE_SIGNATURE != device_id)
    {
        APP_PRINT_ERR("[sensor] Wrong device ID 0x%02X — check SDO pin (0x76=GND, 0x77=VCC)\n",
                      device_id);
        R_I2C_MASTER_W_Close(&g_i2c_master0_ctrl);
        return FSP_ERR_NOT_FOUND;
    }

    /* reset */
    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, reset_data, TWO_BYTE, false);
    if (FSP_SUCCESS == err)
        err = validate_i2c_event();
    if (FSP_SUCCESS != err)
    {
        APP_PRINT_ERR("[sensor] Soft reset write failed: %d\n", (int)err);
        R_I2C_MASTER_W_Close(&g_i2c_master0_ctrl);
        return err;
    }
    APP_PRINT_INFO("[sensor] Soft reset OK\n");

    /* BME280 needs up to 2 ms after soft reset — use 10 ms to be safe */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Enable humidity oversampling x1 — MUST be written before ctrl_meas */
    uint8_t hum_data[2]  = { REGISTER_CTRL_HUM, 0x01 };
    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, hum_data, TWO_BYTE, false);
    if (FSP_SUCCESS == err)
        err = validate_i2c_event();
    if (FSP_SUCCESS != err)
    {
        APP_PRINT_ERR("[sensor] ctrl_hum write failed: %d\n", (int)err);
        R_I2C_MASTER_W_Close(&g_i2c_master0_ctrl);
        return err;
    }
    APP_PRINT_INFO("[sensor] ctrl_hum (0xF2=0x01) written OK — humidity osrs x1\n");

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, ctrl_data, TWO_BYTE, false);
    if (FSP_SUCCESS == err)
        err = validate_i2c_event();
    if (FSP_SUCCESS != err)
        APP_PRINT_ERR("[sensor] ctrl_meas write failed: %d\n", (int)err);
    else
        APP_PRINT_INFO("[sensor] ctrl_meas (0xF4=0x3F) written OK — sensor in normal mode\n");

    return err;
}

void deinit_sensor(void)
{
    R_I2C_MASTER_W_Close(&g_i2c_master0_ctrl);
}

fsp_err_t read_sensor_data(uint8_t *xyz_data)
{
    fsp_err_t err        = FSP_SUCCESS;
    uint8_t temp[SENSOR_DATA_SIZE] = { RESET_VALUE };
    uint8_t internal_reg = TEMP_REG;

    if (NULL == xyz_data)
        return FSP_ERR_INVALID_POINTER;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, &internal_reg, ONE_BYTE, true);
    if (FSP_SUCCESS != err) return err;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return err;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Read(&g_i2c_master0_ctrl, &temp[0], DATA_REGISTERS, false);
    if (FSP_SUCCESS != err) return err;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return err;

    APP_PRINT_INFO("[sensor] raw[0..2] = 0x%02X 0x%02X 0x%02X\n",
                  temp[0], temp[1], temp[2]);

    for (uint8_t itr = RESET_VALUE; itr < SENSOR_DATA_SIZE; itr++)
        *(xyz_data + itr) = temp[itr];

    return err;
}

uint16_t read_reg_data(uint8_t addr)
{
    fsp_err_t err        = FSP_SUCCESS;
    uint8_t data[2]      = { RESET_VALUE };
    uint8_t internal_reg = addr;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, &internal_reg, ONE_BYTE, true);
    if (FSP_SUCCESS != err) return 0;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return 0;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Read(&g_i2c_master0_ctrl, &data[0], 0x2, false);
    if (FSP_SUCCESS != err) return 0;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return 0;

    APP_PRINT_INFO("[sensor] reg[0x%02X] = 0x%02X 0x%02X -> 0x%04X\n",
                  addr, data[0], data[1], (uint16_t)(data[0] | (data[1] << 8)));
    return (uint16_t)(data[0] | (data[1] << 8));
}

static fsp_err_t get_device_id(uint8_t *dev_id)
{
    fsp_err_t err       = FSP_SUCCESS;
    uint8_t reg_address = DEVICE_ID_REG;

    if (NULL == dev_id)
        return FSP_ERR_INVALID_POINTER;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, &reg_address, ONE_BYTE, true);
    if (FSP_SUCCESS != err) return err;

    err = validate_i2c_event();
    if (FSP_ERR_TRANSFER_ABORTED == err) return err;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Read(&g_i2c_master0_ctrl, dev_id, ONE_BYTE, false);
    if (FSP_SUCCESS != err) return err;

    return validate_i2c_event();
}

void g_i2c_master_w0_callback(i2c_master_callback_args_t *p_args)
{
    if (NULL != p_args)
        i2c_event = p_args->event;
}

/* Read 2-byte raw humidity from 0xFD–0xFE.
 * Returns FSP_SUCCESS and sets *raw_h on success.
 * If the sensor returns 0x8000 humidity measurement was skipped
 * (ctrl_hum not written before ctrl_meas). */
fsp_err_t read_humidity_raw(uint16_t *raw_h)
{
    fsp_err_t err        = FSP_SUCCESS;
    uint8_t   data[2]    = { RESET_VALUE };
    uint8_t   reg        = REGISTER_HUM_MSB;

    if (NULL == raw_h)
        return FSP_ERR_INVALID_POINTER;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Write(&g_i2c_master0_ctrl, &reg, ONE_BYTE, true);
    if (FSP_SUCCESS != err) return err;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return err;

    i2c_event = (i2c_master_event_t)RESET_VALUE;
    err = R_I2C_MASTER_W_Read(&g_i2c_master0_ctrl, data, TWO_BYTE, false);
    if (FSP_SUCCESS != err) return err;

    err = validate_i2c_event();
    if (FSP_SUCCESS != err) return err;

    *raw_h = (uint16_t)((data[0] << 8) | data[1]);
    APP_PRINT_INFO("[sensor] hum raw = 0x%04X\n", *raw_h);
    return FSP_SUCCESS;
}

static fsp_err_t validate_i2c_event(void)
{
    const TickType_t poll_delay = (pdMS_TO_TICKS(10U) > 0U) ? pdMS_TO_TICKS(10U) : 1U;

    for (uint32_t iter = 0U; iter < 50U; iter++)
    {
        if (i2c_event != (i2c_master_event_t)RESET_VALUE)
        {
            if (i2c_event != I2C_MASTER_EVENT_ABORTED)
            {
                i2c_event = (i2c_master_event_t)RESET_VALUE;
                return FSP_SUCCESS;
            }
            /* Callback fired with ABORTED: sensor NACKed the address/data.
             * This means the IRQ IS working — check slave address or power. */
            APP_PRINT_ERR("[i2c] NACK received (sensor not ACKing — check address/power)\n");
            i2c_event = (i2c_master_event_t)RESET_VALUE;
            return FSP_ERR_TRANSFER_ABORTED;
        }
        vTaskDelay(poll_delay);
    }

    APP_PRINT_ERR("[i2c] Timeout — callback never fired in 500 ms (check IRQ/pin config)\n");
    i2c_event = (i2c_master_event_t)RESET_VALUE;
    return FSP_ERR_TRANSFER_ABORTED;
}
