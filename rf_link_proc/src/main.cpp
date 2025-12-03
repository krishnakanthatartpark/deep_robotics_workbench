#include "rf_signal_node.h"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<RFSignalNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
