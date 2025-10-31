#pragma once

#include <vector>
#include <string>
#include "../core/ZipReader.h"
#include "../utils/OutputFormatter.h"

class CheckCommand {
private:
    bool verbose_;
    bool quiet_;

public:
    CheckCommand(bool verbose = false, bool quiet = false);
    ~CheckCommand();

    int execute(const std::vector<std::string>& args);
};