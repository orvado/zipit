#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

class ZipEntry {
public:
    enum class CompressionMethod : uint16_t {
        STORED = 0,
        DEFLATED = 8
    };

    enum class GeneralPurposeFlag : uint16_t {
        NONE = 0,
        ENCRYPTED = 1,
        DATA_DESCRIPTOR = 8,
        UTF8_NAMES = 2048
    };

private:
    std::string name_;
    uint32_t crc32_;
    uint64_t compressedSize_;
    uint64_t uncompressedSize_;
    CompressionMethod compressionMethod_;
    GeneralPurposeFlag generalPurposeFlag_;
    uint16_t lastModTime_;
    uint16_t lastModDate_;
    uint32_t localHeaderOffset_;
    uint64_t offset_;
    bool isDirectory_;
    std::vector<uint8_t> compressedData_;
    std::vector<uint8_t> uncompressedData_;
    std::string archivePath_;

public:
    ZipEntry();
    explicit ZipEntry(const std::string& name);

    // Getters
    const std::string& getName() const { return name_; }
    uint32_t getCRC32() const { return crc32_; }
    uint64_t getCompressedSize() const { return compressedSize_; }
    uint64_t getUncompressedSize() const { return uncompressedSize_; }
    CompressionMethod getCompressionMethod() const { return compressionMethod_; }
    GeneralPurposeFlag getGeneralPurposeFlag() const { return generalPurposeFlag_; }
    uint16_t getLastModTime() const { return lastModTime_; }
    uint16_t getLastModDate() const { return lastModDate_; }
    uint32_t getLocalHeaderOffset() const { return localHeaderOffset_; }
    uint64_t getOffset() const { return offset_; }
    bool isDirectory() const { return isDirectory_; }
    const std::vector<uint8_t>& getCompressedData() const { return compressedData_; }
    const std::vector<uint8_t>& getUncompressedData() const { return uncompressedData_; }
    
    // Archive information
    void setArchivePath(const std::string& path) { archivePath_ = path; }
    const std::string& getArchivePath() const { return archivePath_; }

    // Setters
    void setName(const std::string& name) { name_ = name; }
    void setCRC32(uint32_t crc32) { crc32_ = crc32; }
    void setCompressedSize(uint64_t size) { compressedSize_ = size; }
    void setUncompressedSize(uint64_t size) { uncompressedSize_ = size; }
    void setCompressionMethod(CompressionMethod method) { compressionMethod_ = method; }
    void setGeneralPurposeFlag(GeneralPurposeFlag flag) { generalPurposeFlag_ = flag; }
    void setLastModTime(uint16_t time) { lastModTime_ = time; }
    void setLastModDate(uint16_t date) { lastModDate_ = date; }
    void setLocalHeaderOffset(uint32_t offset) { localHeaderOffset_ = offset; }
    void setOffset(uint64_t offset) { offset_ = offset; }
    void setDirectory(bool isDir) { isDirectory_ = isDir; }
    void setCompressedData(const std::vector<uint8_t>& data) { compressedData_ = data; }
    void setUncompressedData(const std::vector<uint8_t>& data) { uncompressedData_ = data; }

    // Utility methods
    std::chrono::system_clock::time_point getLastModifiedTime() const;
    void setLastModifiedTime(const std::chrono::system_clock::time_point& time);
    
    double getCompressionRatio() const;
    std::string getFormattedSize() const;
    std::string getFormattedCompressedSize() const;
    std::string getFormattedCompressionRatio() const;
    
    bool isValid() const;
    void clear();

    // Serialization methods
    std::vector<uint8_t> serializeLocalHeader() const;
    std::vector<uint8_t> serializeCentralDirectoryHeader() const;
    bool deserializeLocalHeader(const std::vector<uint8_t>& data, size_t offset);
    bool deserializeCentralDirectoryHeader(const std::vector<uint8_t>& data, size_t offset);

    // Constants
    static constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
    static constexpr uint32_t CENTRAL_DIRECTORY_SIGNATURE = 0x02014b50;
    static constexpr uint32_t END_OF_CENTRAL_DIR_SIGNATURE = 0x06054b50;

private:
    // Helper methods
    std::string formatSize(uint64_t size) const;
    void writeUint16(std::vector<uint8_t>& data, uint16_t value) const;
    void writeUint32(std::vector<uint8_t>& data, uint32_t value) const;
    uint16_t readUint16(const std::vector<uint8_t>& data, size_t offset) const;
    uint32_t readUint32(const std::vector<uint8_t>& data, size_t offset) const;
};