#!/bin/bash
set -e

# Get the project root directory (one level up from scripts/)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(realpath "$SCRIPT_DIR/..")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Building Project ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_ROOT"
make -j$(nproc)
echo "=== Build Complete ==="
