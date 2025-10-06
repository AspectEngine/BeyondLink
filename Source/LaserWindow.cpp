//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：LaserWindow.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：显示窗口实现，负责创建Win32窗口、SwapChain、全屏四边形渲染
//       将激光纹理显示到屏幕，并计算FPS显示在标题栏
//==============================================================================

#include "../include/LaserWindow.h"
#include <d3dcompiler.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace BeyondLink {

//==========================================================================
// 构造函数：LaserWindow
// 描述：初始化窗口参数和FPS计时器
// 参数：
//   width - 窗口宽度
//   height - 窗口高度
//   title - 窗口标题
//==========================================================================
LaserWindow::LaserWindow(int width, int height, const std::string& title)
    : m_Width(width)
    , m_Height(height)
    , m_Title(title)
    , m_BaseTitle(title)
    , m_hWnd(nullptr)
    , m_ShouldClose(false)
    , m_CurrentFPS(0.0f)
    , m_FrameCount(0)
    , m_LastFPSUpdate(0.0)
    , m_FPSUpdateInterval(0.5)  // 每 0.5 秒更新一次 FPS
    , m_Device(nullptr)
    , m_Context(nullptr)
    , m_SwapChain(nullptr)
    , m_BackBufferRTV(nullptr)
    , m_FullscreenQuadVB(nullptr)
    , m_DisplayVS(nullptr)
    , m_DisplayPS(nullptr)
    , m_SamplerState(nullptr)
    , m_InputLayout(nullptr)
{
    // 初始化高精度计时器
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    // 将计数器转换为秒（QuadPart / Frequency = 秒）
    m_LastFPSUpdate = static_cast<double>(now.QuadPart) / static_cast<double>(freq.QuadPart);
}

//==========================================================================
// 析构函数：~LaserWindow
// 描述：释放窗口资源，确保所有COM对象正确释放
//==========================================================================
LaserWindow::~LaserWindow() {
    Shutdown();
}

//==========================================================================
// 函数：Initialize
// 描述：初始化显示窗口（创建Win32窗口、SwapChain、全屏四边形、显示着色器）
// 参数：
//   device - 共享的D3D11设备
//   context - 共享的D3D11设备上下文
// 返回值：
//   true - 初始化成功
//   false - 初始化失败
//==========================================================================
bool LaserWindow::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (!device || !context) {
        std::cerr << "[LaserWindow] ERROR: Device and context are required!" << std::endl;
        return false;
    }
    
    std::cout << "Initializing LaserWindow with shared device..." << std::endl;
    
    // 保存共享的设备和上下文（使用BeyondLink系统已创建的D3D设备）
    // 这样可以共享资源，避免跨设备纹理传输
    m_Device = device;
    m_Context = context;
    // 增加引用计数，防止外部释放后此处访问失效
    m_Device->AddRef();
    m_Context->AddRef();
    
    // 创建Win32窗口
    if (!CreateWindowHandle()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    // 创建交换链（使用共享设备）
    if (!CreateSwapChain()) {
        std::cerr << "Failed to create swap chain" << std::endl;
        return false;
    }

    // 创建渲染目标
    if (!CreateRenderTarget()) {
        std::cerr << "Failed to create render target" << std::endl;
        return false;
    }

    // 创建全屏四边形
    if (!CreateFullscreenQuad()) {
        std::cerr << "Failed to create fullscreen quad" << std::endl;
        return false;
    }

    // 编译显示着色器
    if (!CompileDisplayShaders()) {
        std::cerr << "Failed to compile display shaders" << std::endl;
        return false;
    }

    // 设置视口
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = static_cast<float>(m_Width);
    m_Viewport.Height = static_cast<float>(m_Height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    // 显示窗口
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    std::cout << "LaserWindow initialized: " << m_Width << "x" << m_Height << std::endl;
    return true;
}

//==========================================================================
// 函数：Shutdown
// 描述：释放所有窗口和D3D资源
//==========================================================================
void LaserWindow::Shutdown() {
    if (m_InputLayout) {
        m_InputLayout->Release();
        m_InputLayout = nullptr;
    }
    if (m_SamplerState) {
        m_SamplerState->Release();
        m_SamplerState = nullptr;
    }
    if (m_DisplayPS) {
        m_DisplayPS->Release();
        m_DisplayPS = nullptr;
    }
    if (m_DisplayVS) {
        m_DisplayVS->Release();
        m_DisplayVS = nullptr;
    }
    if (m_FullscreenQuadVB) {
        m_FullscreenQuadVB->Release();
        m_FullscreenQuadVB = nullptr;
    }
    if (m_BackBufferRTV) {
        m_BackBufferRTV->Release();
        m_BackBufferRTV = nullptr;
    }
    if (m_SwapChain) {
        m_SwapChain->Release();
        m_SwapChain = nullptr;
    }
    if (m_Context) {
        m_Context->Release();
        m_Context = nullptr;
    }
    if (m_Device) {
        m_Device->Release();
        m_Device = nullptr;
    }

    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

//==========================================================================
// 函数：CreateWindowHandle
// 描述：创建Win32窗口并注册窗口类
// 返回值：
//   true - 窗口创建成功
//   false - 窗口创建失败
// 实现细节：
//   1. 检查窗口类是否已注册（支持多窗口）
//   2. 注册窗口类（如果未注册）
//   3. 计算包含边框的实际窗口大小
//   4. 创建窗口并传递this指针用于消息处理
//==========================================================================
bool LaserWindow::CreateWindowHandle() {
    // 检查窗口类是否已注册（避免重复注册）
    WNDCLASSEXA wcCheck = {};
    if (!GetClassInfoExA(GetModuleHandle(nullptr), "BeyondLinkWindowClass", &wcCheck)) {
        // 注册窗口类
        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        // CS_HREDRAW | CS_VREDRAW: 窗口大小改变时重绘整个窗口
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;  // 窗口过程函数
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);  // 黑色背景
        wc.lpszClassName = "BeyondLinkWindowClass";

        if (!RegisterClassExA(&wc)) {
            DWORD error = GetLastError();
            std::cerr << "Failed to register window class. Error: " << error << std::endl;
            return false;
        }
        std::cout << "Window class registered successfully" << std::endl;
    } else {
        std::cout << "Window class already registered" << std::endl;
    }

    // 计算窗口大小（包含边框、标题栏等）
    // 确保客户区（渲染区域）为指定的 m_Width x m_Height
    RECT rc = { 0, 0, m_Width, m_Height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // 创建窗口
    m_hWnd = CreateWindowExA(
        0,  // 扩展窗口样式
        "BeyondLinkWindowClass",
        m_Title.c_str(),
        WS_OVERLAPPEDWINDOW,  // 标准可调整大小窗口
        CW_USEDEFAULT, CW_USEDEFAULT,  // 默认位置
        rc.right - rc.left, rc.bottom - rc.top,  // 包含边框的实际大小
        nullptr,  // 无父窗口
        nullptr,  // 无菜单
        GetModuleHandle(nullptr),
        this  // 传递this指针（在WM_NCCREATE中接收）
    );

    if (m_hWnd == nullptr) {
        DWORD error = GetLastError();
        std::cerr << "Failed to create window. Error: " << error << std::endl;
        return false;
    }

    std::cout << "Window created successfully: HWND = " << m_hWnd << std::endl;
    return true;
}

//==========================================================================
// 函数：WindowProc
// 描述：窗口消息处理函数（静态回调）
// 参数：
//   hwnd - 窗口句柄
//   msg - 消息类型
//   wparam - 消息参数1
//   lparam - 消息参数2
// 返回值：
//   LRESULT - 消息处理结果
// 实现细节：
//   1. 通过GWLP_USERDATA存储和检索LaserWindow对象指针
//   2. 处理WM_CLOSE、WM_DESTROY、WM_KEYDOWN等消息
//   3. ESC键触发窗口关闭
//   4. 设备切换（0-3键）在Main.cpp的主循环中处理
//==========================================================================
LRESULT CALLBACK LaserWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LaserWindow* window = nullptr;

    // WM_NCCREATE是窗口创建时的第一个消息
    // 在此时接收并存储this指针到窗口的GWLP_USERDATA
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lparam);
        window = reinterpret_cast<LaserWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        // 从GWLP_USERDATA中检索对象指针
        window = reinterpret_cast<LaserWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) {
        switch (msg) {
            case WM_CLOSE:
                // 用户点击关闭按钮
                window->m_ShouldClose = true;
                return 0;

            case WM_DESTROY:
                // 窗口被销毁，发送退出消息
                PostQuitMessage(0);
                return 0;

            case WM_KEYDOWN:
                // ESC键退出程序
                if (wparam == VK_ESCAPE) {
                    window->m_ShouldClose = true;
                }
                // 注意：0-3键的设备切换在Main.cpp的主循环中处理
                return 0;
        }
    }

    // 默认消息处理
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

//==========================================================================
// 函数：CreateSwapChain
// 描述：创建DXGI交换链，用于将渲染结果呈现到窗口
// 返回值：
//   true - 创建成功
//   false - 创建失败
// 实现细节：
//   1. 配置交换链参数（单缓冲、R8G8B8A8格式、窗口模式）
//   2. 通过D3D设备获取DXGI Factory
//   3. 使用Factory创建SwapChain
//   4. 禁用Alt+Enter全屏切换
//==========================================================================
bool LaserWindow::CreateSwapChain() {
    if (!m_Device || !m_Context) {
        std::cerr << "[LaserWindow] ERROR: Device must be set before creating SwapChain!" << std::endl;
        return false;
    }
    
    // 配置交换链描述符
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;  // 单缓冲（前缓冲 + 后缓冲）
    scd.BufferDesc.Width = m_Width;
    scd.BufferDesc.Height = m_Height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 8位RGBA格式
    scd.BufferDesc.RefreshRate.Numerator = 0;  // 窗口模式必须为0（由系统控制）
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 作为渲染目标
    scd.OutputWindow = m_hWnd;  // 关联到窗口
    scd.SampleDesc.Count = 1;  // 无多重采样
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;  // 窗口模式
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;  // 缓冲交换后丢弃旧内容
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;  // 允许模式切换

    // 获取 DXGI Factory（需要通过 Device -> Adapter -> Factory 的路径）
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to get IDXGIDevice. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // 步骤1: Device -> Adapter
    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    dxgiDevice->Release();  // 不再需要，立即释放
    if (FAILED(hr)) {
        std::cerr << "Failed to get IDXGIAdapter. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // 步骤2: Adapter -> Factory
    IDXGIFactory* factory = nullptr;
    hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
    adapter->Release();  // 不再需要，立即释放
    if (FAILED(hr)) {
        std::cerr << "Failed to get IDXGIFactory. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // 步骤3: 使用 Factory 和共享设备创建 SwapChain
    hr = factory->CreateSwapChain(m_Device, &scd, &m_SwapChain);
    if (FAILED(hr)) {
        factory->Release();
        std::cerr << "Failed to create SwapChain. HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    
    // 禁用 Alt+Enter 全屏切换（避免意外切换全屏）
    factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
    factory->Release();
    
    std::cout << "SwapChain created successfully" << std::endl;
    return true;
}

//==========================================================================
// 函数：CreateRenderTarget
// 描述：从SwapChain获取后缓冲并创建渲染目标视图（RTV）
// 返回值：
//   true - 创建成功
//   false - 创建失败
//==========================================================================
bool LaserWindow::CreateRenderTarget() {
    // 获取SwapChain的后缓冲纹理（索引0）
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // 为后缓冲创建渲染目标视图
    hr = m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_BackBufferRTV);
    backBuffer->Release();  // 创建RTV后不再需要纹理接口

    return SUCCEEDED(hr);
}

//==========================================================================
// 函数：CreateFullscreenQuad
// 描述：创建全屏四边形顶点缓冲，用于纹理显示
// 返回值：
//   true - 创建成功
//   false - 创建失败
// 实现细节：
//   - 使用4个顶点，TRIANGLE_STRIP拓扑绘制
//   - NDC坐标：(-1,-1)左下 到 (1,1)右上
//   - UV坐标：(0,0)左上 到 (1,1)右下
//==========================================================================
bool LaserWindow::CreateFullscreenQuad() {
    // 定义顶点结构（位置 + UV纹理坐标）
    struct Vertex {
        float x, y, z;  // NDC坐标（-1到1）
        float u, v;     // UV坐标（0到1）
    };

    // 4个顶点组成的全屏四边形（使用TRIANGLE_STRIP拓扑）
    Vertex vertices[] = {
        { -1.0f,  1.0f, 0.0f,  0.0f, 0.0f },  // 左上 (NDC:-1,1  UV:0,0)
        {  1.0f,  1.0f, 0.0f,  1.0f, 0.0f },  // 右上 (NDC:1,1   UV:1,0)
        { -1.0f, -1.0f, 0.0f,  0.0f, 1.0f },  // 左下 (NDC:-1,-1 UV:0,1)
        {  1.0f, -1.0f, 0.0f,  1.0f, 1.0f },  // 右下 (NDC:1,-1  UV:1,1)
    };

    // 创建顶点缓冲描述符
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;  // GPU只读
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    // 初始数据
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    // 创建顶点缓冲
    HRESULT hr = m_Device->CreateBuffer(&bd, &initData, &m_FullscreenQuadVB);
    return SUCCEEDED(hr);
}

//==========================================================================
// 函数：CompileDisplayShaders
// 描述：编译和创建显示着色器（顶点着色器、像素着色器）
// 返回值：
//   true - 编译成功
//   false - 编译失败
// 实现细节：
//   - 顶点着色器：传递位置和UV到像素着色器
//   - 像素着色器：采样激光纹理并应用5倍亮度增强
//   - 创建线性采样器（CLAMP寻址模式）
//==========================================================================
bool LaserWindow::CompileDisplayShaders() {
    // 顶点着色器代码（内联HLSL）
    // 功能：将NDC坐标和UV传递到像素着色器
    const char* vsCode = R"(
        struct VSInput {
            float3 Position : POSITION;
            float2 TexCoord : TEXCOORD0;
        };
        
        struct VSOutput {
            float4 Position : SV_POSITION;
            float2 TexCoord : TEXCOORD0;
        };
        
        VSOutput main(VSInput input) {
            VSOutput output;
            output.Position = float4(input.Position, 1.0);
            output.TexCoord = input.TexCoord;
            return output;
        }
    )";

    // 像素着色器代码（内联HLSL）
    // 功能：采样激光纹理并应用亮度增强
    const char* psCode = R"(
        Texture2D LaserTexture : register(t0);  // 激光纹理（HDR格式）
        SamplerState LinearSampler : register(s0);
        
        struct PSInput {
            float4 Position : SV_POSITION;
            float2 TexCoord : TEXCOORD0;
        };
        
        float4 main(PSInput input) : SV_TARGET {
            // 采样HDR激光纹理
            float4 color = LaserTexture.Sample(LinearSampler, input.TexCoord);
            
            // 亮度增强（模拟激光的高亮效果）
            float brightness = 5.0;  // 5倍亮度增强
            float3 result = saturate(color.rgb * brightness);  // saturate防止溢出
            
            return float4(result, 1.0);
        }
    )";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr;

    // 编译顶点着色器（Shader Model 5.0）
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
                                       nullptr, &m_DisplayVS);
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    // 创建输入布局
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
                                       nullptr, &m_DisplayPS);
    psBlob->Release();
    if (FAILED(hr)) {
        return false;
    }

    // 创建采样器
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = m_Device->CreateSamplerState(&sampDesc, &m_SamplerState);
    return SUCCEEDED(hr);
}

//==========================================================================
// 函数：ProcessMessages
// 描述：处理Windows消息队列中的所有消息（非阻塞）
// 返回值：
//   true - 继续运行
//   false - 接收到WM_QUIT消息，应退出
// 实现细节：
//   使用PeekMessage而非GetMessage，避免阻塞主循环
//==========================================================================
bool LaserWindow::ProcessMessages() {
    MSG msg = {};
    // PM_REMOVE: 从队列中移除消息
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_ShouldClose = true;
            return false;
        }

        // 转换虚拟键消息为字符消息
        TranslateMessage(&msg);
        // 分发消息到窗口过程函数
        DispatchMessage(&msg);
    }

    return true;
}

//==========================================================================
// 函数：DisplayLaserTexture
// 描述：将激光纹理显示到窗口（更新FPS后调用RenderToScreen）
// 参数：
//   laserTexture - 激光纹理的SRV，nullptr则显示黑屏
//==========================================================================
void LaserWindow::DisplayLaserTexture(ID3D11ShaderResourceView* laserTexture) {
    // 更新 FPS 计数
    UpdateFPS();
    
    if (!laserTexture) {
        // 清除为黑色
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_Context->ClearRenderTargetView(m_BackBufferRTV, clearColor);
        m_SwapChain->Present(0, 0);
        return;
    }

    RenderToScreen(laserTexture);
}

//==========================================================================
// 函数：RenderToScreen
// 描述：将激光纹理渲染到窗口后缓冲并呈现
// 参数：
//   texture - 激光纹理的SRV
// 实现细节：
//   1. 设置后缓冲为渲染目标
//   2. 清除为黑色
//   3. 设置渲染管线（着色器、纹理、采样器、顶点缓冲）
//   4. 绘制全屏四边形（TRIANGLE_STRIP，4个顶点）
//   5. 解绑资源并Present到屏幕
//==========================================================================
void LaserWindow::RenderToScreen(ID3D11ShaderResourceView* texture) {
    if (!m_SwapChain || !m_BackBufferRTV) {
        std::cerr << "[RenderToScreen] ERROR: SwapChain or RTV is nullptr!" << std::endl;
        return;
    }
    
    // 步骤1: 设置渲染目标为后缓冲
    ID3D11RenderTargetView* rtvs[] = { m_BackBufferRTV };
    m_Context->OMSetRenderTargets(1, rtvs, nullptr);

    // 步骤2: 清除背景为纯黑色
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_Context->ClearRenderTargetView(m_BackBufferRTV, clearColor);

    // 步骤3: 设置视口（512x512）
    m_Context->RSSetViewports(1, &m_Viewport);

    // 步骤4: 设置顶点着色器和像素着色器
    m_Context->VSSetShader(m_DisplayVS, nullptr, 0);
    m_Context->PSSetShader(m_DisplayPS, nullptr, 0);

    // 步骤5: 绑定激光纹理和线性采样器到像素着色器
    m_Context->PSSetShaderResources(0, 1, &texture);  // t0寄存器
    m_Context->PSSetSamplers(0, 1, &m_SamplerState);   // s0寄存器

    // 步骤6: 设置输入布局（Position + TexCoord）
    m_Context->IASetInputLayout(m_InputLayout);

    // 步骤7: 设置顶点缓冲（全屏四边形）
    UINT stride = sizeof(float) * 5;  // 每个顶点：3个float位置 + 2个float UV
    UINT offset = 0;
    ID3D11Buffer* buffers[] = { m_FullscreenQuadVB };
    m_Context->IASetVertexBuffers(0, 1, buffers, &stride, &offset);

    // 步骤8: 设置图元拓扑为TRIANGLE_STRIP（4个顶点绘制2个三角形）
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 步骤9: 绘制全屏四边形
    m_Context->Draw(4, 0);  // 绘制4个顶点

    // 步骤10: Present前解绑SRV，防止资源状态冲突
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_Context->PSSetShaderResources(0, 1, &nullSRV);
    
    // 步骤11: 呈现到屏幕（0: 不等待垂直同步，0: 无特殊标志）
    HRESULT hr = m_SwapChain->Present(0, 0);
    if (FAILED(hr)) {
        std::cerr << "[RenderToScreen] Present() failed with HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        
        // 如果是设备移除错误，报告详细信息
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            HRESULT reason = m_Device->GetDeviceRemovedReason();
            static bool errorReported = false;
            if (!errorReported) {
                std::cerr << "=== CRITICAL ERROR ===" << std::endl;
                std::cerr << "D3D Device has been removed! Reason: 0x" << std::hex << reason << std::dec << std::endl;
                std::cerr << "This usually indicates a GPU driver crash or timeout." << std::endl;
                errorReported = true;
            }
        }
    }
}

//==========================================================================
// 函数：UpdateFPS
// 描述：更新FPS计数并定期刷新窗口标题
// 实现细节：
//   - 每帧调用，累加帧数
//   - 使用高精度计时器（QueryPerformanceCounter）
//   - 每0.5秒计算一次FPS并更新标题
//   - 公式：FPS = 帧数 / 时间间隔
//==========================================================================
void LaserWindow::UpdateFPS() {
    m_FrameCount++;  // 累加帧数
    
    // 获取当前时间（高精度）
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);  // 获取计时器频率
    QueryPerformanceCounter(&now);     // 获取当前计数器值
    double currentTime = static_cast<double>(now.QuadPart) / static_cast<double>(freq.QuadPart);
    
    // 计算距离上次FPS更新的时间间隔
    double deltaTime = currentTime - m_LastFPSUpdate;
    
    // 每隔0.5秒更新一次FPS（避免频繁更新标题栏）
    if (deltaTime >= m_FPSUpdateInterval) {
        // 计算FPS：帧数除以时间间隔
        m_CurrentFPS = static_cast<float>(m_FrameCount) / static_cast<float>(deltaTime);
        
        // 更新窗口标题显示
        UpdateWindowTitle();
        
        // 重置计数器和时间戳
        m_FrameCount = 0;
        m_LastFPSUpdate = currentTime;
    }
}

//==========================================================================
// 函数：UpdateWindowTitle
// 描述：更新窗口标题，显示当前FPS
// 实现细节：
//   格式：基础标题 + " | FPS: xx.x"
//==========================================================================
void LaserWindow::UpdateWindowTitle() {
    if (!m_hWnd) {
        return;
    }
    
    // 格式化标题：基础标题 + FPS 信息（保留1位小数）
    char titleBuffer[256];
    sprintf_s(titleBuffer, "%s | FPS: %.1f", m_BaseTitle.c_str(), m_CurrentFPS);
    
    // 更新窗口标题栏文本
    SetWindowTextA(m_hWnd, titleBuffer);
}

} // namespace BeyondLink

