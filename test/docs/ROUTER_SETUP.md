# Zenoh Router 测试配置指南

## 当前状态: Peer-to-Peer 模式

当前测试使用的是 **本地 P2P 模式**,数据流:
```
benchmark_pub → Zenoh (本地直连) → data_bridge → UDP → benchmark_recv
```

- ✅ 优点: 低延迟,无需额外组件,自动发现
- ⚠️ 缺点: 仅限本地通信,无法跨主机测试

---

## 如何切换到 Router 模式

### 架构对比

#### P2P 模式 (当前)
```
App1 (publisher) ←→ App2 (subscriber)
     [本地共享内存/socket直连]
```

#### Router 模式
```
App1 (publisher) → Router → App2 (subscriber)
                  ↑
                7447端口
```

---

## 步骤 1: 安装并启动 Zenoh Router

### 方法 A: 使用预编译二进制 (推荐)

```bash
cd /home/rx01239/code/zenoh_bridge

# 下载 zenohd (根据系统架构选择)
# ARM64 (aarch64):
wget https://github.com/eclipse-zenoh/zenoh/releases/download/0.11.0/zenohd-0.11.0-aarch64-unknown-linux-gnu.zip
unzip zenohd-0.11.0-aarch64-unknown-linux-gnu.zip

# x86_64:
# wget https://github.com/eclipse-zenoh/zenoh/releases/download/0.11.0/zenohd-0.11.0-x86_64-unknown-linux-gnu.zip
# unzip zenohd-0.11.0-x86_64-unknown-linux-gnu.zip

chmod +x zenohd
```

### 方法 B: 使用 Cargo 安装

```bash
cargo install zenoh --version 0.11.0 --features zenohd
```

### 启动 Router

```bash
# 前台运行 (查看日志)
./zenohd

# 或后台运行
./zenohd &

# 验证端口监听
ss -tuln | grep 7447
# 应该看到: tcp   LISTEN 0   128   0.0.0.0:7447   0.0.0.0:*
```

---

## 步骤 2: 修改客户端配置

### 方式 A: 代码级别配置 (推荐用于测试)

修改 `src/benchmark.cpp` 和 `src/receiver_bridge.cpp`:

```cpp
// 当前代码 (P2P模式):
zenoh::Config zenoh_config = zenoh::Config::create_default();

// 修改为 (Router模式):
zenoh::Config zenoh_config = zenoh::Config::create_default();
zenoh_config.insert_json5("mode", "\"client\"").unwrap();
zenoh_config.insert_json5("connect/endpoints", "[\"tcp/127.0.0.1:7447\"]").unwrap();
```

**需要修改的文件:**
1. `src/benchmark.cpp` (BenchmarkPublisher::publishLoop, 第145行)
2. `src/receiver_bridge.cpp` (ReceiverBridge构造函数, 第15行)

### 方式 B: JSON 配置文件 (需要实现解析)

`config/benchmark_config.json` 已更新:
```json
{
  "zenoh_mode": "client",
  "zenoh_connect": "tcp/127.0.0.1:7447",  ← 连接到本地router
  ...
}
```

**需要完成**: 实现 `BridgeConfig::loadFromFile()` 中的 JSON 解析

---

## 步骤 3: 代码修改示例

### 文件: `src/benchmark.cpp`

```cpp
void BenchmarkPublisher::publishLoop(int publisher_id) {
    try {
        // 配置 Zenoh 连接 Router
        zenoh::Config zenoh_config = zenoh::Config::create_default();
        
        // 关键修改: 设置为 client 模式并连接到 router
        zenoh_config.insert_json5("mode", "\"client\"").unwrap();
        zenoh_config.insert_json5("connect/endpoints", "[\"tcp/127.0.0.1:7447\"]").unwrap();
        
        auto session = zenoh::Session::open(std::move(zenoh_config));
        auto publisher = session.declare_publisher(config_.zenoh_topic);
        
        // ... 后续代码不变
```

### 文件: `src/receiver_bridge.cpp`

```cpp
ReceiverBridge::ReceiverBridge(const BridgeConfig& config)
    : config_(config) {
    
    // 配置 Zenoh 连接 Router
    zenoh::Config zenoh_config = zenoh::Config::create_default();
    
    // 关键修改: 设置为 client 模式并连接到 router
    zenoh_config.insert_json5("mode", "\"client\"").unwrap();
    zenoh_config.insert_json5("connect/endpoints", "[\"tcp/127.0.0.1:7447\"]").unwrap();
    
    session_ = zenoh::Session::open(std::move(zenoh_config));
}
```

---

## 步骤 4: 重新编译并测试

```bash
cd /home/rx01239/code/zenoh_bridge/build
make -j$(nproc)

# 启动 router (另一个终端)
cd /home/rx01239/code/zenoh_bridge
./zenohd

# 运行测试
./benchmark_recv 8888 &
sleep 1
./data_bridge &
sleep 1
./benchmark_pub -s 1024 -r 1000 -d 5 -v
```

---

## 验证 Router 是否生效

### 1. 检查 Router 日志
```bash
# zenohd 输出应该显示:
[INFO] zenoh::net::runtime: Zenoh router started
[INFO] New session opened: <session_id>
```

### 2. 查看网络流量
```bash
# 监控 7447 端口流量
sudo tcpdump -i lo port 7447 -n
# 应该看到 TCP 连接和数据传输
```

### 3. 性能对比

| 模式 | 延迟 | 吞吐量 | 适用场景 |
|------|------|--------|----------|
| **P2P** | <1ms | >100k msg/s | 本地测试,单机部署 |
| **Router** | 1-2ms | 50k-100k msg/s | 跨主机,分布式系统 |

---

## 跨主机测试 (可选)

### 场景: 发布端和订阅端在不同机器

#### 机器 A (Router + data_bridge):
```bash
# 启动 router
./zenohd --listen tcp/0.0.0.0:7447

# 启动 data_bridge
./data_bridge
```

#### 机器 B (benchmark_pub):
```bash
# 修改连接地址为机器A的IP
zenoh_config.insert_json5("connect/endpoints", "[\"tcp/192.168.1.100:7447\"]").unwrap();

./benchmark_pub -s 1024 -r 1000 -d 10
```

---

## 故障排查

### 问题 1: 无法连接到 router
```bash
# 检查 router 是否运行
ps aux | grep zenohd

# 检查端口监听
ss -tuln | grep 7447

# 检查防火墙
sudo ufw allow 7447/tcp
```

### 问题 2: 性能下降
- **原因**: Router 引入额外的序列化/网络传输开销
- **解决**: 
  - 使用 SHM (共享内存) 传输: `zenoh_config.insert_json5("transport/shared_memory/enabled", "true")`
  - 调整批处理大小

### 问题 3: API 调用失败
```cpp
// 错误处理示例
auto result = zenoh_config.insert_json5("mode", "\"client\"");
if (!result) {
    std::cerr << "Failed to configure Zenoh mode" << std::endl;
}
```

---

## 总结

✅ **当前**: P2P 模式,本地直连,低延迟  
🔄 **切换**: 启动 `zenohd` + 修改代码中的 `zenoh::Config`  
🌐 **生产**: Router 模式,支持跨主机,云边协同

需要我帮你实现 Router 模式的代码修改吗?
