#include <iostream>
#include <thread>
#include <chrono>
#include "zenoh.hxx"

int main(int argc, char** argv) {
    // Initialize configuration
    zenoh::Config config = zenoh::Config::create_default();
    if (argc > 1) {
        std::cout << "Loading configuration from '" << argv[1] << "'..." << std::endl;
        config = zenoh::Config::from_file(argv[1]);
    }

    // Open session
    std::cout << "Opening session..." << std::endl;
    auto session = zenoh::Session::open(std::move(config));

    std::string key = "demo/example/**";

    std::cout << "Declaring subscriber on '" << key << "'..." << std::endl;
    
    // Callback for data
    auto callback = [](const zenoh::Sample& sample) {
        std::cout << "Received Data ('" << sample.get_keyexpr().as_string_view() << "': '" 
                  << sample.get_payload().as_string() << "')" << std::endl;
    };

    auto subscriber = session.declare_subscriber(key, callback, [](){});

    std::cout << "Waiting for data... (Press Ctrl+C to quit)" << std::endl;
    
    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
