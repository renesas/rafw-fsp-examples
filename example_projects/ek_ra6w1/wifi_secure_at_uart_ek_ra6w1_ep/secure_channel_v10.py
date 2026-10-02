#
# Copyright (c) 2020 - 2026 Renesas Electronics Corporation and/or its affiliates
#
# SPDX-License-Identifier: BSD-3-Clause
#

import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import scrolledtext
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
import threading
import sys
import re
import os
if os.name != 'nt':
    import termios
    import fcntl

# Status variables
secure_channel_active = False
known_key = "atkey_on_prod".encode("ascii").ljust(16, b"\x00")
random_iv = get_random_bytes(16)
last_encrypted_block = random_iv
last_received_block = random_iv
stop_receiving = False

def set_linux_raw_mode(serial_obj):
    """raw mode on linux"""
    fd = serial_obj.fileno()
    attrs = termios.tcgetattr(fd)
     # input flags - Turn off input processing
    attrs[0] &= ~(termios.IGNBRK | termios.BRKINT | termios.PARMRK |
                  termios.ISTRIP | termios.INLCR | termios.IGNCR |
                  termios.ICRNL | termios.IXON)

    # output flags - Turn off output processing
    attrs[1] &= ~termios.OPOST

    # control flags - 8-bit chars
    attrs[2] &= ~termios.CSIZE
    attrs[2] |= termios.CS8

    # local flags - disable echo, canonical mode, etc.
    attrs[3] &= ~(termios.ECHO | termios.ECHONL | termios.ICANON |
                  termios.ISIG | termios.IEXTEN)

    # set read timeout
    attrs[6][termios.VMIN] = 0   # minimum number of characters to read
    attrs[6][termios.VTIME] = 10 # timeout in tenths of seconds (i.e. 1 sec)

    termios.tcsetattr(fd, termios.TCSANOW, attrs)

def parse_hex_escapes(text):
    def replacer(match):
        return bytes([int(match.group(1), 16)])
    return re.sub(rb'\\x([0-9A-Fa-f]{2})', replacer, text.encode('utf-8')).decode('latin1').encode('latin1')

# AES encryption function
def encrypt_argument(argument, plain_data_area):
    global last_encrypted_block
    plain_data_area.insert(tk.END, f"[IV before]: {last_encrypted_block.hex()}\n")
    print(">> Last IV before encryption:", last_encrypted_block.hex())
    cipher = AES.new(known_key, AES.MODE_CBC, iv=last_encrypted_block)
    padded_data = pad(argument)
    print(">> argument raw :", padded_data.encode('utf-8').hex())
    encrypted_data = cipher.encrypt(padded_data.encode('utf-8'))
    print(">> argument encrypted :", encrypted_data.hex())
    print(">> cipher len:", len(encrypted_data))
    last_encrypted_block = encrypted_data[-16:]
    print(">> Last IV after encryption:", last_encrypted_block.hex())
    plain_data_area.insert(tk.END, f"[IV after ]: {last_encrypted_block.hex()}\n")
    return encrypted_data

def format_raw_data(data_bytes):
    return "0x" + "".join(f"{b:02x}" for b in data_bytes)

def decrypt_data(encrypted_data):
    global last_received_block
    cipher = AES.new(known_key, AES.MODE_CBC, iv=last_received_block)
    decrypted_padded_data = cipher.decrypt(bytes.fromhex(encrypted_data))
    last_received_block = bytes.fromhex(encrypted_data)[-16:]
    return unpad(decrypted_padded_data).decode('utf-8', errors='ignore')

def receive_data():
    global com_port, stop_receiving, secure_channel_active

    incomplete_line = b""
    waiting_for_more_data = False
    SEC_MARK1 = b"\r\n+SEC:"
    SEC_MARK2 = b"+SEC:"
    sec_buf = b""

    while not stop_receiving:
        if com_port and com_port.in_waiting > 0:
            if secure_channel_active:
                # Read ALL available bytes into one buffer
                sec_buf += com_port.read(com_port.in_waiting)

                # Optional debug
                # print(">> HEX CHUNK:", sec_buf.hex())

                def emit_plaintext_line(line_bytes: bytes):
                    """Print one plaintext line (already stripped from CRLF/LF)"""
                    global secure_channel_active
                    if not line_bytes:
                        return
                    decoded = line_bytes.decode("utf-8", errors="ignore").strip()
                    if not decoded:
                        return

                    received_data_area.insert(tk.END, "\r\n" + decoded + "\r\n")
                    received_data_area.yview(tk.END)

                    plain_data_area.insert(tk.END, "\r\n" + decoded + "\n\n")
                    plain_data_area.yview(tk.END)

                    # Device reset/error detection
                    is_reset  = "+INIT:DONE,0" in decoded
                    if is_reset:
                        random_iv = get_random_bytes(16)
                        iv_entry.delete(0, tk.END)
                        iv_entry.insert(0, random_iv.hex())
                        secure_channel_active = False
                        msg = "Device reset detected -> Secure Channel OFF.\n"
                        ats_button.config(text="Enable/Sync Secure Channel")
                        plain_data_area.insert(tk.END, msg)
                        plain_data_area.yview(tk.END)
                        return

                def consume_plaintext_lines_from_prefix(prefix: bytes) -> bytes:
                    """
                    Consume complete plaintext lines from prefix.
                    Returns leftover bytes (partial line, if any).
                    Supports both CRLF and LF-only.
                    """
                    buf = prefix

                    while True:
                        # Find earliest line ending: CRLF preferred, else LF
                        crlf = buf.find(b"\r\n")
                        lf = buf.find(b"\n")

                        if crlf != -1 and (lf == -1 or crlf < lf):
                            # Complete line ends at CRLF
                            line = buf[:crlf]
                            emit_plaintext_line(line)
                            buf = buf[crlf + 2:]
                            continue

                        if lf != -1:
                            # Complete line ends at LF
                            line = buf[:lf]
                            # Strip trailing CR if exists
                            if line.endswith(b"\r"):
                                line = line[:-1]
                            emit_plaintext_line(line)
                            buf = buf[lf + 1:]
                            continue

                        break

                    return buf  # leftover (partial line)

                # ---- Main parse loop: decrypt +SEC frames, print other plaintext lines ----
                while True:
                    # Find earliest +SEC marker (either with or without leading CRLF)
                    i1 = sec_buf.find(SEC_MARK1)  # b"\r\n+SEC:"
                    i2 = sec_buf.find(SEC_MARK2)  # b"+SEC:"

                    # Pick earliest occurrence
                    if i1 != -1 and (i2 == -1 or i1 < i2):
                        i = i1
                        mark = SEC_MARK1
                    elif i2 != -1:
                        i = i2
                        mark = SEC_MARK2
                    else:
                        i = -1
                        mark = None

                    if i == -1:
                        # No secure frame marker present -> treat what we can as plaintext lines
                        # (BUT do not print partial line - keep it in sec_buf)
                        sec_buf = consume_plaintext_lines_from_prefix(sec_buf)
                        break

                    # If there are bytes before the marker, treat them as plaintext stream
                    if i > 0:
                        prefix = sec_buf[:i]
                        sec_buf = sec_buf[i:]
                        leftover = consume_plaintext_lines_from_prefix(prefix)

                        # If there is leftover partial plaintext, keep it before the marker
                        if leftover:
                            sec_buf = leftover + sec_buf

                        # Continue loop to process +SEC now at/near start
                        continue

                    # i == 0 -> sec_buf begins with "+SEC:" or "\r\n+SEC:"
                    mark_len = len(mark)

                    # Need comma after "+SEC:<len>,"
                    j = sec_buf.find(b",", mark_len)
                    if j == -1:
                        break  # need more bytes

                    len_str = sec_buf[mark_len:j]
                    if not len_str.isdigit():
                        # Resync: drop 1 byte and retry
                        sec_buf = sec_buf[1:]
                        continue

                    cipher_len = int(len_str)
                    start = j + 1
                    end = start + cipher_len

                    if len(sec_buf) < end:
                        break  # need more ciphertext bytes

                    cipher = sec_buf[start:end]
                    sec_buf = sec_buf[end:]  # consume secure frame (no trailing CRLF required)

                    # Show secure frame in RX window (minimal)
                    received_data_area.insert(tk.END, f"+SEC:{cipher_len},<bin>\r\n")
                    received_data_area.yview(tk.END)

                    # Decrypt using existing API (expects hex string)
                    try:
                        decrypted = decrypt_data(cipher.hex())
                        plain_data_area.insert(tk.END, "+" + f"{decrypted}\r\n")
                    except Exception as e:
                        plain_data_area.insert(tk.END, f"[Decrypt Error] {e}\n\n")
                    plain_data_area.yview(tk.END)

                    # Continue loop: there may be plaintext OK/ERROR right after this

                # Prevent runaway buffer growth if device spams without line endings
                if len(sec_buf) > 8192:
                    sec_buf = sec_buf[-8192:]

                continue  # done handling secure-mode input

            # ---------- original logic (unchanged) for non-secure mode ----------
            raw_line = com_port.read_until(b'\r\n')
            print(">> HEX:", raw_line.hex())
            incomplete_line += raw_line
            if b"\r\n" not in incomplete_line:
                print("there is no rn\r\n")
                continue
            elif waiting_for_more_data:
                full_line, remaining = incomplete_line.rsplit(b"\r\n", 1)
                incomplete_line = remaining
                waiting_for_more_data = False
            else:
                full_line, remaining = incomplete_line.rsplit(b"\r\n", 1)
                incomplete_line = remaining

            try:
                decoded_line = full_line.decode('utf-8', errors='ignore')
            except:
                decoded_line = "<decode error>"

            # Device confirmation that secure channel is OFF
            if "SECUREC CHANNEL:0" in decoded_line:
                plain_data_area.insert(tk.END, "Secure Channel disabled.\n\n")
                plain_data_area.yview(tk.END)


            received_data_area.insert(tk.END, "\r\n" + f"{decoded_line}\r\n")
            received_data_area.yview(tk.END)

            plain_data_area.insert(tk.END, "\r\n" + f"{decoded_line}\n\n")
            plain_data_area.yview(tk.END)

def start_receiving_thread():
    global stop_receiving
    stop_receiving = False
    threading.Thread(target=receive_data, daemon=True).start()
    received_data_area.insert(tk.END, "Started data receiving thread.\n\n")
    plain_data_area.insert(tk.END, "Started data receiving thread.\n\n")

def send_at_command(at_command=None):
    global com_port
    if not com_port:
        plain_data_area.insert(tk.END, "Error: COM port is not open.\n\n")
        return

    command_str = at_command or at_command_entry.get().strip()
    command_bytes = parse_hex_escapes(command_str)

    if secure_channel_active and command_bytes.startswith(b'\x1BCERT,'):
        raw = command_bytes.rstrip(b'\r\n')
        parts = raw.split(b',', 6)

        if len(parts) < 7:
            plain_data_area.insert(tk.END, "[ERROR]: Invalid ESC CERT format\n\n")
        else:
            header_without_len = b','.join(parts[:5])  # \x1BCERT,0,0,0,1
            orig_len_field = int(parts[5].decode('ascii', errors='replace'))
            plain_content = parts[6]
            actual_len = len(plain_content)

            if orig_len_field != actual_len:
                plain_data_area.insert(
                    tk.END,
                    f"\n[ERROR] Length mismatch: user_len={orig_len_field}, actual_len={actual_len}.\n"
                    f"Use <length>={actual_len}. Not sending.\n\n"
                )
                return

            print("\n=== ESC CERT ===")
            encrypted_argument = encrypt_argument(plain_content.decode('utf-8'), plain_data_area)
            length_field = len(encrypted_argument)
            final_command = (header_without_len +
                    b',' + str(length_field).encode('ascii') + b',' +
                    encrypted_argument + b'\r\n')

            com_port.write(final_command)
            plain_data_area.insert(tk.END, f"[Plain ESC CERT]: {command_str}\n")
            plain_data_area.insert(tk.END, f"[Orig len field]: {orig_len_field}\n")
            plain_data_area.insert(tk.END, f"[Plain len]: {len(plain_content)}\n")
            plain_data_area.insert(tk.END, f"[len field]: {length_field}\n")

            preview_command = (
                    header_without_len +
                    b',' + str(length_field).encode('ascii') + b',' +
                    b'<PLAINTEXT>' + b'\r\n'  # placeholder (not real bytes sent)
            )
            plain_data_area.insert(tk.END,
                                   f"[Preview cmd]: {preview_command.decode('ascii', errors='backslashreplace')}\n")
            #plain_data_area.insert(tk.END, f"[Sent Bytes]: {final_command.hex()}\n\n")

    elif secure_channel_active and b'=' in command_bytes and not (
        command_bytes.upper().startswith(b'ATS=') and
        re.fullmatch(rb'ATS=[0-9A-Fa-f]{32}', command_bytes)
    ):
        plain_command, argument = command_bytes.split(b'=', 1)

        # Encrypt argument -> raw AES-CBC ciphertext bytes
        cipher = encrypt_argument(argument.decode('utf-8', errors='ignore'), plain_data_area)

        # Length framing (ASCII)
        len_ascii = str(len(cipher)).encode('ascii')

        # AT+CMD=<len>,<ciphertext>\r\n
        final_command = plain_command + b'=' + len_ascii + b',' + cipher + b'\r\n'

        com_port.write(final_command)

        plain_data_area.insert(tk.END, f"[Cipher Len]: {len(cipher)}\n\n")
        plain_data_area.insert(tk.END, f"[Encrypted Argument HEX]: {cipher.hex()}\n\n")
        plain_data_area.insert(tk.END, f"[Sent Bytes HEX]: {final_command.hex()}\n\n")
    else:
        final_command = command_bytes + b"\r\n"
        com_port.write(final_command)
        plain_data_area.insert(tk.END, f"[Sent]: {command_str}\n\n")

def handle_ats_button():
    global secure_channel_active, last_received_block, last_encrypted_block
    iv_value = iv_entry.get().strip()

    if secure_channel_active:
        send_at_command("ATS=off")
        secure_channel_active = False
        ats_button.config(text="Enable/Sync Secure Channel")
        plain_data_area.insert(tk.END, "Disable command sent.\n\n")

        # Prep fresh IV for next enable
        random_iv = get_random_bytes(16)
        last_encrypted_block = random_iv
        last_received_block = random_iv
        iv_entry.delete(0, tk.END)
        iv_entry.insert(0, random_iv.hex())
    else:
        if len(iv_value) != 32:
            plain_data_area.insert(tk.END, "Error: IV must be 32 characters long.\n\n")
            return

        send_at_command(f"ATS={iv_value}")
        secure_channel_active = True
        ats_button.config(text="Disable Secure Channel")
        plain_data_area.insert(tk.END, "Secure Channel enabled.\n\n")
        last_encrypted_block = bytes.fromhex(iv_value)
        last_received_block = bytes.fromhex(iv_value)

def pad(data):
    padding_size = AES.block_size - len(data) % AES.block_size
    return data + chr(padding_size) * padding_size

def unpad(data):
    padding_size = data[-1]
    return data[:-padding_size]

def toggle_port():
    global com_port, stop_receiving

    if com_port and com_port.is_open:
        stop_receiving = True
        com_port.close()
        com_port = None
        port_button.config(text="Open Port", bg="lightgreen")
        received_data_area.insert(tk.END, "Port Closed.\n\n")
        plain_data_area.insert(tk.END, "Port Closed.\n\n")
    else:
        com_port_name = com_port_var.get()
        try:
            #com_port = serial.Serial(port=com_port_name, baudrate=115200, timeout=1)
            com_port = serial.Serial(port=com_port_name, baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,timeout=1,)
            if os.name != 'nt':  # Linux or MAC
                set_linux_raw_mode(com_port)
            stop_receiving = False
            start_receiving_thread()
            port_button.config(text="Close Port", bg="lightcoral")
            received_data_area.insert(tk.END, f"Port Opened: {com_port_name}\n\n")
            plain_data_area.insert(tk.END, f"Port Opened: {com_port_name}\n\n")
        except Exception as e:
            plain_data_area.insert(tk.END, f"Failed to open port: {e}\n\n")

# UI setup
root = tk.Tk()
text = tk.Text(root, font=("Ubuntu mono", 12))
root.title("ATS Secure Channel Control")

com_port = None

port_frame = tk.Frame(root)
port_frame.pack(anchor="w", pady=10)
tk.Label(port_frame, text="Select COM Port:").pack(side=tk.LEFT)
com_port_var = tk.StringVar(value="")
available_ports = [port.device for port in serial.tools.list_ports.comports()]
if not available_ports:
    print("No COM ports found!")
    sys.exit()
com_port_menu = tk.OptionMenu(port_frame, com_port_var, *available_ports)
com_port_menu.pack(side=tk.LEFT)
port_button = tk.Button(port_frame, text="Open Port", command=toggle_port, bg="lightgreen")
port_button.pack(side=tk.LEFT, padx=5)

command_frame = tk.Frame(root)
command_frame.pack(pady=10)
tk.Label(command_frame, text="Enter full AT Command (including arguments):").pack(side=tk.LEFT)
at_command_entry = tk.Entry(command_frame, width=50)
at_command_entry.pack(side=tk.LEFT)
at_command_entry.bind("<Return>", lambda event: send_at_command())
tk.Button(command_frame, text="Send AT Command", command=send_at_command).pack(side=tk.LEFT, padx=5)

iv_frame = tk.Frame(root)
iv_frame.pack(pady=10)
tk.Label(iv_frame, text="Enter IV Value (32 characters):").pack(side=tk.LEFT)
iv_entry = tk.Entry(iv_frame, width=50)
iv_entry.insert(0, random_iv.hex())
iv_entry.pack(side=tk.LEFT)
ats_button = tk.Button(iv_frame, text="Enable/Sync Secure Channel", width=22, command=handle_ats_button)
ats_button.pack(side=tk.LEFT, padx=5)

log_frame = tk.Frame(root)
log_frame.pack(pady=10)

tk.Label(log_frame, text="Received Data Log:").pack()
received_data_area = scrolledtext.ScrolledText(log_frame, width=60, height=10)
received_data_area.pack()

tk.Label(log_frame, text="Plain Data Log:").pack()
plain_data_area = scrolledtext.ScrolledText(log_frame, width=60, height=10)
plain_data_area.pack()

root.mainloop()