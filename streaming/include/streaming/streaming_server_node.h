#ifndef STREAMING_SERVER_NODE_H
#define STREAMING_SERVER_NODE_H

#include "stream_manager.h"
#include "streaming/srv/start_stream.hpp"
#include "streaming/srv/stop_stream.hpp"
#include "streaming/srv/list_streams.hpp"

#include <rclcpp/rclcpp.hpp>

class StreamingServerNode : public rclcpp::Node {
public:
    StreamingServerNode();

private:
    void handleStartStream(
        const std::shared_ptr<streaming::srv::StartStream::Request> request,
        std::shared_ptr<streaming::srv::StartStream::Response> response);

    void handleStopStream(
        const std::shared_ptr<streaming::srv::StopStream::Request> request,
        std::shared_ptr<streaming::srv::StopStream::Response> response);

    void handleListStreams(
        const std::shared_ptr<streaming::srv::ListStreams::Request> request,
        std::shared_ptr<streaming::srv::ListStreams::Response> response);

    rclcpp::Service<streaming::srv::StartStream>::SharedPtr start_service_;
    rclcpp::Service<streaming::srv::StopStream>::SharedPtr stop_service_;
    rclcpp::Service<streaming::srv::ListStreams>::SharedPtr list_service_;

    StreamManager manager_;
};

#endif  // STREAMING_SERVER_NODE_HPP
