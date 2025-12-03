#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

enum class NavStatus { IDLE, EXECUTING, COMPLETED, FAILED };

struct RobotState {
    double posX = 0.0;
    double posY = 0.0;
    double posZ = 0.0;
    double yaw = 0.0;
    double speed = 0.0;
    int battery = 95;
    NavStatus navStatus = NavStatus::IDLE;
    int targetPoint = 0;
};

void sendMessage(tcp::socket& socket, const std::string& msg) {
    boost::asio::write(socket, boost::asio::buffer(msg));
}

void sendJson(tcp::socket& socket, const json& j) {
    std::string data = j.dump();
    boost::asio::write(socket, boost::asio::buffer(data));
}

int main() {
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 30000));

        std::cout << "[DummyServer] Listening on port 30000..." << std::endl;

        tcp::socket socket(io);
        acceptor.accept(socket);
        std::cout << "[DummyServer] Client connected!" << std::endl;

        RobotState robot;
        std::mutex stateMutex;
        std::atomic<bool> running{true};

        // Background thread to simulate movement while executing navigation
        std::thread motionThread([&]() {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::lock_guard<std::mutex> lock(stateMutex);

                if (robot.navStatus == NavStatus::EXECUTING) {
                    robot.posX += 0.1;
                    robot.posY += 0.1;
                    robot.speed = 0.5;

                    // After 10 updates, complete task
                    if (robot.posX > 1.0) {
                        robot.navStatus = NavStatus::COMPLETED;
                        robot.speed = 0.0;
                        std::cout << "[DummyServer] Navigation completed." << std::endl;
                    }
                }
            }
        });

        char buf[1024];
        while (running) {
            std::memset(buf, 0, sizeof(buf));
            boost::system::error_code ec;
            std::size_t len = socket.read_some(boost::asio::buffer(buf), ec);

            if (ec == boost::asio::error::eof || len == 0) {
                std::cout << "[DummyServer] Client disconnected." << std::endl;
                break;
            }

            std::string msg(buf, len);
            std::cout << "[DummyServer] Received: " << msg << std::endl;

            json response;

            // Identify message by keywords (simplified)
            if (msg.find("1002") != std::string::npos) {
                // Runtime state
                std::lock_guard<std::mutex> lock(stateMutex);
                response = {
                    {"posX", robot.posX},
                    {"posY", robot.posY},
                    {"posZ", robot.posZ},
                    {"angleYaw", robot.yaw},
                    {"speed", robot.speed},
                    {"electricity", robot.battery},
                    {"motionState", (robot.navStatus == NavStatus::EXECUTING ? "Walking" : "Idle")}
                };
                sendJson(socket, response);
            }
            else if (msg.find("1003") != std::string::npos) {
                // Start navigation
                std::lock_guard<std::mutex> lock(stateMutex);
                robot.navStatus = NavStatus::EXECUTING;
                robot.posX = 0.0;
                robot.posY = 0.0;
                robot.targetPoint = 1;
                response = {
                    {"value", robot.targetPoint},
                    {"errorCode", 0},
                    {"errorStatus", 0}
                };
                sendJson(socket, response);
                std::cout << "[DummyServer] Navigation started." << std::endl;
            }
            else if (msg.find("1007") != std::string::npos) {
                // Query task state
                std::lock_guard<std::mutex> lock(stateMutex);
                std::string statusStr = "IDLE";
                switch (robot.navStatus) {
                    case NavStatus::EXECUTING: statusStr = "EXECUTING"; break;
                    case NavStatus::COMPLETED: statusStr = "COMPLETED"; break;
                    case NavStatus::FAILED: statusStr = "FAILED"; break;
                    default: break;
                }
                response = {
                    {"value", robot.targetPoint},
                    {"status", statusStr},
                    {"errorCode", 0}
                };
                sendJson(socket, response);
            }
            else if (msg.find("1004") != std::string::npos) {
                // Cancel navigation
                std::lock_guard<std::mutex> lock(stateMutex);
                robot.navStatus = NavStatus::IDLE;
                response = {{"success", true}};
                sendJson(socket, response);
                std::cout << "[DummyServer] Navigation canceled." << std::endl;
            }
            else {
                response = {{"error", "Unknown command"}};
                sendJson(socket, response);
            }
        }

        running = false;
        motionThread.join();
        socket.close();
        std::cout << "[DummyServer] Shutting down." << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Server exception: " << e.what() << std::endl;
    }

    return 0;
}