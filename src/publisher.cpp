#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "zenoh.hxx"

using namespace std::chrono_literals;

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

    std::string key = "demo/example/zenoh-cpp-pub";
    
    std::cout << "Declaring publisher on '" << key << "'..." << std::endl;
    auto publisher = session.declare_publisher(key);

    int count = 0;
    while (true) {
        std::string msg = "Hello Zenoh! " + std::to_string(count++);
        std::cout << "Putting Data ('" << key << "': '" << msg << "')..." << std::endl;
        publisher.put(msg);
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
