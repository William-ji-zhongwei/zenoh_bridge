#pragma once

#include "common.h"
#include <zenoh.hxx>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <optional>

namespace vr_bridge {

/**
 * @brief Control Bridge - Receives robot control data from cloud via Zenoh
 *        and forwards it to local robot via UDP
 */
class ControlBridge {
public:
    explicit ControlBridge(const BridgeConfig& config);
    ~ControlBridge();
    
    // Start the control bridge
    bool start();
    
    // Stop the control bridge
    void stop();
    
    // Check if running
    bool isRunning() const { return running_; }

private:
    // Zenoh callback for receiving control data
    void onControlDataReceived(const zenoh::Sample& sample);
    
    // Forward control data to robot via UDP
    bool forwardToRobot(const uint8_t* data, size_t len);
    
    // Initialize UDP socket
    bool initUdpSocket();
    
    // Close UDP socket
    void closeUdpSocket();

private:
    BridgeConfig config_;
    std::atomic<bool> running_{false};
    
    // Zenoh session and subscriber
    std::optional<zenoh::Session> session_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    
    // UDP socket
    int udp_socket_{-1};
    struct sockaddr_in robot_addr_;
};

} // namespace vr_bridge
