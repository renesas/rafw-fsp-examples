/**
 ****************************************************************************************
 *
 * @file app_atcmd_features.c
 *
 * @brief Peripheral code for Apps at-command
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

//#include "gpio.h"

#if defined (__SUPPORT_ATCMD__) && (defined (__SUPPORT_AZURE_IOT__) || defined (__SUPPORT_AWS_IOT__))

#include "app_atcommand.h"
static UINT8 support_feature;
extern INT32 _GPIO_RETAIN_HIGH(UINT32 gpio_port, UINT32 gpio_num);
extern INT32 _GPIO_RETAIN_LOW(UINT32 gpio_port, UINT32 gpio_num);

int app_atcmd_gpio_control(UINT32 port, UINT32 pins, UINT32 set)
{
    int ret = 0;
    HANDLE gpio_handle;
    UINT16 data;

    /* pin_mux for gpio */
    if ((port == GPIO_UNIT_A) && (pins & 0x3))
        _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc))
        _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x30))
        _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc0))
        _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x300))
        _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc00))
        _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
    /* if ((port == GPIO_UNIT_A) && (pins & 0x8000))
     _da16x_io_pinmux(PIN_HMUX, HMUX_ADGPIO); */
    if ((port == GPIO_UNIT_C) && (pins & 0x1c0))
        _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);

    gpio_handle = GPIO_CREATE(port);

    GPIO_INIT(gpio_handle);

    /* output */
    GPIO_IOCTL(gpio_handle, GPIO_SET_OUTPUT, &pins);

    if (set) {
        data = pins;
        GPIO_WRITE(gpio_handle, pins, &data, sizeof(UINT16));
    } else {
        data = 0;
        GPIO_WRITE(gpio_handle, pins, &data, sizeof(UINT16));
    }

    GPIO_CLOSE(gpio_handle);

    return ret;
}

int app_atcmd_gpio_read(UINT32 port, UINT32 pins)
{
    HANDLE gpio_handle;
    UINT16 data;

    /* pin_mux for gpio */
    if ((port == GPIO_UNIT_A) && (pins & 0x3))
        _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc))
        _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x30))
        _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc0))
        _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x300))
        _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc00))
        _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x8000))
        _da16x_io_pinmux(PIN_HMUX, HMUX_ADGPIO);
    if ((port == GPIO_UNIT_C) && (pins & 0x1c0))
        _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);

    gpio_handle = GPIO_CREATE(port);

    GPIO_INIT(gpio_handle);

    GPIO_READ(gpio_handle, pins, &data, sizeof(UINT16));

    GPIO_CLOSE(gpio_handle);

    return data;
}

int app_atcmd_gpio_retention(UINT32 port, UINT32 pins, UINT32 set)
{
    int ret = 0;

    /* pin_mux for gpio */
    if ((port == GPIO_UNIT_A) && (pins & 0x3))
        _da16x_io_pinmux(PIN_AMUX, AMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc))
        _da16x_io_pinmux(PIN_BMUX, BMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x30))
        _da16x_io_pinmux(PIN_CMUX, CMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc0))
        _da16x_io_pinmux(PIN_DMUX, DMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x300))
        _da16x_io_pinmux(PIN_EMUX, EMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0xc00))
        _da16x_io_pinmux(PIN_FMUX, FMUX_GPIO);
    if ((port == GPIO_UNIT_A) && (pins & 0x8000))
        _da16x_io_pinmux(PIN_HMUX, HMUX_ADGPIO);
    if ((port == GPIO_UNIT_C) && (pins & 0x1c0))
        _da16x_io_pinmux(PIN_UMUX, UMUX_GPIO);

    if (set)
        _GPIO_RETAIN_HIGH(port, pins);
    else
        _GPIO_RETAIN_LOW(port, pins);

    return ret;
}

int app_atcmd_gpio_save_pullup(UINT32 port, UINT32 pins)
{
    int ret = 0;

    SAVE_PULLUP_PINS_INFO(port, pins);

    return ret;
}

int app_atcmd_user_pinmuxA(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = AMUX_GPIO;

    if (strcmp(pin_config, "AMUX_AD12") == 0)
        pin_feature = AMUX_AD12;
    else if (strcmp(pin_config, "AMUX_SPIs") == 0)
        pin_feature = AMUX_SPIs;
    else if (strcmp(pin_config, "AMUX_I2S") == 0)
        pin_feature = AMUX_I2S;
    else if (strcmp(pin_config, "AMUX_I2Cs") == 0)
        pin_feature = AMUX_I2Cs;
    else if (strcmp(pin_config, "AMUX_UART1d") == 0)
        pin_feature = AMUX_UART1d;
    else if (strcmp(pin_config, "AMUX_I2Cm") == 0)
        pin_feature = AMUX_I2Cm;
    else if (strcmp(pin_config, "AMUX_5GC") == 0)
        pin_feature = AMUX_5GC;
    else if (strcmp(pin_config, "AMUX_ADGPIO") == 0)
        pin_feature = AMUX_ADGPIO;
    else if (strcmp(pin_config, "AMUX_ADWRP") == 0)
        pin_feature = AMUX_ADWRP;
    else if (strcmp(pin_config, "AMUX_GPIO") == 0)
        pin_feature = AMUX_GPIO;
    else if (strcmp(pin_config, "AMUX_GPIOALT") == 0)
        pin_feature = AMUX_GPIOALT;

    _da16x_io_pinmux(PIN_AMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxB(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = BMUX_GPIO;

    if (strcmp(pin_config, "BMUX_AD12") == 0)
        pin_feature = BMUX_AD12;
    else if (strcmp(pin_config, "BMUX_SPIs") == 0)
        pin_feature = BMUX_SPIs;
    else if (strcmp(pin_config, "BMUX_I2S") == 0)
        pin_feature = BMUX_I2S;
    else if (strcmp(pin_config, "BMUX_I2Cs") == 0)
        pin_feature = BMUX_I2Cs;
    else if (strcmp(pin_config, "BMUX_UART1d") == 0)
        pin_feature = BMUX_UART1d;
    else if (strcmp(pin_config, "BMUX_ADCGPIO") == 0)
        pin_feature = BMUX_ADCGPIO;
    else if (strcmp(pin_config, "BMUX_ADCI2S") == 0)
        pin_feature = BMUX_ADCI2S;
    else if (strcmp(pin_config, "BMUX_GPIO") == 0)
        pin_feature = BMUX_GPIO;
    else if (strcmp(pin_config, "BMUX_GPIOALT") == 0)
        pin_feature = BMUX_GPIOALT;

    _da16x_io_pinmux(PIN_BMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxC(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = CMUX_GPIO;

    if (strcmp(pin_config, "CMUX_5GC") == 0)
        pin_feature = CMUX_5GC;
    else if (strcmp(pin_config, "CMUX_I2Cs") == 0)
        pin_feature = CMUX_I2Cs;
    else if (strcmp(pin_config, "CMUX_SDm") == 0)
        pin_feature = CMUX_SDm;
    else if (strcmp(pin_config, "CMUX_SDs") == 0)
        pin_feature = CMUX_SDs;
    else if (strcmp(pin_config, "CMUX_UART1c") == 0)
        pin_feature = CMUX_UART1c;
    else if (strcmp(pin_config, "CMUX_I2Cm") == 0)
        pin_feature = CMUX_I2Cm;
    else if (strcmp(pin_config, "CMUX_UART1d") == 0)
        pin_feature = CMUX_UART1d;
    else if (strcmp(pin_config, "CMUX_I2S") == 0)
        pin_feature = CMUX_I2S;
    else if (strcmp(pin_config, "CMUX_GPIO") == 0)
        pin_feature = CMUX_GPIO;
    else if (strcmp(pin_config, "CMUX_GPIOALT") == 0)
        pin_feature = CMUX_GPIOALT;

    _da16x_io_pinmux(PIN_CMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxD(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = DMUX_GPIO;

    if (strcmp(pin_config, "DMUX_SPIs") == 0)
        pin_feature = DMUX_SPIs;
    else if (strcmp(pin_config, "DMUX_SDm") == 0)
        pin_feature = DMUX_SDm;
    else if (strcmp(pin_config, "DMUX_SDs") == 0)
        pin_feature = DMUX_SDs;
    else if (strcmp(pin_config, "DMUX_UART1d") == 0)
        pin_feature = DMUX_UART1d;
    else if (strcmp(pin_config, "DMUX_I2Cs") == 0)
        pin_feature = DMUX_I2Cs;
    else if (strcmp(pin_config, "DMUX_SPIm") == 0)
        pin_feature = DMUX_SPIm;
    else if (strcmp(pin_config, "DMUX_I2S") == 0)
        pin_feature = DMUX_I2S;
    else if (strcmp(pin_config, "DMUX_GPIO") == 0)
        pin_feature = DMUX_GPIO;
    else if (strcmp(pin_config, "DMUX_GPIOALT") == 0)
        pin_feature = DMUX_GPIOALT;

    _da16x_io_pinmux(PIN_DMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxE(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = EMUX_GPIO;

    if (strcmp(pin_config, "EMUX_5GC") == 0)
        pin_feature = EMUX_5GC;
    else if (strcmp(pin_config, "EMUX_SPIs") == 0)
        pin_feature = EMUX_SPIs;
    else if (strcmp(pin_config, "EMUX_SDm") == 0)
        pin_feature = EMUX_SDm;
    else if (strcmp(pin_config, "EMUX_SDs") == 0)
        pin_feature = EMUX_SDs;
    else if (strcmp(pin_config, "EMUX_I2Cm") == 0)
        pin_feature = EMUX_I2Cm;
    else if (strcmp(pin_config, "EMUX_BT") == 0)
        pin_feature = EMUX_BT;
    else if (strcmp(pin_config, "EMUX_SPIm") == 0)
        pin_feature = EMUX_SPIm;
    else if (strcmp(pin_config, "EMUX_I2S") == 0)
        pin_feature = EMUX_I2S;
    else if (strcmp(pin_config, "EMUX_GPIO") == 0)
        pin_feature = EMUX_GPIO;
    else if (strcmp(pin_config, "EMUX_GPIOALT") == 0)
        pin_feature = EMUX_GPIOALT;

    _da16x_io_pinmux(PIN_EMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxF(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = FMUX_GPIO;

    if (strcmp(pin_config, "FMUX_GPIOBT") == 0)
        pin_feature = FMUX_GPIOBT;
    else if (strcmp(pin_config, "FMUX_GPIOI2S") == 0)
        pin_feature = FMUX_GPIOI2S;
    else if (strcmp(pin_config, "FMUX_GPIOSDm") == 0)
        pin_feature = FMUX_GPIOSDm;
    else if (strcmp(pin_config, "FMUX_SPIs") == 0)
        pin_feature = FMUX_SPIs;
    else if (strcmp(pin_config, "FMUX_UART2") == 0)
        pin_feature = FMUX_UART2;
    else if (strcmp(pin_config, "FMUX_SPIm") == 0)
        pin_feature = FMUX_SPIm;
    else if (strcmp(pin_config, "FMUX_GPIO") == 0)
        pin_feature = FMUX_GPIO;
    else if (strcmp(pin_config, "FMUX_GPIOALT") == 0)
        pin_feature = FMUX_GPIOALT;

    _da16x_io_pinmux(PIN_FMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxH(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = HMUX_ADGPIO;

    if (strcmp(pin_config, "HMUX_JTAG") == 0)
        pin_feature = HMUX_JTAG;
    else if (strcmp(pin_config, "HMUX_D_SYS_OUT") == 0)
        pin_feature = HMUX_D_SYS_OUT;
    else if (strcmp(pin_config, "HMUX_ADGPIO") == 0)
        pin_feature = HMUX_ADGPIO;
    else if (strcmp(pin_config, "HMUX_ADGPIOALT") == 0)
        pin_feature = HMUX_ADGPIOALT;

    _da16x_io_pinmux(PIN_HMUX, pin_feature);

    return ret;
}

int app_atcmd_user_pinmuxU(char *pin_config)
{
    int ret = 0;
    UINT8 pin_feature = UMUX_GPIO;

    if (strcmp(pin_config, "UMUX_JTAG") == 0)
        pin_feature = UMUX_JTAG;
    else if (strcmp(pin_config, "UMUX_UART2GPIO") == 0)
        pin_feature = UMUX_UART2GPIO;
    else if (strcmp(pin_config, "UMUX_GPIO") == 0)
        pin_feature = UMUX_GPIO;

    _da16x_io_pinmux(PIN_UMUX, pin_feature);

    return ret;
}

void app_atcmd_set_features(BOARD_FEATURES feature)
{
    support_feature = feature;
}

BOARD_FEATURES app_atcmd_get_features(void)
{
    return (BOARD_FEATURES)support_feature;
}

uint8_t app_atcmd_mcu_wakeup_port(char *port_num)
{
    uint8_t gpio_port = GPIO_UNIT_A;

    if (strcmp(port_num, "GPIO_UNIT_A") == 0)
        gpio_port = GPIO_UNIT_A;
    else if (strcmp(port_num, "GPIO_UNIT_C") == 0)
        gpio_port = GPIO_UNIT_C;

    return gpio_port;
}

uint16_t app_atcmd_mcu_wakeup_pin(char *pin_num)
{
    uint16_t gpio_pin = GPIO_PIN11;

    if (strcmp(pin_num, "GPIO_PIN0") == 0)
        gpio_pin = GPIO_PIN0;
    else if (strcmp(pin_num, "GPIO_PIN1") == 0)
        gpio_pin = GPIO_PIN1;
    else if (strcmp(pin_num, "GPIO_PIN2") == 0)
        gpio_pin = GPIO_PIN2;
    else if (strcmp(pin_num, "GPIO_PIN3") == 0)
        gpio_pin = GPIO_PIN3;
    else if (strcmp(pin_num, "GPIO_PIN4") == 0)
        gpio_pin = GPIO_PIN4;
    else if (strcmp(pin_num, "GPIO_PIN5") == 0)
        gpio_pin = GPIO_PIN5;
    else if (strcmp(pin_num, "GPIO_PIN6") == 0)
        gpio_pin = GPIO_PIN6;
    else if (strcmp(pin_num, "GPIO_PIN7") == 0)
        gpio_pin = GPIO_PIN7;
    else if (strcmp(pin_num, "GPIO_PIN8") == 0)
        gpio_pin = GPIO_PIN8;
    else if (strcmp(pin_num, "GPIO_PIN9") == 0)
        gpio_pin = GPIO_PIN9;
    else if (strcmp(pin_num, "GPIO_PIN10") == 0)
        gpio_pin = GPIO_PIN10;
    else if (strcmp(pin_num, "GPIO_PIN11") == 0)
        gpio_pin = GPIO_PIN11;

    return gpio_pin;
}

#endif  /* (__SUPPORT_ATCMD__) && ((__SUPPORT_AZURE_IOT__) || (__SUPPORT_AWS_IOT__)) */
