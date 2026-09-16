#include "PointPickerWidget.h"
#include "GeoGazetteer.h"
#include <algorithm>
#include <cmath>

PointPickerWidget::PointPickerWidget(QWidget *parent)
    : QWidget(parent),
      m_flipY(false),
      m_hasImage(false),
      m_hasPickedPoint(false),
      m_pickedNormPos(0.5, 0.5),
      m_pickedLat(0.0),
      m_pickedLon(0.0),
      m_hasHover(false),
      m_hoverLat(0.0),
      m_hoverLon(0.0),
      m_scale(1.0),
      m_panOffset(0.0, 0.0),
      m_isDragging(false),
      m_dragStartPos(0, 0),
      m_lastMousePos(0, 0) {
    setMinimumHeight(350);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

QString PointPickerWidget::GetPickedPlaceName() const {
    return m_pickedPlaceName;
}

void PointPickerWidget::ResetZoom() {
    m_scale = 1.0;
    m_panOffset = QPointF(0.0, 0.0);
    m_isDragging = false;
    setCursor(Qt::CrossCursor);
    update();
}

QRectF PointPickerWidget::getBaseImageRect() const {
    if (width() <= 0 || height() <= 0 || m_meta.width <= 0 || m_meta.height <= 0) {
        return QRectF(0, 0, width(), height());
    }

    double imgAspect = static_cast<double>(m_meta.width) / static_cast<double>(m_meta.height);
    double widgetW = static_cast<double>(width());
    double widgetH = static_cast<double>(height());
    double widgetAspect = widgetW / widgetH;

    double baseW, baseH;
    if (widgetAspect > imgAspect) {
        baseH = widgetH;
        baseW = baseH * imgAspect;
    } else {
        baseW = widgetW;
        baseH = baseW / imgAspect;
    }

    double baseLeft = (widgetW - baseW) / 2.0;
    double baseTop = (widgetH - baseH) / 2.0;
    return QRectF(baseLeft, baseTop, baseW, baseH);
}

QRectF PointPickerWidget::getActiveImageRect() const {
    QRectF baseRect = getBaseImageRect();
    return QRectF(baseRect.x() + m_panOffset.x(),
                   baseRect.y() + m_panOffset.y(),
                   baseRect.width() * m_scale,
                   baseRect.height() * m_scale);
}

void PointPickerWidget::clampOffset() {
    if (!m_hasImage || width() <= 0 || height() <= 0) return;

    if (m_scale <= 1.0) {
        m_scale = 1.0;
        m_panOffset = QPointF(0.0, 0.0);
        return;
    }

    QRectF baseRect = getBaseImageRect();
    double imgW = baseRect.width() * m_scale;
    double imgH = baseRect.height() * m_scale;
    double widgetW = static_cast<double>(width());
    double widgetH = static_cast<double>(height());

    double panX = m_panOffset.x();
    double panY = m_panOffset.y();

    if (imgW <= widgetW) {
        panX = 0.0;
    } else {
        double minPanX = widgetW - baseRect.x() - imgW;
        double maxPanX = -baseRect.x();
        panX = (std::max)(minPanX, (std::min)(maxPanX, panX));
    }

    if (imgH <= widgetH) {
        panY = 0.0;
    } else {
        double minPanY = widgetH - baseRect.y() - imgH;
        double maxPanY = -baseRect.y();
        panY = (std::max)(minPanY, (std::min)(maxPanY, panY));
    }

    m_panOffset = QPointF(panX, panY);
}

QPointF PointPickerWidget::mapWidgetToNorm(const QPointF& widgetPos) const {
    QRectF activeRect = getActiveImageRect();
    if (activeRect.width() <= 0 || activeRect.height() <= 0) return QPointF(0.5, 0.5);

    double normX = (widgetPos.x() - activeRect.x()) / activeRect.width();
    double normY = (widgetPos.y() - activeRect.y()) / activeRect.height();

    normX = (std::max)(0.0, (std::min)(1.0, normX));
    normY = (std::max)(0.0, (std::min)(1.0, normY));

    return QPointF(normX, normY);
}

QPointF PointPickerWidget::mapNormToWidget(const QPointF& normPos) const {
    QRectF activeRect = getActiveImageRect();
    double px = activeRect.x() + normPos.x() * activeRect.width();
    double py = activeRect.y() + normPos.y() * activeRect.height();

    return QPointF(px, py);
}

void PointPickerWidget::Clear() {
    m_previewImage = QImage();
    m_hasImage = false;
    m_hasPickedPoint = false;
    ResetZoom();
}

bool PointPickerWidget::LoadFromConverter(GeoTiffConverter& converter, bool flipY) {
    m_meta = converter.GetMetadata();
    m_flipY = flipY;

    if (m_meta.width <= 0 || m_meta.height <= 0) {
        Clear();
        return false;
    }

    int targetW = 600;
    int targetH = 380;
    std::vector<float> thumbBuffer;

    if (!converter.GeneratePreviewThumbnail(thumbBuffer, targetW, targetH)) {
        Clear();
        return false;
    }

    // 统计极值以进行色彩归一化
    float minVal = 1e9f, maxVal = -1e9f;
    for (float val : thumbBuffer) {
        if (m_meta.has_nodata && std::abs(val - m_meta.nodata_value) < 1e-4f) continue;
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    if (minVal >= maxVal) {
        minVal = -100.0f;
        maxVal = 100.0f;
    }

    m_previewImage = QImage(targetW, targetH, QImage::Format_RGB32);

    for (int y = 0; y < targetH; ++y) {
        QRgb* scanline = reinterpret_cast<QRgb*>(m_previewImage.scanLine(y));
        for (int x = 0; x < targetW; ++x) {
            float val = thumbBuffer[static_cast<size_t>(y) * targetW + x];

            if (m_meta.has_nodata && std::abs(val - m_meta.nodata_value) < 1e-4f) {
                scanline[x] = qRgb(18, 20, 26); // NoData 背景暗色
                continue;
            }

            int r = 0, g = 0, b = 0;
            if (val < 0.0f) {
                // 水体 / 海底水深 (负高程): 深蓝 -> 湖蓝
                float wt = (val - minVal) / (0.0f - minVal + 1e-5f);
                wt = (std::max)(0.0f, (std::min)(1.0f, wt));
                r = static_cast<int>(10 + wt * 30);
                g = static_cast<int>(40 + wt * 140);
                b = static_cast<int>(90 + wt * 140);
            } else {
                // 陆地 (正高程): 森林绿 -> 暖黄 -> 高山雪白
                float lt = val / (maxVal + 1e-5f);
                lt = (std::max)(0.0f, (std::min)(1.0f, lt));
                if (lt < 0.5f) {
                    float k = lt * 2.0f;
                    r = static_cast<int>(34 + k * 180);
                    g = static_cast<int>(139 - k * 30);
                    b = static_cast<int>(34 - k * 10);
                } else {
                    float k = (lt - 0.5f) * 2.0f;
                    r = static_cast<int>(214 + k * 41);
                    g = static_cast<int>(109 + k * 146);
                    b = static_cast<int>(24 + k * 231);
                }
            }
            scanline[x] = qRgb(r, g, b);
        }
    }

    m_hasImage = true;
    ResetZoom();

    // 默认点选位置设为图像中心点
    SetPickedLocation((m_meta.min_lat + m_meta.max_lat) / 2.0, (m_meta.min_lon + m_meta.max_lon) / 2.0);
    return true;
}

void PointPickerWidget::SetPickedLocation(double lat, double lon) {
    if (!m_hasImage) return;

    m_pickedLat = lat;
    m_pickedLon = lon;
    m_pickedPlaceName = GeoGazetteer::Instance().FormatPlaceDescription(m_pickedLat, m_pickedLon);

    double lonSpan = m_meta.max_lon - m_meta.min_lon;
    double latSpan = m_meta.max_lat - m_meta.min_lat;

    if (lonSpan > 1e-12 && latSpan > 1e-12) {
        double normX = (lon - m_meta.min_lon) / lonSpan;
        double normY = 0.5;
        if (m_flipY) {
            normY = 1.0 - ((lat - m_meta.min_lat) / latSpan);
        } else {
            normY = (m_meta.max_lat - lat) / latSpan;
        }

        m_pickedNormPos = QPointF((std::max)(0.0, (std::min)(1.0, normX)),
                                  (std::max)(0.0, (std::min)(1.0, normY)));
        m_hasPickedPoint = true;
        update();
    }
}

bool PointPickerWidget::GetPickedLocation(double& outLat, double& outLon) const {
    if (!m_hasPickedPoint) return false;
    outLat = m_pickedLat;
    outLon = m_pickedLon;
    return true;
}

void PointPickerWidget::mousePressEvent(QMouseEvent *event) {
    if (!m_hasImage || width() <= 0 || height() <= 0) return;

    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_lastMousePos = event->pos();
        m_isDragging = false;
    }
}

void PointPickerWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_hasImage || width() <= 0 || height() <= 0) return;

    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->pos() - m_lastMousePos;
        if (!m_isDragging && (event->pos() - m_dragStartPos).manhattanLength() > 4) {
            m_isDragging = true;
            setCursor(Qt::ClosedHandCursor);
        }

        if (m_isDragging) {
            m_panOffset += QPointF(delta.x(), delta.y());
            clampOffset();
            m_lastMousePos = event->pos();
            m_hasHover = false;
            update();
            return;
        }
    }

    // 计算当前光标悬停位置对应的实际经纬度
    QRectF activeRect = getActiveImageRect();
    if (activeRect.contains(event->pos())) {
        QPointF normPos = mapWidgetToNorm(event->pos());
        double lonSpan = m_meta.max_lon - m_meta.min_lon;
        double latSpan = m_meta.max_lat - m_meta.min_lat;

        m_hoverLon = m_meta.min_lon + normPos.x() * lonSpan;
        if (m_flipY) {
            m_hoverLat = m_meta.min_lat + (1.0 - normPos.y()) * latSpan;
        } else {
            m_hoverLat = m_meta.max_lat - normPos.y() * latSpan;
        }

        m_hoverPlaceName = GeoGazetteer::Instance().FormatPlaceDescription(m_hoverLat, m_hoverLon);
        m_hoverWidgetPos = event->pos();
        m_hasHover = true;
    } else {
        m_hasHover = false;
    }
    update();
}

void PointPickerWidget::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    m_hasHover = false;
    update();
}

void PointPickerWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (!m_hasImage || width() <= 0 || height() <= 0) return;

    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            m_isDragging = false;
            setCursor(Qt::CrossCursor);
        } else {
            // 鼠标纯点击，更新点选位置
            QPointF normPos = mapWidgetToNorm(event->pos());
            m_pickedNormPos = normPos;

            double lonSpan = m_meta.max_lon - m_meta.min_lon;
            double latSpan = m_meta.max_lat - m_meta.min_lat;

            m_pickedLon = m_meta.min_lon + normPos.x() * lonSpan;

            if (m_flipY) {
                m_pickedLat = m_meta.min_lat + (1.0 - normPos.y()) * latSpan;
            } else {
                m_pickedLat = m_meta.max_lat - normPos.y() * latSpan;
            }

            m_pickedPlaceName = GeoGazetteer::Instance().FormatPlaceDescription(m_pickedLat, m_pickedLon);
            m_hasPickedPoint = true;
            update();

            emit signalPointPicked(m_pickedLat, m_pickedLon);
        }
    }
}

void PointPickerWidget::wheelEvent(QWheelEvent *event) {
    if (!m_hasImage || width() <= 0 || height() <= 0) return;

    double numDegrees = event->angleDelta().y() / 8.0;
    double numSteps = numDegrees / 15.0;
    if (std::abs(numSteps) < 1e-4) return;

    double factor = std::pow(1.15, numSteps);
    double oldScale = m_scale;
    double newScale = (std::max)(1.0, (std::min)(50.0, oldScale * factor));

    if (std::abs(newScale - oldScale) > 1e-5) {
        QPointF mousePos = event->pos();
        QRectF baseRect = getBaseImageRect();

        QRectF oldActiveRect(baseRect.x() + m_panOffset.x(),
                             baseRect.y() + m_panOffset.y(),
                             baseRect.width() * oldScale,
                             baseRect.height() * oldScale);

        double normX = (mousePos.x() - oldActiveRect.x()) / oldActiveRect.width();
        double normY = (mousePos.y() - oldActiveRect.y()) / oldActiveRect.height();

        m_scale = newScale;

        double newActiveW = baseRect.width() * newScale;
        double newActiveH = baseRect.height() * newScale;

        double newActiveX = mousePos.x() - normX * newActiveW;
        double newActiveY = mousePos.y() - normY * newActiveH;

        m_panOffset = QPointF(newActiveX - baseRect.x(), newActiveY - baseRect.y());
        clampOffset();
        update();
    }
    event->accept();
}

void PointPickerWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    if (m_hasImage) {
        ResetZoom();
    }
}

void PointPickerWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    clampOffset();
}

void PointPickerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景底色
    painter.fillRect(rect(), QColor(18, 20, 26));

    if (!m_hasImage) {
        painter.setPen(QColor(140, 150, 170));
        painter.setFont(QFont("Segoe UI", 12));
        painter.drawText(rect(), Qt::AlignCenter, "🗺️ 导入 GeoTIFF 文件后在此区域点击即可实时点选经纬度坐标");
        return;
    }

    // 绘制高清缩略图 (保持真实地理宽高比例，配合当前 scale 与 panOffset)
    QRectF activeRect = getActiveImageRect();
    painter.drawImage(activeRect, m_previewImage);

    // 绘制轻微经纬度网格辅线 (映射至实际图像坐标系)
    painter.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
    for (int i = 1; i < 4; ++i) {
        QPointF ptH0 = mapNormToWidget(QPointF(i / 4.0, 0.0));
        QPointF ptH1 = mapNormToWidget(QPointF(i / 4.0, 1.0));
        painter.drawLine(ptH0, ptH1);

        QPointF ptV0 = mapNormToWidget(QPointF(0.0, i / 4.0));
        QPointF ptV1 = mapNormToWidget(QPointF(1.0, i / 4.0));
        painter.drawLine(ptV0, ptV1);
    }

    // 绘制地图图像上的地名矢量标注图层
    double lonSpan = m_meta.max_lon - m_meta.min_lon;
    double latSpan = m_meta.max_lat - m_meta.min_lat;

    if (lonSpan > 1e-12 && latSpan > 1e-12) {
        const auto& allPlaces = GeoGazetteer::Instance().GetAllPlaces();
        QFont fontPlace("Segoe UI", 9, QFont::Bold);
        painter.setFont(fontPlace);

        for (const auto& place : allPlaces) {
            // 判断地名点是否处于当前加载图像的地理经纬度范围之内
            if (place.lon >= m_meta.min_lon && place.lon <= m_meta.max_lon &&
                place.lat >= m_meta.min_lat && place.lat <= m_meta.max_lat) {

                double normX = (place.lon - m_meta.min_lon) / lonSpan;
                double normY = m_flipY ? 1.0 - ((place.lat - m_meta.min_lat) / latSpan)
                                       : (m_meta.max_lat - place.lat) / latSpan;

                QPointF placePt = mapNormToWidget(QPointF(normX, normY));

                // 仅当像素坐标位于控件显示视野内部时才进行绘制
                if (rect().contains(placePt.toPoint())) {
                    int px = static_cast<int>(placePt.x());
                    int py = static_cast<int>(placePt.y());

                    // 根据地名类别选择不同的标记点样式
                    QColor dotColor(255, 230, 0); // 默认黄色
                    if (place.category.contains("海域") || place.category.contains("海峡") || place.category.contains("海洋") || place.category.contains("水域")) {
                        dotColor = QColor(0, 210, 255); // 海水类：湖蓝
                    } else if (place.category.contains("岛屿") || place.category.contains("群岛")) {
                        dotColor = QColor(0, 230, 168); // 岛屿类：青绿
                    }

                    // 绘制小圆点地标
                    painter.setPen(QPen(QColor(0, 0, 0, 180), 1));
                    painter.setBrush(dotColor);
                    painter.drawEllipse(QPoint(px, py), 4, 4);

                    // 绘制地名文字背景半透明气泡
                    QFontMetrics fm(fontPlace);
                    int textW = fm.horizontalAdvance(place.name) + 10;
                    int textH = fm.height() + 2;
                    QRect textBgRect(px + 6, py - textH / 2, textW, textH);

                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(15, 18, 24, 200));
                    painter.drawRoundedRect(textBgRect, 3, 3);

                    // 绘制地名文字
                    painter.setPen(QColor(240, 245, 255));
                    painter.drawText(textBgRect, Qt::AlignCenter, place.name);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // A. 绘制点选准星标（Crosshair Marker & Callout Badge）
    // -------------------------------------------------------------------------
    if (m_hasPickedPoint) {
        QPointF pt = mapNormToWidget(m_pickedNormPos);
        int px = static_cast<int>(pt.x());
        int py = static_cast<int>(pt.y());

        // 外发光圆圈
        painter.setPen(QPen(QColor(255, 50, 50, 200), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPoint(px, py), 12, 12);

        // 内点与准星
        painter.setPen(QPen(QColor(0, 230, 168), 2));
        painter.drawLine(px - 18, py, px + 18, py);
        painter.drawLine(px, py - 18, px, py + 18);

        painter.setBrush(QColor(255, 230, 0));
        painter.drawEllipse(QPoint(px, py), 4, 4);

        // 准星右上方绘制精美固定地名跟随卡片
        if (!m_pickedPlaceName.isEmpty()) {
            QFont fontCallout("Segoe UI", 9, QFont::Bold);
            painter.setFont(fontCallout);
            QFontMetrics fm(fontCallout);
            int badgeW = fm.horizontalAdvance(m_pickedPlaceName) + 16;
            int badgeH = 22;
            QRect badgeRect(px + 16, py - 26, badgeW, badgeH);

            painter.setPen(QPen(QColor(0, 230, 168, 220), 1));
            painter.setBrush(QColor(10, 14, 20, 220));
            painter.drawRoundedRect(badgeRect, 4, 4);

            painter.setPen(QColor(255, 230, 0));
            painter.drawText(badgeRect, Qt::AlignCenter, m_pickedPlaceName);
        }
    }

    // -------------------------------------------------------------------------
    // B. 鼠标悬停随动浮动气泡 (支持避让已选点准星卡片)
    // -------------------------------------------------------------------------
    if (m_hasHover && !m_isDragging) {
        int hx = static_cast<int>(m_hoverWidgetPos.x());
        int hy = static_cast<int>(m_hoverWidgetPos.y());

        QString hoverTip = QString("Lat %1°, Lon %2°")
                               .arg(m_hoverLat, 0, 'f', 6)
                               .arg(m_hoverLon, 0, 'f', 6);

        QFont fontHover("Segoe UI", 9);
        painter.setFont(fontHover);
        QFontMetrics fm(fontHover);
        int tipW = fm.horizontalAdvance(hoverTip) + 16;
        int tipH = 20;

        // 判定光标是否接近已选点准星，实现避让
        bool nearPickedPoint = false;
        if (m_hasPickedPoint) {
            QPointF pt = mapNormToWidget(m_pickedNormPos);
            double dist = std::hypot(hx - pt.x(), hy - pt.y());
            if (dist < 60.0) nearPickedPoint = true;
        }

        int tooltipX = nearPickedPoint ? (hx - tipW - 14) : (hx + 14);
        int tooltipY = hy + 14;

        if (tooltipX + tipW > width()) tooltipX = hx - tipW - 10;
        if (tooltipX < 10) tooltipX = 10;
        if (tooltipY + tipH > height() - 32) tooltipY = hy - tipH - 10;

        QRect hoverRect(tooltipX, tooltipY, tipW, tipH);
        painter.setPen(QPen(QColor(0, 210, 255, 180), 1));
        painter.setBrush(QColor(15, 20, 28, 210));
        painter.drawRoundedRect(hoverRect, 3, 3);

        painter.setPen(QColor(220, 235, 255));
        painter.drawText(hoverRect, Qt::AlignCenter, hoverTip);
    }

    // -------------------------------------------------------------------------
    // C. 在右上角绘制缩放倍率悬浮提示
    // -------------------------------------------------------------------------
    QString zoomTip;
    if (m_scale > 1.05) {
        zoomTip = QString("🔍 缩放: %1x (双击复位)").arg(m_scale, 0, 'f', 1);
    } else {
        zoomTip = QString("🔍 1.0x (滚轮缩放 / 拖拽平移)");
    }

    QFont fontZoom("Segoe UI", 9);
    painter.setFont(fontZoom);

    QRect tipZoomRect(width() - 200, 10, 190, 24);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawRoundedRect(tipZoomRect, 4, 4);

    painter.setPen(QColor(200, 210, 230));
    painter.drawText(tipZoomRect, Qt::AlignCenter, zoomTip);

    // -------------------------------------------------------------------------
    // D. 底部统一非重叠一体化双栏状态栏 (Full-width Unified Bottom Bar)
    // -------------------------------------------------------------------------
    QRect bottomBarRect(0, height() - 28, width(), 28);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(12, 15, 22, 220));
    painter.drawRect(bottomBarRect);

    // 顶部分割微光线条
    painter.setPen(QPen(QColor(255, 255, 255, 30), 1));
    painter.drawLine(0, height() - 28, width(), height() - 28);

    QFont fontBottom("Segoe UI", 9);
    painter.setFont(fontBottom);

    // D1. 底部左侧：已选点坐标与地名信息
    if (m_hasPickedPoint) {
        QString placeStr = m_pickedPlaceName.isEmpty() ? "📍 已选点" : m_pickedPlaceName;
        QString pickedTip = QString("📍 已选点: %1 | Lat %2°, Lon %3°")
                                .arg(placeStr)
                                .arg(m_pickedLat, 0, 'f', 6)
                                .arg(m_pickedLon, 0, 'f', 6);

        QRect leftTextRect(12, height() - 28, width() / 2 - 20, 28);
        painter.setPen(QColor(0, 230, 168));
        painter.drawText(leftTextRect, Qt::AlignVCenter | Qt::AlignLeft, pickedTip);
    } else {
        QRect leftTextRect(12, height() - 28, width() / 2 - 20, 28);
        painter.setPen(QColor(140, 150, 170));
        painter.drawText(leftTextRect, Qt::AlignVCenter | Qt::AlignLeft, "🗺️ 点击地图即可实时选择目标坐标点");
    }

    // D2. 底部右侧：鼠标悬停光标实时坐标与地名信息
    if (m_hasHover && !m_isDragging) {
        QString hoverBarTip = QString("🔍 光标位置: Lat %1°, Lon %2° | %3")
                                  .arg(m_hoverLat, 0, 'f', 6)
                                  .arg(m_hoverLon, 0, 'f', 6)
                                  .arg(m_hoverPlaceName);

        QRect rightTextRect(width() / 2, height() - 28, width() / 2 - 12, 28);
        painter.setPen(QColor(0, 210, 255));
        painter.drawText(rightTextRect, Qt::AlignVCenter | Qt::AlignRight, hoverBarTip);
    } else {
        QRect rightTextRect(width() / 2, height() - 28, width() / 2 - 12, 28);
        painter.setPen(QColor(140, 150, 170));
        painter.drawText(rightTextRect, Qt::AlignVCenter | Qt::AlignRight, "滚轮缩放 / 左键拖拽平移地图");
    }
}

