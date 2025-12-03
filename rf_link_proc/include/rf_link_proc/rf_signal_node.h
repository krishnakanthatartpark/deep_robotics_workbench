#ifndef RF_SIGNAL_NODE_H
#define RF_SIGNAL_NODE_H

#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int8.hpp>
#include "rf_signal_handler.h"     
#include "nl80211_client.h"
#include "rf_link_proc/msg/rf_info.hpp"


class RFSignalNode : public rclcpp::Node {
public:
  RFSignalNode();

private:
  void timerCallback();

  rclcpp::Publisher<rf_link_proc::msg::RFInfo>::SharedPtr rf_info_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  RFSignalHandler handler_;
  std::unique_ptr<NL80211Client> client_;
  double calculateDistance(float txPowerDbm, int8_t rssi, uint32_t freqMHz);
  double calculateFresnelRadius(double freqMHz, double distance, int zoneNumber);
  std::string interface_;
  
};

#endif  // RF_SIGNAL_PUBLISHER_NODE_H
