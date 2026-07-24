#!/usr/bin/env python3
# **********************************************************************************************************************
# File Name    : provisioning.py
# Description  : Command line utility for discovering BLE devices and performing WiFi provisioning commands.
# **********************************************************************************************************************
#
# Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
#
# SPDX-License-Identifier: BSD-3-Clause
#
# **********************************************************************************************************************

from importlib.metadata import version

import sys
import argparse
import asyncio
import pprint
import json
import yaml
import time
import enum
import struct
from rich import print

from bleak import BleakClient, BleakScanner, BleakGATTCharacteristic

prefix = bytearray([0x1, 0x0, 0x0, 0x0, 0x88, 0x0, 0x0, 0x0])

cmd_1 = { "dialog_cmd" : "factory_reset"      }
cmd_2 = { "dialog_cmd" : "chk_network"        }
cmd_3 = { "dialog_cmd" : "reboot"             }
cmd_4 = { "dialog_cmd" : "get_azureConString" }
cmd_5 = { "dialog_cmd" : "get_mode"           }
cmd_6 = { "dialog_cmd" : "get_thingName"      }
cmd_7 = { "dialog_cmd" : "scan"               }

cmd_8 = {
          "dialog_cmd"       : "network_info",
          "ping_addr"        : "8.8.8.8",
          "svr_addr"         : "192.168.0.1",
          "svr_port"         : 10195,
          "customer_svr_url" : "www.google.com",
          "svr_url"          : "www.google.com"
}

cmd_9 = {
          "dialog_cmd"    : "select_ap",
          "SSID"          : "myNetwork2",
          "security_type" : 3,
          "password"      : "!HelloWorld123",
          "isHidden"      : 0
}

cmd_10 = { "dialog_cmd" : "disconnect"          }
cmd_11 = { "dialog_cmd" : "wifi_status"         }


UUID_SERV_WIFI         = '9161b201-1b4b-4727-a3ca-47b35cdcf5c1'
UUID_CHAR_WIFI_COMMAND = '9161b202-1b4b-4727-a3ca-47b35cdcf5c1'
UUID_CHAR_WIFI_STATUS  = '9161b203-1b4b-4727-a3ca-47b35cdcf5c1'
UUID_CHAR_WIFI_OUTPUT  = '9161b204-1b4b-4727-a3ca-47b35cdcf5c1'
UUID_CHAR_WIFI_RESULT  = '9161b205-1b4b-4727-a3ca-47b35cdcf5c1'

class WIFI_ACTION_STATUS(enum.Enum):
  COMBO_WIFI_CMD_SCAN_AP_SUCCESS               = 1
  COMBO_WIFI_CMD_SCAN_AP_FAIL                  = 2

  COMBO_WIFI_CMD_FW_BLE_DOWNLOAD_SUCCESS       = 3
  COMBO_WIFI_CMD_FW_BLE_DOWNLOAD_FAIL          = 4

  COMBO_WIFI_CMD_INQ_WIFI_STATUS_CONNECTED     = 5
  COMBO_WIFI_CMD_INQ_WIFI_STATUS_NOT_CONNECTED = 6

  # feedback for Provisioning data write
  COMBO_WIFI_PROV_DATA_VALIDITY_CHK_ERR        = 7
  COMBO_WIFI_PROV_DATA_SAVE_SUCCESS            = 8

  COMBO_WIFI_CMD_ACK                           = 100
  COMBO_WIFI_CMD_SELECT_AP_SUCCESS             = 101
  COMBO_WIFI_CMD_SELECT_AP_FAIL                = 102
  COMBO_WIFI_PROV_WRONG_PW                     = 103
  COMBO_WIFI_PROV_NETWORK_INFO                 = 104
  COMBO_WIFI_PROV_AP_FAIL                      = 105
  COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_FAIL         = 106
  COMBO_WIFI_PROV_DNS_FAIL_GOOGLE_OK           = 107
  COMBO_WIFI_PROV_NO_URL_PING_FAIL             = 108
  COMBO_WIFI_PROV_NO_URL_PING_OK               = 109
  COMBO_WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK = 110
  COMBO_WIFI_PROV_DNS_OK_PING_OK               = 111
  COMBO_WIFI_PROV_REBOOT_ACK                   = 112
  COMBO_WIFI_PROV_DNS_OK_PING_N_GOOGLE_FAIL    = 113

  COMBO_WIFI_CMD_UNKNOWN_RCV                   = 114


found_device = None

async def ble_device_find(required_device):
    print(f'Looking for {required_device}...')

    def device_detection_cb(device, data):
        global found_device

        if device not in detected_devices:
          detected_devices[device] = data

          if device.name and required_device in device.name:
            print(f'Found: {device} {data.service_uuids}')
            found_device = (device, data)
            detected_event.set()
          else: print(f'Detected: {device}')

    async with BleakScanner(device_detection_cb) as scanner:
        detected_devices = {}
        detected_event = asyncio.Event()

        await detected_event.wait()

    return found_device

async def ble_services_get(client):
    return client.services

async def ble_characteristics_get(service):
    return service.characteristics

async def ble_characteristic_set(client, uuid, cmd, response=True, prefix=bytes()):
    print(prefix + bytes(cmd.encode()))
    print(len(prefix + bytes(cmd.encode())))
    await client.write_gatt_char(uuid, prefix + bytes(cmd.encode()), response)

async def ble_characteristic_get(client, uuid, raw=False):
    value = await client.read_gatt_char(uuid)
    return value if raw else value.decode('windows-1252', 'ignore')

async def ble_characteristic_subscribe(client, uuid, cb):
    await client.start_notify(uuid, cb)

async def ble_wifi_cmd_send(device, cmd):
    print(f'Connecting to {device}, please wait...')

    async with BleakClient(device) as client:
        print(f'connected')

        pro_service_cmd = '9161b202-1b4b-4727-a3ca-47b35cdcf5c1'
        await client.write_gatt_char(pro_service_cmd, bytes(cmd.encode()), response=True)

        pro_service_status = '9161b204-1b4b-4727-a3ca-47b35cdcf5c1'
        value = await client.read_gatt_char(pro_service_status)

        return value.decode('windows-1252', 'ignore')

    return None

async def ble_device_advertisement_info_print(device):
    advertisement_data = device[1]

    print(advertisement_data.local_name)
    print(advertisement_data.manufacturer_data)
    print(advertisement_data.service_data)
    print(advertisement_data.service_uuids)

async def main():
    print(version('bleak'))

    device = await ble_device_find('RRQ-DEVICE')
    print(device)

    async with BleakClient(device[0]) as client:
        services = await ble_services_get(client)
        for s in services:
          print(f'S:{s}')
          chars = await ble_characteristics_get(s)
          for c in chars:
            print(f'  - C:{c}')
            for cc in c.descriptors:
              print(f'    * D:{cc}')


        scan_done_event = asyncio.Event()

        def scan_callback(sender: BleakGATTCharacteristic, data: bytearray):
            value = int.from_bytes(data, byteorder='little')
            print(f"-----------------> Notification: {sender}: {data}: {hex(value)}")
            print(f"Notification: {sender}: {data}: {value}: {WIFI_ACTION_STATUS(value).name}")

            scan_done_event.set()


        ########################################################
        # ========== Subscibe on events ========================
        ########################################################
        print('Subscribing')
        await ble_characteristic_subscribe(client, UUID_CHAR_WIFI_STATUS, scan_callback)


        #########################################################
        ## ========== Send 'scan' command =======================
        #########################################################
        input('Press ENTER to send "scan" .....')
        await ble_characteristic_set(client, UUID_CHAR_WIFI_COMMAND, json.dumps(cmd_7), True)


        ########################################################
        # ========== Wait until 'scan' command is ready ========
        ########################################################
        await scan_done_event.wait()
        scan_done_event.clear()


        ########################################################
        # ========== Read 'scan' result ========================
        ########################################################
        input('Press ENTER to read FIRST chank of scanned APs .....')
        output: bytes = await ble_characteristic_get(client, UUID_CHAR_WIFI_OUTPUT, True)

        ctrl_data: bytes = output[0:4]
        remain, total = struct.unpack('<hh', ctrl_data)
        networks: str = output[4:].decode('windows-1252', 'ignore')

        print(f'{remain=} of {total=}')
        print(output[4:].decode('windows-1252', 'ignore'))

        while remain:
            input('Press ENTER to read NEXT chunk of scanned APs .....')
            output: str = await ble_characteristic_get(client, UUID_CHAR_WIFI_OUTPUT, True)

            ctrl_data: bytes = output[0:4]
            remain, total = struct.unpack('<hh', ctrl_data)
            networks += output[4:].decode('windows-1252', 'ignore')

            print(f'{remain=} of {total=}')
            print(output[4:].decode('windows-1252', 'ignore'))

        json_networks = yaml.safe_load(networks)
        print('\n\nFound WiFi Networks:')
        print(json_networks)
        print('\n')


        ########################################################
        # ========== Send 'network_info' command ===============
        ########################################################
        input('Press ENTER to send "network_info" command .....')
        await ble_characteristic_set(client, UUID_CHAR_WIFI_COMMAND, json.dumps(cmd_8), True)

        await scan_done_event.wait()
        scan_done_event.clear()

        output = await ble_characteristic_get(client, UUID_CHAR_WIFI_RESULT)
        print(output)


        ########################################################
        # ========== Send AP 'connect' command =================
        ########################################################
        input('Press ENTER to send "connect" command .....')
        await ble_characteristic_set(client, UUID_CHAR_WIFI_COMMAND, json.dumps(cmd_9), True)

        await scan_done_event.wait()
        scan_done_event.clear()


        ########################################################
        # ========== Send 'check_network' command ==============
        ########################################################

        #    result = await ble_wifi_cmd_send(device, json.dumps(cmd_2))
        #    print(result)

        ########################################################
        # ========== Send 'reboot' command =====================
        ########################################################
        #    result = await ble_wifi_cmd_send(device, json.dumps(cmd_3))
        #    print(result)

        sys.exit(0)


if __name__ == "__main__":
    asyncio.run(main())
