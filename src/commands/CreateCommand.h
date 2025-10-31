#pragma once

#include <vector>
#include <string>
#include "../core/ZipWriter.h"
#include "../utils/OutputFormatter.h"

class CreateCommand {
private:
    bool verbose_;
    bool quiet_;
    bool force_;
    bool recursive_;

public:
    CreateCommand(bool verbose = false, bool quiet = false, bool force = false, bool recursive = false);
    ~CreateCommand();

    int execute(const std::vector<std::string>& args);

private:
    bool addFilesToArchive(ZipWriter& writer, const std::vector<std::string>& files);
    bool addFileOrDirectory(ZipWriter& writer, const std::string& path, const std::string& basePath = "");
    std::string getArchivePath(const std::string& filePath, const std::string& basePath);
};