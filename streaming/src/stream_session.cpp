#include "stream_session.h"
#include <cv_bridge/cv_bridge.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include <rclcpp/rclcpp.hpp>

StreamSession::StreamSession(rclcpp::Node::SharedPtr node)
: node_(node), running_(false), frame_counter_(0) {
    gst_init(nullptr, nullptr);
    callback_group_img_ = node_->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
}

void StreamSession::start(const std::string& stream_name,
                          const std::string& srt_uri,
                          const std::string& source,
                          const std::string& source_type,
                          int width, int height, int framerate) {
    if (running_) return;

    stream_name_ = stream_name;
    srt_uri_ = srt_uri;
    source_ = source;
    source_type_ = source_type;
    width_ = width;
    height_ = height;
    framerate_ = framerate;
    frame_duration_ = GST_SECOND / framerate_;

    setupPipeline();

    if (source_type_ == "ros_topic") {
        auto callback = std::bind(&StreamSession::rosCallback, this, std::placeholders::_1);
        rclcpp::SubscriptionOptions opts;
        opts.callback_group = callback_group_img_;
        image_sub_ = node_->create_subscription<sensor_msgs::msg::Image>(
            source_, 1, callback, opts);
    }

    running_ = true;
}

void StreamSession::stop() {
    if (!running_) return;
    running_ = false;
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    image_sub_.reset();
}

bool StreamSession::isRunning() {
    return running_;
}

void StreamSession::setupPipeline() {
    std::string caps_str = "video/x-raw,format=RGB,width=" + std::to_string(width_) +
                       ",height=" + std::to_string(height_) +
                       ",framerate=" + std::to_string(framerate_) + "/1";

    std::string nvmm_caps_str = "video/x-raw(memory:NVMM),format=NV12,width=" +
                                std::to_string(width_) + ",height=" + std::to_string(height_) +
                                ",framerate=" + std::to_string(framerate_) + "/1";

    std::string pipeline_str =
            "appsrc name=mysrc is-live=true format=time do-timestamp=true "
            "caps=" + caps_str + " "
            "! videoconvert "
            "! video/x-raw,format=I420,width=" + std::to_string(width_) +
            ",height=" + std::to_string(height_) +
            ",framerate=" + std::to_string(framerate_) + "/1 "
            "! queue max-size-buffers=1 leaky=downstream "
            "! x264enc speed-preset=ultrafast byte-stream=true insert-vui=true key-int-max=15 bitrate=2000000 "
            "! h264parse config-interval=1 "
            "! mpegtsmux alignment=7 "
            "! srtsink uri=" + srt_uri_ + " streamid=#!::m=publish,r=" + stream_name_;



    GError* err = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &err);
    if (err) {
        RCLCPP_FATAL(node_->get_logger(), "GStreamer pipeline error: %s", err->message);
        g_error_free(err);
        throw std::runtime_error("Failed to create GStreamer pipeline");
    }

    appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysrc");
    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

void StreamSession::rosCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
    try {
        cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(msg, msg->encoding);

        cv::Mat converted_image;

        if (msg->encoding == "mono8") {
            cv::cvtColor(cv_ptr->image, converted_image, cv::COLOR_GRAY2BGR);
        } else if (msg->encoding == "rgb8") {
            converted_image = cv_ptr->image;
        } else if (msg->encoding == "bgr8") {
            converted_image = cv_ptr->image;
            cv::cvtColor(cv_ptr->image, converted_image, cv::COLOR_BGR2RGB);
        } else {
            RCLCPP_WARN(node_->get_logger(),
                "Unsupported encoding: %s. Skipping frame.", msg->encoding.c_str());
            return;
        }

        if (!converted_image.empty()) {
            pushFrameToGStreamer(converted_image);
            RCLCPP_INFO(node_->get_logger(), "Pushed frame to GStreamer. Size: %dx%d", converted_image.cols, converted_image.rows);
        }

    } catch (const cv_bridge::Exception& e) {
        RCLCPP_ERROR(node_->get_logger(), "cv_bridge exception: %s", e.what());
    }
}


void StreamSession::sdkCallback(cv::Mat image) {
    if (!image.empty()) {
        pushFrameToGStreamer(image);
    }
}

void StreamSession::pushFrameToGStreamer(const cv::Mat& image) {
    int size = image.total() * image.elemSize();
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    std::memcpy(map.data, image.data, size);
    gst_buffer_unmap(buffer, &map);

    guint64 pts = frame_counter_ * frame_duration_;
    GST_BUFFER_PTS(buffer) = pts;
    GST_BUFFER_DTS(buffer) = pts;
    GST_BUFFER_DURATION(buffer) = frame_duration_;
    frame_counter_++;

    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        RCLCPP_WARN(node_->get_logger(), "GStreamer push-buffer failed: %d", ret);
    }
}
