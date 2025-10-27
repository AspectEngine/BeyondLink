//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：BeyondLink.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：BeyondLink系统的主实现文件，协调渲染器、网络协议和激光源管理
//==============================================================================

#include "BeyondLink.h"
#include <iostream>

namespace BeyondLink {

//==========================================================================
// 构造函数：BeyondLinkSystem
// 描述：初始化BeyondLink系统
// 参数：
//   settings - 激光系统配置参数
//==========================================================================
BeyondLinkSystem::BeyondLinkSystem(const Core::LaserSettings& settings)
    : m_Settings(settings)
    , m_Initialized(false)
{
}

//==========================================================================
// 析构函数：~BeyondLinkSystem
// 描述：清理系统资源
//==========================================================================
BeyondLinkSystem::~BeyondLinkSystem() {
    Shutdown();
}

//==========================================================================
// 函数：Initialize
// 描述：初始化整个BeyondLink系统（渲染器、网络协议、激光源）
// 参数：
//   hwnd - 窗口句柄（可选，用于D3D设备创建）
// 返回值：
//   true - 初始化成功
//   false - 初始化失败
//==========================================================================
bool BeyondLinkSystem::Initialize(HWND hwnd) {
    if (m_Initialized) {
        return true;
    }

    std::cout << "=== BeyondLink System Initializing ===" << std::endl;

    // 创建并初始化渲染器
    m_Renderer = std::make_unique<LaserRenderer>(m_Settings);
    if (!m_Renderer->Initialize(hwnd)) {
        std::cerr << "Failed to initialize LaserRenderer" << std::endl;
        return false;
    }

    // 创建网络协议处理器
    m_Protocol = std::make_unique<Core::LaserProtocol>(m_Settings);
    
    // 设置数据回调
    m_Protocol->SetDataCallback([this](int deviceID, const std::vector<Core::LaserPoint>& points) {
        OnLaserDataReceived(deviceID, points);
    });

    // 为所有设备创建激光源
    for (int i = 0; i < m_Settings.MaxLaserDevices; ++i) {
        EnsureLaserSource(i);
    }

    m_Initialized = true;
    std::cout << "BeyondLink System initialized successfully" << std::endl;
    std::cout << "- Max Devices: " << m_Settings.MaxLaserDevices << std::endl;
    std::cout << "- Network Port: " << m_Settings.NetworkPort << std::endl;
    std::cout << "- Texture Size: " << m_Settings.TextureSize << std::endl;
    std::cout << "- Scanner Simulation: " << (m_Settings.ScannerSimulation ? "Enabled" : "Disabled") << std::endl;
    
    return true;
}

//==========================================================================
// 函数：Shutdown
// 描述：关闭系统，释放所有资源（网络、渲染器、激光源）
//==========================================================================
void BeyondLinkSystem::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    std::cout << "BeyondLink System shutting down..." << std::endl;

    // 停止网络接收
    StopNetworkReceiver();

    // 清理激光源
    {
        std::lock_guard<std::mutex> lock(m_SourcesMutex);
        m_LaserSources.clear();
    }

    // 关闭渲染器
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }

    // 清理协议处理器
    m_Protocol.reset();

    m_Initialized = false;
    std::cout << "BeyondLink System shutdown complete" << std::endl;
}

//==========================================================================
// 函数：IsRunning
// 描述：检查系统是否正在运行
// 返回值：
//   true - 系统已初始化且网络协议正在运行
//   false - 系统未运行
//==========================================================================
bool BeyondLinkSystem::IsRunning() const {
    return m_Initialized && m_Protocol && m_Protocol->IsRunning();
}

//==========================================================================
// 函数：StartNetworkReceiver
// 描述：启动UDP网络接收器，开始监听Beyond数据
// 参数：
//   localIP - 本地IP地址（空字符串表示INADDR_ANY）
// 返回值：
//   true - 启动成功
//   false - 启动失败
//==========================================================================
bool BeyondLinkSystem::StartNetworkReceiver(const std::string& localIP) {
    if (!m_Initialized || !m_Protocol) {
        std::cerr << "System not initialized" << std::endl;
        return false;
    }

    if (m_Protocol->IsRunning()) {
        std::cout << "Network receiver already running" << std::endl;
        return true;
    }

    if (m_Protocol->Start(localIP)) {
        std::cout << "Network receiver started on port " << m_Protocol->GetPort() << std::endl;
        return true;
    }

    std::cerr << "Failed to start network receiver" << std::endl;
    return false;
}

//==========================================================================
// 函数：StopNetworkReceiver
// 描述：停止网络接收器
//==========================================================================
void BeyondLinkSystem::StopNetworkReceiver() {
    if (m_Protocol && m_Protocol->IsRunning()) {
        m_Protocol->Stop();
        std::cout << "Network receiver stopped" << std::endl;
    }
}

//==========================================================================
// 函数：Update
// 描述：更新所有激光源的点数据处理（应用扫描仪模拟等）
//==========================================================================
void BeyondLinkSystem::Update() {
    if (!m_Initialized) {
        return;
    }

    // 更新所有激光源的点数据处理
    std::lock_guard<std::mutex> lock(m_SourcesMutex);
    for (auto& pair : m_LaserSources) {
        auto& source = pair.second;
        if (source) {
            source->UpdatePointList(m_Settings.ScannerSimulation);
        }
    }
}

//==========================================================================
// 函数：Render
// 描述：渲染所有激光源到各自的纹理
//==========================================================================
void BeyondLinkSystem::Render() {
    if (!m_Initialized || !m_Renderer) {
        return;
    }

    // 渲染所有激光源
    m_Renderer->RenderAll();
}

//==========================================================================
// 函数：GetLaserTexture
// 描述：获取指定设备的激光纹理（用于显示）
// 参数：
//   deviceID - 设备ID (0-3)
// 返回值：
//   ID3D11ShaderResourceView* - 激光纹理的SRV指针，失败返回nullptr
//==========================================================================
ID3D11ShaderResourceView* BeyondLinkSystem::GetLaserTexture(int deviceID) {
    if (!m_Initialized || !m_Renderer) {
        return nullptr;
    }

    return m_Renderer->GetLaserTexture(deviceID);
}

//==========================================================================
// 函数：GetLaserSource
// 描述：获取指定设备的激光源对象
// 参数：
//   deviceID - 设备ID (0-3)
// 返回值：
//   shared_ptr<LaserSource> - 激光源对象指针，不存在返回nullptr
//==========================================================================
std::shared_ptr<Core::LaserSource> BeyondLinkSystem::GetLaserSource(int deviceID) {
    std::lock_guard<std::mutex> lock(m_SourcesMutex);
    
    auto it = m_LaserSources.find(deviceID);
    if (it != m_LaserSources.end()) {
        return it->second;
    }
    
    return nullptr;
}

//==========================================================================
// 函数：GetNetworkStats
// 描述：获取网络统计信息（接收的数据包数、字节数等）
// 返回值：
//   NetworkStats - 网络统计结构体
//==========================================================================
Core::LaserProtocol::NetworkStats BeyondLinkSystem::GetNetworkStats() const {
    if (m_Protocol) {
        return m_Protocol->GetStats();
    }
    return Core::LaserProtocol::NetworkStats();
}

//==========================================================================
// 函数：OnLaserDataReceived
// 描述：网络数据接收回调函数，将接收到的点数据传递给对应的激光源
// 参数：
//   deviceID - 设备ID
//   points - 激光点数据列表
//==========================================================================
void BeyondLinkSystem::OnLaserDataReceived(int deviceID, const std::vector<Core::LaserPoint>& points) {
    // 确保激光源存在
    EnsureLaserSource(deviceID);

    // 更新激光源的点数据
    std::lock_guard<std::mutex> lock(m_SourcesMutex);
    auto it = m_LaserSources.find(deviceID);
    if (it != m_LaserSources.end()) {
        it->second->SetPointList(points);
    }
}

//==========================================================================
// 函数：EnsureLaserSource
// 描述：确保指定设备的激光源已创建，不存在则创建新的激光源
// 参数：
//   deviceID - 设备ID
//==========================================================================
void BeyondLinkSystem::EnsureLaserSource(int deviceID) {
    std::lock_guard<std::mutex> lock(m_SourcesMutex);
    
    if (m_LaserSources.find(deviceID) == m_LaserSources.end()) {
        // 创建新的激光源
        auto source = std::make_shared<Core::LaserSource>(deviceID, m_Settings);
        m_LaserSources[deviceID] = source;
        
        // 添加到渲染器
        if (m_Renderer) {
            m_Renderer->AddLaserSource(deviceID, source);
        }
        
        std::cout << "Created laser source for device " << deviceID << std::endl;
    }
}

} // namespace BeyondLink
