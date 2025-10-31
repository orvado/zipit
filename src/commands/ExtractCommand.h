#pragma once

#include <vector>
#include <string>
#include "../core/ZipReader.h"
#include "../utils/OutputFormatter.h"

class ExtractCommand {
private:
    bool verbose_;
    bool quiet_;
    bool force_;

public:
    ExtractCommand(bool verbose = false, bool quiet = false, bool force = false);
    ~ExtractCommand();

    int execute(const std::vector<std::string>& args);

private:
    bool extractEntry(const std::shared_ptr<ZipEntry>& entry, const std::string& destination);
    bool extractFile(const std::shared_ptr<ZipEntry>& entry, const std::string& destination);
    bool extractDirectory(const std::shared_ptr<ZipEntry>& entry, const std::string& destination);
    std::string getDestinationPath(const std::string& entryName, const std::string& destination);
    bool ensureDirectoryExists(const std::string& dirPath);
};