/***********************************************************************************************************************
* File Name    : main_thread_entry.c
* Description  : Main thread entry function.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include "main_thread.h"
#if (SUPPORT_MATTER_APP == 1)
#include "matter.h"
#endif
#include "at_cmd.h"
#include "sdio_drv.h"
#include "usb_console_main.h"
#include "sdio_cmd.h"

/* USER CODE BEGIN PV */
MODE_FLAG mode_flag;

uint8_t exirq_flag = 0; // push button 
uint8_t wakeup_enable_flag = 0;
uint8_t rd_flag = 0;    // MCU wake up by DA16200
uint8_t rs_flag = 0;
uint8_t door_state = 0;   /* 0 : close, 1 : open */
uint8_t window_state = 0; /* 0 : close, 1 : open */
uint8_t door_state_changed_bymcu = 0; /* 0 : byApp, 1 : byMCU */

int32_t temperature = 22;
uint32_t app_sys_tick;
uint32_t g_app_at_start = 0;

#if (SUPPORT_MATTER_APP == 1)
uint8_t light;
uint8_t light_lvl;
uint8_t lock_state;
uint16_t current_lift_pos;
#endif

extern uint32_t g_sdio_reinitialize_needs;
#define MAX_READ_PERIOD_BEFORE_SLEEP  (5 * 1000)  /* about 5 seconds */
#define AP_BLINK_PERIOD               (3 * 1000)   /* about 3 seconds */

int read_temperature(void);
void reset_app_sys_tickcount(void);
uint32_t app_sys_tickremain(uint32_t timeout_ms);
void LED_Toggle(bsp_io_port_pin_t pin, uint8_t state);

#if (SUPPORT_MATTER_APP == 1)
void led_control(uint16_t type, uint32_t value)
{
    switch(type) {
    case DEVICE_TYPE_ONOFFLIGHT:
        if (value) {
            light = 1;
            TURN_BLUE_ON
        } else {
            light = 0;
            TURN_BLUE_OFF
        }
        break;
    case DEVICE_TYPE_DIMMABLELIGHT:
        if (light) {
            if (value <= 100) {
                TURN_BLUE_ON
                TURN_GREEN_OFF
                TURN_RED_OFF
            } else if ((value > 100) && (value <= 200)) {
                TURN_BLUE_ON
                TURN_GREEN_ON
                TURN_RED_OFF
            } else if (value > 200) {
                TURN_BLUE_ON
                TURN_GREEN_ON
                TURN_RED_ON
            }
        } else {
            TURN_RED_OFF
            TURN_GREEN_OFF
            TURN_BLUE_OFF
        }
        light_lvl = (uint8_t)value;
        break;
    case DEVICE_TYPE_DOORLOCK:
        lock_state = (uint8_t)value;
        if (value == 1)
            TURN_RED_OFF
        else
            TURN_RED_ON
        break;
    case DEVICE_TYPE_WINDOWCOVERING:
        if ((value > 0) && (value <= 5000)) {
            TURN_BLUE_ON
            TURN_GREEN_OFF
            TURN_RED_OFF
        } else if ((value > 5000) && (value < 10000)) {
            TURN_BLUE_ON
            TURN_GREEN_ON
            TURN_RED_OFF
        } else if (value == 10000) {
            TURN_BLUE_ON
            TURN_GREEN_ON
            TURN_RED_ON
        } else {
            TURN_RED_OFF
            TURN_GREEN_OFF
            TURN_BLUE_OFF
        }

        PRINTF(" ### COVER State (%s) ###\r\n", (current_lift_pos == value)? "NOT MOVING" : ((current_lift_pos < value)? "CLOSING" : "OPENING"));
        current_lift_pos = (uint16_t)value;

        break;
    }
}
#endif

/*******************************************************************************************************************//**
 * @brief       temperature sensor report. fixed value
 * @param[in]   None
 * @retval      temperature         temperature sensor value but now fixed for test
 ***********************************************************************************************************************/
int read_temperature(void)
{
    /* value fixed */
    temperature++;
    if (temperature > 50)
        temperature = 22;

    PRINTF("\r[%s] %d\n", __func__, temperature);
    return temperature;
}

void reset_app_sys_tickcount(void)
{
    app_sys_tick = xTaskGetTickCount();
}

uint32_t app_sys_tickremain(uint32_t timeout_ms)
{
    uint32_t tick, remains;

    tick = xTaskGetTickCount();
    if ((remains = (tick - app_sys_tick)) >= timeout_ms)
        return 0;
    else
        return remains;
}

void LED_Toggle(bsp_io_port_pin_t pin, uint8_t state)
{
    /* Enable access to the PFS registers. If using r_ioport module then register protection is automatically
     * handled. This code uses BSP IO functions to show how it is used.
     */
    R_BSP_PinAccessEnable();

    if(state == ON)
    {
        /* Write to this pin */
        R_BSP_PinWrite((bsp_io_port_pin_t) pin, BSP_IO_LEVEL_HIGH);
    }
    else
    {
        /* Write to this pin */
        R_BSP_PinWrite((bsp_io_port_pin_t) pin, BSP_IO_LEVEL_LOW);
    }

    /* Protect PFS registers */
    R_BSP_PinAccessDisable();
}

#if (SUPPORT_MATTER_APP == 1)
void main_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: add your own code here */

    fsp_err_t err = FSP_SUCCESS;
    matterDeviceStatus stat = _Status_init_stat;
    uint8_t cnt_read_stat = 0;
    g_app_at_start = 1; // NO Waing for cmd.
    TURN_BLUE_OFF
    TURN_GREEN_OFF
    TURN_RED_OFF
    while (1) {
        if (g_app_at_start) {
            PRINTF("\r\nStart APP-ATCMD Main Entry start\r\n");
            PRINTF("\r\nTo initial matter device type if the device type is not onoff light, Type 'matdev [value]'\r\n");
            PRINTF("\r\nTo Factory reset, Type 'AT+PROVSTART'\r\n");
            break;
        }
        vTaskDelay(100);
    }

    mode_flag = START_MODE;
    while (1) {
        if (mode_flag == START_MODE) { /* initialize & start point */
            PRINTF("\r\n>>START_MODE\r\n");
            cnt_read_stat = 0;
            RE_READ_STAT:
            PRINTF_CMD(REQ_MATTER_STATUS);
            PRINTF_WAITRES(RES_MATTER_STATUS, 1000);
            stat = get_device_status();
            PRINTF("\r\n[%s] current stat = %d, cnt_read_stat = %d\r\n", __func__, stat, cnt_read_stat);

            if (stat == _Status_init_stat) {
                if (FSP_SUCCESS != err) {/* check error */
                    /* HAL_BUSY or HAL_TIMEOUT */
                    cnt_read_stat++;
                    PRINTF("\r\n[%s] UART_Receive()'s rc =%d, retryCnt = %d\r\n", __func__, err, cnt_read_stat);
                    if (err) {
                        set_uart_baudrate(UART_BAUDRATE_IF);
                        vTaskDelay(100);
                    }
                    goto RE_READ_STAT;
                }
            }

            if (stat == _Status_network_OK || stat == _Status_STA_done) {
                mode_flag = RUN_MODE;
                set_dev_status(_Status_check_clear);
            } else if (stat == _Status_init_stat) {
                mode_flag = RUN_MODE;
            } else {
                mode_flag = SET_MODE;
            }
        } else if (mode_flag == RUN_MODE) {
            if (get_device_status() == _Status_commissioning_mode_start) {/* DA16200 AP mode stat during provisioning */
                mode_flag = COMMISSIONING_MODE;
                reset_app_sys_tickcount();
                continue;
            }

            if (exirq_flag) {
                PRINTF("\r\nWake-up by EXTI in RUN_MODE: exirq_flag=%d\n", exirq_flag);
            } else {
#ifdef DEBUGGING
                PRINTF("\r\nWake-up by RTC in RUN_MODE: exirq_flag=%d\n", exirq_flag);
#endif
            }
            stat = get_device_status();
            if (stat == _Status_MCUOTA) {
                mode_flag = READ_MODE;
                reset_app_sys_tickcount();
                continue;
            }

            if (exirq_flag) {
                mode_flag = READ_MODE;

                if (exirq_flag) {/* Open sw Wake. go to open state. */
                    wake_up_wifi();
                    wakeup_enable_flag = 1;
                    exirq_flag = 0;
                    //set_dev_status(_Status_check_clear);
                    PRINTF("\r\nwake-up in run mode, set module wakeup pin\r\n");
                    PRINTF_CMD(REQ_MATTER_STATUS);
                    //PRINTF_WAITRES(RES_MATTER_STATUS, 1000);
                }
                reset_app_sys_tickcount();
                stat = get_device_status();
                vTaskDelay(5);
                /* continue; */
            } else {
                if (get_device_status() == ATCMD_Status_need_configuration) {
                    mode_flag = SET_MODE;
                } else {
#ifdef DEBUGGING
                    read_temperature(); /* if run case */
                    vTaskDelay(30);
                    PRINTF("\r[%s] Read Sensors of MCU in RUN_MODE\n", __func__);
#endif
                }
            }
        } else if (mode_flag == SET_MODE) {
            stat = get_device_status();
            if ((stat == _Status_IDLE) || (stat == _Status_need_configuration)) {
                /* configuration cfg set */
                matter_config_set();
                vTaskDelay(500);

                matter_certificate(0); /* CD */
                vTaskDelay(500);
                matter_certificate(1); /* DAC */
                vTaskDelay(500);
                matter_certificate(2); /* PAI */
                vTaskDelay(500);
                matter_certificate(3); /* DAC PRI KEY */
                vTaskDelay(500);
                matter_certificate(4); /* DAC PUB KEY */
                vTaskDelay(500);
                PRINTF_CMD(REQ_MATTER_ONBOARDING_INFO);
                PRINTF_WAITRES(RES_MATTER_ONBOARDING, 1000);
                vTaskDelay(100);

#if defined(SUPPORT_MATTER_BLE)
                PRINTF_CMD(REQ_MATTER_BLEADV_STOP);
                vTaskDelay(500);
                PRINTF_CMD(REQ_MATTER_BLEADV_START);
#endif
                blink_auto_LED(false);
                set_dev_status(_Status_check_clear);
            } else if (stat == _Status_network_Fail) {
                //PRINTF_ATCMD(REQ_APMODE);
                //set_dev_status(_Status_check_clear);
                PRINTF("\r\nmode_flag == SET_MODE _Status_network_Fail\r\n");
            }

            if (app_sys_tickremain(3) == 0) {
                reset_app_sys_tickcount();
                mode_flag = RUN_MODE;
            }
        } else if (mode_flag == READ_MODE) {
            PRINTF_WAITRES(RES_MATTER_STATUS, 1000);
            stat = get_device_status();
            if (stat == _Status_network_OK || stat == _Status_STA_done) {
                mode_flag = RUN_MODE;
                reset_app_sys_tickcount();
                if (stat == _Status_STA_done) {
                    set_dev_status(_Status_check_clear);
                    PRINTF("\r\nwifi-module STA done -> MCU goto idle\r\n");
                } else if (stat == _Status_network_OK) {
                    mode_flag = READ_MODE;
                    PRINTF("\r\nwifi-module network ok -> MCU goto idle\r\n");
                    continue;
                }
                set_dev_status(_Status_check_clear);
            } else if (stat == _Status_commissioning_mode_start) {
                mode_flag = COMMISSIONING_MODE;
                reset_app_sys_tickcount();
            } else if (stat == _Status_network_Fail || stat == _Status_need_configuration) {
                mode_flag = SET_MODE;
                reset_app_sys_tickcount();
            } else if (stat == _Status_commissioning_mode_done) { /* DA16200 AP mode stat during provisioning */
                blink_auto_LED(false);
                mode_flag = RUN_MODE; /* add for ap blink */
                reset_app_sys_tickcount();
            } else if (stat == _Status_MCUOTA) {
                mode_flag = READ_MODE;
                reset_app_sys_tickcount();
                continue;
            }

            if (wakeup_enable_flag && (get_device_status() == _Status_STA_start)) {
                wakeup_enable_flag = 0;
                reset_app_sys_tickcount();

                if (matter_device_type == DEVICE_TYPE_ONOFFLIGHT) {
                    if (light)
                        led_control(DEVICE_TYPE_ONOFFLIGHT, 0);
                    else
                        led_control(DEVICE_TYPE_ONOFFLIGHT, 1);
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d"
#else
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d\r\n"
#endif
                        , REQ_MATTER_ENDPOINT_ID
                        , REQ_MATTER_ONOFF_ID
                        , REQ_MATTER_ONOFF_ONOFF_ID
                        , light
                        , REQ_MATTER_ONOFF_ONOFF_TYPE
                        , REQ_MATTER_WRITE);
                } else if (matter_device_type == DEVICE_TYPE_DIMMABLELIGHT) {
                    if (light_lvl > 100)
                        light_lvl = light_lvl - 100;
                    else
                        light_lvl = 254 - (100 - light_lvl);
                    led_control(DEVICE_TYPE_DIMMABLELIGHT, light_lvl);

#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d"
#else
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d\r\n"
#endif
                        , REQ_MATTER_ENDPOINT_ID
                        , REQ_MATTER_LEVEL_ID
                        , REQ_MATTER_LEVEL_CURLVL_ID
                        , light_lvl
                        , REQ_MATTER_LEVEL_CURLVL_TYPE
                        , REQ_MATTER_WRITE);
                } else if (matter_device_type == DEVICE_TYPE_DOORLOCK) {
                    if (lock_state == REQ_MATTER_LOCK_LOCKSTATE_LOCK)
                        lock_state = REQ_MATTER_LOCK_LOCKSTATE_UNLOCK;
                    else
                        lock_state = REQ_MATTER_LOCK_LOCKSTATE_LOCK;
                    led_control(DEVICE_TYPE_DOORLOCK, lock_state);
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d"
#else
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d\r\n"
#endif
                        , REQ_MATTER_ENDPOINT_ID
                        , REQ_MATTER_LOCK_ID
                        , REQ_MATTER_LOCK_LOCKSTATE_ID
                        , lock_state = (lock_state == 1)?REQ_MATTER_LOCK_LOCKSTATE_UNLOCK:REQ_MATTER_LOCK_LOCKSTATE_LOCK
                        , REQ_MATTER_LOCK_LOCKSTATE_TYPE
                        , REQ_MATTER_WRITE);
                } else if (matter_device_type == DEVICE_TYPE_WINDOWCOVERING) {
                    if (current_lift_pos >= 3000)
                        current_lift_pos = current_lift_pos - 3000;
                    else
                        current_lift_pos = 10000 - (3000 - current_lift_pos);
                    led_control(DEVICE_TYPE_WINDOWCOVERING, current_lift_pos);
#if (SUPPORT_SPI == 1) || (SUPPORT_SDIO == 1)
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d"
#else
                    PRINTF_CMD(REQ_MATTER_ATTRIBUTE_CMD"%d,%d,%d,%d,%d,%d\r\n"
#endif
                        , REQ_MATTER_ENDPOINT_ID
                        , REQ_MATTER_COVER_ID
                        , REQ_MATTER_COVER_TARPOSLIFTPER_ID
                        , (current_lift_pos >= 3000)?(current_lift_pos - 3000):(10000 - (3000 - current_lift_pos))
                        , REQ_MATTER_COVER_TARPOSLIFTPER_TYPE
                        , REQ_MATTER_WRITE);
                }
                PRINTF_WAITRES(RES_MATTER_ATTSET_OK, 1000);
            }
        } else if (mode_flag == COMMISSIONING_MODE) {
            if (app_sys_tickremain(AP_BLINK_PERIOD) == 0) {
                reset_app_sys_tickcount();
                blink_auto_LED(true);
                PRINTF("\r\nWaiting Commissioning.........\r\n");
                if (get_device_status() == _Status_commissioning_mode_done) {
                    mode_flag = RUN_MODE;
                } else if (get_device_status() == ATCMD_Status_need_configuration) {
                    mode_flag = SET_MODE;
                }
            }
        }
        vTaskDelay(30);
    }
}
#else
/* Main_Thread entry function */
/* pvParameters contains TaskHandle_t */
void main_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);

    /* TODO: add your own code here */

    fsp_err_t err = FSP_SUCCESS;
    atcmd_error_code result = ERR_CMD_OK;
    DeviceStatus stat = ATCMD_Status_init_stat;
    uint8_t ap_mode = 0;
    uint8_t factory_reset = 0;
    uint8_t cnt_read_stat = 0;

    PRINTF("\r\nPlease enter AT-CMD. or enter ATTAPP to start App-AT command\r\n");

    while (1) {
        if (g_app_at_start) {
            PRINTF("\r\nStart APP-ATCMD Main Entry start\r\n");
            break;
        }
#if (SUPPORT_SDIO == 1)
        if (g_sdio_reinitialize_needs) {
#if DEBUG_XTRA
            PRINTF("SDIO reinit 1\r\n");
#endif
            init_drv_sdio();
            g_sdio_reinitialize_needs = 0;
        }
#endif
        vTaskDelay(100);
    }

    mode_flag = START_MODE;
    while (1) {
        if (mode_flag == START_MODE) { /* initialize & start point */
            PRINTF("\r\n>>START_MODE\r\n");
            cnt_read_stat = 0;

            RE_READ_STAT: PRINTF_ATCMD(REQ_STATUS);
            PRINTF_ATCMD_WAITRES(BIT_STATUS, 1000);
            stat = get_device_status();
            PRINTF("\r\n[%s] current stat = %d, rd_flag = %d, cnt_read_stat = %d\r\n", __func__, stat, rd_flag,
                    cnt_read_stat);
            //if (stat == ATCMD_Status_IDLE) {
            //    vTaskDelay(50);
            //    goto RE_READ_STAT;
            //}
            if (rd_flag && stat == ATCMD_Status_init_stat) {
                if (FSP_SUCCESS != err) {/* check error */
                    /* HAL_BUSY or HAL_TIMEOUT */
                    cnt_read_stat++;
                    PRINTF("\r\n[%s] UART_Receive()'s rc =%d, retryCnt = %d\r\n", __func__, err, cnt_read_stat);
                    if (err) {
                        set_uart_baudrate(UART_BAUDRATE_IF);
                        vTaskDelay(100);
                    }
                    goto RE_READ_STAT;
                }
            }

            if (stat == ATCMD_Status_network_OK || stat == ATCMD_Status_STA_done) {
                mode_flag = RUN_MODE;
                set_dev_status(ATCMD_Status_check_clear);
            } else if (stat == ATCMD_Status_init_stat) {
                mode_flag = RUN_MODE;
            } else {
                mode_flag = SET_MODE;
            }
        } else if (mode_flag == RUN_MODE) {
            if (get_device_status() == ATCMD_Status_AP_mode_start) {/* DA16200 AP mode stat during provisioning */
                mode_flag = AP_MODE;
                ap_mode = 1;
                reset_app_sys_tickcount();
                continue;
            }

            if (get_device_status() >= ATCMD_Status_AP_mode_start && factory_reset) {
                factory_reset = 0;
            }

            if (factory_reset) {
                /* wait 2 seconds and goto SET_MODE cofiguration */
                //           HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, WIFI_MOD_REBOOT_PERIOD, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
            } else {
                //            HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, MCU_WAKEUP_PERIOD, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
            }

            if (rd_flag || exirq_flag) {
                PRINTF("\r\nWake-up by EXTI in RUN_MODE: rd_flag=%d, exirq_flag=%d\n", rd_flag, exirq_flag);
            } else {
#ifdef DEBUGGING
                PRINTF("\r\nWake-up by RTC in RUN_MODE: rd_flag=%d, exirq_flag=%d\n", rd_flag, exirq_flag);
#endif
            }
            door_state_changed_bymcu = 0;

            if (rd_flag || exirq_flag) {
                mode_flag = READ_MODE;

                if (exirq_flag) {/* Open sw Wake. go to open state. */
                    wake_up_wifi();
                    wakeup_enable_flag = 1;
                    exirq_flag = 0;
                    set_dev_status(ATCMD_Status_check_clear);
                    PRINTF("\r\nwake-up in run mode, set module wakeup pin\r\n");
                }
                reset_app_sys_tickcount();
                stat = get_device_status();
                vTaskDelay(5);
                /* continue; */
            } else {
                if (factory_reset) {
                    set_dev_status(ATCMD_Status_need_configuration);
                    factory_reset = 0;
                    mode_flag = SET_MODE;
                } else {
#ifdef DEBUGGING
                    read_temperature(); /* if run case */
                    vTaskDelay(30);
                    PRINTF("\r[%s] Read Sensors of MCU in RUN_MODE\n", __func__);
#endif
                }
            }
        } else if (mode_flag == SET_MODE) {
            stat = get_device_status();
            if (stat == ATCMD_Status_IDLE) {
                /* configuration cfg set */
                atcmd_config_set();
                vTaskDelay(500);

                atcmd_certificate(0); /* CA */
                vTaskDelay(500);
#if (PLATFORM_ID == 0)
                if (AWS_USE_FLEET_PROV == 0) {
                    atcmd_certificate(1); /* Cert */
                    vTaskDelay(500);
                    atcmd_certificate(2); /* key */
                    vTaskDelay(500);
                }
#endif
#if defined(AT_ACK_CHECK)
                PRINTF_ATCMD(REQ_RESTART);
                vTaskDelay(500);
                PRINTF_ATCMD(REQ_APMODE);
#else /* !AT_ACK_CHECK */
                PRINTF_ATCMD(REQ_RESTART);
                vTaskDelay(500);
                PRINTF_ATCMD(REQ_APMODE);
#endif /* AT_ACK_CHECK */
                set_dev_status(ATCMD_Status_check_clear);
            } else if (stat == ATCMD_Status_need_configuration) {
                /* configuration cfg set */
                atcmd_config_set();
                vTaskDelay(500);

                atcmd_certificate(0); /* CA */
                vTaskDelay(500);
#if (PLATFORM_ID == 0)
                if (AWS_USE_FLEET_PROV == 0) {
                    atcmd_certificate(1); /* Cert */
                    vTaskDelay(500);
                    atcmd_certificate(2); /* key */
                    vTaskDelay(500);
                }
#endif
#if defined(AT_ACK_CHECK)
                PRINTF_ATCMD(REQ_RESTART);
#else
                PRINTF_ATCMD(REQ_RESTART);
#endif /* AT_ACK_CHECK */
                set_dev_status(ATCMD_Status_check_clear);
            } else if (stat == ATCMD_Status_network_Fail) {
#if defined(AT_ACK_CHECK)
                PRINTF_ATCMD(REQ_APMODE);
#else
                PRINTF_ATCMD(REQ_APMODE);
#endif /* AT_ACK_CHECK */

                set_dev_status(ATCMD_Status_check_clear);
            }

            if (app_sys_tickremain(3) == 0) {
                reset_app_sys_tickcount();
                mode_flag = RUN_MODE;
            }
        } else if (mode_flag == READ_MODE) {
#ifdef DEBUGGING
            PRINTF("\r\n>>UART_READ mode: rs_flag=%d, rd_flag=%d\r\n", rs_flag, rd_flag);
#endif
            if (rs_flag == 1 || rd_flag == 1) {
                result = ERR_CMD_OK;
                rs_flag = 0;
                rd_flag = 0;

                if (result == ERR_CMD_OK) {
                    stat = get_device_status();
                    if (stat == ATCMD_Status_network_OK || stat == ATCMD_Status_STA_done) {
                        mode_flag = RUN_MODE;
                        reset_app_sys_tickcount();
                        ap_mode = 0;
                        if (stat == ATCMD_Status_STA_done) {
                            //PRINTF("\r\nwifi-module goto sleep -> MCU goto sleep\r\n");
                            PRINTF("\r\nwifi-module STA done -> MCU goto idle\r\n");
                        } else if (stat == ATCMD_Status_network_OK) {
                            mode_flag = READ_MODE;
                            //PRINTF("\r\nwifi-module network ok -> MCU goto READ_MODE for MAX_READ_PERIOD_BEFORE_SLEEP\r\n");
                            PRINTF("\r\nwifi-module network ok -> MCU goto idle\r\n");
                            continue;
                        }
                        set_dev_status(ATCMD_Status_check_clear);
                    } else if (stat == ATCMD_Status_AP_mode_start) { /* DA16200 AP mode stat during provisioning */
                        mode_flag = AP_MODE; /* add for ap blink */
                        ap_mode = 1;
                        reset_app_sys_tickcount();
                    } else if (stat == ATCMD_Status_network_Fail || stat == ATCMD_Status_need_configuration) {
                        mode_flag = SET_MODE;
                        reset_app_sys_tickcount();
                        ap_mode = 0;
                    } else if (stat == ATCMD_Status_factoryreset_done) { /* DA16200 AP mode stat during provisioning */
                        mode_flag = RUN_MODE; /* add for ap blink */
                        factory_reset = 1;
                        reset_app_sys_tickcount();
                    } else if (stat == ATCMD_Status_MCUOTA) { /* DA16200 AP mode stat during provisioning */
                        mode_flag = READ_MODE; /* add for ap blink */
                        reset_app_sys_tickcount();
                        continue;
                    }
                } else { /* add for ap blink */
                    if (ap_mode) {
                        mode_flag = AP_MODE;
                        reset_app_sys_tickcount();
                    }
                }
            }

            if (wakeup_enable_flag && (get_device_status() == ATCMD_Status_STA_start)) {
                wakeup_enable_flag = 0;
                reset_app_sys_tickcount();

                if (door_state == 0) {
                    PRINTF_ATCMD(REQ_DOOR_OPENED);
                    TURN_BLUE_ON
                    door_state = 1;
                    door_state_changed_bymcu = 1;
                } else {
                    PRINTF_ATCMD(REQ_DOOR_CLOSED);
                    TURN_BLUE_OFF
                    door_state = 0;
                    door_state_changed_bymcu = 1;
                }
            }
            /*  check it later why it need??? not work configuration by factory button
            if ((app_sys_tickremain(MAX_READ_PERIOD_BEFORE_SLEEP) == 0))
            {  
               if (stat != ATCMD_Status_MCUOTA)
                {
                    PRINTF("\r\ngoto sleep mode %d\r\n", stat);
                    mode_flag = RUN_MODE;
                }
            }*/            
        } else if (mode_flag == AP_MODE) {
            if (rd_flag) {
                mode_flag = READ_MODE;
            }

            if (app_sys_tickremain(AP_BLINK_PERIOD) == 0) {
                reset_app_sys_tickcount();
                blink_auto_LED(true);  // not fixed check it later
            }
          
        }

#if (SUPPORT_SDIO == 1)
        if (g_sdio_reinitialize_needs) {
            //PRINTF("SDIO reinit 1\r\n");
            init_drv_sdio();
            g_sdio_reinitialize_needs = 0;
        }
#endif
        vTaskDelay(30);
    }
}
#endif // SUPPORT_MATTER_APP
