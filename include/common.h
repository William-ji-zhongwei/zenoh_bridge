#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

namespace data_bridge {

// Protocol type for local forwarding
enum class ProtocolType {
    UDP,
    GRPC
};

// Configuration for a single data stream
struct StreamConfig {
    std::string zenoh_topic;          // Zenoh topic to subscribe
    ProtocolType protocol;            // Local forwarding protocol
    std::string local_host;           // Local destination host
    int local_port;                   // Local destination port
    std::string grpc_service;         // gRPC service name (only for gRPC)
    std::string grpc_method;          // gRPC method name (only for gRPC)
};

// Global configuration
struct BridgeConfig {
    // Zenoh settings
    std::string zenoh_mode = "client";
    std::string zenoh_connect = "";   // Empty means peer mode
    
    // Data streams to forward
    std::vector<StreamConfig> streams;
    
    // Load configuration from JSON file
    bool loadFromFile(const std::string& filepath);
    
    // Get default configuration
    static BridgeConfig getDefault();
};

} // namespace data_bridge
