//==============================================================================
// 文件：BeyondLinkCAPI.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：BeyondLink C API 实现
//      将C++类接口封装为C函数，供UE插件调用
//==============================================================================

#include "../include/BeyondLinkCAPI.h"
#include "../include/BeyondLink.h"
#include <iostream>

using namespace BeyondLink;

//==========================================================================
// 生命周期管理
//==========================================================================

BeyondLinkHandle BeyondLink_Create() {
    try {
        // 使用默认配置创建系统
        Core::LaserSettings settings;
        settings.MaxLaserDevices = 8;
        settings.NetworkPort = 5568;
        settings.TextureSize = 1024;
        settings.ScannerSimulation = true;
        settings.EnableMipmaps = true;
        settings.EdgeFade = 0.1f;
        settings.VelocitySmoothing = 0.83f;
        settings.LaserQuality = Core::LaserSettings::QualityLevel::High;
        
        auto* system = new BeyondLinkSystem(settings);
        return reinterpret_cast<BeyondLinkHandle>(system);
    } catch (const std::exception& e) {
        std::cerr << "BeyondLink_Create failed: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cerr << "BeyondLink_Create failed: unknown error" << std::endl;
        return nullptr;
    }
}

void BeyondLink_Destroy(BeyondLinkHandle handle) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        delete system;
    } catch (...) {
        std::cerr << "BeyondLink_Destroy failed" << std::endl;
    }
}

int BeyondLink_Initialize(BeyondLinkHandle handle) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        return system->Initialize(nullptr) ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "BeyondLink_Initialize failed: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "BeyondLink_Initialize failed: unknown error" << std::endl;
        return 0;
    }
}

void BeyondLink_Shutdown(BeyondLinkHandle handle) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->Shutdown();
    } catch (...) {
        std::cerr << "BeyondLink_Shutdown failed" << std::endl;
    }
}

//==========================================================================
// 网络控制
//==========================================================================

int BeyondLink_StartNetwork(BeyondLinkHandle handle, const char* localIP) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        std::string ip = (localIP && localIP[0] != '\0') ? localIP : "";
        return system->StartNetworkReceiver(ip) ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "BeyondLink_StartNetwork failed: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "BeyondLink_StartNetwork failed: unknown error" << std::endl;
        return 0;
    }
}

void BeyondLink_StopNetwork(BeyondLinkHandle handle) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->StopNetworkReceiver();
    } catch (...) {
        std::cerr << "BeyondLink_StopNetwork failed" << std::endl;
    }
}

int BeyondLink_IsRunning(BeyondLinkHandle handle) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        return system->IsRunning() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

//==========================================================================
// 更新和渲染
//==========================================================================

void BeyondLink_Update(BeyondLinkHandle handle) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->Update();
    } catch (...) {
        std::cerr << "BeyondLink_Update failed" << std::endl;
    }
}

void BeyondLink_Render(BeyondLinkHandle handle) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->Render();
    } catch (...) {
        std::cerr << "BeyondLink_Render failed" << std::endl;
    }
}

//==========================================================================
// 纹理访问（UE集成关键接口）
//==========================================================================

void* BeyondLink_GetD3D11Device(BeyondLinkHandle handle) {
    if (!handle) return nullptr;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto* renderer = system->GetRenderer();
        return renderer ? renderer->GetDevice() : nullptr;
    } catch (...) {
        return nullptr;
    }
}

void* BeyondLink_GetD3D11Context(BeyondLinkHandle handle) {
    if (!handle) return nullptr;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto* renderer = system->GetRenderer();
        return renderer ? renderer->GetContext() : nullptr;
    } catch (...) {
        return nullptr;
    }
}

void* BeyondLink_GetTexture2D(BeyondLinkHandle handle, int deviceID) {
    if (!handle) return nullptr;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto* renderer = system->GetRenderer();
        return renderer ? renderer->GetLaserTextureRaw(deviceID) : nullptr;
    } catch (...) {
        return nullptr;
    }
}

void* BeyondLink_GetSharedHandle(BeyondLinkHandle handle, int deviceID) {
    if (!handle) return nullptr;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto* renderer = system->GetRenderer();
        return renderer ? renderer->GetSharedTextureHandle(deviceID) : nullptr;
    } catch (...) {
        return nullptr;
    }
}

void* BeyondLink_GetTextureSRV(BeyondLinkHandle handle, int deviceID) {
    if (!handle) return nullptr;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        return system->GetLaserTexture(deviceID);
    } catch (...) {
        return nullptr;
    }
}

//==========================================================================
// 配置接口
//==========================================================================

void BeyondLink_SetTextureSize(BeyondLinkHandle handle, int size) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->GetSettings().TextureSize = size;
    } catch (...) {
        std::cerr << "BeyondLink_SetTextureSize failed" << std::endl;
    }
}

void BeyondLink_SetQuality(BeyondLinkHandle handle, int level) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        switch (level) {
            case 0:
                system->GetSettings().LaserQuality = Core::LaserSettings::QualityLevel::Low;
                break;
            case 1:
                system->GetSettings().LaserQuality = Core::LaserSettings::QualityLevel::Medium;
                break;
            case 2:
                system->GetSettings().LaserQuality = Core::LaserSettings::QualityLevel::High;
                break;
            case 3:
                system->GetSettings().LaserQuality = Core::LaserSettings::QualityLevel::Ultra;
                break;
            default:
                system->GetSettings().LaserQuality = Core::LaserSettings::QualityLevel::High;
                break;
        }
    } catch (...) {
        std::cerr << "BeyondLink_SetQuality failed" << std::endl;
    }
}

void BeyondLink_EnableScannerSimulation(BeyondLinkHandle handle, int enable) {
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        system->GetSettings().ScannerSimulation = (enable != 0);
    } catch (...) {
        std::cerr << "BeyondLink_EnableScannerSimulation failed" << std::endl;
    }
}

//==========================================================================
// 状态查询
//==========================================================================

int BeyondLink_GetPointCount(BeyondLinkHandle handle, int deviceID) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto source = system->GetLaserSource(deviceID);
        return source ? static_cast<int>(source->GetPointCount()) : 0;
    } catch (...) {
        return 0;
    }
}

void BeyondLink_GetNetworkStats(BeyondLinkHandle handle, 
    unsigned long long* packetsReceived, 
    unsigned long long* bytesReceived) {
    
    if (!handle) return;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        auto stats = system->GetNetworkStats();
        
        if (packetsReceived) {
            *packetsReceived = stats.PacketsReceived;
        }
        if (bytesReceived) {
            *bytesReceived = stats.BytesReceived;
        }
    } catch (...) {
        std::cerr << "BeyondLink_GetNetworkStats failed" << std::endl;
    }
}

//==========================================================================
// 纹理信息查询
//==========================================================================

int BeyondLink_GetTextureWidth(BeyondLinkHandle handle) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        return system->GetSettings().TextureSize;
    } catch (...) {
        return 0;
    }
}

int BeyondLink_GetTextureHeight(BeyondLinkHandle handle) {
    if (!handle) return 0;
    
    try {
        auto* system = reinterpret_cast<BeyondLinkSystem*>(handle);
        return system->GetSettings().TextureSize;
    } catch (...) {
        return 0;
    }
}

