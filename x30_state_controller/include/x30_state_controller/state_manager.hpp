#pragma once

#include "x30_state_controller/udp_client.hpp"
#include <map>
#include <string>
#include <memory>
#include <mutex>

namespace x30_state_controller {

/**
 * @brief High-level state machine for X30 robot state transitions
 * 
 * Maps string commands to UDP binary codes, validates transitions,
 * and tracks current robot state.
 */
class StateManager {
public:
    /**
     * @brief Construct state manager
     * @param client Shared pointer to UDP client
     */
    explicit StateManager(std::shared_ptr<UDPClient> client);

    /**
     * @brief Execute a state transition command
     * @param command String command (sit, stand, torque, step_start, step_stop)
     * @param result_msg Output message describing result
     * @param response_code Output response code from robot
     * @param response_value Output response value from robot
     * @return true if command succeeded
     */
    bool executeTransition(const std::string& command, 
                          std::string& result_msg,
                          uint32_t& response_code,
                          uint32_t& response_value);

    /**
     * @brief Get current robot state as string
     */
    std::string getCurrentState() const;

private:
    enum class RobotState {
        UNKNOWN,
        SITTING,
        STANDING,
        TORQUE_CONTROLLED,
        WALKING
    };

    RobotState stringToState(const std::string& state_str) const;
    std::string stateToString(RobotState state) const;
    
    std::shared_ptr<UDPClient> client_;
    RobotState current_state_;
    std::map<std::string, uint32_t> command_codes_;
    mutable std::mutex state_mutex_;
    
    // Response tracking (separate mutex to avoid deadlock)
    std::mutex response_mutex_;
    std::condition_variable response_cv_;
    bool response_received_;
    uint32_t last_response_code_;
    uint32_t last_response_value_;
};

} // namespace x30_state_controller
