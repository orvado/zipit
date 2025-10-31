#include "FileUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <random>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

bool FileUtils::exists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool FileUtils::isFile(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool FileUtils::isDirectory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool FileUtils::isReadable(const std::string& path) {
    std::error_code ec;
    auto perms = std::filesystem::status(path, ec).permissions();
    return (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none;
}

bool FileUtils::isWritable(const std::string& path) {
    std::error_code ec;
    auto perms = std::filesystem::status(path, ec).permissions();
    return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
}

FileUtils::FileInfo FileUtils::getFileInfo(const std::string& path) {
    FileInfo info;
    info.path = path;
    
    std::error_code ec;
    auto fstatus = std::filesystem::status(path, ec);
    
    if (ec) {
        info.isDirectory = false;
        info.isRegularFile = false;
        info.isReadable = false;
        info.isWritable = false;
        info.size = 0;
        return info;
    }
    
    info.isDirectory = std::filesystem::is_directory(fstatus);
    info.isRegularFile = std::filesystem::is_regular_file(fstatus);
    info.isReadable = isReadable(path);
    info.isWritable = isWritable(path);
    
    if (info.isRegularFile) {
        std::error_code sizeEc;
        info.size = std::filesystem::file_size(path, sizeEc);
        if (sizeEc) {
            info.size = 0;
        }
    } else {
        info.size = 0;
    }
    
    std::error_code timeEc;
    auto ftime = std::filesystem::last_write_time(path, timeEc);
    if (!timeEc) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        info.lastModified = sctp;
    }
    
    return info;
}

uint64_t FileUtils::getFileSize(const std::string& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    return ec ? 0 : size;
}

std::chrono::system_clock::time_point FileUtils::getLastModifiedTime(const std::string& path) {
    std::error_code ec;
    auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return std::chrono::system_clock::now();
    }
    
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
}

std::string FileUtils::getFileExtension(const std::string& path) {
    std::filesystem::path p(path);
    return p.extension().string();
}

std::string FileUtils::getFileName(const std::string& path) {
    std::filesystem::path p(path);
    return p.filename().string();
}

std::string FileUtils::getBaseName(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

std::string FileUtils::getDirectoryName(const std::string& path) {
    std::filesystem::path p(path);
    return p.parent_path().string();
}

bool FileUtils::createDirectory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directory(path, ec);
}

bool FileUtils::createDirectories(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec);
}

std::vector<FileUtils::FileInfo> FileUtils::listDirectory(const std::string& path, bool recursive) {
    if (!isDirectory(path)) {
        return {};
    }
    
    if (recursive) {
        return listDirectoryRecursive(path);
    }
    
    std::vector<FileInfo> files;
    std::error_code ec;
    
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        FileInfo info = getFileInfo(entry.path().string());
        info.relativePath = getRelativePath(path, info.path);
        files.push_back(info);
    }
    
    return files;
}

std::vector<std::string> FileUtils::listFiles(const std::string& path, bool recursive) {
    auto fileInfos = listDirectory(path, recursive);
    std::vector<std::string> files;
    
    for (const auto& info : fileInfos) {
        if (info.isRegularFile) {
            files.push_back(info.path);
        }
    }
    
    return files;
}

std::vector<std::string> FileUtils::listDirectories(const std::string& path, bool recursive) {
    auto fileInfos = listDirectory(path, recursive);
    std::vector<std::string> directories;
    
    for (const auto& info : fileInfos) {
        if (info.isDirectory) {
            directories.push_back(info.path);
        }
    }
    
    return directories;
}

std::vector<uint8_t> FileUtils::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return data;
}

bool FileUtils::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool FileUtils::appendFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool FileUtils::copyFile(const std::string& source, const std::string& destination, OverwriteMode mode) {
    if (!exists(source)) {
        return false;
    }
    
    if (exists(destination) && !handleOverwrite(destination, mode)) {
        return false;
    }
    
    std::error_code ec;
    return std::filesystem::copy_file(source, destination, 
        std::filesystem::copy_options::overwrite_existing, ec);
}

bool FileUtils::moveFile(const std::string& source, const std::string& destination, OverwriteMode mode) {
    if (!exists(source)) {
        return false;
    }
    
    if (exists(destination) && !handleOverwrite(destination, mode)) {
        return false;
    }
    
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    return !ec;
}

bool FileUtils::deleteFile(const std::string& path) {
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

bool FileUtils::deleteDirectory(const std::string& path, bool recursive) {
    if (!isDirectory(path)) {
        return false;
    }
    
    std::error_code ec;
    if (recursive) {
        return std::filesystem::remove_all(path, ec) > 0;
    } else {
        return std::filesystem::remove(path, ec);
    }
}

std::string FileUtils::normalizePath(const std::string& path) {
    std::filesystem::path p(path);
    return p.lexically_normal().string();
}

std::string FileUtils::joinPath(const std::string& base, const std::string& relative) {
    std::filesystem::path p1(base);
    std::filesystem::path p2(relative);
    return (p1 / p2).string();
}

std::string FileUtils::getRelativePath(const std::string& base, const std::string& path) {
    std::error_code ec;
    auto relative = std::filesystem::relative(path, base, ec);
    return ec ? path : relative.string();
}

std::string FileUtils::getAbsolutePath(const std::string& path) {
    std::error_code ec;
    auto absolute = std::filesystem::absolute(path, ec);
    return ec ? path : absolute.string();
}

std::string FileUtils::getCurrentDirectory() {
    std::error_code ec;
    auto current = std::filesystem::current_path(ec);
    return ec ? "" : current.string();
}

bool FileUtils::setCurrentDirectory(const std::string& path) {
    std::error_code ec;
    std::filesystem::current_path(path, ec);
    return !ec;
}

bool FileUtils::matchesPattern(const std::string& filename, const std::string& pattern) {
    if (!isWildcardPattern(pattern)) {
        return filename == pattern;
    }
    
    return wildcardMatch(filename, pattern);
}

std::vector<std::string> FileUtils::findFiles(const std::string& path, const std::string& pattern, bool recursive) {
    auto fileInfos = listDirectory(path, recursive);
    std::vector<std::string> matches;
    
    for (const auto& info : fileInfos) {
        if (info.isRegularFile && matchesPattern(getFileName(info.path), pattern)) {
            matches.push_back(info.path);
        }
    }
    
    return matches;
}

std::string FileUtils::formatFileSize(uint64_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size_d = static_cast<double>(size);
    
    while (size_d >= 1024.0 && unit < 4) {
        size_d /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << size_d << " " << units[unit];
    return oss.str();
}

std::string FileUtils::formatTimestamp(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool FileUtils::isValidPath(const std::string& path) {
    if (path.empty() || path.length() > MAX_PATH_LENGTH) {
        return false;
    }
    
    try {
        std::filesystem::path p(path);
        return !p.empty();
    } catch (...) {
        return false;
    }
}

bool FileUtils::isValidFileName(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "<>:\"|?*";
    for (char c : invalidChars) {
        if (filename.find(c) != std::string::npos) {
            return false;
        }
    }
    
    return true;
}

std::string FileUtils::createTempFile(const std::string& prefix, const std::string& suffix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string filename;
    
    do {
        filename = prefix + std::to_string(dis(gen)) + suffix;
    } while (exists(joinPath(tempDir, filename)));
    
    return joinPath(tempDir, filename);
}

std::string FileUtils::createTempDirectory(const std::string& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string dirname;
    
    do {
        dirname = prefix + std::to_string(dis(gen));
    } while (exists(joinPath(tempDir, dirname)));
    
    std::string fullPath = joinPath(tempDir, dirname);
    if (createDirectory(fullPath)) {
        return fullPath;
    }
    
    return "";
}

bool FileUtils::isTempPath(const std::string& path) {
    std::string tempDir = std::filesystem::temp_directory_path().string();
    return path.find(tempDir) == 0;
}

// Private helper methods
std::vector<FileUtils::FileInfo> FileUtils::listDirectoryRecursive(const std::string& path, const std::string& relativeBase) {
    std::vector<FileInfo> files;
    
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path, ec)) {
        FileInfo info = getFileInfo(entry.path().string());
        info.relativePath = getRelativePath(path, info.path);
        files.push_back(info);
    }
    
    return files;
}

bool FileUtils::handleOverwrite(const std::string& destination, OverwriteMode mode) {
    switch (mode) {
        case OverwriteMode::OVERWRITE:
            return true;
        case OverwriteMode::SKIP:
            return false;
        case OverwriteMode::RENAME:
            return writeFile(generateUniqueFileName(destination), std::vector<uint8_t>());
        case OverwriteMode::ASK:
        default:
            std::cout << "File '" << destination << "' already exists. Overwrite? (y/n): ";
            char response;
            std::cin >> response;
            return (response == 'y' || response == 'Y');
    }
}

std::string FileUtils::generateUniqueFileName(const std::string& path) {
    std::filesystem::path p(path);
    std::string stem = p.stem().string();
    std::string extension = p.extension().string();
    std::string parent = p.parent_path().string();
    
    int counter = 1;
    std::string newPath;
    
    do {
        newPath = joinPath(parent, stem + "_" + std::to_string(counter) + extension);
        counter++;
    } while (exists(newPath));
    
    return newPath;
}

bool FileUtils::isWildcardPattern(const std::string& pattern) {
    return pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos;
}

bool FileUtils::wildcardMatch(const std::string& str, const std::string& pattern) {
    // Simple wildcard matching implementation
    size_t i = 0, j = 0;
    size_t starIdx = -1, match = 0;
    
    while (i < str.length()) {
        if (j < pattern.length() && (pattern[j] == '?' || pattern[j] == str[i])) {
            i++;
            j++;
        } else if (j < pattern.length() && pattern[j] == '*') {
            starIdx = j;
            match = i;
            j++;
        } else if (starIdx != -1) {
            j = starIdx + 1;
            match++;
            i = match;
        } else {
            return false;
        }
    }
    
    while (j < pattern.length() && pattern[j] == '*') {
        j++;
    }
    
    return j == pattern.length();
}