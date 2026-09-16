# Data 目录说明

本目录用于存放输入源 GeoTIFF 栅格地形高程数据（`.tif`）以及由本工具导出的二进制高度场（`.bin`）与元数据文件（`.txt` / `.json`）。

## 说明

1. **大文件排除**：
   - 由于原始 GeoTIFF (`.tif`) 和导出的二进制高度场 (`.bin`) 通常体积较大（数十 MB 至数百 MB，超出了 GitHub 单文件 100MB 限制），已在根目录 `.gitignore` 中默认排除。
2. **示例格式**：
   - 仓库内保留了导出的元数据格式示例：`*.txt` 与 `*.json`，可供查阅导出元数据字段结构。
3. **数据下载**：
   - 全球高程/水深栅格数据推荐从 [GEBCO (General Bathymetric Chart of the Oceans)](https://www.gebco.net/) 下载对应区域的 GeoTIFF 格式数据。
