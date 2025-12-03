#include "streaming_server_node.h"

StreamingServerNode::StreamingServerNode()
    : Node("streaming_server_node")
{
    start_service_ = this->create_service<streaming::srv::StartStream>(
        "start_stream",
        std::bind(&StreamingServerNode::handleStartStream, this, std::placeholders::_1, std::placeholders::_2)
    );

    stop_service_ = this->create_service<streaming::srv::StopStream>(
        "stop_stream",
        std::bind(&StreamingServerNode::handleStopStream, this, std::placeholders::_1, std::placeholders::_2)
    );

    list_service_ = this->create_service<streaming::srv::ListStreams>(
        "list_streams",
        std::bind(&StreamingServerNode::handleListStreams, this, std::placeholders::_1, std::placeholders::_2)
    );

    RCLCPP_INFO(this->get_logger(), "Streaming Server Node ready.");
}

void StreamingServerNode::handleStartStream(
    const std::shared_ptr<streaming::srv::StartStream::Request> request,
    std::shared_ptr<streaming::srv::StartStream::Response> response)
{
    StreamConfig config;
    config.stream_name = request->stream_name;
    config.srt_uri = request->srt_uri;
    config.source = request->source;
    config.source_type = request->source_type;
    config.width = request->width;
    config.height = request->height;
    config.framerate = request->framerate;
    config.node = shared_from_this();

    bool success = manager_.startStream(config);
    response->success = success;
    response->message = success ? "Stream started successfully." : "Failed to start stream.";
}

void StreamingServerNode::handleStopStream(
    const std::shared_ptr<streaming::srv::StopStream::Request> request,
    std::shared_ptr<streaming::srv::StopStream::Response> response)
{
    bool success = manager_.stopStream(request->stream_name);
    response->success = success;
    response->message = success ? "Stream stopped successfully." : "Stream not found or failed to stop.";
}

void StreamingServerNode::handleListStreams(
    const std::shared_ptr<streaming::srv::ListStreams::Request> /*request*/,
    std::shared_ptr<streaming::srv::ListStreams::Response> response)
{
    auto streams = manager_.listStreams();
    response->stream_names = streams;
}
