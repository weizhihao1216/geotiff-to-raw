# 贡献指南 (Contributing Guide)

感谢您对 **geotiff-to-raw** 项目的关注与支持！无论是提交缺陷报告、改进文档、优化算法，还是提交新功能，我们都非常欢迎您的参与。

---

## 1. 提交 Issue

在提交 Issue 之前，请先检索现有的 Issues，避免重复报告：
- **Bug 报告**：请尽可能详细描述复现步骤、运行环境（操作系统、Qt 版本、GDAL 版本）、输入文件的尺寸与投影类型，并附带截图或日志输出。
- **功能建议**：请说明建议该功能的背景、应用场景（例如对接哪种游戏引擎/渲染器）以及预期的工作方式。

---

## 2. 代码开发与 Pull Request 流程

1. **Fork 本仓库** 到您的个人 GitHub 账号。
2. **克隆仓库**：
   ```bash
   git clone https://github.com/<你的用户名>/geotiff-to-raw.git
   cd geotiff-to-raw
   ```
3. **创建特性分支**：
   ```bash
   git checkout -b feature/your-feature-name
   # 或
   git checkout -b fix/issue-description
   ```
4. **编码规范与要求**：
   - **语言标准**：保持兼容 **C++11** 标准，避免使用 C++14/17 专属语法（以保证各大游戏引擎工程集成的通用性与跨编译器兼容度）。
   - **编码格式**：所有源文件与头文件请统一使用 **UTF-8 (带/不带 BOM 均可)** 编码，换行符保持 LF 或 CRLF。
   - **SDK 独立性**：`src/RawTerrainReader.h` 与 `RawTerrainReader.cpp` 必须保持**纯 C++11 标准库实现，严禁引入 Qt 或 GDAL 等任何第三方依赖**，以确保游戏引擎端能够直接拿来即用。
   - **内存与资源安全**：GDAL 数据集与动态内存申请必须遵循 RAII 原则，严防内存或文件句柄泄漏。
   - **大文件与测试数据规范**：严禁直接向 Git 提交大体积源栅格（`.tif`）或二进制高度场（`.bin`）；若贡献了新的基准测试数据集，请联系维护者打包上传至 GitHub Releases 附件托管。
5. **本地编译与验证**：
   - 运行 `build.bat` 或通过 CMake 确认 Release 构建无警告、无错误。
   - 使用包含有效高程的数据集对 GUI 界面点选和 SDK 插值接口进行实测验证。
6. **提交 Commit 与发起 PR**：
   - Commit 信息请保持简明清晰（建议遵循 Conventional Commits 规范，例如 `feat: ...`, `fix: ...`, `docs: ...`）。
   - 推送至您的分支并向主仓库发起 Pull Request。

---

## 3. 开源许可证

提交给本项目的代码均默认在 [MIT License](LICENSE) 许可下发布。
