#include "x30_state_controller/udp_client.hpp"
#include <iostream>
#include <cstring>

namespace x30_state_controller {

UDPClient::UDPClient(const std::string& host, uint16_t port)
    : socket_(io_context_, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)),
      active_(true) {
    
    // Resolve remote endpoint
    boost::asio::ip::udp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(boost::asio::ip::udp::v4(), host, std::to_string(port));
    remote_endpoint_ = *endpoints.begin();
    
    // Set socket timeout
    socket_.set_option(boost::asio::socket_base::receive_buffer_size(1024));
    
    // Start IO thread
    io_thread_ = std::thread(&UDPClient::ioThreadFunc, this);
    
    // Start receiving
    startReceive();
}

UDPClient::~UDPClient() {
    close();
}

void UDPClient::sendCommand(uint32_t code, uint32_t value, uint32_t type, 
                            ResponseCallback callback) {
    if (!active_) {
        std::cerr << "[UDPClient] Cannot send - client is closed" << std::endl;
        return;
    }

    // Pack command in little-endian format: <III (3 x uint32_t)
    std::array<uint8_t, 12> packet;
    std::memcpy(&packet[0], &code, 4);
    std::memcpy(&packet[4], &value, 4);
    std::memcpy(&packet[8], &type, 4);

    // Store callback
    if (callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_callback_ = callback;
    }

    // Send asynchronously
    socket_.async_send_to(
        boost::asio::buffer(packet),
        remote_endpoint_,
        [code, value, type](const boost::system::error_code& error, std::size_t /*bytes_sent*/) {
            if (!error) {
                std::cout << "[UDPClient] Sent: code=0x" << std::hex << code 
                          << ", value=" << std::dec << value 
                          << ", type=" << type << std::endl;
            } else {
                std::cerr << "[UDPClient] Send error: " << error.message() << std::endl;
            }
        }
    );
}

void UDPClient::close() {
    if (active_.exchange(false)) {
        io_context_.stop();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        boost::system::error_code ec;
        socket_.close(ec);
    }
}

bool UDPClient::isActive() const {
    return active_;
}

void UDPClient::startReceive() {
    socket_.async_receive_from(
        boost::asio::buffer(receive_buffer_),
        remote_endpoint_,
        [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
            handleReceive(error, bytes_transferred);
        }
    );
}

void UDPClient::handleReceive(const boost::system::error_code& error, 
                              std::size_t bytes_transferred) {
    if (!error && bytes_transferred >= 12) {
        // Unpack response: <III (3 x uint32_t)
        uint32_t code, value, type;
        std::memcpy(&code, &receive_buffer_[0], 4);
        std::memcpy(&value, &receive_buffer_[4], 4);
        std::memcpy(&type, &receive_buffer_[8], 4);

        std::cout << "[UDPClient] Received: code=0x" << std::hex << code 
                  << ", value=" << std::dec << value 
                  << ", type=" << type << std::endl;

        // Invoke callback if present
        ResponseCallback callback;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback = std::move(pending_callback_);
            pending_callback_ = nullptr;
        }
        
        if (callback) {
            callback(code, value, type);
        }
    }

    // Continue receiving if still active
    if (active_) {
        startReceive();
    }
}

void UDPClient::ioThreadFunc() {
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> 
        work_guard(io_context_.get_executor());
    
    try {
        io_context_.run();
    } catch (const std::exception& e) {
        std::cerr << "[UDPClient] IO thread exception: " << e.what() << std::endl;
    }
}

} // namespace x30_state_controller
