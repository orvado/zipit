#pragma once

#include <string>
#include <vector>
#include <map>

class ArgumentParser {
private:
    struct Argument {
        std::string name;
        std::string description;
        bool required;
        std::string value;
    };

    std::map<std::string, Argument> arguments_;
    std::vector<std::string> positionalArgs_;

public:
    ArgumentParser();
    ~ArgumentParser();

    void addArgument(const std::string& name, const std::string& description, bool required = false);
    bool parse(int argc, char* argv[]);
    bool hasArgument(const std::string& name) const;
    std::string getArgument(const std::string& name) const;
    void printHelp() const;
};