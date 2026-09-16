#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "ConversionWorker.h"
#include "RawTerrainReader.h"
#include "PointPickerWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onBrowseInput();
    void onBrowseOutput();
    void onInputFileChanged(const QString &filePath);
    void onStartConversion();
    void onClearLog();

    // 引擎接口测试与点选槽函数
    void onBrowseTestBin();
    void onQueryElevation();
    void onPointPicked(double lat, double lon);

    void slotLog(const QString &msg);
    void slotProgress(int percentage);
    void slotConversionFinished(bool success, const QString &message);

private:
    void setupUi();
    void applyStyleSheet();
    void updateMetadataPreview(const QString &filePath);

    // UI Widgets - 转换配置
    QLineEdit *m_editInputPath;
    QPushButton *m_btnBrowseInput;

    QLineEdit *m_editOutputPath;
    QPushButton *m_btnBrowseOutput;

    QCheckBox *m_chkFlipY;
    QCheckBox *m_chkGenTxt;
    QCheckBox *m_chkGenJson;

    // Metadata Preview Labels
    QLabel *m_lblSize;
    QLabel *m_lblLonRange;
    QLabel *m_lblLatRange;
    QLabel *m_lblRes;
    QLabel *m_lblNoData;

    QPushButton *m_btnStart;
    QPushButton *m_btnClearLog;

    // UI Widgets - 引擎 RawTerrainReader 接口测试模块 & 地图点选控件
    QLineEdit *m_editTestBinPath;
    QPushButton *m_btnBrowseTestBin;
    QLineEdit *m_editTestLat;
    QLineEdit *m_editTestLon;
    QCheckBox *m_chkTestBilinear;
    QPushButton *m_btnQueryElevation;
    QLabel *m_lblTestResult;

    PointPickerWidget *m_pointPickerWidget; // 图像/地图点选控件

    QProgressBar *m_progressBar;
    QTextEdit *m_textLog;

    ConversionWorker *m_worker;
};

#endif // MAIN_WINDOW_H
