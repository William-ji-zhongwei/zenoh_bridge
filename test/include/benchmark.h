#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

namespace benchmark {

// Statistics for performance measurement
struct Statistics {
    std::atomic<uint64_t> total_messages{0};
    std::atomic<uint64_t> total_bytes{0};
    std::atomic<uint64_t> dropped_messages{0};
    std::chrono::steady_clock::time_point start_time;
    mutable std::chrono::steady_clock::time_point end_time;
    
    // Latency tracking
    std::vector<double> latencies_ms;
    mutable std::mutex latency_mutex;
    
    void reset();
    void recordMessage(size_t bytes, double latency_ms = 0.0);
    void printReport() const;
    double getMessagesPerSecond() const;
    double getMegabytesPerSecond() const;
    double getAverageLatencyMs() const;
    double getP99LatencyMs() const;
};

// Benchmark configuration
struct BenchmarkConfig {
    std::string zenoh_topic = "benchmark/data";
    size_t message_size = 1024;           // bytes per message
    size_t messages_per_second = 1000;    // target message rate
    size_t duration_seconds = 10;         // benchmark duration
    size_t num_publishers = 1;            // number of concurrent publishers
    bool measure_latency = true;          // measure end-to-end latency
    bool verbose = false;                 // print detailed stats
};

// High-throughput data publisher for benchmarking
class BenchmarkPublisher {
public:
    explicit BenchmarkPublisher(const BenchmarkConfig& config);
    ~BenchmarkPublisher();
    
    // Start the benchmark
    bool start();
    
    // Stop the benchmark
    void stop();
    
    // Get statistics
    const Statistics& getStats() const { return stats_; }
    
    // Print final report
    void printReport() const;

private:
    void publishLoop(int publisher_id);
    std::vector<uint8_t> generateTestData(size_t size);
    
private:
    BenchmarkConfig config_;
    std::atomic<bool> running_{false};
    Statistics stats_;
    
    std::vector<std::thread> publisher_threads_;
};

// UDP receiver for benchmarking (measures what receiver gets)
class BenchmarkReceiver {
public:
    explicit BenchmarkReceiver(int port);
    ~BenchmarkReceiver();
    
    bool start();
    void stop();
    
    const Statistics& getStats() const { return stats_; }
    void printReport() const;

private:
    void receiveLoop();
    
private:
    int port_;
    int socket_{-1};
    std::atomic<bool> running_{false};
    Statistics stats_;
    std::thread receive_thread_;
};

} // namespace benchmark
