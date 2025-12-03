#pragma once

#include <functional>
#include <string>
#include <memory>
#include <boost/asio.hpp>

namespace x30_state_controller {

/**
 * @brief Low-level UDP transport for X30 robot communication
 * 
 * Provides asynchronous UDP send/receive with callback-based response handling.
 * Thread-safe implementation using boost::asio.
 */
class UDPClient {
public:
    /**
     * @brief Callback type for UDP responses
     * @param code Response command code
     * @param value Response value
     * @param type Response type
     */
    using ResponseCallback = std::function<void(uint32_t code, uint32_t value, uint32_t type)>;

    /**
     * @brief Construct UDP client
     * @param host Target IP address
     * @param port Target UDP port
     */
    UDPClient(const std::string& host, uint16_t port);
    
    /**
     * @brief Destructor - closes connection and stops IO
     */
    ~UDPClient();

    /**
     * @brief Send UDP command packet
     * @param code Command code (e.g., 0x21010202)
     * @param value Command value parameter
     * @param type Command type parameter
     * @param callback Function to call when response received (optional)
     */
    void sendCommand(uint32_t code, uint32_t value, uint32_t type, 
                     ResponseCallback callback = nullptr);

    /**
     * @brief Close UDP connection and stop IO thread
     */
    void close();

    /**
     * @brief Check if client is active
     */
    bool isActive() const;

private:
    void startReceive();
    void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);
    void ioThreadFunc();

    boost::asio::io_context io_context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remote_endpoint_;
    std::thread io_thread_;
    std::array<uint8_t, 1024> receive_buffer_;
    ResponseCallback pending_callback_;
    std::mutex callback_mutex_;
    std::atomic<bool> active_;
};

} // namespace x30_state_controller
