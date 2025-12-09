#!/bin/bash

# Exit on error
set -e

# Directory setup
BUILD_DIR="build"
DIST_DIR="dist"

echo "Cleaning previous distribution..."
rm -rf $DIST_DIR
mkdir -p $DIST_DIR

# Build and Install (to dist/)
echo "Building project..."
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake .. -DCMAKE_INSTALL_PREFIX=../$DIST_DIR
make -j$(nproc)
make install
cd ..

# Copy config file
cp zenoh_config.json $DIST_DIR/

echo "Done! Distribution package is in '$DIST_DIR/'"
echo "To run on another machine:"
echo "  cd dist/bin"
echo "  ./zenoh_pub"
