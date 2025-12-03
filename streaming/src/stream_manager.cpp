#include "stream_manager.h"
#include <rclcpp/rclcpp.hpp>

bool StreamManager::startStream(const StreamConfig& config) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    if (active_streams_.count(config.stream_name)) {
        RCLCPP_WARN(config.node->get_logger(),
                    "Stream '%s' is already running.", config.stream_name.c_str());
        return false;
    }

    auto session = std::make_shared<StreamSession>(config.node);
    try {
        session->start(config.stream_name,
                       config.srt_uri,
                       config.source,
                       config.source_type,
                       config.width,
                       config.height,
                       config.framerate);
    } catch (const std::exception& e) {
        RCLCPP_ERROR(config.node->get_logger(),
                     "Failed to start stream '%s': %s", config.stream_name.c_str(), e.what());
        return false;
    }

    active_streams_[config.stream_name] = session;
    RCLCPP_INFO(config.node->get_logger(), "Stream '%s' started.", config.stream_name.c_str());
    return true;
}

bool StreamManager::stopStream(const std::string& stream_name) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    auto it = active_streams_.find(stream_name);
    if (it == active_streams_.end()) {
        RCLCPP_WARN(rclcpp::get_logger("StreamManager"),
                    "Stream '%s' not found.", stream_name.c_str());
        return false;
    }

    it->second->stop();
    active_streams_.erase(it);
    RCLCPP_INFO(rclcpp::get_logger("StreamManager"),
                "Stream '%s' stopped.", stream_name.c_str());
    return true;
}

std::vector<std::string> StreamManager::listStreams() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    std::vector<std::string> names;
    for (const auto& pair : active_streams_) {
        names.push_back(pair.first);
    }
    return names;
}
