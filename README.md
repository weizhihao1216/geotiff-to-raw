# geotiff-to-raw - 地理地形与高程数据处理平台

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-11-blue.svg" alt="C++11" />
  <img src="https://img.shields.io/badge/Qt-5.x-41CD52.svg?logo=qt" alt="Qt5" />
  <img src="https://img.shields.io/badge/GDAL-1.11%20%7C%202.x%20%7C%203.x-028d35.svg" alt="GDAL" />
  <img src="https://img.shields.io/badge/Platform-Windows%20x64-0078D6.svg?logo=windows" alt="Windows x64" />
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="MIT License" />
</p>

`geotiff-to-raw`（可执行目标名：`geotiff-to-raw.exe`）是一个基于 C++11、GDAL 与 Qt5 开发的高性能 GeoTIFF 栅格地形/水深高程数据处理与格式转换平台。

本工具旨在将各类超大地理空间高程/水深栅格数据（如 GEBCO 全球水深与陆地高程、ETOPO、SRTM 等 GeoTIFF）高效转换为紧凑的 32 位浮点数原始二进制高度场 (`.bin`)，并同步导出规范的元数据描述（`.txt` 与 `.json`），供各类游戏引擎（Unreal Engine、Unity、自研 C++ 渲染引擎）直接进行纳秒级内存加载与双线性插值采样。

工具集成了**现代暗色图形化界面 (GUI)** 与**命令行静默批处理 (CLI)** 双模式，内置**交互式热力地图缩略预览**、**离线地名空间检索 (`GeoGazetteer`)**、**后台异步流水线转换**，并提供**零第三方依赖的游戏引擎端 C++ SDK (`RawTerrainReader`)**。

---

## 🌟 核心特性

- **双模式运行 (GUI + CLI)**:
  - 默认双击或无参运行启动图形化 UI 界面 (`MainWindow`)。
  - 传入命令行参数时自动切换至静默批处理模式，适合构建流水线或脚本自动化批处理。
- **可视化地图点选与高程预览 (`PointPickerWidget`)**:
  - 动态生成 GeoTIFF 高程/水深热力图缩略图。
  - 支持地图拖拽平移 (Pan)、滚轮缩放 (Zoom)、双击/按钮重置视野。
  - 鼠标移动实时悬停显示经纬度与离线近邻地名。
  - 点击选点直接交互获取经纬度与对应海拔高度，并可一键填入采样测试模块。
- **离线地名空间索引 (`GeoGazetteer`)**:
  - 内置离线地理数据库（涵盖主要城市、海域、岛屿、海峡等）。
  - 基于 Haversine 大圆距离实时计算与搜索近邻地名描述，无需联网即可定位。
- **坐标系翻转适配 (`--flip-y`)**:
  - 支持 Y 轴垂直翻转导出（自南向北像素排布），完美适配 OpenGL / UE / Unity 等左下角/西南角坐标系引擎。
- **内置引擎 SDK 测试面板**:
  - GUI 界面集成 `RawTerrainReader` 查询测试功能，可直接对导出的 `.bin` 数据进行特定经纬度高程查询验证（支持双线性插值与最近邻采样）。
- **零依赖游戏引擎 C++ SDK (`RawTerrainReader`)**:
  - 纯 C++11 标准库实现，**零第三方库依赖**。
  - 可直接复制至 Unreal Engine、Unity Native 插件或自研 C++ 渲染引擎中，提供输入 `(Lat, Lon)` 实时双线性插值采样海拔高度的核心 API。
- **异步多线程转换 (`ConversionWorker`)**:
  - 大文件栅格读取与二进制写入采用后台工作线程处理，界面 UI 保持流畅响应，支持实时日志与百分比进度条。
- **一键自动化构建与打包 (`build.bat`)**:
  - 一键 CMake 配置、编译 Release 目标，并自动将可执行文件、依赖 DLLs (Qt, GDAL)、QSS 样式表及 SDK 源码打包至 `dist/` 目录。

---

## 📁 目录结构

```
geotiff-to-raw/
├── CMakeLists.txt         # CMake 构建工程定义
├── build.bat              # Windows 一键编译与分发打包脚本
├── README.md              # 项目主说明文档
├── CONTRIBUTING.md        # 开源贡献指南
├── LICENSE                # MIT 开源许可证
├── .gitignore             # Git 忽略规则配置 (排除构建产物、dist包及超大测试数据)
├── resources/             # Qt 资源文件目录
│   ├── resources.qrc      # QRC 资源清单
│   └── themestyle.qss     # 现代暗色主题样式表
├── src/                   # 源代码目录
│   ├── main.cpp           # 程序统一入口 (GUI / CLI 双模式路由)
│   ├── MainWindow.h/cpp   # Qt 主窗口及界面交互逻辑
│   ├── PointPickerWidget.h/cpp # 可视化地图点选与高程预览控件
│   ├── GeoTiffConverter.h/cpp # GDAL 栅格转换与元数据导出核心类
│   ├── ConversionWorker.h/cpp # QThread 后台异步转换工作类
│   ├── RawTerrainReader.h/cpp # 引擎侧地形数据读取与插值采样 SDK (纯 C++11)
│   └── GeoGazetteer.h/cpp # 离线地名数据库与空间匹配索引
├── data/                  # 示例数据与元数据目录
│   ├── README.md          # 数据说明与 GEBCO 下载指南
│   ├── gebco_...txt       # 导出的元数据格式示例 (文本)
│   └── gebco_...json      # 导出的元数据格式示例 (JSON)
└── dist/                  # [运行 build.bat 后生成] 免安装绿色发布包 (exe/dlls/sdk)
```

> [!NOTE]
> 编译输出目录 `build/`、绿色发布包 `dist/` 以及大体积测试数据（`.tif`、`.bin`）已在 `.gitignore` 中默认排除，保持 Git 仓库轻量与纯净。

---

## 📥 预编译版本与测试数据下载 (免环境配置)

如果您不需要修改源码，仅希望直接使用该工具或获取开箱即用的高度场测试数据，可前往本仓库右侧的 [Releases](https://github.com/) 页面按需下载对应版本的发布包：

| 发布包名称 (符合标准格式 A) | 体积 | 包含内容 | 适用场景 / 推荐人群 |
| :--- | :---: | :--- | :--- |
| **`geotiff-to-raw_v1.0.0_windows_x64.zip`**<br>*(标准程序发布包)* | **~11 MB** | • `geotiff-to-raw.exe` 二合一主程序<br>• Qt5 / GDAL 完整运行时 DLL<br>• 平台插件与主题样式表<br>• `sdk/` 游戏引擎端 C++ 采样源码<br>• 完整说明文档与许可 | **⭐ 绝大多数用户首选**。<br>自带 GeoTIFF 数据；或后续版本升级时快速下载（无需重复下载庞大地图数据）。 |
| **`geotiff-to-raw_v1.0.0_sample_data.zip`**<br>*(示例测试数据集包)* | **~83 MB** | • 示例 GeoTIFF 高程源图 (`.tif`)<br>• 转换导出的 32 位浮点高度场 (`.bin`)<br>• 配套空间元数据 (`.json` / `.txt`)<br>• `data/README.md` 数据说明 | **需要示例数据的开发者**。<br>解压后将文件放入 `data/` 目录，即可直接在软件中测试或对接游戏引擎 SDK 验证插值效果。 |
| **`geotiff-to-raw_v1.0.0_windows_x64_full.zip`**<br>*(全合一开箱即用版)* | **~94 MB** | • 内置标准程序包的所有可执行文件与驱动<br>• **内置** `data/` 目录下的完整测试数据 | **新手 / 懒人一体化体验**。<br>解压后双击即可直接点选测试或运行 SDK，无需单独下载或手动移动数据文件。 |

> [!NOTE]
> **关于数据文件的发布策略说明**：
> 原始高程 GeoTIFF (`.tif`) 与转换后的二进制高度场 (`.bin`) 通常体积在数十 MB 至数百 MB 以上。为了避免 Git 源码仓库体积永久膨胀并保证 `git clone` 毫秒级拉取，**大体积测试数据未直接存放在 Git 仓库中，而是作为独立附件上传至 GitHub Releases 发布页**。

---

## 🛠️ 从源码编译

### 环境要求

- **操作系统**: Windows 7 / 10 / 11 (x64)
- **编译器**: Visual Studio 2017 (MSVC v141) 或更新版本 (支持 C++11)
- **构建工具**: CMake 3.10 或更高版本
- **第三方库与环境变量**:
  - **环境变量 `ThirdPart`**: 建议在系统或用户环境变量中配置 `ThirdPart`（例如 `D:\HR\DEV\ThirdParty`），工程将自动索引 `ThirdPart/gdal111` 与 `ThirdPart/Qt5.11`；
  - **Qt 5.x** (包含 Widgets, Core, Gui 模块)；
  - **GDAL** (1.11 / 2.x / 3.x)。

### 1. 一键编译与打包 (推荐)

直接运行根目录下的 `build.bat` 脚本：

```cmd
build.bat
```

脚本将自动完成：
1. 优先读取系统 `ThirdPart` 环境变量推导依赖路径（亦支持单独指定 `QT_DIR`, `GDAL_INC`, `GDAL_LIB`, `GDAL_DLL`）；
2. 执行 CMake 配置（生成 64 位 Visual Studio 工程）；
3. 以 Release 模式编译 `geotiff-to-raw.exe`；
4. 将可执行程序、动态库 (Qt5, GDAL, qwindows.dll 平台插件)、QSS 样式表及 `RawTerrainReader` SDK 源码自动整理复制至 `dist/` 目录。

编译完成后，进入 `dist/` 目录直接双击 `geotiff-to-raw.exe` 即可运行。

### 2. 手动 CMake 编译

在配置好 `ThirdPart` 环境变量后，直接运行即可自动识别依赖：

```cmd
cmake -B build -A x64
cmake --build build --config Release
```

若需自定义指定特定路径，亦可通过命令行显式传入参数：

```cmd
cmake -B build -A x64 ^
      -DGDAL_INCLUDE_DIR="D:/path/to/gdal/include" ^
      -DGDAL_LIBRARY="D:/path/to/gdal/lib/gdal_i.lib" ^
      -DQT5_INSTALL_DIR="D:/path/to/Qt5.11/vc140/x64"

cmake --build build --config Release
```

---

## 📖 使用指南

### 1. 图形界面模式 (GUI Mode)

双击 `geotiff-to-raw.exe` 即可启动图形界面：

1. **输入与选项配置**:
   - 点击“浏览”或直接将 GeoTIFF 文件 (`.tif` / `.tiff`) **拖拽至窗口**中。
   - 勾选转换选项：
     - `垂直翻转 Y 轴 (--flip-y)`：将像素行翻转为自南向北排布。
     - `生成 .txt 元数据` / `生成 .json 元数据`。
2. **可视化地图点选**:
   - 文件加载后，右侧自动渲染地形/水深热力缩略图。
   - **鼠标左键拖拽平移**，**滚轮缩放**视野，**双击**恢复全景。
   - 鼠标悬停实时查看光标位置的经纬度与就近地名。点击目标点自动将坐标提取至下方测试模块。
3. **开始转换**:
   - 点击“开始转换”按钮，后台工作线程异步处理，进度条与日志窗口实时更新。
4. **引擎 SDK 查询验证**:
   - 在底部的“引擎接口测试”模块中，点击“加载刚转换的文件”即可加载 `.bin`，输入经纬度验证高程采样与双线性插值效果。

---

## 2. 命令行批处理模式 (CLI Batch Mode)

传入命令行参数时，工具自动运行于静默批处理模式，适合接入自动化资产处理管线：

#### 语法格式

```cmd
geotiff-to-raw.exe <input_tif_path> <output_bin_path> [选项]
```

#### 参数说明

| 参数 / 选项 | 说明 |
| :--- | :--- |
| `<input_tif_path>` | 输入的 GeoTIFF 高程/水深文件路径 (`.tif` / `.tiff`) **[必选]** |
| `<output_bin_path>` | 导出的 32 位浮点二进制文件路径 (`.bin`) **[必选]** |
| `--flip-y` | 垂直翻转像素行（自南向北排布，适应西南角原点的渲染引擎） |
| `--meta-txt <path>` | 自定义导出的文本元数据 (`.txt`) 文件路径 |
| `--meta-json <path>`| 自定义导出的 JSON 元数据 (`.json`) 文件路径 |
| `--no-txt` | 不生成文本格式元数据 |
| `--no-json` | 不生成 JSON 格式元数据 |
| `-h, --help` | 显示命令行帮助信息 |

#### CLI 示例

```cmd
# 基础转换 (生成 gebco.bin, gebco.txt, gebco.json)
geotiff-to-raw.exe GEBCO_2026.tif gebco.bin

# 启用 Y 轴垂直翻转
geotiff-to-raw.exe GEBCO_2026.tif gebco.bin --flip-y

# 自定义元数据输出位置
geotiff-to-raw.exe GEBCO_2026.tif gebco.bin --meta-txt D:/meta/info.txt --meta-json D:/meta/info.json
```

---

## 📄 元数据结构说明

转换后同步生成的 `.txt` 与 `.json` 记录了空间参考的关键地理坐标与分辨率信息：

### 1. `metadata.txt`
```txt
width: 5280
height: 6240
min_lat: 15.0000000000
max_lat: 41.0000000000
min_lon: 104.0000000000
max_lon: 126.0000000000
res_x: 0.0041666667
res_y: 0.0041666667
has_nodata: 1
nodata_value: -32767.000000
```

### 2. `metadata.json`
```json
{
  "width": 5280,
  "height": 6240,
  "min_lat": 15.0000000000,
  "max_lat": 41.0000000000,
  "min_lon": 104.0000000000,
  "max_lon": 126.0000000000,
  "res_x": 0.0041666667,
  "res_y": 0.0041666667,
  "has_nodata": true,
  "nodata_value": -32767.000000
}
```

---

## 🎮 游戏引擎端 C++ SDK 集成 (`RawTerrainReader`)

`src/RawTerrainReader.h` 和 `src/RawTerrainReader.cpp` 是专门为游戏引擎与自研渲染器编写的轻量采样器：
- **纯标准 C++11 实现**，无 Qt、无 GDAL 依赖；
- 支持自动解析同名 `.txt` 或 `.json` 元数据；
- 支持纳秒级双线性插值采样与边界保护。

### 代码调用示例

```cpp
#include "RawTerrainReader.h"
#include <iostream>

int main() {
    RawTerrainReader reader;

    // 1. 加载导出的二进制高度场 (留空则自动寻找同名 .json / .txt)
    //    若导出时启用了 --flip-y，第三个参数 isFlippedY 传入 true
    if (!reader.Load("gebco.bin", "", false)) {
        std::cerr << "加载地形高度场数据失败！" << std::endl;
        return -1;
    }

    double queryLat = 31.2304; // 目标纬度
    double queryLon = 121.4737; // 目标经度
    float elevation = 0.0f;

    // 2. 核心查询接口: 查询指定经纬度的高程 (支持双线性插值)
    if (reader.GetElevation(queryLat, queryLon, elevation, /*useBilinear=*/true)) {
        std::cout << "坐标 (" << queryLat << ", " << queryLon 
                  << ") 的高度为: " << elevation << " 米" << std::endl;
    } else {
        std::cout << "该坐标超出数据范围或属于 NoData 缺省值！" << std::endl;
    }

    // 3. 便捷单行接口: 超出范围或无效值时自动返回默认回退值 (如 0.0f)
    float eleDirect = reader.GetElevationDirect(queryLat, queryLon, 0.0f, true);

    return 0;
}
```

---

## 🤝 参与贡献

欢迎对本项目的代码优化、跨平台支持及功能增强提出宝贵建议！请参考 [CONTRIBUTING.md](CONTRIBUTING.md) 了解详细的开发与 Pull Request 流程。

---

## 📄 开源许可证

本项目基于 [MIT License](LICENSE) 开源协议，欢迎自由商用、修改及学习。
