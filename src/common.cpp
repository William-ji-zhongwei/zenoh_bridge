#include "common.h"
#include <fstream>
#include <sstream>

namespace data_bridge {

bool BridgeConfig::loadFromFile(const std::string& filepath) {
    // TODO: Implement JSON parsing using nlohmann/json or similar
    // For now, just return false to use default config
    std::cerr << "[Config] JSON parsing not yet implemented, using default config" << std::endl;
    return false;
}

BridgeConfig BridgeConfig::getDefault() {
    BridgeConfig config;
    config.zenoh_mode = "client";
    config.zenoh_connect = "";
    
    // Default stream: benchmark data (compatible with benchmark_pub)
    StreamConfig stream1;
    stream1.zenoh_topic = "benchmark/data";
    stream1.protocol = ProtocolType::UDP;
    stream1.local_host = "127.0.0.1";
    stream1.local_port = 8888;
    
    config.streams.push_back(stream1);
    
    return config;
}

} // namespace data_bridge
