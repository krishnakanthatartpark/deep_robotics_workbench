#ifndef NL80211_CLIENT_H
#define NL80211_CLIENT_H

#include <string>

class RFSignalHandler;  // Forward declaration

/**
 * @brief Handles netlink communication with nl80211 to query AP data (signal, frequency, etc.).
 */
class NL80211Client {
public:
    /**
     * @brief Construct a new NL80211Client object for a specific wireless interface.
     * @param iface Name of the interface (e.g., "wlan0", "wlo1").
     */
    explicit NL80211Client(const std::string& iface);

    /**
     * @brief Sends queries to retrieve both signal strength and frequency from the AP.
     * @param handler Reference to handler that parses the netlink messages.
     * @return true if both queries succeed.
     */
    bool queryAPSignal(RFSignalHandler& handler);

private:
    /**
     * @brief Generic method to send a specific nl80211 command and receive data.
     * @param cmd The nl80211 command to execute (e.g., NL80211_CMD_GET_STATION).
     * @param handler Reference to handler to process the response.
     * @return true on successful netlink communication.
     */
    bool sendAndReceive(int cmd, RFSignalHandler& handler, const uint8_t* mac = nullptr);

    bool getAPMac(const std::string& iface, uint8_t mac_out[6]);

    std::string iface_;  ///< Wireless interface name.
};

#endif  // NL80211_CLIENT_H
