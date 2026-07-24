/***********************************************************************************************************************
 * File Name    : i2c_slave.h
 * Description  : Contains data structures and functions used in i2c_slave.h/c
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/

#ifndef I2C_SLAVE_H_
#define I2C_SLAVE_H_


/* macro definition */
/* for on board LED */
#define LED_ON             BSP_IO_LEVEL_HIGH
#define LED_OFF            BSP_IO_LEVEL_LOW

/* MACRO for checking if two buffers are equal */
#define BUFF_EQUAL         (0U)

/* buffer size for slave and master data */
#define BUF_LEN            0x06

/* Human eye noticeable LED toggle delay */
#define TOGGLE_DELAY       0x3E8

#define EP_INFO    "\r\nThis EP demonstrates I2C slave operation using two I2C channels." \
        "\r\nIt performs Slave read and write operation continuously once initialization  " \
        "\r\nis successful. On successful I2C transaction(6 bytes), Data transceived is  "\
        "\r\ncompared. Led blinks on data match else it is turned ON as sign of failure. " \
        "\r\nFor both cases corresponding slave operation message is displayed on RTT. "\
        "\r\nAny API/event failure message is also displayed.\n\n\n\n"

/*
 * function declarations
 */
fsp_err_t init_i2c_driver(void);
fsp_err_t process_slave_WriteRead(void);
void deinit_i2c_driver(void);
void set_led(bsp_io_level_t led_state);


#endif /* I2C_SLAVE_H_ */
