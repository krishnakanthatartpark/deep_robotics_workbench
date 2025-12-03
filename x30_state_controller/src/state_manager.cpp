#include "x30_state_controller/state_manager.hpp"
#include <iostream>
#include <chrono>

namespace x30_state_controller {

StateManager::StateManager(std::shared_ptr<UDPClient> client)
    : client_(client),
      current_state_(RobotState::UNKNOWN),
      response_received_(false),
      last_response_code_(0),
      last_response_value_(0) {
    
    // Initialize command code mapping based on Python script
    command_codes_["sit"] = 0x21010202;         // CMD_SIT_STAND
    command_codes_["stand"] = 0x21010202;       // CMD_SIT_STAND (toggle)
    command_codes_["torque"] = 0x2101020A;      // CMD_TORQUE_CTRL
    command_codes_["step_start"] = 0x21010201;  // CMD_STEP_CTRL
    command_codes_["step_stop"] = 0x21010201;   // CMD_STEP_CTRL (toggle)
}

bool StateManager::executeTransition(const std::string& command,
                                     std::string& result_msg,
                                     uint32_t& response_code,
                                     uint32_t& response_value) {
    // Validate command (need to access command_codes_)
    uint32_t code;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = command_codes_.find(command);
        if (it == command_codes_.end()) {
            result_msg = "Unknown command: " + command;
            return false;
        }
        code = it->second;
    }
    
    // Reset response tracking (use separate mutex)
    {
        std::lock_guard<std::mutex> lock(response_mutex_);
        response_received_ = false;
        last_response_code_ = 0;
        last_response_value_ = 0;
    }

    // Define callback to capture response
    auto callback = [this](uint32_t code, uint32_t value, uint32_t /*type*/) {
        std::lock_guard<std::mutex> lock(response_mutex_);
        last_response_code_ = code;
        last_response_value_ = value;
        response_received_ = true;
        response_cv_.notify_one();
    };

    // Send command
    client_->sendCommand(code, 0, 0, callback);

    // Wait for response with timeout (use response_mutex_)
    std::unique_lock<std::mutex> wait_lock(response_mutex_);
    bool received = response_cv_.wait_for(
        wait_lock,
        std::chrono::milliseconds(500),
        [this] { return response_received_; }
    );

    if (!received) {
        result_msg = "Timeout waiting for robot response";
        return false;
    }

    // Update state based on command (use state_mutex_)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (command == "sit") {
            current_state_ = RobotState::SITTING;
        } else if (command == "stand") {
            current_state_ = RobotState::STANDING;
        } else if (command == "torque") {
            current_state_ = RobotState::TORQUE_CONTROLLED;
        } else if (command == "step_start") {
            current_state_ = RobotState::WALKING;
        } else if (command == "step_stop") {
            current_state_ = RobotState::STANDING;
        }
    }

    response_code = last_response_code_;
    response_value = last_response_value_;
    result_msg = "Transition to '" + command + "' successful. State: " + stateToString(current_state_);
    
    return true;
}

std::string StateManager::getCurrentState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return stateToString(current_state_);
}

StateManager::RobotState StateManager::stringToState(const std::string& state_str) const {
    if (state_str == "SITTING") return RobotState::SITTING;
    if (state_str == "STANDING") return RobotState::STANDING;
    if (state_str == "TORQUE_CONTROLLED") return RobotState::TORQUE_CONTROLLED;
    if (state_str == "WALKING") return RobotState::WALKING;
    return RobotState::UNKNOWN;
}

std::string StateManager::stateToString(RobotState state) const {
    switch (state) {
        case RobotState::SITTING: return "SITTING";
        case RobotState::STANDING: return "STANDING";
        case RobotState::TORQUE_CONTROLLED: return "TORQUE_CONTROLLED";
        case RobotState::WALKING: return "WALKING";
        default: return "UNKNOWN";
    }
}

} // namespace x30_state_controller
