#pragma once
#include "udp_client.hpp"
#include <rclcpp/rclcpp.hpp>
#include <x30_interfaces/srv/mode.hpp>


class StateTransitionServer : public rclcpp::Node {
public:
StateTransitionServer();
~StateTransitionServer() = default;

private:
using Mode = x30_interfaces::srv::Mode;
void send_and_fill(uint32_t code, Mode::Response::SharedPtr response);
void handle_sit_stand(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);
void handle_torque_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);
void handle_step_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);
void handle_crawl_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);
void handle_run_sequence(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response>);


std::unique_ptr<UdpClient> udp_client_;


rclcpp::Service<Mode>::SharedPtr sit_stand_srv_;
rclcpp::Service<Mode>::SharedPtr torque_srv_;
rclcpp::Service<Mode>::SharedPtr step_srv_;
rclcpp::Service<Mode>::SharedPtr crawl_srv_;
rclcpp::Service<Mode>::SharedPtr sequence_srv_;
};