#pragma once

#include "ZipEntry.h"
#include <vector>
#include <memory>
#include <string>

class ZipArchive {
public:
    enum class Mode {
        READ,
        WRITE,
        APPEND
    };

private:
    std::string filename_;
    Mode mode_;
    bool isOpen_;
    std::vector<std::shared_ptr<ZipEntry>> entries_;

public:
    ZipArchive();
    ~ZipArchive();

    // Archive operations
    bool open(const std::string& filename, Mode mode);
    void close();
    bool isOpen() const;

    // Entry management
    std::vector<std::shared_ptr<ZipEntry>> getEntries() const;
    bool addEntry(const std::shared_ptr<ZipEntry>& entry);
    bool removeEntry(const std::string& name);
    std::shared_ptr<ZipEntry> findEntry(const std::string& name) const;

    // Archive operations
    bool save();
};