/***********************************************************************************************************************
* File Name    : cli_ble_cmds.c
* Description  : Contains CLI functions common to BLE operation control within the application.
***********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "cli_ble_queue.h"
#include "cli_ble_msg.h"
/* For BLE_BD_ADDR_LEN include */
#include "rm_ble_abs_api.h"

#include "rm_map_persistant_w.h"
#include "rm_vee_flash_w_rrq_nvram.h"

#define BLE_EVENT_PATTERN           (0x0A0A)
/* aa:bb:cc:dd:ee:ff - string is expected to be in next format */
#define BLE_BD_ADDR_STRING_SIZE     sizeof("aa:bb:cc:dd:ee:ff") - 1
#define BLE_CFG_BD_ADDRESS          "PUBLIC_BD_ADDR"
extern EventGroupHandle_t       g_ble_event_group_handle;

static void cli_ble_thread_notify (void)
{
    xEventGroupSetBits(g_ble_event_group_handle, BLE_EVENT_PATTERN);
}

static void cli_ble_svc_start (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_SVC_START };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_svc_stop (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_SVC_STOP };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_adv_start (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_ADV_START };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_adv_stop (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_ADV_STOP };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_disconnect (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_DISCONNECT };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_pb_bd_addr_write (char *parsed_bd_addr)
{   
#ifndef RM_MAP_PERSISTANT_W
    uint32_t ret = write_nvram_blecfg_string(BLE_CFG_BD_ADDRESS, parsed_bd_addr);
    assert(0 == ret);
#else
    RM_MAP_PERSISTANT_W_Write_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, BLE_CFG_BD_ADDRESS, parsed_bd_addr);
#endif
}

static void cli_ble_bd_addr_get (void)
{
    cli_ble_msg_t msg = { .type = CLI_BLE_MSG_TYPE_BD_ADDR_GET };

    cli_ble_queue_enqueue(&msg);

    cli_ble_thread_notify();
}

static void cli_ble_pb_bd_addr_read (void)
{
    char *bd_addr = NULL;

#ifndef RM_MAP_PERSISTANT_W
    bd_addr = (uint8_t *)read_nvram_blecfg_string(BLE_CFG_BD_ADDRESS);
#else
    RM_MAP_PERSISTANT_W_Read_STRING(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, BLE_CFG_BD_ADDRESS, (char *)&bd_addr);
#endif
    if (bd_addr) {
        printf("BD address from NVRAM: %s\n", bd_addr);
    }
}

static void cli_ble_pb_bd_addr_clear (void)
{
#ifndef RM_MAP_PERSISTANT_W
    uint32_t ret = delete_nvram_blecfg_env(BLE_CFG_BD_ADDRESS);
    assert(1 == ret);
#else
    RM_MAP_PERSISTANT_W_Erase(RM_MAP_PERSISTANT_W_get_ctrl(), ENV_GROUP_BLECFG, BLE_CFG_BD_ADDRESS);
#endif
}

bool cmd_ble_start_adv (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_adv_start();

    return true;
}

bool cmd_ble_stop_adv (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_adv_stop();

    return true;
}

bool cmd_ble_start_svc (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_svc_start();

    return true;
}

bool cmd_ble_stop_svc (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_svc_stop();

    return true;
}

bool cmd_ble_write_public_bd_addr (int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        printf("usage: ble set_bd_addr <aa:bb:cc:dd:ee:ff>\n");
        return false;
    }

    if (BLE_BD_ADDR_STRING_SIZE != strlen(argv[1])) {
        printf("usage: ble set_bd_addr <aa:bb:cc:dd:ee:ff> - addr should be 6 hex represented bytes, each limited by ':'");
        return false;
    }

    char ch;
    bool contain_non_hex = false;
    char *in_bd_addr = argv[1];
    /* BD address is expected in aa:bb:cc:dd:ee:ff format, 5 - amount of delimiters */
    const int limiter_max = 5;
    int limiter_cnt = 0;

    for (uint32_t i = 0; i < strlen(argv[1]); i++) {
        if (in_bd_addr[i] == ':') {
            limiter_cnt++;
            i++;
        }
            
        ch = toupper(in_bd_addr[i]);
        /* Check for char to be hex */
        if ((ch < '0' || ch > '9') && 
            (ch < 'A' || ch > 'F')) {
            printf("BD address is expected as a hex string, example: aa:bb:cc:dd:ee:ff\n");
            contain_non_hex = true;
            break;
        }
    }

    if (limiter_max != limiter_cnt) {
        printf("BD address is expected as a string with 5 limiters ':'\n");
        return false;
    }

    if (contain_non_hex)
        return false;

    cli_ble_pb_bd_addr_write(argv[1]);

    return true;
}

bool cmd_ble_get_public_bd_addr (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_bd_addr_get();

    return true;
}

bool cmd_ble_read_public_bd_addr (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_pb_bd_addr_read();

    return true;
}

bool cmd_ble_clear_public_bd_addr (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_pb_bd_addr_clear();

    return true;
}

bool cmd_ble_disconnect (int argc, char *argv[])
{
    FSP_PARAMETER_NOT_USED(argc);
    FSP_PARAMETER_NOT_USED(argv);

    cli_ble_disconnect();

    return true;
}
