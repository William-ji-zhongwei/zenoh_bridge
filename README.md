# Zenoh Cross-Domain C++ Project

This project demonstrates Zenoh Pub/Sub communication using C++.
It is designed to be portable:
1.  **Compile-time**: Dependencies (Zenoh C/C++) are included in `deps/zenoh`, so you don't need to install them on your system.
2.  **Runtime**: The build script creates a self-contained package with binaries and libraries.

## Project Structure

*   `src/`: Source code (`publisher.cpp`, `subscriber.cpp`).
*   `deps/zenoh/`: Pre-compiled Zenoh headers and libraries.
    *   `include/`: Headers.
    *   `lib/aarch64/`: ARM64 libraries.
    *   `lib/x86_64/`: x86_64 libraries.
*   `scripts/`: Build and run scripts.
    *   `build.sh`: Compiles the project.
    *   `run.sh`: Runs the publisher or subscriber.
    *   `package.sh`: Builds and packages the application into a tarball.
*   `config/`: Configuration files.
    *   `zenoh_config.json`: Zenoh configuration file.

## How to Build and Run Locally

You only need a C++ compiler (g++/clang) and CMake. You do **NOT** need to install Zenoh system-wide.

**Note**: This project supports both `aarch64` and `x86_64`. Ensure you have the correct `libzenohc.so` in `deps/zenoh/lib/<arch>/`.

### Build

```bash
./scripts/build.sh
```

### Run

```bash
./scripts/run.sh sub
# In another terminal
./scripts/run.sh pub
```

## How to Package for Distribution

To create a portable package (tarball):

```bash
./scripts/package.sh
```

This will create a package in the `output/` directory with a timestamp, e.g., `output/zenoh_project_package_YYYYMMDD_HHMMSS.tar.gz`.

### Running on Another Machine

1.  Copy the generated `.tar.gz` file to the target machine.
2.  Extract it: `tar -xzf zenoh_project_package_*.tar.gz`
3.  Run using the included script:
    ```bash
    ./run.sh sub
    # In another terminal
    ./run.sh pub
    ```

## Configuration

Edit `config/zenoh_config.json` to change Zenoh settings (e.g., connect to a router, change transport mode).
