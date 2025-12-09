cmake_minimum_required(VERSION 3.15)
project(zenoh_cross_domain_project VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
# Assuming zenoh-cpp and zenoh-c are installed on the system.
# If not, you might need to adjust CMAKE_PREFIX_PATH or use FetchContent.
find_package(zenohc CONFIG REQUIRED)
find_package(zenohcpp CONFIG REQUIRED)

# Publisher Executable
add_executable(zenoh_pub src/publisher.cpp)
target_link_libraries(zenoh_pub PRIVATE zenohcpp::libzenohcpp)

# Subscriber Executable
add_executable(zenoh_sub src/subscriber.cpp)
target_link_libraries(zenoh_sub PRIVATE zenohcpp::libzenohcpp)

# Compiler warnings (optional but recommended)
if(MSVC)
    target_compile_options(zenoh_pub PRIVATE /W4)
    target_compile_options(zenoh_sub PRIVATE /W4)
else()
    target_compile_options(zenoh_pub PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(zenoh_sub PRIVATE -Wall -Wextra -Wpedantic)
endif()
