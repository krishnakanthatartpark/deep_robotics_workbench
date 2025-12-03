#pragma once
#include "udp_client.hpp"
#include <rclcpp/rclcpp.hpp>
#include <example_interfaces/srv/trigger.hpp>


class StateTransitionServer : public rclcpp::Node {
public:
StateTransitionServer();


private:
using Trigger = example_interfaces::srv::Trigger;
void send_and_fill(uint32_t code, Trigger::Response::SharedPtr response);
void handle_sit_stand(const std::shared_ptr<Trigger::Request>, std::shared_ptr<Trigger::Response>);
void handle_torque_ctrl(const std::shared_ptr<Trigger::Request>, std::shared_ptr<Trigger::Response>);
void handle_step_ctrl(const std::shared_ptr<Trigger::Request>, std::shared_ptr<Trigger::Response>);
void handle_crawl_ctrl(const std::shared_ptr<Trigger::Request>, std::shared_ptr<Trigger::Response>);
void handle_run_sequence(const std::shared_ptr<Trigger::Request>, std::shared_ptr<Trigger::Response>);


std::unique_ptr<UdpClient> udp_client_;


rclcpp::Service<Trigger>::SharedPtr sit_stand_srv_;
rclcpp::Service<Trigger>::SharedPtr torque_srv_;
rclcpp::Service<Trigger>::SharedPtr step_srv_;
rclcpp::Service<Trigger>::SharedPtr crawl_srv_;
rclcpp::Service<Trigger>::SharedPtr sequence_srv_;
};