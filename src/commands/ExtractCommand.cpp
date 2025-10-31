#include "ExtractCommand.h"
#include "../utils/FileUtils.h"
#include "../compression/CompressionEngine.h"
#include <iostream>

ExtractCommand::ExtractCommand(bool verbose, bool quiet, bool force)
    : verbose_(verbose), quiet_(quiet), force_(force) {
}

ExtractCommand::~ExtractCommand() {
}

int ExtractCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: Extract command requires archive name" << std::endl;
        return 1;
    }

    std::string archiveName = args[0];
    std::string destination = args.size() > 1 ? args[1] : ".";
    
    OutputFormatter formatter(verbose_, quiet_);

    // Open archive
    ZipReader reader;
    if (!reader.open(archiveName)) {
        formatter.printError("Cannot open archive: " + archiveName);
        return 1;
    }

    // Read entries
    auto entries = reader.readEntries();
    if (entries.empty()) {
        formatter.printInfo("Archive is empty");
        return 0;
    }

    // Ensure destination directory exists
    if (!ensureDirectoryExists(destination)) {
        formatter.printError("Cannot create destination directory: " + destination);
        return 1;
    }

    // Extract entries
    size_t extractedCount = 0;
    for (const auto& entry : entries) {
        if (extractEntry(entry, destination)) {
            extractedCount++;
            if (verbose_) {
                formatter.printInfo("Extracted: " + entry->getName());
            }
        } else {
            formatter.printError("Failed to extract: " + entry->getName());
        }
    }

    if (!quiet_) {
        formatter.printSuccess("Extracted " + std::to_string(extractedCount) + " entries to " + destination);
    }

    return 0;
}

bool ExtractCommand::extractEntry(const std::shared_ptr<ZipEntry>& entry, const std::string& destination) {
    if (!entry) return false;

    if (entry->isDirectory()) {
        return extractDirectory(entry, destination);
    } else {
        return extractFile(entry, destination);
    }
}

bool ExtractCommand::extractFile(const std::shared_ptr<ZipEntry>& entry, const std::string& destination) {
    if (!entry) return false;

    std::string destPath = getDestinationPath(entry->getName(), destination);
    
    // Check if file already exists
    if (FileUtils::exists(destPath) && !force_) {
        std::cerr << "File already exists: " << destPath << " (use -f to overwrite)" << std::endl;
        return false;
    }

    // Ensure parent directory exists
    std::string parentDir = FileUtils::getDirectoryName(destPath);
    if (!ensureDirectoryExists(parentDir)) {
        return false;
    }

    // Read compressed data from archive
    ZipReader reader;
    if (!reader.open(entry->getArchivePath())) {
        return false;
    }

    if (!reader.readEntryData(entry)) {
        reader.close();
        return false;
    }

    const auto& compressedData = entry->getCompressedData();
    
    // Decompress data if needed
    std::vector<uint8_t> uncompressedData;
    if (entry->getCompressionMethod() == ZipEntry::CompressionMethod::DEFLATED) {
        CompressionEngine engine;
        auto result = engine.decompress(compressedData, entry->getUncompressedSize());
        if (!result.success) {
            std::cerr << "Decompression failed: " << result.errorMessage << std::endl;
            reader.close();
            return false;
        }
        uncompressedData = result.decompressedData;
    } else {
        uncompressedData = compressedData;
    }

    reader.close();

    // Write file
    if (!FileUtils::writeFile(destPath, uncompressedData)) {
        std::cerr << "Cannot write file: " << destPath << std::endl;
        return false;
    }

    return true;
}

bool ExtractCommand::extractDirectory(const std::shared_ptr<ZipEntry>& entry, const std::string& destination) {
    if (!entry) return false;

    std::string destPath = getDestinationPath(entry->getName(), destination);
    
    // Remove trailing slash for directory creation
    if (!destPath.empty() && destPath.back() == '/') {
        destPath.pop_back();
    }

    return ensureDirectoryExists(destPath);
}

std::string ExtractCommand::getDestinationPath(const std::string& entryName, const std::string& destination) {
    if (destination == "." || destination.empty()) {
        return entryName;
    }
    
    // Ensure destination doesn't end with slash to avoid double slashes
    std::string cleanDest = destination;
    if (!cleanDest.empty() && cleanDest.back() == '/' && cleanDest.back() == '\\') {
        cleanDest.pop_back();
    }
    
    return FileUtils::joinPath(cleanDest, entryName);
}

bool ExtractCommand::ensureDirectoryExists(const std::string& dirPath) {
    if (dirPath.empty() || dirPath == "." || dirPath == "..") {
        return true;
    }

    if (FileUtils::exists(dirPath)) {
        return FileUtils::isDirectory(dirPath);
    }

    return FileUtils::createDirectories(dirPath);
}