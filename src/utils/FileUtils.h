#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <chrono>

class FileUtils {
public:
    struct FileInfo {
        std::string path;
        std::string relativePath;
        uint64_t size;
        std::chrono::system_clock::time_point lastModified;
        bool isDirectory;
        bool isRegularFile;
        bool isReadable;
        bool isWritable;
    };

    enum class OverwriteMode {
        ASK,
        OVERWRITE,
        SKIP,
        RENAME
    };

public:
    // File existence and validation
    static bool exists(const std::string& path);
    static bool isFile(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool isReadable(const std::string& path);
    static bool isWritable(const std::string& path);
    
    // File information
    static FileInfo getFileInfo(const std::string& path);
    static uint64_t getFileSize(const std::string& path);
    static std::chrono::system_clock::time_point getLastModifiedTime(const std::string& path);
    static std::string getFileExtension(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getBaseName(const std::string& path);
    static std::string getDirectoryName(const std::string& path);
    
    // Directory operations
    static bool createDirectory(const std::string& path);
    static bool createDirectories(const std::string& path);
    static std::vector<FileInfo> listDirectory(const std::string& path, bool recursive = false);
    static std::vector<std::string> listFiles(const std::string& path, bool recursive = false);
    static std::vector<std::string> listDirectories(const std::string& path, bool recursive = false);
    
    // File operations
    static std::vector<uint8_t> readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    static bool appendFile(const std::string& path, const std::vector<uint8_t>& data);
    static bool copyFile(const std::string& source, const std::string& destination, OverwriteMode mode = OverwriteMode::ASK);
    static bool moveFile(const std::string& source, const std::string& destination, OverwriteMode mode = OverwriteMode::ASK);
    static bool deleteFile(const std::string& path);
    static bool deleteDirectory(const std::string& path, bool recursive = false);
    
    // Path operations
    static std::string normalizePath(const std::string& path);
    static std::string joinPath(const std::string& base, const std::string& relative);
    static std::string getRelativePath(const std::string& base, const std::string& path);
    static std::string getAbsolutePath(const std::string& path);
    static std::string getCurrentDirectory();
    static bool setCurrentDirectory(const std::string& path);
    
    // Pattern matching
    static bool matchesPattern(const std::string& filename, const std::string& pattern);
    static std::vector<std::string> findFiles(const std::string& path, const std::string& pattern, bool recursive = false);
    
    // Utility methods
    static std::string formatFileSize(uint64_t size);
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& time);
    static bool isValidPath(const std::string& path);
    static bool isValidFileName(const std::string& filename);
    
    // Temporary files
    static std::string createTempFile(const std::string& prefix = "zipit_", const std::string& suffix = "");
    static std::string createTempDirectory(const std::string& prefix = "zipit_");
    static bool isTempPath(const std::string& path);

private:
    // Internal helpers
    static std::vector<FileInfo> listDirectoryRecursive(const std::string& path, const std::string& relativeBase = "");
    static bool handleOverwrite(const std::string& destination, OverwriteMode mode);
    static std::string generateUniqueFileName(const std::string& path);
    static bool isWildcardPattern(const std::string& pattern);
    static bool wildcardMatch(const std::string& str, const std::string& pattern);
    
    // Constants
    static constexpr size_t MAX_PATH_LENGTH = 32767; // Windows MAX_PATH
    static constexpr size_t BUFFER_SIZE = 8192;
};