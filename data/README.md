# Data 数据目录与发布说明

本目录用于存放输入源 GeoTIFF 栅格地形高程数据（`.tif` / `.tiff`）以及由本工具导出的 32 位浮点原始二进制高度场（`.bin`）与空间元数据文件（`.txt` / `.json`）。

---

## 📦 大数据文件发布策略 (Release 资产说明)

1. **为什么不直接提交到 Git 仓库？**
   - 原始 GeoTIFF (`.tif`) 与导出的高度场 (`.bin`) 通常体积较大（数十 MB 至数 GB，极易超出 GitHub 单文件 100MB 限制），直接提交会导致 Git 仓库历史体积永久膨胀并严重影响 `git clone` 速度。
   - 因此，根据工程根目录的 [`.gitignore`](../.gitignore)，所有 `data/*.tif` 与 `data/*.bin` 文件默认被 Git 排除。

2. **如何获取开箱即用的测试数据？**
   - **方式 A（下载独立数据包）**：在 GitHub Releases 页面下载 `geotiff-to-raw_v1.0.0_sample_data.zip`，解压到本 `data/` 目录；
   - **方式 B（下载全合一程序包）**：直接在 Releases 页面下载 `geotiff-to-raw_v1.0.0_windows_x64_full.zip`，该包已完整内置本目录的 `.bin` 与 `.tif` 数据，解压后双击即可直接体验。
   - **数据包内容**：包含 GEBCO 示例高程源图 (`.tif`)、转换导出的 32 位浮点原始二进制高度场 (`.bin`) 及空间描述元数据 (`.txt` / `.json`)。

3. **仓库内保留的示例元数据**：
   - 仓库中直接保留了轻量级的文本格式示例元数据（`*.txt` 与 `*.json`），供开发者在不下载庞大数据集的前提下，随时查阅导出的空间范围、分辨率和 NoData 等规范。

4. **生产环境全量数据源**：
   - 全球高程/水深栅格数据推荐前往 [GEBCO (General Bathymetric Chart of the Oceans)](https://www.gebco.net/) 官网免费下载目标区域的 GeoTIFF 格式高精度数据。
