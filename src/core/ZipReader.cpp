#include "ZipReader.h"
#include "../compression/CompressionEngine.h"
#include <iostream>
#include <algorithm>

ZipReader::ZipReader() : isOpen_(false) {
}

ZipReader::~ZipReader() {
    close();
}

bool ZipReader::open(const std::string& filename) {
    if (isOpen_) {
        close();
    }

    filename_ = filename;
    file_.open(filename, std::ios::binary);
    
    if (!file_.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    isOpen_ = true;
    entries_.clear();

    // Find and read end of central directory
    if (!findEndOfCentralDirectory()) {
        std::cerr << "Error: Cannot find end of central directory" << std::endl;
        close();
        return false;
    }

    if (!readEndOfCentralDirectory()) {
        std::cerr << "Error: Cannot read end of central directory" << std::endl;
        close();
        return false;
    }

    // Read central directory
    if (!readCentralDirectory()) {
        std::cerr << "Error: Cannot read central directory" << std::endl;
        close();
        return false;
    }

    return true;
}

void ZipReader::close() {
    if (file_.is_open()) {
        file_.close();
    }
    isOpen_ = false;
    entries_.clear();
}

std::vector<std::shared_ptr<ZipEntry>> ZipReader::readEntries() {
    if (!isOpen_) {
        return {};
    }

    // Read local headers for all entries
    for (auto& entry : entries_) {
        if (!readLocalHeader(entry)) {
            std::cerr << "Warning: Cannot read local header for " << entry->getName() << std::endl;
        }
    }

    return entries_;
}

bool ZipReader::readEntryData(const std::shared_ptr<ZipEntry>& entry) {
    if (!isOpen_ || !entry) {
        return false;
    }

    std::vector<uint8_t> data;
    if (!readEntryData(entry, data)) {
        return false;
    }

    entry->setCompressedData(data);
    return true;
}

bool ZipReader::validateArchive() {
    auto result = validateArchiveDetailed();
    return result.isValid;
}

ZipReader::ValidationResult ZipReader::validateArchiveDetailed() {
    ValidationResult result;
    result.isValid = true;
    result.totalEntries = entries_.size();
    result.validEntries = 0;
    result.totalUncompressedSize = 0;
    result.totalCompressedSize = 0;
    result.overallCompressionRatio = 0.0;

    if (!isOpen_) {
        result.isValid = false;
        result.errors.push_back("Archive is not open");
        return result;
    }

    // Validate end of central directory record
    if (eocdRecord_.signature != EOCD_SIGNATURE) {
        result.isValid = false;
        result.errors.push_back("Invalid end of central directory signature");
    }

    if (eocdRecord_.diskNumber != 0 || eocdRecord_.centralDirDiskNumber != 0) {
        result.warnings.push_back("Multi-disk archives are not fully supported");
    }

    if (eocdRecord_.diskEntries != eocdRecord_.totalEntries) {
        result.warnings.push_back("Disk entries count differs from total entries count");
    }

    // Validate each entry
    for (const auto& entry : entries_) {
        std::string errorMessage;
        if (validateEntry(entry, errorMessage)) {
            result.validEntries++;
            result.totalUncompressedSize += entry->getUncompressedSize();
            result.totalCompressedSize += entry->getCompressedSize();
        } else {
            result.isValid = false;
            result.errors.push_back(errorMessage);
        }
    }

    // Calculate overall compression ratio
    if (result.totalUncompressedSize > 0) {
        result.overallCompressionRatio = 
            (1.0 - static_cast<double>(result.totalCompressedSize) / result.totalUncompressedSize) * 100.0;
    }

    return result;
}

bool ZipReader::validateEntry(const std::shared_ptr<ZipEntry>& entry, std::string& errorMessage) {
    if (!entry) {
        errorMessage = "Null entry pointer";
        return false;
    }

    // Validate entry name
    if (entry->getName().empty()) {
        errorMessage = "Entry has empty name";
        return false;
    }

    // Validate compression method
    if (entry->getCompressionMethod() != ZipEntry::CompressionMethod::STORED &&
        entry->getCompressionMethod() != ZipEntry::CompressionMethod::DEFLATED) {
        errorMessage = "Unsupported compression method for entry: " + entry->getName();
        return false;
    }

    // Validate sizes
    if (entry->getCompressedSize() > 0 && entry->getUncompressedSize() == 0) {
        errorMessage = "Compressed size > 0 but uncompressed size = 0 for entry: " + entry->getName();
        return false;
    }

    // Try to read local header
    if (!readLocalHeader(entry)) {
        errorMessage = "Cannot read local header for entry: " + entry->getName();
        return false;
    }

    // Validate data integrity for non-empty files
    if (entry->getUncompressedSize() > 0 && entry->getUncompressedSize() < 100000000) { // Limit to 100MB for validation
        std::vector<uint8_t> compressedData;
        if (!readEntryData(entry, compressedData)) {
            errorMessage = "Cannot read data for entry: " + entry->getName();
            return false;
        }

        // Decompress data if needed
        std::vector<uint8_t> uncompressedData;
        if (entry->getCompressionMethod() == ZipEntry::CompressionMethod::DEFLATED) {
#ifdef ZIPIT_NO_ZLIB
            // Skip CRC32 validation when zlib is not available
            return true;
#else
            CompressionEngine engine;
            auto result = engine.decompress(compressedData, entry->getUncompressedSize());
            if (!result.success) {
                errorMessage = "Cannot decompress data for entry: " + entry->getName() + " - " + result.errorMessage;
                return false;
            }
            uncompressedData = result.decompressedData;
#endif
        } else {
            uncompressedData = compressedData;
        }

#ifndef ZIPIT_NO_ZLIB
        // Validate CRC32
        uint32_t calculatedCrc = CompressionEngine::calculateCRC32(uncompressedData);
        if (calculatedCrc != entry->getCRC32()) {
            errorMessage = "CRC32 mismatch for entry: " + entry->getName() + 
                         " (expected: " + std::to_string(entry->getCRC32()) + 
                         ", calculated: " + std::to_string(calculatedCrc) + ")";
            return false;
        }
#endif
    }

    return true;
}

std::shared_ptr<ZipEntry> ZipReader::findEntry(const std::string& name) const {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [&name](const std::shared_ptr<ZipEntry>& entry) {
            return entry->getName() == name;
        });
    
    return (it != entries_.end()) ? *it : nullptr;
}

// Private implementation methods
bool ZipReader::findEndOfCentralDirectory() {
    if (!file_.seekg(0, std::ios::end)) {
        return false;
    }

    uint64_t fileSize = file_.tellg();
    if (fileSize < EOCD_BASE_SIZE) {
        return false;
    }

    // Search for EOCD signature from the end
    uint64_t searchSize = std::min(static_cast<uint64_t>(MAX_EOCD_SEARCH_SIZE), fileSize);
    uint64_t searchStart = fileSize - searchSize;

    file_.seekg(searchStart);
    std::vector<uint8_t> buffer(searchSize);
    file_.read(reinterpret_cast<char*>(buffer.data()), searchSize);

    // Look for EOCD signature
    for (int64_t i = searchSize - EOCD_BASE_SIZE; i >= 0; --i) {
        if (buffer[i] == 0x50 && buffer[i + 1] == 0x4b && 
            buffer[i + 2] == 0x05 && buffer[i + 3] == 0x06) {
            
            file_.seekg(searchStart + i);
            return true;
        }
    }

    return false;
}

bool ZipReader::readEndOfCentralDirectory() {
    eocdRecord_.signature = readUint32(file_);
    if (eocdRecord_.signature != EOCD_SIGNATURE) {
        return false;
    }

    eocdRecord_.diskNumber = readUint16(file_);
    eocdRecord_.centralDirDiskNumber = readUint16(file_);
    eocdRecord_.diskEntries = readUint16(file_);
    eocdRecord_.totalEntries = readUint16(file_);
    eocdRecord_.centralDirSize = readUint32(file_);
    eocdRecord_.centralDirOffset = readUint32(file_);
    eocdRecord_.commentLength = readUint16(file_);
    
    if (eocdRecord_.commentLength > 0) {
        eocdRecord_.comment = readString(file_, eocdRecord_.commentLength);
    }

    return true;
}

bool ZipReader::readCentralDirectory() {
    if (!seekTo(eocdRecord_.centralDirOffset)) {
        std::cerr << "Cannot seek to central directory offset: " << eocdRecord_.centralDirOffset << std::endl;
        return false;
    }

    for (uint16_t i = 0; i < eocdRecord_.totalEntries; ++i) {
        CentralDirectoryEntry cdEntry;
        uint32_t currentPos = getCurrentPosition();
        
        if (!readCentralDirectoryEntry(currentPos, cdEntry)) {
            std::cerr << "Cannot read central directory entry at position: " << currentPos << std::endl;
            return false;
        }

        auto entry = std::make_shared<ZipEntry>(cdEntry.name);
        entry->setCRC32(cdEntry.crc32);
        entry->setCompressedSize(cdEntry.compressedSize);
        entry->setUncompressedSize(cdEntry.uncompressedSize);
        entry->setCompressionMethod(static_cast<ZipEntry::CompressionMethod>(cdEntry.compressionMethod));
        entry->setGeneralPurposeFlag(static_cast<ZipEntry::GeneralPurposeFlag>(cdEntry.generalPurposeFlag));
        entry->setLastModTime(cdEntry.lastModTime);
        entry->setLastModDate(cdEntry.lastModDate);
        entry->setLocalHeaderOffset(cdEntry.localHeaderOffset);
        entry->setArchivePath(filename_);
        
        entries_.push_back(entry);

        // Calculate next entry position
        uint32_t entrySize = 46 + cdEntry.nameLength + cdEntry.extraLength + cdEntry.commentLength;
        uint32_t nextPos = currentPos + entrySize;
        
        if (!seekTo(nextPos)) {
            std::cerr << "Cannot seek to next central directory entry at: " << nextPos << std::endl;
            return false;
        }
    }

    return true;
}

bool ZipReader::readCentralDirectoryEntry(uint32_t offset, CentralDirectoryEntry& entry) {
    if (!seekTo(offset)) {
        return false;
    }

    entry.signature = readUint32(file_);
    if (entry.signature != CENTRAL_DIR_SIGNATURE) {
        return false;
    }

    entry.versionMadeBy = readUint16(file_);
    entry.versionNeeded = readUint16(file_);
    entry.generalPurposeFlag = readUint16(file_);
    entry.compressionMethod = readUint16(file_);
    entry.lastModTime = readUint16(file_);
    entry.lastModDate = readUint16(file_);
    entry.crc32 = readUint32(file_);
    entry.compressedSize = readUint32(file_);
    entry.uncompressedSize = readUint32(file_);
    entry.nameLength = readUint16(file_);
    entry.extraLength = readUint16(file_);
    entry.commentLength = readUint16(file_);
    entry.diskNumberStart = readUint16(file_);
    entry.internalAttributes = readUint16(file_);
    entry.externalAttributes = readUint32(file_);
    entry.localHeaderOffset = readUint32(file_);

    if (entry.nameLength > 0) {
        entry.name = readString(file_, entry.nameLength);
    }

    if (entry.extraLength > 0) {
        entry.extra = readBytes(file_, entry.extraLength);
    }

    if (entry.commentLength > 0) {
        entry.comment = readString(file_, entry.commentLength);
    }

    return true;
}

bool ZipReader::readLocalHeader(const std::shared_ptr<ZipEntry>& entry) {
    if (!entry || !seekTo(entry->getLocalHeaderOffset())) {
        return false;
    }

    uint32_t signature = readUint32(file_);
    if (signature != LOCAL_HEADER_SIGNATURE) {
        return false;
    }

    uint16_t versionNeeded = readUint16(file_);
    uint16_t generalPurposeFlag = readUint16(file_);
    uint16_t compressionMethod = readUint16(file_);
    uint16_t lastModTime = readUint16(file_);
    uint16_t lastModDate = readUint16(file_);
    uint32_t crc32 = readUint32(file_);
    uint32_t compressedSize = readUint32(file_);
    uint32_t uncompressedSize = readUint32(file_);
    uint16_t nameLength = readUint16(file_);
    uint16_t extraLength = readUint16(file_);

    // Skip name and extra fields
    file_.seekg(nameLength + extraLength, std::ios::cur);

    // Update entry with local header information
    entry->setOffset(getCurrentPosition());
    
    return true;
}

bool ZipReader::readEntryData(const std::shared_ptr<ZipEntry>& entry, std::vector<uint8_t>& data) {
    if (!entry || !seekTo(entry->getOffset())) {
        return false;
    }

    data.resize(entry->getCompressedSize());
    file_.read(reinterpret_cast<char*>(data.data()), entry->getCompressedSize());
    
    return file_.good() && data.size() == entry->getCompressedSize();
}

// Utility methods
uint32_t ZipReader::readUint32(std::ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), 4);
    return value;
}

uint16_t ZipReader::readUint16(std::ifstream& file) {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), 2);
    return value;
}

uint8_t ZipReader::readUint8(std::ifstream& file) {
    uint8_t value;
    file.read(reinterpret_cast<char*>(&value), 1);
    return value;
}

std::string ZipReader::readString(std::ifstream& file, size_t length) {
    std::string result(length, '\0');
    file.read(&result[0], length);
    return result;
}

std::vector<uint8_t> ZipReader::readBytes(std::ifstream& file, size_t length) {
    std::vector<uint8_t> result(length);
    file.read(reinterpret_cast<char*>(result.data()), length);
    return result;
}

bool ZipReader::seekTo(uint32_t position) {
    file_.seekg(position);
    return file_.good();
}

uint32_t ZipReader::getCurrentPosition() {
    return static_cast<uint32_t>(file_.tellg());
}