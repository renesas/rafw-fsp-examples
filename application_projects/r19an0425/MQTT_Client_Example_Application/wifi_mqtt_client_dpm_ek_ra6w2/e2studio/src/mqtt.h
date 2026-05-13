/***********************************************************************************************************************

* File Name    : mqtt.h

* Description  : Contains data structures and functions used in mqtt_client.c and app_task_entry.c

**********************************************************************************************************************/

/***********************************************************************************************************************

* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates

*

* SPDX-License-Identifier: BSD-3-Clause

***********************************************************************************************************************/

#define MQTT_H_
#define EVENT_VAL -1

/* MQTT Configuration */
#define MQTT_BROKER_IP            "192.168.49.35"
#define MQTT_BROKER_PORT           8883
#define MQTT_SUB_TOPIC             "rrq61x_sub"
#define MQTT_PUB_TOPIC             "rrq61x_pub"
#define MQTT_PUBLISH_MSG           "Hello from RA6W2"
#define MQTT_AUTO_MODE              MQTT_INIT_MAGIC
#define DPM_WAKEUP_INTERVAL_US      20000

void print_ep_info_banner();
void netif_status_callback();
