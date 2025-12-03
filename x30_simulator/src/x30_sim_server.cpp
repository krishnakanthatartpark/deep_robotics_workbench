#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

// Protocol Header Definition
#pragma pack(push, 1)
struct ProtocolHeader {
    uint8_t sync_byte1 = 0xeb;
    uint8_t sync_byte2 = 0x90;
    uint8_t sync_byte3 = 0xeb;
    uint8_t sync_byte4 = 0x90;
    uint16_t length = 0;
    uint16_t sequenceNumber = 0;
    std::array<uint8_t, 8> reserved = {0};
};
#pragma pack(pop)

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
    int motionCommand = 0; // Last received motion command
};

void sendMessage(tcp::socket& socket, const std::string& msg) {
    ProtocolHeader header;
    header.length = static_cast<uint16_t>(msg.size());
    // Note: Assuming little-endian system (standard x86/ARM)
    
    std::vector<uint8_t> buffer;
    buffer.resize(sizeof(ProtocolHeader) + msg.size());
    
    std::memcpy(buffer.data(), &header, sizeof(ProtocolHeader));
    std::memcpy(buffer.data() + sizeof(ProtocolHeader), msg.data(), msg.size());

    boost::asio::write(socket, boost::asio::buffer(buffer));
}

void sendJson(tcp::socket& socket, const json& j) {
    std::string data = j.dump();
    sendMessage(socket, data);
}

// Helper to extract value from XML tag
std::string getXmlValue(const std::string& xml, const std::string& tag) {
    std::string startTag = "<" + tag + ">";
    std::string endTag = "</" + tag + ">";
    size_t startPos = xml.find(startTag);
    if (startPos == std::string::npos) return "";
    startPos += startTag.length();
    size_t endPos = xml.find(endTag, startPos);
    if (endPos == std::string::npos) return "";
    return xml.substr(startPos, endPos - startPos);
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

        std::vector<uint8_t> receive_buffer(4096);
        while (running) {
            boost::system::error_code ec;
            std::size_t len = socket.read_some(boost::asio::buffer(receive_buffer), ec);

            if (ec == boost::asio::error::eof || len == 0) {
                std::cout << "[DummyServer] Client disconnected." << std::endl;
                break;
            }

            // Simple parsing: skip header if present (16 bytes) and look for XML
            // In a real server we would parse the header, but here we just scan for content
            std::string msg(reinterpret_cast<char*>(receive_buffer.data()), len);
            
            // If message starts with sync bytes, skip header
            size_t xmlStart = 0;
            if (len > sizeof(ProtocolHeader)) {
                // Check for sync bytes 0xeb 0x90 0xeb 0x90
                if (static_cast<uint8_t>(msg[0]) == 0xeb && 
                    static_cast<uint8_t>(msg[1]) == 0x90) {
                    xmlStart = sizeof(ProtocolHeader);
                }
            }
            
            std::string payload = msg.substr(xmlStart);
            // std::cout << "[DummyServer] Received payload: " << payload << std::endl;

            json response;

            // Identify message by keywords
            if (payload.find("1002") != std::string::npos) {
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
            else if (payload.find("<Type>2</Type>") != std::string::npos) {
                // Motion Control (Type 2)
                std::lock_guard<std::mutex> lock(stateMutex);
                
                std::string cmdStr = getXmlValue(payload, "Command");
                std::string valStr = getXmlValue(payload, "Value");
                
                int command = 0;
                float value = 0.0f;
                
                try {
                    if (!cmdStr.empty()) command = std::stoi(cmdStr);
                    if (!valStr.empty()) value = std::stof(valStr);
                } catch (...) {}

                robot.motionCommand = command;
                std::cout << "[DummyServer] Motion Command: " << command << ", Value: " << value << std::endl;

                // Handle specific state commands
                std::string actionName = "Unknown";
                switch (command) {
                    case 13: actionName = "Emergency Stop"; break;
                    case 14: actionName = "Stop Stepping"; break;
                    case 15: actionName = "Sit Down"; break;
                    case 16: actionName = "Stand Up"; break;
                    case 18: actionName = "Start Stepping"; break;
                    case 20: actionName = "Switch Gait"; break;
                    case 1: actionName = "Move Forward"; break;
                    case 2: actionName = "Move Backward"; break;
                    case 3: actionName = "Turn Left"; break;
                    case 4: actionName = "Turn Right"; break;
                    case 6: actionName = "Stop Move"; break;
                    case 11: actionName = "Move Left"; break;
                    case 12: actionName = "Move Right"; break;
                }
                
                if (command >= 13 && command <= 20) {
                     std::cout << "[DummyServer] Executing State Transition: " << actionName << std::endl;
                }

                // Construct XML response
                std::stringstream ss;
                ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                ss << "<PatrolDevice>\n";
                ss << "  <Type>2</Type>\n";
                ss << "  <Items>\n";
                ss << "    <Value>" << value << "</Value>\n";
                ss << "    <ErrorCode>0</ErrorCode>\n";
                ss << "  </Items>\n";
                ss << "</PatrolDevice>";
                
                sendMessage(socket, ss.str());
            }
            else if (payload.find("1003") != std::string::npos) {
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
            else if (payload.find("1007") != std::string::npos) {
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
            else if (payload.find("1004") != std::string::npos) {
                // Cancel navigation
                std::lock_guard<std::mutex> lock(stateMutex);
                robot.navStatus = NavStatus::IDLE;
                response = {{"success", true}};
                sendJson(socket, response);
                std::cout << "[DummyServer] Navigation canceled." << std::endl;
            }
            else {
                // Ignore unknown
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