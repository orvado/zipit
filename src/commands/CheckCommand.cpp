#include "CheckCommand.h"
#include <iomanip>

CheckCommand::CheckCommand(bool verbose, bool quiet)
    : verbose_(verbose), quiet_(quiet) {
}

CheckCommand::~CheckCommand() {
}

int CheckCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Error: Check command requires archive name" << std::endl;
        return 1;
    }

    std::string archiveName = args[0];
    OutputFormatter formatter(verbose_, quiet_);

    // Open and validate archive
    ZipReader reader;
    if (!reader.open(archiveName)) {
        formatter.printError("Cannot open archive: " + archiveName);
        return 1;
    }

    // Perform detailed validation
    auto result = reader.validateArchiveDetailed();
    
    // Print validation results
    formatter.printValidationResult(result.errors, result.warnings);
    
    // Print summary if verbose
    if (verbose_ && !quiet_) {
        std::cout << "\nValidation Summary:\n";
        std::cout << "  Total entries: " << result.totalEntries << "\n";
        std::cout << "  Valid entries: " << result.validEntries << "\n";
        std::cout << "  Total uncompressed size: " << formatter.formatFileSize(result.totalUncompressedSize) << "\n";
        std::cout << "  Total compressed size: " << formatter.formatFileSize(result.totalCompressedSize) << "\n";
        std::cout << "  Overall compression ratio: " << std::fixed << std::setprecision(1) << result.overallCompressionRatio << "%\n";
    }

    return result.isValid ? 0 : 1;
}