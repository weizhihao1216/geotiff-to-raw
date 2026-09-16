#ifndef GEO_GAZETTEER_H
#define GEO_GAZETTEER_H

#include <QString>
#include <vector>

/**
 * @brief 离线地名与地标信息结构体
 */
struct PlaceItem {
    QString name;       // 地名 (例: 上海市 / 东海海域 / 台湾海峡)
    QString region;     // 所属省份/国家/区域
    QString category;   // 分类 (例: 城市 / 海域 / 岛屿 / 群岛)
    double lat;         // 纬度
    double lon;         // 经度
};

/**
 * @brief 离线地理名称与近邻空间索引匹配引擎
 */
class GeoGazetteer {
public:
    static GeoGazetteer& Instance();

    /**
     * @brief 查找距离指定 (lat, lon) 最近的离线地名
     * @param lat 目标纬度
     * @param lon 目标经度
     * @param outPlace 查找到的最近地名结构体
     * @param outDistanceKm 距离 (千米)
     * @param maxDistKm 最大搜索半径 (千米)
     * @return 是否找到匹配项
     */
    bool FindNearestPlace(double lat, double lon, PlaceItem& outPlace, double& outDistanceKm, double maxDistKm = 800.0) const;

    /**
     * @brief 格式化生成简短的地名描述字符串
     */
    QString FormatPlaceDescription(double lat, double lon) const;

    /**
     * @brief 获取所有离线地名数据列表
     */
    const std::vector<PlaceItem>& GetAllPlaces() const;

private:
    GeoGazetteer();
    void initBuiltinData();

    static double calculateHaversineDistance(double lat1, double lon1, double lat2, double lon2);

    std::vector<PlaceItem> m_places;
};

#endif // GEO_GAZETTEER_H
