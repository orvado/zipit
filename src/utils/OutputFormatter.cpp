#include "OutputFormatter.h"
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <map>

OutputFormatter::OutputFormatter(bool verbose, bool quiet, ListFormat format)
    : verbose_(verbose), quiet_(quiet), listFormat_(format) {
}

OutputFormatter::~OutputFormatter() {
}

void OutputFormatter::printEntry(const std::shared_ptr<ZipEntry>& entry) const {
    if (quiet_) return;

    switch (listFormat_) {
        case ListFormat::SIMPLE:
            printEntrySimple(entry);
            break;
        case ListFormat::DETAILED:
            printEntryDetailed(entry);
            break;
        case ListFormat::TREE:
            // Tree format handled separately
            break;
    }
}

void OutputFormatter::printEntries(const std::vector<std::shared_ptr<ZipEntry>>& entries) const {
    if (quiet_) return;

    if (listFormat_ == ListFormat::TREE) {
        printTree(entries);
        return;
    }

    for (const auto& entry : entries) {
        printEntry(entry);
    }

    if (listFormat_ == ListFormat::DETAILED) {
        printArchiveSummary(entries);
    }
}

void OutputFormatter::printTree(const std::vector<std::shared_ptr<ZipEntry>>& entries) const {
    if (quiet_) return;

    std::cout << "Archive contents:\n";
    
    // Group entries by directory
    std::map<std::string, std::vector<std::shared_ptr<ZipEntry>>> directoryMap;
    std::vector<std::shared_ptr<ZipEntry>> rootEntries;

    for (const auto& entry : entries) {
        std::string path = entry->getName();
        size_t slashPos = path.find_last_of('/');
        
        if (slashPos == std::string::npos) {
            rootEntries.push_back(entry);
        } else {
            std::string dir = path.substr(0, slashPos);
            directoryMap[dir].push_back(entry);
        }
    }

    // Print root entries
    for (size_t i = 0; i < rootEntries.size(); ++i) {
        bool isLast = (i == rootEntries.size() - 1) && directoryMap.empty();
        printTreeEntry(rootEntries[i], "", isLast, rootEntries[i]->isDirectory());
    }

    // Print directory contents
    for (const auto& [dir, dirEntries] : directoryMap) {
        std::cout << "\n" << dir << "/\n";
        for (size_t i = 0; i < dirEntries.size(); ++i) {
            bool isLast = (i == dirEntries.size() - 1);
            printTreeEntry(dirEntries[i], "├── ", isLast, dirEntries[i]->isDirectory());
        }
    }
}

void OutputFormatter::printArchiveSummary(const std::vector<std::shared_ptr<ZipEntry>>& entries) const {
    if (quiet_) return;

    uint64_t totalSize = 0;
    uint64_t totalCompressed = 0;
    size_t fileCount = 0;
    size_t dirCount = 0;

    for (const auto& entry : entries) {
        totalSize += entry->getUncompressedSize();
        totalCompressed += entry->getCompressedSize();
        if (entry->isDirectory()) {
            dirCount++;
        } else {
            fileCount++;
        }
    }

    std::cout << "\nSummary:\n";
    std::cout << "  Files: " << fileCount << "\n";
    std::cout << "  Directories: " << dirCount << "\n";
    std::cout << "  Total size: " << formatFileSize(totalSize) << "\n";
    std::cout << "  Compressed size: " << formatFileSize(totalCompressed) << "\n";
    
    if (totalSize > 0) {
        double ratio = (1.0 - static_cast<double>(totalCompressed) / totalSize) * 100.0;
        std::cout << "  Compression ratio: " << std::fixed << std::setprecision(1) << ratio << "%\n";
    }
}

void OutputFormatter::printValidationResult(const std::vector<std::string>& errors, const std::vector<std::string>& warnings) const {
    if (quiet_) return;

    if (errors.empty() && warnings.empty()) {
        printSuccess("Archive validation passed - no issues found");
        return;
    }

    if (!errors.empty()) {
        std::cout << "\nErrors found:\n";
        for (const auto& error : errors) {
            std::cout << "  ❌ " << error << "\n";
        }
    }

    if (!warnings.empty()) {
        std::cout << "\nWarnings:\n";
        for (const auto& warning : warnings) {
            std::cout << "  ⚠️  " << warning << "\n";
        }
    }
}

void OutputFormatter::printError(const std::string& message) const {
    if (!quiet_) {
        std::cerr << "Error: " << message << std::endl;
    }
}

void OutputFormatter::printSuccess(const std::string& message) const {
    if (!quiet_) {
        std::cout << "✓ " << message << std::endl;
    }
}

void OutputFormatter::printWarning(const std::string& message) const {
    if (!quiet_) {
        std::cout << "Warning: " << message << std::endl;
    }
}

void OutputFormatter::printInfo(const std::string& message) const {
    if (!quiet_) {
        std::cout << message << std::endl;
    }
}

void OutputFormatter::printProgress(const std::string& operation, int current, int total) const {
    if (quiet_) return;

    if (total > 0) {
        int percent = (current * 100) / total;
        std::cout << "\r" << operation << ": " << percent << "% (" << current << "/" << total << ")";
        if (current == total) {
            std::cout << std::endl;
        }
    } else {
        std::cout << "\r" << operation << "...";
    }
    std::cout.flush();
}

// Private helper methods
void OutputFormatter::printEntrySimple(const std::shared_ptr<ZipEntry>& entry) const {
    std::cout << entry->getName() << std::endl;
}

void OutputFormatter::printEntryDetailed(const std::shared_ptr<ZipEntry>& entry) const {
    std::cout << std::left << std::setw(40) << entry->getName()
              << std::setw(12) << formatFileSize(entry->getUncompressedSize())
              << std::setw(12) << formatFileSize(entry->getCompressedSize())
              << std::setw(8) << entry->getFormattedCompressionRatio()
              << std::setw(12) << formatDate(entry->getLastModifiedTime())
              << std::setw(10) << getCompressionMethodString(entry->getCompressionMethod())
              << std::endl;
}

void OutputFormatter::printTreeEntry(const std::shared_ptr<ZipEntry>& entry, const std::string& prefix, bool isLast, bool isDirectory) const {
    std::string connector = isLast ? "└── " : "├── ";
    std::cout << prefix << connector << entry->getName();
    
    if (verbose_) {
        std::cout << " (" << formatFileSize(entry->getUncompressedSize());
        if (!isDirectory) {
            std::cout << ", " << entry->getFormattedCompressionRatio();
        }
        std::cout << ")";
    }
    
    std::cout << std::endl;
}

std::string OutputFormatter::formatFileSize(uint64_t size) const {
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

std::string OutputFormatter::formatDate(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

std::string OutputFormatter::getCompressionMethodString(ZipEntry::CompressionMethod method) const {
    switch (method) {
        case ZipEntry::CompressionMethod::STORED: return "Stored";
        case ZipEntry::CompressionMethod::DEFLATED: return "Deflated";
        default: return "Unknown";
    }
}