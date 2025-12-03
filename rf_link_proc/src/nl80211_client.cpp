#include "nl80211_client.h"
#include "rf_signal_handler.h"

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

NL80211Client::NL80211Client(const std::string& iface)
    : iface_(iface) {}

bool NL80211Client::getAPMac(const std::string& iface, uint8_t mac_out[6]) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct iwreq req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.ifr_name, iface.c_str(), IFNAMSIZ);

    if (ioctl(sock, SIOCGIWAP, &req) == -1) {
        close(sock);
        return false;
    }

    std::memcpy(mac_out, req.u.ap_addr.sa_data, 6);
    close(sock);
    return true;
}

bool NL80211Client::sendAndReceive(int cmd, RFSignalHandler& handler, const uint8_t* mac) {
    struct nl_sock* sock = nl_socket_alloc();
    if (!sock) {
        std::cerr << "Failed to allocate socket\n";
        return false;
    }

    nl_socket_disable_seq_check(sock);
    nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM,
        [](nl_msg* msg, void* arg) -> int {
            auto* h = static_cast<RFSignalHandler*>(arg);
            return h->callback(msg);
        },
        &handler);

    if (nl_connect(sock, NETLINK_GENERIC) < 0) {
        std::cerr << "Failed to connect to netlink\n";
        nl_socket_free(sock);
        return false;
    }

    int driver_id = genl_ctrl_resolve(sock, "nl80211");
    if (driver_id < 0) {
        std::cerr << "Failed to resolve nl80211\n";
        nl_socket_free(sock);
        return false;
    }

    int if_index = if_nametoindex(iface_.c_str());
    if (if_index == 0) {
        std::cerr << "Invalid interface name: " << iface_ << "\n";
        nl_socket_free(sock);
        return false;
    }

    nl_msg* msg = nlmsg_alloc();
    if (!msg) {
        std::cerr << "Failed to allocate netlink message\n";
        nl_socket_free(sock);
        return false;
    }

    genlmsg_put(msg, 0, 0, driver_id, 0, 0, cmd, 0);
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

    if (cmd == NL80211_CMD_GET_STATION && mac) {
    nla_put(msg, NL80211_ATTR_MAC, 6, mac);  // Include peer MAC for STA queries
    }

    bool ok = (nl_send_auto(sock, msg) >= 0);
    if (ok) nl_recvmsgs_default(sock);

    nlmsg_free(msg);
    nl_socket_free(sock);
    return ok;
}

bool NL80211Client::queryAPSignal(RFSignalHandler& handler) {
    uint8_t ap_mac[6];
    if(getAPMac(iface_, ap_mac)){
        bool ok1 = sendAndReceive(NL80211_CMD_GET_STATION, handler, ap_mac);    // Signal strength (RSSI)
        bool ok2 = sendAndReceive(NL80211_CMD_GET_INTERFACE, handler);  // Frequency
        bool ok3 = sendAndReceive(NL80211_CMD_GET_WIPHY, handler);      // Tx Power
        return ok1 && ok2 && ok3;
    }
    else{
        std::cerr << "Failed to get AP MAC address" << std::endl;
        return false;
    } 
}
