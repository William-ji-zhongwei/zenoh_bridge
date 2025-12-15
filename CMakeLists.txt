cmake_minimum_required(VERSION 3.15)
project(zenoh_cross_domain_project VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RPATH Configuration
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# --- Dependencies (Local Vendor) ---
set(ZENOH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/zenoh")

# Detect Architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(ARCH_DIR "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(ARCH_DIR "x86_64")
else()
    message(WARNING "Unknown processor: ${CMAKE_SYSTEM_PROCESSOR}. Defaulting to x86_64.")
    set(ARCH_DIR "x86_64")
endif()
message(STATUS "Target Architecture: ${ARCH_DIR}")

# 1. Zenoh-C Library (Imported)
add_library(libzenohc SHARED IMPORTED)
set_target_properties(libzenohc PROPERTIES
    IMPORTED_LOCATION "${ZENOH_ROOT}/lib/${ARCH_DIR}/libzenohc.so"
    INTERFACE_INCLUDE_DIRECTORIES "${ZENOH_ROOT}/include"
)

# 2. Zenoh-CPP (Header-only wrapper)
add_library(zenohcxx INTERFACE)
target_link_libraries(zenohcxx INTERFACE libzenohc)
target_include_directories(zenohcxx INTERFACE "${ZENOH_ROOT}/include")
target_compile_definitions(zenohcxx INTERFACE ZENOHCXX_ZENOHC)

# Alias to match previous usage
add_library(zenohcxx::zenohc ALIAS zenohcxx)

# Publisher Executable
add_executable(zenoh_pub src/publisher.cpp)
target_link_libraries(zenoh_pub PRIVATE zenohcxx::zenohc)

# Subscriber Executable
add_executable(zenoh_sub src/subscriber.cpp)
target_link_libraries(zenoh_sub PRIVATE zenohcxx::zenohc)

# Data Receiver Bridge Executable
add_executable(data_bridge 
    src/data_bridge.cpp
    src/receiver_bridge.cpp
    src/common.cpp
)
target_include_directories(data_bridge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(data_bridge PRIVATE zenohcxx::zenohc)

# Benchmark/Test Tools (from test/ directory)
add_executable(benchmark_pub 
    test/src/benchmark_pub.cpp
    test/src/benchmark.cpp
)
target_include_directories(benchmark_pub PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/test/include"
)
target_link_libraries(benchmark_pub PRIVATE zenohcxx::zenohc)

# Benchmark Receiver (UDP)
add_executable(benchmark_recv 
    test/src/benchmark_recv.cpp
    test/src/benchmark.cpp
)
target_include_directories(benchmark_recv PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/test/include"
)
target_link_libraries(benchmark_recv PRIVATE zenohcxx::zenohc)

# Compiler warnings (optional but recommended)
if(MSVC)
    target_compile_options(zenoh_pub PRIVATE /W4)
    target_compile_options(zenoh_sub PRIVATE /W4)
    target_compile_options(data_bridge PRIVATE /W4)
    target_compile_options(benchmark_pub PRIVATE /W4)
    target_compile_options(benchmark_recv PRIVATE /W4)
else()
    target_compile_options(zenoh_pub PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(zenoh_sub PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(data_bridge PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(benchmark_pub PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(benchmark_recv PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install Rules
install(TARGETS zenoh_pub zenoh_sub data_bridge benchmark_pub benchmark_recv DESTINATION bin)
install(FILES "${ZENOH_ROOT}/lib/${ARCH_DIR}/libzenohc.so" DESTINATION lib)

