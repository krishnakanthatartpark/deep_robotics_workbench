#include "x30_ros/state_transition_server.hpp"
#include <chrono>
#include <thread>


static constexpr uint32_t CMD_SIT_STAND = 0x21010202;
static constexpr uint32_t CMD_TORQUE_CTRL = 0x2101020A;
static constexpr uint32_t CMD_STEP_CTRL = 0x21010201;
static constexpr uint32_t CMD_CRWL_CTRL = 0x21010406;
static constexpr uint32_t SWITCH_VEL_SRC = 0x3101EE03;


StateTransitionServer::StateTransitionServer() : Node("state_transition_server") {
std::string ip = this->declare_parameter("motion_ip", "0.0.0.0"); 
int port = this->declare_parameter("motion_port", 43893);
double timeout = this->declare_parameter("recv_timeout_sec", 0.5);


udp_client_ = std::make_unique<UdpClient>(ip, port, timeout);
logger_init_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100),
      [this]() {
        udp_client_->set_logger(shared_from_this());
        logger_init_timer_->cancel();
        RCLCPP_INFO(this->get_logger(), "Logger attached to UDP client");
      }
  );


set_mode_srv_ = create_service<Mode>("/set_mode",
std::bind(&StateTransitionServer::handle_set_mode, this, std::placeholders::_1, std::placeholders::_2));


}


void StateTransitionServer::send_and_fill(uint32_t code, Mode::Response::SharedPtr res) {
bool ok = udp_client_->send_command(code);
res->success = ok;
res->message = ok ? "sent" : "failed";
}




void StateTransitionServer::handle_set_mode(const std::shared_ptr<Mode::Request> request, std::shared_ptr<Mode::Response> response) {
  std::string mode = request->mode;

  using namespace std::chrono_literals;

  // <-- Logger added here to capture every mode request
  RCLCPP_INFO(this->get_logger(), "Mode service called with mode: '%s'", mode.c_str());


  if (mode == "stand_up") {
    response->message = "Change the mode to stand_up";
    std::thread([this]() {
        udp_client_->send_command(CMD_STEP_CTRL);
        std::this_thread::sleep_for(5s); 
        udp_client_->send_command(CMD_SIT_STAND); 
    }).detach();
  } else if (mode == "stand_down") {
    response->message = "Change the mode to stand_down";
    std::thread([this]() {
        udp_client_->send_command(CMD_STEP_CTRL);
        std::this_thread::sleep_for(5s); 
        udp_client_->send_command(CMD_SIT_STAND); 
    }).detach();
  } else if (mode == "stand_up_step") {
    std::thread([this]() {
        udp_client_->send_command(CMD_STEP_CTRL);
        std::this_thread::sleep_for(5s); 
        udp_client_->send_command(CMD_SIT_STAND); 
        std::this_thread::sleep_for(5s); 
        udp_client_->send_command(CMD_TORQUE_CTRL);
        std::this_thread::sleep_for(5s); 
        udp_client_->send_command(CMD_STEP_CTRL);

    }).detach();
  } else if (mode == "crawl") {
    response->message = "Change the mode to crawl";
    std::thread([this]() {
        udp_client_->send_command(CMD_CRWL_CTRL);
    }).detach();
    
  }
   else if (mode == "navigation_mode") {
    response->message = "Change the mode to navigation_mode";
    std::thread([this]() {
        udp_client_->send_command(SWITCH_VEL_SRC, 2);
    }).detach();
    
  }

  else if (mode == "joystick_mode") {
    response->message = "Change the mode to navigation_mode";
    std::thread([this]() {
        udp_client_->send_command(SWITCH_VEL_SRC, 1);
    }).detach();
    
  }

  else {
    // <-- Warning logger for invalid modes
    RCLCPP_WARN(this->get_logger(), "Invalid mode requested: '%s'", mode.c_str());
    response->success = false;
    response->message = "Invalid mode";
    return;
  }

  response->success = true;
}