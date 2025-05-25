#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>

namespace fs = std::filesystem;

// 国际化支持模块 - 提供中文界面
namespace i18n {
    // 所有用户可见的文本消息
    const std::map<std::string, std::string> messages = {
        // 通用消息
        {"welcome", "欢迎使用 hitpag 智能压缩/解压工具"},
        {"version", "hitpag 版本 1.0.0"},
        {"goodbye", "感谢使用 hitpag，再见！"},
        
        // 帮助信息
        {"usage", "用法: hitpag [选项] 源路径 目标路径"},
        {"help_options", "选项:"},
        {"help_i", "  -i         交互模式"},
        {"help_h", "  -h, --help 显示帮助信息"},
        {"help_v", "  -v         显示版本信息"},
        {"help_examples", "示例:"},
        {"help_example1", "  hitpag arch.tar.gz ./extracted_dir    # 解压 arch.tar.gz 到 extracted_dir"},
        {"help_example2", "  hitpag ./my_folder my_archive.zip     # 压缩 my_folder 到 my_archive.zip"},
        {"help_example3", "  hitpag -i big_file.rar .              # 交互式解压 big_file.rar 到当前目录"},
        
        // 错误消息
        {"error_missing_args", "错误: 缺少参数"},
        {"error_invalid_source", "错误: 源路径不存在"},
        {"error_invalid_target", "错误: 目标路径无效"},
        {"error_same_path", "错误: 源路径和目标路径不能相同"},
        {"error_unknown_format", "错误: 无法识别的文件格式"},
        {"error_tool_not_found", "错误: 未找到所需的工具: "},
        {"error_operation_failed", "错误: 操作失败: "},
        {"error_permission_denied", "错误: 权限被拒绝"},
        {"error_not_enough_space", "错误: 磁盘空间不足"},
        
        // 交互模式消息
        {"interactive_mode", "交互模式已启动"},
        {"ask_operation", "请选择操作类型:"},
        {"operation_compress", "1. 压缩"},
        {"operation_decompress", "2. 解压"},
        {"ask_format", "请选择压缩格式:"},
        {"format_tar", "1. tar (无压缩)"},
        {"format_tar_gz", "2. tar.gz (gzip压缩)"},
        {"format_tar_bz2", "3. tar.bz2 (bzip2压缩)"},
        {"format_tar_xz", "4. tar.xz (xz压缩)"},
        {"format_zip", "5. zip"},
        {"format_7z", "6. 7z"},
        {"format_rar", "7. rar"},
        {"ask_overwrite", "目标已存在，是否覆盖? (y/n): "},
        {"ask_delete_source", "操作完成后是否删除源文件? (y/n): "},
        {"invalid_choice", "无效的选择，请重试"},
        
        // 操作消息
        {"compressing", "正在压缩..."},
        {"decompressing", "正在解压..."},
        {"operation_complete", "操作完成"},
        {"operation_canceled", "操作已取消"},
        
        // 进度显示
        {"progress", "进度: "},
        {"remaining_time", "预计剩余时间: "},
        {"processing_file", "正在处理: "}
    };
    
    // 获取消息文本
    std::string get(const std::string& key) {
        auto it = messages.find(key);
        if (it != messages.end()) {
            return it->second;
        }
        return "[" + key + "]";
    }
}

// 错误处理模块
namespace error {
    enum class ErrorCode {
        SUCCESS,
        MISSING_ARGS,
        INVALID_SOURCE,
        INVALID_TARGET,
        SAME_PATH,
        UNKNOWN_FORMAT,
        TOOL_NOT_FOUND,
        OPERATION_FAILED,
        PERMISSION_DENIED,
        NOT_ENOUGH_SPACE
    };
    
    class HitpagException : public std::runtime_error {
    private:
        ErrorCode code_;
        
    public:
        HitpagException(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}
        
        ErrorCode code() const { return code_; }
    };
    
    void throw_error(ErrorCode code, const std::string& additional_info = "") {
        std::string message;
        
        switch (code) {
            case ErrorCode::MISSING_ARGS:
                message = i18n::get("error_missing_args");
                break;
            case ErrorCode::INVALID_SOURCE:
                message = i18n::get("error_invalid_source");
                break;
            case ErrorCode::INVALID_TARGET:
                message = i18n::get("error_invalid_target");
                break;
            case ErrorCode::SAME_PATH:
                message = i18n::get("error_same_path");
                break;
            case ErrorCode::UNKNOWN_FORMAT:
                message = i18n::get("error_unknown_format");
                break;
            case ErrorCode::TOOL_NOT_FOUND:
                message = i18n::get("error_tool_not_found") + additional_info;
                break;
            case ErrorCode::OPERATION_FAILED:
                message = i18n::get("error_operation_failed") + additional_info;
                break;
            case ErrorCode::PERMISSION_DENIED:
                message = i18n::get("error_permission_denied");
                break;
            case ErrorCode::NOT_ENOUGH_SPACE:
                message = i18n::get("error_not_enough_space");
                break;
            default:
                message = "未知错误";
        }
        
        throw HitpagException(code, message);
    }
}

// 参数解析模块
namespace args {
    struct Options {
        bool interactive_mode = false;
        bool show_help = false;
        bool show_version = false;
        std::string source_path;
        std::string target_path;
    };
    
    Options parse(int argc, char* argv[]) {
        Options options;
        
        // 如果没有参数，显示帮助
        if (argc < 2) {
            options.show_help = true;
            return options;
        }
        
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        
        // 解析选项
        size_t i = 0;
        while (i < args.size() && args[i][0] == '-') {
            if (args[i] == "-i") {
                options.interactive_mode = true;
            } else if (args[i] == "-h" || args[i] == "--help") {
                options.show_help = true;
                return options;
            } else if (args[i] == "-v" || args[i] == "--version") {
                options.show_version = true;
                return options;
            } else {
                error::throw_error(error::ErrorCode::MISSING_ARGS);
            }
            ++i;
        }
        
        // 解析路径参数
        if (i < args.size()) {
            options.source_path = args[i++];
        } else {
            error::throw_error(error::ErrorCode::MISSING_ARGS);
        }
        
        if (i < args.size()) {
            options.target_path = args[i++];
        } else if (!options.interactive_mode) {
            error::throw_error(error::ErrorCode::MISSING_ARGS);
        }
        
        return options;
    }
    
    void show_help() {
        std::cout << i18n::get("welcome") << std::endl;
        std::cout << std::endl;
        std::cout << i18n::get("usage") << std::endl;
        std::cout << std::endl;
        std::cout << i18n::get("help_options") << std::endl;
        std::cout << i18n::get("help_i") << std::endl;
        std::cout << i18n::get("help_h") << std::endl;
        std::cout << i18n::get("help_v") << std::endl;
        std::cout << std::endl;
        std::cout << i18n::get("help_examples") << std::endl;
        std::cout << i18n::get("help_example1") << std::endl;
        std::cout << i18n::get("help_example2") << std::endl;
        std::cout << i18n::get("help_example3") << std::endl;
    }
    
    void show_version() {
        std::cout << i18n::get("version") << std::endl;
    }
}

// 文件类型识别模块
namespace file_type {
    enum class FileType {
        REGULAR_FILE,
        DIRECTORY,
        ARCHIVE_TAR,
        ARCHIVE_TAR_GZ,
        ARCHIVE_TAR_BZ2,
        ARCHIVE_TAR_XZ,
        ARCHIVE_ZIP,
        ARCHIVE_RAR,
        ARCHIVE_7Z,
        UNKNOWN
    };
    
    enum class OperationType {
        COMPRESS,
        DECOMPRESS,
        UNKNOWN
    };
    
    struct RecognitionResult {
        FileType source_type = FileType::UNKNOWN;
        FileType target_type = FileType::UNKNOWN;
        OperationType operation = OperationType::UNKNOWN;
    };
    
    // 通过文件扩展名识别文件类型
    FileType recognize_by_extension(const std::string& path) {
        std::string ext = fs::path(path).extension().string();
        std::string stem_ext = fs::path(fs::path(path).stem()).extension().string();
        std::string full_ext = stem_ext + ext;
        
        // 转换为小写
        for (auto& c : ext) c = std::tolower(c);
        for (auto& c : stem_ext) c = std::tolower(c);
        for (auto& c : full_ext) c = std::tolower(c);
        
        if (ext == ".gz" && stem_ext == ".tar") return FileType::ARCHIVE_TAR_GZ;
        if (ext == ".tgz") return FileType::ARCHIVE_TAR_GZ;
        if (ext == ".bz2" && stem_ext == ".tar") return FileType::ARCHIVE_TAR_BZ2;
        if (ext == ".tbz2") return FileType::ARCHIVE_TAR_BZ2;
        if (ext == ".xz" && stem_ext == ".tar") return FileType::ARCHIVE_TAR_XZ;
        if (ext == ".txz") return FileType::ARCHIVE_TAR_XZ;
        if (ext == ".tar") return FileType::ARCHIVE_TAR;
        if (ext == ".zip") return FileType::ARCHIVE_ZIP;
        if (ext == ".rar") return FileType::ARCHIVE_RAR;
        if (ext == ".7z") return FileType::ARCHIVE_7Z;
        
        return FileType::UNKNOWN;
    }
    
    // 通过文件头信息识别文件类型
    FileType recognize_by_header(const std::string& path) {
        // 打开文件
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return FileType::UNKNOWN;
        }
        
        // 读取文件头
        std::array<char, 8> header;
        file.read(header.data(), header.size());
        
        // 检查文件头标识
        if (header[0] == 0x50 && header[1] == 0x4B) {
            return FileType::ARCHIVE_ZIP;  // ZIP: PK
        }
        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72) {
            return FileType::ARCHIVE_RAR;  // RAR: Rar!
        }
        if (header[0] == 0x37 && header[1] == 0x7A && header[2] == 0xBC && header[3] == 0xAF) {
            return FileType::ARCHIVE_7Z;   // 7Z: 7z¼¯
        }
        if (header[0] == 0x1F && header[1] == 0x8B) {
            return FileType::ARCHIVE_TAR_GZ;  // GZIP: 1F 8B
        }
        if (header[0] == 0x42 && header[1] == 0x5A && header[2] == 0x68) {
            return FileType::ARCHIVE_TAR_BZ2;  // BZIP2: BZh
        }
        if (header[0] == 0xFD && header[1] == 0x37 && header[2] == 0x7A && header[3] == 0x58) {
            return FileType::ARCHIVE_TAR_XZ;  // XZ: FD 37 7A 58
        }
        
        // 检查是否为 TAR 文件 (TAR 文件头在偏移 257 处有 "ustar")
        file.seekg(257);
        std::array<char, 5> tar_header;
        file.read(tar_header.data(), tar_header.size());
        if (std::string(tar_header.data(), 5) == "ustar") {
            return FileType::ARCHIVE_TAR;
        }
        
        return FileType::UNKNOWN;
    }
    
    // 识别文件类型和操作类型
    RecognitionResult recognize(const std::string& source_path, const std::string& target_path) {
        RecognitionResult result;
        
        // 检查源路径是否存在
        if (!fs::exists(source_path)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // 识别源路径类型
        if (fs::is_directory(source_path)) {
            result.source_type = FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_path)) {
            // 先通过扩展名识别
            result.source_type = recognize_by_extension(source_path);
            
            // 如果无法通过扩展名识别，尝试通过文件头识别
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = recognize_by_header(source_path);
            }
            
            // 调试输出
            std::string type_str;
            switch (result.source_type) {
                case FileType::REGULAR_FILE: type_str = "普通文件"; break;
                case FileType::DIRECTORY: type_str = "目录"; break;
                case FileType::ARCHIVE_TAR: type_str = "TAR 归档"; break;
                case FileType::ARCHIVE_TAR_GZ: type_str = "TAR.GZ 压缩归档"; break;
                case FileType::ARCHIVE_TAR_BZ2: type_str = "TAR.BZ2 压缩归档"; break;
                case FileType::ARCHIVE_TAR_XZ: type_str = "TAR.XZ 压缩归档"; break;
                case FileType::ARCHIVE_ZIP: type_str = "ZIP 压缩归档"; break;
                case FileType::ARCHIVE_RAR: type_str = "RAR 压缩归档"; break;
                case FileType::ARCHIVE_7Z: type_str = "7Z 压缩归档"; break;
                default: type_str = "未知类型"; break;
            }
            std::cout << "DEBUG: 源文件类型识别结果: " << type_str << std::endl;
            
            // 如果仍然无法识别，则视为普通文件
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // 识别目标路径类型
        if (target_path.empty()) {
            result.target_type = FileType::UNKNOWN;
        } else if (fs::exists(target_path)) {
            if (fs::is_directory(target_path)) {
                result.target_type = FileType::DIRECTORY;
            } else if (fs::is_regular_file(target_path)) {
                result.target_type = FileType::REGULAR_FILE;
            } else {
                error::throw_error(error::ErrorCode::INVALID_TARGET);
            }
        } else {
            // 目标路径不存在，通过扩展名判断
            result.target_type = recognize_by_extension(target_path);
            
            // 如果无法通过扩展名识别，则根据路径最后一个字符是否为斜杠判断是否为目录
            if (result.target_type == FileType::UNKNOWN) {
                if (target_path.back() == '/' || target_path.back() == '\\') {
                    result.target_type = FileType::DIRECTORY;
                } else {
                    // 如果源是压缩文件，且目标无法识别，则默认目标为目录（解压目标）
                    if (result.source_type == FileType::ARCHIVE_TAR ||
                        result.source_type == FileType::ARCHIVE_TAR_GZ ||
                        result.source_type == FileType::ARCHIVE_TAR_BZ2 ||
                        result.source_type == FileType::ARCHIVE_TAR_XZ ||
                        result.source_type == FileType::ARCHIVE_ZIP ||
                        result.source_type == FileType::ARCHIVE_RAR ||
                        result.source_type == FileType::ARCHIVE_7Z) {
                        result.target_type = FileType::DIRECTORY;
                    } else {
                        result.target_type = FileType::REGULAR_FILE;
                    }
                }
            }
        }
        
        // 调试输出
        std::string target_type_str;
        switch (result.target_type) {
            case FileType::REGULAR_FILE: target_type_str = "普通文件"; break;
            case FileType::DIRECTORY: target_type_str = "目录"; break;
            case FileType::ARCHIVE_TAR: target_type_str = "TAR 归档"; break;
            case FileType::ARCHIVE_TAR_GZ: target_type_str = "TAR.GZ 压缩归档"; break;
            case FileType::ARCHIVE_TAR_BZ2: target_type_str = "TAR.BZ2 压缩归档"; break;
            case FileType::ARCHIVE_TAR_XZ: target_type_str = "TAR.XZ 压缩归档"; break;
            case FileType::ARCHIVE_ZIP: target_type_str = "ZIP 压缩归档"; break;
            case FileType::ARCHIVE_RAR: target_type_str = "RAR 压缩归档"; break;
            case FileType::ARCHIVE_7Z: target_type_str = "7Z 压缩归档"; break;
            default: target_type_str = "未知类型"; break;
        }
        std::cout << "DEBUG: 目标路径类型识别结果: " << target_type_str << std::endl;
        
        // 确定操作类型
        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            // 源是目录或普通文件，目标是压缩文件，执行压缩操作
            if (result.target_type == FileType::ARCHIVE_TAR ||
                result.target_type == FileType::ARCHIVE_TAR_GZ ||
                result.target_type == FileType::ARCHIVE_TAR_BZ2 ||
                result.target_type == FileType::ARCHIVE_TAR_XZ ||
                result.target_type == FileType::ARCHIVE_ZIP ||
                result.target_type == FileType::ARCHIVE_RAR ||
                result.target_type == FileType::ARCHIVE_7Z) {
                result.operation = OperationType::COMPRESS;
            }
        } else if (result.source_type == FileType::ARCHIVE_TAR ||
                   result.source_type == FileType::ARCHIVE_TAR_GZ ||
                   result.source_type == FileType::ARCHIVE_TAR_BZ2 ||
                   result.source_type == FileType::ARCHIVE_TAR_XZ ||
                   result.source_type == FileType::ARCHIVE_ZIP ||
                   result.source_type == FileType::ARCHIVE_RAR ||
                   result.source_type == FileType::ARCHIVE_7Z) {
            // 源是压缩文件，目标是目录，执行解压操作
            if (result.target_type == FileType::DIRECTORY) {
                result.operation = OperationType::DECOMPRESS;
            }
        }
        
        // 如果无法确定操作类型，则抛出错误
        if (result.operation == OperationType::UNKNOWN) {
            error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }
        
        return result;
    }
    
    // 获取文件类型的字符串表示
    std::string get_file_type_string(FileType type) {
        switch (type) {
            case FileType::REGULAR_FILE: return "普通文件";
            case FileType::DIRECTORY: return "目录";
            case FileType::ARCHIVE_TAR: return "TAR 归档";
            case FileType::ARCHIVE_TAR_GZ: return "TAR.GZ 压缩归档";
            case FileType::ARCHIVE_TAR_BZ2: return "TAR.BZ2 压缩归档";
            case FileType::ARCHIVE_TAR_XZ: return "TAR.XZ 压缩归档";
            case FileType::ARCHIVE_ZIP: return "ZIP 压缩归档";
            case FileType::ARCHIVE_RAR: return "RAR 压缩归档";
            case FileType::ARCHIVE_7Z: return "7Z 压缩归档";
            default: return "未知类型";
        }
    }
    
    // 获取操作类型的字符串表示
    std::string get_operation_type_string(OperationType type) {
        switch (type) {
            case OperationType::COMPRESS: return "压缩";
            case OperationType::DECOMPRESS: return "解压";
            default: return "未知操作";
        }
    }
}

// 进度显示模块
namespace progress {
    class ProgressBar {
    private:
        int width_;
        int last_percent_;
        
    public:
        ProgressBar(int width = 50) : width_(width), last_percent_(-1) {}
        
        void update(int percent) {
            if (percent == last_percent_) {
                return;
            }
            
            last_percent_ = percent;
            
            int pos = width_ * percent / 100;
            
            std::cout << "\r" << i18n::get("progress") << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << percent << "%" << std::flush;
            
            if (percent == 100) {
                std::cout << std::endl;
            }
        }
        
        void set_processing_file(const std::string& filename) {
            std::cout << "\r" << i18n::get("processing_file") << filename << std::endl;
        }
    };
}

// 压缩/解压调度模块
namespace operation {
    // 检查工具是否可用
    bool is_tool_available(const std::string& tool) {
        std::string command = "which " + tool + " > /dev/null 2>&1";
        return system(command.c_str()) == 0;
    }
    
    // 执行命令并捕获输出
    int execute_command(const std::string& command, progress::ProgressBar& progress_bar) {
        std::array<char, 128> buffer;
        std::string result;
        
        // 创建管道
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, command);
        }
        
        // 读取输出
        int line_count = 0;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result = buffer.data();
            
            // 更新进度（这里只是一个简单的模拟，实际上需要根据不同工具的输出格式解析进度）
            line_count++;
            int percent = std::min(99, line_count % 100);
            progress_bar.update(percent);
            
            // 如果输出包含文件名，显示正在处理的文件
            if (result.find('/') != std::string::npos) {
                progress_bar.set_processing_file(result);
            }
        }
        
        // 完成
        progress_bar.update(100);
        
        return WEXITSTATUS(pclose(pipe.release()));
    }
    
    // 压缩操作
    void compress(const std::string& source_path, const std::string& target_path, file_type::FileType target_type) {
        std::string command;
        std::string tool;
        
        // 根据目标类型选择压缩工具和命令
        switch (target_type) {
            case file_type::FileType::ARCHIVE_TAR:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -cf \"" + target_path + "\" -C \"" + fs::path(source_path).parent_path().string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_GZ:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -czf \"" + target_path + "\" -C \"" + fs::path(source_path).parent_path().string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_BZ2:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -cjf \"" + target_path + "\" -C \"" + fs::path(source_path).parent_path().string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_XZ:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -cJf \"" + target_path + "\" -C \"" + fs::path(source_path).parent_path().string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "zip";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                if (fs::is_directory(source_path)) {
                    command = "cd \"" + fs::path(source_path).parent_path().string() + "\" && zip -r \"" + fs::absolute(target_path).string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                } else {
                    command = "cd \"" + fs::path(source_path).parent_path().string() + "\" && zip \"" + fs::absolute(target_path).string() + "\" \"" + fs::path(source_path).filename().string() + "\"";
                }
                break;
                
            case file_type::FileType::ARCHIVE_RAR:
                tool = "rar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "rar a \"" + target_path + "\" \"" + source_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "7z a \"" + target_path + "\" \"" + source_path + "\"";
                break;
                
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }
        
        // 显示压缩信息
        std::cout << i18n::get("compressing") << std::endl;
        
        // 创建进度条
        progress::ProgressBar progress_bar;
        
        // 执行命令
        int result = execute_command(command, progress_bar);
        
        // 检查结果
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, std::to_string(result));
        }
        
        std::cout << i18n::get("operation_complete") << std::endl;
    }
    
    // 解压操作
    void decompress(const std::string& source_path, const std::string& target_path, file_type::FileType source_type) {
        std::string command;
        std::string tool;
        
        // 创建目标目录（如果不存在）
        if (!fs::exists(target_path)) {
            fs::create_directories(target_path);
        }
        
        // 根据源类型选择解压工具和命令
        switch (source_type) {
            case file_type::FileType::ARCHIVE_TAR:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -xf \"" + source_path + "\" -C \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_GZ:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -xzf \"" + source_path + "\" -C \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_BZ2:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -xjf \"" + source_path + "\" -C \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_TAR_XZ:
                tool = "tar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "tar -xJf \"" + source_path + "\" -C \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "unzip";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "unzip \"" + source_path + "\" -d \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_RAR:
                tool = "unrar";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "unrar x \"" + source_path + "\" \"" + target_path + "\"";
                break;
                
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) {
                    error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, tool);
                }
                command = "7z x \"" + source_path + "\" -o\"" + target_path + "\"";
                break;
                
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }
        
        // 显示解压信息
        std::cout << i18n::get("decompressing") << std::endl;
        
        // 创建进度条
        progress::ProgressBar progress_bar;
        
        // 执行命令
        int result = execute_command(command, progress_bar);
        
        // 检查结果
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, std::to_string(result));
        }
        
        std::cout << i18n::get("operation_complete") << std::endl;
    }
}

// 交互模式模块
namespace interactive {
    // 获取用户输入
    std::string get_input() {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
    
    // 获取用户选择（数字）
    int get_choice(int min, int max) {
        while (true) {
            std::string input = get_input();
            try {
                int choice = std::stoi(input);
                if (choice >= min && choice <= max) {
                    return choice;
                }
            } catch (...) {
                // 忽略转换错误
            }
            std::cout << i18n::get("invalid_choice") << std::endl;
        }
    }
    
    // 获取用户确认（y/n）
    bool get_confirmation() {
        while (true) {
            std::string input = get_input();
            if (input == "y" || input == "Y") {
                return true;
            } else if (input == "n" || input == "N") {
                return false;
            }
            std::cout << i18n::get("invalid_choice") << std::endl;
        }
    }
    
    // 交互式操作
    void run(const std::string& source_path) {
        std::cout << i18n::get("interactive_mode") << std::endl;
        
        // 检查源路径是否存在
        if (!fs::exists(source_path)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // 确定源文件类型
        file_type::FileType source_type;
        if (fs::is_directory(source_path)) {
            source_type = file_type::FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_path)) {
            // 先通过扩展名识别
            source_type = file_type::recognize_by_extension(source_path);
            
            // 如果无法通过扩展名识别，尝试通过文件头识别
            if (source_type == file_type::FileType::UNKNOWN) {
                source_type = file_type::recognize_by_header(source_path);
            }
            
            // 如果仍然无法识别，则视为普通文件
            if (source_type == file_type::FileType::UNKNOWN) {
                source_type = file_type::FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // 显示源文件类型
        std::cout << "源文件类型: " << file_type::get_file_type_string(source_type) << std::endl;
        
        // 确定操作类型
        file_type::OperationType operation_type;
        if (source_type == file_type::FileType::DIRECTORY || source_type == file_type::FileType::REGULAR_FILE) {
            // 源是目录或普通文件，默认为压缩操作
            operation_type = file_type::OperationType::COMPRESS;
        } else {
            // 源是压缩文件，默认为解压操作
            operation_type = file_type::OperationType::DECOMPRESS;
        }
        
        // 询问操作类型
        std::cout << i18n::get("ask_operation") << std::endl;
        std::cout << i18n::get("operation_compress") << std::endl;
        std::cout << i18n::get("operation_decompress") << std::endl;
        int choice = get_choice(1, 2);
        operation_type = (choice == 1) ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;
        
        // 如果是压缩操作，询问压缩格式
        file_type::FileType target_type = file_type::FileType::UNKNOWN;
        if (operation_type == file_type::OperationType::COMPRESS) {
            std::cout << i18n::get("ask_format") << std::endl;
            std::cout << i18n::get("format_tar") << std::endl;
            std::cout << i18n::get("format_tar_gz") << std::endl;
            std::cout << i18n::get("format_tar_bz2") << std::endl;
            std::cout << i18n::get("format_tar_xz") << std::endl;
            std::cout << i18n::get("format_zip") << std::endl;
            std::cout << i18n::get("format_7z") << std::endl;
            std::cout << i18n::get("format_rar") << std::endl;
            
            choice = get_choice(1, 7);
            switch (choice) {
                case 1: target_type = file_type::FileType::ARCHIVE_TAR; break;
                case 2: target_type = file_type::FileType::ARCHIVE_TAR_GZ; break;
                case 3: target_type = file_type::FileType::ARCHIVE_TAR_BZ2; break;
                case 4: target_type = file_type::FileType::ARCHIVE_TAR_XZ; break;
                case 5: target_type = file_type::FileType::ARCHIVE_ZIP; break;
                case 6: target_type = file_type::FileType::ARCHIVE_7Z; break;
                case 7: target_type = file_type::FileType::ARCHIVE_RAR; break;
            }
        }
        
        // 询问目标路径
        std::string target_path;
        std::cout << "请输入目标路径: ";
        target_path = get_input();
        
        // 如果目标路径为空，使用默认路径
        if (target_path.empty()) {
            if (operation_type == file_type::OperationType::COMPRESS) {
                // 默认压缩到当前目录，使用源文件名加上相应的扩展名
                std::string ext;
                switch (target_type) {
                    case file_type::FileType::ARCHIVE_TAR: ext = ".tar"; break;
                    case file_type::FileType::ARCHIVE_TAR_GZ: ext = ".tar.gz"; break;
                    case file_type::FileType::ARCHIVE_TAR_BZ2: ext = ".tar.bz2"; break;
                    case file_type::FileType::ARCHIVE_TAR_XZ: ext = ".tar.xz"; break;
                    case file_type::FileType::ARCHIVE_ZIP: ext = ".zip"; break;
                    case file_type::FileType::ARCHIVE_7Z: ext = ".7z"; break;
                    case file_type::FileType::ARCHIVE_RAR: ext = ".rar"; break;
                    default: ext = ".unknown";
                }
                target_path = fs::path(source_path).filename().string() + ext;
            } else {
                // 默认解压到当前目录
                target_path = ".";
            }
        }
        
        // 检查目标路径是否存在
        if (fs::exists(target_path)) {
            std::cout << i18n::get("ask_overwrite");
            if (!get_confirmation()) {
                std::cout << i18n::get("operation_canceled") << std::endl;
                return;
            }
        }
        
        // 询问是否删除源文件
        bool delete_source = false;
        std::cout << i18n::get("ask_delete_source");
        delete_source = get_confirmation();
        
        // 执行操作
        try {
            if (operation_type == file_type::OperationType::COMPRESS) {
                operation::compress(source_path, target_path, target_type);
            } else {
                operation::decompress(source_path, target_path, source_type);
            }
            
            // 如果需要删除源文件
            if (delete_source) {
                if (fs::is_directory(source_path)) {
                    fs::remove_all(source_path);
                } else {
                    fs::remove(source_path);
                }
            }
        } catch (const error::HitpagException& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

// 主程序
int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数
        args::Options options = args::parse(argc, argv);
        
        // 显示帮助信息
        if (options.show_help) {
            args::show_help();
            return 0;
        }
        
        // 显示版本信息
        if (options.show_version) {
            args::show_version();
            return 0;
        }
        
        // 交互模式
        if (options.interactive_mode) {
            interactive::run(options.source_path);
            return 0;
        }
        
        // 自动模式
        // 识别文件类型和操作类型
        file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);
        
        // 执行操作
        if (result.operation == file_type::OperationType::COMPRESS) {
            operation::compress(options.source_path, options.target_path, result.target_type);
        } else if (result.operation == file_type::OperationType::DECOMPRESS) {
            operation::decompress(options.source_path, options.target_path, result.source_type);
        }
        
    } catch (const error::HitpagException& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "未知错误" << std::endl;
        return 1;
    }
    
    return 0;
}
