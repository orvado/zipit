#pragma once

#include "ZipEntry.h"
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <cstdint>

struct EndOfCentralDirectoryRecord {
    uint32_t signature;
    uint16_t diskNumber;
    uint16_t centralDirDiskNumber;
    uint16_t diskEntries;
    uint16_t totalEntries;
    uint32_t centralDirSize;
    uint32_t centralDirOffset;
    uint16_t commentLength;
    std::string comment;
};

struct CentralDirectoryEntry {
    uint32_t signature;
    uint16_t versionMadeBy;
    uint16_t versionNeeded;
    uint16_t generalPurposeFlag;
    uint16_t compressionMethod;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t nameLength;
    uint16_t extraLength;
    uint16_t commentLength;
    uint16_t diskNumberStart;
    uint16_t internalAttributes;
    uint32_t externalAttributes;
    uint32_t localHeaderOffset;
    std::string name;
    std::vector<uint8_t> extra;
    std::string comment;
};

class ZipReader {
private:
    std::string filename_;
    bool isOpen_;
    std::ifstream file_;
    std::vector<std::shared_ptr<ZipEntry>> entries_;
    EndOfCentralDirectoryRecord eocdRecord_;

    // Internal parsing methods
    bool findEndOfCentralDirectory();
    bool readEndOfCentralDirectory();
    bool readCentralDirectory();
    bool readCentralDirectoryEntry(uint32_t offset, CentralDirectoryEntry& entry);
    bool readLocalHeader(const std::shared_ptr<ZipEntry>& entry);
    bool readEntryData(const std::shared_ptr<ZipEntry>& entry, std::vector<uint8_t>& data);
    
    // Utility methods
    uint32_t readUint32(std::ifstream& file);
    uint16_t readUint16(std::ifstream& file);
    uint8_t readUint8(std::ifstream& file);
    std::string readString(std::ifstream& file, size_t length);
    std::vector<uint8_t> readBytes(std::ifstream& file, size_t length);
    bool seekTo(uint32_t position);
    uint32_t getCurrentPosition();

public:
    ZipReader();
    ~ZipReader();

    // Reader operations
    bool open(const std::string& filename);
    void close();
    bool isOpen() const { return isOpen_; }

    // Reading operations
    std::vector<std::shared_ptr<ZipEntry>> readEntries();
    bool readEntryData(const std::shared_ptr<ZipEntry>& entry);
    bool validateArchive();
    
    // Information methods
    size_t getEntryCount() const { return entries_.size(); }
    std::shared_ptr<ZipEntry> findEntry(const std::string& name) const;
    const std::vector<std::shared_ptr<ZipEntry>>& getEntries() const { return entries_; }
    
    // Validation methods
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        size_t totalEntries;
        size_t validEntries;
        uint64_t totalUncompressedSize;
        uint64_t totalCompressedSize;
        double overallCompressionRatio;
    };
    
    ValidationResult validateArchiveDetailed();
    bool validateEntry(const std::shared_ptr<ZipEntry>& entry, std::string& errorMessage);
    
    // Constants
    static constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
    static constexpr uint32_t CENTRAL_DIR_SIGNATURE = 0x02014b50;
    static constexpr uint32_t EOCD_SIGNATURE = 0x06054b50;
    static constexpr uint32_t ZIP64_EOCD_SIGNATURE = 0x06064b50;
    static constexpr uint32_t ZIP64_EOCD_LOCATOR_SIGNATURE = 0x07064b50;
    static constexpr size_t MAX_COMMENT_LENGTH = 65535;
    static constexpr size_t EOCD_BASE_SIZE = 22;
    static constexpr size_t MAX_EOCD_SEARCH_SIZE = 65557; // EOCD + comment + signature
};