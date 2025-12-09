#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(realpath "$SCRIPT_DIR/..")"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist"
OUTPUT_DIR="$PROJECT_ROOT/output"

# Generate timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
PACKAGE_NAME="zenoh_project_package_${TIMESTAMP}.tar.gz"

echo "Cleaning previous distribution..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$OUTPUT_DIR"

# Build and Install (to dist/)
echo "Building project..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_ROOT" -DCMAKE_INSTALL_PREFIX="$DIST_DIR"
make -j$(nproc)
make install

# Copy config file
echo "Copying config..."
cp "$PROJECT_ROOT/config/zenoh_config.json" "$DIST_DIR/"

# Copy run script
echo "Copying run script..."
cp "$PROJECT_ROOT/scripts/run.sh" "$DIST_DIR/"

# Create archive
echo "Creating archive..."
cd "$PROJECT_ROOT"
# Create the tarball from the contents of dist/
tar -czf "$OUTPUT_DIR/$PACKAGE_NAME" -C "$DIST_DIR" .

echo "Done! Package created at '$OUTPUT_DIR/$PACKAGE_NAME'"
echo "Contents:"
tar -tf "$OUTPUT_DIR/$PACKAGE_NAME"
