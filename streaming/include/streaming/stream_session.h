#ifndef STREAM_SESSION_H
#define STREAM_SESSION_H

#include <string>
#include <memory>
#include <atomic>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <opencv2/opencv.hpp>

#include <gst/gst.h>

class StreamSession {
public:
    explicit StreamSession(rclcpp::Node::SharedPtr node);

    void start(const std::string& stream_name,
               const std::string& srt_uri,
               const std::string& source,
               const std::string& source_type,
               int width,
               int height,
               int framerate);

    void stop();
    bool isRunning();

    // Can be used externally to push SDK-generated frames
    void sdkCallback(cv::Mat image);

private:
    void setupPipeline();
    void rosCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    void pushFrameToGStreamer(const cv::Mat& image);

    // ROS
    rclcpp::Node::SharedPtr node_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::CallbackGroup::SharedPtr callback_group_img_;

    // Stream configuration
    std::string stream_name_;
    std::string srt_uri_;
    std::string source_;
    std::string source_type_;
    int width_;
    int height_;
    int framerate_;

    // GStreamer
    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;
    guint64 frame_counter_;
    guint64 frame_duration_;

    std::atomic<bool> running_;
    std::mutex mutex_;
};

#endif // STREAM_SESSION_HPP
