# Zenoh Bridge 压测工具

本目录包含 Zenoh Bridge 的性能测试和压测相关代码。

## 目录结构

```
test/
├── README.md              # 本文件
├── include/               # 压测工具头文件
│   └── benchmark.h       # 压测框架定义
├── src/                   # 压测工具源代码
│   ├── benchmark.cpp     # 压测核心实现
│   ├── benchmark_pub.cpp # 压测发布工具
│   └── benchmark_recv.cpp# 压测接收工具
├── scripts/               # 测试脚本
│   └── run_benchmark_tests.sh  # 自动化测试套件
└── docs/                  # 测试文档
    ├── BENCHMARK.md       # 压测使用指南
    └── ROUTER_SETUP.md    # Router 配置指南
```

## 工具说明

### 1. benchmark_pub - 压测数据发布工具

**功能**: 模拟高频数据发布,用于测试 Zenoh 通信性能

**编译**:
```bash
cd /home/rx01239/code/zenoh_bridge/build
make benchmark_pub
```

**使用**:
```bash
./benchmark_pub [选项]

选项:
  -s, --size <bytes>        消息大小 (默认: 1024)
  -r, --rate <msg/s>        发送速率 (默认: 1000)
  -d, --duration <seconds>  测试时长 (默认: 10)
  -p, --publishers <num>    发布线程数 (默认: 1)
  -t, --topic <name>        Zenoh topic (默认: benchmark/data)
  -v, --verbose             详细输出
  -h, --help                显示帮助
```

**示例**:
```bash
# 基础测试: 1KB 消息, 1000 msg/s, 持续 5 秒
./benchmark_pub -s 1024 -r 1000 -d 5

# 高频测试: 10000 msg/s
./benchmark_pub -s 1024 -r 10000 -d 10 -v

# 大消息测试: 100KB 消息
./benchmark_pub -s 102400 -r 100 -d 10

# 多线程并发: 4 个发布者
./benchmark_pub -s 1024 -r 5000 -d 10 -p 4
```

### 2. benchmark_recv - UDP 接收性能监控

**功能**: 接收 UDP 数据并统计性能指标

**编译**:
```bash
cd /home/rx01239/code/zenoh_bridge/build
make benchmark_recv
```

**使用**:
```bash
./benchmark_recv <port>

参数:
  <port>  UDP 监听端口 (例如: 8888)
```

**示例**:
```bash
# 监听 8888 端口
./benchmark_recv 8888

# 输出示例:
# [Interim] 5.02s: 4998 msgs (4.88 MB) | 995.62 msg/s, 0.97 MB/s | Avg: 0.50ms, P99: 1.20ms | Drop: 0 (0.00%)
```

**输出指标**:
- **消息统计**: 接收消息数、总字节数
- **吞吐量**: 每秒消息数、每秒 MB 数
- **延迟**: 平均延迟、P99 延迟
- **丢包率**: 丢失消息数和比例

### 3. run_benchmark_tests.sh - 自动化测试套件

**功能**: 运行预定义的 6 个测试场景

**使用**:
```bash
cd /home/rx01239/code/zenoh_bridge
chmod +x test/scripts/run_benchmark_tests.sh
./test/scripts/run_benchmark_tests.sh
```

**测试场景**:
1. **基准测试**: 100 msg/s, 1KB 消息
2. **中等负载**: 1000 msg/s, 1KB 消息
3. **高频测试**: 5000 msg/s, 1KB 消息
4. **大消息测试**: 1000 msg/s, 10KB 消息
5. **极限并发**: 10000 msg/s, 4 线程
6. **吞吐量测试**: 50 MB/s

## 完整测试流程

### 步骤 1: 编译所有工具
```bash
cd /home/rx01239/code/zenoh_bridge
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 步骤 2: 启动接收器
```bash
# 终端 1: UDP 接收器
./benchmark_recv 8888
```

### 步骤 3: 启动数据桥
```bash
# 终端 2: Zenoh → UDP 转发桥
./data_bridge
```

### 步骤 4: 运行压测
```bash
# 终端 3: 压测发布
./benchmark_pub -s 1024 -r 1000 -d 10 -v
```

## 性能基准参考

基于 ARM64 (aarch64) Linux 测试:

| 场景 | 消息大小 | 速率 | 吞吐量 | 延迟 | 丢包率 |
|------|---------|------|--------|------|--------|
| 轻负载 | 1KB | 100 msg/s | 0.1 MB/s | <0.5ms | 0% |
| 正常负载 | 1KB | 1000 msg/s | 1 MB/s | <1ms | 0% |
| 高频负载 | 1KB | 5000 msg/s | 5 MB/s | <2ms | <1% |
| 大消息 | 10KB | 1000 msg/s | 10 MB/s | <5ms | 0% |
| 极限并发 | 1KB | 10000 msg/s | 10 MB/s | <5ms | <5% |

## 相关文档

- [BENCHMARK.md](docs/BENCHMARK.md) - 详细的压测使用指南
- [ROUTER_SETUP.md](docs/ROUTER_SETUP.md) - Zenoh Router 配置说明
- [../README.md](../README.md) - 项目总体介绍

## 故障排查

### 问题 1: 编译失败
```bash
# 检查 Zenoh 库路径
ls -la deps/zenoh/lib/aarch64/  # 或 x86_64/

# 清理重新编译
cd build && rm -rf * && cmake .. && make -j$(nproc)
```

### 问题 2: 无法接收数据
```bash
# 检查 data_bridge 是否运行
ps aux | grep data_bridge

# 检查端口占用
ss -tuln | grep 8888

# 查看日志
tail -f /tmp/data_bridge.log
```

### 问题 3: 性能不达预期
- 检查系统负载: `top` 或 `htop`
- 调整 UDP 缓冲区: `sysctl net.core.rmem_max`
- 使用 P2P 模式而非 Router 模式(延迟更低)

## 开发指南

### 添加新的测试场景

编辑 `test/scripts/run_benchmark_tests.sh`:
```bash
# Test 7: 你的测试场景
run_benchmark "Your Test" <size> <rate> <duration> <publishers>
```

### 扩展性能指标

修改 `test/include/benchmark.h` 和 `test/src/benchmark.cpp`:
```cpp
// 添加新的统计字段
struct Statistics {
    // ... 现有字段
    size_t your_new_metric;
};
```

### 自定义输出格式

修改 `test/src/benchmark_recv.cpp` 中的 `printInterimStats()` 和 `printFinalStats()` 函数。

## 贡献

如果你发现性能问题或想添加新的测试场景,欢迎提交 Issue 或 Pull Request。
