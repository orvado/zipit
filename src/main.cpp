#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <iomanip>

// Forward declarations for core classes
class ZipArchive;
class ZipEntry;

// Utility functions
void printVersion();
void printHelp();
void printError(const std::string& message);

// Command handlers
#include "commands/ListCommand.h"
#include "commands/CheckCommand.h"
#include "commands/CreateCommand.h"
#include "commands/ExtractCommand.h"
#include "commands/AppendCommand.h"
#include "commands/SearchCommand.h"

int handleCreateCommand(const std::vector<std::string>& args);
int handleExtractCommand(const std::vector<std::string>& args);
int handleAppendCommand(const std::vector<std::string>& args);
int handleCheckCommand(const std::vector<std::string>& args);
int handleListCommand(const std::vector<std::string>& args);
int handleSearchCommand(const std::vector<std::string>& args);

// Global settings
struct GlobalSettings {
    bool verbose = false;
    bool quiet = false;
    bool force = false;
    bool recursive = false;
};

GlobalSettings g_settings;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command;
    std::vector<std::string> args;
    
    // Parse arguments - first non-option is the command
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            g_settings.verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            g_settings.quiet = true;
        } else if (arg == "-f" || arg == "--force") {
            g_settings.force = true;
        } else if (arg == "-r" || arg == "--recursive") {
            g_settings.recursive = true;
        } else if (command.empty()) {
            // First non-option argument is the command
            command = arg;
        } else {
            // Remaining arguments are command arguments
            args.push_back(arg);
        }
    }
    
    if (command.empty()) {
        printHelp();
        return 1;
    }

    // Route to appropriate command handler
    try {
        if (command == "create") {
            return handleCreateCommand(args);
        } else if (command == "extract") {
            return handleExtractCommand(args);
        } else if (command == "append") {
            return handleAppendCommand(args);
        } else if (command == "check") {
            return handleCheckCommand(args);
        } else if (command == "list") {
            return handleListCommand(args);
        } else if (command == "search") {
            return handleSearchCommand(args);
        } else {
            printError("Unknown command: " + command);
            printHelp();
            return 1;
        }
    } catch (const std::exception& e) {
        printError("Error: " + std::string(e.what()));
        return 1;
    }
}

void printVersion() {
    std::cout << "ZipIt version 1.0.0\n";
    std::cout << "A modern C++ zip archive utility\n";
    std::cout << "Built with C++17 and zlib\n";
}

void printHelp() {
    std::cout << "ZipIt - C++ Zip Archive Utility\n\n";
    std::cout << "Usage: zipit <command> [options] <arguments>\n\n";
    std::cout << "Commands:\n";
    std::cout << "  create   <archive.zip> <files...>     Create new archive\n";
    std::cout << "  extract  <archive.zip> [dest]        Extract archive\n";
    std::cout << "  append   <archive.zip> <files...>     Add files to existing archive\n";
    std::cout << "  check    <archive.zip>               Validate archive\n";
    std::cout << "  list     <archive.zip>               List archive contents\n";
    std::cout << "  search   <archive.zip> <pattern>     Search in archive\n\n";
    std::cout << "Options:\n";
    std::cout << "  -v, --verbose     Verbose output\n";
    std::cout << "  -q, --quiet       Minimal output\n";
    std::cout << "  -f, --force       Force overwrite\n";
    std::cout << "  -r, --recursive   Process directories recursively\n";
    std::cout << "  -h, --help        Show help information\n";
    std::cout << "  --version         Show version information\n\n";
    std::cout << "Examples:\n";
    std::cout << "  zipit create archive.zip file1.txt file2.txt\n";
    std::cout << "  zipit create archive.zip -r directory/\n";
    std::cout << "  zipit extract archive.zip /path/to/extract\n";
    std::cout << "  zipit list archive.zip\n";
    std::cout << "  zipit search archive.zip \"*.txt\"\n";
}

void printError(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
}

void printVerbose(const std::string& message) {
    if (g_settings.verbose && !g_settings.quiet) {
        std::cout << "Verbose: " << message << std::endl;
    }
}

void printInfo(const std::string& message) {
    if (!g_settings.quiet) {
        std::cout << message << std::endl;
    }
}

// Command handlers (placeholder implementations)
int handleCreateCommand(const std::vector<std::string>& args) {
    printVerbose("Creating archive");
    
    CreateCommand command(g_settings.verbose, g_settings.quiet, g_settings.force, g_settings.recursive);
    return command.execute(args);
}

int handleExtractCommand(const std::vector<std::string>& args) {
    printVerbose("Extracting archive");
    
    ExtractCommand command(g_settings.verbose, g_settings.quiet, g_settings.force);
    return command.execute(args);
}

int handleAppendCommand(const std::vector<std::string>& args) {
    printVerbose("Appending to archive");
    
    AppendCommand command(g_settings.verbose, g_settings.quiet, g_settings.force, g_settings.recursive);
    return command.execute(args);
}

int handleCheckCommand(const std::vector<std::string>& args) {
    printVerbose("Checking archive");
    
    CheckCommand command(g_settings.verbose, g_settings.quiet);
    return command.execute(args);
}

int handleListCommand(const std::vector<std::string>& args) {
    printVerbose("Listing archive contents");
    
    ListCommand command(g_settings.verbose, g_settings.quiet, false);
    return command.execute(args);
}

int handleSearchCommand(const std::vector<std::string>& args) {
    printVerbose("Searching in archives");
    
    SearchCommand command(g_settings.verbose, g_settings.quiet, true);  // case sensitive by default
    return command.execute(args);
}
