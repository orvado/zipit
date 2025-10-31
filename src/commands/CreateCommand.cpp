#include "CreateCommand.h"
#include "../utils/FileUtils.h"
#include <iostream>

CreateCommand::CreateCommand(bool verbose, bool quiet, bool force, bool recursive)
    : verbose_(verbose), quiet_(quiet), force_(force), recursive_(recursive) {
}

CreateCommand::~CreateCommand() {
}

int CreateCommand::execute(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Error: Create command requires archive name and at least one file/directory" << std::endl;
        return 1;
    }

    std::string archiveName = args[0];
    std::vector<std::string> files(args.begin() + 1, args.end());
    
    OutputFormatter formatter(verbose_, quiet_);

    // Check if archive already exists
    if (FileUtils::exists(archiveName) && !force_) {
        formatter.printError("Archive already exists: " + archiveName + " (use -f to overwrite)");
        return 1;
    }

    // Create zip writer
    ZipWriter writer;
    if (!writer.open(archiveName)) {
        formatter.printError("Cannot create archive: " + archiveName);
        return 1;
    }

    // Add files to archive
    if (!addFilesToArchive(writer, files)) {
        writer.close();
        FileUtils::deleteFile(archiveName);
        return 1;
    }

    // Finalize archive
    if (!writer.finalize()) {
        formatter.printError("Failed to finalize archive");
        writer.close();
        FileUtils::deleteFile(archiveName);
        return 1;
    }

    writer.close();
    
    if (!quiet_) {
        formatter.printSuccess("Archive created successfully: " + archiveName);
        if (verbose_) {
            std::cout << "  Entries: " << writer.getEntryCount() << std::endl;
        }
    }

    return 0;
}

bool CreateCommand::addFilesToArchive(ZipWriter& writer, const std::vector<std::string>& files) {
    OutputFormatter formatter(verbose_, quiet_);

    for (const auto& file : files) {
        if (!FileUtils::exists(file)) {
            formatter.printError("File not found: " + file);
            return false;
        }

        if (!addFileOrDirectory(writer, file)) {
            return false;
        }
    }

    return true;
}

bool CreateCommand::addFileOrDirectory(ZipWriter& writer, const std::string& path, const std::string& basePath) {
    OutputFormatter formatter(verbose_, quiet_);

    if (FileUtils::isFile(path)) {
        // Add file
        std::string archivePath = getArchivePath(path, basePath);
        if (verbose_) {
            formatter.printInfo("Adding file: " + archivePath);
        }
        
        if (!writer.addFile(path, archivePath)) {
            formatter.printError("Failed to add file: " + path);
            return false;
        }
    } else if (FileUtils::isDirectory(path)) {
        // Add directory
        std::string dirName = FileUtils::getFileName(path);
        std::string archivePath = basePath.empty() ? dirName : basePath + "/" + dirName;
        
        if (verbose_) {
            formatter.printInfo("Adding directory: " + archivePath + "/");
        }
        
        if (!writer.addDirectory(path, archivePath + "/")) {
            formatter.printError("Failed to add directory: " + path);
            return false;
        }

        // Recursively add directory contents if requested
        if (recursive_) {
            auto entries = FileUtils::listDirectory(path, false);
            std::string newBasePath = basePath.empty() ? dirName : basePath + "/" + dirName;
            
            for (const auto& entry : entries) {
                std::string entryName = FileUtils::getFileName(entry.path);
                if (entryName != "." && entryName != "..") {
                    if (!addFileOrDirectory(writer, entry.path, newBasePath)) {
                        return false;
                    }
                }
            }
        }
    } else {
        std::cerr << "Error: Invalid path: " << path << std::endl;
        return false;
    }

    return true;
}

std::string CreateCommand::getArchivePath(const std::string& filePath, const std::string& basePath) {
    if (basePath.empty()) {
        return FileUtils::getFileName(filePath);
    }
    
    std::string fileName = FileUtils::getFileName(filePath);
    return basePath + "/" + fileName;
}