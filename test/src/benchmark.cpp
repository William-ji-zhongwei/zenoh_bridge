#include "benchmark.h"
#include <zenoh.hxx>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace benchmark {

// Statistics implementation
void Statistics::reset() {
    total_messages = 0;
    total_bytes = 0;
    dropped_messages = 0;
    start_time = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(latency_mutex);
    latencies_ms.clear();
}

void Statistics::recordMessage(size_t bytes, double latency_ms) {
    total_messages++;
    total_bytes += bytes;
    
    if (latency_ms > 0.0) {
        std::lock_guard<std::mutex> lock(latency_mutex);
        latencies_ms.push_back(latency_ms);
    }
}

void Statistics::printReport() const {
    end_time = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count() / 1000.0;
    
    std::cout << "\n========== Performance Report ==========" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Duration:          " << duration << " seconds" << std::endl;
    std::cout << "Total Messages:    " << total_messages.load() << std::endl;
    std::cout << "Total Bytes:       " << total_bytes.load() / (1024.0 * 1024.0) << " MB" << std::endl;
    std::cout << "Dropped Messages:  " << dropped_messages.load() << std::endl;
    std::cout << "Messages/sec:      " << getMessagesPerSecond() << std::endl;
    std::cout << "Throughput:        " << getMegabytesPerSecond() << " MB/s" << std::endl;
    
    if (!latencies_ms.empty()) {
        std::cout << "\nLatency Statistics:" << std::endl;
        std::cout << "  Average:         " << getAverageLatencyMs() << " ms" << std::endl;
        std::cout << "  P99:             " << getP99LatencyMs() << " ms" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

double Statistics::getMessagesPerSecond() const {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count() / 1000.0;
    return duration > 0 ? total_messages.load() / duration : 0.0;
}

double Statistics::getMegabytesPerSecond() const {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count() / 1000.0;
    return duration > 0 ? (total_bytes.load() / (1024.0 * 1024.0)) / duration : 0.0;
}

double Statistics::getAverageLatencyMs() const {
    std::lock_guard<std::mutex> lock(latency_mutex);
    if (latencies_ms.empty()) return 0.0;
    
    double sum = 0.0;
    for (auto lat : latencies_ms) {
        sum += lat;
    }
    return sum / latencies_ms.size();
}

double Statistics::getP99LatencyMs() const {
    std::lock_guard<std::mutex> lock(latency_mutex);
    if (latencies_ms.empty()) return 0.0;
    
    auto sorted = latencies_ms;
    std::sort(sorted.begin(), sorted.end());
    
    size_t p99_index = static_cast<size_t>(sorted.size() * 0.99);
    return sorted[std::min(p99_index, sorted.size() - 1)];
}

// BenchmarkPublisher implementation
BenchmarkPublisher::BenchmarkPublisher(const BenchmarkConfig& config)
    : config_(config) {
}

BenchmarkPublisher::~BenchmarkPublisher() {
    stop();
}

bool BenchmarkPublisher::start() {
    if (running_) {
        std::cerr << "[Benchmark] Already running" << std::endl;
        return false;
    }
    
    std::cout << "[Benchmark] Starting benchmark publisher..." << std::endl;
    std::cout << "  Topic: " << config_.zenoh_topic << std::endl;
    std::cout << "  Message Size: " << config_.message_size << " bytes" << std::endl;
    std::cout << "  Target Rate: " << config_.messages_per_second << " msg/s" << std::endl;
    std::cout << "  Publishers: " << config_.num_publishers << std::endl;
    std::cout << "  Duration: " << config_.duration_seconds << " seconds" << std::endl;
    
    stats_.reset();
    running_ = true;
    
    // Start publisher threads
    for (size_t i = 0; i < config_.num_publishers; ++i) {
        publisher_threads_.emplace_back(&BenchmarkPublisher::publishLoop, this, i);
    }
    
    return true;
}

void BenchmarkPublisher::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    for (auto& thread : publisher_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    publisher_threads_.clear();
    
    stats_.end_time = std::chrono::steady_clock::now();
}

void BenchmarkPublisher::publishLoop(int publisher_id) {
    try {
        // Open Zenoh session
        zenoh::Config zenoh_config = zenoh::Config::create_default();
        auto session = zenoh::Session::open(std::move(zenoh_config));
        auto publisher = session.declare_publisher(config_.zenoh_topic);
        
        std::cout << "[Publisher " << publisher_id << "] Started" << std::endl;
        
        // Calculate delay between messages for this publisher
        size_t messages_per_publisher = config_.messages_per_second / config_.num_publishers;
        auto delay = std::chrono::microseconds(1000000 / messages_per_publisher);
        
        // Generate test data
        auto test_data = generateTestData(config_.message_size);
        
        auto start_time = std::chrono::steady_clock::now();
        auto next_send_time = start_time;
        
        size_t msg_count = 0;
        
        while (running_) {
            auto now = std::chrono::steady_clock::now();
            
            // Check if benchmark duration exceeded
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            if (elapsed.count() >= static_cast<long>(config_.duration_seconds)) {
                break;
            }
            
            // Time-based rate limiting
            if (now >= next_send_time) {
                // Add timestamp if measuring latency
                if (config_.measure_latency) {
                    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                        now.time_since_epoch()).count();
                    std::memcpy(test_data.data(), &timestamp, sizeof(timestamp));
                }
                
                // Publish message
                publisher.put(test_data);
                stats_.recordMessage(test_data.size());
                
                msg_count++;
                next_send_time += delay;
                
                // Verbose output
                if (config_.verbose && msg_count % 1000 == 0) {
                    std::cout << "[Publisher " << publisher_id << "] Sent " << msg_count << " messages" << std::endl;
                }
            } else {
                // Sleep a bit to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        std::cout << "[Publisher " << publisher_id << "] Finished. Sent " << msg_count << " messages" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[Publisher " << publisher_id << "] Error: " << e.what() << std::endl;
    }
}

std::vector<uint8_t> BenchmarkPublisher::generateTestData(size_t size) {
    std::vector<uint8_t> data(size);
    
    // Fill with pattern (reserve first 8 bytes for timestamp if needed)
    size_t start = config_.measure_latency ? sizeof(uint64_t) : 0;
    for (size_t i = start; i < size; ++i) {
        data[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    return data;
}

void BenchmarkPublisher::printReport() const {
    stats_.printReport();
}

// BenchmarkReceiver implementation
BenchmarkReceiver::BenchmarkReceiver(int port)
    : port_(port) {
}

BenchmarkReceiver::~BenchmarkReceiver() {
    stop();
}

bool BenchmarkReceiver::start() {
    if (running_) {
        std::cerr << "[BenchmarkReceiver] Already running" << std::endl;
        return false;
    }
    
    std::cout << "[BenchmarkReceiver] Starting on UDP port " << port_ << "..." << std::endl;
    
    // Create UDP socket
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        std::cerr << "[BenchmarkReceiver] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set socket to reuse address
    int reuse = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "[BenchmarkReceiver] Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
    }
    
    // Bind socket
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[BenchmarkReceiver] Failed to bind: " << strerror(errno) << std::endl;
        close(socket_);
        socket_ = -1;
        return false;
    }
    
    stats_.reset();
    running_ = true;
    receive_thread_ = std::thread(&BenchmarkReceiver::receiveLoop, this);
    
    std::cout << "[BenchmarkReceiver] Started" << std::endl;
    return true;
}

void BenchmarkReceiver::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (socket_ >= 0) {
        close(socket_);
        socket_ = -1;
    }
    
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    
    stats_.end_time = std::chrono::steady_clock::now();
}

void BenchmarkReceiver::receiveLoop() {
    std::vector<uint8_t> buffer(65536);
    
    while (running_) {
        ssize_t received = recv(socket_, buffer.data(), buffer.size(), 0);
        
        if (received < 0) {
            if (running_) {
                std::cerr << "[BenchmarkReceiver] Receive error: " << strerror(errno) << std::endl;
            }
            break;
        }
        
        if (received == 0) {
            continue;
        }
        
        auto now = std::chrono::steady_clock::now();
        
        // Extract timestamp if present (first 8 bytes)
        double latency_ms = 0.0;
        if (received >= static_cast<ssize_t>(sizeof(uint64_t))) {
            uint64_t send_timestamp;
            std::memcpy(&send_timestamp, buffer.data(), sizeof(send_timestamp));
            
            auto recv_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count();
            
            latency_ms = (recv_timestamp - send_timestamp) / 1000.0;
        }
        
        stats_.recordMessage(received, latency_ms);
    }
}

void BenchmarkReceiver::printReport() const {
    stats_.printReport();
}

} // namespace benchmark
