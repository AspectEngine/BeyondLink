@echo off
REM ==============================================================================
REM BeyondLink C API Test Runner
REM 自动编译并运行单元测试
REM ==============================================================================

setlocal enabledelayedexpansion

echo ========================================
echo BeyondLink C API Test Runner
echo ========================================
echo.

REM 检查是否在正确的目录
if not exist "..\include\BeyondLinkCAPI.h" (
    echo [错误] 请在 Test 目录下运行此脚本
    pause
    exit /b 1
)

REM 设置路径
set INCLUDE_DIR=..\include
set LIB_DIR=..\Build\Binaries\Debug
set DLL_DIR=..\bin
set OUT_DIR=.\Output

REM 创建输出目录
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM 检查静态库是否存在
if not exist "%LIB_DIR%\BeyondLink.lib" (
    echo [错误] 找不到 BeyondLink.lib
    echo 请先编译 BeyondLink 静态库
    echo 路径: %LIB_DIR%\BeyondLink.lib
    pause
    exit /b 1
)

echo [1/4] 检查编译器...
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo [警告] 未找到 cl.exe，尝试初始化 Visual Studio 环境...
    
    REM 尝试常见的 VS 安装路径
    set VS_PATHS[0]="C:\MyApplication\VisualStudio2022\VC\Auxiliary\Build\vcvarsall.bat"
    set VS_PATHS[1]="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    set VS_PATHS[2]="C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    set VS_PATHS[3]="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    set VS_PATHS[4]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    
    set FOUND=0
    for /L %%i in (0,1,4) do (
        if exist !VS_PATHS[%%i]! (
            echo 找到 Visual Studio: !VS_PATHS[%%i]!
            call !VS_PATHS[%%i]! x64
            set FOUND=1
            goto :compiler_found
        )
    )
    
    if !FOUND! equ 0 (
        echo [错误] 未找到 Visual Studio
        echo 请手动运行 vcvarsall.bat x64 或在 Developer Command Prompt 中运行此脚本
        pause
        exit /b 1
    )
)

:compiler_found
echo [OK] 编译器已就绪
echo.

echo [2/4] 编译测试程序...
cl /nologo /std:c++17 /EHsc /MDd /Zi /utf-8 ^
   /I"%INCLUDE_DIR%" ^
   BeyondLinkCAPI_Test.cpp ^
   /link "%LIB_DIR%\BeyondLink.lib" d3d11.lib dxgi.lib d3dcompiler.lib ws2_32.lib ^
   /OUT:"%OUT_DIR%\BeyondLinkTest.exe"

if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)
echo [OK] 编译成功
echo.

echo [3/4] 拷贝依赖 DLL...
if exist "%DLL_DIR%\linetD2_x64.dll" (
    copy /Y "%DLL_DIR%\linetD2_x64.dll" "%OUT_DIR%\" >nul
    echo [OK] linetD2_x64.dll
) else (
    echo [警告] 找不到 linetD2_x64.dll
)

if exist "%DLL_DIR%\matrix64.dll" (
    copy /Y "%DLL_DIR%\matrix64.dll" "%OUT_DIR%\" >nul
    echo [OK] matrix64.dll
) else (
    echo [警告] 找不到 matrix64.dll
)
echo.

echo [4/4] 运行测试...
echo ========================================
echo.
"%OUT_DIR%\BeyondLinkTest.exe"

set TEST_RESULT=%errorlevel%

echo.
echo ========================================
if %TEST_RESULT% equ 0 (
    echo [成功] 所有测试通过！
) else (
    echo [失败] 部分测试失败，返回码: %TEST_RESULT%
)
echo ========================================

pause
exit /b %TEST_RESULT%

