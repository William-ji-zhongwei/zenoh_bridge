# Zenoh Cross-Domain C++ Project

This is a sample C++ project using [Zenoh](https://zenoh.io/) for cross-domain communication.

## Prerequisites

You need to have `zenoh-c` and `zenoh-cpp` installed on your system.
See [Zenoh C++ installation guide](https://github.com/eclipse-zenoh/zenoh-cpp) for details.

Usually, you can install them via Cargo (if using Rust) or build from source.

## Build

```bash
mkdir build
cd build
cmake ..
make
```

## Run

### Subscriber
```bash
./zenoh_sub [config_file]
```

### Publisher
```bash
./zenoh_pub [config_file]
```

## Configuration

You can pass a configuration file (JSON5) as the first argument.
Example `zenoh_config.json5` is provided.

## Structure

- `src/`: Source code
- `CMakeLists.txt`: CMake build configuration
- `zenoh_config.json5`: Sample configuration
