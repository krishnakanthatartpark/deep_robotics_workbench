#!/usr/bin/env python3
import socket
import struct
import time

# === Motion host target ===
UDP_IP = "192.168.1.103"
UDP_PORT = 43893

# === UDP command format ===
CMD_FMT = "<III"  # code, value, type (uint32_t each, little-endian)

# === Control mode commands ===
CMD_NON_MANUAL = 0x21010C03
CMD_MANUAL = 0x21010C02

# === Helper: Send UDP command ===
def send_command(sock, code, value=0, cmd_type=0):
    packet = struct.pack(CMD_FMT, code, value, cmd_type)
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"[SEND] code=0x{code:X}, value={value}, type={cmd_type}")

# === Optional: receive response ===
def recv_response(sock):
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) >= 12:
            code, val, typ = struct.unpack(CMD_FMT, data[:12])
            print(f"[RECV] from {addr}: code=0x{code:X}, value={val}, type={typ}")
        else:
            print(f"[RECV] raw ({len(data)} bytes): {data}")
    except socket.timeout:
        pass

# === Run control mode test sequence ===
def run_control_mode_test(sock):
    print("\n--- Control Mode Test ---\n")

    # 1. Switch to Non-Manual Mode
    print("→ Switching to Non-Manual Mode")
    send_command(sock, CMD_NON_MANUAL)
    recv_response(sock)
    time.sleep(2)

    # 2. Switch to Manual Mode
    print("→ Switching to Manual Mode")
    send_command(sock, CMD_MANUAL)
    recv_response(sock)
    time.sleep(2)

    # 3. Switch back to Non-Manual Mode
    print("→ Switching back to Non-Manual Mode")
    send_command(sock, CMD_NON_MANUAL)
    recv_response(sock)
    time.sleep(2)

    print("\n✅ Control mode test complete.")


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(0.5)

    print(f"Connecting to {UDP_IP}:{UDP_PORT}")

    try:
        run_control_mode_test(sock)
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()

