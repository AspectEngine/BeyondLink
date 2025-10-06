//==============================================================================
// 文件：LaserWindow.h
// 作者：Yunsio
// 日期：2025-10-06
// 描述：激光可视化显示窗口
//      创建 Windows 窗口并使用 DirectX 11 显示激光纹理
//      包含 FPS 计数和窗口消息处理
//==============================================================================

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <d3d11.h>
#include <dxgi.h>
#include <string>

namespace BeyondLink {

class BeyondLinkSystem;

//==========================================================================
// 类：LaserWindow
// 描述：激光显示窗口类
//      负责创建 Windows 窗口、SwapChain 和显示激光纹理
//      使用全屏四边形渲染纹理，支持 FPS 显示
//==========================================================================
class LaserWindow {
public:
    //==========================================================================
    // 构造函数：LaserWindow
    // 描述：创建激光显示窗口
    // 参数：
    //   width - 窗口宽度（像素）
    //   height - 窗口高度（像素）
    //   title - 窗口标题
    //==========================================================================
    LaserWindow(int width, int height, const std::string& title);
    ~LaserWindow();

    //==========================================================================
    // 函数：Initialize
    // 描述：初始化窗口和 DirectX 资源
    //      使用共享的 D3D11 设备创建 SwapChain
    // 参数：
    //   device - D3D11 设备指针（必须非空）
    //   context - D3D11 设备上下文指针（必须非空）
    // 返回值：
    //   true - 初始化成功
    //   false - 初始化失败
    //==========================================================================
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    
    //==========================================================================
    // 函数：Shutdown
    // 描述：释放所有窗口和 DirectX 资源
    //==========================================================================
    void Shutdown();

    //==========================================================================
    // 函数：ProcessMessages
    // 描述：处理窗口消息（每帧调用）
    // 返回值：
    //   true - 继续运行
    //   false - 收到退出消息
    //==========================================================================
    bool ProcessMessages();

    //==========================================================================
    // 函数：DisplayLaserTexture
    // 描述：显示激光纹理到窗口
    //      使用全屏四边形和亮度增强渲染
    // 参数：
    //   laserTexture - 激光纹理的 SRV（如果为 nullptr 则显示黑屏）
    //==========================================================================
    void DisplayLaserTexture(ID3D11ShaderResourceView* laserTexture);

    //==========================================================================
    // 函数：GetHWND
    // 描述：获取窗口句柄
    // 返回值：
    //   Windows 窗口句柄
    //==========================================================================
    HWND GetHWND() const { return m_hWnd; }

    //==========================================================================
    // 函数：ShouldClose
    // 描述：检查窗口是否应该关闭
    // 返回值：
    //   true - 用户请求关闭窗口
    //   false - 窗口继续运行
    //==========================================================================
    bool ShouldClose() const { return m_ShouldClose; }

    //==========================================================================
    // 函数：GetCurrentFPS
    // 描述：获取当前 FPS
    // 返回值：
    //   当前帧率
    //==========================================================================
    float GetCurrentFPS() const { return m_CurrentFPS; }

private:
    //==========================================================================
    // 函数：CreateWindowHandle
    // 描述：创建 Windows 窗口并注册窗口类
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateWindowHandle();
    
    //==========================================================================
    // 函数：WindowProc
    // 描述：静态窗口消息处理函数
    // 参数：
    //   hwnd - 窗口句柄
    //   msg - 消息 ID
    //   wparam - WPARAM 参数
    //   lparam - LPARAM 参数
    // 返回值：
    //   消息处理结果
    //==========================================================================
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    //==========================================================================
    // 函数：CreateSwapChain
    // 描述：创建 DXGI SwapChain（使用共享的 D3D11 设备）
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateSwapChain();
    
    //==========================================================================
    // 函数：CreateRenderTarget
    // 描述：从 SwapChain 创建后缓冲 RTV
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateRenderTarget();
    
    //==========================================================================
    // 函数：CreateFullscreenQuad
    // 描述：创建全屏四边形顶点缓冲（用于纹理显示）
    // 返回值：
    //   true - 创建成功
    //   false - 创建失败
    //==========================================================================
    bool CreateFullscreenQuad();
    
    //==========================================================================
    // 函数：CompileDisplayShaders
    // 描述：编译显示着色器（简单纹理采样 + 亮度增强）
    // 返回值：
    //   true - 编译成功
    //   false - 编译失败
    //==========================================================================
    bool CompileDisplayShaders();

    //==========================================================================
    // 函数：RenderToScreen
    // 描述：将纹理渲染到屏幕
    //      绘制全屏四边形，应用亮度增强（5x）
    // 参数：
    //   texture - 要显示的纹理 SRV
    //==========================================================================
    void RenderToScreen(ID3D11ShaderResourceView* texture);

    //==========================================================================
    // 函数：UpdateFPS
    // 描述：更新 FPS 计数（每帧调用）
    //==========================================================================
    void UpdateFPS();
    
    //==========================================================================
    // 函数：UpdateWindowTitle
    // 描述：更新窗口标题（显示 FPS）
    //==========================================================================
    void UpdateWindowTitle();

private:
    // 窗口属性
    int m_Width;                                 // 窗口宽度
    int m_Height;                                // 窗口高度
    std::string m_Title;                         // 当前窗口标题
    std::string m_BaseTitle;                     // 基础标题（不含 FPS）
    HWND m_hWnd;                                 // 窗口句柄
    bool m_ShouldClose;                          // 是否应该关闭

    // FPS 统计
    float m_CurrentFPS;                          // 当前 FPS
    int m_FrameCount;                            // 帧计数器
    double m_LastFPSUpdate;                      // 上次 FPS 更新时间
    double m_FPSUpdateInterval;                  // FPS 更新间隔（秒）

    // DirectX
    ID3D11Device* m_Device;                      // D3D11 设备（共享）
    ID3D11DeviceContext* m_Context;              // D3D11 上下文（共享）
    IDXGISwapChain* m_SwapChain;                 // DXGI 交换链
    ID3D11RenderTargetView* m_BackBufferRTV;     // 后缓冲 RTV

    // 全屏四边形渲染
    ID3D11Buffer* m_FullscreenQuadVB;            // 全屏四边形顶点缓冲
    ID3D11VertexShader* m_DisplayVS;             // 显示顶点着色器
    ID3D11PixelShader* m_DisplayPS;              // 显示像素着色器
    ID3D11SamplerState* m_SamplerState;          // 纹理采样器
    ID3D11InputLayout* m_InputLayout;            // 输入布局

    D3D11_VIEWPORT m_Viewport;                   // 视口
};

} // namespace BeyondLink
