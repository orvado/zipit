#pragma once

#include <vector>
#include <string>
#include "../core/ZipReader.h"
#include "../utils/OutputFormatter.h"

class ListCommand {
private:
    bool verbose_;
    bool quiet_;
    bool treeFormat_;

public:
    ListCommand(bool verbose = false, bool quiet = false, bool treeFormat = false);
    ~ListCommand();

    int execute(const std::vector<std::string>& args);
};