// src/x30_hal_main.cpp
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "x30_ros/x30_hal_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // Create lifecycle node instance
  auto node = std::make_shared<x30_hal::X30HalLifecycleNode>(rclcpp::NodeOptions());

  // Use executor and add node's base interface so lifecycle services work
  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(node->get_node_base_interface());
  exec.spin();

  rclcpp::shutdown();
  return 0;
}
