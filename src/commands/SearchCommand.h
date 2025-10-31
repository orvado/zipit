#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../core/ZipEntry.h"
#include "../utils/OutputFormatter.h"

class SearchCommand {
public:
    SearchCommand(bool verbose = false, bool quiet = false, bool caseSensitive = false);
    ~SearchCommand();

    int execute(const std::vector<std::string>& args);

private:
    bool verbose_;
    bool quiet_;
    bool caseSensitive_;
    
    bool searchInArchive(const std::string& archiveName, const std::string& pattern);
    bool matchesPattern(const std::string& text, const std::string& pattern) const;
    std::string convertToRegex(const std::string& pattern) const;
    void printMatch(const std::string& archiveName, const std::shared_ptr<ZipEntry>& entry, const std::string& pattern) const;
};