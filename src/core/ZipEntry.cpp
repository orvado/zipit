#include "ZipEntry.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>

ZipEntry::ZipEntry()
    : crc32_(0)
    , compressedSize_(0)
    , uncompressedSize_(0)
    , compressionMethod_(CompressionMethod::STORED)
    , generalPurposeFlag_(GeneralPurposeFlag::NONE)
    , lastModTime_(0)
    , lastModDate_(0)
    , localHeaderOffset_(0)
    , offset_(0)
    , isDirectory_(false) {
}

ZipEntry::ZipEntry(const std::string& name)
    : name_(name)
    , crc32_(0)
    , compressedSize_(0)
    , uncompressedSize_(0)
    , compressionMethod_(CompressionMethod::STORED)
    , generalPurposeFlag_(GeneralPurposeFlag::NONE)
    , lastModTime_(0)
    , lastModDate_(0)
    , localHeaderOffset_(0)
    , offset_(0)
    , isDirectory_(name_.empty() ? false : name_.back() == '/') {
}

std::chrono::system_clock::time_point ZipEntry::getLastModifiedTime() const {
    std::tm tm = {};
    tm.tm_year = ((lastModDate_ >> 9) & 0x7F) + 80;
    tm.tm_mon = ((lastModDate_ >> 5) & 0x0F) - 1;
    tm.tm_mday = lastModDate_ & 0x1F;
    tm.tm_hour = (lastModTime_ >> 11) & 0x1F;
    tm.tm_min = (lastModTime_ >> 5) & 0x3F;
    tm.tm_sec = (lastModTime_ & 0x1F) * 2;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void ZipEntry::setLastModifiedTime(const std::chrono::system_clock::time_point& time) {
    std::time_t time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::localtime(&time_t);
    
    lastModDate_ = ((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday;
    lastModTime_ = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2);
}

double ZipEntry::getCompressionRatio() const {
    if (uncompressedSize_ == 0) return 0.0;
    return (1.0 - static_cast<double>(compressedSize_) / uncompressedSize_) * 100.0;
}

std::string ZipEntry::getFormattedSize() const {
    return formatSize(uncompressedSize_);
}

std::string ZipEntry::getFormattedCompressedSize() const {
    return formatSize(compressedSize_);
}

std::string ZipEntry::getFormattedCompressionRatio() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << getCompressionRatio() << "%";
    return oss.str();
}

bool ZipEntry::isValid() const {
    return !name_.empty() && 
           (compressionMethod_ == CompressionMethod::STORED || 
            compressionMethod_ == CompressionMethod::DEFLATED);
}

void ZipEntry::clear() {
    name_.clear();
    crc32_ = 0;
    compressedSize_ = 0;
    uncompressedSize_ = 0;
    compressionMethod_ = CompressionMethod::STORED;
    generalPurposeFlag_ = GeneralPurposeFlag::NONE;
    lastModTime_ = 0;
    lastModDate_ = 0;
    localHeaderOffset_ = 0;
    offset_ = 0;
    isDirectory_ = false;
    compressedData_.clear();
    uncompressedData_.clear();
}

std::vector<uint8_t> ZipEntry::serializeLocalHeader() const {
    std::vector<uint8_t> header;
    
    // Local file header signature
    writeUint32(header, LOCAL_HEADER_SIGNATURE);
    
    // Version needed to extract
    writeUint16(header, 20);
    
    // General purpose bit flag
    writeUint16(header, static_cast<uint16_t>(generalPurposeFlag_));
    
    // Compression method
    writeUint16(header, static_cast<uint16_t>(compressionMethod_));
    
    // Last mod time and date
    writeUint16(header, lastModTime_);
    writeUint16(header, lastModDate_);
    
    // CRC32
    writeUint32(header, crc32_);
    
    // Compressed size
    writeUint32(header, static_cast<uint32_t>(compressedSize_));
    
    // Uncompressed size
    writeUint32(header, static_cast<uint32_t>(uncompressedSize_));
    
    // File name length
    writeUint16(header, static_cast<uint16_t>(name_.length()));
    
    // Extra field length
    writeUint16(header, 0);
    
    // File name
    header.insert(header.end(), name_.begin(), name_.end());
    
    return header;
}

std::vector<uint8_t> ZipEntry::serializeCentralDirectoryHeader() const {
    std::vector<uint8_t> header;
    
    // Central directory file header signature
    writeUint32(header, CENTRAL_DIRECTORY_SIGNATURE);
    
    // Version made by
    writeUint16(header, 20);
    
    // Version needed to extract
    writeUint16(header, 20);
    
    // General purpose bit flag
    writeUint16(header, static_cast<uint16_t>(generalPurposeFlag_));
    
    // Compression method
    writeUint16(header, static_cast<uint16_t>(compressionMethod_));
    
    // Last mod time and date
    writeUint16(header, lastModTime_);
    writeUint16(header, lastModDate_);
    
    // CRC32
    writeUint32(header, crc32_);
    
    // Compressed size
    writeUint32(header, static_cast<uint32_t>(compressedSize_));
    
    // Uncompressed size
    writeUint32(header, static_cast<uint32_t>(uncompressedSize_));
    
    // File name length
    writeUint16(header, static_cast<uint16_t>(name_.length()));
    
    // Extra field length
    writeUint16(header, 0);
    
    // File comment length
    writeUint16(header, 0);
    
    // Disk number start
    writeUint16(header, 0);
    
    // Internal file attributes
    writeUint16(header, isDirectory_ ? 0x10 : 0);
    
    // External file attributes
    writeUint32(header, isDirectory_ ? 0x10 : 0x20);
    
    // Relative offset of local header
    writeUint32(header, localHeaderOffset_);
    
    // File name
    header.insert(header.end(), name_.begin(), name_.end());
    
    return header;
}

bool ZipEntry::deserializeLocalHeader(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 30 > data.size()) return false;
    
    // Check signature
    if (readUint32(data, offset) != LOCAL_HEADER_SIGNATURE) return false;
    
    // Read fields
    uint16_t versionNeeded = readUint16(data, offset + 4);
    generalPurposeFlag_ = static_cast<GeneralPurposeFlag>(readUint16(data, offset + 6));
    compressionMethod_ = static_cast<CompressionMethod>(readUint16(data, offset + 8));
    lastModTime_ = readUint16(data, offset + 10);
    lastModDate_ = readUint16(data, offset + 12);
    crc32_ = readUint32(data, offset + 14);
    compressedSize_ = readUint32(data, offset + 18);
    uncompressedSize_ = readUint32(data, offset + 22);
    uint16_t nameLength = readUint16(data, offset + 26);
    uint16_t extraLength = readUint16(data, offset + 28);
    
    // Read file name
    if (offset + 30 + nameLength > data.size()) return false;
    name_.assign(reinterpret_cast<const char*>(data.data() + offset + 30), nameLength);
    
    isDirectory_ = !name_.empty() && name_.back() == '/';
    
    return true;
}

bool ZipEntry::deserializeCentralDirectoryHeader(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 46 > data.size()) return false;
    
    // Check signature
    if (readUint32(data, offset) != CENTRAL_DIRECTORY_SIGNATURE) return false;
    
    // Read fields
    uint16_t versionMadeBy = readUint16(data, offset + 4);
    uint16_t versionNeeded = readUint16(data, offset + 6);
    generalPurposeFlag_ = static_cast<GeneralPurposeFlag>(readUint16(data, offset + 8));
    compressionMethod_ = static_cast<CompressionMethod>(readUint16(data, offset + 10));
    lastModTime_ = readUint16(data, offset + 12);
    lastModDate_ = readUint16(data, offset + 14);
    crc32_ = readUint32(data, offset + 18);
    compressedSize_ = readUint32(data, offset + 22);
    uncompressedSize_ = readUint32(data, offset + 26);
    uint16_t nameLength = readUint16(data, offset + 28);
    uint16_t extraLength = readUint16(data, offset + 30);
    uint16_t commentLength = readUint16(data, offset + 32);
    uint16_t diskNumberStart = readUint16(data, offset + 34);
    uint16_t internalAttributes = readUint16(data, offset + 36);
    uint32_t externalAttributes = readUint32(data, offset + 38);
    localHeaderOffset_ = readUint32(data, offset + 42);
    
    // Read file name
    if (offset + 46 + nameLength > data.size()) return false;
    name_.assign(reinterpret_cast<const char*>(data.data() + offset + 46), nameLength);
    
    isDirectory_ = !name_.empty() && name_.back() == '/';
    
    return true;
}

// Private helper methods
std::string ZipEntry::formatSize(uint64_t size) const {
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

void ZipEntry::writeUint16(std::vector<uint8_t>& data, uint16_t value) const {
    data.push_back(static_cast<uint8_t>(value & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void ZipEntry::writeUint32(std::vector<uint8_t>& data, uint32_t value) const {
    data.push_back(static_cast<uint8_t>(value & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

uint16_t ZipEntry::readUint16(const std::vector<uint8_t>& data, size_t offset) const {
    return static_cast<uint16_t>(data[offset]) |
           (static_cast<uint16_t>(data[offset + 1]) << 8);
}

uint32_t ZipEntry::readUint32(const std::vector<uint8_t>& data, size_t offset) const {
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}