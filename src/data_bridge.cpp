#include "receiver_bridge.h"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\n[Main] Interrupt signal (" << signum << ") received" << std::endl;
    running.store(false);
}

int main(int argc, char** argv) {
    std::cout << "===========================================\n";
    std::cout << "  Zenoh Data Receiver Bridge\n";
    std::cout << "===========================================\n\n";

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Load configuration
    data_bridge::BridgeConfig config;
    
    if (argc > 1) {
        std::string config_file = argv[1];
        std::cout << "[Main] Loading config from: " << config_file << std::endl;
        if (!config.loadFromFile(config_file)) {
            std::cout << "[Main] Failed to load config, using defaults" << std::endl;
            config = data_bridge::BridgeConfig::getDefault();
        }
    } else {
        std::cout << "[Main] No config file specified, using defaults" << std::endl;
        config = data_bridge::BridgeConfig::getDefault();
    }

    // Print configuration
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Zenoh Mode: " << config.zenoh_mode << std::endl;
    std::cout << "  Zenoh Connect: " << (config.zenoh_connect.empty() ? "peer" : config.zenoh_connect) << std::endl;
    std::cout << "  Streams: " << config.streams.size() << std::endl;
    
    for (size_t i = 0; i < config.streams.size(); ++i) {
        const auto& stream = config.streams[i];
        std::cout << "  [" << i << "] Topic: " << stream.zenoh_topic << std::endl;
        std::cout << "      Protocol: " << (stream.protocol == data_bridge::ProtocolType::UDP ? "UDP" : "GRPC") << std::endl;
        std::cout << "      Target: " << stream.local_host << ":" << stream.local_port << std::endl;
    }
    std::cout << std::endl;

    // Create and start receiver bridge
    try {
        data_bridge::ReceiverBridge bridge(config);
        
        if (!bridge.start()) {
            std::cerr << "[Main] Failed to start receiver bridge" << std::endl;
            return 1;
        }

        std::cout << "[Main] Receiver bridge running. Press Ctrl+C to stop..." << std::endl;

        // Main loop
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\n[Main] Shutting down receiver bridge..." << std::endl;
        bridge.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "[Main] Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[Main] Shutdown complete. Goodbye!" << std::endl;
    return 0;
}
