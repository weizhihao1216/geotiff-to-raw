#define _CRT_SECURE_NO_WARNINGS

#include <QApplication>
#include <iostream>
#include <string>
#include <vector>
#include "MainWindow.h"
#include "GeoTiffConverter.h"

#ifdef _WIN32
#include <windows.h>
#endif

void PrintUsage(const char* exeName) {
    std::cout << "===============================================================\n";
    std::cout << " geotiff-to-raw: GeoTIFF 转 Raw Binary 转换工具\n";
    std::cout << "===============================================================\n";
    std::cout << "图形界面模式:\n";
    std::cout << "  直接双击运行程序或在命令行中无参数运行 " << exeName << " 打开 GUI 界面。\n\n";
    std::cout << "命令行模式 (静默批处理):\n";
    std::cout << "  " << exeName << " <input_tif_path> <output_bin_path> [选项]\n\n";
    std::cout << "必选参数:\n";
    std::cout << "  <input_tif_path>   输入的 GeoTIFF 高程/地形文件路径 (.tif / .tiff)\n";
    std::cout << "  <output_bin_path>  导出的 32位浮点二进制文件路径 (.bin)\n\n";
    std::cout << "可选选项:\n";
    std::cout << "  --flip-y           垂直翻转像素行（适应从左下角/西南角开始列优先的渲染引擎）\n";
    std::cout << "  --meta-txt <path>  手动指定文本元数据 (.txt) 输出路径\n";
    std::cout << "  --meta-json <path> 手动指定 JSON 元数据 (.json) 输出路径\n";
    std::cout << "  --no-txt           不生成文本格式元数据\n";
    std::cout << "  --no-json          不生成 JSON 格式元数据\n";
    std::cout << "  -h, --help         显示帮助说明信息\n";
    std::cout << "===============================================================\n";
}

std::string ReplaceOrAppendExtension(const std::string& filepath, const std::string& newExt) {
    size_t lastDot = filepath.find_last_of(".");
    size_t lastSep = filepath.find_last_of("/\\");

    if (lastDot != std::string::npos && (lastSep == std::string::npos || lastDot > lastSep)) {
        return filepath.substr(0, lastDot) + newExt;
    }
    return filepath + newExt;
}

int RunCliMode(int argc, char* argv[]) {
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fpDummy;
        freopen_s(&fpDummy, "CONOUT$", "w", stdout);
        freopen_s(&fpDummy, "CONOUT$", "w", stderr);
    }
#endif

    std::string inputTiffPath = argv[1];
    std::string outputBinPath = argv[2];

    bool flipY = false;
    bool genTxt = true;
    bool genJson = true;

    std::string customMetaTxtPath = ReplaceOrAppendExtension(outputBinPath, ".txt");
    std::string customMetaJsonPath = ReplaceOrAppendExtension(outputBinPath, ".json");

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--flip-y") {
            flipY = true;
        } else if (arg == "--no-txt") {
            genTxt = false;
        } else if (arg == "--no-json") {
            genJson = false;
        } else if (arg == "--meta-txt" && i + 1 < argc) {
            customMetaTxtPath = argv[++i];
        } else if (arg == "--meta-json" && i + 1 < argc) {
            customMetaJsonPath = argv[++i];
        } else {
            std::cerr << "[警告] 忽略未识别的参数: " << arg << std::endl;
        }
    }

    std::cout << "\n=== geotiff-to-raw 命令行模式转换开始 ===" << std::endl;
    std::cout << "输入文件: " << inputTiffPath << std::endl;
    std::cout << "输出文件: " << outputBinPath << std::endl;
    if (flipY) {
        std::cout << "Y 轴模式: 已启用垂直翻转 (--flip-y)" << std::endl;
    }

    GeoTiffConverter converter;
    if (!converter.Open(inputTiffPath)) {
        std::cerr << "[错误] 打开 GeoTIFF 文件失败！" << std::endl;
        return 2;
    }

    const GeoMetadata& meta = converter.GetMetadata();
    std::cout << "\n--- 解析元数据信息 ---" << std::endl;
    std::cout << "图像尺寸: " << meta.width << " x " << meta.height << " 像素" << std::endl;
    std::cout << "经度范围 [Lon]: [" << meta.min_lon << ", " << meta.max_lon << "]" << std::endl;
    std::cout << "纬度范围 [Lat]: [" << meta.min_lat << ", " << meta.max_lat << "]" << std::endl;
    std::cout << "分辨率 [ResX, ResY]: " << meta.res_x << ", " << meta.res_y << std::endl;
    if (meta.has_nodata) {
        std::cout << "NoData 标记值: " << meta.nodata_value << std::endl;
    }
    std::cout << "-----------------------\n" << std::endl;

    std::vector<float> rasterBuffer;
    if (!converter.ReadRasterData(rasterBuffer, flipY)) {
        std::cerr << "[错误] 读取栅格数据失败！" << std::endl;
        return 3;
    }

    if (!converter.WriteBinary(outputBinPath, rasterBuffer)) {
        std::cerr << "[错误] 写入原始二进制文件失败！" << std::endl;
        return 4;
    }

    if (genTxt) {
        converter.WriteMetadataTxt(customMetaTxtPath);
    }
    if (genJson) {
        converter.WriteMetadataJson(customMetaJsonPath);
    }

    std::cout << "\n=== 转换任务成功完成！ ===" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("geotiff-to-raw");
    app.setApplicationDisplayName("geotiff-to-raw - GeoTIFF 转 Raw Binary 转换工具");

    if (argc >= 2) {
        std::string firstArg = argv[1];
        if (firstArg == "-h" || firstArg == "--help") {
#ifdef _WIN32
            if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                FILE* fpDummy;
                freopen_s(&fpDummy, "CONOUT$", "w", stdout);
                freopen_s(&fpDummy, "CONOUT$", "w", stderr);
            }
#endif
            PrintUsage(argv[0]);
            return 0;
        }
    }

    // 若从命令行传入了输入和输出文件参数，则运行静默命令行转换模式
    if (argc >= 3) {
        return RunCliMode(argc, argv);
    }

    // 默认启动图形化 GUI 界面并最大化显示
    MainWindow window;
    window.showMaximized();

    return app.exec();
}
