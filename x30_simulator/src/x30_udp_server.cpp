#include <rclcpp/rclcpp.hpp>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class X30SimServerNode : public rclcpp::Node
{
public:
    X30SimServerNode() : Node("x30_sim_server")
    {
        RCLCPP_INFO(this->get_logger(), "X30 Simulator UDP Server Started!");

        // Start UDP server thread
        server_thread_ = std::thread([this]() { run_udp_server(); });
        server_thread_.detach();
    }

private:
    std::thread server_thread_;

    void run_udp_server()
    {
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), 43893));

        RCLCPP_INFO(this->get_logger(),
            "Dummy UDP server listening on port 43893...");

        char buffer[1024];
        udp::endpoint sender_endpoint;

        while (rclcpp::ok())
        {
            size_t len = socket.receive_from(boost::asio::buffer(buffer),
                                             sender_endpoint);

            if (len >= 4)
            {
                uint32_t cmd = 0;
                memcpy(&cmd, buffer, sizeof(cmd));

                RCLCPP_INFO(this->get_logger(),
                    "Received CMD: 0x%08X", cmd);

                // Send dummy ACK
                uint32_t ack = 0xAABBCCDD;
                socket.send_to(boost::asio::buffer(&ack, sizeof(ack)),
                               sender_endpoint);
            }
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<X30SimServerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
