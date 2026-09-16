#include "RawTerrainReader.h"

RawTerrainReader::RawTerrainReader()
    : m_isLoaded(false) {
}

RawTerrainReader::~RawTerrainReader() {
    Unload();
}

void RawTerrainReader::Unload() {
    std::vector<float>().swap(m_heightData);
    m_meta = RawTerrainMeta();
    m_isLoaded = false;
}

bool RawTerrainReader::Load(const std::string& binPath, const std::string& metaPath, bool isFlippedY) {
    Unload();

    std::string metaFile = metaPath;

    // 1. 如果未手动指定元数据路径，自动尝试推导同目录同名的 .txt 或 .json 文件
    if (metaFile.empty()) {
        size_t dotPos = binPath.find_last_of('.');
        std::string basePath = (dotPos != std::string::npos) ? binPath.substr(0, dotPos) : binPath;

        std::string candidateTxt = basePath + ".txt";
        std::string candidateJson = basePath + ".json";

        std::ifstream testTxt(candidateTxt);
        if (testTxt.is_open()) {
            metaFile = candidateTxt;
            testTxt.close();
        } else {
            std::ifstream testJson(candidateJson);
            if (testJson.is_open()) {
                metaFile = candidateJson;
                testJson.close();
            }
        }
    }

    if (metaFile.empty()) {
        std::cerr << "[RawTerrainReader 错误] 未能自动定位对应的元数据文件 (.txt 或 .json)！" << std::endl;
        return false;
    }

    // 2. 解析元数据文件
    bool metaParsed = false;
    if (metaFile.size() >= 4 && metaFile.substr(metaFile.size() - 4) == ".json") {
        metaParsed = ParseMetadataJson(metaFile);
    } else {
        metaParsed = ParseMetadataTxt(metaFile);
    }

    if (!metaParsed) {
        std::cerr << "[RawTerrainReader 错误] 解析元数据文件失败: " << metaFile << std::endl;
        return false;
    }

    m_meta.is_flipped_y = isFlippedY;

    // 3. 读取二进制高程数据文件 (.bin)
    std::ifstream binFile(binPath, std::ios::binary | std::ios::ate);
    if (!binFile.is_open()) {
        std::cerr << "[RawTerrainReader 错误] 打开二进制高程文件失败: " << binPath << std::endl;
        return false;
    }

    std::streamsize fileSize = binFile.tellg();
    binFile.seekg(0, std::ios::beg);

    size_t expectedPixels = static_cast<size_t>(m_meta.width) * static_cast<size_t>(m_meta.height);
    size_t expectedBytes = expectedPixels * sizeof(float);

    if (fileSize < static_cast<std::streamsize>(expectedBytes)) {
        std::cerr << "[RawTerrainReader 错误] 二进制文件大小 (" << fileSize 
                  << " 字节) 小于元数据期望大小 (" << expectedBytes << " 字节)！" << std::endl;
        return false;
    }

    try {
        m_heightData.resize(expectedPixels);
    } catch (const std::bad_alloc&) {
        std::cerr << "[RawTerrainReader 错误] 内存分配失败，无法加载地形数据！" << std::endl;
        return false;
    }

    binFile.read(reinterpret_cast<char*>(m_heightData.data()), expectedBytes);
    if (!binFile.good()) {
        std::cerr << "[RawTerrainReader 错误] 读取二进制高程文件数据流中断！" << std::endl;
        Unload();
        return false;
    }

    m_isLoaded = true;
    std::cout << "[RawTerrainReader] 地形数据成功加载！分辨率: " 
              << m_meta.width << "x" << m_meta.height << ", 像素总数: " << expectedPixels << std::endl;

    return true;
}

bool RawTerrainReader::ParseMetadataTxt(const std::string& txtPath) {
    std::ifstream inFile(txtPath);
    if (!inFile.is_open()) return false;

    std::string line;
    while (std::getline(inFile, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string valStr = line.substr(colonPos + 1);

        // 去除前后空白字符
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        valStr.erase(0, valStr.find_first_not_of(" \t\r\n"));
        valStr.erase(valStr.find_last_not_of(" \t\r\n") + 1);

        try {
            if (key == "width") m_meta.width = std::stoi(valStr);
            else if (key == "height") m_meta.height = std::stoi(valStr);
            else if (key == "min_lat") m_meta.min_lat = std::stod(valStr);
            else if (key == "max_lat") m_meta.max_lat = std::stod(valStr);
            else if (key == "min_lon") m_meta.min_lon = std::stod(valStr);
            else if (key == "max_lon") m_meta.max_lon = std::stod(valStr);
            else if (key == "res_x") m_meta.res_x = std::stod(valStr);
            else if (key == "res_y") m_meta.res_y = std::stod(valStr);
            else if (key == "has_nodata") m_meta.has_nodata = (std::stoi(valStr) != 0);
            else if (key == "nodata_value") m_meta.nodata_value = static_cast<float>(std::stod(valStr));
        } catch (...) {
            continue;
        }
    }

    return (m_meta.width > 0 && m_meta.height > 0);
}

bool RawTerrainReader::ParseMetadataJson(const std::string& jsonPath) {
    std::ifstream inFile(jsonPath);
    if (!inFile.is_open()) return false;

    std::string line;
    while (std::getline(inFile, line)) {
        size_t quote1 = line.find('"');
        if (quote1 == std::string::npos) continue;
        size_t quote2 = line.find('"', quote1 + 1);
        if (quote2 == std::string::npos) continue;

        std::string key = line.substr(quote1 + 1, quote2 - quote1 - 1);
        size_t colonPos = line.find(':', quote2);
        if (colonPos == std::string::npos) continue;

        std::string valStr = line.substr(colonPos + 1);
        size_t commaPos = valStr.find(',');
        if (commaPos != std::string::npos) {
            valStr = valStr.substr(0, commaPos);
        }

        valStr.erase(0, valStr.find_first_not_of(" \t\r\n\""));
        valStr.erase(valStr.find_last_not_of(" \t\r\n\"") + 1);

        try {
            if (key == "width") m_meta.width = std::stoi(valStr);
            else if (key == "height") m_meta.height = std::stoi(valStr);
            else if (key == "min_lat") m_meta.min_lat = std::stod(valStr);
            else if (key == "max_lat") m_meta.max_lat = std::stod(valStr);
            else if (key == "min_lon") m_meta.min_lon = std::stod(valStr);
            else if (key == "max_lon") m_meta.max_lon = std::stod(valStr);
            else if (key == "res_x") m_meta.res_x = std::stod(valStr);
            else if (key == "res_y") m_meta.res_y = std::stod(valStr);
            else if (key == "has_nodata") m_meta.has_nodata = (valStr == "true" || valStr == "1");
            else if (key == "nodata_value") m_meta.nodata_value = static_cast<float>(std::stod(valStr));
        } catch (...) {
            continue;
        }
    }

    return (m_meta.width > 0 && m_meta.height > 0);
}

bool RawTerrainReader::GetPixelValue(int col, int row, float& outVal) const {
    if (!m_isLoaded || col < 0 || col >= m_meta.width || row < 0 || row >= m_meta.height) {
        return false;
    }
    size_t index = static_cast<size_t>(row) * static_cast<size_t>(m_meta.width) + static_cast<size_t>(col);
    outVal = m_heightData[index];
    return true;
}

bool RawTerrainReader::GetElevation(double lat, double lon, float& outElevation, bool useBilinear) const {
    if (!m_isLoaded) return false;

    // 1. 经纬度范围碰撞检测
    if (lon < m_meta.min_lon || lon > m_meta.max_lon || lat < m_meta.min_lat || lat > m_meta.max_lat) {
        return false;
    }

    // 2. 将经纬度映射到连续的栅格像素坐标 (u, v)
    double lonSpan = m_meta.max_lon - m_meta.min_lon;
    double latSpan = m_meta.max_lat - m_meta.min_lat;

    if (lonSpan <= 1e-12 || latSpan <= 1e-12) return false;

    double u = ((lon - m_meta.min_lon) / lonSpan) * (m_meta.width - 1);
    double v = 0.0;

    if (m_meta.is_flipped_y) {
        // 自南向北 (row 0 在最南侧/min_lat)
        v = ((lat - m_meta.min_lat) / latSpan) * (m_meta.height - 1);
    } else {
        // 自北向南 (标准的 GeoTIFF 图像排布，row 0 在最北侧/max_lat)
        v = ((m_meta.max_lat - lat) / latSpan) * (m_meta.height - 1);
    }

    // 3. 最近邻采样
    if (!useBilinear) {
        int col = static_cast<int>(std::round(u));
        int row = static_cast<int>(std::round(v));
        if (!GetPixelValue(col, row, outElevation)) return false;

        if (m_meta.has_nodata && std::abs(outElevation - m_meta.nodata_value) < 1e-4f) {
            return false; // 代表 NoData 无效像素
        }
        return true;
    }

    // 4. 双线性插值 (Bilinear Interpolation)
    int col0 = static_cast<int>(std::floor(u));
    int row0 = static_cast<int>(std::floor(v));
    int col1 = (std::min)(col0 + 1, m_meta.width - 1);
    int row1 = (std::min)(row0 + 1, m_meta.height - 1);

    double fx = u - col0;
    double fy = v - row0;

    float v00 = 0.0f, v10 = 0.0f, v01 = 0.0f, v11 = 0.0f;
    bool valid00 = GetPixelValue(col0, row0, v00) && (!m_meta.has_nodata || std::abs(v00 - m_meta.nodata_value) >= 1e-4f);
    bool valid10 = GetPixelValue(col1, row0, v10) && (!m_meta.has_nodata || std::abs(v10 - m_meta.nodata_value) >= 1e-4f);
    bool valid01 = GetPixelValue(col0, row1, v01) && (!m_meta.has_nodata || std::abs(v01 - m_meta.nodata_value) >= 1e-4f);
    bool valid11 = GetPixelValue(col1, row1, v11) && (!m_meta.has_nodata || std::abs(v11 - m_meta.nodata_value) >= 1e-4f);

    // 如果周边 4 像素均为有效像素，计算标准双线性插值
    if (valid00 && valid10 && valid01 && valid11) {
        double top = (1.0 - fx) * v00 + fx * v10;
        double bottom = (1.0 - fx) * v01 + fx * v11;
        outElevation = static_cast<float>((1.0 - fy) * top + fy * bottom);
        return true;
    }

    // 若部分像素为 NoData，进行带权有效像素退化加权插值
    double weightSum = 0.0;
    double valueSum = 0.0;

    auto accum = [&](bool valid, float val, double w) {
        if (valid) {
            valueSum += val * w;
            weightSum += w;
        }
    };

    accum(valid00, v00, (1.0 - fx) * (1.0 - fy));
    accum(valid10, v10, fx * (1.0 - fy));
    accum(valid01, v01, (1.0 - fx) * fy);
    accum(valid11, v11, fx * fy);

    if (weightSum > 1e-6) {
        outElevation = static_cast<float>(valueSum / weightSum);
        return true;
    }

    return false; // 全部 4 个邻近点均为 NoData
}

float RawTerrainReader::GetElevationDirect(double lat, double lon, float fallbackValue, bool useBilinear) const {
    float elevation = fallbackValue;
    if (GetElevation(lat, lon, elevation, useBilinear)) {
        return elevation;
    }
    return fallbackValue;
}
