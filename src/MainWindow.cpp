#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include "GeoTiffConverter.h"
#include "RawTerrainReader.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_worker(nullptr) {
    setupUi();
    applyStyleSheet();
    setAcceptDrops(true);
    setWindowTitle("geotiff-to-raw - GeoTIFF 转 Raw Binary 数据导出与测试工具 v1.0");
    resize(1180, 780);
}

MainWindow::~MainWindow() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->terminate();
        m_worker->wait();
    }
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主根布局：左右双列分栏 (QHBoxLayout)
    QHBoxLayout *rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setSpacing(14);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    // =========================================================================
    // 左侧分栏：转换配置、元数据预览、执行控制、控制台日志
    // =========================================================================
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    // 1. 顶栏标题 Header Card
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(12, 10, 12, 10);

    QLabel *lblTitle = new QLabel("geotiff-to-raw: GeoTIFF 高程/地形转 Raw Binary 工具", headerFrame);
    lblTitle->setObjectName("lblTitle");
    QLabel *lblDesc = new QLabel("快速解析 GEBCO/GeoTIFF 海底高程栅格，提取 32 位浮点 Raw 数据与地理空间元数据", headerFrame);
    lblDesc->setObjectName("lblSubTitle");

    headerLayout->addWidget(lblTitle);
    headerLayout->addWidget(lblDesc);
    leftLayout->addWidget(headerFrame);

    // 2. 步骤 1: 文件选择与参数配置卡片
    QGroupBox *boxFiles = new QGroupBox("1. 文件选择与转换参数", this);
    QGridLayout *gridFiles = new QGridLayout(boxFiles);
    gridFiles->setSpacing(10);

    QLabel *lblInput = new QLabel("输入 GeoTIFF:", boxFiles);
    m_editInputPath = new QLineEdit(boxFiles);
    m_editInputPath->setPlaceholderText("选择或拖拽 GeoTIFF 文件 (.tif / .tiff)...");
    m_btnBrowseInput = new QPushButton("浏览...", boxFiles);

    QLabel *lblOutput = new QLabel("输出 Binary:", boxFiles);
    m_editOutputPath = new QLineEdit(boxFiles);
    m_editOutputPath->setPlaceholderText("设置导出的原始二进制文件路径 (.bin)...");
    m_btnBrowseOutput = new QPushButton("浏览...", boxFiles);

    gridFiles->addWidget(lblInput, 0, 0);
    gridFiles->addWidget(m_editInputPath, 0, 1);
    gridFiles->addWidget(m_btnBrowseInput, 0, 2);

    gridFiles->addWidget(lblOutput, 1, 0);
    gridFiles->addWidget(m_editOutputPath, 1, 1);
    gridFiles->addWidget(m_btnBrowseOutput, 1, 2);

    // 选项复选框
    QVBoxLayout *boxOptions = new QVBoxLayout();
    boxOptions->setSpacing(4);
    m_chkFlipY = new QCheckBox("Y 轴数据垂直翻转 (--flip-y, 适配 Bottom-to-Top 渲染引擎)", boxFiles);

    QHBoxLayout *metaCheckLayout = new QHBoxLayout();
    m_chkGenTxt = new QCheckBox("生成文本元数据 (.txt)", boxFiles);
    m_chkGenTxt->setChecked(true);
    m_chkGenJson = new QCheckBox("生成 JSON 元数据 (.json)", boxFiles);
    m_chkGenJson->setChecked(true);
    metaCheckLayout->addWidget(m_chkGenTxt);
    metaCheckLayout->addWidget(m_chkGenJson);
    metaCheckLayout->addStretch();

    boxOptions->addWidget(m_chkFlipY);
    boxOptions->addLayout(metaCheckLayout);

    gridFiles->addLayout(boxOptions, 2, 0, 1, 3);
    leftLayout->addWidget(boxFiles);

    // 3. 步骤 2: 栅格地理元数据预览卡片
    QGroupBox *boxMeta = new QGroupBox("2. 输入图像元数据实时预览", this);
    QGridLayout *gridMeta = new QGridLayout(boxMeta);
    gridMeta->setSpacing(8);

    gridMeta->addWidget(new QLabel("图像尺寸 (W x H):", boxMeta), 0, 0);
    m_lblSize = new QLabel("尚未加载文件", boxMeta);
    m_lblSize->setObjectName("metaValue");
    gridMeta->addWidget(m_lblSize, 0, 1);

    gridMeta->addWidget(new QLabel("NoData 标记:", boxMeta), 0, 2);
    m_lblNoData = new QLabel("未检测", boxMeta);
    m_lblNoData->setObjectName("metaValue");
    gridMeta->addWidget(m_lblNoData, 0, 3);

    gridMeta->addWidget(new QLabel("经度范围 [Lon]:", boxMeta), 1, 0);
    m_lblLonRange = new QLabel("-", boxMeta);
    m_lblLonRange->setObjectName("metaValue");
    gridMeta->addWidget(m_lblLonRange, 1, 1);

    gridMeta->addWidget(new QLabel("纬度范围 [Lat]:", boxMeta), 1, 2);
    m_lblLatRange = new QLabel("-", boxMeta);
    m_lblLatRange->setObjectName("metaValue");
    gridMeta->addWidget(m_lblLatRange, 1, 3);

    gridMeta->addWidget(new QLabel("像素分辨率 [X, Y]:", boxMeta), 2, 0);
    m_lblRes = new QLabel("-", boxMeta);
    m_lblRes->setObjectName("metaValue");
    gridMeta->addWidget(m_lblRes, 2, 1);

    leftLayout->addWidget(boxMeta);

    // 4. 转换操作与控制进度
    QHBoxLayout *layoutBtns = new QHBoxLayout();
    m_btnStart = new QPushButton("开始转换与导出", this);
    m_btnStart->setObjectName("btnStart");
    m_btnStart->setMinimumHeight(38);

    m_btnClearLog = new QPushButton("清空日志", this);
    m_btnClearLog->setMinimumHeight(38);

    layoutBtns->addWidget(m_btnStart, 3);
    layoutBtns->addWidget(m_btnClearLog, 1);
    leftLayout->addLayout(layoutBtns);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    leftLayout->addWidget(m_progressBar);

    // 5. 运行控制台日志
    m_textLog = new QTextEdit(this);
    m_textLog->setReadOnly(true);
    leftLayout->addWidget(m_textLog, 1);

    rootLayout->addLayout(leftLayout, 5); // 左侧占比 5

    // =========================================================================
    // 右侧分栏：引擎端 RawTerrainReader 交互点选与测试专属面板
    // =========================================================================
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);

    QGroupBox *boxTest = new QGroupBox("3. 引擎端 RawTerrainReader 交互点选与高程测试面板", this);
    QVBoxLayout *layoutTestInner = new QVBoxLayout(boxTest);
    layoutTestInner->setSpacing(8);

    // A. 选点 .bin 文件配置
    QHBoxLayout *layoutTestBin = new QHBoxLayout();
    QLabel *lblTestBin = new QLabel("测试 .bin 文件:", boxTest);
    m_editTestBinPath = new QLineEdit(boxTest);
    m_editTestBinPath->setPlaceholderText("选择或自动使用导出的 .bin 文件路径...");
    m_btnBrowseTestBin = new QPushButton("浏览...", boxTest);

    layoutTestBin->addWidget(lblTestBin);
    layoutTestBin->addWidget(m_editTestBinPath, 1);
    layoutTestBin->addWidget(m_btnBrowseTestBin);
    layoutTestInner->addLayout(layoutTestBin);

    // B. 大视野地图点选 View Widget
    m_pointPickerWidget = new PointPickerWidget(boxTest);
    layoutTestInner->addWidget(m_pointPickerWidget, 1);

    // C. 坐标与插值控制
    QHBoxLayout *layoutCoordInputs = new QHBoxLayout();
    QLabel *lblTestLat = new QLabel("目标纬度 [Lat]:", boxTest);
    m_editTestLat = new QLineEdit(boxTest);
    m_editTestLat->setPlaceholderText("例: 30.5000");

    QLabel *lblTestLon = new QLabel("目标经度 [Lon]:", boxTest);
    m_editTestLon = new QLineEdit(boxTest);
    m_editTestLon->setPlaceholderText("例: 120.2000");

    m_chkTestBilinear = new QCheckBox("双线性插值", boxTest);
    m_chkTestBilinear->setChecked(true);

    layoutCoordInputs->addWidget(lblTestLat);
    layoutCoordInputs->addWidget(m_editTestLat);
    layoutCoordInputs->addWidget(lblTestLon);
    layoutCoordInputs->addWidget(m_editTestLon);
    layoutCoordInputs->addWidget(m_chkTestBilinear);
    layoutTestInner->addLayout(layoutCoordInputs);

    // D. 查询按钮与高亮结果显示
    QHBoxLayout *layoutQueryResult = new QHBoxLayout();
    m_btnQueryElevation = new QPushButton("查询高程/水深 (RawTerrainReader)", boxTest);
    m_btnQueryElevation->setObjectName("btnQuery");
    m_btnQueryElevation->setMinimumHeight(36);

    m_lblTestResult = new QLabel("查询结果: 等待测试", boxTest);
    m_lblTestResult->setObjectName("metaValue");

    layoutQueryResult->addWidget(m_btnQueryElevation, 2);
    layoutQueryResult->addWidget(m_lblTestResult, 3);
    layoutTestInner->addLayout(layoutQueryResult);

    rightLayout->addWidget(boxTest);
    rootLayout->addLayout(rightLayout, 6); // 右侧占比 6 (宽阔点选视野)

    // 信号槽绑定
    connect(m_btnBrowseInput, &QPushButton::clicked, this, &MainWindow::onBrowseInput);
    connect(m_btnBrowseOutput, &QPushButton::clicked, this, &MainWindow::onBrowseOutput);
    connect(m_editInputPath, &QLineEdit::textChanged, this, &MainWindow::onInputFileChanged);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartConversion);
    connect(m_btnClearLog, &QPushButton::clicked, this, &MainWindow::onClearLog);

    connect(m_btnBrowseTestBin, &QPushButton::clicked, this, &MainWindow::onBrowseTestBin);
    connect(m_btnQueryElevation, &QPushButton::clicked, this, &MainWindow::onQueryElevation);
    connect(m_pointPickerWidget, &PointPickerWidget::signalPointPicked, this, &MainWindow::onPointPicked);
}

void MainWindow::applyStyleSheet() {
    // 1. 优先尝试加载外部 themestyle.qss (供热重载与调试)
    QString qssPath = QCoreApplication::applicationDirPath() + "/themestyle.qss";
    QFile file(qssPath);
    if (!file.exists()) {
        qssPath = "themestyle.qss";
        file.setFileName(qssPath);
    }

    // 2. 若外部文件未找到或无法读取，自动回退至编译入二进制的 Qt 资源文件 (:/themestyle.qss)
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qssPath = ":/themestyle.qss";
        file.setFileName(qssPath);
        file.open(QFile::ReadOnly | QFile::Text);
    }

    if (file.isOpen()) {
        QString styleContent = QString::fromUtf8(file.readAll());
        setStyleSheet(styleContent);
        file.close();
    } else {
        // 3. 兜底默认背景样式
        setStyleSheet("QMainWindow { background-color: #16181d; color: #ffffff; }");
    }
}

void MainWindow::onBrowseInput() {
    QString fileName = QFileDialog::getOpenFileName(this, "选择输入 GeoTIFF 文件", "", "GeoTIFF 图像 (*.tif *.tiff);;所有文件 (*.*)");
    if (!fileName.isEmpty()) {
        m_editInputPath->setText(fileName);
    }
}

void MainWindow::onBrowseOutput() {
    QString fileName = QFileDialog::getSaveFileName(this, "设置导出的 Binary 文件路径", m_editOutputPath->text(), "Raw Binary (*.bin);;所有文件 (*.*)");
    if (!fileName.isEmpty()) {
        m_editOutputPath->setText(fileName);
        m_editTestBinPath->setText(fileName);
    }
}

void MainWindow::onBrowseTestBin() {
    QString fileName = QFileDialog::getOpenFileName(this, "选择要测试的 .bin 文件", m_editTestBinPath->text(), "Raw Binary (*.bin);;所有文件 (*.*)");
    if (!fileName.isEmpty()) {
        m_editTestBinPath->setText(fileName);
    }
}

void MainWindow::onInputFileChanged(const QString &filePath) {
    if (filePath.isEmpty()) return;

    QFileInfo info(filePath);
    if (info.exists() && info.isFile()) {
        if (m_editOutputPath->text().isEmpty() || m_editOutputPath->text().endsWith(".bin")) {
            QString defaultBin = info.absolutePath() + "/" + info.completeBaseName() + ".bin";
            m_editOutputPath->setText(defaultBin);
            m_editTestBinPath->setText(defaultBin);
        }
        updateMetadataPreview(filePath);
    }
}

void MainWindow::updateMetadataPreview(const QString &filePath) {
    GeoTiffConverter converter;
    if (converter.Open(filePath.toUtf8().constData())) {
        const GeoMetadata& meta = converter.GetMetadata();
        m_lblSize->setText(QString("%1 x %2 像素").arg(meta.width).arg(meta.height));
        m_lblLonRange->setText(QString("[%1, %2]").arg(meta.min_lon, 0, 'f', 6).arg(meta.max_lon, 0, 'f', 6));
        m_lblLatRange->setText(QString("[%1, %2]").arg(meta.min_lat, 0, 'f', 6).arg(meta.max_lat, 0, 'f', 6));
        m_lblRes->setText(QString("[%1, %2]").arg(meta.res_x, 0, 'f', 8).arg(meta.res_y, 0, 'f', 8));

        if (meta.has_nodata) {
            m_lblNoData->setText(QString("%1").arg(meta.nodata_value, 0, 'f', 2));
        } else {
            m_lblNoData->setText("无 NoData 标记");
        }

        // 生成地图缩略图并更新点选控件
        if (m_pointPickerWidget->LoadFromConverter(converter, m_chkFlipY->isChecked())) {
            slotLog(QString("[可视化点选] 已成功加载 GeoTIFF 高程图，支持在右侧大窗口直接点击选点！"));
        }

        // 自动将默认测试坐标填充为图像的中心经纬度
        double centerLat = (meta.min_lat + meta.max_lat) / 2.0;
        double centerLon = (meta.min_lon + meta.max_lon) / 2.0;

        m_editTestLat->setText(QString::number(centerLat, 'f', 6));
        m_editTestLon->setText(QString::number(centerLon, 'f', 6));

        slotLog(QString("[自动解析] 已成功读取文件元数据: %1").arg(QFileInfo(filePath).fileName()));
    } else {
        m_lblSize->setText("打开失败");
        m_lblLonRange->setText("-");
        m_lblLatRange->setText("-");
        m_lblRes->setText("-");
        m_lblNoData->setText("-");
        m_pointPickerWidget->Clear();
    }
}

void MainWindow::onPointPicked(double lat, double lon) {
    m_editTestLat->setText(QString::number(lat, 'f', 6));
    m_editTestLon->setText(QString::number(lon, 'f', 6));
    QString placeName = m_pointPickerWidget->GetPickedPlaceName();
    slotLog(QString("[地图点选] 点击位置: %1 -> 纬度: %2°, 经度: %3°")
                .arg(placeName.isEmpty() ? "未知区域" : placeName)
                .arg(lat, 0, 'f', 6)
                .arg(lon, 0, 'f', 6));

    // 点击后自动触发 RawTerrainReader 高程查询
    onQueryElevation();
}

void MainWindow::onQueryElevation() {
    QString binPath = m_editTestBinPath->text().trimmed();
    if (binPath.isEmpty() || !QFileInfo::exists(binPath)) {
        QMessageBox::warning(this, "测试参数错误", "请选择有效的 .bin 二进制文件！");
        return;
    }

    bool okLat = false, okLon = false;
    double lat = m_editTestLat->text().toDouble(&okLat);
    double lon = m_editTestLon->text().toDouble(&okLon);

    if (!okLat || !okLon) {
        QMessageBox::warning(this, "测试参数错误", "请输入有效的数字格式经纬度！");
        return;
    }

    RawTerrainReader reader;
    bool flipY = m_chkFlipY->isChecked();

    if (!reader.Load(binPath.toUtf8().constData(), "", flipY)) {
        m_lblTestResult->setText("查询失败: 无法读取 .bin 文件或对应的元数据！");
        slotLog("[RawTerrainReader 测试错误] 无法打开 .bin 文件或匹配对应的元数据 (.txt/.json)！");
        return;
    }

    // 更新点选控件高亮显示
    m_pointPickerWidget->SetPickedLocation(lat, lon);

    float elevation = 0.0f;
    bool useBilinear = m_chkTestBilinear->isChecked();
    bool success = reader.GetElevation(lat, lon, elevation, useBilinear);

    if (success) {
        QString modeStr = useBilinear ? "双线性插值" : "最近邻采样";
        QString placeName = m_pointPickerWidget->GetPickedPlaceName();
        m_lblTestResult->setText(QString("高程结果: %1 米 (%2) [%3]")
                                     .arg(elevation, 0, 'f', 3)
                                     .arg(modeStr)
                                     .arg(placeName));
        slotLog(QString("[RawTerrainReader 测试成功] 坐标 (%1, %2) [%3] -> 高程/水深: %4 米 [%5]")
                    .arg(lat, 0, 'f', 6)
                    .arg(lon, 0, 'f', 6)
                    .arg(placeName)
                    .arg(elevation, 0, 'f', 3)
                    .arg(modeStr));
    } else {
        m_lblTestResult->setText("查询结果: 超出地理范围或属于 NoData 无效像素");
        slotLog(QString("[RawTerrainReader 测试提示] 坐标 (%1, %2) 无有效高程数据 (超出范围或 NoData)").arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString filePath = urls.first().toLocalFile();
        if (filePath.endsWith(".tif", Qt::CaseInsensitive) || filePath.endsWith(".tiff", Qt::CaseInsensitive)) {
            m_editInputPath->setText(filePath);
            slotLog(QString("[拖拽识别] 已导入 GeoTIFF: %1").arg(filePath));
        } else if (filePath.endsWith(".bin", Qt::CaseInsensitive)) {
            m_editTestBinPath->setText(filePath);
            slotLog(QString("[拖拽识别] 已导入测试 Bin 文件: %1").arg(filePath));
        } else {
            QMessageBox::warning(this, "文件类型提示", "请拖入 GeoTIFF (.tif/.tiff) 或 Raw Binary (.bin) 文件！");
        }
    }
}

void MainWindow::onStartConversion() {
    QString inputTiff = m_editInputPath->text().trimmed();
    QString outputBin = m_editOutputPath->text().trimmed();

    if (inputTiff.isEmpty() || !QFileInfo::exists(inputTiff)) {
        QMessageBox::critical(this, "错误提示", "请选择有效的 GeoTIFF 输入文件！");
        return;
    }
    if (outputBin.isEmpty()) {
        QMessageBox::critical(this, "错误提示", "请设置导出 Raw Binary (.bin) 文件的输出路径！");
        return;
    }

    m_btnStart->setEnabled(false);
    m_btnBrowseInput->setEnabled(false);
    m_btnBrowseOutput->setEnabled(false);
    m_progressBar->setValue(0);

    slotLog("==================================================");
    slotLog("开始执行 GeoTIFF -> Raw Binary 转换任务...");

    m_worker = new ConversionWorker(inputTiff, outputBin,
                                    m_chkFlipY->isChecked(),
                                    m_chkGenTxt->isChecked(),
                                    m_chkGenJson->isChecked(),
                                    "", "", this);

    connect(m_worker, &ConversionWorker::sigLog, this, &MainWindow::slotLog);
    connect(m_worker, &ConversionWorker::sigProgress, this, &MainWindow::slotProgress);
    connect(m_worker, &ConversionWorker::sigFinished, this, &MainWindow::slotConversionFinished);

    m_worker->start();
}

void MainWindow::onClearLog() {
    m_textLog->clear();
}

void MainWindow::slotLog(const QString &msg) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_textLog->append(QString("[%1] %2").arg(timeStr, msg));
}

void MainWindow::slotProgress(int percentage) {
    m_progressBar->setValue(percentage);
}

void MainWindow::slotConversionFinished(bool success, const QString &message) {
    m_btnStart->setEnabled(true);
    m_btnBrowseInput->setEnabled(true);
    m_btnBrowseOutput->setEnabled(true);

    if (success) {
        m_editTestBinPath->setText(m_editOutputPath->text());
        QMessageBox::information(this, "导出成功", QString("转换任务已成功完成！\n已自动填入右侧接口测试栏。\n导出文件: %1").arg(m_editOutputPath->text()));
    } else {
        QMessageBox::critical(this, "转换失败", QString("发生错误: %1").arg(message));
    }
}
