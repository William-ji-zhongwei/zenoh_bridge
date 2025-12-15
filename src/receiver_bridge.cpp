#include "receiver_bridge.h"
#include <cstring>
#include <unistd.h>

namespace data_bridge {

ReceiverBridge::StreamHandler::~StreamHandler() {
    if (udp_socket >= 0) {
        close(udp_socket);
    }
}

ReceiverBridge::ReceiverBridge(const BridgeConfig& config)
    : config_(config),
      session_(zenoh::Session::open(zenoh::Config::create_default())) {
}

ReceiverBridge::~ReceiverBridge() {
    stop();
}

bool ReceiverBridge::start() {
    if (running_) {
        std::cerr << "[ReceiverBridge] Already running" << std::endl;
        return false;
    }
    
    std::cout << "[ReceiverBridge] Starting..." << std::endl;
    std::cout << "[ReceiverBridge] Zenoh session already opened in constructor" << std::endl;
    
    // Initialize all streams
    for (const auto& stream_config : config_.streams) {
        auto handler = std::make_unique<StreamHandler>();
        handler->config = stream_config;
        
        if (!initStream(*handler)) {
            std::cerr << "[ReceiverBridge] Failed to initialize stream: " 
                      << stream_config.zenoh_topic << std::endl;
            continue;
        }
        
        handlers_.push_back(std::move(handler));
    }
    
    if (handlers_.empty()) {
        std::cerr << "[ReceiverBridge] No streams initialized" << std::endl;
        return false;
    }
    
    running_ = true;
    std::cout << "[ReceiverBridge] Started with " << handlers_.size() << " stream(s)" << std::endl;
    
    return true;
}

void ReceiverBridge::stop() {
    if (!running_) {
        return;
    }
    
    std::cout << "[ReceiverBridge] Stopping..." << std::endl;
    running_ = false;
    
    // Close all streams
    for (auto& handler : handlers_) {
        closeStream(*handler);
    }
    handlers_.clear();
    
    std::cout << "[ReceiverBridge] Stopped" << std::endl;
}

bool ReceiverBridge::initStream(StreamHandler& handler) {
    const auto& config = handler.config;
    
    std::cout << "[ReceiverBridge] Initializing stream:" << std::endl;
    std::cout << "  Topic: " << config.zenoh_topic << std::endl;
    std::cout << "  Protocol: " << (config.protocol == ProtocolType::UDP ? "UDP" : "gRPC") << std::endl;
    std::cout << "  Destination: " << config.local_host << ":" << config.local_port << std::endl;
    
    // Initialize protocol-specific resources
    if (config.protocol == ProtocolType::UDP) {
        // Create UDP socket
        handler.udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (handler.udp_socket < 0) {
            std::cerr << "[ReceiverBridge] Failed to create UDP socket: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Setup destination address
        memset(&handler.udp_addr, 0, sizeof(handler.udp_addr));
        handler.udp_addr.sin_family = AF_INET;
        handler.udp_addr.sin_port = htons(config.local_port);
        
        if (inet_pton(AF_INET, config.local_host.c_str(), &handler.udp_addr.sin_addr) <= 0) {
            std::cerr << "[ReceiverBridge] Invalid UDP address: " << config.local_host << std::endl;
            close(handler.udp_socket);
            handler.udp_socket = -1;
            return false;
        }
        
    } else if (config.protocol == ProtocolType::GRPC) {
        // TODO: Initialize gRPC client stub
        std::cout << "[ReceiverBridge] gRPC support not yet implemented" << std::endl;
        std::cout << "  Service: " << config.grpc_service << std::endl;
        std::cout << "  Method: " << config.grpc_method << std::endl;
    }
    
    // Create Zenoh subscriber
    try {
        auto on_sample = [this, &handler](const zenoh::Sample& sample) {
            this->onDataReceived(handler.config, sample);
        };
        
        auto on_drop = []() {
            std::cout << "[ReceiverBridge] Subscriber dropped" << std::endl;
        };
        
        handler.subscriber = std::make_unique<zenoh::Subscriber<void>>(
            session_.declare_subscriber(config.zenoh_topic, on_sample, on_drop)
        );
        
        std::cout << "[ReceiverBridge] Subscribed to: " << config.zenoh_topic << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ReceiverBridge] Failed to create subscriber: " << e.what() << std::endl;
        closeStream(handler);
        return false;
    }
    
    return true;
}

void ReceiverBridge::closeStream(StreamHandler& handler) {
    handler.subscriber.reset();
    
    if (handler.udp_socket >= 0) {
        close(handler.udp_socket);
        handler.udp_socket = -1;
    }
    
    // TODO: Close gRPC resources
}

void ReceiverBridge::onDataReceived(const StreamConfig& config, const zenoh::Sample& sample) {
    // Get payload as vector directly
    const auto& payload = sample.get_payload();
    auto bytes = payload.as_vector();
    
    std::cout << "[ReceiverBridge] Received data on '" << config.zenoh_topic 
              << "': " << bytes.size() << " bytes" << std::endl;
    
    // Find handler for this topic
    for (auto& handler : handlers_) {
        if (handler->config.zenoh_topic == config.zenoh_topic) {
            if (!forwardData(*handler, bytes.data(), bytes.size())) {
                std::cerr << "[ReceiverBridge] Failed to forward data" << std::endl;
            }
            break;
        }
    }
}

bool ReceiverBridge::forwardData(StreamHandler& handler, const uint8_t* data, size_t len) {
    switch (handler.config.protocol) {
        case ProtocolType::UDP:
            return forwardViaUDP(handler, data, len);
        case ProtocolType::GRPC:
            return forwardViaGRPC(handler, data, len);
        default:
            std::cerr << "[ReceiverBridge] Unknown protocol type" << std::endl;
            return false;
    }
}

bool ReceiverBridge::forwardViaUDP(StreamHandler& handler, const uint8_t* data, size_t len) {
    if (handler.udp_socket < 0) {
        std::cerr << "[ReceiverBridge] UDP socket not initialized" << std::endl;
        return false;
    }
    
    ssize_t sent = sendto(handler.udp_socket, data, len, 0,
                         (struct sockaddr*)&handler.udp_addr, sizeof(handler.udp_addr));
    
    if (sent < 0) {
        std::cerr << "[ReceiverBridge] Failed to send UDP data: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (static_cast<size_t>(sent) != len) {
        std::cerr << "[ReceiverBridge] Partial UDP send: " << sent << "/" << len << " bytes" << std::endl;
        return false;
    }
    
    std::cout << "[ReceiverBridge] Forwarded " << sent << " bytes via UDP to " 
              << handler.config.local_host << ":" << handler.config.local_port << std::endl;
    return true;
}

bool ReceiverBridge::forwardViaGRPC(StreamHandler& handler, const uint8_t* data, size_t len) {
    // TODO: Implement gRPC forwarding
    std::cerr << "[ReceiverBridge] gRPC forwarding not yet implemented" << std::endl;
    std::cout << "[ReceiverBridge] Would send " << len << " bytes to gRPC service: " 
              << handler.config.grpc_service << "/" << handler.config.grpc_method << std::endl;
    return false;
}

} // namespace data_bridge
