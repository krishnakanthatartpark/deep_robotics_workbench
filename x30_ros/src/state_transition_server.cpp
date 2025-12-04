#include "x30_ros/state_transition_server.hpp"
#include <chrono>
#include <thread>


static constexpr uint32_t CMD_SIT_STAND = 0x21010202;
static constexpr uint32_t CMD_TORQUE_CTRL = 0x2101020A;
static constexpr uint32_t CMD_STEP_CTRL = 0x21010201;
static constexpr uint32_t CMD_CRWL_CTRL = 0x21010406;


StateTransitionServer::StateTransitionServer() : Node("state_transition_server") {
std::string ip = this->declare_parameter("motion_ip", "192.168.1.103");
int port = this->declare_parameter("motion_port", 43893);
double timeout = this->declare_parameter("recv_timeout_sec", 0.5);


udp_client_ = std::make_unique<UdpClient>(ip, port, timeout);
udp_client_->set_logger(shared_from_this());


sit_stand_srv_ = create_service<Mode>("/stand_sequence",
std::bind(&StateTransitionServer::handle_sit_stand, this, _1, _2));


torque_srv_ = create_service<Mode>("/sit_sequence",
std::bind(&StateTransitionServer::handle_torque_ctrl, this, _1, _2));


step_srv_ = create_service<Mode>("/torque_ctrl",
std::bind(&StateTransitionServer::handle_step_ctrl, this, _1, _2));


crawl_srv_ = create_service<Mode>("/crawl_ctrl",
std::bind(&StateTransitionServer::handle_crawl_ctrl, this, _1, _2));


sequence_srv_ = create_service<Mode>("/stepping_sequence",
std::bind(&StateTransitionServer::handle_run_sequence, this, _1, _2));
}


void StateTransitionServer::send_and_fill(uint32_t code, Mode::Response::SharedPtr res) {
bool ok = udp_client_->send_command(code);
res->success = ok;
res->message = ok ? "sent" : "failed";
}


void StateTransitionServer::handle_sit_stand(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response> r) {
send_and_fill(CMD_SIT_STAND, r);
}


void StateTransitionServer::handle_torque_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response> r) {
send_and_fill(CMD_TORQUE_CTRL, r);
}


void StateTransitionServer::handle_step_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response> r) {
send_and_fill(CMD_STEP_CTRL, r);
}


void StateTransitionServer::handle_crawl_ctrl(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response> r) {
send_and_fill(CMD_CRWL_CTRL, r);
}


void StateTransitionServer::handle_run_sequence(const std::shared_ptr<Mode::Request>, std::shared_ptr<Mode::Response> r) {
std::thread([this]() {
using namespace std::chrono_literals;
udp_client_->send_command(CMD_SIT_STAND);
std::this_thread::sleep_for(5s);
udp_client_->send_command(CMD_STEP_CTRL);
std::this_thread::sleep_for(5s);
udp_client_->send_command(CMD_SIT_STAND);
std::this_thread::sleep_for(3s);
udp_client_->send_command(CMD_TORQUE_CTRL);
std::this_thread::sleep_for(5s);
udp_client_->send_command(CMD_STEP_CTRL);
std::this_thread::sleep_for(5s);
udp_client_->send_command(CMD_CRWL_CTRL);
}).detach();


r->success = true;
r->message = "sequence started";
}