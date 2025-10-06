//==============================================================================
// 文件：LaserProtocol.h
// 作者：Yunsio
// 日期：2025-10-06
// 描述：Beyond 激光网络协议处理模块
//      负责 UDP 多播接收、数据包解析、设备识别
//      使用 IP_PKTINFO 从目标地址提取设备 ID
//==============================================================================

#pragma once

// Must include winsock2 BEFORE any Windows headers
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>  // For IP_PKTINFO
#include <mswsock.h>  // For LPFN_WSARECVMSG and WSAID_WSARECVMSG
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "LaserPoint.h"
#include "LaserSettings.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace BeyondLink {
namespace Core {

//==========================================================================
// 类：LaserProtocol
// 描述：Beyond 激光网络通信协议处理器
//      - 绑定 UDP 端口 5568
//      - 加入 155 个多播组（设备 0-4，子网 0-30）
//      - 使用 WSARecvMsg + IP_PKTINFO 提取目标地址
//      - 调用 linetD2_x64.dll 解析 Pangolin 二进制格式
//      - 异步后台接收线程
//==========================================================================
class LaserProtocol {
public:
    //==========================================================================
    // 构造函数：LaserProtocol
    // 描述：创建激光网络协议处理器
    // 参数：
    //   settings - 激光系统配置参数
    //==========================================================================
    explicit LaserProtocol(const LaserSettings& settings);
    ~LaserProtocol();

    //==========================================================================
    // 函数：Start
    // 描述：启动网络接收（创建 socket、加入多播组、启动接收线程）
    // 参数：
    //   localIP - 本地 IP 地址（空字符串表示 INADDR_ANY）
    // 返回值：
    //   true - 启动成功
    //   false - 启动失败
    //==========================================================================
    bool Start(const std::string& localIP = "");
    
    //==========================================================================
    // 函数：Stop
    // 描述：停止网络接收（离开多播组、关闭 socket、停止接收线程）
    //==========================================================================
    void Stop();
    
    //==========================================================================
    // 函数：IsRunning
    // 描述：检查网络接收是否正在运行
    // 返回值：
    //   true - 正在运行
    //   false - 已停止
    //==========================================================================
    bool IsRunning() const { return m_Running; }

    //==========================================================================
    // 类型：DataCallback
    // 描述：数据接收回调函数类型
    // 参数：
    //   deviceID - 设备 ID (0-3)
    //   points - 解析后的激光点列表
    //==========================================================================
    using DataCallback = std::function<void(int deviceID, const std::vector<LaserPoint>&)>;
    
    //==========================================================================
    // 函数：SetDataCallback
    // 描述：设置数据接收回调函数
    // 参数：
    //   callback - 回调函数
    //==========================================================================
    void SetDataCallback(DataCallback callback) { m_DataCallback = callback; }

    //==========================================================================
    // 结构体：NetworkStats
    // 描述：网络统计信息
    //==========================================================================
    struct NetworkStats {
        uint64_t PacketsReceived = 0;    // 接收的数据包总数
        uint64_t BytesReceived = 0;      // 接收的字节总数
        uint64_t PacketsDropped = 0;     // 丢弃的数据包数（保留）
        uint32_t LastPacketSize = 0;     // 最后一个数据包的大小
    };
    
    //==========================================================================
    // 函数：GetStats
    // 描述：获取网络统计信息
    // 返回值：
    //   网络统计信息结构体
    //==========================================================================
    NetworkStats GetStats() const { return m_Stats; }
    
    //==========================================================================
    // 函数：GetPort
    // 描述：获取绑定的 UDP 端口号
    // 返回值：
    //   端口号
    //==========================================================================
    int GetPort() const { return m_Port; }

private:
    //==========================================================================
    // 函数：InitializeWinsock
    // 描述：初始化 Windows Socket 库（Windows 平台）
    // 返回值：
    //   true - 初始化成功
    //   false - 初始化失败
    //==========================================================================
    bool InitializeWinsock();
    
    //==========================================================================
    // 函数：CleanupWinsock
    // 描述：清理 Windows Socket 库（Windows 平台）
    //==========================================================================
    void CleanupWinsock();
    
    //==========================================================================
    // 函数：CreateSocket
    // 描述：创建 UDP socket 并绑定到指定端口
    //      同时启用 IP_PKTINFO 以接收目标地址信息
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateSocket();
    
    //==========================================================================
    // 函数：JoinMulticastGroups
    // 描述：加入所有多播组（设备 0-4，子网 0-30，共 155 个组）
    //      多播地址格式：239.255.{DeviceID}.{SubnetID}
    // 参数：
    //   localIP - 本地 IP 地址
    // 返回值：
    //   true - 至少加入一个多播组
    //   false - 全部失败
    //==========================================================================
    bool JoinMulticastGroups(const std::string& localIP);
    
    //==========================================================================
    // 函数：LeaveMulticastGroups
    // 描述：离开所有已加入的多播组
    //==========================================================================
    void LeaveMulticastGroups();
    
    //==========================================================================
    // 函数：CloseSocket
    // 描述：关闭 UDP socket
    //==========================================================================
    void CloseSocket();
    
    //==========================================================================
    // 函数：ReceiveThread
    // 描述：后台接收线程函数
    //      循环接收 UDP 数据包，提取目标地址，解析激光数据
    //==========================================================================
    void ReceiveThread();
    
    //==========================================================================
    // 函数：ParsePacket
    // 描述：解析 UDP 数据包为激光点列表
    //      1. 调用 linetD2_x64.dll::ReadLaserData 解析包
    //      2. 根据 extractedDeviceID 调用 GetData 获取点数据
    //      3. 转换为 LaserPoint 格式（Y 轴反转，颜色归一化）
    // 参数：
    //   data - UDP 数据包内容
    //   length - 数据包长度
    //   extractedDeviceID - 从目标地址提取的设备 ID
    //   deviceID - 输出：解析到的设备 ID
    //   points - 输出：解析后的激光点列表
    // 返回值：
    //   true - 解析成功
    //   false - 解析失败
    //==========================================================================
    bool ParsePacket(const uint8_t* data, size_t length, 
                     int extractedDeviceID, int& deviceID, 
                     std::vector<LaserPoint>& points);
    
    //==========================================================================
    // 函数：GetMulticastAddress
    // 描述：生成多播地址字符串
    // 参数：
    //   deviceID - 设备 ID (0-4)
    //   subnetID - 子网 ID (0-30)
    // 返回值：
    //   多播地址字符串，格式：239.255.{DeviceID}.{SubnetID}
    //==========================================================================
    std::string GetMulticastAddress(int deviceID, int subnetID) const;

private:
    LaserSettings m_Settings;                    // 系统配置
    int m_Port;                                  // UDP 端口号
    int m_MaxDevices;                            // 最大设备数量
    
#ifdef _WIN32
    SOCKET m_Socket = INVALID_SOCKET;            // Windows Socket 句柄
    WSADATA m_WSAData;                           // WSA 数据
    
    // linetD2_x64.dll 函数指针（用于解析 Pangolin 协议）
    HMODULE m_DllHandle;                                            // DLL 句柄
    void (*m_InitDll)(int maxDevices);                             // 初始化函数
    void (*m_ReadLaserData)(void* data, int length);               // 读取激光数据
    void* (*m_GetData)(int device, int* pointCount);               // 获取解析后的数据
    void (*m_Release)();                                           // 释放资源
#else
    int m_Socket = -1;                           // Linux Socket 句柄
#endif
    
    // 线程控制
    std::atomic<bool> m_Running;                 // 运行标志
    std::thread m_ReceiveThread;                 // 接收线程
    
    // 数据回调
    DataCallback m_DataCallback;                 // 数据回调函数
    std::mutex m_CallbackMutex;                  // 回调互斥锁
    
    // 统计信息
    mutable NetworkStats m_Stats;                // 网络统计
    mutable std::mutex m_StatsMutex;             // 统计互斥锁
    
    // 多播组管理
    std::vector<std::string> m_JoinedGroups;     // 已加入的多播组列表
};

} // namespace Core
} // namespace BeyondLink
