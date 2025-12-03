#!/usr/bin/env python3
import socket
import struct
import time

# === Motion host target ===
UDP_IP = "192.168.1.103"
UDP_PORT = 43893

# === UDP command format ===
CMD_FMT = "<III"  # code, value, type (uint32_t each, little-endian)

# === Basic state transition commands ===
CMD_SIT_STAND = 0x21010202
CMD_TORQUE_CTRL = 0x2101020A
CMD_STEP_CTRL = 0x21010201
CMD_CRWL_CTRL = 0x21010406

# === Helper: Send a single UDP command ===
def send_command(sock, code, value=0, cmd_type=0):
    packet = struct.pack(CMD_FMT, code, value, cmd_type)
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"[SEND] code=0x{code:X}, value={value}, type={cmd_type}")

# === Helper: Try to receive any feedback ===
def recv_response(sock):
    try:
        data, addr = sock.recvfrom(1024)
        if len(data) >= 12:
            code, value, typ = struct.unpack(CMD_FMT, data[:12])
            print(f"[RECV] from {addr}: code=0x{code:X}, value={value}, type={typ}")
        else:
            print(f"[RECV] raw ({len(data)} bytes): {data}")
    except socket.timeout:
        pass

# === Run a full state transition sequence ===
def run_state_test(sock):
    print("\n--- State Transition Test Sequence ---\n")

    # 1. Sit / Stand toggle
    print("→ Toggle Sit/Stand")
    send_command(sock, CMD_SIT_STAND)
    recv_response(sock)
    time.sleep(5)

    # # 2. Switch to torque control
    # print("→ Toggle Torque-Controlled Standing")
    # send_command(sock, CMD_TORQUE_CTRL)
    # recv_response(sock)
    # time.sleep(5)

    # # # 3. Start stepping
    # print("→ Toggle Stepping Mode (start walking)")
    # send_command(sock, CMD_STEP_CTRL)
    # recv_response(sock)
    # time.sleep(5)

    # # # 4. Stop stepping (send same command again)
    print("→ Stop Stepping")
    send_command(sock, CMD_STEP_CTRL)
    recv_response(sock)
    time.sleep(5)

    # # 5. Return to sitting
    print("→ Return to Sit/Stand toggle")
    send_command(sock, CMD_SIT_STAND)
    recv_response(sock)
    time.sleep(3)

    # print("→ Toggle Sit/Stand")
    # send_command(sock, CMD_SIT_STAND)
    # recv_response(sock)
    # time.sleep(5)

    # # 2. Switch to torque control
    print("→ Toggle Torque-Controlled Standing")
    send_command(sock, CMD_TORQUE_CTRL)
    recv_response(sock)
    time.sleep(5)

    print("→ Toggle Stepping Mode (start walking)")
    send_command(sock, CMD_STEP_CTRL)
    recv_response(sock)
    time.sleep(5)

    print("→ Toggle Crawl Mode")
    send_command(sock, CMD_CRWL_CTRL)
    recv_response(sock)
    time.sleep(5)

    print("\n✅ State transition test complete.")


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(0.5)

    print(f"Connecting to {UDP_IP}:{UDP_PORT}")

    try:
        run_state_test(sock)
    except KeyboardInterrupt:
        print("\nInterrupted by user.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()

