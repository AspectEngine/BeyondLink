//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：Main.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 版本：v1.5.0
// 描述：主程序入口点，初始化系统、创建显示窗口、处理设备切换和渲染循环
//==============================================================================

#include "BeyondLink.h"
#include "LaserWindow.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

using namespace BeyondLink;

// 版本信息宏（由 CMake 定义）
#ifndef BEYONDLINK_VERSION_MAJOR
#define BEYONDLINK_VERSION_MAJOR 1
#endif
#ifndef BEYONDLINK_VERSION_MINOR
#define BEYONDLINK_VERSION_MINOR 5
#endif
#ifndef BEYONDLINK_VERSION_PATCH
#define BEYONDLINK_VERSION_PATCH 0
#endif

// 获取版本字符串
std::string GetVersionString() {
    std::ostringstream oss;
    oss << "v" << BEYONDLINK_VERSION_MAJOR << "." 
        << BEYONDLINK_VERSION_MINOR << "." 
        << BEYONDLINK_VERSION_PATCH;
    return oss.str();
}

//==========================================================================
// 函数：WinMain
// 描述：Windows应用程序主入口点
//       - 初始化BeyondLink系统和渲染器
//       - 创建512x512显示窗口
//       - 启动UDP网络接收器
//       - 主循环：处理键盘输入（0-3切换设备），更新、渲染、显示
//       - 每5秒输出一次统计信息（网络流量、设备状态）
// 参数：
//   hInstance - 应用程序实例句柄
//   hPrevInstance - 前一个实例句柄（Win32中始终为NULL）
//   lpCmdLine - 命令行参数
//   nCmdShow - 窗口显示方式
// 返回值：
//   0 - 正常退出
//   -1 - 初始化失败或异常
//==========================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ===== 异常处理包装 =====
    try {

    //==========================================================================
    // 输出版本信息和标题
    //==========================================================================
    std::string version = GetVersionString();
    std::cout << "========================================" << std::endl;
    std::cout << "  BeyondLink " << version << std::endl;
    std::cout << "  Beyond Laser Visualization System" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    //==========================================================================
    // 步骤1: 创建激光系统配置
    // 配置参数说明：
    //   MaxLaserDevices = 9        - 最多支持9台激光设备（1-9）
    //   NetworkPort = 5568         - UDP接收端口（Beyond默认端口）
    //   TextureSize = 1024         - 激光纹理分辨率（1024x1024）
    //   ScannerSimulation = true   - 启用扫描仪模拟（插值、平滑、淡化）
    //   EnableMipmaps = true       - 启用Mipmap生成
    //   EdgeFade = 0.1             - 边缘淡化强度（0-1）
    //   VelocitySmoothing = 0.83   - 速度平滑系数（0-1，越大越平滑）
    //==========================================================================
    Core::LaserSettings settings;
    settings.MaxLaserDevices = 9;
    settings.NetworkPort = 5568;
    settings.TextureSize = 1024;
    settings.ScannerSimulation = true;
    settings.EnableMipmaps = true;
    settings.EdgeFade = 0.1f;
    settings.VelocitySmoothing = 0.83f;

    //==========================================================================
    // 步骤2: 创建并初始化BeyondLink系统
    // 顺序说明：
    //   1. 创建系统（构造函数）
    //   2. 初始化系统（创建D3D设备、编译着色器、初始化DLL）
    //   3. 创建显示窗口（使用系统的D3D设备）
    // 注意：窗口必须使用系统的D3D设备，否则无法共享纹理资源
    //==========================================================================
    BeyondLinkSystem system(settings);
    if (!system.Initialize(nullptr)) {
        std::cerr << "Failed to initialize BeyondLink system" << std::endl;
        return -1;
    }

    //==========================================================================
    // 步骤3: 创建512x512显示窗口（共享D3D设备）
    // 默认显示设备1，窗口标题会在设备切换时动态更新
    //==========================================================================
    std::string windowTitle = "BeyondLink " + version + " - Device 1";
    LaserWindow window(512, 512, windowTitle);
    if (!window.Initialize(system.GetRenderer()->GetDevice(), 
                          system.GetRenderer()->GetContext())) {
        std::cerr << "Failed to initialize display window" << std::endl;
        system.Shutdown();
        return -1;
    }

    std::cout << "Display window created: 512x512" << std::endl;
    std::cout << "Press ESC or close window to exit" << std::endl;
    std::cout << std::endl;

    //==========================================================================
    // 步骤4: 启动UDP网络接收器
    // 功能：
    //   - 初始化Winsock
    //   - 创建UDP Socket并绑定到5568端口
    //   - 加入279个多播组（设备1-9，子网0-30）
    //   - 启动接收线程
    //==========================================================================
    if (!system.StartNetworkReceiver()) {
        std::cerr << "Failed to start network receiver" << std::endl;
        system.Shutdown();
        window.Shutdown();
        return -1;
    }

    // 输出系统状态和控制说明
    std::cout << "System is running..." << std::endl;
    std::cout << "Listening for Beyond laser data on port " << settings.NetworkPort << std::endl;
    std::cout << "Waiting for data from Beyond software..." << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  1-9 - Switch between laser devices" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "  Close Window - Exit" << std::endl;
    std::cout << std::endl;

    //==========================================================================
    // 主渲染循环
    // 循环流程：
    //   1. 检查键盘输入（1-9键切换设备）
    //   2. 处理窗口消息
    //   3. 更新所有激光源的点数据处理
    //   4. 渲染所有激光源到纹理
    //   5. 显示当前设备的纹理到窗口
    //   6. 每5秒输出统计信息
    //   7. 限制帧率到60 FPS
    //==========================================================================
    auto lastStatsTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    int currentDevice = 0;  // 当前显示的设备索引（0-8，对应显示设备1-9）
    
    std::cout << "=== Device Control ===" << std::endl;
    std::cout << "  Press 1-9 to switch between laser devices" << std::endl;
    std::cout << "  Currently viewing Device " << (currentDevice + 1) << std::endl;
    std::cout << std::endl;

    while (!window.ShouldClose()) {
        // ----- 处理窗口消息 -----
        // 处理WM_QUIT、WM_CLOSE、WM_KEYDOWN(ESC)等消息
        if (!window.ProcessMessages()) {
            break;  // 接收到退出消息（WM_QUIT）
        }
        
        // 再次检查窗口是否应该关闭（防止在消息处理后继续执行）
        if (window.ShouldClose()) {
            break;
        }
        
        // ----- 设备切换逻辑 -----
        // 使用 GetAsyncKeyState 直接检查键盘状态（不依赖消息队列）
        // 1-9键对应设备1-9（Beyond的9个Fixture）
        // 内部索引仍为0-8，显示时+1转换为1-9
        // 使用静态变量防止按键重复触发
        static bool keyPressed[9] = { false, false, false, false, false, false, false, false, false };
        
        for (int key = 1; key <= 9; ++key) {
            // 检查 1-9 键是否被按下（虚拟键码：'1' = 0x31）
            bool isKeyDown = (GetAsyncKeyState('0' + key) & 0x8000) != 0;
            int deviceIndex = key - 1;  // 按键1对应索引0，按键2对应索引1，以此类推
            
            if (isKeyDown && !keyPressed[deviceIndex]) {
                // 按键刚被按下（上次检查时未按下）
                keyPressed[deviceIndex] = true;
                
                if (deviceIndex != currentDevice) {
                    currentDevice = deviceIndex;
                    // 输出切换提示（显示设备编号1-9，包含对应的多播地址）
                    std::cout << "\n>>> Switched to Device " << (currentDevice + 1) << " (Multicast: 239.255." 
                              << currentDevice << ".x) <<<\n" << std::endl;
                    
                    // 动态更新窗口基础标题（显示设备编号1-9和版本号）
                    std::string newTitle = "BeyondLink " + version + " - Device " + std::to_string(currentDevice + 1);
                    window.SetBaseTitle(newTitle);
                }
            } else if (!isKeyDown && keyPressed[deviceIndex]) {
                // 按键被释放
                keyPressed[deviceIndex] = false;
            }
        }

        // ----- 更新阶段 -----
        // 处理所有激光源的原始点数据：
        //   - 应用扫描仪模拟（插值、平滑、淡化）
        //   - 根据质量级别降采样
        //   - 检测并生成高强度光束点
        system.Update();

        // ----- 渲染阶段 -----
        // 将所有激光源渲染到各自的HDR纹理（1024x1024）
        system.Render();

        // ----- 显示阶段 -----
        // 获取当前选中设备的纹理并显示到窗口（512x512）
        auto laserTexture = system.GetLaserTexture(currentDevice);
        window.DisplayLaserTexture(laserTexture);

        frameCount++;

        // ----- 统计信息输出 -----
        // 每5秒输出一次网络和设备状态
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime).count();
        
        if (elapsed >= 5) {
            // 获取网络统计信息
            auto stats = system.GetNetworkStats();
            
            // ----- 输出网络和渲染统计 -----
            std::cout << "\n=== Status Report ===" << std::endl;
            std::cout << "Network: " << stats.PacketsReceived << " packets | " 
                     << stats.BytesReceived << " bytes | FPS: " << (frameCount / elapsed) << std::endl;
            
            // ----- 显示所有设备的状态 -----
            // 格式：[OK] 有数据   [--] 无数据   >>> 当前查看的设备
            // 显示设备编号1-9（内部索引0-8）
            std::cout << "\nAll Devices Status:" << std::endl;
            for (int dev = 0; dev < 9; ++dev) {
                auto source = system.GetLaserSource(dev);
                size_t points = source ? source->GetPointCount() : 0;
                
                // 视觉指示器：>>> 表示正在查看此设备
                std::string indicator = (dev == currentDevice) ? ">>> " : "    ";
                std::string status = (points > 0) ? "[OK]" : "[--]";
                
                // 显示设备编号为1-9（dev+1），多播地址使用内部索引（dev）
                std::cout << indicator << "Device " << (dev + 1) << " (239.255." << dev << ".x): " 
                         << status << " " << points << " points";
                
                if (dev == currentDevice) {
                    std::cout << " <- VIEWING";  // 标记当前正在查看的设备
                }
                std::cout << std::endl;
            }
            
            // ----- 警告信息 -----
            if (stats.PacketsReceived == 0) {
                // 完全没有接收到网络数据
                std::cout << "\n[!] WARNING: No network data received!" << std::endl;
                std::cout << "  Check Beyond network output settings." << std::endl;
            } else {
                // 接收到数据，但当前设备没有点数据
                auto currentSource = system.GetLaserSource(currentDevice);
                if (!currentSource || currentSource->GetPointCount() == 0) {
                    std::cout << "\n[!] WARNING: Device " << (currentDevice + 1) << " has no data!" << std::endl;
                    std::cout << "  Beyond may not be sending to 239.255." << currentDevice << ".x" << std::endl;
                    std::cout << "  Check Beyond Zone/Device configuration (Fixture " << (currentDevice + 1) << ")." << std::endl;
                }
            }
            std::cout << "===================\n" << std::endl;
            
            // 重置统计计数器
            lastStatsTime = now;
            frameCount = 0;
        }

        // ----- 帧率限制 -----
        // 限制到约60 FPS（16ms/帧）
        // 避免CPU/GPU过度使用，保持稳定帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    //==========================================================================
    // 清理阶段：关闭所有系统和释放资源
    // 顺序：先关闭系统（停止网络线程、释放D3D资源），再关闭窗口
    //==========================================================================
    system.Shutdown();
    window.Shutdown();

    return 0;
    
    //==========================================================================
    // 异常捕获：处理运行时异常
    //==========================================================================
    } catch (const std::exception& e) {
        // 标准异常：显示详细错误信息
        MessageBoxA(nullptr, e.what(), "BeyondLink - Error", MB_OK | MB_ICONERROR);
        return -1;
    } catch (...) {
        // 未知异常：显示通用错误提示
        MessageBoxA(nullptr, "An unknown error occurred", "BeyondLink - Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}

//==============================================================================
// End of Main.cpp
//==============================================================================

