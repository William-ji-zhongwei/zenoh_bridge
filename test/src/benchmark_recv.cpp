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
    std::cout << "Usage: " << prog_name << " [port]" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  port              UDP port to listen on (default: 8888)" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << prog_name << " 8888" << std::endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    int port = 8888;
    
    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        port = std::stoi(argv[1]);
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Zenoh Benchmark Receiver (UDP)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Create and start receiver
    benchmark::BenchmarkReceiver receiver(port);
    
    if (!receiver.start()) {
        std::cerr << "Failed to start benchmark receiver" << std::endl;
        return 1;
    }
    
    std::cout << "\n[Main] Receiver running..." << std::endl;
    std::cout << "[Main] Press Ctrl+C to stop and show statistics\n" << std::endl;
    
    // Wait for interrupt
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Print interim stats every 5 seconds
        static int counter = 0;
        if (++counter % 5 == 0) {
            const auto& stats = receiver.getStats();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - stats.start_time).count();
            
            if (elapsed > 0) {
                std::cout << "[Stats] " << elapsed << "s | "
                          << "Messages: " << stats.total_messages.load() << " | "
                          << "Rate: " << (stats.total_messages.load() / elapsed) << " msg/s | "
                          << "Throughput: " << (stats.total_bytes.load() / (1024.0 * 1024.0 * elapsed)) << " MB/s"
                          << std::endl;
            }
        }
    }
    
    std::cout << "\n[Main] Stopping receiver..." << std::endl;
    receiver.stop();
    
    // Print final report
    receiver.printReport();
    
    std::cout << "[Main] Receiver stopped!" << std::endl;
    
    return 0;
}
