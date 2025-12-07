#include "x30_ros/udp_client.hpp"
std::string hex_u32(uint32_t v) {
std::ostringstream ss;
ss << "0x" << std::uppercase << std::hex << v;
return ss.str();
}


UdpClient::UdpClient(const std::string &ip, uint16_t port, double recv_timeout_sec)
: ip_(ip), port_(port), recv_timeout_sec_(recv_timeout_sec), sock_fd_(-1) {
open_socket();
}


UdpClient::~UdpClient() {
close_socket();
}


void UdpClient::set_logger(std::shared_ptr<rclcpp::Node> logger_node) {
logger_ = logger_node;
}


bool UdpClient::send_command(uint32_t code, uint32_t value, uint32_t type) {
std::lock_guard<std::mutex> lock(mutex_);
if (sock_fd_ < 0) return false;


UdpPacket p{code, value, type};


ssize_t s = sendto(sock_fd_, &p, sizeof(p), 0,
(struct sockaddr *)&remote_addr_, sizeof(remote_addr_));


if (s != sizeof(p)) return false;


recv_response_locked();
return true;
}


void UdpClient::open_socket() {
std::lock_guard<std::mutex> lock(mutex_);
sock_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


memset(&remote_addr_, 0, sizeof(remote_addr_));
remote_addr_.sin_family = AF_INET;
remote_addr_.sin_port = htons(port_);
inet_pton(AF_INET, ip_.c_str(), &remote_addr_.sin_addr);
}


void UdpClient::close_socket() {
std::lock_guard<std::mutex> lock(mutex_);
if (sock_fd_ >= 0) close(sock_fd_);
sock_fd_ = -1;
}


void UdpClient::recv_response_locked() {
uint8_t buf[1024];
struct sockaddr_in from;
socklen_t len = sizeof(from);
recvfrom(sock_fd_, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
}