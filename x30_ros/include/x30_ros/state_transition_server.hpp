#pragma once
#include "x30_ros/udp_client.hpp"
#include <rclcpp/rclcpp.hpp>
#include <x30_interfaces/srv/mode.hpp>


class StateTransitionServer : public rclcpp::Node {
public:
StateTransitionServer();
~StateTransitionServer() = default;

private:
using Mode = x30_interfaces::srv::Mode;
rclcpp::TimerBase::SharedPtr logger_init_timer_;
void send_and_fill(uint32_t code, Mode::Response::SharedPtr response);
void handle_set_mode(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);



std::unique_ptr<UdpClient> udp_client_;


rclcpp::Service<Mode>::SharedPtr set_mode_srv_;

};