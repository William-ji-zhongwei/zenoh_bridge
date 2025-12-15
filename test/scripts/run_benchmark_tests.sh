#!/bin/bash
# Zenoh Bridge Benchmark Test Script

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BUILD_DIR="$SCRIPT_DIR/../../build"

echo "=========================================="
echo "  Zenoh Bridge Benchmark Test Suite"
echo "=========================================="
echo ""

# Function to run a single benchmark test
run_benchmark() {
    local test_name=$1
    local msg_size=$2
    local msg_rate=$3
    local duration=$4
    local num_pubs=$5
    
    echo ">>> Test: $test_name"
    echo "    Message Size: ${msg_size} bytes"
    echo "    Target Rate: ${msg_rate} msg/s"
    echo "    Publishers: ${num_pubs}"
    echo "    Duration: ${duration}s"
    echo ""
    
    # Start data_bridge in background
    echo "[1/4] Starting data_bridge..."
    $BUILD_DIR/data_bridge > /tmp/data_bridge.log 2>&1 &
    BRIDGE_PID=$!
    sleep 2
    
    # Start benchmark receiver in background
    echo "[2/4] Starting benchmark receiver..."
    $BUILD_DIR/benchmark_recv 8888 > /tmp/benchmark_recv.log 2>&1 &
    RECV_PID=$!
    sleep 2
    
    # Run benchmark publisher
    echo "[3/4] Running benchmark publisher..."
    $BUILD_DIR/benchmark_pub -s $msg_size -r $msg_rate -d $duration -p $num_pubs
    
    # Wait a bit for data to flush
    sleep 2
    
    # Stop receiver and bridge
    echo "[4/4] Stopping services..."
    kill -SIGINT $RECV_PID 2>/dev/null || true
    sleep 1
    kill -SIGINT $BRIDGE_PID 2>/dev/null || true
    sleep 1
    
    # Show receiver stats
    echo ""
    echo "=== Receiver Statistics ==="
    tail -20 /tmp/benchmark_recv.log
    
    echo ""
    echo "----------------------------------------"
    echo ""
    sleep 2
}

# Test Suite

echo "Starting benchmark test suite..."
echo ""

# Test 1: Low throughput baseline (1KB @ 100 msg/s)
run_benchmark "Low Throughput Baseline" 1024 100 5 1

# Test 2: Medium throughput (1KB @ 1000 msg/s)
run_benchmark "Medium Throughput" 1024 1000 10 1

# Test 3: High message rate (256B @ 5000 msg/s)
run_benchmark "High Message Rate" 256 5000 10 1

# Test 4: Large messages (10KB @ 500 msg/s)
run_benchmark "Large Messages" 10240 500 10 1

# Test 5: Very high throughput (1KB @ 10000 msg/s with 4 publishers)
run_benchmark "Very High Throughput" 1024 10000 15 4

# Test 6: Extreme throughput (10KB @ 5000 msg/s with 4 publishers)
run_benchmark "Extreme Throughput" 10240 5000 20 4

echo "=========================================="
echo "  Benchmark Suite Complete!"
echo "=========================================="
echo ""
echo "Logs saved to:"
echo "  - /tmp/data_bridge.log"
echo "  - /tmp/benchmark_recv.log"
echo ""
