@echo off
REM ====================================================================
REM BeyondLink Static Library Build Script
REM 自动查找MSBuild并编译静态库
REM ====================================================================

echo ========================================
echo BeyondLink Static Library Builder
echo ========================================
echo.

REM 尝试多个MSBuild路径
set MSBUILD_PATHS[0]="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
set MSBUILD_PATHS[1]="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
set MSBUILD_PATHS[2]="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
set MSBUILD_PATHS[3]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
set MSBUILD_PATHS[4]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
set MSBUILD_PATHS[5]="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"

REM 查找MSBuild
set MSBUILD_EXE=
for /L %%i in (0,1,5) do (
    if exist !MSBUILD_PATHS[%%i]! (
        set MSBUILD_EXE=!MSBUILD_PATHS[%%i]!
        goto :found_msbuild
    )
)

REM 尝试从PATH中查找
where msbuild.exe >nul 2>&1
if %errorlevel% equ 0 (
    set MSBUILD_EXE=msbuild.exe
    goto :found_msbuild
)

echo [错误] 未找到 MSBuild.exe
echo.
echo 请执行以下操作之一:
echo 1. 安装 Visual Studio 2019/2022
echo 2. 在 Developer Command Prompt 中运行此脚本
echo 3. 手动设置MSBuild路径
pause
exit /b 1

:found_msbuild
echo [OK] 找到 MSBuild: %MSBUILD_EXE%
echo.

echo [1/2] 编译 Release 版本...
%MSBUILD_EXE% BeyondLink.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild /m /v:minimal
if %errorlevel% neq 0 (
    echo [错误] Release 编译失败
    pause
    exit /b 1
)
echo [OK] Release 编译成功
echo.

echo [2/2] 编译 Debug 版本...
%MSBUILD_EXE% BeyondLink.sln /p:Configuration=Debug /p:Platform=x64 /t:Rebuild /m /v:minimal
if %errorlevel% neq 0 (
    echo [警告] Debug 编译失败（可选）
) else (
    echo [OK] Debug 编译成功
)
echo.

echo ========================================
echo 编译完成！
echo ========================================
echo.
echo 输出文件:
echo   Release: Build\Binaries\Release\BeyondLink.lib
if exist "Build\Binaries\Debug\BeyondLink.lib" (
    echo   Debug:   Build\Binaries\Debug\BeyondLink.lib
)
echo.
echo 下一步: 运行测试
echo   cd Test
echo   RunTest.bat
echo.
pause

