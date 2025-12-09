#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Detect environment
if [ -d "$SCRIPT_DIR/bin" ] && [ -f "$SCRIPT_DIR/zenoh_config.json" ]; then
    # Distribution environment
    BIN_DIR="$SCRIPT_DIR/bin"
    CONFIG_FILE="$SCRIPT_DIR/zenoh_config.json"
else
    # Development environment
    PROJECT_ROOT="$(realpath "$SCRIPT_DIR/..")"
    BIN_DIR="$PROJECT_ROOT/build"
    CONFIG_FILE="$PROJECT_ROOT/config/zenoh_config.json"
fi

if [ -z "$1" ]; then
    echo "Usage: $0 [pub|sub]"
    echo "Examples:"
    echo "  $0 pub   # Run Publisher"
    echo "  $0 sub   # Run Subscriber"
    exit 1
fi

MODE=$1
echo "=== Running $MODE ==="

if [ "$MODE" == "pub" ]; then
    if [ -f "$BIN_DIR/zenoh_pub" ]; then
        "$BIN_DIR/zenoh_pub" "$CONFIG_FILE"
    else
        echo "Error: zenoh_pub executable not found in $BIN_DIR."
        if [ ! -d "$SCRIPT_DIR/bin" ]; then
             echo "Please run scripts/build.sh first."
        fi
        exit 1
    fi
elif [ "$MODE" == "sub" ]; then
    if [ -f "$BIN_DIR/zenoh_sub" ]; then
        "$BIN_DIR/zenoh_sub" "$CONFIG_FILE"
    else
        echo "Error: zenoh_sub executable not found in $BIN_DIR."
        if [ ! -d "$SCRIPT_DIR/bin" ]; then
             echo "Please run scripts/build.sh first."
        fi
        exit 1
    fi
else
    echo "Error: Unknown mode '$MODE'"
    echo "Available modes: pub, sub"
    exit 1
fi
