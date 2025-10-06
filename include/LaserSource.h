//==============================================================================
// 文件：LaserSource.h
// 作者：Yunsio
// 日期：2025-10-06
// 描述：单个激光设备的数据处理模块
//      负责接收原始点数据、应用扫描仪模拟、生成光束效果
//==============================================================================

#pragma once

#include "LaserPoint.h"
#include "LaserSettings.h"
#include <vector>
#include <memory>
#include <mutex>

namespace BeyondLink {
namespace Core {

//==========================================================================
// 类：LaserSource
// 描述：单个激光设备的数据处理管线
//      从网络接收原始激光点 → 扫描仪模拟 → 光束检测 → 准备渲染
//      每个设备（0-3）对应一个 LaserSource 实例
//==========================================================================
class LaserSource {
public:
    //==========================================================================
    // 构造函数：LaserSource
    // 描述：创建指定设备的激光源处理器
    // 参数：
    //   deviceID - 设备 ID (0-3)
    //   settings - 激光系统配置参数
    //==========================================================================
    explicit LaserSource(int deviceID, const LaserSettings& settings);
    ~LaserSource();

    //==========================================================================
    // 函数：SetPointList (复制版本)
    // 描述：设置原始激光点列表（从网络接收的数据）
    // 参数：
    //   points - 原始激光点列表
    //==========================================================================
    void SetPointList(const std::vector<LaserPoint>& points);
    
    //==========================================================================
    // 函数：SetPointList (移动版本)
    // 描述：设置原始激光点列表（移动语义，避免拷贝）
    // 参数：
    //   points - 原始激光点列表（将被移动）
    //==========================================================================
    void SetPointList(std::vector<LaserPoint>&& points);

    //==========================================================================
    // 函数：UpdatePointList
    // 描述：更新处理点列表，应用扫描仪模拟和光束检测
    //      通常在每帧调用，将原始点转换为可渲染的点
    // 参数：
    //   enableScannerSim - 是否启用扫描仪模拟
    //==========================================================================
    void UpdatePointList(bool enableScannerSim);

    //==========================================================================
    // 函数：GetProcessedPoints
    // 描述：获取处理后的主点列表（用于渲染）
    // 返回值：
    //   处理后的激光点列表的常量引用
    //==========================================================================
    const std::vector<LaserPoint>& GetProcessedPoints() const { return m_ProcessedPoints; }
    
    //==========================================================================
    // 函数：GetBeamPoints
    // 描述：获取光束点列表
    // 返回值：
    //   光束点列表的常量引用
    //==========================================================================
    const std::vector<LaserPoint>& GetBeamPoints() const { return m_BeamPoints; }
    
    //==========================================================================
    // 函数：GetHotBeamPoints
    // 描述：获取高强度光束点列表（静止光束的增强渲染）
    // 返回值：
    //   高强度光束点列表的常量引用
    //==========================================================================
    const std::vector<LaserPoint>& GetHotBeamPoints() const { return m_HotBeamPoints; }

    //==========================================================================
    // 函数：GetPointCount
    // 描述：获取处理后的点数量
    // 返回值：
    //   点数量
    //==========================================================================
    size_t GetPointCount() const { return m_ProcessedPoints.size(); }
    
    //==========================================================================
    // 函数：GetBeamPointCount
    // 描述：获取光束点数量
    // 返回值：
    //   光束点数量
    //==========================================================================
    size_t GetBeamPointCount() const { return m_BeamPoints.size(); }
    
    //==========================================================================
    // 函数：GetHotBeamPointCount
    // 描述：获取高强度光束点数量
    // 返回值：
    //   高强度光束点数量
    //==========================================================================
    size_t GetHotBeamPointCount() const { return m_HotBeamPoints.size(); }

    //==========================================================================
    // 函数：GetDeviceID
    // 描述：获取设备 ID
    // 返回值：
    //   设备 ID (0-3)
    //==========================================================================
    int GetDeviceID() const { return m_DeviceID; }
    
    //==========================================================================
    // 函数：GetLineWidth / SetLineWidth
    // 描述：获取/设置激光线条宽度
    //==========================================================================
    float GetLineWidth() const { return m_LineWidth; }
    void SetLineWidth(float width) { m_LineWidth = width; }
    
    //==========================================================================
    // 函数：GetMaxBeamBrush / SetMaxBeamBrush
    // 描述：获取/设置最大光束画刷大小
    //==========================================================================
    float GetMaxBeamBrush() const { return m_MaxBeamBrush; }
    void SetMaxBeamBrush(float brush) { m_MaxBeamBrush = brush; }
    
    //==========================================================================
    // 函数：IsBeamBrushEnabled / SetBeamBrushEnabled
    // 描述：获取/设置光束画刷是否启用
    //==========================================================================
    bool IsBeamBrushEnabled() const { return m_EnableBeamBrush; }
    void SetBeamBrushEnabled(bool enabled) { m_EnableBeamBrush = enabled; }

    //==========================================================================
    // 函数：GetMutex
    // 描述：获取互斥锁用于线程安全访问
    // 返回值：
    //   互斥锁的引用
    //==========================================================================
    std::mutex& GetMutex() { return m_Mutex; }

private:
    //==========================================================================
    // 函数：ApplyScannerSimulation
    // 描述：应用扫描仪物理模拟（惯性、速度平滑、边缘淡化）
    // 参数：
    //   points - 输入点列表
    // 返回值：
    //   模拟后的点列表
    //==========================================================================
    std::vector<LaserPoint> ApplyScannerSimulation(const std::vector<LaserPoint>& points);
    
    //==========================================================================
    // 函数：InterpolatePoints
    // 描述：在相邻点之间插值，增加点密度以提升渲染平滑度
    // 参数：
    //   points - 输入点列表
    //   sampleCount - 每对点之间插入的样本数
    // 返回值：
    //   插值后的点列表
    //==========================================================================
    std::vector<LaserPoint> InterpolatePoints(const std::vector<LaserPoint>& points, int sampleCount);
    
    //==========================================================================
    // 函数：DownsamplePoints
    // 描述：按指定倍数降采样点列表（根据质量设置）
    // 参数：
    //   points - 输入点列表
    //   factor - 降采样倍数（1=无降采样，2=保留一半，8=保留1/8）
    // 返回值：
    //   降采样后的点列表
    //==========================================================================
    std::vector<LaserPoint> DownsamplePoints(const std::vector<LaserPoint>& points, int factor);
    
    //==========================================================================
    // 函数：GenerateHotBeams
    // 描述：检测并生成高强度光束点（用于静止光束的增强渲染）
    //      识别连续重复的点位置，生成额外的高亮点
    // 参数：
    //   points - 输入点列表
    //==========================================================================
    void GenerateHotBeams(const std::vector<LaserPoint>& points);
    
    //==========================================================================
    // 函数：RemoveDuplicatePoints
    // 描述：移除连续重复的点（用于 BeamBrush 模式优化）
    // 参数：
    //   points - 输入点列表
    // 返回值：
    //   去重后的点列表
    //==========================================================================
    std::vector<LaserPoint> RemoveDuplicatePoints(const std::vector<LaserPoint>& points);
    
    //==========================================================================
    // 函数：SmoothVector
    // 描述：平滑二维向量（用于速度向量的惯性模拟）
    // 参数：
    //   currentX, currentY - 当前向量（将被修改）
    //   targetX, targetY - 目标向量
    //   smoothing - 平滑因子 [0.0, 1.0]
    //==========================================================================
    void SmoothVector(float& currentX, float& currentY, 
                      float targetX, float targetY, 
                      float smoothing);

private:
    int m_DeviceID;                              // 设备 ID (0-3)
    LaserSettings m_Settings;                    // 系统配置参数
    
    // 点数据缓冲
    std::vector<LaserPoint> m_RawPoints;         // 原始接收的点
    std::vector<LaserPoint> m_ProcessedPoints;   // 处理后的主点列表
    std::vector<LaserPoint> m_BeamPoints;        // 光束点列表
    std::vector<LaserPoint> m_HotBeamPoints;     // 高强度光束点
    
    // 渲染参数
    float m_LineWidth;                           // 线宽
    float m_MaxBeamBrush;                        // 最大光束画刷大小
    bool m_EnableBeamBrush;                      // 是否启用光束画刷
    
    // 线程安全
    mutable std::mutex m_Mutex;                  // 互斥锁
    
    // 缓冲容量管理
    static constexpr size_t InitialCapacity = 10000;  // 初始缓冲容量
};

} // namespace Core
} // namespace BeyondLink
