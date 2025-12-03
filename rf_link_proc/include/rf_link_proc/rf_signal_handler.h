#pragma once

#include <cstdint>
#include <netlink/msg.h>

class RFSignalHandler {
public:
    RFSignalHandler() = default;

    // Called by Netlink to handle received messages
    int callback(struct nl_msg* msg);

    // Getters
    int8_t getSignalStrength() const;
    uint32_t getFrequencyMHz() const;
    float getTxPowerDbm() const;

private:
    // Helpers for parsing each command's response
    bool handleStationInfo(struct nl_msg* msg);
    bool handleInterfaceInfo(struct nl_msg* msg);

    // Internal storage
    int8_t signalStrength_ = 0;      // in dBm
    uint32_t frequencyMHz_ = 0;      // in MHz
    float txPowerDbm_ = 0.0f;        // in dBm
};
