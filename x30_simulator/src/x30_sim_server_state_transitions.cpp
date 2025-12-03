#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <ctime>
#include <boost/asio.hpp>
#include <tinyxml2.h>

using boost::asio::ip::tcp;

class X30SimServer
{
public:
  X30SimServer(boost::asio::io_context &io_context, int port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    std::cout << "[X30SimServer] Listening on port " << port << "..." << std::endl;
    acceptConnection();
  }

private:
  tcp::acceptor acceptor_;
  tcp::socket socket_{acceptor_.get_executor()};

  void acceptConnection()
  {
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
                           {
      if (!ec) {
        std::cout << "[X30SimServer] Client connected.\n";
        startRead();
      } else {
        std::cerr << "[Error] Accept failed: " << ec.message() << std::endl;
      } });
  }

  void startRead()
  {
    auto buffer = std::make_shared<std::vector<uint8_t>>(2048);
    socket_.async_read_some(
        boost::asio::buffer(*buffer),
        [this, buffer](boost::system::error_code ec, std::size_t bytes_transferred)
        {
          if (!ec)
          {
            buffer->resize(bytes_transferred);
            onReceive(*buffer);
            startRead(); // continue reading
          }
          else
          {
            std::cerr << "[Error] Read failed: " << ec.message() << std::endl;
          }
        });
  }

  void onReceive(const std::vector<uint8_t> &buffer)
  {
    if (buffer.size() < 16)
    {
      std::cerr << "[Error] Incomplete APDU header received.\n";
      return;
    }

    // Validate header sync
    if (!(buffer[0] == 0xEB && buffer[1] == 0x90 && buffer[2] == 0xEB && buffer[3] == 0x90))
    {
      std::cerr << "[Error] Invalid APDU header.\n";
      return;
    }

    uint16_t asdu_len = buffer[4] | (buffer[5] << 8);
    uint16_t msg_id = buffer[6] | (buffer[7] << 8);

    if (buffer.size() < 16 + asdu_len)
    {
      std::cerr << "[Error] Incomplete ASDU received (" << buffer.size() << " < " << 16 + asdu_len << ")\n";
      return;
    }

    std::string xml_str(reinterpret_cast<const char *>(&buffer[16]), asdu_len);

    std::cout << "\n[X30SimServer] Received XML:\n" << xml_str << std::endl;

    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml_str.c_str()) != tinyxml2::XML_SUCCESS)
    {
      std::cerr << "[Error] Invalid XML.\n";
      return;
    }

    auto *root = doc.FirstChildElement("PatrolDevice");
    if (!root)
    {
      std::cerr << "[Error] Missing PatrolDevice root element.\n";
      return;
    }

    int type = root->FirstChildElement("Type")->IntText();
    int cmd = root->FirstChildElement("Command")->IntText();

    std::string status_msg;
    switch (cmd)
    {
    case 15:
      status_msg = "SIT_OK";
      break;
    case 16:
      status_msg = "STAND_OK";
      break;
    case 18:
      status_msg = "STEP_OK";
      break;
    default:
      status_msg = "UNKNOWN_COMMAND";
      break;
    }

    std::cout << "[X30SimServer] Type=" << type
              << " Command=" << cmd
              << " → Reply: " << status_msg << std::endl;

    // Construct reply XML
    std::ostringstream reply_xml;
    reply_xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              << "<PatrolDevice>\n"
              << "  <Type>" << type << "</Type>\n"
              << "  <Command>" << cmd << "</Command>\n"
              << "  <Time>" << getCurrentTime() << "</Time>\n"
              << "  <Items>\n"
              << "    <Status>" << status_msg << "</Status>\n"
              << "  </Items>\n"
              << "</PatrolDevice>";

    sendApdu(reply_xml.str(), msg_id);
  }

  void sendApdu(const std::string &xml, uint16_t msg_id)
  {
    uint16_t len = static_cast<uint16_t>(xml.size());
    std::vector<uint8_t> apdu;

    // Header (16 bytes)
    apdu.push_back(0xEB);
    apdu.push_back(0x90);
    apdu.push_back(0xEB);
    apdu.push_back(0x90);

    // ASDU length
    apdu.push_back(len & 0xFF);
    apdu.push_back((len >> 8) & 0xFF);

    // Message ID
    apdu.push_back(msg_id & 0xFF);
    apdu.push_back((msg_id >> 8) & 0xFF);

    // Reserved 8 bytes
    for (int i = 0; i < 8; i++)
      apdu.push_back(0x00);

    // Append XML data
    apdu.insert(apdu.end(), xml.begin(), xml.end());

    // Send
    boost::asio::async_write(socket_, boost::asio::buffer(apdu),
                             [](boost::system::error_code ec, std::size_t bytes_transferred)
                             {
                               if (ec)
                                 std::cerr << "[Error] Failed to send reply: " << ec.message() << std::endl;
                               else
                                 std::cout << "[X30SimServer] Sent reply (" << bytes_transferred << " bytes)\n";
                             });
  }

  std::string getCurrentTime()
  {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
  }
};

int main(int argc, char **argv)
{
  try
  {
    boost::asio::io_context io;
    int port = 30000; // default port
    if (argc > 1)
      port = std::stoi(argv[1]);
    X30SimServer server(io, port);
    io.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "[Fatal] Exception: " << e.what() << std::endl;
  }
  return 0;
}
