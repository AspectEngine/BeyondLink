//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：LaserRenderer.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：DirectX 11 激光渲染器实现，负责将激光点数据渲染到HDR纹理
//       使用点图元拓扑、加法混合、无深度测试
//==============================================================================

#include "LaserRenderer.h"
#include <d3dcompiler.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace BeyondLink {

//==========================================================================
// 构造函数：LaserRenderer
// 描述：初始化渲染器成员变量
// 参数：
//   settings - 激光系统配置参数
//==========================================================================
LaserRenderer::LaserRenderer(const Core::LaserSettings& settings)
    : m_Settings(settings)
    , m_Initialized(false)
    , m_Device(nullptr)
    , m_Context(nullptr)
    , m_FeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , m_AdditiveBlend(nullptr)
    , m_NoDepthState(nullptr)
    , m_NoCullingState(nullptr)
    , m_VertexShader(nullptr)
    , m_PixelShader(nullptr)
    , m_InputLayout(nullptr)
{
}

//==========================================================================
// 析构函数：~LaserRenderer
// 描述：释放渲染器资源
//==========================================================================
LaserRenderer::~LaserRenderer() {
    Shutdown();
}

//==========================================================================
// 函数：Initialize
// 描述：初始化渲染器（创建D3D设备、编译着色器、创建渲染状态）
// 参数：
//   hwnd - 窗口句柄（可选，用于交换链创建）
// 返回值：
//   true - 初始化成功
//   false - 初始化失败
//==========================================================================
bool LaserRenderer::Initialize(HWND hwnd) {
    if (m_Initialized) {
        return true;
    }

    // 创建DirectX设备
    if (!CreateDevice()) {
        std::cerr << "Failed to create D3D11 device" << std::endl;
        return false;
    }

    // 创建渲染状态
    if (!CreateRenderStates()) {
        std::cerr << "Failed to create render states" << std::endl;
        Shutdown();
        return false;
    }

    // 编译着色器
    if (!CompileShaders()) {
        std::cerr << "Failed to compile shaders" << std::endl;
        Shutdown();
        return false;
    }

    // 创建常量缓冲
    if (!CreateConstantBuffers()) {
        std::cerr << "Failed to create constant buffers" << std::endl;
        Shutdown();
        return false;
    }

    // 设置视口
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = static_cast<float>(m_Settings.TextureSize);
    m_Viewport.Height = static_cast<float>(m_Settings.TextureSize);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    m_Initialized = true;
    std::cout << "LaserRenderer initialized successfully" << std::endl;
    return true;
}

//==========================================================================
// 函数：Shutdown
// 描述：释放所有D3D资源（纹理、缓冲、着色器、渲染状态）
//==========================================================================
void LaserRenderer::Shutdown() {
    // 释放所有激光源资源
    for (auto& pair : m_SourceResources) {
        DestroySourceResources(pair.first);
    }
    m_SourceResources.clear();

    // 释放着色器
    if (m_InputLayout) {
        m_InputLayout->Release();
        m_InputLayout = nullptr;
    }
    if (m_PixelShader) {
        m_PixelShader->Release();
        m_PixelShader = nullptr;
    }
    if (m_VertexShader) {
        m_VertexShader->Release();
        m_VertexShader = nullptr;
    }

    // 释放渲染状态
    if (m_NoCullingState) {
        m_NoCullingState->Release();
        m_NoCullingState = nullptr;
    }
    if (m_NoDepthState) {
        m_NoDepthState->Release();
        m_NoDepthState = nullptr;
    }
    if (m_AdditiveBlend) {
        m_AdditiveBlend->Release();
        m_AdditiveBlend = nullptr;
    }

    // 释放设备和上下文
    if (m_Context) {
        m_Context->Release();
        m_Context = nullptr;
    }
    if (m_Device) {
        m_Device->Release();
        m_Device = nullptr;
    }

    m_Initialized = false;
}

bool LaserRenderer::CreateDevice() {
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // 默认适配器
        D3D_DRIVER_TYPE_HARDWARE,   // 硬件加速
        nullptr,                    // 软件光栅化器
        createDeviceFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_Device,
        &m_FeatureLevel,
        &m_Context
    );

    if (FAILED(hr)) {
        std::cerr << "D3D11CreateDevice failed: " << std::hex << hr << std::endl;
        return false;
    }

    std::cout << "Created D3D11 device with feature level: " 
              << (m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 ? "11.1" : "11.0") << std::endl;
    return true;
}

bool LaserRenderer::CreateRenderStates() {
    HRESULT hr;

    // 创建加法混合状态
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = m_Device->CreateBlendState(&blendDesc, &m_AdditiveBlend);
    if (FAILED(hr)) {
        return false;
    }

    // 创建无深度测试状态
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

    hr = m_Device->CreateDepthStencilState(&depthDesc, &m_NoDepthState);
    if (FAILED(hr)) {
        return false;
    }

    // 创建无剔除光栅化状态
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = FALSE;

    hr = m_Device->CreateRasterizerState(&rasterDesc, &m_NoCullingState);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool LaserRenderer::CompileShaders() {
    // 简单的着色器代码（内联）
    const char* vsCode = R"(
        struct VSInput {
            float2 Position : POSITION;
            float3 Color : COLOR;
            float Focus : TEXCOORD0;
        };
        
        struct VSOutput {
            float4 Position : SV_POSITION;
            float3 Color : COLOR;
            float Focus : TEXCOORD0;
        };
        
        VSOutput main(VSInput input) {
            VSOutput output;
            output.Position = float4(input.Position, 0.0, 1.0);
            output.Color = input.Color;
            output.Focus = input.Focus;
            return output;
        }
    )";

    const char* psCode = R"(
        struct PSInput {
            float4 Position : SV_POSITION;
            float3 Color : COLOR;
            float Focus : TEXCOORD0;
        };
        
        float4 main(PSInput input) : SV_TARGET {
            return float4(input.Color, 1.0);
        }
    )";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr;

    // 编译顶点着色器
    hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr,
                    "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    
    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "VS compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return false;
    }

    hr = m_Device->CreateVertexShader(vsBlob->GetBufferPointer(), 
                                       vsBlob->GetBufferSize(), 
                                       nullptr, &m_VertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    // 创建输入布局（LaserPoint: X,Y,R,G,B,Z,Focus = 28字节）
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },       // X, Y at offset 0
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },      // R, G, B at offset 8
        { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },        // Focus at offset 24 (not 20!)
    };

    hr = m_Device->CreateInputLayout(layout, ARRAYSIZE(layout),
                                       vsBlob->GetBufferPointer(),
                                       vsBlob->GetBufferSize(),
                                       &m_InputLayout);
    vsBlob->Release();
    if (FAILED(hr)) {
        return false;
    }

    // 编译像素着色器
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
                    "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    
    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "PS compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return false;
    }

    hr = m_Device->CreatePixelShader(psBlob->GetBufferPointer(), 
                                       psBlob->GetBufferSize(), 
                                       nullptr, &m_PixelShader);
    psBlob->Release();
    if (FAILED(hr)) {
        return false;
    }

    std::cout << "Shaders compiled successfully" << std::endl;
    return true;
}

bool LaserRenderer::CreateConstantBuffers() {
    // 常量缓冲暂时未使用
    return true;
}

void LaserRenderer::AddLaserSource(int deviceID, std::shared_ptr<Core::LaserSource> source) {
    if (m_SourceResources.find(deviceID) != m_SourceResources.end()) {
        DestroySourceResources(deviceID);
    }

    if (CreateSourceResources(deviceID)) {
        m_SourceResources[deviceID].Source = source;
        std::cout << "Added laser source for device " << deviceID << std::endl;
    }
}

void LaserRenderer::RemoveLaserSource(int deviceID) {
    auto it = m_SourceResources.find(deviceID);
    if (it != m_SourceResources.end()) {
        DestroySourceResources(deviceID);
        m_SourceResources.erase(it);
    }
}

std::shared_ptr<Core::LaserSource> LaserRenderer::GetLaserSource(int deviceID) {
    auto it = m_SourceResources.find(deviceID);
    if (it != m_SourceResources.end()) {
        return it->second.Source;
    }
    return nullptr;
}

bool LaserRenderer::CreateSourceResources(int deviceID) {
    SourceResources resources = {};

    // 创建渲染目标纹理
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_Settings.TextureSize;
    texDesc.Height = m_Settings.TextureSize;
    texDesc.MipLevels = m_Settings.EnableMipmaps ? 0 : 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = m_Settings.EnableMipmaps ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

    HRESULT hr = m_Device->CreateTexture2D(&texDesc, nullptr, &resources.Texture);
    if (FAILED(hr)) {
        return false;
    }

    // 创建RTV
    hr = m_Device->CreateRenderTargetView(resources.Texture, nullptr, &resources.RTV);
    if (FAILED(hr)) {
        resources.Texture->Release();
        return false;
    }

    // 创建SRV
    hr = m_Device->CreateShaderResourceView(resources.Texture, nullptr, &resources.SRV);
    if (FAILED(hr)) {
        resources.RTV->Release();
        resources.Texture->Release();
        return false;
    }

    // 创建顶点缓冲
    resources.VertexCapacity = 10000;
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = static_cast<UINT>(resources.VertexCapacity * sizeof(Core::LaserPoint));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_Device->CreateBuffer(&vbDesc, nullptr, &resources.VertexBuffer);
    if (FAILED(hr)) {
        resources.SRV->Release();
        resources.RTV->Release();
        resources.Texture->Release();
        return false;
    }

    m_SourceResources[deviceID] = resources;
    return true;
}

void LaserRenderer::DestroySourceResources(int deviceID) {
    auto it = m_SourceResources.find(deviceID);
    if (it == m_SourceResources.end()) {
        return;
    }

    auto& resources = it->second;
    if (resources.VertexBuffer) resources.VertexBuffer->Release();
    if (resources.SRV) resources.SRV->Release();
    if (resources.RTV) resources.RTV->Release();
    if (resources.Texture) resources.Texture->Release();
}

//==========================================================================
// 函数：RenderAll
// 描述：渲染所有激光源到各自的纹理
//==========================================================================
void LaserRenderer::RenderAll() {
    if (!m_Initialized) {
        return;
    }

    for (auto& pair : m_SourceResources) {
        int deviceID = pair.first;
        auto& resources = pair.second;
        
        if (resources.Source) {
            RenderSource(deviceID, resources.Source.get());
        }
    }
}

//==========================================================================
// 函数：RenderSource
// 描述：渲染单个激光源到其专用纹理
//       - 上传顶点数据到动态顶点缓冲
//       - 设置渲染目标为设备纹理
//       - 清除为黑色
//       - 使用加法混合绘制点云
// 参数：
//   deviceID - 设备ID
//   source - 激光源对象指针
//==========================================================================
void LaserRenderer::RenderSource(int deviceID, Core::LaserSource* source) {
    auto it = m_SourceResources.find(deviceID);
    if (it == m_SourceResources.end()) {
        return;
    }

    auto& resources = it->second;
    
    // 获取处理后的点数据
    std::lock_guard<std::mutex> lock(source->GetMutex());
    const auto& points = source->GetProcessedPoints();
    
    if (points.empty()) {
        return;
    }

    // 上传顶点数据
    UploadVertexData(points, resources.VertexBuffer, resources.VertexCapacity);

    // 保存当前渲染状态
    ID3D11RenderTargetView* oldRTV = nullptr;
    ID3D11DepthStencilView* oldDSV = nullptr;
    m_Context->OMGetRenderTargets(1, &oldRTV, &oldDSV);

    // 设置渲染目标
    ID3D11RenderTargetView* rtvs[] = { resources.RTV };
    m_Context->OMSetRenderTargets(1, rtvs, nullptr);

    // 清除渲染目标
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_Context->ClearRenderTargetView(resources.RTV, clearColor);

    // 设置视口
    m_Context->RSSetViewports(1, &m_Viewport);

    // 设置渲染状态
    m_Context->OMSetBlendState(m_AdditiveBlend, nullptr, 0xffffffff);
    m_Context->OMSetDepthStencilState(m_NoDepthState, 0);
    m_Context->RSSetState(m_NoCullingState);

    // 设置着色器
    m_Context->VSSetShader(m_VertexShader, nullptr, 0);
    m_Context->PSSetShader(m_PixelShader, nullptr, 0);

    // 设置输入布局和拓扑
    m_Context->IASetInputLayout(m_InputLayout);
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    // 设置顶点缓冲
    UINT stride = sizeof(Core::LaserPoint);
    UINT offset = 0;
    ID3D11Buffer* buffers[] = { resources.VertexBuffer };
    m_Context->IASetVertexBuffers(0, 1, buffers, &stride, &offset);

    // 绘制
    UINT vertexCount = static_cast<UINT>(points.size());
    m_Context->Draw(vertexCount, 0);

    // 生成Mipmap
    if (m_Settings.EnableMipmaps) {
        m_Context->GenerateMips(resources.SRV);
    }

    // 恢复渲染状态
    if (oldRTV || oldDSV) {
        ID3D11RenderTargetView* rtvs[] = { oldRTV };
        m_Context->OMSetRenderTargets(1, oldRTV ? rtvs : nullptr, oldDSV);
    }
    if (oldRTV) oldRTV->Release();
    if (oldDSV) oldDSV->Release();
}

void LaserRenderer::UploadVertexData(const std::vector<Core::LaserPoint>& points, 
                                      ID3D11Buffer*& vertexBuffer, 
                                      size_t& bufferCapacity) {
    if (points.empty()) {
        return;
    }

    // 如果缓冲区不够大，重新创建更大的缓冲区
    if (points.size() > bufferCapacity) {
        // 计算新容量（增加余量避免频繁重建）
        bufferCapacity = points.size() + 5000;
        
        // 释放旧缓冲区
        if (vertexBuffer) {
            vertexBuffer->Release();
            vertexBuffer = nullptr;
        }
        
        // 创建新的更大缓冲区
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = static_cast<UINT>(bufferCapacity * sizeof(Core::LaserPoint));
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        HRESULT hr = m_Device->CreateBuffer(&vbDesc, nullptr, &vertexBuffer);
        if (FAILED(hr)) {
            std::cerr << "Failed to recreate vertex buffer with capacity: " << bufferCapacity << std::endl;
            return;
        }
        
        std::cout << "Expanded vertex buffer to capacity: " << bufferCapacity << " points" << std::endl;
    }

    // 映射缓冲区并上传数据
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_Context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        std::memcpy(mapped.pData, points.data(), points.size() * sizeof(Core::LaserPoint));
        m_Context->Unmap(vertexBuffer, 0);
    }
}

ID3D11ShaderResourceView* LaserRenderer::GetLaserTexture(int deviceID) {
    auto it = m_SourceResources.find(deviceID);
    if (it != m_SourceResources.end()) {
        return it->second.SRV;
    }
    return nullptr;
}

} // namespace BeyondLink

