#include <cmath>
#include "rf_signal_node.h"

RFSignalNode::RFSignalNode()
    : rclcpp::Node("rf_signal_publisher_node") {

  // Declare and get the interface parameter
  this->declare_parameter<std::string>("interface", "wlxdc62796646c7");
  this->get_parameter("interface", interface_);

  rf_info_pub_ = this->create_publisher<rf_link_proc::msg::RFInfo>("rf_info", 10);

  client_ = std::make_unique<NL80211Client>(interface_);

  timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&RFSignalNode::timerCallback, this));
}


double RFSignalNode::calculateDistance(float txPowerDbm, int8_t rssi, uint32_t freqMHz) {
  // FSPL formula rearranged for distance:
  // distance (m) = 10 ^ ((txPower - rssi - 20*log10(freq) + 27.55) / 20)
  double freqMHzDouble = static_cast<double>(freqMHz);
  double distance = std::pow(10.0, (txPowerDbm - static_cast<double>(rssi) - 20.0 * std::log10(freqMHzDouble) + 27.55) / 20.0);
  return distance;
}

double RFSignalNode::calculateFresnelRadius(double freqMHz, double distance, int zoneNumber) {
  // Fresnel radius at midpoint: r = sqrt(nλd1*d2 / (d1 + d2))
  const double c = 3e8; // Speed of light in m/s
  double freqHz = freqMHz * 1e6;
  double lambda = c / freqHz;
  double d1 = distance / 2.0;
  double d2 = distance / 2.0;
  return std::sqrt(zoneNumber * lambda * d1 * d2 / (d1 + d2));
}

void RFSignalNode::timerCallback() {
  if (client_ && client_->queryAPSignal(handler_)) {
    int8_t signal = handler_.getSignalStrength();
    uint32_t freqMHz = handler_.getFrequencyMHz();
    float txPowerDbm = handler_.getTxPowerDbm();
    std::cout << freqMHz << txPowerDbm << std::endl;
    if (freqMHz == 0 || txPowerDbm == 0.0f) {
      RCLCPP_WARN(this->get_logger(), "Frequency or Tx Power not available");
      return;
    }

    double distance = calculateDistance(txPowerDbm, signal, freqMHz);
    double fresnel_radius = calculateFresnelRadius(freqMHz, distance, 1);  // First Fresnel zone

    RCLCPP_INFO(this->get_logger(),
                "Signal: %d dBm, Freq: %u MHz, TxPower: %.2f dBm, Distance: %.2f m, Fresnel radius: %.2f m",
                signal, freqMHz, txPowerDbm, distance, fresnel_radius);

    rf_link_proc::msg::RFInfo msg;
    msg.signal_strength = signal;
    msg.tx_power = txPowerDbm;
    msg.frequency = freqMHz;
    msg.estimated_distance = distance;
    msg.fresnel_radius = fresnel_radius;
    msg.interface = interface_; // or pass from param

    rf_info_pub_->publish(msg);

  } else {
    RCLCPP_WARN(this->get_logger(), "Failed to query RF signal strength");
  }
}
