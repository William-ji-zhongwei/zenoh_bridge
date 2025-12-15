#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(realpath "$SCRIPT_DIR/..")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
OUTPUT_DIR="$PROJECT_ROOT/output"

# Detect Architecture
ARCH=$(uname -m)
if [[ "$ARCH" == "x86_64" ]]; then
    ARCH_TAG="x86_64"
elif [[ "$ARCH" == "aarch64" ]]; then
    ARCH_TAG="aarch64"
else
    ARCH_TAG="$ARCH"
fi

# Generate timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
PACKAGE_NAME="zenoh_project_package_${ARCH_TAG}_${TIMESTAMP}.tar.gz"

echo "Cleaning previous distribution..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$OUTPUT_DIR"

# Prepare package directory
PACKAGE_ROOT="$DIST_DIR/zenoh_bridge"
mkdir -p "$PACKAGE_ROOT"

# Build and Install (to dist/zenoh_bridge)
echo "Building project..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_ROOT" -DCMAKE_INSTALL_PREFIX="$PACKAGE_ROOT"
make -j$(nproc)
make install

# Copy config file
echo "Copying config..."
cp "$PROJECT_ROOT/config/zenoh_config.json" "$PACKAGE_ROOT/"

# Copy run script
echo "Copying run script..."
cp "$PROJECT_ROOT/scripts/run.sh" "$PACKAGE_ROOT/"

# Create archive
echo "Creating archive..."
cd "$PROJECT_ROOT"
# Create the tarball containing zenoh_bridge folder
tar -czf "$OUTPUT_DIR/$PACKAGE_NAME" -C "$DIST_DIR" zenoh_bridge

echo "Done! Package created at '$OUTPUT_DIR/$PACKAGE_NAME'"
echo "Contents:"
tar -tf "$OUTPUT_DIR/$PACKAGE_NAME"
