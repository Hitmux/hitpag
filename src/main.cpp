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
#include <algorithm> // For std::tolower

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
        {"error_missing_args", "Error: Missing arguments"},
        {"error_invalid_source", "Error: Source path does not exist"},
        {"error_invalid_target", "Error: Invalid target path"},
        {"error_same_path", "Error: Source and target paths cannot be the same"},
        {"error_unknown_format", "Error: Unrecognized file format"},
        {"error_tool_not_found", "Error: Required tool not found: "},
        {"error_operation_failed", "Error: Operation failed: "},
        {"error_permission_denied", "Error: Permission denied"},
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
        {"format_rar", "7. rar"},
        {"ask_overwrite", "Target already exists, overwrite? (y/n): "},
        {"ask_delete_source", "Delete source file after operation? (y/n): "},
        {"invalid_choice", "Invalid choice, please try again"},
        
        // Operation messages
        {"compressing", "Compressing..."},
        {"decompressing", "Decompressing..."},
        {"operation_complete", "Operation complete"},
        {"operation_canceled", "Operation canceled"},
        
        // Progress display
        {"progress", "Progress: "},
        {"remaining_time", "Estimated remaining time: "},
        {"processing_file", "Processing: "}
    };
    
    // Get message text
    std::string get(const std::string& key) {
        auto it = messages.find(key);
        if (it != messages.end()) {
            return it->second;
        }
        return "[" + key + "]";
    }
}

// Error handling module
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
                message = "Unknown error";
        }
        
        throw HitpagException(code, message);
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
        
        // If no arguments, show help
        if (argc < 2) {
            options.show_help = true;
            return options;
        }
        
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        
        // Parse options
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
        
        // Parse path arguments
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
        FileType target_type = FileType::UNKNOWN;
        OperationType operation = OperationType::UNKNOWN;
    };
    
    // Recognize file type by extension
    FileType recognize_by_extension(const std::string& path) {
        std::string ext = fs::path(path).extension().string();
        std::string stem_ext = fs::path(fs::path(path).stem()).extension().string();
        
        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::transform(stem_ext.begin(), stem_ext.end(), stem_ext.begin(), ::tolower);
        
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
    
    // Recognize file type by header information
    FileType recognize_by_header(const std::string& path) {
        // Open file
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return FileType::UNKNOWN;
        }
        
        // Read file header
        std::array<char, 8> header;
        file.read(header.data(), header.size());
        
        // Check file header signatures
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
        
        // Check for TAR file (TAR header has "ustar" at offset 257)
        file.seekg(257);
        std::array<char, 5> tar_header;
        file.read(tar_header.data(), tar_header.size());
        if (std::string(tar_header.data(), 5) == "ustar") {
            return FileType::ARCHIVE_TAR;
        }
        
        return FileType::UNKNOWN;
    }
    
    // Recognize file type and operation type
    RecognitionResult recognize(const std::string& source_path, const std::string& target_path) {
        RecognitionResult result;
        
        // Check if source path exists
        if (!fs::exists(source_path)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // Recognize source path type
        if (fs::is_directory(source_path)) {
            result.source_type = FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_path)) {
            // First try to recognize by extension
            result.source_type = recognize_by_extension(source_path);
            
            // If not recognized by extension, try to recognize by file header
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = recognize_by_header(source_path);
            }
            
            // Debug output
            std::string type_str;
            switch (result.source_type) {
                case FileType::REGULAR_FILE: type_str = "Regular File"; break;
                case FileType::DIRECTORY: type_str = "Directory"; break;
                case FileType::ARCHIVE_TAR: type_str = "TAR Archive"; break;
                case FileType::ARCHIVE_TAR_GZ: type_str = "TAR.GZ Archive"; break;
                case FileType::ARCHIVE_TAR_BZ2: type_str = "TAR.BZ2 Archive"; break;
                case FileType::ARCHIVE_TAR_XZ: type_str = "TAR.XZ Archive"; break;
                case FileType::ARCHIVE_ZIP: type_str = "ZIP Archive"; break;
                case FileType::ARCHIVE_RAR: type_str = "RAR Archive"; break;
                case FileType::ARCHIVE_7Z: type_str = "7Z Archive"; break;
                default: type_str = "Unknown Type"; break;
            }
            std::cout << "DEBUG: Source file type recognition result: " << type_str << std::endl;
            
            // If still unrecognized, treat as regular file
            if (result.source_type == FileType::UNKNOWN) {
                result.source_type = FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // Recognize target path type
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
            // Target path does not exist, determine by extension
            result.target_type = recognize_by_extension(target_path);
            
            // If not recognized by extension, determine if it's a directory based on trailing slash
            if (result.target_type == FileType::UNKNOWN) {
                if (!target_path.empty() && (target_path.back() == '/' || target_path.back() == '\\')) {
                    result.target_type = FileType::DIRECTORY;
                } else {
                    // If source is an archive and target is unrecognized, default target to directory (decompression target)
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
        
        // Debug output
        std::string target_type_str;
        switch (result.target_type) {
            case FileType::REGULAR_FILE: target_type_str = "Regular File"; break;
            case FileType::DIRECTORY: target_type_str = "Directory"; break;
            case FileType::ARCHIVE_TAR: target_type_str = "TAR Archive"; break;
            case FileType::ARCHIVE_TAR_GZ: target_type_str = "TAR.GZ Archive"; break;
            case FileType::ARCHIVE_TAR_BZ2: target_type_str = "TAR.BZ2 Archive"; break;
            case FileType::ARCHIVE_TAR_XZ: target_type_str = "TAR.XZ Archive"; break;
            case FileType::ARCHIVE_ZIP: target_type_str = "ZIP Archive"; break;
            case FileType::ARCHIVE_RAR: target_type_str = "RAR Archive"; break;
            case FileType::ARCHIVE_7Z: target_type_str = "7Z Archive"; break;
            default: target_type_str = "Unknown Type"; break;
        }
        std::cout << "DEBUG: Target path type recognition result: " << target_type_str << std::endl;
        
        // Determine operation type
        if (result.source_type == FileType::DIRECTORY || result.source_type == FileType::REGULAR_FILE) {
            // Source is a directory or regular file, target is an archive, perform compression
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
            // Source is an archive, target is a directory, perform decompression
            if (result.target_type == FileType::DIRECTORY) {
                result.operation = OperationType::DECOMPRESS;
            }
        }
        
        // If operation type cannot be determined, throw an error
        if (result.operation == OperationType::UNKNOWN) {
            error::throw_error(error::ErrorCode::UNKNOWN_FORMAT);
        }
        
        return result;
    }
    
    // Get string representation of file type
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
    
    // Get string representation of operation type
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

// Compression/decompression dispatch module
namespace operation {
    // Check if tool is available
    bool is_tool_available(const std::string& tool) {
        std::string command = "which " + tool + " > /dev/null 2>&1";
        return system(command.c_str()) == 0;
    }
    
    // Execute command and capture output
    int execute_command(const std::string& command, progress::ProgressBar& progress_bar) {
        std::array<char, 128> buffer;
        std::string result_line;
        
        // Create pipe
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, command);
        }
        
        // Read output
        int line_count = 0;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result_line = buffer.data();
            
            // Update progress (this is just a simple simulation, actual progress parsing depends on tool output format)
            line_count++;
            int percent = std::min(99, line_count % 100);
            progress_bar.update(percent);
            
            // If output contains a filename, display the file being processed
            if (result_line.find('/') != std::string::npos) {
                progress_bar.set_processing_file(result_line);
            }
        }
        
        // Complete
        progress_bar.update(100);
        
        return WEXITSTATUS(pclose(pipe.release()));
    }
    
    // Compression operation
    void compress(const std::string& source_path, const std::string& target_path, file_type::FileType target_type) {
        std::string command;
        std::string tool;
        
        // Select compression tool and command based on target type
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
        
        // Display compression info
        std::cout << i18n::get("compressing") << std::endl;
        
        // Create progress bar
        progress::ProgressBar progress_bar;
        
        // Execute command
        int result = execute_command(command, progress_bar);
        
        // Check result
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, std::to_string(result));
        }
        
        std::cout << i18n::get("operation_complete") << std::endl;
    }
    
    // Decompression operation
    void decompress(const std::string& source_path, const std::string& target_path, file_type::FileType source_type) {
        std::string command;
        std::string tool;
        
        // Create target directory (if it doesn't exist)
        if (!fs::exists(target_path)) {
            fs::create_directories(target_path);
        }
        
        // Select decompression tool and command based on source type
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
        
        // Display decompression info
        std::cout << i18n::get("decompressing") << std::endl;
        
        // Create progress bar
        progress::ProgressBar progress_bar;
        
        // Execute command
        int result = execute_command(command, progress_bar);
        
        // Check result
        if (result != 0) {
            error::throw_error(error::ErrorCode::OPERATION_FAILED, std::to_string(result));
        }
        
        std::cout << i18n::get("operation_complete") << std::endl;
    }
}

// Interactive mode module
namespace interactive {
    // Get user input
    std::string get_input() {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
    
    // Get user choice (number)
    int get_choice(int min, int max) {
        while (true) {
            std::string input = get_input();
            try {
                int choice = std::stoi(input);
                if (choice >= min && choice <= max) {
                    return choice;
                }
            } catch (...) {
                // Ignore conversion errors
            }
            std::cout << i18n::get("invalid_choice") << std::endl;
        }
    }
    
    // Get user confirmation (y/n)
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
    
    // Interactive operation
    void run(const std::string& source_path) {
        std::cout << i18n::get("interactive_mode") << std::endl;
        
        // Check if source path exists
        if (!fs::exists(source_path)) {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // Determine source file type
        file_type::FileType source_type;
        if (fs::is_directory(source_path)) {
            source_type = file_type::FileType::DIRECTORY;
        } else if (fs::is_regular_file(source_path)) {
            // First try to recognize by extension
            source_type = file_type::recognize_by_extension(source_path);
            
            // If not recognized by extension, try to recognize by file header
            if (source_type == file_type::FileType::UNKNOWN) {
                source_type = file_type::recognize_by_header(source_path);
            }
            
            // If still unrecognized, treat as regular file
            if (source_type == file_type::FileType::UNKNOWN) {
                source_type = file_type::FileType::REGULAR_FILE;
            }
        } else {
            error::throw_error(error::ErrorCode::INVALID_SOURCE);
        }
        
        // Display source file type
        std::cout << "Source file type: " << file_type::get_file_type_string(source_type) << std::endl;
        
        // Determine operation type
        file_type::OperationType operation_type;
        if (source_type == file_type::FileType::DIRECTORY || source_type == file_type::FileType::REGULAR_FILE) {
            // Source is a directory or regular file, default to compression
            operation_type = file_type::OperationType::COMPRESS;
        } else {
            // Source is an archive file, default to decompression
            operation_type = file_type::OperationType::DECOMPRESS;
        }
        
        // Ask for operation type
        std::cout << i18n::get("ask_operation") << std::endl;
        std::cout << i18n::get("operation_compress") << std::endl;
        std::cout << i18n::get("operation_decompress") << std::endl;
        int choice = get_choice(1, 2);
        operation_type = (choice == 1) ? file_type::OperationType::COMPRESS : file_type::OperationType::DECOMPRESS;
        
        // If it's a compression operation, ask for compression format
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
        
        // Ask for target path
        std::string target_path;
        std::cout << "Please enter target path: ";
        target_path = get_input();
        
        // If target path is empty, use default path
        if (target_path.empty()) {
            if (operation_type == file_type::OperationType::COMPRESS) {
                // Default to compressing to current directory, using source filename with appropriate extension
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
                // Default to decompressing to current directory
                target_path = ".";
            }
        }
        
        // Check if target path exists
        if (fs::exists(target_path)) {
            std::cout << i18n::get("ask_overwrite");
            if (!get_confirmation()) {
                std::cout << i18n::get("operation_canceled") << std::endl;
                return;
            }
        }
        
        // Ask whether to delete source file
        bool delete_source = false;
        std::cout << i18n::get("ask_delete_source");
        delete_source = get_confirmation();
        
        // Perform operation
        try {
            if (operation_type == file_type::OperationType::COMPRESS) {
                operation::compress(source_path, target_path, target_type);
            } else {
                operation::decompress(source_path, target_path, source_type);
            }
            
            // If source file needs to be deleted
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

// Main program
int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        args::Options options = args::parse(argc, argv);
        
        // Display help information
        if (options.show_help) {
            args::show_help();
            return 0;
        }
        
        // Display version information
        if (options.show_version) {
            args::show_version();
            return 0;
        }
        
        // Interactive mode
        if (options.interactive_mode) {
            interactive::run(options.source_path);
            return 0;
        }
        
        // Automatic mode
        // Recognize file type and operation type
        file_type::RecognitionResult result = file_type::recognize(options.source_path, options.target_path);
        
        // Perform operation
        if (result.operation == file_type::OperationType::COMPRESS) {
            operation::compress(options.source_path, options.target_path, result.target_type);
        } else if (result.operation == file_type::OperationType::DECOMPRESS) {
            operation::decompress(options.source_path, options.target_path, result.source_type);
        }
        
    } catch (const error::HitpagException& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}