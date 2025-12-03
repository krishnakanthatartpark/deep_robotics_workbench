#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <atomic>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "std_msgs/msg/int32.hpp"
#include "x30_interfaces/srv/mode.hpp"

#include <x30_robotserver_sdk/robotserver_sdk.h> // SDK header

namespace x30_hal {

/**
 * Lifecycle HAL node for X30 robot.
 *
 * States:
 *  - on_configure: construct SDK object (no connect)
 *  - on_activate: connect SDK and subscribe to cmd_vel
 *  - on_deactivate: unsubscribe and disconnect SDK
 *  - on_cleanup: destroy SDK object
 */
class X30HalLifecycleNode : public rclcpp_lifecycle::LifecycleNode {
public:
  explicit X30HalLifecycleNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~X30HalLifecycleNode() override;

  // lifecycle callbacks
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_configure(const rclcpp_lifecycle::State &);
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_activate(const rclcpp_lifecycle::State &);
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State &);
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State &);
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State &);

private:
  // cmd_vel callback (only active when node is ACTIVE)
  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg);

  void stateCommandCallback(const std_msgs::msg::Int32::SharedPtr msg);

  // Mode service callback  
  void modeServiceCallback(
    const std::shared_ptr<x30_interfaces::srv::Mode::Request> request,
    std::shared_ptr<x30_interfaces::srv::Mode::Response> response);

  // Motion helper that calls the SDK (thread-safely)
  void sendMotionCommand(float linear_x, float linear_y, float angular_z);

  void handleStateCommand(int command_id);


  // Members
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;

  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr state_cmd_sub_;

  // Mode service server
  rclcpp::Service<x30_interfaces::srv::Mode>::SharedPtr mode_service_;

  // SDK instance (constructed on configure)
  std::unique_ptr<robotserver_sdk::RobotServerSdk> sdk_;
  std::mutex sdk_mutex_; // protects sdk_

  // connection parameters
  std::string sdk_host_;
  uint16_t sdk_port_;

  // connection state
  std::atomic<bool> sdk_connected_{false};

  // small deadband and protocol parameters
  static constexpr float EPS = 1e-3f;
  static constexpr float MAX_FWD = 1.50f;
  static constexpr float MAX_BWD = 1.30f;
  static constexpr float MAX_ANG = 0.45f;
  static constexpr float MAX_LATERAL = 0.15f;
};

} // namespace x30_hal
