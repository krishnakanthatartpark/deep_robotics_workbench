#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H

#include "stream_session.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct StreamConfig {
    std::string stream_name;
    std::string srt_uri;
    std::string source;
    std::string source_type;
    int width;
    int height;
    int framerate;
    rclcpp::Node::SharedPtr node;
};

class StreamManager {
public:
    bool startStream(const StreamConfig& config);
    bool stopStream(const std::string& stream_name);
    std::vector<std::string> listStreams();

private:
    std::map<std::string, std::shared_ptr<StreamSession>> active_streams_;
    std::mutex manager_mutex_;
};

#endif 
