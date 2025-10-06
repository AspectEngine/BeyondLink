//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：LaserProtocol.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：激光网络协议实现，负责UDP多播接收、linetD2_x64.dll调用、
//       数据包解析和设备ID识别
//==============================================================================

#include "../include/LaserProtocol.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace BeyondLink {
namespace Core {

//==========================================================================
// 构造函数：LaserProtocol
// 描述：初始化网络协议处理器，加载linetD2_x64.dll并获取函数指针
// 参数：
//   settings - 激光系统配置参数
//==========================================================================
LaserProtocol::LaserProtocol(const LaserSettings& settings)
    : m_Settings(settings)
    , m_Port(settings.NetworkPort)
    , m_MaxDevices(settings.MaxLaserDevices)
    , m_Running(false)
    , m_DllHandle(nullptr)
    , m_InitDll(nullptr)
    , m_ReadLaserData(nullptr)
    , m_GetData(nullptr)
    , m_Release(nullptr)
{
#ifdef _WIN32
    m_Socket = INVALID_SOCKET;
    
    // 加载 linetD2_x64.dll（与Depence源码一致）
    // 先尝试从当前目录加载
    m_DllHandle = LoadLibraryA("linetD2_x64.dll");
    
    // 如果失败，尝试从可执行文件目录加载
    if (!m_DllHandle) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir(exePath);
        size_t pos = exeDir.find_last_of("\\/");
        if (pos != std::string::npos) {
            exeDir = exeDir.substr(0, pos + 1);
            std::string dllPath = exeDir + "linetD2_x64.dll";
            m_DllHandle = LoadLibraryA(dllPath.c_str());
        }
    }
    
    if (m_DllHandle) {
        // 获取函数指针
        m_InitDll = reinterpret_cast<void(*)(int)>(
            GetProcAddress(m_DllHandle, "Init"));
        m_ReadLaserData = reinterpret_cast<void(*)(void*, int)>(
            GetProcAddress(m_DllHandle, "ReadLaserData"));
        m_GetData = reinterpret_cast<void*(*)(int, int*)>(
            GetProcAddress(m_DllHandle, "GetData"));
        m_Release = reinterpret_cast<void(*)()>(
            GetProcAddress(m_DllHandle, "Release"));
        
        if (m_InitDll && m_ReadLaserData && m_GetData && m_Release) {
            // 初始化DLL（关键！）
            m_InitDll(m_MaxDevices);
            std::cout << "linetD2_x64.dll loaded and initialized successfully" << std::endl;
        } else {
            std::cerr << "Failed to get function pointers from linetD2_x64.dll" << std::endl;
        }
    } else {
        std::cerr << "Failed to load linetD2_x64.dll, error: " << GetLastError() << std::endl;
    }
#else
    m_Socket = -1;
#endif
}

//==========================================================================
// 析构函数：~LaserProtocol
// 描述：停止协议处理器，释放DLL资源
//==========================================================================
LaserProtocol::~LaserProtocol() {
    Stop();
    
#ifdef _WIN32
    // 释放 DLL
    if (m_Release) {
        m_Release();
    }
    if (m_DllHandle) {
        FreeLibrary(m_DllHandle);
        m_DllHandle = nullptr;
    }
#endif
}

//==========================================================================
// 函数：InitializeWinsock
// 描述：初始化Windows Socket库（仅Windows平台）
// 返回值：
//   true - 初始化成功
//   false - 初始化失败
//==========================================================================
bool LaserProtocol::InitializeWinsock() {
#ifdef _WIN32
    int result = WSAStartup(MAKEWORD(2, 2), &m_WSAData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

//==========================================================================
// 函数：CleanupWinsock
// 描述：清理Windows Socket库（仅Windows平台）
//==========================================================================
void LaserProtocol::CleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

//==========================================================================
// 函数：CreateSocket
// 描述：创建UDP Socket，绑定端口，启用IP_PKTINFO以接收目标地址信息
// 返回值：
//   true - 创建成功
//   false - 创建失败
//==========================================================================
bool LaserProtocol::CreateSocket() {
    // 创建UDP socket
    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
#ifdef _WIN32
    if (m_Socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
#else
    if (m_Socket < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return false;
    }
#endif

    // 设置socket选项：允许地址重用
    int reuse = 1;
    if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        CloseSocket();
        return false;
    }

    // 绑定到指定端口
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_Port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_Socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
#ifdef _WIN32
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Bind failed" << std::endl;
#endif
        CloseSocket();
        return false;
    }

    // 设置接收缓冲区大小
    int bufferSize = 256 * 1024; // 256 KB
    setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, 
               reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));

#ifdef _WIN32
    // 启用 IP_PKTINFO 以获取目标地址信息（关键！）
    DWORD optval = 1;
    if (setsockopt(m_Socket, IPPROTO_IP, IP_PKTINFO,
                   reinterpret_cast<const char*>(&optval), sizeof(optval)) < 0) {
        std::cerr << "Failed to set IP_PKTINFO: " << WSAGetLastError() << std::endl;
        CloseSocket();
        return false;
    }
    std::cout << "IP_PKTINFO enabled - will receive destination address info" << std::endl;
#endif

    return true;
}

//==========================================================================
// 函数：GetMulticastAddress
// 描述：生成多播地址字符串（格式：239.255.{deviceID}.{subnetID}）
// 参数：
//   deviceID - 设备ID (0-4)
//   subnetID - 子网ID (0-30)
// 返回值：
//   string - 多播地址字符串
//==========================================================================
std::string LaserProtocol::GetMulticastAddress(int deviceID, int subnetID) const {
    std::ostringstream oss;
    oss << "239.255." << deviceID << "." << subnetID;
    return oss.str();
}

//==========================================================================
// 函数：JoinMulticastGroups
// 描述：加入所有必要的多播组（设备0-4，子网0-30，共155个组）
// 参数：
//   localIP - 本地IP地址（空字符串表示0.0.0.0）
// 返回值：
//   true - 至少加入一个组成功
//   false - 全部失败
//==========================================================================
bool LaserProtocol::JoinMulticastGroups(const std::string& localIP) {
    // 确定本地IP
    std::string local = localIP.empty() ? "0.0.0.0" : localIP;
    
    // 为每个设备的所有子网加入多播组
    for (int deviceID = 0; deviceID <= m_MaxDevices; ++deviceID) {
        for (int subnetID = 0; subnetID <= 30; ++subnetID) {
            std::string multicastAddr = GetMulticastAddress(deviceID, subnetID);
            
            ip_mreq mreq;
            std::memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr.s_addr = inet_addr(multicastAddr.c_str());
            mreq.imr_interface.s_addr = inet_addr(local.c_str());
            
            if (setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                          reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
#ifdef _WIN32
                std::cerr << "Failed to join multicast group " << multicastAddr 
                         << ": " << WSAGetLastError() << std::endl;
#else
                std::cerr << "Failed to join multicast group " << multicastAddr << std::endl;
#endif
                continue;
            }
            
            m_JoinedGroups.push_back(multicastAddr);
            
            // 每2个子网暂停1ms，避免过载
            if (subnetID % 2 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
    
    std::cout << "Joined " << m_JoinedGroups.size() << " multicast groups" << std::endl;
    return !m_JoinedGroups.empty();
}

//==========================================================================
// 函数：LeaveMulticastGroups
// 描述：离开所有已加入的多播组
//==========================================================================
void LaserProtocol::LeaveMulticastGroups() {
    for (const auto& multicastAddr : m_JoinedGroups) {
        ip_mreq mreq;
        std::memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicastAddr.c_str());
        mreq.imr_interface.s_addr = INADDR_ANY;
        
        setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   reinterpret_cast<const char*>(&mreq), sizeof(mreq));
    }
    m_JoinedGroups.clear();
}

//==========================================================================
// 函数：CloseSocket
// 描述：关闭UDP Socket
//==========================================================================
void LaserProtocol::CloseSocket() {
#ifdef _WIN32
    if (m_Socket != INVALID_SOCKET) {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
#else
    if (m_Socket >= 0) {
        close(m_Socket);
        m_Socket = -1;
    }
#endif
}

//==========================================================================
// 函数：Start
// 描述：启动网络协议处理器（初始化Socket、加入多播组、启动接收线程）
// 参数：
//   localIP - 本地IP地址
// 返回值：
//   true - 启动成功
//   false - 启动失败
//==========================================================================
bool LaserProtocol::Start(const std::string& localIP) {
    if (m_Running) {
        return true;
    }
    
    // 初始化网络
    if (!InitializeWinsock()) {
        return false;
    }
    
    // 创建socket
    if (!CreateSocket()) {
        CleanupWinsock();
        return false;
    }
    
    // 加入多播组
    if (!JoinMulticastGroups(localIP)) {
        CloseSocket();
        CleanupWinsock();
        return false;
    }
    
    // 启动接收线程
    m_Running = true;
    m_ReceiveThread = std::thread(&LaserProtocol::ReceiveThread, this);
    
    std::cout << "LaserProtocol started on port " << m_Port << std::endl;
    return true;
}

//==========================================================================
// 函数：Stop
// 描述：停止网络协议处理器（停止接收线程、离开多播组、关闭Socket）
//==========================================================================
void LaserProtocol::Stop() {
    if (!m_Running) {
        return;
    }
    
    m_Running = false;
    
    // 等待接收线程结束
    if (m_ReceiveThread.joinable()) {
        m_ReceiveThread.join();
    }
    
    // 离开多播组
    LeaveMulticastGroups();
    
    // 关闭socket
    CloseSocket();
    
    // 清理网络
    CleanupWinsock();
    
    std::cout << "LaserProtocol stopped" << std::endl;
}

//==========================================================================
// 函数：ReceiveThread
// 描述：网络接收线程，使用WSARecvMsg接收UDP数据包并提取目标地址
//       通过IP_PKTINFO控制消息获取目标多播地址，从而识别设备ID
//==========================================================================
void LaserProtocol::ReceiveThread() {
    const size_t MaxPacketSize = 65536;
    std::vector<uint8_t> buffer(MaxPacketSize);
    
#ifdef _WIN32
    // 获取 WSARecvMsg 函数指针
    LPFN_WSARECVMSG WSARecvMsgFunc = nullptr;
    GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;
    DWORD dwBytes = 0;
    
    if (WSAIoctl(m_Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                 &WSARecvMsg_GUID, sizeof(WSARecvMsg_GUID),
                 &WSARecvMsgFunc, sizeof(WSARecvMsgFunc),
                 &dwBytes, nullptr, nullptr) != 0) {
        std::cerr << "Failed to get WSARecvMsg function pointer: " << WSAGetLastError() << std::endl;
        return;
    }
    std::cout << "WSARecvMsg function loaded successfully" << std::endl;
#endif
    
    while (m_Running) {
#ifdef _WIN32
        // 准备接收缓冲区
        WSABUF wsaBuf;
        wsaBuf.buf = reinterpret_cast<char*>(buffer.data());
        wsaBuf.len = static_cast<ULONG>(buffer.size());
        
        // 准备源地址缓冲区
        sockaddr_in fromAddr;
        std::memset(&fromAddr, 0, sizeof(fromAddr));
        
        // 准备控制消息缓冲区（用于接收 IP_PKTINFO）
        char controlBuf[1024];
        std::memset(controlBuf, 0, sizeof(controlBuf));
        
        // 准备 WSAMSG 结构
        WSAMSG msg;
        std::memset(&msg, 0, sizeof(msg));
        msg.name = reinterpret_cast<sockaddr*>(&fromAddr);
        msg.namelen = sizeof(fromAddr);
        msg.lpBuffers = &wsaBuf;
        msg.dwBufferCount = 1;
        msg.Control.buf = controlBuf;
        msg.Control.len = sizeof(controlBuf);
        msg.dwFlags = 0;
        
        // 接收数据包
        DWORD bytesReceived = 0;
        int result = WSARecvMsgFunc(m_Socket, &msg, &bytesReceived, nullptr, nullptr);
        
        if (result != 0 || bytesReceived == 0) {
            int error = WSAGetLastError();
            if (error != WSAEINTR && error != WSAEWOULDBLOCK && error != 0) {
                std::cerr << "WSARecvMsg failed: " << error << std::endl;
            }
            continue;
        }
        
        // 提取目标地址（从 IP_PKTINFO）
        sockaddr_in destAddr;
        std::memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        
        for (WSACMSGHDR* cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
                IN_PKTINFO* pktInfo = reinterpret_cast<IN_PKTINFO*>(WSA_CMSG_DATA(cmsg));
                destAddr.sin_addr = pktInfo->ipi_addr;
                break;
            }
        }
        
        // 从目标地址提取设备 ID
        int extractedDeviceID = -1;
        if (destAddr.sin_addr.s_addr != 0) {
            // 解析 239.255.X.Y 格式的地址
            unsigned char* addrBytes = reinterpret_cast<unsigned char*>(&destAddr.sin_addr.s_addr);
            if (addrBytes[0] == 239 && addrBytes[1] == 255) {
                extractedDeviceID = addrBytes[2];  // 第三个字节是设备 ID
            }
        }
        
#else
        sockaddr_in fromAddr;
        socklen_t fromLen = sizeof(fromAddr);
        
        int bytesReceived = recvfrom(m_Socket, 
                                     reinterpret_cast<char*>(buffer.data()), 
                                     static_cast<int>(buffer.size()),
                                     0,
                                     reinterpret_cast<sockaddr*>(&fromAddr),
                                     &fromLen);
        
        if (bytesReceived <= 0) {
            continue;
        }
        
        int extractedDeviceID = -1; // Linux 需要其他方法
#endif
        
        // 更新统计
        {
            std::lock_guard<std::mutex> lock(m_StatsMutex);
            m_Stats.PacketsReceived++;
            m_Stats.BytesReceived += bytesReceived;
            m_Stats.LastPacketSize = bytesReceived;
        }
        
        // 解析数据包（传递提取的设备 ID）
        int deviceID = -1;
        std::vector<LaserPoint> points;
        
        if (ParsePacket(buffer.data(), bytesReceived, extractedDeviceID, deviceID, points)) {
            // 调用数据回调
            std::lock_guard<std::mutex> lock(m_CallbackMutex);
            if (m_DataCallback) {
                m_DataCallback(deviceID, points);
            }
        }
    }
}

//==========================================================================
// 函数：ParsePacket
// 描述：解析Beyond激光数据包，调用linetD2_x64.dll的ReadLaserData和GetData
//       根据提取的设备ID获取对应设备的点数据
// 参数：
//   data - UDP数据包内容
//   length - 数据包长度
//   extractedDeviceID - 从目标多播地址提取的设备ID
//   deviceID - [输出] 实际解析到的设备ID
//   points - [输出] 解析出的激光点列表
// 返回值：
//   true - 解析成功并获取到点数据
//   false - 解析失败或无点数据
//==========================================================================
bool LaserProtocol::ParsePacket(const uint8_t* data, size_t length, 
                                int extractedDeviceID, int& deviceID, std::vector<LaserPoint>& points) {
    if (length == 0 || !m_ReadLaserData || !m_GetData) {
        return false;
    }
    
    // 调用 ReadLaserData 解析数据包
    std::vector<uint8_t> dataCopy(data, data + length);
    m_ReadLaserData(dataCopy.data(), static_cast<int>(length));
    
    // 使用从网络层提取的设备 ID
    if (extractedDeviceID >= 0 && extractedDeviceID < 4) {
        int pointCount = 0;
        void* pointDataPtr = m_GetData(extractedDeviceID, &pointCount);
        
        if (pointCount > 0 && pointDataPtr != nullptr) {
            deviceID = extractedDeviceID;
            
            // 解析点数据
            points.clear();
            points.reserve(pointCount);
            
            const float* floatData = reinterpret_cast<const float*>(pointDataPtr);
            
            for (int i = 0; i < pointCount; ++i) {
                // 读取位置和颜色（偏移 i*6）
                float X = floatData[i * 6 + 0];
                float Y = floatData[i * 6 + 1];
                float Focus = floatData[i * 6 + 2];
                float R = floatData[i * 6 + 3];
                float G = floatData[i * 6 + 4];
                float B = floatData[i * 6 + 5];
                
                // 归一化颜色值（0-255 → 0-1）
                if (R > 1.0f || G > 1.0f || B > 1.0f) {
                    R /= 255.0f;
                    G /= 255.0f;
                    B /= 255.0f;
                }
                
                // Y轴反转
                Y = -Y;
                
                // 颜色和Focus范围保护
                R = (std::max)(0.0f, (std::min)(1.0f, R));
                G = (std::max)(0.0f, (std::min)(1.0f, G));
                B = (std::max)(0.0f, (std::min)(1.0f, B));
                Focus = (std::max)(0.0f, (std::min)(255.0f, Focus)) / 255.0f;
                
                // 创建点（颜色属于前一个点）
                LaserPoint point(X, Y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                points.push_back(point);
                
                // 将颜色赋值给前一个点
                if (i > 0) {
                    points[points.size() - 2].R = R;
                    points[points.size() - 2].G = G;
                    points[points.size() - 2].B = B;
                    points[points.size() - 2].Focus = Focus;
                }
            }
            
            return true;
        }
    }
    
    return false;
}

} // namespace Core
} // namespace BeyondLink

