# Zenoh 数据接收桥接 (Zenoh Data Receiver Bridge)

## 概述

这是一个基于 Zenoh 的数据接收桥接程序，用于从云端（Zenoh）接收数据并转发到本地服务。

## 核心功能

- **从 Zenoh 接收数据**：订阅配置的 Zenoh topic
- **多协议转发**：支持 UDP 和 gRPC（gRPC 待实现）
- **灵活配置**：支持多个数据流，每个流可配置独立的 topic 和协议

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                        云端 (Cloud)                          │
│                   通过 Zenoh 发送数据                         │
└──────────────────────┬──────────────────────────────────────┘
                       │
                    Zenoh
                       │
┌──────────────────────┴──────────────────────────────────────┐
│                 Data Receiver Bridge                        │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │  订阅多个 Zenoh Topics                              │    │
│  │  - vr/robot/control                                │    │
│  │  - vr/robot/command                                │    │
│  │  - ...                                             │    │
│  └────────┬───────────────────────────────────────────┘    │
│           │                                                 │
│           ├─→ UDP Forwarder  ──→ UDP:8888                  │
│           ├─→ UDP Forwarder  ──→ UDP:8889                  │
│           └─→ gRPC Forwarder ──→ gRPC Service (TODO)       │
│                                                              │
└──────────────────────────────────────────────────────────────┘
                       │
                       ↓
            本地服务 (UDP/gRPC)
```

## 配置说明

### 配置文件格式 (JSON)

```json
{
  "zenoh_mode": "client",
  "zenoh_connect": "",
  "streams": [
    {
      "zenoh_topic": "vr/robot/control",
      "protocol": "udp",
      "local_host": "127.0.0.1",
      "local_port": 8888,
      "grpc_service": "",
      "grpc_method": ""
    }
  ]
}
```

### 配置字段说明

- **zenoh_mode**: Zenoh 模式 (`client` 或 `peer`)
- **zenoh_connect**: Zenoh 连接地址（空字符串表示 peer 模式）
- **streams**: 数据流配置数组
  - **zenoh_topic**: 订阅的 Zenoh topic
  - **protocol**: 本地转发协议 (`udp` 或 `grpc`)
  - **local_host**: 本地目标主机
  - **local_port**: 本地目标端口
  - **grpc_service**: gRPC 服务名（仅 gRPC 协议）
  - **grpc_method**: gRPC 方法名（仅 gRPC 协议）

## 编译

```bash
# 清理并构建
cd /home/rx01239/code/zenoh_bridge
rm -rf build && mkdir build
cd build
cmake ..
make -j$(nproc)
```

## 运行

### 使用默认配置
```bash
./build/data_bridge
```

### 使用配置文件
```bash
./build/data_bridge config/bridge_config.json
```

## 测试

### 功能测试 - UDP 转发

1. **启动接收端**（监听 UDP 8888 端口）：
```bash
nc -ul 8888
```

2. **启动 data_bridge**：
```bash
./build/data_bridge
```

3. **发送测试数据**（使用 zenoh_pub）：
```bash
echo "test control data" | ./build/zenoh_pub vr/robot/control
```

4. 查看 nc 是否收到数据

### 性能测试 - 压测工具

完整的压测工具和文档位于 `test/` 目录:

```bash
# 查看测试工具文档
cat test/README.md

# 运行完整测试套件
./test/scripts/run_benchmark_tests.sh

# 手动压测示例
./build/benchmark_pub -s 1024 -r 1000 -d 10 -v
```

详细的性能测试指南请参考:
- [test/README.md](test/README.md) - 测试工具使用说明
- [test/docs/BENCHMARK.md](test/docs/BENCHMARK.md) - 压测详细指南
- [test/docs/ROUTER_SETUP.md](test/docs/ROUTER_SETUP.md) - Zenoh Router 配置

## 支持的协议

### UDP ✅
- 已实现
- 支持单向数据转发
- 低延迟

### gRPC ⏳
- 待实现
- 需要集成 gRPC C++ SDK
- 支持双向通信

## 文件结构

```
zenoh_bridge/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── common.h              # 配置结构定义
│   └── receiver_bridge.h     # 接收桥接主模块
├── src/
│   ├── common.cpp            # 配置实现
│   ├── receiver_bridge.cpp   # 接收桥接实现
│   ├── vr_bridge.cpp         # 主程序
│   ├── publisher.cpp         # Zenoh 发布示例
│   └── subscriber.cpp        # Zenoh 订阅示例
├── test/                     # 性能测试和压测工具
│   ├── README.md             # 测试工具使用说明
│   ├── include/
│   │   └── benchmark.h       # 压测框架头文件
│   ├── src/
│   │   ├── benchmark.cpp     # 压测核心实现
│   │   ├── benchmark_pub.cpp # 压测发布工具
│   │   └── benchmark_recv.cpp# 压测接收工具
│   ├── scripts/
│   │   └── run_benchmark_tests.sh  # 自动化测试套件
│   └── docs/
│       ├── BENCHMARK.md      # 压测使用指南
│       └── ROUTER_SETUP.md   # Router 配置说明
├── config/
│   ├── bridge_config.json    # 主程序配置示例
│   └── benchmark_config.json # 压测配置示例
├── scripts/
│   ├── build.sh              # 编译脚本
│   ├── package.sh            # 打包脚本
│   └── run.sh                # 运行脚本
├── deps/
│   └── zenoh/                # Zenoh C/C++ 库
├── build/                    # 编译输出目录
│   ├── data_bridge           # 主程序
│   ├── zenoh_pub             # Zenoh 发布工具
│   ├── zenoh_sub             # Zenoh 订阅工具
│   ├── benchmark_pub         # 压测发布工具
│   └── benchmark_recv        # 压测接收工具
└── output/                   # 打包输出目录
```

## TODO 清单

### 高优先级
- [ ] 实现 JSON 配置文件解析（使用 nlohmann/json）
- [ ] 实现 gRPC 转发功能
- [ ] 添加数据验证和错误处理
- [ ] 添加重连机制

### 中优先级
- [ ] 添加日志系统
- [ ] 添加性能监控
- [ ] 支持数据压缩
- [ ] 添加配置热重载

### 低优先级
- [ ] 添加统计信息（吞吐量、延迟等）
- [ ] 添加单元测试
- [ ] 添加 Docker 支持

## 与原 VR 遥操作系统的区别

**之前**：
- 三个模块：控制接收、摄像头发布、反馈转发
- 双向通信（Zenoh ↔ 本地）
- 硬编码配置

**现在**：
- 单一模块：数据接收
- 单向通信（Zenoh → 本地）
- 灵活配置（支持多流、多协议）
- 更通用化的设计

## 使用示例

### 示例 1：单个 UDP 流
```json
{
  "streams": [
    {
      "zenoh_topic": "robot/control",
      "protocol": "udp",
      "local_host": "192.168.1.100",
      "local_port": 8888
    }
  ]
}
```

### 示例 2：多个流，混合协议
```json
{
  "streams": [
    {
      "zenoh_topic": "robot/control",
      "protocol": "udp",
      "local_host": "127.0.0.1",
      "local_port": 8888
    },
    {
      "zenoh_topic": "robot/telemetry",
      "protocol": "udp",
      "local_host": "127.0.0.1",
      "local_port": 8889
    },
    {
      "zenoh_topic": "robot/mission",
      "protocol": "grpc",
      "local_host": "127.0.0.1",
      "local_port": 50051,
      "grpc_service": "MissionService",
      "grpc_method": "UpdateMission"
    }
  ]
}
```
