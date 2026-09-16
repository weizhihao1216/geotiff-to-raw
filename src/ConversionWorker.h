#ifndef CONVERSION_WORKER_H
#define CONVERSION_WORKER_H

#include <QThread>
#include <QString>
#include "GeoTiffConverter.h"

/**
 * @brief 异步处理 GeoTIFF 转换的后台线程工作类
 */
class ConversionWorker : public QThread {
    Q_OBJECT
public:
    ConversionWorker(const QString& inputTiff,
                     const QString& outputBin,
                     bool flipY,
                     bool genTxt,
                     bool genJson,
                     const QString& customTxtPath = "",
                     const QString& customJsonPath = "",
                     QObject* parent = nullptr);

signals:
    /**
     * @brief 实时日志输出信号
     */
    void sigLog(const QString& msg);

    /**
     * @brief 进度百分比更新信号
     */
    void sigProgress(int percentage);

    /**
     * @brief 转换完成信号
     * @param success 是否成功
     * @param message 结果消息或错误原因
     */
    void sigFinished(bool success, const QString& message);

protected:
    void run() override;

private:
    QString m_inputTiff;
    QString m_outputBin;
    bool m_flipY;
    bool m_genTxt;
    bool m_genJson;
    QString m_customTxtPath;
    QString m_customJsonPath;
};

#endif // CONVERSION_WORKER_H
