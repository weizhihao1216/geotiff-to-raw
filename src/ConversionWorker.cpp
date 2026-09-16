#include "ConversionWorker.h"
#include <QFileInfo>
#include <QDebug>

ConversionWorker::ConversionWorker(const QString& inputTiff,
                                     const QString& outputBin,
                                     bool flipY,
                                     bool genTxt,
                                     bool genJson,
                                     const QString& customTxtPath,
                                     const QString& customJsonPath,
                                     QObject* parent)
    : QThread(parent),
      m_inputTiff(inputTiff),
      m_outputBin(outputBin),
      m_flipY(flipY),
      m_genTxt(genTxt),
      m_genJson(genJson),
      m_customTxtPath(customTxtPath),
      m_customJsonPath(customJsonPath) {
}

void ConversionWorker::run() {
    emit sigProgress(10);
    emit sigLog(QString("[开始] 打开 GeoTIFF 文件: %1").arg(m_inputTiff));

    GeoTiffConverter converter;
    if (!converter.Open(m_inputTiff.toUtf8().constData())) {
        emit sigLog("[错误] 无法打开 GeoTIFF 数据集，请检查文件格式及权限！");
        emit sigFinished(false, "打开 GeoTIFF 文件失败");
        return;
    }

    const GeoMetadata& meta = converter.GetMetadata();
    emit sigProgress(30);
    emit sigLog(QString("[信息] 数据尺寸: %1 x %2 像素").arg(meta.width).arg(meta.height));
    emit sigLog(QString("[信息] 经度范围: [%1, %2]").arg(meta.min_lon, 0, 'f', 6).arg(meta.max_lon, 0, 'f', 6));
    emit sigLog(QString("[信息] 纬度范围: [%1, %2]").arg(meta.min_lat, 0, 'f', 6).arg(meta.max_lat, 0, 'f', 6));

    if (m_flipY) {
        emit sigLog("[信息] 已启用 Y 轴垂直翻转 (Bottom-to-Top)");
    }

    std::vector<float> buffer;
    emit sigLog("[信息] 正在读取栅格波段并转换为 GDT_Float32 内存缓冲区...");
    emit sigProgress(50);

    if (!converter.ReadRasterData(buffer, m_flipY)) {
        emit sigLog("[错误] 栅格数据读取或转换失败！");
        emit sigFinished(false, "读取波段数据失败");
        return;
    }

    emit sigProgress(80);
    emit sigLog(QString("[信息] 正在导出原始二进制文件: %1").arg(m_outputBin));

    if (!converter.WriteBinary(m_outputBin.toUtf8().constData(), buffer)) {
        emit sigLog("[错误] 二进制文件写入失败！");
        emit sigFinished(false, "写入 Raw Binary 失败");
        return;
    }

    if (m_genTxt) {
        QString txtPath = m_customTxtPath;
        if (txtPath.isEmpty()) {
            QFileInfo info(m_outputBin);
            txtPath = info.absolutePath() + "/" + info.completeBaseName() + ".txt";
        }
        emit sigLog(QString("[信息] 导出文本元数据: %1").arg(txtPath));
        converter.WriteMetadataTxt(txtPath.toUtf8().constData());
    }

    if (m_genJson) {
        QString jsonPath = m_customJsonPath;
        if (jsonPath.isEmpty()) {
            QFileInfo info(m_outputBin);
            jsonPath = info.absolutePath() + "/" + info.completeBaseName() + ".json";
        }
        emit sigLog(QString("[信息] 导出 JSON 元数据: %1").arg(jsonPath));
        converter.WriteMetadataJson(jsonPath.toUtf8().constData());
    }

    emit sigProgress(100);
    emit sigLog("[成功] 所有导出任务已顺利完成！");
    emit sigFinished(true, "转换任务已成功完成");
}
