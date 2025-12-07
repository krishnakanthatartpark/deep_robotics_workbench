#pragma once
#include <rclcpp/rclcpp.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <string>
#include <memory>



struct UdpPacket {
uint32_t code;
uint32_t value;
uint32_t type;
} __attribute__((packed));


class UdpClient {
public:
UdpClient(const std::string &ip, uint16_t port, double recv_timeout_sec);
~UdpClient();


bool send_command(uint32_t code, uint32_t value = 0, uint32_t type = 0);
void set_logger(std::shared_ptr<rclcpp::Node> logger_node);


private:
void open_socket();
void close_socket();
void recv_response_locked();


std::string ip_;
uint16_t port_;
double recv_timeout_sec_;
int sock_fd_;
struct sockaddr_in remote_addr_;
std::mutex mutex_;
std::shared_ptr<rclcpp::Node> logger_;
};