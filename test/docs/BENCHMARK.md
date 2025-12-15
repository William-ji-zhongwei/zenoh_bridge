# Zenoh Bridge 性能压测工具

## 概述

这是一套完整的性能压测工具，用于测试 Zenoh 数据桥接程序在高频、高吞吐量场景下的性能表现。

## 工具组件

### 1. benchmark_pub - 基准测试发布器
高性能的 Zenoh 数据发布工具，支持：
- 可配置的消息大小和发送频率
- 多线程并发发布
- 精确的速率控制
- 延迟测量（端到端）

### 2. benchmark_recv - 基准测试接收器
UDP 接收端测试工具，用于测量：
- 实际接收速率
- 数据吞吐量
- 端到端延迟
- 丢包率

### 3. data_bridge - 数据桥接服务
被测试的核心组件，从 Zenoh 接收数据并转发到 UDP。

## 快速开始

### 基础测试

**终端 1 - 启动接收器：**
```bash
./build/benchmark_recv 8888
```

**终端 2 - 启动桥接服务：**
```bash
./build/data_bridge
```

**终端 3 - 运行压测：**
```bash
# 1KB 消息，1000 msg/s，持续 10 秒
./build/benchmark_pub -s 1024 -r 1000 -d 10
```

## 使用说明

### benchmark_pub 参数

```bash
./build/benchmark_pub [options]

选项:
  -t <topic>        Zenoh topic (默认: benchmark/data)
  -s <size>         消息大小（字节）(默认: 1024)
  -r <rate>         消息速率（msg/s）(默认: 1000)
  -d <duration>     测试时长（秒）(默认: 10)
  -p <publishers>   并发发布者数量 (默认: 1)
  -v                详细输出
  -h                显示帮助
```

### benchmark_recv 参数

```bash
./build/benchmark_recv [port]

参数:
  port              UDP 监听端口 (默认: 8888)
```

## 压测场景

### 场景 1：低吞吐量基线测试
```bash
# 1KB @ 100 msg/s
./build/benchmark_pub -s 1024 -r 100 -d 10
```
**预期结果：** 近乎 0 丢包，低延迟（<1ms）

### 场景 2：中等吞吐量测试
```bash
# 1KB @ 1000 msg/s = ~1 MB/s
./build/benchmark_pub -s 1024 -r 1000 -d 10
```
**预期结果：** <1% 丢包，延迟 <5ms

### 场景 3：高消息频率测试
```bash
# 256B @ 5000 msg/s = ~1.25 MB/s
./build/benchmark_pub -s 256 -r 5000 -d 10
```
**目的：** 测试高频小消息处理能力

### 场景 4：大消息测试
```bash
# 10KB @ 500 msg/s = ~5 MB/s
./build/benchmark_pub -s 10240 -r 500 -d 10
```
**目的：** 测试大数据包处理能力

### 场景 5：超高吞吐量测试
```bash
# 1KB @ 10000 msg/s with 4 publishers = ~10 MB/s
./build/benchmark_pub -s 1024 -r 10000 -p 4 -d 15
```
**目的：** 压测系统极限吞吐量

### 场景 6：极限压力测试
```bash
# 10KB @ 5000 msg/s with 4 publishers = ~50 MB/s
./build/benchmark_pub -s 10240 -r 5000 -p 4 -d 20
```
**目的：** 找到系统瓶颈

## 自动化测试套件

运行完整的测试套件：

```bash
./scripts/benchmark_test.sh
```

测试套件包含 6 个预定义场景，从低到高逐步增加压力。

## 性能指标

### 关键指标

1. **消息速率** (Messages/sec)
   - 实际处理的消息数量
   - 与目标速率的对比

2. **吞吐量** (MB/s)
   - 数据传输速率
   - 网络带宽利用率

3. **延迟** (Latency)
   - 平均延迟：端到端数据传输时间
   - P99 延迟：99% 消息的延迟上限

4. **丢包率**
   - 发送但未接收的消息百分比
   - 系统过载的指标

### 示例输出

```
========== Performance Report ==========
Duration:          10.00 seconds
Total Messages:    10000
Total Bytes:       10.00 MB
Dropped Messages:  0
Messages/sec:      1000.00
Throughput:        1.00 MB/s

Latency Statistics:
  Average:         0.52 ms
  P99:             1.23 ms
========================================
```

## 性能优化建议

### 系统调优

1. **增加 UDP 缓冲区：**
```bash
sudo sysctl -w net.core.rmem_max=26214400
sudo sysctl -w net.core.rmem_default=26214400
```

2. **检查网络接口：**
```bash
ifconfig
ethtool -S <interface>
```

3. **监控 CPU 和内存：**
```bash
htop
```

### 程序优化

- 使用多个发布者线程（`-p` 参数）
- 调整消息大小以优化网络利用率
- 考虑使用 Zenoh 的共享内存功能

## 故障排查

### 高丢包率

**可能原因：**
- UDP 缓冲区太小
- 接收端处理速度不够
- 网络拥塞

**解决方案：**
- 增加系统 UDP 缓冲区
- 降低发送速率
- 优化接收端代码

### 高延迟

**可能原因：**
- CPU 过载
- 上下文切换过多
- 网络延迟

**解决方案：**
- 减少并发发布者数量
- 使用 CPU 亲和性绑定
- 检查网络质量

### 吞吐量未达预期

**可能原因：**
- Zenoh 配置不当
- 系统资源限制
- 网络带宽限制

**解决方案：**
- 检查 Zenoh 模式（peer vs router）
- 监控系统资源使用
- 测试网络带宽上限

## 典型测试结果

### 本地测试（Localhost）

| 场景 | 消息大小 | 目标速率 | 实际速率 | 吞吐量 | 平均延迟 | 丢包率 |
|------|----------|----------|----------|--------|----------|--------|
| 低负载 | 1KB | 100 msg/s | 100 msg/s | 0.1 MB/s | <0.5ms | 0% |
| 中负载 | 1KB | 1000 msg/s | 999 msg/s | 1.0 MB/s | ~1ms | <0.1% |
| 高频小包 | 256B | 5000 msg/s | 4980 msg/s | 1.2 MB/s | ~2ms | <1% |
| 大包 | 10KB | 500 msg/s | 500 msg/s | 5.0 MB/s | ~3ms | 0% |
| 超高吞吐 | 1KB | 10k msg/s | 9.8k msg/s | 9.8 MB/s | ~5ms | ~2% |

*注：实际结果取决于硬件配置和系统负载*

### 跨网络测试

跨网络测试时，延迟和丢包率会显著增加，建议：
- 降低目标速率
- 增加测试时长以获得稳定结果
- 监控网络质量（ping, iperf）

## 扩展测试

### 长时间稳定性测试

```bash
# 运行 1 小时稳定性测试
./build/benchmark_pub -s 1024 -r 1000 -d 3600
```

### 变化负载测试

逐步增加负载，观察系统行为：
```bash
for rate in 100 500 1000 2000 5000 10000; do
    echo "Testing at $rate msg/s"
    ./build/benchmark_pub -s 1024 -r $rate -d 10
    sleep 5
done
```

### 多 Topic 测试

修改 data_bridge 配置支持多个 topic，并行发送：
```bash
# 终端 1
./build/benchmark_pub -t benchmark/stream1 -s 1024 -r 1000 -d 30 &

# 终端 2
./build/benchmark_pub -t benchmark/stream2 -s 1024 -r 1000 -d 30 &
```

## 结论

这套压测工具可以：
- ✅ 验证系统在高负载下的稳定性
- ✅ 测量实际性能指标
- ✅ 发现系统瓶颈
- ✅ 优化配置参数
- ✅ 对比不同配置的性能

建议在部署前进行充分的压测，确保系统满足实际需求。
