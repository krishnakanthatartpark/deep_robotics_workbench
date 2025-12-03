#include "rf_signal_handler.h"

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/attr.h>
#include <iostream>

int RFSignalHandler::callback(struct nl_msg* msg) {
    struct nlmsghdr* nlh = nlmsg_hdr(msg);
    auto* gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlh));
    switch (gnlh->cmd) {
        case NL80211_CMD_NEW_STATION:
            return handleStationInfo(msg) ? NL_OK : NL_STOP;
        case NL80211_CMD_NEW_INTERFACE:
        case NL80211_CMD_NEW_WIPHY:
            return handleInterfaceInfo(msg) ? NL_OK : NL_STOP;
        default:
            return NL_SKIP;
    }
}

bool RFSignalHandler::handleStationInfo(struct nl_msg* msg) {
    struct nlmsghdr* nlh = nlmsg_hdr(msg);
    auto* gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlh));
    struct nlattr* attrs[NL80211_ATTR_MAX + 1] = {nullptr};

    nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), nullptr);

    if (attrs[NL80211_ATTR_STA_INFO]) {
        struct nlattr* sinfo[NL80211_STA_INFO_MAX + 1] = {nullptr};
        nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
                         attrs[NL80211_ATTR_STA_INFO], nullptr);

        if (sinfo[NL80211_STA_INFO_SIGNAL]) {
            signalStrength_ = nla_get_s8(sinfo[NL80211_STA_INFO_SIGNAL]);
            std::cout << "[INFO] Signal strength: " << static_cast<int>(signalStrength_) << " dBm" << std::endl;
        }
    }

    return true;
}

bool RFSignalHandler::handleInterfaceInfo(struct nl_msg* msg) {
    struct nlmsghdr* nlh = nlmsg_hdr(msg);
    auto* gnlh = static_cast<genlmsghdr*>(nlmsg_data(nlh));
    struct nlattr* attrs[NL80211_ATTR_MAX + 1] = {nullptr};

    nla_parse(attrs, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), nullptr);

    if (attrs[NL80211_ATTR_WIPHY_FREQ]) {
        frequencyMHz_ = nla_get_u32(attrs[NL80211_ATTR_WIPHY_FREQ]);
        std::cout << "[INFO] Frequency: " << frequencyMHz_ << " MHz" << std::endl;
    }

    if (attrs[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
        int32_t tx_power_mBm = nla_get_u32(attrs[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
        txPowerDbm_ = static_cast<float>(tx_power_mBm) / 100.0f;
        std::cout << "[INFO] Client Tx Power: " << txPowerDbm_ << " dBm" << std::endl;
    }

    return true;
}

int8_t RFSignalHandler::getSignalStrength() const {
    return signalStrength_;
}

uint32_t RFSignalHandler::getFrequencyMHz() const {
    return frequencyMHz_;
}

float RFSignalHandler::getTxPowerDbm() const {
    return txPowerDbm_;
}
