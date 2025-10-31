#include "ListCommand.h"

ListCommand::ListCommand(bool verbose, bool quiet, bool treeFormat)
    : verbose_(verbose), quiet_(quiet), treeFormat_(treeFormat) {
}

ListCommand::~ListCommand() {
}

int ListCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: List command requires archive name" << std::endl;
        return 1;
    }

    std::string archiveName = args[0];
    
    // Create output formatter
    OutputFormatter::ListFormat format = treeFormat_ ? 
        OutputFormatter::ListFormat::TREE : 
        (verbose_ ? OutputFormatter::ListFormat::DETAILED : OutputFormatter::ListFormat::SIMPLE);
    
    OutputFormatter formatter(verbose_, quiet_, format);

    // Open and read archive
    ZipReader reader;
    if (!reader.open(archiveName)) {
        formatter.printError("Cannot open archive: " + archiveName);
        return 1;
    }

    // Read entries
    auto entries = reader.readEntries();
    if (entries.empty()) {
        formatter.printInfo("Archive is empty");
        return 0;
    }

    // Print entries
    formatter.printEntries(entries);

    return 0;
}