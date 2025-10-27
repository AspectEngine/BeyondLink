//==============================================================================
// BeyondLink - Beyond激光可视化系统
// 文件：LaserSource.cpp
// 作者：Yunsio
// 日期：2025-10-06
// 描述：激光源数据处理实现，负责点数据的扫描仪模拟、插值、降采样、
//       光束检测、速度平滑和边缘淡化
//==============================================================================

#include "LaserSource.h"
#include <algorithm>
#include <cmath>

namespace BeyondLink {
namespace Core {

//==========================================================================
// 构造函数：LaserSource
// 描述：初始化激光源，预分配内存
// 参数：
//   deviceID - 设备ID
//   settings - 激光系统配置参数
//==========================================================================
LaserSource::LaserSource(int deviceID, const LaserSettings& settings)
    : m_DeviceID(deviceID)
    , m_Settings(settings)
    , m_LineWidth(settings.LineWidth)
    , m_MaxBeamBrush(settings.MaxBeamBrush)
    , m_EnableBeamBrush(settings.EnableBeamBrush)
{
    // 预分配内存
    m_RawPoints.reserve(InitialCapacity);
    m_ProcessedPoints.reserve(InitialCapacity);
    m_BeamPoints.reserve(InitialCapacity);
    m_HotBeamPoints.reserve(1000);
}

//==========================================================================
// 析构函数：~LaserSource
// 描述：清理激光源资源
//==========================================================================
LaserSource::~LaserSource() {
}

//==========================================================================
// 函数：SetPointList
// 描述：设置原始激光点数据（拷贝版本）
// 参数：
//   points - 激光点列表
//==========================================================================
void LaserSource::SetPointList(const std::vector<LaserPoint>& points) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RawPoints = points;
}

//==========================================================================
// 函数：SetPointList
// 描述：设置原始激光点数据（移动版本）
// 参数：
//   points - 激光点列表（右值引用）
//==========================================================================
void LaserSource::SetPointList(std::vector<LaserPoint>&& points) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RawPoints = std::move(points);
}

//==========================================================================
// 函数：UpdatePointList
// 描述：更新处理后的点数据，应用扫描仪模拟、插值、降采样、光束检测
// 参数：
//   enableScannerSim - 是否启用扫描仪模拟
//==========================================================================
void LaserSource::UpdatePointList(bool enableScannerSim) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_RawPoints.empty()) {
        m_ProcessedPoints.clear();
        m_BeamPoints.clear();
        m_HotBeamPoints.clear();
        return;
    }
    
    // 生成高强度光束点
    GenerateHotBeams(m_RawPoints);
    
    // 应用扫描仪模拟
    if (enableScannerSim && m_RawPoints.size() > 1) {
        std::vector<LaserPoint> interpolated = ApplyScannerSimulation(m_RawPoints);
        
        // 根据质量设置降采样
        int downsampleFactor = 1;
        int downsampleFactorBeams = 1;
        
        switch (m_Settings.LaserQuality) {
            case LaserSettings::QualityLevel::Low:
                downsampleFactor = 8;
                downsampleFactorBeams = 8;
                break;
            case LaserSettings::QualityLevel::Medium:
                downsampleFactor = 4;
                downsampleFactorBeams = 8;
                break;
            case LaserSettings::QualityLevel::High:
                downsampleFactor = 2;
                downsampleFactorBeams = 2;
                break;
            case LaserSettings::QualityLevel::Ultra:
                downsampleFactor = 1;
                downsampleFactorBeams = 1;
                break;
        }
        
        m_ProcessedPoints = DownsamplePoints(interpolated, downsampleFactor);
        m_BeamPoints = DownsamplePoints(interpolated, downsampleFactorBeams);
        
        // 如果启用BeamBrush，移除重复点
        if (m_EnableBeamBrush) {
            m_ProcessedPoints = RemoveDuplicatePoints(m_ProcessedPoints);
        }
    } else {
        // 不使用扫描仪模拟，直接使用原始点
        m_ProcessedPoints = m_RawPoints;
        m_BeamPoints = m_RawPoints;
        
        if (m_EnableBeamBrush) {
            m_ProcessedPoints = RemoveDuplicatePoints(m_ProcessedPoints);
        }
    }
}

//==========================================================================
// 函数：ApplyScannerSimulation
// 描述：应用扫描仪模拟效果（插值、速度平滑、边缘淡化）
// 参数：
//   points - 原始激光点列表
// 返回值：
//   vector<LaserPoint> - 模拟后的点列表
//==========================================================================
std::vector<LaserPoint> LaserSource::ApplyScannerSimulation(const std::vector<LaserPoint>& points) {
    if (points.size() < 2) {
        return points;
    }
    
    // 插值增加点密度
    std::vector<LaserPoint> interpolated = InterpolatePoints(points, m_Settings.SampleCount);
    
    // 应用速度平滑和边缘淡化
    const float smoothing = m_Settings.VelocitySmoothing;
    const float edgeFade = (std::max)(0.1f, m_Settings.EdgeFade);
    
    std::vector<LaserPoint> smoothed;
    smoothed.reserve(interpolated.size());
    
    float currentVelX = 0.0f;
    float currentVelY = 0.0f;
    float currentPosX = interpolated[0].X;
    float currentPosY = interpolated[0].Y;
    
    for (size_t i = 0; i < interpolated.size(); ++i) {
        const auto& point = interpolated[i];
        
        // 计算到目标点的向量
        float targetVelX = point.X - currentPosX;
        float targetVelY = point.Y - currentPosY;
        float distance = std::sqrt(targetVelX * targetVelX + targetVelY * targetVelY);
        
        float intensity = 1.0f;
        
        if (distance > 0.0f) {
            // 平滑速度向量
            SmoothVector(currentVelX, currentVelY, targetVelX, targetVelY, smoothing);
            
            // 更新位置
            float stepSize = 100.0f / m_Settings.SampleCount * 0.01f;
            currentPosX += currentVelX * stepSize;
            currentPosY += currentVelY * stepSize;
            
            // 计算边缘淡化强度
            float velLength = std::sqrt(currentVelX * currentVelX + currentVelY * currentVelY);
            intensity = (std::min)(4.0f, 1.0f / velLength * 0.2f * edgeFade * 2.0f);
            intensity = (std::min)(4.0f, intensity) / (std::max)(1.0, edgeFade * 2.0 * 4.0);
            
            // 插值淡化效果
            float fadePercent = (std::max)(0.0, edgeFade - 0.5) * 2.0;
            intensity = intensity * (1.0f - fadePercent) + fadePercent;
        }
        
        // 如果是光束点（Z > 0），不应用淡化
        if (point.Z > 0.0f) {
            intensity = 1.0f;
        }
        
        LaserPoint smoothedPoint = point;
        smoothedPoint.X = currentPosX;
        smoothedPoint.Y = currentPosY;
        
        if (point.Z == 0.0f) {
            smoothedPoint.R *= intensity;
            smoothedPoint.G *= intensity;
            smoothedPoint.B *= intensity;
        }
        
        smoothed.push_back(smoothedPoint);
    }
    
    return smoothed;
}

//==========================================================================
// 函数：InterpolatePoints
// 描述：在相邻点之间进行线性插值，增加点密度
// 参数：
//   points - 原始点列表
//   sampleCount - 每对相邻点之间插值的点数
// 返回值：
//   vector<LaserPoint> - 插值后的点列表
//==========================================================================
std::vector<LaserPoint> LaserSource::InterpolatePoints(const std::vector<LaserPoint>& points, int sampleCount) {
    if (points.size() < 2 || sampleCount <= 1) {
        return points;
    }
    
    std::vector<LaserPoint> result;
    result.reserve((points.size() - 1) * sampleCount);
    
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& p0 = points[i];
        const auto& p1 = points[i + 1];
        
        for (int s = 0; s < sampleCount; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(sampleCount - 1);
            
            LaserPoint interpolated;
            interpolated.X = p0.X + (p1.X - p0.X) * t;
            interpolated.Y = p0.Y + (p1.Y - p0.Y) * t;
            interpolated.R = p0.R + (p1.R - p0.R) * t;
            interpolated.G = p0.G + (p1.G - p0.G) * t;
            interpolated.B = p0.B + (p1.B - p0.B) * t;
            interpolated.Z = (p0.Z > 0.0f) ? (p0.Z + (p1.Z - p0.Z) * t) : 0.0f;
            interpolated.Focus = p0.Focus + (p1.Focus - p0.Focus) * t;
            
            result.push_back(interpolated);
        }
    }
    
    return result;
}

//==========================================================================
// 函数：DownsamplePoints
// 描述：降采样点数据，每隔factor个点取一个
// 参数：
//   points - 原始点列表
//   factor - 降采样因子
// 返回值：
//   vector<LaserPoint> - 降采样后的点列表
//==========================================================================
std::vector<LaserPoint> LaserSource::DownsamplePoints(const std::vector<LaserPoint>& points, int factor) {
    if (factor <= 1 || points.empty()) {
        return points;
    }
    
    size_t newSize = (points.size() + factor - 1) / factor;
    std::vector<LaserPoint> result;
    result.reserve(newSize);
    
    for (size_t i = 0; i < points.size(); i += factor) {
        if (i < points.size()) {
            result.push_back(points[i]);
        }
    }
    
    return result;
}

//==========================================================================
// 函数：GenerateHotBeams
// 描述：检测静止光束点（连续重复位置），生成高强度光束点
// 参数：
//   points - 激光点列表
//==========================================================================
void LaserSource::GenerateHotBeams(const std::vector<LaserPoint>& points) {
    m_HotBeamPoints.clear();
    
    if (points.size() < 2) {
        return;
    }
    
    std::vector<size_t> beamIndices;
    int consecutiveCount = 0;
    
    for (size_t i = 1; i < points.size(); ++i) {
        const auto& curr = points[i];
        const auto& prev = points[i - 1];
        
        if (curr.IsSamePosition(prev) && 
            !curr.IsBlankPoint() && 
            !prev.IsBlankPoint() &&
            i < points.size() - 1) {
            
            consecutiveCount++;
            beamIndices.push_back(i - 1);
            
            if (i == points.size() - 1) {
                beamIndices.push_back(i);
            }
        } else {
            if (consecutiveCount > m_Settings.BeamRepeatThreshold) {
                // 生成高强度光束点
                const auto& beamPoint = points[beamIndices.back()];
                for (int j = 0; j < m_Settings.BeamIntensityCount; ++j) {
                    LaserPoint hotPoint = beamPoint;
                    hotPoint.Z = (std::max)(0.0001f, 
                        static_cast<float>(j) / static_cast<float>(m_Settings.BeamIntensityCount - 1));
                    m_HotBeamPoints.push_back(hotPoint);
                }
            }
            beamIndices.clear();
            consecutiveCount = 0;
        }
    }
}

//==========================================================================
// 函数：RemoveDuplicatePoints
// 描述：移除连续重复位置的点
// 参数：
//   points - 原始点列表
// 返回值：
//   vector<LaserPoint> - 去重后的点列表
//==========================================================================
std::vector<LaserPoint> LaserSource::RemoveDuplicatePoints(const std::vector<LaserPoint>& points) {
    if (points.empty()) {
        return points;
    }
    
    std::vector<LaserPoint> result;
    result.reserve(points.size());
    result.push_back(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        if (!points[i].IsSamePosition(result.back())) {
            result.push_back(points[i]);
        }
    }
    
    return result;
}

//==========================================================================
// 函数：SmoothVector
// 描述：对速度向量进行平滑处理（指数移动平均）
// 参数：
//   currentX - [输入/输出] 当前X方向速度
//   currentY - [输入/输出] 当前Y方向速度
//   targetX - 目标X方向速度
//   targetY - 目标Y方向速度
//   smoothing - 平滑系数 (0-1)
//==========================================================================
void LaserSource::SmoothVector(float& currentX, float& currentY, 
                               float targetX, float targetY, 
                               float smoothing) {
    currentX += (targetX - currentX) * (1.0f - smoothing);
    currentY += (targetY - currentY) * (1.0f - smoothing);
}

} // namespace Core
} // namespace BeyondLink

