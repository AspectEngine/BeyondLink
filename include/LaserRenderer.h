//==============================================================================
// 文件：LaserRenderer.h
// 作者：Yunsio
// 日期：2025-10-06
// 描述：DirectX 11 激光渲染器
//      使用 GPU 硬件加速渲染激光点云
//      支持 HDR 纹理、加法混合、多设备并行渲染
//==============================================================================

#pragma once

// DirectX must be included AFTER winsock2 (via LaserProtocol.h)
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "LaserPoint.h"
#include "LaserSource.h"
#include "LaserSettings.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <d3d11.h>
#include <directxmath.h>

namespace BeyondLink {

//==========================================================================
// 类：LaserRenderer
// 描述：DirectX 11 激光渲染器核心类
//      管理 D3D11 设备、着色器、纹理和渲染管线
//      为每个激光设备创建独立的渲染目标纹理
//==========================================================================
class LaserRenderer {
public:
    //==========================================================================
    // 构造函数：LaserRenderer
    // 描述：创建激光渲染器
    // 参数：
    //   settings - 激光系统配置参数
    //==========================================================================
    explicit LaserRenderer(const Core::LaserSettings& settings);
    ~LaserRenderer();

    //==========================================================================
    // 函数：Initialize
    // 描述：初始化 DirectX 11 渲染器
    //      创建设备、编译着色器、设置渲染状态
    // 参数：
    //   hwnd - 窗口句柄（可选，nullptr 表示无窗口模式）
    // 返回值：
    //   true - 初始化成功
    //   false - 初始化失败
    //==========================================================================
    bool Initialize(HWND hwnd = nullptr);
    
    //==========================================================================
    // 函数：Shutdown
    // 描述：释放所有 DirectX 资源
    //==========================================================================
    void Shutdown();
    
    //==========================================================================
    // 函数：IsInitialized
    // 描述：检查渲染器是否已初始化
    // 返回值：
    //   true - 已初始化
    //   false - 未初始化
    //==========================================================================
    bool IsInitialized() const { return m_Initialized; }

    //==========================================================================
    // 函数：AddLaserSource
    // 描述：添加激光源并为其创建渲染资源
    //      为指定设备创建：纹理、RTV、SRV、顶点缓冲
    // 参数：
    //   deviceID - 设备 ID (0-3)
    //   source - 激光源智能指针
    //==========================================================================
    void AddLaserSource(int deviceID, std::shared_ptr<Core::LaserSource> source);
    
    //==========================================================================
    // 函数：RemoveLaserSource
    // 描述：移除激光源并释放其渲染资源
    // 参数：
    //   deviceID - 设备 ID
    //==========================================================================
    void RemoveLaserSource(int deviceID);
    
    //==========================================================================
    // 函数：GetLaserSource
    // 描述：获取指定设备的激光源
    // 参数：
    //   deviceID - 设备 ID
    // 返回值：
    //   激光源智能指针（如果不存在则返回 nullptr）
    //==========================================================================
    std::shared_ptr<Core::LaserSource> GetLaserSource(int deviceID);

    //==========================================================================
    // 函数：RenderAll
    // 描述：渲染所有激光源到各自的纹理
    //      遍历所有设备，将点云渲染到对应的 HDR 纹理
    //==========================================================================
    void RenderAll();

    //==========================================================================
    // 函数：GetLaserTexture
    // 描述：获取指定设备的渲染纹理 SRV（用于显示）
    // 参数：
    //   deviceID - 设备 ID
    // 返回值：
    //   纹理的 ShaderResourceView 指针（如果不存在则返回 nullptr）
    //==========================================================================
    ID3D11ShaderResourceView* GetLaserTexture(int deviceID);

    //==========================================================================
    // 函数：GetDevice / GetContext
    // 描述：获取 DirectX 11 设备和上下文指针
    // 返回值：
    //   D3D11 设备/上下文指针
    //==========================================================================
    ID3D11Device* GetDevice() const { return m_Device; }
    ID3D11DeviceContext* GetContext() const { return m_Context; }

private:
    //==========================================================================
    // 函数：CreateDevice
    // 描述：创建 DirectX 11 设备和上下文
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateDevice();
    
    //==========================================================================
    // 函数：CreateRenderStates
    // 描述：创建渲染状态（混合、深度、光栅化）
    //      - 加法混合（SrcBlend=ONE, DestBlend=ONE）
    //      - 无深度测试
    //      - 无背面剔除
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateRenderStates();
    
    //==========================================================================
    // 函数：CompileShaders
    // 描述：编译顶点着色器和像素着色器
    //      使用内联 HLSL 代码（不依赖外部文件）
    // 返回值：
    //   true - 编译成功
    //   false - 编译失败
    //==========================================================================
    bool CompileShaders();
    
    //==========================================================================
    // 函数：CreateConstantBuffers
    // 描述：创建常量缓冲区（预留，当前未使用）
    // 返回值：
    //   true - 创建成功
    //==========================================================================
    bool CreateConstantBuffers();

    //==========================================================================
    // 函数：CreateSourceResources
    // 描述：为指定激光源创建渲染资源
    //      创建 HDR 纹理、RTV、SRV、动态顶点缓冲
    // 参数：
    //   deviceID - 设备 ID
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateSourceResources(int deviceID);
    
    //==========================================================================
    // 函数：DestroySourceResources
    // 描述：释放指定激光源的渲染资源
    // 参数：
    //   deviceID - 设备 ID
    //==========================================================================
    void DestroySourceResources(int deviceID);

    //==========================================================================
    // 函数：RenderSource
    // 描述：渲染单个激光源到其纹理
    //      1. 上传顶点数据到 GPU
    //      2. 设置渲染状态和着色器
    //      3. 绘制点云（POINTLIST）
    //      4. 生成 Mipmap（可选）
    // 参数：
    //   deviceID - 设备 ID
    //   source - 激光源指针
    //==========================================================================
    void RenderSource(int deviceID, Core::LaserSource* source);

    //==========================================================================
    // 函数：UploadVertexData
    // 描述：上传顶点数据到 GPU 动态缓冲区
    //      使用 MAP_WRITE_DISCARD 高效更新
    //      如果缓冲区不够大，会自动重新创建更大的缓冲区
    // 参数：
    //   points - 激光点列表
    //   vertexBuffer - 顶点缓冲区（引用，可能会被重新创建）
    //   bufferCapacity - 缓冲区容量（引用，扩展时会更新）
    //==========================================================================
    void UploadVertexData(const std::vector<Core::LaserPoint>& points, 
                          ID3D11Buffer*& vertexBuffer, 
                          size_t& bufferCapacity);

private:
    Core::LaserSettings m_Settings;              // 系统配置
    bool m_Initialized;                          // 初始化标志

    // DirectX 11 核心对象
    ID3D11Device* m_Device;                      // D3D11 设备
    ID3D11DeviceContext* m_Context;              // D3D11 设备上下文
    D3D_FEATURE_LEVEL m_FeatureLevel;            // 功能级别（11.0 或 11.1）

    // 渲染状态
    ID3D11BlendState* m_AdditiveBlend;           // 加法混合状态
    ID3D11DepthStencilState* m_NoDepthState;     // 无深度测试状态
    ID3D11RasterizerState* m_NoCullingState;     // 无剔除光栅化状态

    // 着色器
    ID3D11VertexShader* m_VertexShader;          // 顶点着色器
    ID3D11PixelShader* m_PixelShader;            // 像素着色器
    ID3D11InputLayout* m_InputLayout;            // 输入布局

    //==========================================================================
    // 结构体：SourceResources
    // 描述：单个激光源的渲染资源集合
    //==========================================================================
    struct SourceResources {
        ID3D11Texture2D* Texture;                // 渲染目标纹理（HDR）
        ID3D11RenderTargetView* RTV;             // 渲染目标视图
        ID3D11ShaderResourceView* SRV;           // 着色器资源视图
        ID3D11Buffer* VertexBuffer;              // 动态顶点缓冲区
        size_t VertexCapacity;                   // 顶点缓冲区容量
        std::shared_ptr<Core::LaserSource> Source;  // 激光源智能指针
    };
    std::unordered_map<int, SourceResources> m_SourceResources;  // 设备ID → 资源映射

    // 视口
    D3D11_VIEWPORT m_Viewport;                   // 渲染视口
};

} // namespace BeyondLink
