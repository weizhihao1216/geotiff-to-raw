#ifndef GEO_TIFF_CONVERTER_H
#define GEO_TIFF_CONVERTER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <algorithm>

// GDAL Includes
#include "gdal_priv.h"
#include "cpl_conv.h"

/**
 * @brief GeoTIFF 图像的地理元数据结构体
 */
struct GeoMetadata {
    int width = 0;              ///< 图像宽度 (像素/列数)
    int height = 0;             ///< 图像高度 (行数)
    double min_lon = 0.0;       ///< 最小经度 (West)
    double max_lon = 0.0;       ///< 最大经度 (East)
    double min_lat = 0.0;       ///< 最小纬度 (South)
    double max_lat = 0.0;       ///< 最大纬度 (North)
    double res_x = 0.0;         ///< 经度方向分辨率 (X分辨率)
    double res_y = 0.0;         ///< 纬度方向分辨率 (Y分辨率)
    bool has_nodata = false;    ///< 是否包含 NoData 无效值标记
    double nodata_value = 0.0;  ///< NoData 无效值数值
    std::string projection_wkt; ///< WKT 投影空间参考定义
};

/**
 * @brief GeoTIFF 数据提取与格式转换核心类
 */
class GeoTiffConverter {
public:
    GeoTiffConverter();
    ~GeoTiffConverter();

    /**
     * @brief 全局初始化 GDAL 驱动注册
     */
    static void InitGDAL();

    /**
     * @brief 打开 GeoTIFF 文件并解析元数据
     * @param inputPath GeoTIFF 文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool Open(const std::string& inputPath);

    /**
     * @brief 关闭已打开的数据集
     */
    void Close();

    /**
     * @brief 获取已解析的地理元数据
     */
    const GeoMetadata& GetMetadata() const { return m_metadata; }

    /**
     * @brief 读取 Band 1 的栅格像素数据并统一转换为 GDT_Float32
     * @param buffer 接收数据的 32位浮点数 vector 缓冲区
     * @param flipY 是否垂直翻转行数据（自南向北像素排布）
     * @return 成功返回 true，失败返回 false
     */
    bool ReadRasterData(std::vector<float>& buffer, bool flipY = false);

    /**
     * @brief 快速重采样生成缩略图浮点缓冲区 (供点选预览界面使用)
     * @param outBuffer 输出 32 位浮点数组 (大小 targetW * targetH)
     * @param targetW 目标宽度
     * @param targetH 目标高度
     * @return 成功返回 true，失败返回 false
     */
    bool GeneratePreviewThumbnail(std::vector<float>& outBuffer, int targetW = 600, int targetH = 400);

    /**
     * @brief 将浮点数 vector 写入原始二进制文件 (.bin)
     * @param outputPath 输出的 bin 文件路径
     * @param buffer 浮点数缓冲区
     * @return 成功返回 true，失败返回 false
     */
    bool WriteBinary(const std::string& outputPath, const std::vector<float>& buffer);

    /**
     * @brief 导出文本格式元数据描述文件 (.txt)
     * @param outputPath 文本输出路径
     * @return 成功返回 true，失败返回 false
     */
    bool WriteMetadataTxt(const std::string& outputPath);

    /**
     * @brief 导出 JSON 格式元数据描述文件 (.json)
     * @param outputPath JSON 输出路径
     * @return 成功返回 true，失败返回 false
     */
    bool WriteMetadataJson(const std::string& outputPath);

private:
    GDALDataset* m_poDataset;
    GeoMetadata m_metadata;
    std::string m_inputPath;
    bool m_isOpen;
};

#endif // GEO_TIFF_CONVERTER_H
