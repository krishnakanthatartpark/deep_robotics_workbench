#include <rclcpp/rclcpp.hpp>
#include "x30_state_controller/udp_client.hpp"
#include "x30_state_controller/state_manager.hpp"
#include "x30_state_controller/srv/state_transition.hpp"

namespace x30_state_controller {

class StateTransitionNode : public rclcpp::Node {
public:
    StateTransitionNode() : Node("x30_state_transition_node") {
        // Declare parameters
        this->declare_parameter("robot_ip", "192.168.1.103");
        this->declare_parameter("robot_port", 43893);

        // Get parameters
        std::string robot_ip = this->get_parameter("robot_ip").as_string();
        int robot_port = this->get_parameter("robot_port").as_int();

        RCLCPP_INFO(this->get_logger(), "Initializing X30 State Controller");
        RCLCPP_INFO(this->get_logger(), "  Robot IP: %s", robot_ip.c_str());
        RCLCPP_INFO(this->get_logger(), "  Robot Port: %d", robot_port);

        // Create UDP client and state manager
        udp_client_ = std::make_shared<UDPClient>(robot_ip, static_cast<uint16_t>(robot_port));
        state_manager_ = std::make_shared<StateManager>(udp_client_);

        // Create service server
        service_ = this->create_service<x30_state_controller::srv::StateTransition>(
            "state_transition",
            std::bind(&StateTransitionNode::handleServiceRequest, this,
                     std::placeholders::_1, std::placeholders::_2)
        );

        RCLCPP_INFO(this->get_logger(), "Service '/state_transition' ready");
    }

    ~StateTransitionNode() {
        if (udp_client_) {
            udp_client_->close();
        }
    }

private:
    void handleServiceRequest(
        const std::shared_ptr<x30_state_controller::srv::StateTransition::Request> request,
        std::shared_ptr<x30_state_controller::srv::StateTransition::Response> response) {
        
        RCLCPP_INFO(this->get_logger(), "Received command: '%s'", request->command.c_str());

        std::string result_msg;
        uint32_t resp_code = 0;
        uint32_t resp_value = 0;

        bool success = state_manager_->executeTransition(
            request->command, result_msg, resp_code, resp_value
        );

        response->success = success;
        response->message = result_msg;
        response->response_code = resp_code;
        response->response_value = resp_value;

        if (success) {
            RCLCPP_INFO(this->get_logger(), "✓ %s", result_msg.c_str());
        } else {
            RCLCPP_WARN(this->get_logger(), "✗ %s", result_msg.c_str());
        }
    }

    std::shared_ptr<UDPClient> udp_client_;
    std::shared_ptr<StateManager> state_manager_;
    rclcpp::Service<x30_state_controller::srv::StateTransition>::SharedPtr service_;
};

} // namespace x30_state_controller

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<x30_state_controller::StateTransitionNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
