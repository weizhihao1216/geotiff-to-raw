#include "GeoTiffConverter.h"

GeoTiffConverter::GeoTiffConverter()
    : m_poDataset(nullptr), m_isOpen(false) {
}

GeoTiffConverter::~GeoTiffConverter() {
    Close();
}

void GeoTiffConverter::InitGDAL() {
    static bool registered = false;
    if (!registered) {
        GDALAllRegister();
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
        registered = true;
    }
}

bool GeoTiffConverter::Open(const std::string& inputPath) {
    Close();
    InitGDAL();

    m_inputPath = inputPath;
    m_poDataset = static_cast<GDALDataset*>(GDALOpen(inputPath.c_str(), GA_ReadOnly));

    if (!m_poDataset) {
        std::cerr << "[错误] 无法打开 GDAL 数据集: " << inputPath << std::endl;
        return false;
    }

    m_metadata.width = m_poDataset->GetRasterXSize();
    m_metadata.height = m_poDataset->GetRasterYSize();

    if (m_metadata.width <= 0 || m_metadata.height <= 0) {
        std::cerr << "[错误] 无效的栅格尺寸: " << m_metadata.width << "x" << m_metadata.height << std::endl;
        Close();
        return false;
    }

    // 获取地理仿射变换参数
    double adfGeoTransform[6] = { 0.0, 1.0, 0.0, 0.0, 0.0, -1.0 };
    if (m_poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
        double x0 = adfGeoTransform[0];
        double res_x = adfGeoTransform[1];
        double rot_x = adfGeoTransform[2];
        double y0 = adfGeoTransform[3];
        double rot_y = adfGeoTransform[4];
        double res_y = adfGeoTransform[5];

        if (std::abs(rot_x) > 1e-9 || std::abs(rot_y) > 1e-9) {
            std::cout << "[警告] 栅格图像包含旋转或倾斜参数 (Rotation != 0)，经纬度范围将按外包矩形计算。" << std::endl;
        }

        double x1 = x0 + m_metadata.width * res_x + m_metadata.height * rot_x;
        double y1 = y0 + m_metadata.width * rot_y + m_metadata.height * res_y;

        m_metadata.min_lon = (std::min)(x0, x1);
        m_metadata.max_lon = (std::max)(x0, x1);
        m_metadata.min_lat = (std::min)(y0, y1);
        m_metadata.max_lat = (std::max)(y0, y1);

        m_metadata.res_x = std::abs(res_x);
        m_metadata.res_y = std::abs(res_y);
    } else {
        std::cout << "[警告] 数据集未包含标准的 GeoTransform 地理仿射变换信息。" << std::endl;
    }

    if (m_poDataset->GetProjectionRef()) {
        m_metadata.projection_wkt = m_poDataset->GetProjectionRef();
    }

    // 获取 Band 1 的 NoData 值信息
    GDALRasterBand* poBand = m_poDataset->GetRasterBand(1);
    if (poBand) {
        int bHasNoData = 0;
        double noDataVal = poBand->GetNoDataValue(&bHasNoData);
        if (bHasNoData) {
            m_metadata.has_nodata = true;
            m_metadata.nodata_value = noDataVal;
        }
    } else {
        std::cerr << "[错误] 数据集中未找到 Band 1！" << std::endl;
        Close();
        return false;
    }

    m_isOpen = true;
    return true;
}

void GeoTiffConverter::Close() {
    if (m_poDataset) {
        GDALClose(static_cast<GDALDatasetH>(m_poDataset));
        m_poDataset = nullptr;
    }
    m_isOpen = false;
    m_metadata = GeoMetadata();
}

bool GeoTiffConverter::ReadRasterData(std::vector<float>& buffer, bool flipY) {
    if (!m_isOpen || !m_poDataset) {
        std::cerr << "[错误] 栅格数据集未打开，无法读取数据。" << std::endl;
        return false;
    }

    GDALRasterBand* poBand = m_poDataset->GetRasterBand(1);
    if (!poBand) {
        std::cerr << "[错误] 获取 Band 1 失败。" << std::endl;
        return false;
    }

    size_t totalPixels = static_cast<size_t>(m_metadata.width) * static_cast<size_t>(m_metadata.height);
    try {
        buffer.resize(totalPixels);
    } catch (const std::bad_alloc& e) {
        std::cerr << "[错误] 内存分配失败 (需要 " << (totalPixels * sizeof(float)) / (1024 * 1024) << " MB): " << e.what() << std::endl;
        return false;
    }

    std::cout << "[信息] 正在读取波段数据 (" << m_metadata.width << " x " << m_metadata.height 
              << ", 共 " << totalPixels << " 像素) 并转换为 GDT_Float32..." << std::endl;

    CPLErr err = poBand->RasterIO(GF_Read, 0, 0, m_metadata.width, m_metadata.height,
                                  buffer.data(), m_metadata.width, m_metadata.height,
                                  GDT_Float32, 0, 0);

    if (err != CE_None) {
        std::cerr << "[错误] GDAL RasterIO 读取波段数据失败，错误码: " << err << std::endl;
        return false;
    }

    // 行数据垂直翻转 (Bottom-to-Top 布局)
    if (flipY) {
        std::cout << "[信息] 正在执行 Y 轴像素行垂直翻转..." << std::endl;
        int h = m_metadata.height;
        int w = m_metadata.width;
        for (int r = 0; r < h / 2; ++r) {
            int opposite_r = h - 1 - r;
            float* row1 = &buffer[static_cast<size_t>(r) * w];
            float* row2 = &buffer[static_cast<size_t>(opposite_r) * w];
            std::swap_ranges(row1, row1 + w, row2);
        }
    }

    return true;
}

bool GeoTiffConverter::GeneratePreviewThumbnail(std::vector<float>& outBuffer, int targetW, int targetH) {
    if (!m_isOpen || !m_poDataset || targetW <= 0 || targetH <= 0) return false;

    GDALRasterBand* poBand = m_poDataset->GetRasterBand(1);
    if (!poBand) return false;

    size_t totalPixels = static_cast<size_t>(targetW) * static_cast<size_t>(targetH);
    outBuffer.resize(totalPixels);

    CPLErr err = poBand->RasterIO(GF_Read, 0, 0, m_metadata.width, m_metadata.height,
                                  outBuffer.data(), targetW, targetH,
                                  GDT_Float32, 0, 0);

    return (err == CE_None);
}

bool GeoTiffConverter::WriteBinary(const std::string& outputPath, const std::vector<float>& buffer) {
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[错误] 无法创建输出二进制文件: " << outputPath << std::endl;
        return false;
    }

    size_t dataSizeBytes = buffer.size() * sizeof(float);
    std::cout << "[信息] 正在写入 Raw Binary 文件 (" << (dataSizeBytes / (1024.0 * 1024.0)) << " MB)..." << std::endl;

    outFile.write(reinterpret_cast<const char*>(buffer.data()), dataSizeBytes);
    if (!outFile.good()) {
        std::cerr << "[错误] 二进制文件写入过程发生故障！" << std::endl;
        return false;
    }

    outFile.close();
    std::cout << "[成功] Raw Binary 文件已成功保存至: " << outputPath << std::endl;
    return true;
}

bool GeoTiffConverter::WriteMetadataTxt(const std::string& outputPath) {
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "[错误] 无法创建元数据文本文件: " << outputPath << std::endl;
        return false;
    }

    outFile << std::fixed << std::setprecision(10);
    outFile << "width: " << m_metadata.width << "\n";
    outFile << "height: " << m_metadata.height << "\n";
    outFile << "min_lat: " << m_metadata.min_lat << "\n";
    outFile << "max_lat: " << m_metadata.max_lat << "\n";
    outFile << "min_lon: " << m_metadata.min_lon << "\n";
    outFile << "max_lon: " << m_metadata.max_lon << "\n";
    outFile << "res_x: " << m_metadata.res_x << "\n";
    outFile << "res_y: " << m_metadata.res_y << "\n";
    outFile << "has_nodata: " << (m_metadata.has_nodata ? 1 : 0) << "\n";
    outFile << "nodata_value: " << m_metadata.nodata_value << "\n";

    outFile.close();
    std::cout << "[成功] 文本元数据已保存至: " << outputPath << std::endl;
    return true;
}

bool GeoTiffConverter::WriteMetadataJson(const std::string& outputPath) {
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        std::cerr << "[错误] 无法创建元数据 JSON 文件: " << outputPath << std::endl;
        return false;
    }

    outFile << std::fixed << std::setprecision(10);
    outFile << "{\n";
    outFile << "  \"width\": " << m_metadata.width << ",\n";
    outFile << "  \"height\": " << m_metadata.height << ",\n";
    outFile << "  \"min_lat\": " << m_metadata.min_lat << ",\n";
    outFile << "  \"max_lat\": " << m_metadata.max_lat << ",\n";
    outFile << "  \"min_lon\": " << m_metadata.min_lon << ",\n";
    outFile << "  \"max_lon\": " << m_metadata.max_lon << ",\n";
    outFile << "  \"res_x\": " << m_metadata.res_x << ",\n";
    outFile << "  \"res_y\": " << m_metadata.res_y << ",\n";
    outFile << "  \"has_nodata\": " << (m_metadata.has_nodata ? "true" : "false") << ",\n";
    outFile << "  \"nodata_value\": " << m_metadata.nodata_value << "\n";
    outFile << "}\n";

    outFile.close();
    std::cout << "[成功] JSON 元数据已保存至: " << outputPath << std::endl;
    return true;
}
