# BeyondLink - Beyond 激光实时可视化工具

一个用于接收和可视化 Pangolin Beyond 激光数据的 DirectX 11 应用程序。

[![Version](https://img.shields.io/badge/Version-v1.5.0-green.svg)](https://github.com/AspectEngine/BeyondLink/releases)
[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)

---

## 目录

- [项目简介](#项目简介)
- [主要功能](#主要功能)
- [系统要求](#系统要求)
- [快速开始](#快速开始)
- [项目结构](#项目结构)
- [使用说明](#使用说明)
- [Beyond 配置](#beyond-配置)
- [系统架构](#系统架构)
- [配置选项](#配置选项)
- [常见问题](#常见问题)
- [技术实现](#技术实现)

---

## 项目简介

BeyondLink 是为 Pangolin Beyond 激光控制软件开发的实时可视化工具。通过 UDP 多播接收 Beyond 输出的激光数据，并使用 DirectX 11 渲染显示。

主要特点：
- 支持 9 台激光设备同时可视化（设备 1-9）
- 低延迟、高帧率渲染
- 模拟真实激光扫描仪物理特性
- 实时切换不同设备预览

---

## 主要功能

### 渲染

- DirectX 11 硬件加速的点云渲染
- HDR 纹理支持（R16G16B16A16_FLOAT 格式）
- 加法混合模式模拟激光叠加效果
- 支持 Mipmap 生成
- 5倍亮度后处理增强

### 网络

- UDP 多播接收（自动加入 279 个多播组）
- 使用 IP_PKTINFO 识别数据包目标地址
- 从多播地址自动提取设备 ID（239.255.X.Y 格式）
- 集成 linetD2_x64.dll 解析 Pangolin 协议
- 独立的网络接收线程

### 扫描仪模拟

- 速度平滑（模拟扫描仪惯性）
- 根据扫描速度动态调整亮度
- 自动点插值提升渲染质量
- 识别并增强静止光束
- 4档质量级别（Low/Medium/High/Ultra）

### 交互

- 按 1-9 键实时切换设备
- 每 5 秒输出状态报告
- 窗口标题显示 FPS
- 详细的错误提示

---

## 系统要求

**最低配置：**
- Windows 10/11 (64-bit)
- 双核 CPU（支持 SSE2）
- 支持 DirectX 11 Feature Level 11.0 的显卡
- 2 GB 内存
- 千兆网卡（推荐）

**推荐配置：**
- Windows 10/11 (64-bit)
- 四核或更高 CPU
- 独立显卡（NVIDIA/AMD，支持 DX11.1）
- 4 GB 以上内存
- 千兆网卡

**开发环境：**
- Visual Studio 2022
- CMake 3.15+
- Windows SDK 10.0+
- C++17

---

## 快速开始

### 1. 获取代码

```bash
git clone https://github.com/AspectEngine/BeyondLink.git
cd BeyondLink
```

### 2. 准备依赖

确保 `bin/` 目录下有这两个 DLL 文件（来自 Beyond SDK）：

```
BeyondLink/
├── bin/
│   ├── linetD2_x64.dll
│   └── matrix64.dll
```

### 3. 编译

**使用 Visual Studio 2022：**

1. 打开 Visual Studio 2022
2. 文件 → 打开 → 文件夹，选择 BeyondLink 目录
3. VS 会自动检测 CMakeLists.txt 并配置项目
4. 工具栏选择 Debug x64 或 Release x64
5. 按 Ctrl+Shift+B 编译
6. 按 F5 运行

**使用命令行：**

```bash
# 配置
cmake -B Build/CMake/Debug -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build Build/CMake/Debug --config Debug

# 运行
Build\Binaries\Debug\BeyondLink.exe
```

### 4. 输出位置

- Debug 版本：`Build\Binaries\Debug\BeyondLink.exe`
- Release 版本：`Build\Binaries\Release\BeyondLink.exe`

CMake 会自动复制 DLL 到输出目录。

### 5. 配置 Beyond 并测试

在 Beyond 中：
1. View 菜单 → 勾选 "Enable visualization through via external software"
2. 打开投影区域设置 → External visualization → 设置 Fixture number = 1
3. 播放内容

在 BeyondLink 中按 0 键查看设备 0，应该能看到可视化效果。

---

## 项目结构

```
BeyondLink/
├── include/                    # 头文件
│   ├── BeyondLink.h           # 主系统接口
│   ├── LaserPoint.h           # 激光点数据结构（28字节）
│   ├── LaserProtocol.h        # 网络协议处理
│   ├── LaserRenderer.h        # DirectX 11 渲染器
│   ├── LaserSettings.h        # 配置参数
│   ├── LaserSource.h          # 激光源数据处理
│   └── LaserWindow.h          # 显示窗口
│
├── Source/                     # 源文件
│   ├── BeyondLink.cpp         # 主系统实现
│   ├── LaserProtocol.cpp      # 网络接收和解析（WSARecvMsg）
│   ├── LaserRenderer.cpp      # 渲染管线（D3D11）
│   ├── LaserSource.cpp        # 扫描仪模拟算法
│   ├── LaserWindow.cpp        # 窗口管理
│   └── Main.cpp               # 程序入口
│
├── bin/                        # 依赖 DLL
│   ├── linetD2_x64.dll
│   └── matrix64.dll
│
├── Build/                      # 构建输出
│   ├── Binaries/              # 可执行文件
│   ├── Intermediate/          # 中间文件
│   └── CMake/                 # CMake 生成文件
│
├── CMakeLists.txt             # CMake 配置
├── CMakePresets.json          # CMake 预设
├── .gitignore
└── README.md
```

代码注释比较详细（1200+ 行中文注释），适合学习 DirectX 11 和网络编程。

---

## 使用说明

### 启动后会发生什么

程序会：
1. 初始化 DirectX 11 设备
2. 加载 linetD2_x64.dll
3. 创建 9 个激光源（对应设备 1-9）
4. 绑定 UDP 端口 5568
5. 加入 279 个多播组
6. 打开 512x512 预览窗口

### 键盘控制

- **1-9**：切换到对应设备（设备 1 = Fixture 1 = 239.255.0.x，以此类推）
- **ESC**：退出

### 状态报告

每 5 秒输出一次：

```
=== Status Report ===
Network: 5234 packets | 7680152 bytes | FPS: 59

All Devices Status:
>>> Device 1 (239.255.0.x): [OK] 2220 points <- VIEWING
    Device 2 (239.255.1.x): [OK] 1856 points
    Device 3 (239.255.2.x): [--] 0 points
    Device 4 (239.255.3.x): [--] 0 points
    Device 5 (239.255.4.x): [--] 0 points
    Device 6 (239.255.5.x): [--] 0 points
    Device 7 (239.255.6.x): [--] 0 points
    Device 8 (239.255.7.x): [--] 0 points
    Device 9 (239.255.8.x): [--] 0 points
===================
```

- [OK]：有数据
- [--]：无数据
- `<- VIEWING`：当前正在查看

---

## Beyond 配置

### 重要：Fixture Number 映射关系

Beyond 的 Fixture number 与 BeyondLink 设备编号完全一致：

| Beyond Fixture | 多播地址 | BeyondLink 设备 | 按键 |
|---------------|---------|----------------|-----|
| 1 | 239.255.0.x | 设备 1 | 1 |
| 2 | 239.255.1.x | 设备 2 | 2 |
| 3 | 239.255.2.x | 设备 3 | 3 |
| 4 | 239.255.3.x | 设备 4 | 4 |
| 5 | 239.255.4.x | 设备 5 | 5 |
| 6 | 239.255.5.x | 设备 6 | 6 |
| 7 | 239.255.6.x | 设备 7 | 7 |
| 8 | 239.255.7.x | 设备 8 | 8 |
| 9 | 239.255.8.x | 设备 9 | 9 |

### 配置步骤

**1. 启用外部可视化**

在 Beyond 的 View 菜单中勾选：
```
Enable visualization through via external software
```

**2. 设置 Fixture Number**

打开投影区域（Projection Zone）设置，找到 External visualization 部分，设置 Fixture number。

例如：
- 投影区域 1 → Fixture number = 1（对应设备 1）
- 投影区域 2 → Fixture number = 2（对应设备 2）
- 以此类推

**3. 播放内容**

Beyond 只在播放时发送数据，确保有 Cue 或 Timeline 在运行。

**4. 切换设备**

在 BeyondLink 中按 1-9 键切换到对应设备查看。

### 工作原理

- Beyond 根据 Fixture number 决定发送到哪个多播地址
- BeyondLink 使用 IP_PKTINFO 检测数据包的目标地址
- 从目标地址第三字节提取内部索引（Fixture number - 1），显示时转换为设备编号（Fixture number）

### 快速测试

1. Beyond 设置 Fixture number = 1
2. 启用外部可视化
3. 启动 BeyondLink，按 1 键
4. 播放内容，应该能看到效果

---

## 系统架构

### 模块组成

```
BeyondLinkSystem (主系统)
    ├── LaserProtocol (网络接收)
    ├── LaserRenderer (D3D11渲染)
    ├── LaserSource×9 (数据处理)
    └── LaserWindow (窗口显示)
```

### 数据流程

```
Beyond 软件
    ↓ UDP 多播 (239.255.X.Y:5568)
LaserProtocol::ReceiveThread
    ↓ WSARecvMsg + IP_PKTINFO
提取目标地址 → 设备 ID
    ↓ linetD2_x64.dll
解析激光点数据
    ↓ 回调
LaserSource::SetPointList
    ↓ 扫描仪模拟
处理后的点数据
    ↓ DirectX 11
LaserRenderer::RenderSource
    ↓ 渲染到纹理
LaserWindow::DisplayLaserTexture
    ↓ 显示到屏幕
512x512 窗口
```

---

## 配置选项

在 `Source/Main.cpp` 中可以修改：

```cpp
Core::LaserSettings settings;
settings.MaxLaserDevices = 9;           // 设备数量（1-9）
settings.NetworkPort = 5568;            // 网络端口
settings.TextureSize = 1024;            // 渲染分辨率 (512/1024/2048/4096)
settings.ScannerSimulation = true;      // 扫描仪模拟
settings.EnableMipmaps = true;          // Mipmap
settings.SampleCount = 8;               // 插值采样数
settings.EdgeFade = 0.1f;               // 边缘淡化 [0.0-1.0]
settings.VelocitySmoothing = 0.83f;     // 速度平滑 [0.0-1.0]
settings.LaserQuality = Core::LaserSettings::QualityLevel::High;
```

质量级别对比：

| 级别 | 降采样 | 说明 |
|------|-------|------|
| Low | 8× | 低性能硬件 |
| Medium | 4× | 一般硬件 |
| High | 2× | 推荐 |
| Ultra | 1× | 高端硬件 |

---

## 常见问题

### 收不到数据

**症状：**
```
[!] WARNING: No network data received!
```

**检查：**
1. Beyond 的外部可视化是否启用（View 菜单）
2. Fixture number 是否设置（投影区域设置）
3. Beyond 是否在播放内容
4. 防火墙是否允许 BeyondLink.exe（UDP 5568）
5. 用 Wireshark 过滤 `udp.port == 5568` 确认有数据包

### 切换设备后黑屏

**原因：** Beyond 没有向该设备发送数据

**解决：** 修改 Beyond 的 Fixture number

例如要看设备 2：
- Beyond 中设置 Fixture number = 2
- BeyondLink 中按 2 键

可以用 Wireshark 过滤 `ip.dst == 239.255.1.0/24` 验证 Beyond 是否发送数据（注意：设备 2 对应多播地址 239.255.1.x）。

### DLL 加载失败

**症状：**
```
Failed to load linetD2_x64.dll, error: 126
```

**解决：**
1. 确认 `bin/linetD2_x64.dll` 和 `bin/matrix64.dll` 存在
2. 确认是 64 位 DLL
3. 安装 [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

### GPU 设备移除

**症状：**
```
D3D Device has been removed! Reason: 0x887A0006
```

**原因：** GPU 驱动崩溃或超时

**解决：**
1. 更新显卡驱动
2. 降低纹理大小（TextureSize = 512）
3. 禁用扫描仪模拟（ScannerSimulation = false）
4. 检查 GPU 温度

### 颜色不对

**检查：**
1. Beyond 输出的是 RGB 还是单色
2. Beyond 的颜色校准
3. 可以修改 `LaserWindow.cpp` 中的亮度系数（默认 5.0）

---

## 技术实现

### 网络协议

- 传输：UDP 无连接
- 地址：IPv4 多播
- 端口：5568
- 包大小：通常 1468 字节
- 格式：Pangolin Binary Format

### 关键技术：IP_PKTINFO

这是 BeyondLink 能正确识别多设备的核心。

```cpp
// 启用 IP_PKTINFO
DWORD optval = 1;
setsockopt(socket, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(optval));

// 使用 WSARecvMsg 而非 recvfrom
LPFN_WSARECVMSG WSARecvMsgFunc;
WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, ...);

// 从控制消息提取目标地址
IN_PKTINFO* pktInfo = (IN_PKTINFO*)WSA_CMSG_DATA(cmsg);
unsigned char* destAddr = (unsigned char*)&pktInfo->ipi_addr.s_addr;

// 提取设备 ID（239.255.X.Y → X）
int deviceID = destAddr[2];
```

**为什么需要它？**

普通的 `recvfrom()` 只能获取发送方 IP，无法知道数据包发送到哪个多播地址。Beyond 用不同多播地址区分设备（239.255.0.x、239.255.1.x 等），只有知道目标地址才能正确判断设备 ID。

### 数据结构

LaserPoint（28 字节）：

```cpp
struct LaserPoint {
    float X;        // 偏移 0
    float Y;        // 偏移 4
    float R;        // 偏移 8
    float G;        // 偏移 12
    float B;        // 偏移 16
    float Z;        // 偏移 20（光束标记）
    float Focus;    // 偏移 24
};
```

### linetD2_x64.dll 接口

```cpp
void Init(int maxDevices);                    // 初始化
void ReadLaserData(void* data, int length);   // 解析数据包
void* GetData(int device, int* pointCount);   // 获取点数据
void Release();                               // 释放资源
```

### 渲染管线

1. 顶点着色器：传递 2D 坐标
2. 像素着色器：输出 RGB 颜色
3. 混合模式：SrcBlend=ONE, DestBlend=ONE（加法混合）
4. 拓扑：POINTLIST
5. 输出：R16G16B16A16_FLOAT（HDR）

### 扫描仪模拟

```
原始点
  ↓ 点插值（N × SampleCount）
  ↓ 速度平滑（惯性）
  ↓ 边缘淡化（速度依赖）
  ↓ 降采样（质量级别）
处理后的点
```

速度平滑公式：
```
v(t+1) = v(t) + (v_target - v(t)) × (1 - smoothing)
```

边缘淡化：高速扫描的线较暗，静止光束最亮。

### HDR 渲染

- 纹理：R16G16B16A16_FLOAT（每通道 16 位浮点）
- 混合：加法混合模拟激光叠加
- 后处理：5× 亮度增强 + saturate

---

## CMake 构建

BeyondLink 使用 CMake 3.15+，支持 Visual Studio 2022 和命令行编译。

**优势：**
- 跨 IDE 支持（VS 2022、VS Code）
- 自动链接 DirectX 和 Winsock 库
- 自动复制 DLL 到输出目录
- 预设 Debug/Release 配置

**编译选项：**

| 选项 | Debug | Release |
|------|-------|---------|
| 优化 | /Od | /O2 |
| 调试 | /Zi | /Zi |
| 运行时检查 | /RTC1 | - |
| 内联 | - | /Oi |
| 链接优化 | - | /OPT:REF + /OPT:ICF |

**输出结构：**

```
Build/
├── Binaries/
│   ├── Debug/
│   │   ├── BeyondLink.exe
│   │   ├── BeyondLink.pdb
│   │   └── *.dll
│   └── Release/
│       └── ...
└── CMake/
    ├── Debug/
    └── Release/
```

---

## 许可证

本项目仅供学习和研究使用。

第三方库：
- linetD2_x64.dll：版权归 Beyond/Pangolin 所有
- DirectX 11：版权归 Microsoft 所有

---

## 致谢

感谢 Pangolin Laser Systems 提供的 Beyond 软件和协议。

感谢 Microsoft 提供的 DirectX 和 Windows Sockets API。

### 技术参考

- [Windows Sockets 2 - WSARecvMsg](https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsarecvmsg)
- [DirectX 11 Graphics](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
- [RFC 1112 - IP Multicasting](https://tools.ietf.org/html/rfc1112)

### 开发历程

- **v1.0.0**：基础功能（UDP 多播接收、DirectX 11 渲染、单设备支持）
- **v1.1.0**：扫描仪模拟（点插值、速度平滑、边缘淡化）
- **v1.2.0**：多设备支持（IP_PKTINFO、4 设备切换）
- **v1.3.0**：完善注释和文档（1200+ 行中文注释）
- **v1.4.0**：迁移到 CMake 构建系统（跨 IDE 支持）
- **v1.5.0**（当前版本）：扩展到 9 个设备，优化用户界面（设备编号 1-9）

---

## 版本信息

**当前版本：v1.5.0**

- 发布日期：2025-10-27
- 状态：稳定版
- 主要特性：支持 9 个激光设备，设备编号 1-9，完整的 CMake 构建系统

### 发布说明

**v1.5.0 新增功能：**
- ✅ 支持 9 个激光设备（设备 1-9）
- ✅ 用户友好的设备编号（与 Beyond Fixture number 一致）
- ✅ 优化的键盘控制（按 1-9 键切换设备）
- ✅ 版本信息显示（窗口标题和启动信息）

---

## 联系方式

- GitHub Issues: [BeyondLink Issues](https://github.com/AspectEngine/BeyondLink/issues)
- Email: liuxiunan596@gmail.com

---

**BeyondLink v1.5.0** - Made for Laser Artists 🎨✨

[⬆ 返回顶部](#beyondlink---beyond-激光实时可视化工具)
