# hitpag 2.0.2 - 智能压缩工具

**智能、强大、易用的新一代命令行压缩工具**

[![GitHub](https://img.shields.io/badge/GitHub-Hitmux/hitpag-blue)](https://github.com/Hitmux/hitpag)
[![Website](https://img.shields.io/badge/Website-hitmux.top-green)](https://hitmux.top)

[English](README.md) | [简体中文](README_zh.md)

---

## 🚀 核心功能

### 🧠 智能文件类型识别
- **Magic Number 检测**：通过分析文件头自动识别真实格式，不受扩展名影响
- **处理问题文件**：支持无扩展名或扩展名错误的文件
- **格式覆盖**：当自动检测失败时，可使用 `--format` 选项手动指定格式

```bash
# 即使没有文件扩展名也能自动检测
hitpag mystery_file ./output/

# 处理扩展名错误的文件（实际上是 7z，但命名为 .zip）
hitpag fake.zip ./extracted/      # 自动检测为 7z 格式

# 根据后缀名识别压缩格式
hitpag example backup.tar # example这个文件夹会在压缩包里
hitpag example/ backup2.rat # example这个文件夹不在压缩包里

# 强制指定格式
hitpag --format=rar problem_archive ./output/
```

### ⚡ 高性能压缩
- **多线程支持**：自动检测 CPU 核心数以实现并行压缩加速
- **压缩级别控制**：通过 1-9 级精细调整压缩率与速度的平衡
- **现代算法支持**：支持 LZ4 超快压缩和 Zstandard 高效压缩

```bash
# 多线程高性能压缩
hitpag -l9 -t8 --benchmark data.tar.gz ./large_files/

# 超快压缩（适用于临时文件）
hitpag --format=lz4 temp.lz4 ./temp_data/

# 高效的现代压缩
hitpag --format=zstd archive.zstd ./documents/
```

### 🎯 精准文件过滤
- **正则表达式支持**：强大的包含/排除模式匹配
- **组合过滤**：灵活的多条件组合，实现对内容的精准控制

```bash
# 仅压缩源代码文件
hitpag --include='*.cpp' --include='*.h' --include='*.py' code.7z ./project/

# 排除临时文件和构建文件
hitpag --exclude='*.tmp' --exclude='build/*' --exclude='node_modules/*' clean.tar.gz ./project/
```

### 📊 性能监控与验证
- **基准测试**：提供压缩率、耗时、线程利用率等详细统计
- **完整性验证**：压缩后自动验证归档文件的完整性
- **详细输出**：实时显示进度和操作信息

```bash
# 性能分析
hitpag --benchmark --verbose optimized.tar.xz ./data/

# 关键数据压缩并进行验证
hitpag --verify --benchmark important.7z ./critical_files/
```

### 🔐 安全加密保护
- **密码保护**：支持为 ZIP 和 7Z 格式设置密码加密
- **交互式输入**：安全的密码输入方式，无回显

```bash
# 使用密码保护进行压缩
hitpag -pmysecret secure.7z ./sensitive_data/

# 交互式密码输入
hitpag -p confidential.zip ./private_files/
```

### 🎨 用户友好的界面
- **交互模式**：为命令行新手提供向导式操作
- **智能检测**：自动检测操作类型（压缩/解压）
- **全面的帮助文档**：提供丰富的示例和文档说明

```bash
# 启动交互模式
hitpag -i

# 获取帮助
hitpag --help
```

---

## 📦 支持的格式

### 传统格式
- **TAR 系列**：tar, tar.gz, tar.bz2, tar.xz
- **ZIP**：完全支持，包括密码保护
- **7Z**：最高的压缩率，支持密码保护
- **RAR**：支持解压

### 现代格式
- **LZ4**：超快的压缩/解压速度，适用于实时场景
- **Zstandard (zstd)**：由 Facebook 开发，在压缩率和速度之间取得最佳平衡
- **XAR**：macOS 原生格式

---

## 🛠️ 快速入门

### 安装依赖
```bash
# Ubuntu/Debian
sudo apt install -y tar gzip bzip2 xz-utils zip unzip p7zip-full lz4 zstd

# 可选工具
sudo apt install -y rar unrar xar
```

### 构建与安装
```bash
git clone https://github.com/Hitmux/hitpag.git
cd hitpag
mkdir build && cd build
cmake ..
make
sudo make install  # 可选
```

### 基本用法
```bash
# 自动检测操作
hitpag archive.tar.gz ./extracted/    # 解压
hitpag ./folder/ backup.zip            # 压缩

# 高级功能
hitpag -l9 -t4 --benchmark big_data.tar.xz ./data/
hitpag --format=7z --verbose mystery_file ./output/
```

---

## 💡 应用场景

### 🏢 企业备份
```bash
# 高效备份，排除临时文件
hitpag --benchmark --exclude='*.tmp' --exclude='*.log' \
       -l9 -t8 enterprise_backup.tar.xz ./company_data/
```

### 👨‍💻 开发项目压缩
```bash
# 只打包源代码
hitpag --include='*.cpp' --include='*.h' --include='*.py' \
       --exclude='build/*' source_code.7z ./project/
```

### 🔒 安全文件传输
```bash
# 加密压缩敏感文件
hitpag -p --verify --benchmark secure_transfer.7z ./confidential/
```

### ⚡ 快速临时压缩
```bash
# 超快压缩临时文件
hitpag --format=lz4 temp_backup.lz4 ./temp_work/```

### 🎯 问题文件处理
```bash
# 处理扩展名错误或无扩展名的文件
hitpag downloaded_archive ./extracted/           # 自动检测格式
hitpag --format=rar unknown_format ./output/     # 强制指定格式
```

---

## 📝 命令参考

### 基本选项
- **`-i`** - 交互模式
- **`-p[password]`** - 密码保护
- **`-l[1-9]`** - 压缩级别
- **`-t[count]`** - 线程数
- **`--format=type`** - 强制指定格式

### 高级选项
- **`--verbose`** - 详细输出
- **`--benchmark`** - 性能统计
- **`--verify`** - 完整性验证
- **`--exclude=pattern`** - 排除文件
- **`--include=pattern`** - 包含文件

### 命令示例
```bash
# 智能识别
hitpag file.7z ./output/

# 高性能压缩
hitpag -l9 -t8 --benchmark archive.tar.xz ./data/

# 精准过滤
hitpag --include='*.cpp' --exclude='build/*' code.7z ./project/

# 安全加密
hitpag -p --verify secure.7z ./sensitive/

# 指定格式
hitpag --format=zip --verbose unknown_file ./extracted/
```

---

## 🔍 问题排查

### 常见问题
1.  **识别失败**：使用 `--format` 手动指定格式
2.  **权限错误**：检查文件/目录的权限
3.  **缺少工具**：安装相应的压缩工具
4.  **内存不足**：降低线程数或压缩级别

### 错误代码
- `Error: Invalid format specified` - 格式参数错误
- `Error: Required tool not found` - 缺少压缩工具
- `Error: Source path does not exist` - 源文件不存在

---

## 📈 版本历史

### 2.0.2 - 最新版本
**错误修复与改进**
- 增强了文件头检测，改进了边界检查
- 提升了内存安全性和错误处理能力
- 改进了密码输入时终端设置的恢复机制
- 更健壮的参数验证
- 修复了文件类型识别中的边界情况

### 2.0.0 - 重大更新
**新功能**
- 智能文件头检测系统
- 手动格式覆盖（--format 选项）
- 多线程并行处理
- 现代压缩格式支持（LZ4, Zstandard）
- 文件过滤系统
- 性能基准测试

**改进**
- 增强了错误处理
- 更好的用户界面
- 内存安全性增强
- 跨平台兼容性

**修复**
- 文件头检测的边界检查
- 终端设置恢复
- 参数验证改进

---

## 🤝 贡献

欢迎贡献代码、报告错误或提出功能建议！

- 📝 [提交问题 (Submit Issue)](https://github.com/Hitmux/hitpag/issues)
- 🔧 [提交拉取请求 (Submit PR)](https://github.com/Hitmux/hitpag/pulls)
- 💬 [参与讨论 (Discussions)](https://github.com/Hitmux/hitpag/discussions)

---

## 📄 许可证

本项目基于 [GNU Affero General Public License v3.0](LICENSE) 许可证。

---

**开发者**: [Hitmux](https://hitmux.top)
