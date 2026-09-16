#ifndef POINT_PICKER_WIDGET_H
#define POINT_PICKER_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include "GeoTiffConverter.h"

/**
 * @brief GeoTIFF 地形/海底栅格图像可视化点选与坐标显示控件 (支持地图拖拽与滚轮缩放)
 */
class PointPickerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PointPickerWidget(QWidget *parent = nullptr);
    ~PointPickerWidget() override = default;

    /**
     * @brief 从 GeoTiffConverter 读取栅格生成高程热力/地形缩略图
     */
    bool LoadFromConverter(GeoTiffConverter& converter, bool flipY = false);

    /**
     * @brief 清除选点与预设数据
     */
    void Clear();

    /**
     * @brief 手动指定坐标高亮点选位置
     */
    void SetPickedLocation(double lat, double lon);

    /**
     * @brief 获取选中的地理经纬度
     */
    bool GetPickedLocation(double& outLat, double& outLon) const;

    /**
     * @brief 获取当前点选位置对应的离线地名描述
     */
    QString GetPickedPlaceName() const;

    /**
     * @brief 重置缩放与平移视野回到全景 (1.0x)
     */
    void ResetZoom();

signals:
    /**
     * @brief 用户在图像上点击触发的坐标信号
     */
    void signalPointPicked(double lat, double lon);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRectF getBaseImageRect() const;
    QRectF getActiveImageRect() const;
    QPointF mapWidgetToNorm(const QPointF& widgetPos) const;
    QPointF mapNormToWidget(const QPointF& normPos) const;
    void clampOffset();

    QImage m_previewImage;
    GeoMetadata m_meta;
    bool m_flipY;
    bool m_hasImage;
    bool m_hasPickedPoint;
    QPointF m_pickedNormPos; // 归一化点击坐标 (0.0 ~ 1.0)
    double m_pickedLat;
    double m_pickedLon;
    QString m_pickedPlaceName;

    // 鼠标移动悬停实时坐标状态
    bool m_hasHover;
    double m_hoverLat;
    double m_hoverLon;
    QString m_hoverPlaceName;
    QPointF m_hoverWidgetPos;

    // 地图缩放与平移控制变量
    double m_scale;        // 当前缩放倍率 (>= 1.0)
    QPointF m_panOffset;   // 平移偏移量 (像素)
    bool m_isDragging;     // 是否正在拖拽平移中
    QPoint m_dragStartPos; // 鼠标按下时的初始像素位置
    QPoint m_lastMousePos; // 上一次鼠标拖拽位置
};

#endif // POINT_PICKER_WIDGET_H

