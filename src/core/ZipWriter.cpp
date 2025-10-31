#include "ZipWriter.h"
#include "../utils/FileUtils.h"
#include "../compression/CompressionEngine.h"
#include <iostream>

ZipWriter::ZipWriter() : isOpen_(false), centralDirOffset_(0), centralDirSize_(0) {
}

ZipWriter::~ZipWriter() {
    close();
}

bool ZipWriter::open(const std::string& filename) {
    if (isOpen_) {
        close();
    }

    filename_ = filename;
    file_.open(filename, std::ios::binary);
    
    if (!file_.is_open()) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        return false;
    }

    isOpen_ = true;
    entries_.clear();
    centralDirOffset_ = 0;
    centralDirSize_ = 0;

    return true;
}

void ZipWriter::close() {
    if (file_.is_open()) {
        file_.close();
    }
    isOpen_ = false;
}

bool ZipWriter::addFile(const std::string& filePath, const std::string& archivePath) {
    if (!isOpen_) {
        std::cerr << "Error: Writer is not open" << std::endl;
        return false;
    }

    if (!FileUtils::exists(filePath) || !FileUtils::isFile(filePath)) {
        std::cerr << "Error: File not found: " << filePath << std::endl;
        return false;
    }

    // Read file data
    auto fileData = FileUtils::readFile(filePath);
    if (fileData.empty()) {
        std::cerr << "Error: Cannot read file: " << filePath << std::endl;
        return false;
    }

    // Create entry
    std::string entryName = archivePath.empty() ? FileUtils::getFileName(filePath) : archivePath;
    auto entry = std::make_shared<ZipEntry>(entryName);
    
    // Set file metadata
    auto fileInfo = FileUtils::getFileInfo(filePath);
    entry->setUncompressedSize(fileInfo.size);
    entry->setLastModifiedTime(fileInfo.lastModified);
    entry->setDirectory(false);

    // Compress data
    CompressionEngine engine;
    auto result = engine.compress(fileData);
    
    if (result.success) {
        entry->setCompressedData(result.compressedData);
        entry->setCompressedSize(result.compressedData.size());
        entry->setCRC32(result.crc32);
        entry->setCompressionMethod(ZipEntry::CompressionMethod::DEFLATED);
    } else {
        // Fallback to stored method
        entry->setCompressedData(fileData);
        entry->setCompressedSize(fileData.size());
        entry->setCRC32(CompressionEngine::calculateCRC32(fileData));
        entry->setCompressionMethod(ZipEntry::CompressionMethod::STORED);
    }

    // Set local header offset
    entry->setLocalHeaderOffset(getCurrentPosition());

    // Write local header and data
    if (!writeLocalHeader(entry)) {
        return false;
    }

    if (!writeCompressedData(entry)) {
        return false;
    }

    entries_.push_back(entry);
    return true;
}

bool ZipWriter::addDirectory(const std::string& dirPath, const std::string& archivePath) {
    if (!isOpen_) {
        std::cerr << "Error: Writer is not open" << std::endl;
        return false;
    }

    if (!FileUtils::exists(dirPath) || !FileUtils::isDirectory(dirPath)) {
        std::cerr << "Error: Directory not found: " << dirPath << std::endl;
        return false;
    }

    // Create directory entry
    std::string entryName = archivePath.empty() ? FileUtils::getFileName(dirPath) : archivePath;
    if (entryName.back() != '/') {
        entryName += '/';
    }

    auto entry = std::make_shared<ZipEntry>(entryName);
    entry->setDirectory(true);
    entry->setUncompressedSize(0);
    entry->setCompressedSize(0);
    entry->setCRC32(0);
    entry->setCompressionMethod(ZipEntry::CompressionMethod::STORED);
    
    // Set directory metadata
    auto fileInfo = FileUtils::getFileInfo(dirPath);
    entry->setLastModifiedTime(fileInfo.lastModified);
    entry->setLocalHeaderOffset(getCurrentPosition());

    // Write local header (no data for directories)
    if (!writeLocalHeader(entry)) {
        return false;
    }

    entries_.push_back(entry);
    return true;
}

bool ZipWriter::addEntry(const std::shared_ptr<ZipEntry>& entry) {
    if (!isOpen_ || !entry) {
        return false;
    }

    entry->setLocalHeaderOffset(getCurrentPosition());

    if (!writeLocalHeader(entry)) {
        return false;
    }

    if (!writeCompressedData(entry)) {
        return false;
    }

    entries_.push_back(entry);
    return true;
}

bool ZipWriter::writeCentralDirectory() {
    if (!isOpen_) {
        return false;
    }

    centralDirOffset_ = getCurrentPosition();

    // Write central directory entries
    for (const auto& entry : entries_) {
        if (!writeCentralDirectoryEntry(entry)) {
            return false;
        }
    }

    centralDirSize_ = getCurrentPosition() - centralDirOffset_;
    return true;
}

bool ZipWriter::finalize() {
    if (!isOpen_) {
        return false;
    }

    // Write central directory
    if (!writeCentralDirectory()) {
        return false;
    }

    // Write end of central directory
    if (!writeEndOfCentralDirectory()) {
        return false;
    }

    return true;
}

// Private implementation methods
bool ZipWriter::writeLocalHeader(const std::shared_ptr<ZipEntry>& entry) {
    if (!entry) return false;

    // Write signature
    writeUint32(LOCAL_HEADER_SIGNATURE);
    
    // Version needed to extract
    writeUint16(20);
    
    // General purpose bit flag
    writeUint16(static_cast<uint16_t>(entry->getGeneralPurposeFlag()));
    
    // Compression method
    writeUint16(static_cast<uint16_t>(entry->getCompressionMethod()));
    
    // Last mod time and date
    writeUint16(entry->getLastModTime());
    writeUint16(entry->getLastModDate());
    
    // CRC32
    writeUint32(entry->getCRC32());
    
    // Compressed size
    writeUint32(static_cast<uint32_t>(entry->getCompressedSize()));
    
    // Uncompressed size
    writeUint32(static_cast<uint32_t>(entry->getUncompressedSize()));
    
    // File name length
    writeUint16(static_cast<uint16_t>(entry->getName().length()));
    
    // Extra field length
    writeUint16(0);
    
    // File name
    writeString(entry->getName());

    return file_.good();
}

bool ZipWriter::writeCompressedData(const std::shared_ptr<ZipEntry>& entry) {
    if (!entry) return false;

    const auto& data = entry->getCompressedData();
    writeBytes(data);
    
    return file_.good();
}

bool ZipWriter::writeCentralDirectoryEntry(const std::shared_ptr<ZipEntry>& entry) {
    if (!entry) return false;

    // Write signature
    writeUint32(CENTRAL_DIR_SIGNATURE);
    
    // Version made by
    writeUint16(20);
    
    // Version needed to extract
    writeUint16(20);
    
    // General purpose bit flag
    writeUint16(static_cast<uint16_t>(entry->getGeneralPurposeFlag()));
    
    // Compression method
    writeUint16(static_cast<uint16_t>(entry->getCompressionMethod()));
    
    // Last mod time and date
    writeUint16(entry->getLastModTime());
    writeUint16(entry->getLastModDate());
    
    // CRC32
    writeUint32(entry->getCRC32());
    
    // Compressed size
    writeUint32(static_cast<uint32_t>(entry->getCompressedSize()));
    
    // Uncompressed size
    writeUint32(static_cast<uint32_t>(entry->getUncompressedSize()));
    
    // File name length
    writeUint16(static_cast<uint16_t>(entry->getName().length()));
    
    // Extra field length
    writeUint16(0);
    
    // File comment length
    writeUint16(0);
    
    // Disk number start
    writeUint16(0);
    
    // Internal file attributes
    writeUint16(entry->isDirectory() ? 0x10 : 0);
    
    // External file attributes
    writeUint32(entry->isDirectory() ? 0x10 : 0x20);
    
    // Relative offset of local header
    writeUint32(entry->getLocalHeaderOffset());
    
    // File name
    writeString(entry->getName());

    return file_.good();
}

bool ZipWriter::writeEndOfCentralDirectory() {
    // Write signature
    writeUint32(EOCD_SIGNATURE);
    
    // Disk number
    writeUint16(0);
    
    // Central dir disk number
    writeUint16(0);
    
    // Disk entries
    writeUint16(static_cast<uint16_t>(entries_.size()));
    
    // Total entries
    writeUint16(static_cast<uint16_t>(entries_.size()));
    
    // Central dir size
    writeUint32(centralDirSize_);
    
    // Central dir offset
    writeUint32(centralDirOffset_);
    
    // Comment length
    writeUint16(0);

    return file_.good();
}

// Utility methods
bool ZipWriter::seekTo(uint32_t position) {
    file_.seekp(position);
    return file_.good();
}

uint32_t ZipWriter::getCurrentPosition() {
    return static_cast<uint32_t>(file_.tellp());
}

void ZipWriter::writeUint32(uint32_t value) {
    file_.write(reinterpret_cast<const char*>(&value), 4);
}

void ZipWriter::writeUint16(uint16_t value) {
    file_.write(reinterpret_cast<const char*>(&value), 2);
}

void ZipWriter::writeBytes(const std::vector<uint8_t>& data) {
    file_.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ZipWriter::writeString(const std::string& str) {
    file_.write(str.c_str(), str.length());
}