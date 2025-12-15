#pragma once

#include "common.h"
#include <zenoh.hxx>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

namespace data_bridge {

/**
 * @brief Data Receiver Bridge - Receives data from Zenoh and forwards to local services
 *        Supports multiple protocols: UDP, gRPC
 */
class ReceiverBridge {
public:
    explicit ReceiverBridge(const BridgeConfig& config);
    ~ReceiverBridge();
    
    // Start all configured receivers
    bool start();
    
    // Stop all receivers
    void stop();
    
    // Check if running
    bool isRunning() const { return running_; }

private:
    // Single stream handler
    struct StreamHandler {
        StreamConfig config;
        std::unique_ptr<zenoh::Subscriber<void>> subscriber;
        int udp_socket = -1;
        struct sockaddr_in udp_addr;
        // TODO: Add gRPC client stub for gRPC protocol
        
        ~StreamHandler();
    };
    
    // Initialize a single stream
    bool initStream(StreamHandler& handler);
    
    // Close a single stream
    void closeStream(StreamHandler& handler);
    
    // Zenoh callback for receiving data
    void onDataReceived(const StreamConfig& config, const zenoh::Sample& sample);
    
    // Forward data based on protocol type
    bool forwardData(StreamHandler& handler, const uint8_t* data, size_t len);
    
    // UDP specific forwarding
    bool forwardViaUDP(StreamHandler& handler, const uint8_t* data, size_t len);
    
    // gRPC specific forwarding
    bool forwardViaGRPC(StreamHandler& handler, const uint8_t* data, size_t len);

private:
    BridgeConfig config_;
    std::atomic<bool> running_{false};
    
    // Zenoh session (will be initialized in start())
    zenoh::Session session_;
    
    // Stream handlers
    std::vector<std::unique_ptr<StreamHandler>> handlers_;
};

} // namespace data_bridge
