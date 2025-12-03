#!/usr/bin/env python3
"""
Simple UDP server simulator for testing x30_state_controller
Listens on UDP port 43893 and echoes back received commands
"""
import socket
import struct

UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 43893

CMD_FMT = "<III"  # code, value, type (uint32_t each, little-endian)

# Command name mapping for logging
COMMAND_NAMES = {
    0x21010202: "SIT/STAND",
    0x2101020A: "TORQUE_CTRL",
    0x21010201: "STEP_CTRL",
}

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    
    print(f"[UDP Simulator] Listening on {UDP_IP}:{UDP_PORT}")
    print("[UDP Simulator] Waiting for commands...\n")
    
    try:
        while True:
            data, addr = sock.recvfrom(1024)
            
            if len(data) >= 12:
                # Unpack received command
                code, value, cmd_type = struct.unpack(CMD_FMT, data[:12])
                cmd_name = COMMAND_NAMES.get(code, "UNKNOWN")
                
                print(f"[RECV] from {addr[0]}:{addr[1]}")
                print(f"  Command: 0x{code:08X} ({cmd_name})")
                print(f"  Value:   {value}")
                print(f"  Type:    {cmd_type}")
                
                # Echo back the same command as acknowledgment
                response = struct.pack(CMD_FMT, code, value, cmd_type)
                sock.sendto(response, addr)
                print(f"[SEND] Echo response sent\n")
            else:
                print(f"[WARN] Received malformed packet ({len(data)} bytes) from {addr}")
    
    except KeyboardInterrupt:
        print("\n[UDP Simulator] Shutting down...")
    finally:
        sock.close()

if __name__ == '__main__':
    main()
