#include "x30_ros/x30_hal_node.hpp"

#include <chrono>

using namespace std::chrono_literals;

namespace x30_hal {

X30HalLifecycleNode::X30HalLifecycleNode(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("x30_hal_lifecycle_node", options)
{
  // declare parameters with defaults
  this->declare_parameter<std::string>("sdk_host", "192.168.1.106");
  this->declare_parameter<int>("sdk_port", 30000);

  // we read parameters in on_configure to allow reconfigure later
  RCLCPP_INFO(this->get_logger(), "X30 HAL lifecycle node constructed");
}

X30HalLifecycleNode::~X30HalLifecycleNode()
{
  // ensure sdk disconnected and destroyed
  std::lock_guard<std::mutex> lock(sdk_mutex_);
  if (sdk_) {
    try {
      if (sdk_connected_.load()) {
        sdk_->disconnect();
        sdk_connected_.store(false);
      }
    } catch (...) { }
  }
}

/**
 * on_configure: create SDK instance and read parameters.
 * No network side-effects yet.
 */
rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
X30HalLifecycleNode::on_configure(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(this->get_logger(), "on_configure()");

  // read parameters
  this->get_parameter("sdk_host", sdk_host_);
  int port_i = 9000;
  this->get_parameter("sdk_port", port_i);
  sdk_port_ = static_cast<uint16_t>(port_i);

  // Create SDK object (no connect)
  try {
    robotserver_sdk::SdkOptions opts;
    opts.connectionTimeout = std::chrono::milliseconds(5000);
    opts.requestTimeout = std::chrono::milliseconds(3000);

    std::lock_guard<std::mutex> lock(sdk_mutex_);
    sdk_ = std::make_unique<robotserver_sdk::RobotServerSdk>(opts);

    RCLCPP_INFO(this->get_logger(), "SDK object constructed (host=%s port=%u)", sdk_host_.c_str(), sdk_port_);
  } catch (const std::exception &e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to construct SDK: %s", e.what());
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
  }

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

/**
 * on_activate: connect SDK and create cmd_vel subscription
 */
rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
X30HalLifecycleNode::on_activate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(this->get_logger(), "on_activate()");

  // Attempt to connect to SDK
  {
    std::lock_guard<std::mutex> lock(sdk_mutex_);
    if (!sdk_) {
      RCLCPP_ERROR(this->get_logger(), "SDK not created (configure first)");
      return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
    }

    try {
      if (sdk_->connect(sdk_host_, sdk_port_)) {
        sdk_connected_.store(true);
        RCLCPP_INFO(this->get_logger(), "Connected to SDK at %s:%u", sdk_host_.c_str(), sdk_port_);
      } else {
        sdk_connected_.store(false);
        RCLCPP_ERROR(this->get_logger(), "Failed to connect to SDK at %s:%u", sdk_host_.c_str(), sdk_port_);
        // Let activation succeed but mark failure? Better to return FAILURE so user can retry.
        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
      }
    } catch (const std::exception &e) {
      RCLCPP_ERROR(this->get_logger(), "Exception while connecting to SDK: %s", e.what());
      return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
    }
  }

  // Create cmd_vel subscription (only while ACTIVE)
  // Subscription is stored in member so it will be dropped in on_deactivate
  cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel", rclcpp::QoS(1),
    std::bind(&X30HalLifecycleNode::cmdVelCallback, this, std::placeholders::_1)
  );

    // Optional: State command subscription
  state_cmd_sub_ = this->create_subscription<std_msgs::msg::Int32>(
    "state_command", rclcpp::QoS(10),
    std::bind(&X30HalLifecycleNode::stateCommandCallback, this, std::placeholders::_1)
  );


  RCLCPP_INFO(this->get_logger(), "cmd_vel subscription created and HAL node activated");
  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

/**
 * on_deactivate: stop subscription and disconnect SDK
 */
rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
X30HalLifecycleNode::on_deactivate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(this->get_logger(), "on_deactivate()");

  // destroy subscription so callbacks stop arriving
  cmd_vel_sub_.reset();
  state_cmd_sub_.reset();


  // disconnect SDK
  {
    std::lock_guard<std::mutex> lock(sdk_mutex_);
    if (sdk_ && sdk_connected_.load()) {
      try {
        sdk_->disconnect();
        sdk_connected_.store(false);
        RCLCPP_INFO(this->get_logger(), "Disconnected SDK on deactivate");
      } catch (const std::exception &e) {
        RCLCPP_WARN(this->get_logger(), "Exception while disconnecting SDK: %s", e.what());
      }
    }
  }

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

/**
 * on_cleanup: destroy SDK object and any resources
 */
rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
X30HalLifecycleNode::on_cleanup(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(this->get_logger(), "on_cleanup() - destroying SDK and cleaning up");

  // destroy subscription if still present
  cmd_vel_sub_.reset();
  state_cmd_sub_.reset();

  // destroy SDK
  {
    std::lock_guard<std::mutex> lock(sdk_mutex_);
    if (sdk_) {
      try {
        if (sdk_connected_.load()) {
          sdk_->disconnect();
        }
      } catch (...) { }
      sdk_.reset();
      sdk_connected_.store(false);
    }
  }

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

/**
 * on_shutdown: called during node shutdown
 */
rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
X30HalLifecycleNode::on_shutdown(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(this->get_logger(), "on_shutdown()");
  // re-use cleanup logic
  return on_cleanup(rclcpp_lifecycle::State());
}


// ==============================================================================================================
/**
 * cmd_vel callback — only active when node is ACTIVE (subscription created in on_activate)
 */
void X30HalLifecycleNode::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  if (!msg) return;
  sendMotionCommand(static_cast<float>(msg->linear.x),
                    static_cast<float>(msg->linear.y),
                    static_cast<float>(msg->angular.z));
}

/**
 * Convert cmd_vel to SDK motion control calls (thread safe)
 */
void X30HalLifecycleNode::sendMotionCommand(float linear_x, float linear_y, float angular_z)
{
  // local clamp helper
  auto clampf = [](float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  };

  // log quick debug
  RCLCPP_DEBUG(this->get_logger(), "sendMotionCommand: x=%.3f y=%.3f ang=%.3f", linear_x, linear_y, angular_z);

  std::lock_guard<std::mutex> lock(sdk_mutex_);
  if (!sdk_) {
    RCLCPP_WARN(this->get_logger(), "SDK object not available");
    return;
  }
  if (!sdk_connected_.load()) {
    RCLCPP_WARN(this->get_logger(), "SDK not connected, ignoring cmd_vel");
    return;
  }

  // callback used for all motion control calls
  auto cb = [logger = this->get_logger()](const robotserver_sdk::MotionControlResult &res) {
    RCLCPP_INFO(logger, "MotionControlResult: errorCode=%d", static_cast<int>(res.errorCode));
  };

  bool sent_any = false;

  // lateral (11 left, 12 right)
  if (std::fabs(linear_y) > EPS) {
    float v = clampf(std::fabs(linear_y), 0.0f, MAX_LATERAL);
    if (linear_y > 0.0f) {
      sdk_->request2_Motion_Control(11, v, cb);
    } else {
      sdk_->request2_Motion_Control(12, v, cb);
    }
    sent_any = true;
  }

  // forward/back (1 forward, 2 backward)
  if (std::fabs(linear_x) > EPS) {
    float raw = std::fabs(linear_x);
    float vmax = (linear_x > 0.0f) ? MAX_FWD : MAX_BWD;
    float v = clampf(raw, 0.0f, vmax);
    if (linear_x > 0.0f) {
      sdk_->request2_Motion_Control(1, v, cb);
    } else {
      sdk_->request2_Motion_Control(2, v, cb);
    }
    sent_any = true;
  }

  // rotation (3 left, 4 right)
  if (std::fabs(angular_z) > EPS) {
    float av = clampf(std::fabs(angular_z), 0.0f, MAX_ANG);
    if (angular_z > 0.0f) {
      sdk_->request2_Motion_Control(3, av, cb);
    } else {
      sdk_->request2_Motion_Control(4, av, cb);
    }
    sent_any = true;
  }

  if (!sent_any) {
    // set velocity to 0
    sdk_->request2_Motion_Control(6, 0.0f, cb);
  }
}

void X30HalLifecycleNode::stateCommandCallback(const std_msgs::msg::Int32::SharedPtr msg)
{
  if (!msg) return;
  handleStateCommand(msg->data);
}


void X30HalLifecycleNode::handleStateCommand(int command_id)
{
  std::lock_guard<std::mutex> lock(sdk_mutex_);
  if (!sdk_) {
    RCLCPP_WARN(this->get_logger(), "SDK object not available for State command");
    return;
  }
  if (!sdk_connected_.load()) {
    RCLCPP_WARN(this->get_logger(), "SDK not connected, ignoring State command");
    return;
  }

  auto cb = [logger = this->get_logger()](const robotserver_sdk::MotionControlResult &res) {
    RCLCPP_INFO(logger, "State MotionControlResult: errorCode=%d", static_cast<int>(res.errorCode));
  };

  RCLCPP_INFO(this->get_logger(), "Handling State Command ID: %d", command_id);

  switch (command_id)
  { 
    case 13: // Emergency Stop
    case 14: // Stop Stepping
    case 15: // Sit Down
    case 16: // Stand Up
    case 18: // Start Stepping
    case 20: // Switch Gait
      sdk_->request2_Motion_Control(command_id, -1.0f, cb);
      break;
    default:
      RCLCPP_WARN(this->get_logger(), "Unknown State Command ID: %d", command_id);
      break;
  }
}

} // namespace x30_hal

// Register lifecycle node as component so it can be loaded by component manager
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(x30_hal::X30HalLifecycleNode)
