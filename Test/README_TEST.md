# BeyondLink C API 单元测试

本目录包含BeyondLink C API的单元测试程序。

---

## 测试内容

测试程序 `BeyondLinkCAPI_Test.cpp` 包含以下测试类别：

### 1. 生命周期管理 (4个测试)
- ✅ 创建系统实例
- ✅ 初始化系统
- ✅ 关闭系统
- ✅ 空句柄安全处理

### 2. 网络控制 (2个测试)
- ✅ 启动和停止网络接收
- ✅ 检查网络运行状态

### 3. 配置接口 (3个测试)
- ✅ 设置纹理大小
- ✅ 设置质量级别
- ✅ 启用/禁用扫描仪模拟

### 4. 状态查询 (3个测试)
- ✅ 获取激光点数量
- ✅ 获取网络统计信息
- ✅ 获取纹理尺寸

### 5. 纹理访问 - UE集成关键 (5个测试)
- ✅ 获取D3D11设备
- ✅ 获取D3D11上下文
- ✅ 获取D3D11纹理对象
- ✅ **获取共享纹理句柄（核心功能）**
- ✅ 获取纹理SRV

### 6. 更新和渲染 (2个测试)
- ✅ 基本更新和渲染
- ✅ 带网络接收的更新和渲染

### 7. 多设备支持 (1个测试)
- ✅ 多设备访问（0-3）

### 8. 异常处理和边界情况 (3个测试)
- ✅ 无效设备ID处理
- ✅ 重复初始化处理
- ✅ 重复关闭处理

**总计：23个测试用例**

---

## 编译方法

### 方法1：使用Visual Studio

1. **创建新的控制台项目**
   - 在BeyondLink解决方案中添加新项目
   - 项目类型：C++ 控制台应用程序
   - 项目名称：`BeyondLinkTest`

2. **配置项目**
   - 添加现有项：`Test/BeyondLinkCAPI_Test.cpp`
   - 项目属性 → C/C++ → 附加包含目录：
     ```
     $(SolutionDir)include
     ```
   - 项目属性 → 链接器 → 附加库目录：
     ```
     $(SolutionDir)Build\Binaries\$(Configuration)
     ```
   - 项目属性 → 链接器 → 输入 → 附加依赖项：
     ```
     BeyondLink.lib
     ```

3. **确保DLL可用**
   - 将 `bin/linetD2_x64.dll` 和 `bin/matrix64.dll` 拷贝到测试程序的输出目录
   - 或添加Post-Build事件：
     ```
     xcopy /Y "$(SolutionDir)bin\*.dll" "$(OutDir)"
     ```

4. **编译和运行**
   - 选择配置：**Release | x64** 或 **Debug | x64**
   - 按 `Ctrl+F5` 编译并运行

### 方法2：命令行编译

```bash
# 进入项目目录
cd BeyondLink

# 使用cl.exe编译（需要先运行 vcvarsall.bat）
cl /std:c++17 /EHsc /I"include" Test\BeyondLinkCAPI_Test.cpp /link Build\Binaries\Release\BeyondLink.lib /OUT:BeyondLinkTest.exe

# 运行测试
BeyondLinkTest.exe
```

---

## 运行测试

### 预期输出

```
╔═══════════════════════════════════════════════════════╗
║     BeyondLink C API 单元测试                         ║
╚═══════════════════════════════════════════════════════╝

=== 生命周期管理 ===
  测试: 创建BeyondLink系统实例 ... [通过]
  测试: 初始化系统 ... [通过]
  测试: 关闭系统 ... [通过]
  测试: 空句柄处理 ... [通过]

=== 网络控制 ===
  测试: 启动和停止网络接收 ... [通过]
  测试: 检查网络运行状态 ... [通过]

=== 配置接口 ===
  测试: 设置纹理大小 ... [通过]
  测试: 设置质量级别 ... [通过]
  测试: 启用/禁用扫描仪模拟 ... [通过]

=== 状态查询 ===
  测试: 获取激光点数量 ... [通过]
  测试: 获取网络统计信息 ... [通过]
  测试: 获取纹理尺寸 ... [通过]

=== 纹理访问（UE集成关键） ===
  测试: 获取D3D11设备 ... [通过]
  测试: 获取D3D11上下文 ... [通过]
  测试: 获取D3D11纹理对象 ... [通过]
  测试: 获取共享纹理句柄（关键功能） ... [通过]
  测试: 获取纹理SRV ... [通过]

=== 更新和渲染 ===
  测试: 基本更新和渲染 ... [通过]
  测试: 带网络接收的更新和渲染 ... [通过]

=== 多设备支持 ===
  测试: 多设备访问（0-3） ... [通过]

=== 异常处理和边界情况 ===
  测试: 无效设备ID处理 ... [通过]
  测试: 重复初始化处理 ... [通过]
  测试: 重复关闭处理 ... [通过]

╔═══════════════════════════════════════════════════════╗
║     测试结果统计                                      ║
╚═══════════════════════════════════════════════════════╝
  总测试数: 23
  通过: 23
  失败: 0

  ✓ 所有测试通过！
```

### 返回值

- `0` - 所有测试通过
- `1` - 有测试失败
- `-1` - 发生异常

---

## 常见问题

### Q1: 编译错误 - 找不到BeyondLinkCAPI.h

**解决方案**：
- 确认 `include/` 目录已添加到项目的包含路径
- 检查头文件路径大小写（Windows不敏感，但Linux敏感）

### Q2: 链接错误 - 找不到BeyondLink.lib

**解决方案**：
- 确认已编译BeyondLink静态库
- 确认库路径配置正确
- 确认使用的是x64配置（不是Win32）

### Q3: 运行时错误 - 找不到linetD2_x64.dll

**解决方案**：
- 将 `bin/linetD2_x64.dll` 和 `bin/matrix64.dll` 拷贝到测试程序的输出目录
- 或将bin目录添加到系统PATH环境变量

### Q4: 部分测试失败

**可能原因**：
1. **网络相关测试失败**：防火墙阻止UDP端口5568
   - 解决：允许BeyondLinkTest.exe访问网络
   
2. **纹理相关测试失败**：显卡驱动问题
   - 解决：更新显卡驱动到最新版本
   
3. **DLL加载失败**：linetD2_x64.dll版本不兼容
   - 解决：确认DLL来自Beyond软件或相关SDK

### Q5: 所有测试都通过，但在实际使用中有问题

**建议**：
1. 检查UE插件的Build.cs配置
2. 验证D3D11纹理共享在UE环境中的实现
3. 添加日志输出调试具体问题

---

## 自定义测试

您可以在 `BeyondLinkCAPI_Test.cpp` 中添加自己的测试用例：

```cpp
bool Test_MyCustomTest() {
    TEST_CASE("我的自定义测试");
    BeyondLinkHandle handle = BeyondLink_Create();
    if (!handle) {
        TEST_FAIL("无法创建实例");
        return false;
    }
    
    // 您的测试代码...
    
    BeyondLink_Destroy(handle);
    TEST_PASS();
}

// 在 RunAllTests() 中添加：
Test_MyCustomTest();
```

---

## 持续集成

可以将此测试集成到CI/CD流程中：

```yaml
# 示例：GitHub Actions
- name: Run BeyondLink Tests
  run: |
    cd BeyondLink
    .\BeyondLinkTest.exe
  continue-on-error: false  # 测试失败则构建失败
```

---

## 性能测试

如需性能测试，可以添加计时功能：

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// ... 执行操作
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "耗时: " << duration.count() << " μs" << std::endl;
```

---

## 测试覆盖率

当前测试覆盖率：

- ✅ **C API接口覆盖率**: 100% (21/21个函数)
- ✅ **生命周期覆盖**: 完整
- ✅ **异常处理覆盖**: 完整
- ✅ **多设备支持**: 完整
- ✅ **核心功能（纹理共享）**: 完整

---

## 反馈和改进

如果您发现测试中的问题或希望添加新的测试用例，请：

1. 提交GitHub Issue
2. 发送邮件：liuxiunan596@gmail.com
3. 直接修改测试文件并提交PR

---

**祝测试顺利！** ✅

