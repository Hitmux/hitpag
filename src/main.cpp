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

// Internationalization support module - Provides English interface
namespace i18n {
    // All user-visible text messages
    const std::map<std::string, std::string> messages = {
        // General messages
        {"welcome", "Welcome to hitpag smart compression/decompression tool"},
        {"version", "hitpag Version 1.0.0"},
        {"goodbye", "Thank you for using hitpag, goodbye!"},
        
        // Help messages
        {"usage", "Usage: hitpag [options] SOURCE_PATH TARGET_PATH"},
        {"help_options", "Options:"},
        {"help_i", "  -i         Interactive mode"},
        {"help_h", "  -h, --help Display help information"},
        {"help_v", "  -v         Display version information"},
        {"help_examples", "Examples:"},
        {"help_example1", "  hitpag arch.tar.gz ./extracted_dir    # Decompress arch.tar.gz to extracted_dir"},
        {"help_example2", "  hitpag ./my_folder my_archive.zip     # Compress my_folder to my_archive.zip"},
        {"help_example3", "  hitpag -i big_file.rar .              # Interactive decompression of big_file.rar to current directory"},
        
        // Error messages
        {"error_missing_args", "Error: Missing arguments. {ADDITIONAL_INFO}"},
        {"error_invalid_source", "Error: Source path '{PATH}' does not exist or is invalid. {REASON}"},
        {"error_invalid_target", "Error: Invalid target path '{PATH}'. {REASON}"},
        {"error_same_path", "Error: Source and target paths cannot be the same"},
        {"error_unknown_format", "Error: Unrecognized file format or ambiguous operation. {INFO}"},
        {"error_tool_not_found", "Error: Required tool not found: {TOOL_NAME}. Please ensure it is installed and in your system's PATH."},
        {"error_operation_failed", "Error: Operation failed (command: {COMMAND}, exit code: {EXIT_CODE})."},
        {"error_permission_denied", "Error: Permission denied. {PATH}"},
        {"error_not_enough_space", "Error: Not enough disk space"},
        
        // Interactive mode messages
        {"interactive_mode", "Interactive mode started"},
        {"ask_operation", "Please select operation type:"},
        {"operation_compress", "1. Compress"},
        {"operation_decompress", "2. Decompress"},
        {"ask_format", "Please select compression format:"},
        {"format_tar", "1. tar (no compression)"},
        {"format_tar_gz", "2. tar.gz (gzip compression)"},
        {"format_tar_bz2", "3. tar.bz2 (bzip2 compression)"},
        {"format_tar_xz", "4. tar.xz (xz compression)"},
        {"format_zip", "5. zip"},
        {"format_7z", "6. 7z"},
        {"format_rar", "7. rar (decompression only recommended for interactive mode)"}, // Adjusted
        {"ask_overwrite", "Target '{TARGET_PATH}' already exists, overwrite? (y/n): "},
        {"ask_delete_source", "Delete source '{SOURCE_PATH}' after operation? (y/n): "},
        {"invalid_choice", "Invalid choice, please try again"},
        
        // Operation messages
        {"compressing", "Compressing..."},
        {"decompressing", "Decompressing..."},
        {"operation_complete", "Operation complete"},
        {"operation_canceled", "Operation canceled"},
        
        // Progress display
        {"progress", "Progress: "},
        {"remaining_time", "Estimated remaining time: "}, // Placeholder, not implemented
        {"processing_file", "Processing: "}
    };
    
    // Get message text
    std::string get(const std::string& key, const std::map<std::string, std::string>& placeholders = {}) {
        auto it = messages.find(key);
        std::string message_template;
        if (it != messages.end()) {
            message_template = it->second;
        } else {
            return "[" + key + "]"; // Return key itself if not found
        }

        for(const auto& p : placeholders) {
            std::string placeholder_key = "{" + p.first + "}";
            size_t pos = message_template.find(placeholder_key);
            while(pos != std::string::npos) {
                message_template.replace(pos, placeholder_key.length(), p.second);
                pos = message_template.find(placeholder_key, pos + p.second.length());
            }
        }
        // Remove unused placeholders
        size_t start_ph = message_template.find("{");
        while(start_ph != std::string::npos) {
            size_t end_ph = message_template.find("}", start_ph);
            if (end_ph != std::string::npos) {
                message_template.replace(start_ph, end_ph - start_ph + 1, "");
            } else {
                break; // No closing brace
            }
            start_ph = message_template.find("{");
        }
        return message_template;
    }
}

// Error handling module
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

// Argument parsing module
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
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Unknown option: " + opt}});
            }
            current_arg_idx++;
        }
        
        if (current_arg_idx < args_vec.size()) {
            options.source_path = args_vec[current_arg_idx++];
        } else {
            if (!options.interactive_mode && !options.show_help && !options.show_version) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Source path missing"}});
            }
        }
        
        if (current_arg_idx < args_vec.size()) {
            options.target_path = args_vec[current_arg_idx++];
        } else {
            if (!options.interactive_mode && !options.show_help && !options.show_version && !options.source_path.empty()) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target path missing"}});
            }
        }

        if (current_arg_idx < args_vec.size()) {
            error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Too many arguments"}});
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

// File type recognition module
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

        if (header[0] == 0x50 && header[1] == 0x4B) return FileType::ARCHIVE_ZIP;
        if (header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21) return FileType::ARCHIVE_RAR; 
        if (header[0] == 0x37 && header[1] == 0x7A && header[2] == (char)0xBC && header[3] == (char)0xAF) return FileType::ARCHIVE_7Z;
        if (header[0] == (char)0x1F && header[1] == (char)0x8B) return FileType::ARCHIVE_TAR_GZ; 
        if (header[0] == 0x42 && header[1] == 0x5A && header[2] == 0x68) return FileType::ARCHIVE_TAR_BZ2; 
        if (header[0] == (char)0xFD && header[1] == 0x37 && header[2] == 0x7A && header[3] == 0x58 && file.gcount() >= 6 && header[4] == 0x5A && header[5] == 0x00) return FileType::ARCHIVE_TAR_XZ; 

        file.clear(); 
        file.seekg(257);
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
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_str}, {"REASON", "not a regular file or directory"}});
        }

        bool target_is_archive_extension = false;
        if (!target_path_str.empty()) {
            result.target_type_hint = recognize_by_extension(target_path_str);
            target_is_archive_extension = (result.target_type_hint != FileType::UNKNOWN && 
                                           result.target_type_hint != FileType::REGULAR_FILE && 
                                           result.target_type_hint != FileType::DIRECTORY);
        }

        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            if (target_is_archive_extension) {
                result.operation = OperationType::COMPRESS;
            } else if (!target_path_str.empty() && fs::exists(target_path_str) && fs::is_directory(target_path_str)) {
                 error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Cannot compress into an existing directory without specifying an archive name. Target for compression must be an archive file name."}});
            } else if (!target_path_str.empty()) { 
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Target for compression must have a recognized archive extension."}});
            } else {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target path required for compression."}});
            }
        } else { 
            result.operation = OperationType::DECOMPRESS;
            if (target_path_str.empty()) {
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target directory required for decompression."}});
            }
            if (fs::exists(target_path_str) && !fs::is_directory(target_path_str)) {
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_path_str}, {"REASON", "Target for decompression must be a directory."}});
            }
        }
        
        if (result.operation == OperationType::UNKNOWN) {
            error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }
        
        return result;
    }

    std::string get_file_type_string(FileType type) {
        switch (type) {
            case FileType::REGULAR_FILE: return "Regular File";
            case FileType::DIRECTORY: return "Directory";
            case FileType::ARCHIVE_TAR: return "TAR Archive";
            case FileType::ARCHIVE_TAR_GZ: return "TAR.GZ Archive";
            case FileType::ARCHIVE_TAR_BZ2: return "TAR.BZ2 Archive";
            case FileType::ARCHIVE_TAR_XZ: return "TAR.XZ Archive";
            case FileType::ARCHIVE_ZIP: return "ZIP Archive";
            case FileType::ARCHIVE_RAR: return "RAR Archive";
            case FileType::ARCHIVE_7Z: return "7Z Archive";
            default: return "Unknown Type";
        }
    }
    std::string get_operation_type_string(OperationType type) {
        switch (type) {
            case OperationType::COMPRESS: return "Compress";
            case OperationType::DECOMPRESS: return "Decompress";
            default: return "Unknown Operation";
        }
    }
}

// Progress display module
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
            std::cout << "\r" << std::string(width_ + 10 + i18n::get("progress").length() + 5, ' ') << "\r"; 
            std::cout << i18n::get("processing_file") << filename << std::endl;
            if (last_percent_ != -1 && last_percent_ < 100) { 
                update(last_percent_);
            }
        }
    };
}

// Compression/decompression dispatch module
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
        std::unique_ptr<FILE, decltype(&PCLOSE)> pipe_closer(pipe, PCLOSE);

        int line_count = 0; 
        while (fgets(buffer.data(), buffer.size(), pipe_closer.get()) != nullptr) {
            current_line = buffer.data();
            line_count++;
            int percent = std::min(99, (line_count * 5) % 100); 
            progress_bar.update(percent);

            // Simple check for filenames - can be noisy
            // if (current_line.find('/') != std::string::npos || current_line.find('\\') != std::string::npos) {
            //      if (current_line.length() > 2 && (std::isalpha(static_cast<unsigned char>(current_line[0])) && current_line[1] == ':')) { 
            //          // progress_bar.set_processing_file(current_line); 
            //      } else if (current_line.find_first_not_of(" \t\n\r") != std::string::npos && current_line.length() < 80) { 
            //          // progress_bar.set_processing_file(current_line); 
            //      }
            // }
        }
        
        int exit_status = PCLOSE(pipe_closer.release());
        int actual_exit_code = 0;
        #ifndef _WIN32 
        if (WIFEXITED(exit_status)) {
            actual_exit_code = WEXITSTATUS(exit_status);
        } else {
            actual_exit_code = -1; 
        }
        #else 
        actual_exit_code = exit_status;
        #endif


        if (actual_exit_code == 0) {
            progress_bar.update(100);
        } else {
            std::cerr << std::endl; 
        }
        return actual_exit_code;
    }

    std::string get_archivable_item_name(const std::string& raw_path_str) {
        if (raw_path_str.empty()) return ".";

        fs::path p_orig(raw_path_str);
        std::string s_normalized = p_orig.string();

        if (s_normalized.length() > 0 && s_normalized.back() == fs::path::preferred_separator) {
            if (s_normalized != p_orig.root_path().string() && s_normalized != "." && s_normalized != "./" && s_normalized != ".\\") {
                if (s_normalized.length() > 1 || (s_normalized.length() == 1 && s_normalized[0] != fs::path::preferred_separator)) {
                    s_normalized.pop_back();
                }
            }
        }
        if (raw_path_str == "./" || raw_path_str == ".\\") {
            s_normalized = ".";
        }


        fs::path p_final(s_normalized);
        std::string filename = p_final.filename().string();

        if (filename.empty()) {
            if (p_final.string() == ".") return ".";
            if (p_final.has_root_name() && !p_final.has_root_directory() && p_final.parent_path().empty()) {
                return p_final.string(); 
            }
            if (p_final.is_absolute() && p_final.root_directory().string() == p_final.string()) { 
                return p_final.string();
            }
            return "."; 
        }
        return filename;
    }

    std::string get_archivable_base_dir(const std::string& raw_path_str) {
        if (raw_path_str.empty()) return ".";

        fs::path p_orig(raw_path_str);
        std::string s_normalized = p_orig.string();

        if (s_normalized.length() > 0 && s_normalized.back() == fs::path::preferred_separator) {
            if (s_normalized != p_orig.root_path().string()) { // Don't strip from "/" or "C:\"
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
            return ".";
        }
        // For absolute root like "/", parent_path is "/". For "C:\", parent_path is "C:\".
        // So parent.string() is correct.
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
                 source_is_dir = fs::is_directory(fs::path(source_path));
                 std::cerr << "Warning: weakly_canonical failed for source path '" << source_path << "': " << e.what() << ". Falling back to direct check." << std::endl;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path}});
        }

        std::string item_to_archive = get_archivable_item_name(source_path);
        std::string base_dir_for_cmd = get_archivable_base_dir(source_path);
        
        // Safeguard: if item_to_archive is empty but source_path wasn't, re-evaluate or default.
        // This should ideally be covered by get_archivable_item_name.
        if (item_to_archive.empty() && !source_path.empty()) {
            item_to_archive = fs::path(source_path).filename().string(); // Fallback
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
            case file_type::FileType::ARCHIVE_RAR: // Compression with rar is less common/often proprietary
                tool = "rar"; 
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool + " (for compression)"}});
                command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " a \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                break;
            case file_type::FileType::ARCHIVE_7Z:
                tool = "7z";
                if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", tool}});
                command_str = "cd \"" + base_dir_for_cmd + "\" && " + tool + " a \"" + fs::absolute(target_path).string() + "\" \"" + item_to_archive + "\"";
                break;
            default:
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Unsupported target format for compression."}});
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
                error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_dir_path}, {"REASON", std::string("Failed to create: ") + e.what()}});
            }
        } else if (!fs::is_directory(target_dir_path)) {
             error::throw_error(error::ErrorCode::INVALID_TARGET, {{"PATH", target_dir_path}, {"REASON", "Target exists but is not a directory."}});
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
                    tool = "rar"; // Try rar x
                    if (!is_tool_available(tool)) error::throw_error(error::ErrorCode::TOOL_NOT_FOUND, {{"TOOL_NAME", "unrar or rar e/x"}});
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
                error::throw_error(error::ErrorCode::UNKNOWN_FORMAT, {{"INFO", "Unsupported source format for decompression."}});
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

// Interactive mode module
namespace interactive {
    std::string get_input() {
        std::string input;
        std::getline(std::cin, input);
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
            } catch (const std::out_of_range&) {
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
            std::cout << "Please enter source path: ";
            source_path_ref = get_input();
             if (source_path_ref.empty()) {
                std::cerr << "Source path cannot be empty." << std::endl;
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
            error::throw_error(error::ErrorCode::INVALID_SOURCE, {{"PATH", source_path_ref}, {"REASON", "Not a file or directory"}});
        }
        
        std::cout << "Source: " << source_path_ref << " (" << file_type::get_file_type_string(current_source_type) << ")" << std::endl;
        
        file_type::OperationType op_type;
        if (current_source_type == file_type::FileType::DIRECTORY || current_source_type == file_type::FileType::REGULAR_FILE) {
            op_type = file_type::OperationType::COMPRESS;
            std::cout << "Defaulting to Compress operation." << std::endl;
        } else {
            op_type = file_type::OperationType::DECOMPRESS;
            std::cout << "Defaulting to Decompress operation." << std::endl;
        }
        
        std::cout << i18n::get("ask_operation") << std::endl;
        std::cout << i18n::get("operation_compress") << std::endl;
        std::cout << i18n::get("operation_decompress") << std::endl;
        int op_choice = get_choice(1, 2);
        op_type = (op_choice == 1) ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;
        
        file_type::FileType target_archive_format = file_type::FileType::UNKNOWN;
        std::string target_path_str;

        if (op_type == file_type::OperationType::COMPRESS) {
            std::cout << i18n::get("ask_format") << std::endl;
            std::cout << i18n::get("format_tar_gz") << std::endl; // Default to tar.gz as #1
            std::cout << i18n::get("format_zip") << std::endl;
            std::cout << i18n::get("format_tar") << std::endl;
            std::cout << i18n::get("format_tar_bz2") << std::endl;
            std::cout << i18n::get("format_tar_xz") << std::endl;
            std::cout << i18n::get("format_7z") << std::endl;
            // RAR compression is often not available or desired for creation
            int format_choice = get_choice(1, 6); 
            switch (format_choice) {
                case 1: target_archive_format = file_type::FileType::ARCHIVE_TAR_GZ; break;
                case 2: target_archive_format = file_type::FileType::ARCHIVE_ZIP; break;
                case 3: target_archive_format = file_type::FileType::ARCHIVE_TAR; break;
                case 4: target_archive_format = file_type::FileType::ARCHIVE_TAR_BZ2; break;
                case 5: target_archive_format = file_type::FileType::ARCHIVE_TAR_XZ; break;
                case 6: target_archive_format = file_type::FileType::ARCHIVE_7Z; break;
            }
            
            std::cout << "Please enter target archive name/path (e.g., archive.zip or /path/to/archive.tar.gz): ";
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
                    default: ext = ".archive"; // Should not happen with validated choice
                }
                target_path_str = fs::path(source_path_ref).stem().string() + ext;
                std::cout << "Defaulting target to: " << target_path_str << std::endl;
            }
        } else { // Decompress
            std::cout << "Please enter target directory for extraction (default: current directory './'): ";
            target_path_str = get_input();
            if (target_path_str.empty()) {
                target_path_str = ".";
            }
        }

        if (fs::exists(target_path_str)) {
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
                std::cout << "Deleting source: " << source_path_ref << std::endl;
                std::error_code ec;
                if (fs::is_directory(source_path_ref)) {
                    fs::remove_all(source_path_ref, ec);
                } else {
                    fs::remove(source_path_ref, ec);
                }
                if (ec) {
                     std::cerr << "Warning: Failed to delete source '" << source_path_ref << "': " << ec.message() << std::endl;
                } else {
                     std::cout << "Source deleted." << std::endl;
                }
            }
        } catch (const error::HitpagException& e) {
            std::cerr << e.what() << std::endl; // Already formatted by throw_error
        }
    }
}

// Main program
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
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Source path required in automatic mode."}});
            }
            if (options.target_path.empty()){
                 error::throw_error(error::ErrorCode::MISSING_ARGS, {{"ADDITIONAL_INFO", "Target path required in automatic mode."}});
            }
            // Check for same path only if both exist and can be compared
            if (fs::exists(options.source_path) && fs::exists(options.target_path)) {
                std::error_code ec;
                if (fs::equivalent(fs::path(options.source_path), fs::path(options.target_path), ec)) {
                    if (!ec) error::throw_error(error::ErrorCode::SAME_PATH);
                    // else: cannot determine equivalence, proceed with caution or warn
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
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return static_cast<int>(error::ErrorCode::UNKNOWN_ERROR);
    }
    
    return static_cast<int>(error::ErrorCode::SUCCESS);
}
