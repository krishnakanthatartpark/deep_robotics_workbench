#!/usr/bin/env python3
import socket
import struct
import time

# === UDP target (motion host) ===
UDP_IP = "192.168.1.103"
UDP_PORT = 43893

# === UDP command format ===
# struct CommandHead { uint32_t code; uint32_t paramters_size; uint32_t type; }
CMD_FMT = "<III"

# === Common command codes ===
CMD_HEARTBEAT = 0x21040001
CMD_CONNECT_CONFIRM = 0x21020001


def send_command(sock, code, value=0, cmd_type=0):
    """Send a 12-byte simple command"""
    packet = struct.pack(CMD_FMT, code, value, cmd_type)
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"Sent command: code=0x{code:X}, value={value}, type={cmd_type}")


def recv_response(sock):
    """Non-blocking receive for any response"""
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) >= 12:
            code, val, typ = struct.unpack(CMD_FMT, data[:12])
            print(f"Received from {addr}: code=0x{code:X}, value={val}, type={typ}")
        else:
            print(f"Received raw data ({len(data)} bytes): {data}")
    except socket.timeout:
        pass


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(0.5)

    print(f"Connecting to {UDP_IP}:{UDP_PORT}")
    print("Sending connection confirmation command once...")
    send_command(sock, CMD_CONNECT_CONFIRM)

    time.sleep(0.5)

    print("\nStarting heartbeat loop (2 Hz)... Press Ctrl+C to exit.\n")

    try:
        while True:
            send_command(sock, CMD_HEARTBEAT)
            recv_response(sock)
            time.sleep(0.5)  # 2 Hz heartbeat
    except KeyboardInterrupt:
        print("\nExiting cleanly.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()

