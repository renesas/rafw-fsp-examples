#***********************************************************************************************************************
#* File Name    : vndm_codeless_provisioningapp.py
#* Description  : Codeless Provisioning application utility.
#**********************************************************************************************************************/

#***********************************************************************************************************************
#* Copyright (c) 2020 - 2025 Renesas Electronics Corporation and/or its affiliates
#*
#* SPDX-License-Identifier: BSD-3-Clause
#***********************************************************************************************************************/

import serial
import sys
import logging
import argparse
import json
import re
import time
from colorama import init

wep_number = 0

sec_map = {
    "No": 0,
    "WEP": 1,
    "WPA": 2,
    "WPA2": 3,
    "WPA/WPA2": 4,
    "OWE": 5,
    "WPA3": 6,
    "WPA2/WPA3": 7,
    "WPA Enterprise": 8,
    "WPA2 Enterprise": 9,
    "WPA/WPA2 Enterprise": 10,
    "WPA3 Enterprise": 11,
    "WPA2/WPA3 Enterprise": 12,
    "WPA3 192B Enterprise": 13,
}


class Colors:
    purple = '\033[35m'
    grey = '\033[38m'
    green = '\033[32m'
    yellow = '\033[33m'
    red = '\033[31m'
    reset = '\033[0m'


class CustomFormatter(logging.Formatter):
    format = "[%(levelname)s] %(filename)s,%(funcName)s:%(lineno)d: %(message)s"

    FORMATS = {
        logging.DEBUG: Colors.grey + format + Colors.reset,
        logging.INFO: Colors.green + format + Colors.reset,
        logging.WARNING: Colors.yellow + format + Colors.reset,
        logging.ERROR: Colors.red + format + Colors.reset,
        logging.CRITICAL: Colors.red + format + Colors.reset
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


def convert_results(res):

    global wep_number
    dict_results = []
    pattern = r"\S+\t(-?\d+)\t\d+\t\((.+) Security\)(?:\t(.*))?"

    for i, line in enumerate(res):
        if i == 0:
            line = line.removeprefix("+WFSCAN:")
        match = re.search(pattern, line)
        if match:
            signal, security, ssid = match.groups()
            if not ssid:
                continue
            signal = signal if signal else ""

            if security.startswith("WEP"):
                # Extract WEP number, if it exists, to use on the connect command
                wep_number = int(security[3:]) if len(security) > 3 else 0
                security = 1
            else:
                security = sec_map.get(security, 0)

            json_str = json.dumps({"SSID": ssid, "security_type": security, "signal_strength": int(signal)}, separators=(',',':'))
            dict_results.append(json_str)

    total = len(dict_results)
    for i, text in enumerate(dict_results):
        dict_results[i] = f'{i + 1}/{total}:{text}'
    return dict_results


def parse_command(ssid, psk, security):

    if ',' in ssid or "'" in ssid:
        ssid = "'" + ssid + "'"
    if ',' in psk or "'" in psk:
        psk = "'" + psk + "'"
    command = 'AT+WFJAP=' + ssid + ',' + str(security)

    if security == 0:
        return command
    elif security == 1:
        encryption = wep_number     # WEP
    elif security == 2:
        encryption = 0              # TKIP
    elif (security == 3) or (5 <= security < 8):
        encryption = 1              # AES
    elif security == 4:
        encryption = 2              # TKIP+AES
    else:
        return ''
    return command + ',' + str(encryption) + ',' + psk


def send(s, cmd, data='', wait=None):
    command = f'{cmd}{data}' if data else cmd
    print('<<<', command)
    command += '\r'
    s.write(command.encode('utf8'))

    success = False
    reason = None
    data = []

    connect_cmd = False
    capture_scan_results = 'WFSCAN' in cmd

    while True:
        msg = s.readline().decode('utf8', errors='replace').strip().strip('\x00')
        if msg:
            print('>>>', msg, end='\n')
            if capture_scan_results:
                data.append(msg)
        if (msg.startswith('ERROR')) | ('+DISCONNECTED' in msg):
            return False, reason, data
        elif msg.startswith('OK'):
            if 'WFJAP' in cmd:  # Wait to the connection massage, come after 'OK'
                connect_cmd = True
                continue
            return True, reason, data
        elif msg.startswith('+READY'):
            return 0
        elif connect_cmd and msg.startswith('+WFJAP:'):  # In case of connection request command
            connect_cmd = False
            success = True if msg.startswith('+WFJAP:1') else False
            return success, reason, data
        elif msg.startswith('+WFDAP:0'):
            if 'OTHER,15' in msg:
                reason = 'WRONGPWD'
        elif msg.startswith('+NWPING:'):  # In case of connection request command
            match = re.search(r"\+NWPING:(\d+),(\d+)", msg)
            reason = match and match.group(1) == match.group(2)
            continue

    if wait:
        time.sleep(wait)


class Provisioning:

    WIFI_CMD_SCAN_AP_SUCCESS = 1
    WIFI_CMD_SCAN_AP_FAIL = 2
    WIFI_CMD_ACK = 100
    WIFI_CMD_SELECT_AP_SUCCESS = 101
    WIFI_CMD_SELECT_AP_FAIL = 102
    WIFI_PROV_WRONG_PW = 103
    WIFI_PROV_NETWORK_INFO = 104
    WIFI_PROV_AP_FAIL = 105
    WIFI_PROV_DNS_FAIL_GOOGLE_FAIL = 106
    WIFI_PROV_DNS_FAIL_GOOGLE_OK = 107
    WIFI_PROV_NO_URL_PING_FAIL = 108
    WIFI_PROV_NO_URL_PING_OK = 109
    WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK = 110
    WIFI_PROV_DNS_OK_PING_OK = 111
    WIFI_PROV_REBOOT_ACK = 112
    WIFI_CMD_UNKNOWN_RCV = 114

    def __init__(self, serial_connection):
        self.s = serial_connection

    def factory_reset(self):
        logger.info("prov_cmd_factory_reset")
        send(self.s, 'ATR')
        timeout = 1
        start_time = time.time()
        while time.time() - start_time < timeout:  # Wait to the prints that come after reset
            if self.s.in_waiting:
                msg = self.s.readline().decode('utf8', errors='replace').strip('\x00')
                if msg.strip():
                    print('>>>', msg.strip(), end='\n')

    def chk_network(self, ping_addr):
        logger.info("prov_cmd_check_network")
        # Ping is optional:
        # command = 'AT+NWPING=0,' + ping_addr + ',4'
        # success, reason, data = send(self.s, command)
        success, reason = True, True
        if success and reason:
            logger.info("WIFI_PROV_DNS_OK_PING_OK")
            return self.WIFI_PROV_DNS_OK_PING_OK
        else:
            logger.error("WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK")
            return self.WIFI_PROV_DNS_OK_PING_FAIL_N_GOOGLE_OK

    def reboot(self):
        logger.info("prov_cmd_reboot")
        send(self.s, 'AT+RESTART')
    @staticmethod
    def get_azure_con_string():
        logger.info("prov_cmd_get_azure_connection")

    @staticmethod
    def get_mode():
        logger.info("prov_cmd_get_mode")

    @staticmethod
    def get_thing_name():
        logger.info("prov_cmd_get_name")

    def wifi_scan(self):
        logger.info("prov_cmd_scan")
        success, reason, scan_results = send(self.s, 'AT+WFSCAN')
        parsed_results = convert_results(scan_results) if scan_results else []
        if success:
            if reason:
                logger.error("WIFI_CMD_SCAN_AP_FAIL")
                status = self.WIFI_CMD_SCAN_AP_FAIL
            else:
                logger.info("WIFI_CMD_SCAN_AP_SUCCESS")
                status = self.WIFI_CMD_SCAN_AP_SUCCESS
        else:
            logger.error("WIFI_CMD_SCAN_AP_FAIL")
            status = self.WIFI_CMD_SCAN_AP_FAIL
        return status, parsed_results

    def network_info(self, j):
        logger.info("prov_cmd_network_info")
        print("\tping_ip: " + j['ping_addr'])
        print("\tsvr_addr: " + j['svr_addr'])
        print("\tsvr_port: " + str(j['svr_port']))
        print("\tsvr_url: " + j['svr_url'])
        return self.WIFI_PROV_NETWORK_INFO

    def select_ap(self, ssid, psk, security):
        logger.info("prov_cmd_select_ap")
        ssid_org = ssid.replace('\\\\', '\\')
        command = parse_command(ssid_org, psk, security)
        success, reason, data = send(self.s, command)
        if success:
            logger.info("WIFI_CMD_SELECT_AP_SUCCESS")
            return self.WIFI_CMD_SELECT_AP_SUCCESS
        elif reason == 'WRONGPWD':
            logger.error("WIFI_PROV_WRONG_PW")
            return self.WIFI_PROV_WRONG_PW
        else:
            logger.error("WIFI_CMD_SELECT_AP_FAIL")
            return self.WIFI_CMD_SELECT_AP_FAIL

    @staticmethod
    def disconnect(self):
        logger.info("prov_cmd_disconnect")


init()  # Enables ANSI colors on Windows
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

if not logger.hasHandlers():
    ch = logging.StreamHandler()
    ch.setFormatter(CustomFormatter())
    logger.addHandler(ch)


def main(args):

    if not args.port:
        print(f"{Colors.yellow}Please provide a serial port number (e.g., COM4) when running the script.{Colors.reset}")
        sys.exit(1)

    try:
        s = serial.Serial(args.port, args.baudrate)
    except serial.serialutil.SerialException:
        print(f"{Colors.yellow}Cannot open serial port.{Colors.reset}")
        sys.exit(1)

    print(f"{Colors.purple}Please run Provisioning application\n{Colors.reset}")
    d = json.JSONDecoder()
    e = json.JSONEncoder(separators=(',', ':'), ensure_ascii=False)
    prov = Provisioning(s)

    while True:
        msg = s.readline().decode('utf8').strip('\x00')
        if msg.strip():
            print('[MAIN] >>>', msg.strip(), end='\n')
        if not msg.startswith('+PRINT:'):
            continue
        time.sleep(0.2)
        p = msg.removeprefix('+PRINT:')
        j = d.decode(p)
        cmd = j['dialog_cmd']
        print('json:', j)
        print('command:', cmd)

        if cmd == 'scan':
            status, scan_res = prov.wifi_scan()
            send(s, 'ATr+PRINT=', status, 1)
            for r in scan_res:
                success, reason, scan_results = send(s, 'ATr+PRINT=', r)
                if not success:
                    break

        elif cmd == 'network_info':
            send(s, 'ATr+PRINT=', prov.WIFI_CMD_ACK)
            status = prov.network_info(j)
            ping_addr = j['ping_addr']
            send(s, 'ATr+PRINT=', e.encode({'result': status}))

        elif cmd == 'select_ap':
            send(s, 'ATr+PRINT=', prov.WIFI_CMD_ACK, 1)
            ssid = j['SSID']
            password = j['password']
            security = j['security_type']
            status = prov.select_ap(ssid, password, security)
            send(s, 'ATr+PRINT=', e.encode({'result': status}))

        elif cmd == 'chk_network':
            send(s, 'ATr+PRINT=', prov.WIFI_CMD_ACK, 2)
            status = prov.chk_network(ping_addr)
            send(s, 'ATr+PRINT=', e.encode({'result': status, 'ssid': ssid, 'password': password, 'security': security}))

        elif cmd == 'reboot':

            prov.reboot()


        elif cmd == 'factory_reset':
            prov.factory_reset()

        else:
            print('[MAIN] >>>', msg.strip(), end='\n')

    s.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Serial Port Connection")
    parser.add_argument("port", nargs="?", help="Serial port")
    parser.add_argument("baudrate", nargs="?", type=int, default=115200, help="Baud rate (default: 115200)")

    main_args = parser.parse_args()
    main(main_args)

