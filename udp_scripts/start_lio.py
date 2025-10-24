import socket
import struct

# UDP target
UDP_IP = "192.168.1.105"
UDP_PORT = 60000

# Message fields
code = 0xBAA0001
value = 1
type_field = 0

# Pack data as binary (3 unsigned integers)
# Assuming 4-byte unsigned ints (network byte order, big-endian)
msg = struct.pack('>III', code, value, type_field)

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Send the message
sock.sendto(msg, (UDP_IP, UDP_PORT))
print("Message sent!")

# Optional: receive feedback
sock.settimeout(2)  # wait max 2 seconds
try:
    data, addr = sock.recvfrom(1024)
    recv_code, recv_value, recv_type = struct.unpack('>III', data)
    print(f"Received feedback: code={hex(recv_code)}, value={recv_value}, type={recv_type}")
except socket.timeout:
    print("No response received.")

