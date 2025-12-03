#include "streaming_server_node.h"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<StreamingServerNode>();

    // Use multi-threaded executor
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}

