#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class RosSrtStreamer : public rclcpp::Node {
public:
    RosSrtStreamer()
        : Node("ros_srt_streamer_node") {
        
        // Declare parameters and get their values
        this->declare_parameter<std::string>("image_topic", "/camera/color/image_raw");
        this->declare_parameter<std::string>("srt_uri", "srt://localhost:8890");
        this->declare_parameter<std::string>("stream_name", "test");

        this->get_parameter("image_topic", image_topic_);
        this->get_parameter("srt_uri", srt_uri_);
        this->get_parameter("stream_name", stream_name_);

        // Initialize GStreamer library
        gst_init(nullptr, nullptr);

        // Setup the GStreamer pipeline
        setupPipeline();

        // Create subscriber to the image topic
        sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            image_topic_, 10,
            std::bind(&RosSrtStreamer::imageCallback, this, std::placeholders::_1));
    }

    ~RosSrtStreamer() {
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
    }

private:
    void setupPipeline() {
        // Build GStreamer pipeline string with dynamic SRT URI and stream name
        std::string pipeline_str =
            "appsrc name=mysrc is-live=true format=time do-timestamp=true "
            "caps=video/x-raw,format=RGB,width=1280,height=800,framerate=15/1 "
            "! nvvidconv "
            "! video/x-raw(memory:NVMM),format=NV12 "
            "! nvv4l2h264enc insert-sps-pps=true iframeinterval=15 idrinterval=15 control-rate=1 bitrate=500000 preset-level=1 profile=0 num-B-Frames=0 "
            "! h264parse config-interval=-1 "
            "! mpegtsmux alignment=7 "
            "! srtsink uri=\"srt://localhost:8890?streamid=%23!::m=publish,r=test1\"";

        GError* err = nullptr;
        pipeline_ = gst_parse_launch(pipeline_str.c_str(), &err);
        if (err) {
            RCLCPP_FATAL(this->get_logger(), "GStreamer error: %s", err->message);
            g_error_free(err);
            throw std::runtime_error("Failed to create GStreamer pipeline");
        }

        // Get appsrc element to push buffers later
        appsrc_ = gst_bin_get_by_name(GST_BIN(pipeline_), "mysrc");
        if (!appsrc_) {
            RCLCPP_FATAL(this->get_logger(), "Failed to get appsrc element from pipeline");
            throw std::runtime_error("Failed to get appsrc element");
        }

        // Set pipeline to playing
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            // Convert ROS Image message to OpenCV RGB image
            cv::Mat img = cv_bridge::toCvCopy(msg, "rgb8")->image;
            if (img.empty()) return;

            GstBuffer* buffer;
            GstFlowReturn ret;
            int size = static_cast<int>(img.total() * img.elemSize());

            // Allocate GstBuffer and copy image data
            buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
            GstMapInfo map;
            gst_buffer_map(buffer, &map, GST_MAP_WRITE);
            std::memcpy(map.data, img.data, size);
            gst_buffer_unmap(buffer, &map);

            // Set PTS and DTS from ROS message timestamp (nanoseconds resolution)
            GST_BUFFER_PTS(buffer) = (guint64)msg->header.stamp.sec * GST_SECOND +
                                    (guint64)msg->header.stamp.nanosec;
            GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);

            // Duration for one frame at 15 fps
            GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);

            // Push buffer to the pipeline
            g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
            gst_buffer_unref(buffer);

            if (ret != GST_FLOW_OK) {
                RCLCPP_WARN(this->get_logger(), "GStreamer push-buffer returned: %d", ret);
            }
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge error: %s", e.what());
        }
    }

    std::string image_topic_;
    std::string srt_uri_;
    std::string stream_name_;

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;

    GstElement* pipeline_ = nullptr;
    GstElement* appsrc_ = nullptr;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<RosSrtStreamer>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
