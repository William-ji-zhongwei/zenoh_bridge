#include "benchmark.h"
#include <iostream>
#include <csignal>
#include <thread>

std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\n[Main] Interrupt signal (" << signum << ") received" << std::endl;
    g_running = false;
}

void printUsage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -t <topic>        Zenoh topic (default: benchmark/data)" << std::endl;
    std::cout << "  -s <size>         Message size in bytes (default: 1024)" << std::endl;
    std::cout << "  -r <rate>         Messages per second (default: 1000)" << std::endl;
    std::cout << "  -d <duration>     Benchmark duration in seconds (default: 10)" << std::endl;
    std::cout << "  -p <publishers>   Number of concurrent publishers (default: 1)" << std::endl;
    std::cout << "  -v                Verbose output" << std::endl;
    std::cout << "  -h                Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  # Test with 1KB messages at 1000 msg/s for 10 seconds" << std::endl;
    std::cout << "  " << prog_name << " -s 1024 -r 1000 -d 10" << std::endl;
    std::cout << "\n  # High throughput test: 10KB messages at 10000 msg/s with 4 publishers" << std::endl;
    std::cout << "  " << prog_name << " -s 10240 -r 10000 -p 4 -d 30" << std::endl;
    std::cout << "\n  # Low latency test: small messages at high rate" << std::endl;
    std::cout << "  " << prog_name << " -s 64 -r 50000 -d 10" << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    benchmark::BenchmarkConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-t" && i + 1 < argc) {
            config.zenoh_topic = argv[++i];
        } else if (arg == "-s" && i + 1 < argc) {
            config.message_size = std::stoul(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            config.messages_per_second = std::stoul(argv[++i]);
        } else if (arg == "-d" && i + 1 < argc) {
            config.duration_seconds = std::stoul(argv[++i]);
        } else if (arg == "-p" && i + 1 < argc) {
            config.num_publishers = std::stoul(argv[++i]);
        } else if (arg == "-v") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Zenoh Benchmark Publisher" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Create and start benchmark
    benchmark::BenchmarkPublisher publisher(config);
    
    if (!publisher.start()) {
        std::cerr << "Failed to start benchmark publisher" << std::endl;
        return 1;
    }
    
    std::cout << "\n[Main] Benchmark running..." << std::endl;
    std::cout << "[Main] Press Ctrl+C to stop early\n" << std::endl;
    
    // Wait for duration or interrupt
    auto start = std::chrono::steady_clock::now();
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        
        if (elapsed >= static_cast<long>(config.duration_seconds)) {
            break;
        }
    }
    
    std::cout << "\n[Main] Stopping benchmark..." << std::endl;
    publisher.stop();
    
    // Print final report
    publisher.printReport();
    
    std::cout << "[Main] Benchmark complete!" << std::endl;
    
    return 0;
}
