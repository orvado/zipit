#pragma once

#include "ZipEntry.h"
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <cstdint>

class ZipWriter {
private:
    std::string filename_;
    bool isOpen_;
    std::ofstream file_;
    std::vector<std::shared_ptr<ZipEntry>> entries_;
    uint32_t centralDirOffset_;
    uint32_t centralDirSize_;

    // Internal writing methods
    bool writeLocalHeader(const std::shared_ptr<ZipEntry>& entry);
    bool writeCompressedData(const std::shared_ptr<ZipEntry>& entry);
    bool writeCentralDirectoryEntry(const std::shared_ptr<ZipEntry>& entry);
    bool writeEndOfCentralDirectory();
    
    // Utility methods
    bool seekTo(uint32_t position);
    uint32_t getCurrentPosition();
    void writeUint32(uint32_t value);
    void writeUint16(uint16_t value);
    void writeBytes(const std::vector<uint8_t>& data);
    void writeString(const std::string& str);

public:
    ZipWriter();
    ~ZipWriter();

    // Writer operations
    bool open(const std::string& filename);
    void close();
    bool isOpen() const { return isOpen_; }

    // Writing operations
    bool addFile(const std::string& filePath, const std::string& archivePath = "");
    bool addDirectory(const std::string& dirPath, const std::string& archivePath = "");
    bool addEntry(const std::shared_ptr<ZipEntry>& entry);
    bool writeCentralDirectory();
    bool finalize();
    
    // Information methods
    size_t getEntryCount() const { return entries_.size(); }
    const std::vector<std::shared_ptr<ZipEntry>>& getEntries() const { return entries_; }
    
    // Constants
    static constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
    static constexpr uint32_t CENTRAL_DIR_SIGNATURE = 0x02014b50;
    static constexpr uint32_t EOCD_SIGNATURE = 0x06054b50;
};