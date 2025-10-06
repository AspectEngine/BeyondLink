# BeyondLink - Beyond 激光实时可视化系统

<div align="center">

**一个用于接收和可视化 Beyond 激光数据的高性能 DirectX 11 应用程序**

[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Graphics API: DirectX 11](https://img.shields.io/badge/Graphics-DirectX%2011-green.svg)](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)

</div>

---

## 📖 目录

- [项目简介](#-项目简介)
- [主要特性](#-主要特性)
- [系统要求](#-系统要求)
- [快速开始](#-快速开始)
- [项目结构](#-项目结构)
- [使用指南](#-使用指南)
- [Beyond 配置](#-beyond-配置)
- [架构说明](#-架构说明)
- [配置选项](#-配置选项)
- [故障排查](#-故障排查)
- [技术细节](#-技术细节)
- [许可证](#-许可证)

---

## 🎯 项目简介

**BeyondLink** 是一个专为 **Pangolin Beyond** 激光控制软件设计的实时可视化工具。它通过 UDP 多播接收 Beyond 输出的激光数据，使用 DirectX 11 进行高性能渲染，并支持：

- ✅ **多设备支持**：同时可视化最多 4 台激光设备
- ✅ **实时渲染**：低延迟、高帧率的激光效果预览
- ✅ **扫描仪模拟**：真实模拟激光扫描仪的物理特性
- ✅ **网络协议兼容**：完全兼容 Beyond 的多播协议
- ✅ **动态设备切换**：实时切换不同设备的预览

---

## ✨ 主要特性

### 🎨 渲染能力
- **DirectX 11 硬件加速**：利用 GPU 实现高性能点云渲染
- **HDR 纹理支持**：使用 `R16G16B16A16_FLOAT` 格式保证颜色精度
- **加法混合模式**：真实模拟激光的叠加效果
- **Mipmap 生成**：支持多级纹理细化
- **亮度增强**：5倍亮度后处理，模拟激光的高亮效果

### 🌐 网络功能
- **UDP 多播接收**：自动加入 155 个多播组（设备 0-4，子网 0-30）
- **IP_PKTINFO 地址识别**：使用 Windows Sockets 扩展功能 `WSARecvMsg` 和 `IP_PKTINFO`，准确识别数据包的目标多播地址
- **设备 ID 自动提取**：从目标地址第三字节提取设备 ID（`239.255.X.Y` → 设备 X）
- **高效数据解析**：集成 `linetD2_x64.dll` 进行 Pangolin 协议解析
- **多线程接收**：独立的网络接收线程，不阻塞主渲染循环

### 🔧 扫描仪模拟
- **速度平滑**：模拟扫描仪的惯性和加速特性（指数移动平均）
- **边缘淡化**：根据扫描速度动态调整亮度
- **点插值**：自动增加点密度，提升渲染质量
- **光束检测**：识别并增强静止光束效果
- **质量级别**：支持 Low/Medium/High/Ultra 四档质量设置

### 🖥️ 用户交互
- **实时设备切换**：按下 `0-3` 键即可切换设备预览
- **状态监控**：每 5 秒输出网络和设备状态报告
- **FPS 显示**：窗口标题实时显示渲染帧率
- **错误诊断**：详细的错误信息和警告提示

### 📝 代码质量
- **专业级注释**：所有头文件和源文件都有详细的中文注释（1200+ 行）
- **模块化设计**：清晰的分层架构，易于理解和维护
- **完整文档**：包含函数说明、参数注解、返回值说明和实现细节
- **教学友好**：适合学习 DirectX 11、网络编程和激光可视化技术

---

## 💻 系统要求

### 最低配置
- **操作系统**：Windows 10/11 (64-bit)
- **CPU**：支持 SSE2 的双核处理器
- **GPU**：支持 DirectX 11 Feature Level 11.0
- **内存**：2 GB RAM
- **网络**：千兆以太网（推荐）

### 推荐配置
- **操作系统**：Windows 10/11 (64-bit)
- **CPU**：四核处理器或更高
- **GPU**：支持 DirectX 11.1，独立显卡（NVIDIA/AMD）
- **内存**：4 GB RAM 或更高
- **网络**：千兆以太网

### 开发环境
- **Visual Studio 2022** (或 Rider)
- **Windows SDK 10.0** 或更高
- **C++17 标准支持**
- **vcpkg**（可选，用于包管理）

---

## 🚀 快速开始

### 1. 克隆或下载项目

```bash
git clone https://github.com/AspectEngine/BeyondLink.git
cd BeyondLink
```

### 2. 准备依赖文件

确保项目根目录下有以下 DLL 文件：

```
BeyondLink/
├── bin/
│   ├── linetD2_x64.dll    # Beyond 协议解析库
│   └── matrix64.dll        # 数学运算库
```

> **注意**：这些 DLL 文件来自 Beyond 软件或相关 SDK。

### 3. 打开并编译项目

1. 双击打开 `BeyondLink.sln`
2. 选择 **Debug | x64** 或 **Release | x64** 配置
3. 按 `Ctrl+Shift+B` 或点击 **生成 → 生成解决方案**

### 4. 运行程序

编译完成后，运行：

```
Build\Binaries\Debug\BeyondLink.exe
```

或

```
Build\Binaries\Release\BeyondLink.exe
```

### 5. 配置 Beyond 输出

在 Beyond 软件中进行以下配置：

1. **启用外部可视化**：
   - 打开 **View（查看）** 菜单
   - 勾选 **Enable visualization through via external software**

2. **设置 Fixture Number**：
   - 打开投影区域（Projection Zone）设置
   - 在 **External visualization** 部分，设置 **Fixture number** = `1`（对应 BeyondLink 设备 0）

3. **播放内容**：
   - 在 Beyond 中播放激光内容
   - 在 BeyondLink 窗口中应该能看到实时可视化

详细配置请参见 [Beyond 配置](#-beyond-配置) 章节。

---

### 💡 代码学习指南

BeyondLink 的代码有非常详细的中文注释，适合学习以下技术：

#### 网络编程
- **文件**：`Source/LaserProtocol.cpp`（575 行）
- **学习内容**：
  - UDP 多播（`IP_ADD_MEMBERSHIP`）
  - Windows Sockets 扩展（`WSARecvMsg`, `IP_PKTINFO`）
  - 多线程网络接收
  - DLL 动态加载和函数调用

#### DirectX 11 渲染
- **文件**：`Source/LaserRenderer.cpp`（523 行）、`Source/LaserWindow.cpp`（756 行）
- **学习内容**：
  - D3D11 设备和上下文创建
  - 渲染目标纹理（RTT）和着色器资源视图（SRV）
  - 顶点缓冲和动态更新（`D3D11_MAP_WRITE_DISCARD`）
  - HLSL 着色器编译
  - SwapChain 和后缓冲管理
  - 点云渲染（`POINTLIST` 拓扑）

#### 算法实现
- **文件**：`Source/LaserSource.cpp`（370 行）
- **学习内容**：
  - 点插值（线性插值）
  - 速度平滑（指数移动平均）
  - 边缘淡化（速度依赖亮度）
  - 降采样（质量级别控制）
  - 光束检测（连续重复点识别）

#### 系统架构
- **文件**：`Source/Main.cpp`（272 行）、`Source/BeyondLink.cpp`（287 行）
- **学习内容**：
  - 模块化设计和分层架构
  - 资源管理和生命周期
  - 回调机制和数据流
  - 异常处理和错误诊断

**推荐阅读顺序**：
1. `Main.cpp` - 了解程序整体流程
2. `LaserProtocol.cpp` - 学习网络接收和IP_PKTINFO
3. `LaserSource.cpp` - 学习数据处理算法
4. `LaserRenderer.cpp` - 学习DirectX 11渲染
5. `LaserWindow.cpp` - 学习窗口管理和显示

---

## 📁 项目结构

```
BeyondLink/
├── include/                    # 头文件（含详细中文注释）
│   ├── BeyondLink.h           # 主系统接口（188 行）
│   ├── LaserPoint.h           # 激光点数据结构（111 行，28字节对齐）
│   ├── LaserProtocol.h        # 网络协议处理（258 行，IP_PKTINFO）
│   ├── LaserRenderer.h        # DirectX 11 渲染器（258 行）
│   ├── LaserSettings.h        # 配置参数（86 行）
│   ├── LaserSource.h          # 激光源数据处理（237 行，扫描仪模拟）
│   └── LaserWindow.h          # 显示窗口（226 行）
│
├── Source/                     # 源文件（含详细实现注释）
│   ├── BeyondLink.cpp         # 主系统实现（287 行）
│   ├── LaserProtocol.cpp      # 网络接收和解析（575 行，WSARecvMsg）
│   ├── LaserRenderer.cpp      # 渲染管线（523 行，D3D11 Pipeline）
│   ├── LaserSource.cpp        # 扫描仪模拟（370 行，插值/平滑/淡化）
│   ├── LaserWindow.cpp        # 窗口管理（756 行，SwapChain/FPS）
│   └── Main.cpp               # 程序入口点（272 行，主循环）
│
├── bin/                        # 依赖 DLL
│   ├── linetD2_x64.dll        # Beyond 协议解析（必需）
│   └── matrix64.dll            # 数学运算库（必需）
│
├── Build/                      # 构建输出目录
│   ├── Binaries/              # 可执行文件（Debug/Release）
│   └── Intermediate/          # 中间文件（.obj, .pdb）
│
├── BeyondLink.sln             # Visual Studio 解决方案
├── BeyondLink.vcxproj         # 项目文件（C++17, x64）
└── README.md                  # 本文档（560 行）
```

**代码注释说明**：
- ✅ 所有头文件和源文件均有详细的中文注释
- ✅ 注释总量超过 **1200 行**，包含文件级、类级、函数级注释
- ✅ 每个函数都有参数说明、返回值说明和实现细节
- ✅ 关键算法有逐步说明（如扫描仪模拟、IP_PKTINFO 提取）
- ✅ 适合学习和二次开发

---

## 📚 使用指南

### 启动程序

运行 `BeyondLink.exe` 后，程序将：

1. **初始化 DirectX 11 设备**
2. **加载 linetD2_x64.dll** 解析库
3. **创建 4 个激光源**（设备 0-3）
4. **绑定到 UDP 端口 5568**
5. **加入 155 个多播组**
6. **打开 512x512 预览窗口**

### 键盘控制

| 按键 | 功能 |
|------|------|
| `0` | 切换到设备 0（多播地址 `239.255.0.x`） |
| `1` | 切换到设备 1（多播地址 `239.255.1.x`） |
| `2` | 切换到设备 2（多播地址 `239.255.2.x`） |
| `3` | 切换到设备 3（多播地址 `239.255.3.x`） |
| `ESC` | 退出程序 |

### 状态报告

程序每 5 秒输出一次状态报告：

```
=== Status Report ===
Network: 5234 packets | 7680152 bytes | FPS: 59

All Devices Status:
>>> Device 0 (239.255.0.x): [OK] 2220 points <- VIEWING
    Device 1 (239.255.1.x): [OK] 1856 points
    Device 2 (239.255.2.x): [--] 0 points
    Device 3 (239.255.3.x): [--] 0 points
===================
```

- **`[OK]`**：设备有数据
- **`[--]`**：设备无数据
- **`<- VIEWING`**：当前正在查看的设备

---

## 🎛️ Beyond 配置

### 配置步骤（重要）

#### 步骤 1：启用外部可视化

1. 在 Beyond 菜单栏中，打开 **View（查看）** 菜单
2. 勾选 **Enable visualization through via external software**
3. 这将启用 Beyond 向外部程序发送激光数据

#### 步骤 2：配置投影区域（Projection Zone）

为每个投影区域配置 `Fixture number`，该编号对应 BeyondLink 中的设备 ID：

##### 投影区域 1 → 设备 0
1. 在 Beyond 中打开 **投影区域 1（Projection Zone 1）** 的设置
2. 找到 **External visualization** 部分
3. 设置 **Fixture number** = `1`（对应设备 0）

##### 投影区域 2 → 设备 1
1. 打开 **投影区域 2（Projection Zone 2）** 的设置
2. 设置 **Fixture number** = `2`（对应设备 1）

##### 投影区域 3 → 设备 2
1. 打开 **投影区域 3（Projection Zone 3）** 的设置
2. 设置 **Fixture number** = `3`（对应设备 2）

##### 投影区域 4 → 设备 3
1. 打开 **投影区域 4（Projection Zone 4）** 的设置
2. 设置 **Fixture number** = `4`（对应设备 3）

**重要提示**：

`Fixture number` 与 BeyondLink 设备 ID 的对应关系：

| Beyond Fixture Number | 多播地址 | BeyondLink 设备 ID | 按键 |
|----------------------|---------|-------------------|-----|
| `1` | `239.255.0.x` | 设备 0 | `0` |
| `2` | `239.255.1.x` | 设备 1 | `1` |
| `3` | `239.255.2.x` | 设备 2 | `2` |
| `4` | `239.255.3.x` | 设备 3 | `3` |

**工作原理**：
- Beyond 根据 `Fixture number` 决定发送到哪个多播地址
- BeyondLink 使用 `IP_PKTINFO` 技术检测数据包的目标地址
- 从目标地址提取设备 ID（第三字节 = Fixture number - 1）

### 快速测试

如果您只想测试单个投影区域：

1. 在 Beyond 的当前投影区域设置中，将 **Fixture number** 设置为 `1`
2. 启用 **Enable visualization through via external software**
3. 启动 BeyondLink，按 `0` 键查看设备 0
4. 在 Beyond 中播放内容，应该能在 BeyondLink 中看到可视化效果

### 切换测试不同设备

1. 在 Beyond 中修改 **Fixture number** 为 `2`
2. 在 BeyondLink 中按 `1` 键切换到设备 1
3. 重复以上步骤测试其他设备（`3` → 设备 2，`4` → 设备 3）

### 配置检查清单 ✅

在启动 BeyondLink 之前，请确认以下配置：

- [ ] **Beyond 外部可视化已启用**
  - View（查看） → Enable visualization through via external software ✓
  
- [ ] **Fixture Number 已设置**
  - 投影区域设置 → External visualization → Fixture number = 1-4
  
- [ ] **Beyond 正在播放内容**
  - 至少有一个 Cue 或 Timeline 在播放
  
- [ ] **BeyondLink 正在运行**
  - 控制台显示 "Joined 155 multicast groups"
  - 窗口标题显示 FPS 数值
  
- [ ] **防火墙允许**
  - BeyondLink.exe 可以接收 UDP 数据
  
- [ ] **设备 ID 匹配**
  - Beyond Fixture number = 1 → BeyondLink 按 `0` 键（设备 0）
  - Beyond Fixture number = 2 → BeyondLink 按 `1` 键（设备 1）

### 多播地址格式

BeyondLink 使用以下多播地址格式：

```
239.255.{DeviceID}.{SubnetID}
```

- **DeviceID**：0-4（对应 5 个设备）
- **SubnetID**：0-30（对应 31 个子网）

例如：
- `239.255.0.1` → 设备 0，子网 1
- `239.255.1.5` → 设备 1，子网 5
- `239.255.3.10` → 设备 3，子网 10

---

## 🏗️ 架构说明

### 核心模块

```
┌─────────────────────────────────────────────┐
│           BeyondLinkSystem                  │
│         (主系统协调器)                        │
└──────────────┬──────────────────────────────┘
               │
       ┌───────┴───────┐
       │               │
       ▼               ▼
┌─────────────┐ ┌─────────────┐
│LaserProtocol│ │LaserRenderer│
│ (网络接收)   │ │ (D3D11渲染) │
└──────┬──────┘ └──────┬──────┘
       │               │
       │    ┌──────────┘
       │    │
       ▼    ▼
┌─────────────────┐
│   LaserSource   │
│  (数据处理×4)    │
│ - 设备 0        │
│ - 设备 1        │
│ - 设备 2        │
│ - 设备 3        │
└─────────────────┘
       │
       ▼
┌─────────────────┐
│  LaserWindow    │
│  (窗口显示)      │
└─────────────────┘
```

### 数据流

```
Beyond 软件
    │
    │ UDP 多播 (239.255.X.Y:5568)
    ▼
LaserProtocol::ReceiveThread
    │
    │ WSARecvMsg + IP_PKTINFO
    ▼
提取目标地址 → 确定设备 ID
    │
    │ linetD2_x64.dll::ReadLaserData
    ▼
解析激光点数据 (X, Y, R, G, B, Focus)
    │
    │ 回调 → OnLaserDataReceived
    ▼
LaserSource::SetPointList
    │
    │ UpdatePointList
    ▼
扫描仪模拟处理
    │ - 点插值
    │ - 速度平滑
    │ - 边缘淡化
    ▼
LaserRenderer::RenderSource
    │
    │ DirectX 11 Pipeline
    ▼
渲染到纹理 (R16G16B16A16_FLOAT)
    │
    ▼
LaserWindow::DisplayLaserTexture
    │
    │ 全屏四边形 + 亮度增强
    ▼
显示到窗口 (512x512)
```

---

## ⚙️ 配置选项

在 `Source/Main.cpp` 中可以修改以下配置：

```cpp
Core::LaserSettings settings;
settings.MaxLaserDevices = 4;           // 最大设备数量
settings.NetworkPort = 5568;            // 网络端口
settings.TextureSize = 1024;            // 渲染纹理大小 (512/1024/2048/4096)
settings.ScannerSimulation = true;      // 启用扫描仪模拟
settings.EnableMipmaps = true;          // 启用 Mipmap
settings.SampleCount = 8;               // 插值采样数
settings.EdgeFade = 0.1f;               // 边缘淡化因子 [0.0, 1.0]
settings.VelocitySmoothing = 0.83f;     // 速度平滑因子 [0.0, 1.0]
settings.LaserQuality = Core::LaserSettings::QualityLevel::High;
```

### 质量级别

| 级别 | 降采样倍数 | 适用场景 |
|------|-----------|---------|
| `Low` | 8× | 低性能硬件 |
| `Medium` | 4× | 一般硬件 |
| `High` | 2× | 推荐设置 |
| `Ultra` | 1× | 高端硬件 |

---

## 🔍 故障排查

### 问题 1：无法接收网络数据

**症状**：
```
[!] WARNING: No network data received!
  Check Beyond network output settings.
```

**解决方案**：
1. **检查 Beyond 外部可视化是否启用**：
   - 打开 Beyond 的 **View（查看）** 菜单
   - 确认 **Enable visualization through via external software** 已勾选
   
2. **检查 Fixture Number 设置**：
   - 打开投影区域设置，确认 **Fixture number** 已设置（通常为 1-4）
   
3. **检查 Beyond 是否在播放内容**：
   - Beyond 只有在播放激光内容时才发送数据
   - 确保有内容在播放（Cue 或 Timeline）
   
4. **检查防火墙设置**：
   - 允许 BeyondLink.exe 访问网络
   - 允许 UDP 端口 5568
   
5. **使用 Wireshark 验证**：
   - 过滤器：`udp.port == 5568`
   - 确认能看到从 Beyond 发送的数据包

### 问题 2：切换设备后黑屏

**症状**：
```
>>> Device 1 (239.255.1.x): [--] 0 points
```

**可能原因和解决方案**：

#### 原因 1：Beyond 没有向该设备发送数据
- **解决**：在 Beyond 的投影区域设置中，修改 **Fixture number**
  - 例如：要在 BeyondLink 中看到设备 1，需要将 Beyond 的 **Fixture number** 设置为 `2`
- **快速测试**：
  1. 在 Beyond 中设置 **Fixture number** = `2`
  2. 在 BeyondLink 中按 `1` 键切换到设备 1
  3. 播放 Beyond 内容，应该能看到可视化

#### 原因 2：网络问题（路由/防火墙）
- **检查**：使用 Wireshark 抓包，过滤 `ip.dst == 239.255.1.0/24`
- **解决**：确认数据包到达本机，检查防火墙规则

#### 原因 3：多播组未加入（代码问题）
- **检查**：启动时查看控制台输出 `Joined 155 multicast groups`
- **解决**：确认没有错误提示，重启程序

**验证方法**：
```bash
# 使用 Wireshark 验证 Beyond 是否发送数据
# 过滤器：udp.port == 5568 && ip.dst == 239.255.1.0/24
```

**注意**：BeyondLink 使用 `IP_PKTINFO` 技术自动识别设备 ID，无需手动配置。只要 Beyond 发送到正确的多播地址，程序会自动处理。

### 问题 3：DLL 加载失败

**症状**：
```
Failed to load linetD2_x64.dll, error: 126
```

**解决方案**：
1. 确认 `bin/linetD2_x64.dll` 和 `bin/matrix64.dll` 存在
2. 确认 DLL 是 64 位版本
3. 安装 [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

### 问题 4：GPU 设备移除

**症状**：
```
D3D Device has been removed! Reason: 0x887A0006
```

**原因**：GPU 驱动崩溃或超时（TDR）

**解决方案**：
1. 更新显卡驱动到最新版本
2. 降低纹理大小（`TextureSize = 512`）
3. 禁用扫描仪模拟（`ScannerSimulation = false`）
4. 检查 GPU 温度和负载

### 问题 5：颜色显示不正确

**检查项**：
1. 确认 Beyond 输出的是 RGB 格式（非单色）
2. 检查 Beyond 中的颜色校准设置
3. 调整 `LaserWindow.cpp` 中的亮度增强系数（默认 5.0）

---

## 🔬 技术细节

### 网络协议

#### 基本参数
- **传输协议**：UDP（无连接）
- **地址类型**：IPv4 多播
- **端口**：5568（默认）
- **包大小**：通常 1468 字节
- **数据格式**：Pangolin Binary Format

#### 关键技术：IP_PKTINFO 设备识别

BeyondLink 使用 Windows Sockets 扩展功能实现精确的设备识别：

```cpp
// 1. 启用 IP_PKTINFO 选项
DWORD optval = 1;
setsockopt(socket, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(optval));

// 2. 使用 WSARecvMsg 接收数据包（而非 recvfrom）
LPFN_WSARECVMSG WSARecvMsgFunc;
WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
         &WSAID_WSARECVMSG, ...);

// 3. 从控制消息中提取目标地址
IN_PKTINFO* pktInfo = (IN_PKTINFO*)WSA_CMSG_DATA(cmsg);
unsigned char* destAddr = (unsigned char*)&pktInfo->ipi_addr.s_addr;

// 4. 从目标多播地址提取设备 ID
// 239.255.X.Y → destAddr[2] = X (设备ID)
int deviceID = destAddr[2];
```

**为什么需要 IP_PKTINFO？**
- `recvfrom()` 只能获取**源地址**（发送方）
- `IP_PKTINFO` 提供**目标地址**（接收方的多播地址）
- Beyond 使用不同的多播地址区分设备（`239.255.0.x`、`239.255.1.x`等）
- 只有知道目标地址，才能正确判断数据包属于哪个设备

### 数据结构

#### LaserPoint (28 字节)
```cpp
struct LaserPoint {
    float X;        // 8 字节 (偏移 0)
    float Y;        // 8 字节 (偏移 4)
    float R;        // 8 字节 (偏移 8)
    float G;        // 12 字节 (偏移 12)
    float B;        // 16 字节 (偏移 16)
    float Z;        // 20 字节 (偏移 20，光束标记)
    float Focus;    // 24 字节 (偏移 24)
};
```

### DLL 函数

#### `linetD2_x64.dll` 接口

```cpp
void Init(int maxDevices);                    // 初始化，指定最大设备数
void ReadLaserData(void* data, int length);   // 解析UDP数据包
void* GetData(int device, int* pointCount);   // 获取解析后的点数据
void Release();                               // 释放资源
```

### 渲染管线

1. **顶点着色器**：将 2D 点坐标传递到像素着色器
2. **像素着色器**：输出点的 RGB 颜色
3. **混合模式**：`SrcBlend=ONE, DestBlend=ONE`（加法混合）
4. **拓扑结构**：`POINTLIST`（点云）
5. **输出格式**：`R16G16B16A16_FLOAT`（HDR）

### 扫描仪模拟算法

```
原始点列表 (N 个点)
    │
    ▼
点插值 (N × SampleCount 个点)
    │
    ▼
速度平滑 (惯性模拟)
    │ v(t+1) = v(t) + (v_target - v(t)) × (1 - smoothing)
    ▼
边缘淡化 (速度依赖)
    │ intensity = f(velocity, edgeFade)
    ▼
降采样 (根据质量级别)
    │
    ▼
输出处理后的点列表
```

### 核心技术突破

#### 多设备精确识别
传统的 UDP 多播接收使用 `recvfrom()`，只能获取数据包的**源地址**（发送方IP），无法区分数据包发送到哪个多播地址。当 Beyond 同时向 `239.255.0.x` 和 `239.255.1.x` 发送数据时，接收方无法判断数据属于设备 0 还是设备 1。

**BeyondLink 的解决方案**：
1. 启用 `IP_PKTINFO` Socket 选项
2. 使用 `WSARecvMsg()` 替代 `recvfrom()`
3. 从 `WSACMSGHDR` 控制消息中提取 `IN_PKTINFO` 结构
4. 读取 `ipi_addr` 字段获取**目标地址**（多播地址）
5. 从目标地址第三字节提取设备 ID

这是 BeyondLink 能够正确处理多设备切换的核心技术。

#### 扫描仪物理模拟
真实的激光扫描仪（电流表镜）具有惯性和加速特性，BeyondLink 通过算法模拟这些物理特性：

```
速度平滑（惯性模拟）：
    v(t+1) = v(t) + (v_target - v(t)) × (1 - smoothing)
    
边缘淡化（速度依赖亮度）：
    intensity = f(1/velocity, edgeFade)
    
结果：高速扫描的线条较暗，静止光束最亮
```

#### HDR 渲染管线
- **纹理格式**：`R16G16B16A16_FLOAT`（每通道 16 位浮点）
- **混合模式**：`SrcBlend=ONE, DestBlend=ONE`（加法混合）
- **后处理**：5× 亮度增强 + saturate 防止溢出
- **优势**：可以精确表示激光的叠加和过曝效果

---

## 🧪 单元测试

BeyondLink 提供完整的C API单元测试套件，确保所有功能正常工作。

### 测试覆盖

- ✅ **23个测试用例**覆盖所有C API函数
- ✅ 生命周期管理、网络控制、配置接口
- ✅ 纹理访问（UE集成关键功能）
- ✅ 多设备支持、异常处理

### 快速测试

```bash
# 方法1：使用批处理脚本（推荐）
cd Test
RunTest.bat

# 方法2：手动编译
cl /std:c++17 /EHsc /I"include" Test\BeyondLinkCAPI_Test.cpp /link Build\Binaries\Release\BeyondLink.lib
```

详细说明请参见：**[Test/README_TEST.md](Test/README_TEST.md)**

---

## 🎮 虚幻引擎集成

BeyondLink 现已支持虚幻引擎5.6+集成！通过C API和D3D11纹理共享技术，您可以在UE中实时显示Beyond激光数据。

### 快速集成

1. **编译静态库**：将项目配置改为静态库(.lib)
2. **拷贝文件**：将头文件、.lib和DLL拷贝到UE插件的ThirdParty目录
3. **链接库**：在.Build.cs中配置依赖
4. **使用C API**：通过 `BeyondLinkCAPI.h` 调用功能

### 核心接口

```cpp
// 创建系统
BeyondLinkHandle handle = BeyondLink_Create();
BeyondLink_Initialize(handle);
BeyondLink_StartNetwork(handle, nullptr);

// 每帧更新
BeyondLink_Update(handle);
BeyondLink_Render(handle);

// 获取共享纹理句柄（零拷贝）
void* sharedHandle = BeyondLink_GetSharedHandle(handle, deviceID);
// 在UE中：ID3D11Device::OpenSharedResource(sharedHandle, ...)
```

### 详细文档

完整的UE集成指南请参见：**[UE_INTEGRATION.md](UE_INTEGRATION.md)**

包含：
- ✅ 完整的插件创建步骤
- ✅ D3D11纹理共享实现代码
- ✅ 多线程安全最佳实践
- ✅ 常见问题排查
- ✅ 性能优化建议

---

## 📄 许可证

本项目仅供学习和研究使用。

**第三方库**：
- **linetD2_x64.dll**：版权归 Beyond/Pangolin 所有
- **DirectX 11**：版权归 Microsoft 所有

---

## 🙏 致谢

- 感谢 **Pangolin Laser Systems** 提供的 Beyond 软件和网络协议
- 感谢 **Microsoft** 提供的 DirectX SDK 和 Windows Sockets API
- 参考了相关激光可视化项目的实现思路
- 感谢所有为激光可视化技术做出贡献的开发者

### 技术参考

本项目在开发过程中参考了以下技术资料：

- **Windows Sockets 2 (Winsock)**：[Microsoft Docs - WSARecvMsg](https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsarecvmsg)
- **DirectX 11 Graphics**：[Microsoft Docs - Direct3D 11](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
- **IP Multicast**：[RFC 1112 - Host Extensions for IP Multicasting](https://tools.ietf.org/html/rfc1112)
- **ILDA Protocol**：Laser show data interchange standards

### 开发历程

- **v1.0**：基础网络接收和渲染功能
- **v1.1**：添加扫描仪模拟算法
- **v1.2**：实现多设备支持（使用 IP_PKTINFO）
- **v1.3**：完善代码注释和文档（1200+ 行注释）
- **当前版本**：稳定版本，支持 4 设备实时切换和可视化

---

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- **Issue Tracker**：[GitHub Issues](https://github.com/AspectEngine/BeyondLink/issues)
- **Email**：liuxiunan596@gmail.com

---

<div align="center">

**Made with ❤️ for Laser Artists**

[⬆ 返回顶部](#beyondlink---beyond-激光实时可视化系统)

</div>

