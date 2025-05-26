#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib> // For system, WEXITSTATUS
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <algorithm> // For std::transform, std::min
#include <cctype>    // For std::tolower, isalpha

#ifdef _WIN32
#include <process.h> // For _pclose, _popen on Windows
#define POPEN _popen
#define PCLOSE _pclose
#else
#include <cstdio> // For popen, pclose
#define POPEN popen
#define PCLOSE pclose
#endif


namespace fs = std::filesystem;

// 国际化支持模块 - 提供中文界面
namespace i18n {
    // 所有用户可见的文本消息
    const std::map<std::string, std::string> messages = {
        // 一般消息
        {"welcome", "欢迎使用 hitpag 智能压缩/解压缩工具"},
        {"version", "hitpag 版本 1.0.0"},
        {"goodbye", "感谢使用 hitpag，再见！"},

        // 帮助消息
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
        {"error_missing_args", "错误: 缺少参数。{ADDITIONAL_INFO}"},
        {"error_invalid_source", "错误: 源路径 '{PATH}' 不存在或无效。{REASON}"},
        {"error_invalid_target", "错误: 目标路径 '{PATH}' 无效。{REASON}"},
        {"error_same_path", "错误: 源路径和目标路径不能相同"},
        {"error_unknown_format", "错误: 无法识别的文件格式或操作不明确。{INFO}"},
        {"error_tool_not_found", "错误: 找不到所需工具: {TOOL_NAME}。请确保它已安装并位于系统 PATH 中。"},
        {"error_operation_failed", "错误: 操作失败 (命令: {COMMAND}, 退出码: {EXIT_CODE})。"},
        {"error_permission_denied", "错误: 权限被拒绝。{PATH}"},
        {"error_not_enough_space", "错误: 磁盘空间不足"},

        // 交互模式消息
        {"interactive_mode", "交互模式已启动"},
        {"ask_operation", "请选择操作类型:"},
        {"operation_compress", "1. 压缩"},
        {"operation_decompress", "2. 解压缩"},
        {"ask_format", "请选择压缩格式:"},
        {"format_tar", "1. tar (无压缩)"},
        {"format_tar_gz", "2. tar.gz (gzip 压缩)"},
        {"format_tar_bz2", "3. tar.bz2 (bzip2 压缩)"},
        {"format_tar_xz", "4. tar.xz (xz 压缩)"},
        {"format_zip", "5. zip"},
        {"format_7z", "6. 7z"},
        {"format_rar", "7. rar (仅建议在交互模式下解压)"}, // 已调整
        {"ask_overwrite", "目标 '{TARGET_PATH}' 已存在，是否覆盖？(y/n): "},
        {"ask_delete_source", "操作完成后是否删除源 '{SOURCE_PATH}'？(y/n): "},
        {"invalid_choice", "无效的选择，请重试"},

        // 操作消息
        {"compressing", "正在压缩..."},
        {"decompressing", "正在解压缩..."},
        {"operation_complete", "操作完成"},
        {"operation_canceled", "操作已取消"},

        // 进度显示
        {"progress", "进度: "},
        {"remaining_time", "预计剩余时间: "}, // 占位符，未实现
        {"processing_file", "正在处理: "}
    };

    // 获取消息文本
    std::string get(const std::string& key, const std::map<std::string, std::string>& placeholders = {}) {
        auto it = messages.find(key);
        std::string message_template;
        if (it != messages.end()) {
            message_template = it->second;
        } else {
            return "[" + key + "]"; // 如果未找到则返回键本身
        }

        for(const auto& p : placeholders) {
            std::string placeholder_key = "{" + p.first + "}";
            size_t pos = message_template.find(placeholder_key);
            while(pos != std::string::npos) {
                message_template.replace(pos, placeholder_key.length(), p.second);
                pos = message_template.find(placeholder_key, pos + p.second.length());
            }
        }
        // 移除未使用的占位符
        size_t start_ph = message_template.find("{");
        while(start_ph != std::string::npos) {
            size_t end_ph = message_template.find("}", start_ph);
            if (end_ph != std::string::npos) {
                message_template.replace(start_ph, end_ph - start_ph + 1, "");
            } else {
                break; // 没有闭合的花括号
            }
            start_ph = message_template.find("{");
        }
        return message_template;
    }
}

// 错误处理模块
namespace error {
    enum class ErrorCode {
        SUCCESS = 0,
        MISSING_ARGS = 1,
        INVALID_SOURCE = 2,
        INVALID_TARGET = 3,
        SAME_PATH = 4,
        UNKNOWN_FORMAT = 5,
        TOOL_NOT_FOUND = 6,
        OPERATION_FAILED = 7,
        PERMISSION_DENIED = 8,
        NOT_ENOUGH_SPACE = 9,
        UNKNOWN_ERROR = 99
    };

    class HitpagException : public std::runtime_error {
    private:
        ErrorCode code_;

    public:
        HitpagException(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}

        ErrorCode code() const { return code_; }
    };

    void throw_error(ErrorCode code, const std::map<std::string, std::string>& placeholders = {}) {
        std::string message_key;

        switch (code) {
            case ErrorCode::MISSING_ARGS: message_key = "error_missing_args"; break;
            case ErrorCode::INVALID_SOURCE: message_key = "error_invalid_source"; break;
            case ErrorCode::INVALID_TARGET: message_key = "error_invalid_target"; break;
            case ErrorCode::SAME_PATH: message_key = "error_same_path"; break;
            case ErrorCode::UNKNOWN_FORMAT: message_key = "error_unknown_format"; break;
            case ErrorCode::TOOL_NOT_FOUND: message_key = "error_tool_not_found"; break;
            case ErrorCode::OPERATION_FAILED: message_key = "error_operation_failed"; break;
            case ErrorCode::PERMISSION_DENIED: message_key = "error_permission_denied"; break;
            case ErrorCode::NOT_ENOUGH_SPACE: message_key = "error_not_enough_space"; break;
            default: message_key = "Unknown error"; code = ErrorCode::UNKNOWN_ERROR;
        }

        throw HitpagException(code, i18n::get(message_key, placeholders));
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

        if (argc < 2) {
            options.show_help = true;
            return options;
        }

        std::vector<std::string> args_vec;
        for (int i = 1; i < argc; ++i) {
            args_vec.push_back(argv[i]);
        }

        size_t current_arg_idx = 0;
        while (current_arg_idx < args_vec.size() && !args_vec[current_arg_idx].empty() && args_vec[current_arg_idx][0] == '-') {
            const std::string& opt = args_vec[current_arg_idx];
            if (opt == "-i") {
                options.interactive_mode = true;
            } else if (opt == "-h" || opt == "--help") {
                options.show_help = true;
                return options;
            } else if (opt == "-v" || opt == "--version") {
                options.show_version = true;
                return options;
            } else {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "未知选项: " + opt}});
            }
            current_arg_idx++;
        }

        if (current_arg_idx < args_vec.size()) {
            options.source_path = args_vec[current_arg_idx++];
        } else {
            if (!options.interactive_mode && !options.show_help && !options.show_version) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "缺少源路径"}});
            }
        }

        if (current_arg_idx < args_vec.size()) {
            options.target_path = args_vec[current_arg_idx++];
        } else {
            if (!options.interactive_mode && !options.show_help && !options.show_version && !options.source_path.empty()) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "缺少目标路径"}});
            }
        }

        if (current_arg_idx < args_vec.size()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "参数过多"}});
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
        REGULAR_FILE,    // 普通文件
        DIRECTORY,       // 目录
        ARCHIVE_TAR,     // TAR 归档
        ARCHIVE_TAR_GZ,  // TAR.GZ 归档
        ARCHIVE_TAR_BZ2, // TAR.BZ2 归档
        ARCHIVE_TAR_XZ,  // TAR.XZ 归档
        ARCHIVE_ZIP,     // ZIP 归档
        ARCHIVE_RAR,     // RAR 归档
        ARCHIVE_7Z,      // 7Z 归档
        UNKNOWN          // 未知
    };

    enum class OperationType {
        COMPRESS,   // 压缩
        DECOMPRESS, // 解压缩
        UNKNOWN     // 未知
    };

    struct RecognitionResult {
        FileType source_type = FileType::UNKNOWN;
        FileType target_type_hint = FileType::UNKNOWN;
        OperationType operation = OperationType::UNKNOWN;
    };

    FileType recognize_by_extension(const std::string& path_str) {
        fs::path p(path_str);
        if (!p.has_extension()) return FileType::UNKNOWN;

        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });


        if (ext == ".tar") return FileType::ARCHIVE_TAR;
        if (ext == ".zip") return FileType::ARCHIVE_ZIP;
        if (ext == ".rar") return FileType::ARCHIVE_RAR;
        if (ext == ".7z") return FileType::ARCHIVE_7Z;

        if (p.has_stem() && fs::path(p.stem()).has_extension()) {
            std::string stem_ext_str = fs::path(p.stem()).extension().string();
             std::transform(stem_ext_str.begin(), stem_ext_str.end(), stem_ext_str.begin(), [](unsigned char c){ return std::tolower(c); });

            if (stem_ext_str == ".tar") {
                if (ext == ".gz") return FileType::ARCHIVE_TAR_GZ;
                if (ext == ".bz2") return FileType::ARCHIVE_TAR_BZ2;
                if (ext == ".xz") return FileType::ARCHIVE_TAR_XZ;
            }
        }
        if (ext == ".tgz") return FileType::ARCHIVE_TAR_GZ;
        if (ext == ".tbz2" || ext == ".tbz") return FileType::ARCHIVE_TAR_BZ2;
        if (ext == ".txz") return FileType::ARCHIVE_TAR_XZ;

        return FileType::UNKNOWN;
    }

    FileType recognize_by_header(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return FileType::UNKNOWN;

        std::array<char, 8> header{};
        file.read(header.data(), header.size());
        if(file.gcount() < 4) return FileType::UNKNOWN;

        // 检查文件头魔数
        if (header[0] == 0x50 && header[1] == 0x4B) return FileType::ARCHIVE_ZIP;       // PK
        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21) return FileType::ARCHIVE_RAR; // Rar!
        if (header[0] == 0x37 && header[1] == 0x7A && header[2] == (char)0xBC && header[3] == (char)0xAF) return FileType::ARCHIVE_7Z; // 7z\xBC\xAF
        if (header[0] == (char)0x1F && header[1] == (char)0x8B) return FileType::ARCHIVE_TAR_GZ; // GZIP magic number
        if (header[0] == 0x42 && header[1] == 0x5A && header[2] == 0x68) return FileType::ARCHIVE_TAR_BZ2; // BZ2 magic number
        if (header[0] == (char)0xFD && header[1] == 0x37 && header[2] == 0x7A && header[3] == 0x58 && file.gcount() >= 6 && header[4] == 0x5A && header[5] == 0x00) return FileType::ARCHIVE_TAR_XZ; // XZ magic number

        file.clear();
        file.seekg(257); // TAR header at offset 257 for "ustar"
        std::array<char, 6> tar_header{};
        file.read(tar_header.data(), tar_header.size());
        if (file.gcount() >= 5 && std::string(tar_header.data(), 5) == "ustar") {
            return FileType::ARCHIVE_TAR;
        }

        return FileType::UNKNOWN;
    }

    RecognitionResult recognize(const std::string& source_path_str, const std::string& target_path_str) {
        RecognitionResult result;
        fs::path source_p(source_path_str);

        if (!fs::exists(source_p)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}});
        }

        if (fs::is_directory(source_p)) {
            result.source_type = FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_p)) {
            result.source_type = recognize_by_extension(source_path_str);
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = recognize_by_header(source_path_str);
            }
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}, {"REASON", "不是一个普通文件或目录"}});
        }

        bool target_is_archive_extension = false;
        if (!target_path_str.empty()) {
            result.target_type_hint = recognize_by_extension(target_path_str);
            target_is_archive_extension = (result.target_type_hint != FileType::UNKNOWN &&
                                           result.target_type_hint != FileType::REGULAR_FILE &&
                                           result.target_type_hint != FileType::DIRECTORY);
        }

        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            // 源是文件或目录，因此是压缩操作
            if (target_is_archive_extension) {
                result.operation = OperationType::COMPRESS;
            } else if (!target_path_str.empty() && fs::exists(target_path_str) && fs::is_directory(target_path_str)) {
                 error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "未指定归档名称，无法压缩到现有目录。压缩目标必须是归档文件名。"}});
            } else if (!target_path_str.empty()) {
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "压缩目标必须具有可识别的归档扩展名。"}});
            } else {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "压缩操作需要目标路径。"}});
            }
        } else {
            // 源是归档文件，因此是解压缩操作
            result.operation = OperationType::DECOMPRESS;
            if (target_path_str.empty()) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "解压缩操作需要目标目录。"}});
            }
            if (fs::exists(target_path_str) && !fs::is_directory(target_path_str)) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_path_str}, {"REASON", "解压缩目标必须是一个目录。"}});
            }
        }

        if (result.operation == OperationType::UNKNOWN) {
            error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }

        return result;
    }

    std::string get_file_type_string(FileType type) {
        switch (type) {
            case FileType::REGULAR_FILE: return "普通文件";
            case FileType::DIRECTORY: return "目录";
            case FileType::ARCHIVE_TAR: return "TAR 归档";
            case FileType::ARCHIVE_TAR_GZ: return "TAR.GZ 归档";
            case FileType::ARCHIVE_TAR_BZ2: return "TAR.BZ2 归档";
            case FileType::ARCHIVE_TAR_XZ: return "TAR.XZ 归档";
            case FileType::ARCHIVE_ZIP: return "ZIP 归档";
            case FileType::ARCHIVE_RAR: return "RAR 归档";
            case FileType::ARCHIVE_7Z: return "7Z 归档";
            default: return "未知类型";
        }
    }
    std::string get_operation_type_string(OperationType type) {
        switch (type) {
            case OperationType::COMPRESS: return "压缩";
            case OperationType::DECOMPRESS: return "解压缩";
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
            if (percent == last_percent_ && percent != 0 && percent != 100) {
                return;
            }

            last_percent_ = percent;
            percent = std::max(0, std::min(100, percent));

            int pos = width_ * percent / 100;

            std::cout << "\r" << i18n::get("progress") << "[";
            for (int i = 0; i < width_; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos && percent != 100) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << percent << "%" << std::flush;

            if (percent == 100) {
                std::cout << std::endl;
            }
        }

        void set_processing_file(const std::string& filename) {
            // 清除当前行并打印处理中的文件
            std::cout << "\r" << std::string(width_ + 10 + i18n::get("progress").length() + 5, ' ') << "\r";
            std::cout << i18n::get("processing_file") << filename << std::endl;
            // 重新显示进度条（如果之前有进度）
            if (last_percent_ != -1 && last_percent_ < 100) {
                update(last_percent_);
            }
        }
    };
}

// 压缩/解压缩调度模块
namespace operation {
    bool is_tool_available(const std::string& tool) {
        #ifdef _WIN32
            std::string command = "where " + tool + " > nul 2>&1";
        #else
            std::string command = "which " + tool + " > /dev/null 2>&1";
        #endif
        return system(command.c_str()) == 0;
    }

    int execute_command(const std::string& command_str, progress::ProgressBar& progress_bar) {
        std::array<char, 256> buffer;
        std::string current_line;

        progress_bar.update(0);

        FILE* pipe = POPEN(command_str.c_str(), "r");
        if (!pipe) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", command_str}, {"EXIT_CODE", "pipe_failed"}});
        }
        // 使用 unique_ptr 确保文件指针在函数退出时自动关闭
        std::unique_ptr<FILE, decltype(&PCLOSE)> pipe_closer(pipe, PCLOSE);

        int line_count = 0;
        // 简单模拟进度，实际工具输出可能无法直接解析为百分比
        while (fgets(buffer.data(), buffer.size(), pipe_closer.get()) != nullptr) {
            current_line = buffer.data();
            line_count++;
            int percent = std::min(99, (line_count * 5) % 100); // 简单递增进度
            progress_bar.update(percent);

            // 简单的文件名检查 - 可能会很嘈杂
            // if (current_line.find('/') != std::string::npos || current_line.find('\\') != std::string::npos) {
            //      if (current_line.length() > 2 && (std::isalpha(static_cast<unsigned char>(current_line[0])) && current_line[1] == ':')) {
            //          // progress_bar.set_processing_file(current_line);
            //      } else if (current_line.find_first_not_of(" \t\n\r") != std::string::npos && current_line.length() < 80) {
            //          // progress_bar.set_processing_file(current_line);
            //      }
            // }
        }

        int exit_status = PCLOSE(pipe_closer.release()); // 释放 pipe_closer 的所有权，由 PCLOSE 关闭
        int actual_exit_code = 0;
        #ifndef _WIN32
        if (WIFEXITED(exit_status)) {
            actual_exit_code = WEXITSTATUS(exit_status);
        } else {
            actual_exit_code = -1; // 进程未正常退出
        }
        #else
        actual_exit_code = exit_status;
        #endif


        if (actual_exit_code == 0) {
            progress_bar.update(100);
        } else {
            std::cerr << std::endl; // 命令失败时，换行以避免进度条残留
        }
        return actual_exit_code;
    }

    // 获取要归档的项目名称 (例如，对于 "/a/b/file.txt"，返回 "file.txt"；对于 "/a/b/"，返回 "b")
    std::string get_archivable_item_name(const std::string& raw_path_str) {
        if (raw_path_str.empty()) return "."; // 空路径表示当前目录

        fs::path p_orig(raw_path_str);
        std::string s_normalized = p_orig.string();

        // 处理尾部斜杠，使其行为更像 `tar -C` 或 `zip -r`
        if (s_normalized.length() > 0 && s_normalized.back() == fs::path::preferred_separator) {
            // 不要移除根路径的斜杠 (如 "/" 或 "C:\")
            if (s_normalized != p_orig.root_path().string() && s_normalized != "." && s_normalized != "./" && s_normalized != ".\\") {
                if (s_normalized.length() > 1 || (s_normalized.length() == 1 && s_normalized[0] != fs::path::preferred_separator)) {
                    s_normalized.pop_back(); // 移除尾部斜杠以获取父目录名称
                }
            }
        }
        if (raw_path_str == "./" || raw_path_str == ".\\") {
            s_normalized = "."; // 特殊处理 "./" 或 ".\"
        }


        fs::path p_final(s_normalized);
        std::string filename = p_final.filename().string();

        if (filename.empty()) {
            if (p_final.string() == ".") return "."; // 当前目录
            // 对于根路径，如 "C:\" 或 "/"，filename() 返回空
            if (p_final.has_root_name() && !p_final.has_root_directory() && p_final.parent_path().empty()) {
                return p_final.string(); // 如 "C:"
            }
            if (p_final.is_absolute() && p_final.root_directory().string() == p_final.string()) {
                return p_final.string(); // 如 "/"
            }
            return "."; // 默认当前目录
        }
        return filename;
    }

    // 获取用于压缩命令的基目录 (tar -C /path/to/base item_name)
    std::string get_archivable_base_dir(const std::string& raw_path_str) {
        if (raw_path_str.empty()) return ".";

        fs::path p_orig(raw_path_str);
        std::string s_normalized = p_orig.string();

        // 处理尾部斜杠
        if (s_normalized.length() > 0 && s_normalized.back() == fs::path::preferred_separator) {
            if (s_normalized != p_orig.root_path().string()) { // 不要从 "/" 或 "C:\" 移除斜杠
                if (s_normalized.length() > 1 || (s_normalized.length() == 1 && s_normalized[0] != fs::path::preferred_separator) ) {
                    s_normalized.pop_back();
                }
            }
        }
         if (raw_path_str == "./" || raw_path_str == ".\\") {
            s_normalized = ".";
        }


        fs::path p_for_parent_calc(s_normalized);
        fs::path parent = p_for_parent_calc.parent_path();

        if (parent.empty()) {
            return "."; // 如果没有父目录，则使用当前目录
        }
        // 对于绝对根目录，如 "/"，parent_path 也是 "/"。对于 "C:\"，parent_path 也是 "C:\"。
        // 所以 parent.string() 是正确的。
        return parent.string();
    }


    void compress(const std::string& source_path, const std::string& target_path, file_type::FileType target_format_hint) {
        std::string command_str;
        std::string tool;

        bool source_is_dir = false;
        if (fs::exists(source_path)) {
            try {
                source_is_dir = fs::is_directory(fs::weakly_canonical(fs::path(source_path)));
            } catch (const fs::filesystem_error& e) {
                 source_is_dir = fs::is_directory(fs::path(source_path)); // 备用方案
                 std::cerr << "警告: weakly_canonical 对源路径 '" << source_path << "' 失败: " << e.what() << "。回退到直接检查。" << std::endl;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path}});
        }

        std::string item_to_archive = get_archivable_item_name(source_path);
        std::string base_dir_for_cmd = get_archivable_base_dir(source_path);

        // 防御性检查：如果 item_to_archive 为空但 source_path 不为空，则重新评估或使用默认值。
        // 这应该在 get_archivable_item_name 中覆盖。
        if (item_to_archive.empty() && !source_path.empty()) {
            item_to_archive = fs::path(source_path).filename().string(); // 备用方案
            if (item_to_archive.empty()) item_to_archive = ".";
        }


        switch (target_format_hint) {
            case file_type::FileType::ARCHIVE_TAR:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -cf \"" + target_path + "\" -C \"" + base_dir_for_cmd + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_GZ:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -czf \"" + target_path + "\" -C \"" + base_dir_for_cmd + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_BZ2:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -cjf \"" + target_path + "\" -C \"" + base_dir_for_cmd + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_XZ:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -cJf \"" + target_path + "\" -C \"" + base_dir_for_cmd + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "zip";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                if (source_is_dir) {
                    command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " -r \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                } else {
                    command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                }
                break;
            case file_type::FileType::ARCHIVE_RAR: // RAR 压缩通常是专有格式，不太常用
                tool = "rar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool + " (用于压缩)"}});
                command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " a \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " a \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "不支持的压缩目标格式。"}});
        }

        std::cout << i18n::get("compressing") << std::endl;
        progress::ProgressBar progress_bar;
        int result = execute_command(command_str, progress_bar);
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", command_str}, {"EXIT_CODE", std::to_string(result)}});
        }
        std::cout << i18n::get("operation_complete") << std::endl;
    }

    void decompress(const std::string& source_path, const std::string& target_dir_path, file_type::FileType source_archive_type) {
        std::string command_str;
        std::string tool;

        if (!fs::exists(target_dir_path)) {
            try {
                fs::create_directories(target_dir_path);
            } catch (const fs::filesystem_error& e) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_dir_path}, {"REASON", std::string("创建失败: ") + e.what()}});
            }
        } else if (!fs::is_directory(target_dir_path)) {
             error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_dir_path}, {"REASON", "目标存在但不是目录。"}});
        }

        switch (source_archive_type) {
            case file_type::FileType::ARCHIVE_TAR:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -xf \"" + source_path + "\" -C \"" + target_dir_path + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_GZ:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -xzf \"" + source_path + "\" -C \"" + target_dir_path + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_BZ2:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -xjf \"" + source_path + "\" -C \"" + target_dir_path + "\"";
                break;
            case file_type::FileType::ARCHIVE_TAR_XZ:
                tool = "tar";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -xJf \"" + source_path + "\" -C \"" + target_dir_path + "\"";
                break;
            case file_type::FileType::ARCHIVE_ZIP:
                tool = "unzip";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " -o \"" + source_path + "\" -d \"" + target_dir_path + "\"";
                break;
            case file_type::FileType::ARCHIVE_RAR:
                tool = "unrar";
                if (!is_tool_available(tool)) {
                    tool = "rar"; // 尝试 rar x
                    if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", "unrar 或 rar e/x"}});
                    command_str = tool + " x -o+ \"" + source_path + "\" \"" + fs::path(target_dir_path).string() + fs::path::preferred_separator + "\"";
                } else {
                     command_str = tool + " x -o+ \"" + source_path + "\" \"" + fs::path(target_dir_path).string() + fs::path::preferred_separator + "\"";
                }
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = tool + " x \"" + source_path + "\" -o\"" + target_dir_path + "\" -y";
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "不支持的解压缩源格式。"}});
        }

        std::cout << i18n::get("decompressing") << std::endl;
        progress::ProgressBar progress_bar;
        int result = execute_command(command_str, progress_bar);
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, {{"COMMAND", command_str}, {"EXIT_CODE", std::to_string(result)}});
        }
        std::cout << i18n::get("operation_complete") << std::endl;
    }
}

// 交互模式模块
namespace interactive {
    std::string get_input() {
        std::string input;
        std::getline(std::cin, input);
        // 移除前导和尾随的空白字符
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);
        return input;
    }

    int get_choice(int min_val, int max_val) {
        while (true) {
            std::cout << "> ";
            std::string input = get_input();
            try {
                int choice = std::stoi(input);
                if (choice >= min_val && choice <= max_val) {
                    return choice;
                }
            } catch (const std::invalid_argument&) {
                // 输入不是一个有效的整数
            } catch (const std::out_of_range&) {
                // 输入整数超出范围
            }
            std::cout << i18n::get("invalid_choice") << std::endl;
        }
    }

    bool get_confirmation(const std::string& prompt_key, const std::map<std::string, std::string>& placeholders = {}) {
        std::cout << i18n::get(prompt_key, placeholders);
        while (true) {
            std::string input = get_input();
            if (!input.empty()) {
                char choice = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
                if (choice == 'y') return true;
                if (choice == 'n') return false;
            }
            std::cout << i18n::get("invalid_choice") << " (y/n): ";
        }
    }

    void run(std::string& source_path_ref) {
        std::cout << i18n::get("interactive_mode") << std::endl;

        if (source_path_ref.empty()) {
            std::cout << "请输入源路径: ";
            source_path_ref = get_input();
             if (source_path_ref.empty()) {
                std::cerr << "源路径不能为空。" << std::endl;
                return;
            }
        }

        if (!fs::exists(source_path_ref)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_ref}});
        }

        file_type::FileType current_source_type;
        if (fs::is_directory(source_path_ref)) {
            current_source_type = file_type::FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_path_ref)) {
            current_source_type = file_type::recognize_by_extension(source_path_ref);
            if (current_source_type == file_type::FileType::UNKNOWN) {
                current_source_type = file_type::recognize_by_header(source_path_ref);
            }
            if (current_source_type == file_type::FileType::UNKNOWN) {
                current_source_type = file_type::FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_ref}, {"REASON", "不是文件或目录"}});
        }

        std::cout << "源: " << source_path_ref << " (" << file_type::get_file_type_string(current_source_type) << ")" << std::endl;

        file_type::OperationType op_type;
        if (current_source_type == file_type::FileType::DIRECTORY || current_source_type == file_type::FileType::REGULAR_FILE) {
            op_type = file_type::OperationType::COMPRESS;
            std::cout << "默认操作为压缩。" << std::endl;
        } else {
            op_type = file_type::OperationType::DECOMPRESS;
            std::cout << "默认操作为解压缩。" << std::endl;
        }

        // 允许用户选择操作类型，即使有默认值
        std::cout << i18n::get("ask_operation") << std::endl;
        std::cout << i18n::get("operation_compress") << std::endl;
        std::cout << i18n::get("operation_decompress") << std::endl;
        int op_choice = get_choice(1, 2);
        op_type = (op_choice == 1) ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;

        file_type::FileType target_archive_format = file_type::FileType::UNKNOWN;
        std::string target_path_str;

        if (op_type == file_type::OperationType::COMPRESS) {
            std::cout << i18n::get("ask_format") << std::endl;
            std::cout << i18n::get("format_tar_gz") << std::endl; // 默认将 tar.gz 作为 #1
            std::cout << i18n::get("format_zip") << std::endl;
            std::cout << i18n::get("format_tar") << std::endl;
            std::cout << i18n::get("format_tar_bz2") << std::endl;
            std::cout << i18n::get("format_tar_xz") << std::endl;
            std::cout << i18n::get("format_7z") << std::endl;
            // RAR 压缩通常不可用或不建议用于创建
            int format_choice = get_choice(1, 6);
            switch (format_choice) {
                case 1: target_archive_format = file_type::FileType::ARCHIVE_TAR_GZ; break;
                case 2: target_archive_format = file_type::FileType::ARCHIVE_ZIP; break;
                case 3: target_archive_format = file_type::FileType::ARCHIVE_TAR; break;
                case 4: target_archive_format = file_type::FileType::ARCHIVE_TAR_BZ2; break;
                case 5: target_archive_format = file_type::FileType::ARCHIVE_TAR_XZ; break;
                case 6: target_archive_format = file_type::FileType::ARCHIVE_7Z; break;
            }

            std::cout << "请输入目标归档文件名称/路径 (例如: archive.zip 或 /path/to/archive.tar.gz): ";
            target_path_str = get_input();
            if (target_path_str.empty()) {
                std::string ext;
                switch (target_archive_format) {
                    case file_type::FileType::ARCHIVE_TAR: ext = ".tar"; break;
                    case file_type::FileType::ARCHIVE_TAR_GZ: ext = ".tar.gz"; break;
                    case file_type::FileType::ARCHIVE_TAR_BZ2: ext = ".tar.bz2"; break;
                    case file_type::FileType::ARCHIVE_TAR_XZ: ext = ".tar.xz"; break;
                    case file_type::FileType::ARCHIVE_ZIP: ext = ".zip"; break;
                    case file_type::FileType::ARCHIVE_7Z: ext = ".7z"; break;
                    default: ext = ".archive"; // 经验证的选择不应该发生
                }
                target_path_str = fs::path(source_path_ref).stem().string() + ext;
                std::cout << "默认目标为: " << target_path_str << std::endl;
            }
        } else { // 解压缩
            std::cout << "请输入解压缩目标目录 (默认: 当前目录 './'): ";
            target_path_str = get_input();
            if (target_path_str.empty()) {
                target_path_str = ".";
            }
        }

        if (fs::exists(target_path_str)) {
            // 如果是压缩操作，或者解压缩目标存在但不是目录，则询问是否覆盖
            if (op_type == file_type::OperationType::COMPRESS ||
               (op_type == file_type::OperationType::DECOMPRESS && !fs::is_directory(target_path_str))) {
                if (!get_confirmation("ask_overwrite", {{"TARGET_PATH", target_path_str}})) {
                    std::cout << i18n::get("operation_canceled") << std::endl;
                    return;
                }
            }
        }

        bool delete_source = get_confirmation("ask_delete_source", {{"SOURCE_PATH", source_path_ref}});

        try {
            if (op_type == file_type::OperationType::COMPRESS) {
                operation::compress(source_path_ref, target_path_str, target_archive_format);
            } else {
                operation::decompress(source_path_ref, target_path_str, current_source_type);
            }

            if (delete_source) {
                std::cout << "正在删除源文件: " << source_path_ref << std::endl;
                std::error_code ec;
                if (fs::is_directory(source_path_ref)) {
                    fs::remove_all(source_path_ref, ec);
                } else {
                    fs::remove(source_path_ref, ec);
                }
                if (ec) {
                     std::cerr << "警告: 删除源文件 '" << source_path_ref << "' 失败: " << ec.message() << std::endl;
                } else {
                     std::cout << "源文件已删除。" << std::endl;
                }
            }
        } catch (const error::HitpagException& e) {
            std::cerr << e.what() << std::endl; // 已由 throw_error 格式化
        }
    }
}

// 主程序
int main(int argc, char* argv[]) {
    try {
        args::Options options = args::parse(argc, argv);

        if (options.show_help) {
            args::show_help();
            return static_cast<int>(error::ErrorCode::SUCCESS);
        }
        if (options.show_version) {
            args::show_version();
            return static_cast<int>(error::ErrorCode::SUCCESS);
        }

        if (options.interactive_mode) {
            interactive::run(options.source_path);
        } else {
            if (options.source_path.empty()) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "自动模式下需要源路径。"}});
            }
            if (options.target_path.empty()){
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "自动模式下需要目标路径。"}});
            }
            // 仅当两个路径都存在且可比较时才检查相同路径
            if (fs::exists(options.source_path) && fs::exists(options.target_path)) {
                std::error_code ec;
                if (fs::equivalent(fs::path(options.source_path), fs::path(options.target_path), ec)) {
                    if (!ec) error::throw_error(error::ErrorCode::SAME_PATH);
                    // 否则：无法确定等效性，谨慎进行或警告
                }
            }


            file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);

            if (result.operation == file_type::OperationType::COMPRESS) {
                operation::compress(options.source_path, options.target_path, result.target_type_hint);
            } else if (result.operation == file_type::OperationType::DECOMPRESS) {
                operation::decompress(options.source_path, options.target_path, result.source_type);
            }
        }

        std::cout << i18n::get("goodbye") << std::endl;

    } catch (const error::HitpagException& e) {
        std::cerr << e.what() << std::endl;
        return static_cast<int>(e.code());
    } catch (const std::exception& e) {
        std::cerr << "标准异常: " << e.what() << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    } catch (...) {
        std::cerr << "发生未知错误。" << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    }

    return static_cast<int>(error::ErrorCode::SUCCESS);
}
