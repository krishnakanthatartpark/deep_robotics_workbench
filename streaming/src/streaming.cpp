#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include "rf_link_proc/msg/rf_info.hpp"
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class RosSrtStreamer : public rclcpp::Node {
public:
    RosSrtStreamer()
        : Node("ros_srt_streamer_node"),
          frame_counter_(0),
          frame_duration_(GST_SECOND / 30) // 30 FPS
    {
        this->declare_parameter<std::string>("image_topic", "camera/color/image_raw");
        this->declare_parameter<std::string>("srt_uri", "srt://localhost:8890");
        this->declare_parameter<std::string>("stream_name", "hardwareid_payload");
        this->declare_parameter<int>("width", 1280);
        this->declare_parameter<int>("height", 720);
        this->declare_parameter<int>("framerate", 30);

        this->get_parameter("image_topic", image_topic_);
        this->get_parameter("srt_uri", srt_uri_);
        this->get_parameter("stream_name", stream_name_);
        this->get_parameter("width", width_);
        this->get_parameter("height", height_);
        this->get_parameter("framerate", framerate_);

        gst_init(nullptr, nullptr);
        setupPipeline();

        callback_group_img_ = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
        callback_group_rf_ = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
        
        rclcpp::SubscriptionOptions img_opts;
        img_opts.callback_group = callback_group_img_;
        image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_topic_, 10,
            std::bind(&RosSrtStreamer::imageCallback, this, std::placeholders::_1), img_opts);
        
        rclcpp::SubscriptionOptions rf_opts;
        rf_opts.callback_group = callback_group_rf_;
        rf_sub_ = this->create_subscription<rf_link_proc::msg::RFInfo>(
            "/rf_info", 10, std::bind(&RosSrtStreamer::rfCallback, this, std::placeholders::_1), rf_opts);
    }

    ~RosSrtStreamer() {
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
        }
    }

private:
    void setupPipeline() {
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
            "! nvvidconv "
            "! " + nvmm_caps_str + " "
            "! queue max-size-buffers=1 leaky=downstream "
            "! nvv4l2h264enc preset-level=1 insert-sps-pps=true insert-vui=true idrinterval=15 iframeinterval=15 control-rate=1 bitrate=2000000 EnableTwopassCBR=false "
            "! h264parse config-interval=1 "
            "! mpegtsmux alignment=7 "
            "! srtsink uri=" + srt_uri_ + " streamid=#!::m=publish,r=" + stream_name_;


        GError* err = nullptr;
        pipeline_ = gst_parse_launch(pipeline_str.c_str(), &err);
        if (err) {
            RCLCPP_FATAL(this->get_logger(), "GStreamer error: %s", err->message);
            g_error_free(err);
            throw std::runtime_error("Failed to create GStreamer pipeline");
        }

        appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysrc");
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            cv::Mat img = cv_bridge::toCvCopy(msg, "rgb8")->image;
            if (img.empty()) return;
            cv::Scalar color;
            if (rssi_ > -50) color = cv::Scalar(0, 255, 0);      // green
            else if (rssi_ > -65) color = cv::Scalar(0, 165, 255); // orange
            else color = cv::Scalar(0, 0, 255);                  // red

            cv::putText(img, std::to_string(rssi_), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX,
                        1.0, color, 2);

            int cx = img.cols / 2;
            int cy = img.rows / 2;
            int length = 20;  
            cv::Scalar cross_color(0, 255, 0); 
            int thickness = 2;

            cv::line(img, cv::Point(cx - length, cy), cv::Point(cx + length, cy), cross_color, thickness);
            cv::line(img, cv::Point(cx, cy - length), cv::Point(cx, cy + length), cross_color, thickness);



            int size = img.total() * img.elemSize();
            GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);

            GstMapInfo map;
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            std::memcpy(map.data, img.data, size);
            gst_buffer_unmap(buffer, &map);

            // Assign strictly increasing PTS/DTS
            guint64 pts = frame_counter_ * frame_duration_;
            GST_BUFFER_PTS(buffer) = pts;
            GST_BUFFER_DTS(buffer) = pts;
            GST_BUFFER_DURATION(buffer) = frame_duration_;
            frame_counter_++;

            GstFlowReturn ret;
            g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
            gst_buffer_unref(buffer);

            if (ret != GST_FLOW_OK) {
                RCLCPP_WARN(this->get_logger(), "GStreamer push-buffer returned: %d", ret);
            }

        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge error: %s", e.what());
        }
    }

    void rfCallback(const rf_link_proc::msg::RFInfo::SharedPtr msg){
        rssi_ = static_cast<int>(msg->signal_strength);

    }

    std::string image_topic_, srt_uri_, stream_name_;
    int width_, height_, framerate_;

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::Subscription<rf_link_proc::msg::RFInfo>::SharedPtr rf_sub_;
    
    rclcpp::CallbackGroup::SharedPtr callback_group_img_;
    rclcpp::CallbackGroup::SharedPtr callback_group_rf_;

    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;

    guint64 frame_counter_;
    guint64 frame_duration_;

    int rssi_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RosSrtStreamer>();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    return 0;
}
