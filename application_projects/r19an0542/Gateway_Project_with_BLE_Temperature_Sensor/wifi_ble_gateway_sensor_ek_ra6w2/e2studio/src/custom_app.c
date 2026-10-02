/***********************************************************************************************************************
 * File Name    : custom_app.c
 * Description  : BLE advertisement report parser and MQTT publish handler for the BLE Gateway application.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ***********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "custom_app.h"
#include "r_ble_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include "hal_data.h"
#include "common_utils.h"
#include "mqtt_client.h"

#define MQTT_BROKER_IP           "YOUR_IP_ADDRESS"
#define MQTT_BROKER_PORT         1883
#define MQTT_BROKER_PORT_TLS     8883
#define MQTT_USE_TLS             0
#define MQTT_SUB_TOPIC           "YOUR_SUB_TOPIC"
#define MQTT_PUB_TOPIC           "YOUR_PUB_TOPIC"
#define MQTT_AUTO_MODE           MQTT_INIT_MAGIC
#define MQTT_AUTO_ENABLE         1
#define MQTT_SUB_ENABLE          1
#define MOKO_MIN_PAYLOAD_LEN     19
#define MOKO_TEMP_MSB_OFFSET     13
#define MOKO_TEMP_LSB_OFFSET     14
#define MOKO_HUM_MSB_OFFSET      15
#define MOKO_HUM_LSB_OFFSET      16
#define MOKO_BAT_MSB_OFFSET      17
#define MOKO_BAT_LSB_OFFSET      18
#define MOKO_TEMP_HUM_SCALE      10
#define MOKO_BAT_SCALE           1000
#define MOKO_RAW_PRINT_MAX       31
#define MQTT_PAYLOAD_BUF_LEN     200

static volatile bool s_mqtt_ready = false;

/*MAC ID of Sensor*/
static const uint8_t g_moko_mac[6] =
{
    0xd5, 0xc0, 0xe0, 0x5c, 0x41, 0xe9
};

static void mqtt_configure(void)
{
    const int mqtt_tls_enable = MQTT_USE_TLS;
    const int mqtt_port = mqtt_tls_enable ? MQTT_BROKER_PORT_TLS : MQTT_BROKER_PORT;
    APP_PRINT("[MQTT] mqtt_configure() start\n");
    if (s_mqtt_ready)
    {
        APP_PRINT("[MQTT] mqtt already ready, skipping configure\n");

        return;
    }
    mqtt_client_config_initialize();
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_TLS, mqtt_tls_enable);
    mqtt_client_cfg_sync_rtm(0, NULL, RRQ61X_CONF_INT_MQTT_TLS, mqtt_tls_enable);
    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_BROKER_IP, MQTT_BROKER_IP);
    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_BROKER_IP, MQTT_BROKER_IP, 0, 0);
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_PORT, mqtt_port);
    mqtt_client_cfg_sync_rtm(0, NULL, RRQ61X_CONF_INT_MQTT_PORT, mqtt_port);
    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_SUB_TOPIC, MQTT_SUB_TOPIC);
    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_SUB_TOPIC, MQTT_SUB_TOPIC, 0, 0);
    set_mqtt_param_str(RRQ61X_CONF_STR_MQTT_PUB_TOPIC, MQTT_PUB_TOPIC);
    mqtt_client_cfg_sync_rtm(RRQ61X_CONF_STR_MQTT_PUB_TOPIC, MQTT_PUB_TOPIC, 0, 0);
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_AUTO, MQTT_AUTO_ENABLE);
    mqtt_client_cfg_sync_rtm(0, NULL, RRQ61X_CONF_INT_MQTT_AUTO, MQTT_AUTO_MODE);
    set_mqtt_param_int(RRQ61X_CONF_INT_MQTT_SUB, MQTT_SUB_ENABLE);
    mqtt_client_cfg_sync_rtm(0, NULL, RRQ61X_CONF_INT_MQTT_SUB, MQTT_SUB_ENABLE);
    mqtt_client_save_to_dpm_user_mem();
    s_mqtt_ready = true;
}

void mqtt_initialize(void)
{
    mqtt_configure();
}

static void mqtt_publish_payload(const char *payload)
{
    if (payload != NULL)
    {
        APP_PRINT("[MQTT] mqtt_publish_payload() send topic=%s payload_len=%u\n",MQTT_PUB_TOPIC,(unsigned)strlen(payload));
        mqtt_client_send_message((char *)MQTT_PUB_TOPIC, (char *)payload);
    }
}

void advRptParse(void *pdata)
{
    st_ble_gap_adv_rept_evt_t *rcvdData;
    uint8_t *mac;
    uint8_t *d;
    int dlen;
    int raw_t;
    int raw_h;
    int raw_b;
    int t_i;
    int t_d;
    int h_i;
    int h_d;
    int b_i;
    int b_d;
    int i;
    char payload[MQTT_PAYLOAD_BUF_LEN];
    if (!pdata)
    {
        return;
    }
    rcvdData = (st_ble_gap_adv_rept_evt_t *)pdata;
    mac = rcvdData->param.p_adv_rpt->p_addr;

    /* Accept only the configured target device MAC address */
    if (memcmp(mac, g_moko_mac, sizeof(g_moko_mac)) != 0)
    {
        return;
    }
    d    = rcvdData->param.p_adv_rpt->p_data;
    dlen = rcvdData->param.p_adv_rpt->len;
    APP_PRINT("\n===== MOKO SENSOR DETECTED =====\n");
    APP_PRINT("MAC: %02X:%02X:%02X:%02X:%02X:%02X  (payload %d bytes)\n",mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], dlen);
    APP_PRINT("RAW:");
    for (i = 0; i < dlen && i < MOKO_RAW_PRINT_MAX; i++)
    {
        APP_PRINT(" %02X", d[i]);
    }
    APP_PRINT("\n");
    if (dlen >= MOKO_MIN_PAYLOAD_LEN)
    {
        raw_t = (d[MOKO_TEMP_MSB_OFFSET] << 8) | d[MOKO_TEMP_LSB_OFFSET];
        raw_h = (d[MOKO_HUM_MSB_OFFSET]  << 8) | d[MOKO_HUM_LSB_OFFSET];
        raw_b = (d[MOKO_BAT_MSB_OFFSET]  << 8) | d[MOKO_BAT_LSB_OFFSET];
        t_i = raw_t / MOKO_TEMP_HUM_SCALE;
        t_d = raw_t % MOKO_TEMP_HUM_SCALE;
        h_i = raw_h / MOKO_TEMP_HUM_SCALE;
        h_d = raw_h % MOKO_TEMP_HUM_SCALE;
        b_i = raw_b / MOKO_BAT_SCALE;
        b_d = raw_b % MOKO_BAT_SCALE;
        APP_PRINT("Temperature: %d.%d C\n", t_i, t_d);
        APP_PRINT("Humidity   : %d.%d %%\n", h_i, h_d);
        APP_PRINT("Battery    : %d.%03d V\n", b_i, b_d);
        snprintf(payload, sizeof(payload),"{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"temp\":%d.%d,\"hum\":%d.%d,\"bat\":%d.%02d}\r\n",mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], t_i, t_d, h_i, h_d, b_i, b_d);
        mqtt_configure();
        mqtt_publish_payload(payload);
        APP_PRINT("MQTT publish: %s\n", payload);
    }
    else
    {
        APP_PRINT("MOKO payload too short (%d bytes) to parse.\n", dlen);
    }
    APP_PRINT("================================\n\n");
}

