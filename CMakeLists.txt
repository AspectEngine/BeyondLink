cmake_minimum_required(VERSION 3.15)

project(BeyondLink 
    VERSION 1.5.0
    LANGUAGES CXX
    DESCRIPTION "Beyond Laser Real-time Visualization Tool"
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/Binaries/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/Binaries/$<CONFIG>)

# 自动收集源文件和头文件
file(GLOB SOURCES "Source/*.cpp")
file(GLOB HEADERS "include/*.h")

# 创建可执行文件
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    ws2_32.lib
)

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    /W3                         # 警告级别 3
    /MP                         # 多处理器编译
    $<$<CONFIG:Debug>:/Od>      # Debug: 禁用优化
    $<$<CONFIG:Debug>:/Zi>      # Debug: 生成调试信息
    $<$<CONFIG:Debug>:/RTC1>    # Debug: 运行时检查
    $<$<CONFIG:Release>:/O2>    # Release: 最大优化
    $<$<CONFIG:Release>:/Oi>    # Release: 启用内联函数
)

# 链接选项
target_link_options(${PROJECT_NAME} PRIVATE
    /SUBSYSTEM:WINDOWS
    $<$<CONFIG:Debug>:/DEBUG>
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
)

# 预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _WINDOWS
    _MBCS
    BEYONDLINK_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    BEYONDLINK_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    BEYONDLINK_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# 构建后复制 DLL 文件
file(GLOB DLL_FILES "${CMAKE_SOURCE_DIR}/bin/*.dll")
foreach(DLL_FILE ${DLL_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL_FILE}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying ${DLL_FILE} to output directory..."
    )
endforeach()

# 输出配置信息
message(STATUS "BeyondLink Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

