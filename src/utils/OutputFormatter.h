#pragma once

#include "../core/ZipEntry.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>

class OutputFormatter {
public:
    enum class ListFormat {
        SIMPLE,
        DETAILED,
        TREE
    };

private:
    bool verbose_;
    bool quiet_;
    ListFormat listFormat_;

    // Helper methods
    void printEntrySimple(const std::shared_ptr<ZipEntry>& entry) const;
    void printEntryDetailed(const std::shared_ptr<ZipEntry>& entry) const;
    void printTreeEntry(const std::shared_ptr<ZipEntry>& entry, const std::string& prefix, bool isLast, bool isDirectory) const;
    std::string formatDate(const std::chrono::system_clock::time_point& time) const;
    std::string getCompressionMethodString(ZipEntry::CompressionMethod method) const;

public:
    OutputFormatter(bool verbose = false, bool quiet = false, ListFormat format = ListFormat::DETAILED);
    ~OutputFormatter();

    // Configuration
    void setVerbose(bool verbose) { verbose_ = verbose; }
    void setQuiet(bool quiet) { quiet_ = quiet; }
    void setListFormat(ListFormat format) { listFormat_ = format; }

    // Entry printing
    void printEntry(const std::shared_ptr<ZipEntry>& entry) const;
    void printEntries(const std::vector<std::shared_ptr<ZipEntry>>& entries) const;
    void printTree(const std::vector<std::shared_ptr<ZipEntry>>& entries) const;
    
    // Archive summary
    void printArchiveSummary(const std::vector<std::shared_ptr<ZipEntry>>& entries) const;
    void printValidationResult(const std::vector<std::string>& errors, const std::vector<std::string>& warnings) const;

    // General output
    void printError(const std::string& message) const;
    void printSuccess(const std::string& message) const;
    void printWarning(const std::string& message) const;
    void printInfo(const std::string& message) const;
    void printProgress(const std::string& operation, int current, int total) const;
    
    // Utility methods (made public for CheckCommand)
    std::string formatFileSize(uint64_t size) const;
};