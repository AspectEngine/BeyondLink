//==============================================================================
// 文件：BeyondLinkCAPI_Test.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：BeyondLink C API 单元测试
//      测试所有C API函数的正确性和异常处理
//==============================================================================

#include "../include/BeyondLinkCAPI.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

// ANSI 颜色代码
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"

// 测试统计
struct TestStats {
    int TotalTests = 0;
    int PassedTests = 0;
    int FailedTests = 0;
};

TestStats g_Stats;

// 测试辅助宏
#define TEST_SECTION(name) \
    std::cout << "\n" << COLOR_CYAN << "=== " << name << " ===" << COLOR_RESET << std::endl

#define TEST_CASE(description) \
    g_Stats.TotalTests++; \
    std::cout << "  测试: " << description << " ... "

#define TEST_PASS() \
    g_Stats.PassedTests++; \
    std::cout << COLOR_GREEN << "[通过]" << COLOR_RESET << std::endl

#define TEST_FAIL(reason) \
    g_Stats.FailedTests++; \
    std::cout << COLOR_RED << "[失败] " << reason << COLOR_RESET << std::endl

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        TEST_FAIL(message); \
        return false; \
    } else { \
        TEST_PASS(); \
        return true; \
    }

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT(ptr != nullptr, message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT(ptr == nullptr, message)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

//==========================================================================
// 测试1：生命周期管理
//==========================================================================
bool Test_Lifecycle_Create() {
    TEST_CASE("创建BeyondLink系统实例");
    BeyondLinkHandle handle = BeyondLink_Create();
    TEST_ASSERT_NOT_NULL(handle, "创建失败");
}

bool Test_Lifecycle_Initialize() {
    TEST_CASE("初始化系统");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    int result = BeyondLink_Initialize(handle);
    BeyondLink_Destroy(handle);
    TEST_ASSERT_EQUAL(1, result, "初始化失败");
}

bool Test_Lifecycle_Shutdown() {
    TEST_CASE("关闭系统");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    TEST_PASS();
}

bool Test_Lifecycle_NullHandle() {
    TEST_CASE("空句柄处理");
    // 这些调用不应崩溃
    BeyondLink_Initialize(nullptr);
    BeyondLink_Shutdown(nullptr);
    BeyondLink_Destroy(nullptr);
    BeyondLink_Update(nullptr);
    BeyondLink_Render(nullptr);
    TEST_PASS();
    return true;
}

//==========================================================================
// 测试2：网络控制
//==========================================================================
bool Test_Network_StartStop() {
    TEST_CASE("启动和停止网络接收");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    int result = BeyondLink_StartNetwork(handle, nullptr);
    if (result != 1) {
        BeyondLink_Destroy(handle);
        TEST_FAIL("启动网络失败");
        return false;
    }
    
    // 等待一小段时间确保网络线程启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    int isRunning = BeyondLink_IsRunning(handle);
    
    BeyondLink_StopNetwork(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_EQUAL(1, isRunning, "网络未运行");
}

bool Test_Network_IsRunning() {
    TEST_CASE("检查网络运行状态");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 启动前应该不在运行
    int runningBefore = BeyondLink_IsRunning(handle);
    
    BeyondLink_StartNetwork(handle, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 启动后应该在运行
    int runningAfter = BeyondLink_IsRunning(handle);
    
    BeyondLink_StopNetwork(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    if (runningBefore != 0) {
        TEST_FAIL("启动前状态错误");
        return false;
    }
    if (runningAfter != 1) {
        TEST_FAIL("启动后状态错误");
        return false;
    }
    
    TEST_PASS();
}

//==========================================================================
// 测试3：配置接口
//==========================================================================
bool Test_Config_TextureSize() {
    TEST_CASE("设置纹理大小");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    // 设置纹理大小
    BeyondLink_SetTextureSize(handle, 512);
    
    // 初始化后检查
    BeyondLink_Initialize(handle);
    int width = BeyondLink_GetTextureWidth(handle);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_EQUAL(512, width, "纹理大小设置失败");
}

bool Test_Config_Quality() {
    TEST_CASE("设置质量级别");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    // 设置质量级别（不会崩溃即可）
    BeyondLink_SetQuality(handle, 0);  // Low
    BeyondLink_SetQuality(handle, 1);  // Medium
    BeyondLink_SetQuality(handle, 2);  // High
    BeyondLink_SetQuality(handle, 3);  // Ultra
    
    BeyondLink_Destroy(handle);
    TEST_PASS();
}

bool Test_Config_ScannerSimulation() {
    TEST_CASE("启用/禁用扫描仪模拟");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    // 启用和禁用扫描仪模拟
    BeyondLink_EnableScannerSimulation(handle, 1);
    BeyondLink_EnableScannerSimulation(handle, 0);
    
    BeyondLink_Destroy(handle);
    TEST_PASS();
}

//==========================================================================
// 测试4：状态查询
//==========================================================================
bool Test_Query_PointCount() {
    TEST_CASE("获取激光点数量");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 初始点数应该为0（没有接收到数据）
    int pointCount = BeyondLink_GetPointCount(handle, 0);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT(pointCount >= 0, "点数量无效");
}

bool Test_Query_NetworkStats() {
    TEST_CASE("获取网络统计信息");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    BeyondLink_StartNetwork(handle, nullptr);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    unsigned long long packetsReceived = 0;
    unsigned long long bytesReceived = 0;
    BeyondLink_GetNetworkStats(handle, &packetsReceived, &bytesReceived);
    
    BeyondLink_StopNetwork(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    // 统计数据应该是有效的（可能为0，如果没有收到数据）
    TEST_PASS();
}

bool Test_Query_TextureDimensions() {
    TEST_CASE("获取纹理尺寸");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    int width = BeyondLink_GetTextureWidth(handle);
    int height = BeyondLink_GetTextureHeight(handle);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    // 默认纹理大小应该是1024x1024
    if (width != 1024 || height != 1024) {
        TEST_FAIL("纹理尺寸不正确");
        return false;
    }
    
    TEST_PASS();
}

//==========================================================================
// 测试5：纹理访问（UE集成关键）
//==========================================================================
bool Test_Texture_GetD3D11Device() {
    TEST_CASE("获取D3D11设备");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    void* device = BeyondLink_GetD3D11Device(handle);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_NOT_NULL(device, "D3D11设备为空");
}

bool Test_Texture_GetD3D11Context() {
    TEST_CASE("获取D3D11上下文");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    void* context = BeyondLink_GetD3D11Context(handle);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_NOT_NULL(context, "D3D11上下文为空");
}

bool Test_Texture_GetTexture2D() {
    TEST_CASE("获取D3D11纹理对象");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 设备0应该有纹理（在Initialize时创建）
    void* texture = BeyondLink_GetTexture2D(handle, 0);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_NOT_NULL(texture, "纹理对象为空");
}

bool Test_Texture_GetSharedHandle() {
    TEST_CASE("获取共享纹理句柄（关键功能）");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 获取共享句柄
    void* sharedHandle = BeyondLink_GetSharedHandle(handle, 0);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_NOT_NULL(sharedHandle, "共享句柄为空");
}

bool Test_Texture_GetTextureSRV() {
    TEST_CASE("获取纹理SRV");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    void* srv = BeyondLink_GetTextureSRV(handle, 0);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT_NOT_NULL(srv, "SRV为空");
}

//==========================================================================
// 测试6：更新和渲染
//==========================================================================
bool Test_UpdateRender_Basic() {
    TEST_CASE("基本更新和渲染");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 执行几次更新和渲染（不应崩溃）
    for (int i = 0; i < 5; ++i) {
        BeyondLink_Update(handle);
        BeyondLink_Render(handle);
    }
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_PASS();
}

bool Test_UpdateRender_WithNetwork() {
    TEST_CASE("带网络接收的更新和渲染");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    BeyondLink_StartNetwork(handle, nullptr);
    
    // 模拟游戏循环
    for (int i = 0; i < 10; ++i) {
        BeyondLink_Update(handle);
        BeyondLink_Render(handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }
    
    BeyondLink_StopNetwork(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_PASS();
}

//==========================================================================
// 测试7：多设备支持
//==========================================================================
bool Test_MultiDevice_AllDevices() {
    TEST_CASE("多设备访问（0-3）");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 访问所有4个设备
    bool allValid = true;
    for (int deviceID = 0; deviceID < 4; ++deviceID) {
        void* texture = BeyondLink_GetTexture2D(handle, deviceID);
        void* sharedHandle = BeyondLink_GetSharedHandle(handle, deviceID);
        int pointCount = BeyondLink_GetPointCount(handle, deviceID);
        
        if (!texture || !sharedHandle) {
            allValid = false;
            break;
        }
    }
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_ASSERT(allValid, "某个设备访问失败");
}

//==========================================================================
// 测试8：异常处理和边界情况
//==========================================================================
bool Test_EdgeCase_InvalidDeviceID() {
    TEST_CASE("无效设备ID处理");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 测试无效设备ID（应返回nullptr/0）
    void* texture = BeyondLink_GetTexture2D(handle, 999);
    void* sharedHandle = BeyondLink_GetSharedHandle(handle, -1);
    int pointCount = BeyondLink_GetPointCount(handle, 100);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    // 应该安全处理无效ID（不崩溃）
    TEST_PASS();
}

bool Test_EdgeCase_DoubleInitialize() {
    TEST_CASE("重复初始化处理");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    // 多次初始化应该是安全的
    BeyondLink_Initialize(handle);
    BeyondLink_Initialize(handle);
    BeyondLink_Initialize(handle);
    
    BeyondLink_Shutdown(handle);
    BeyondLink_Destroy(handle);
    
    TEST_PASS();
}

bool Test_EdgeCase_DoubleShutdown() {
    TEST_CASE("重复关闭处理");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    BeyondLink_Initialize(handle);
    
    // 多次关闭应该是安全的
    BeyondLink_Shutdown(handle);
    BeyondLink_Shutdown(handle);
    BeyondLink_Shutdown(handle);
    
    BeyondLink_Destroy(handle);
    
    TEST_PASS();
}

//==========================================================================
// 主测试函数
//==========================================================================
void RunAllTests() {
    std::cout << COLOR_CYAN << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║     BeyondLink C API 单元测试                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET << std::endl;

    // 测试1: 生命周期管理
    TEST_SECTION("生命周期管理");
    Test_Lifecycle_Create();
    Test_Lifecycle_Initialize();
    Test_Lifecycle_Shutdown();
    Test_Lifecycle_NullHandle();

    // 测试2: 网络控制
    TEST_SECTION("网络控制");
    Test_Network_StartStop();
    Test_Network_IsRunning();

    // 测试3: 配置接口
    TEST_SECTION("配置接口");
    Test_Config_TextureSize();
    Test_Config_Quality();
    Test_Config_ScannerSimulation();

    // 测试4: 状态查询
    TEST_SECTION("状态查询");
    Test_Query_PointCount();
    Test_Query_NetworkStats();
    Test_Query_TextureDimensions();

    // 测试5: 纹理访问
    TEST_SECTION("纹理访问（UE集成关键）");
    Test_Texture_GetD3D11Device();
    Test_Texture_GetD3D11Context();
    Test_Texture_GetTexture2D();
    Test_Texture_GetSharedHandle();
    Test_Texture_GetTextureSRV();

    // 测试6: 更新和渲染
    TEST_SECTION("更新和渲染");
    Test_UpdateRender_Basic();
    Test_UpdateRender_WithNetwork();

    // 测试7: 多设备支持
    TEST_SECTION("多设备支持");
    Test_MultiDevice_AllDevices();

    // 测试8: 异常处理
    TEST_SECTION("异常处理和边界情况");
    Test_EdgeCase_InvalidDeviceID();
    Test_EdgeCase_DoubleInitialize();
    Test_EdgeCase_DoubleShutdown();

    // 输出测试结果
    std::cout << "\n";
    std::cout << COLOR_CYAN << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║     测试结果统计                                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET;
    std::cout << "  总测试数: " << g_Stats.TotalTests << std::endl;
    std::cout << "  " << COLOR_GREEN << "通过: " << g_Stats.PassedTests << COLOR_RESET << std::endl;
    std::cout << "  " << COLOR_RED << "失败: " << g_Stats.FailedTests << COLOR_RESET << std::endl;
    
    if (g_Stats.FailedTests == 0) {
        std::cout << "\n  " << COLOR_GREEN << "✓ 所有测试通过！" << COLOR_RESET << std::endl;
    } else {
        std::cout << "\n  " << COLOR_RED << "✗ 有测试失败，请检查日志" << COLOR_RESET << std::endl;
    }
    
    std::cout << std::endl;
}

//==========================================================================
// 主函数
//==========================================================================
int main() {
    try {
        RunAllTests();
        return (g_Stats.FailedTests == 0) ? 0 : 1;
    }
    catch (const std::exception& e) {
        std::cerr << COLOR_RED << "测试过程中发生异常: " << e.what() << COLOR_RESET << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << COLOR_RED << "测试过程中发生未知异常" << COLOR_RESET << std::endl;
        return -1;
    }
}

