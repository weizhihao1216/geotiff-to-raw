#ifndef RAW_TERRAIN_READER_H
#define RAW_TERRAIN_READER_H

#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

/**
 * @brief 原始栅格地形元数据结构体
 */
struct RawTerrainMeta {
    int width = 0;                 ///< 栅格列数 (宽度)
    int height = 0;                ///< 栅格行数 (高度)
    double min_lat = 0.0;          ///< 最小纬度 (South)
    double max_lat = 0.0;          ///< 最大纬度 (North)
    double min_lon = 0.0;          ///< 最小经度 (West)
    double max_lon = 0.0;          ///< 最大经度 (East)
    double res_x = 0.0;            ///< X方向分辨率 (经度方向)
    double res_y = 0.0;            ///< Y方向分辨率 (纬度方向)
    bool has_nodata = false;       ///< 是否包含 NoData 无效值标记
    float nodata_value = -9999.0f; ///< NoData 数值
    bool is_flipped_y = false;     ///< 二进制数据行是否已进行 Y 轴垂直翻转
};

/**
 * @brief 游戏/渲染引擎端轻量级地形/海底高程数据读取与检索类
 * 
 * 特点:
 * 1. 纯 C++11 标准库实现，零第三方库依赖，可直接集成进 Unreal Engine、Unity Native C++ 插件、OpenGL/DirectX 引擎。
 * 2. 提供主接口: 输入经纬度 (Latitude, Longitude)，实时查询对应海拔高度 (Height/Elevation)。
 * 3. 内置双线性插值 (Bilinear Interpolation)，支持高质连续采样与最近邻采样。
 */
class RawTerrainReader {
public:
    RawTerrainReader();
    ~RawTerrainReader();

    /**
     * @brief 加载二进制地形数据文件 (.bin) 及对应元数据 (.txt 或 .json)
     * @param binPath .bin 文件路径
     * @param metaPath 元数据文件路径 (若留空，自动按 .bin 同名同目录匹配 .txt 或 .json)
     * @param isFlippedY 导出 .bin 时是否勾选了 Y 轴垂直翻转
     * @return 成功返回 true，失败返回 false
     */
    bool Load(const std::string& binPath, const std::string& metaPath = "", bool isFlippedY = false);

    /**
     * @brief 卸载并释放内存数据
     */
    void Unload();

    /**
     * @brief 是否已成功加载数据
     */
    bool IsLoaded() const { return m_isLoaded; }

    /**
     * @brief 输入经纬度查询高程/海拔高度 (引擎推荐核心接口)
     * @param lat 纬度 Latitude (degrees, -90.0 ~ 90.0)
     * @param lon 经度 Longitude (degrees, -180.0 ~ 180.0)
     * @param outElevation 接收高程/海拔数值的引用 (单位: 米)
     * @param useBilinear 是否启用双线性插值 (true 为平滑插值，false 为最近邻采样)
     * @return 成功返回 true (坐标在栅格范围内且非 NoData)，超出范围或 NoData 返回 false
     */
    bool GetElevation(double lat, double lon, float& outElevation, bool useBilinear = true) const;

    /**
     * @brief 输入经纬度直接返回高程值 (便捷重载)
     * @param lat 纬度 Latitude
     * @param lon 经度 Longitude
     * @param fallbackValue 超出范围或 NoData 时的默认回退值 (如 0.0f)
     * @param useBilinear 是否启用双线性插值
     * @return 海拔高度值 (米)
     */
    float GetElevationDirect(double lat, double lon, float fallbackValue = 0.0f, bool useBilinear = true) const;

    /**
     * @brief 通过栅格像素行列号直接读取高程值
     * @param col 列号 (0 ~ width - 1)
     * @param row 行号 (0 ~ height - 1)
     * @param outVal 输出高程值
     * @return 成功返回 true，越界返回 false
     */
    bool GetPixelValue(int col, int row, float& outVal) const;

    /**
     * @brief 获取元数据信息
     */
    const RawTerrainMeta& GetMetadata() const { return m_meta; }

    /**
     * @brief 设置数据行是否进行了 Y 轴垂直翻转
     */
    void SetFlippedY(bool flipped) { m_meta.is_flipped_y = flipped; }

private:
    bool ParseMetadataTxt(const std::string& txtPath);
    bool ParseMetadataJson(const std::string& jsonPath);

    std::vector<float> m_heightData;
    RawTerrainMeta m_meta;
    bool m_isLoaded;
};

#endif // RAW_TERRAIN_READER_H
