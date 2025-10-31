#include "AppendCommand.h"
#include "../utils/FileUtils.h"
#include <iostream>
#include <fstream>

AppendCommand::AppendCommand(bool verbose, bool quiet, bool force, bool recursive)
    : verbose_(verbose), quiet_(quiet), force_(force), recursive_(recursive) {
}

AppendCommand::~AppendCommand() {
}

int AppendCommand::execute(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Error: Append command requires archive name and at least one file/directory" << std::endl;
        return 1;
    }

    std::string archiveName = args[0];
    std::vector<std::string> files(args.begin() + 1, args.end());
    
    OutputFormatter formatter(verbose_, quiet_);

    // Check if archive exists
    if (!FileUtils::exists(archiveName)) {
        formatter.printError("Archive does not exist: " + archiveName);
        return 1;
    }

    // Check if archive is a valid zip file
    if (!FileUtils::isFile(archiveName)) {
        formatter.printError("Archive is not a regular file: " + archiveName);
        return 1;
    }

    // Validate existing archive
    ZipReader reader;
    if (!reader.open(archiveName)) {
        formatter.printError("Cannot open archive: " + archiveName);
        return 1;
    }

    auto validationResult = reader.validateArchiveDetailed();
    reader.close();

    if (!validationResult.isValid) {
        formatter.printError("Archive is corrupted or invalid: " + archiveName);
        for (const auto& error : validationResult.errors) {
            formatter.printError("  " + error);
        }
        return 1;
    }

    // Append files to archive
    if (!appendToArchive(archiveName, files)) {
        return 1;
    }

    if (!quiet_) {
        formatter.printSuccess("Files appended successfully to archive: " + archiveName);
    }

    return 0;
}

bool AppendCommand::appendToArchive(const std::string& archiveName, const std::vector<std::string>& files) {
    OutputFormatter formatter(verbose_, quiet_);

    // Create temporary file for the new archive
    std::string tempArchive = FileUtils::createTempFile("zipit_append_", ".zip");
    if (tempArchive.empty()) {
        formatter.printError("Cannot create temporary file");
        return false;
    }

    // Open existing archive for reading
    ZipReader reader;
    if (!reader.open(archiveName)) {
        formatter.printError("Cannot open existing archive: " + archiveName);
        FileUtils::deleteFile(tempArchive);
        return false;
    }

    // Create new archive for writing
    ZipWriter writer;
    if (!writer.open(tempArchive)) {
        formatter.printError("Cannot create temporary archive: " + tempArchive);
        reader.close();
        FileUtils::deleteFile(tempArchive);
        return false;
    }

    // Copy existing entries to new archive
    if (!copyExistingEntries(reader, writer)) {
        reader.close();
        writer.close();
        FileUtils::deleteFile(tempArchive);
        return false;
    }
    reader.close();

    // Add new files to archive
    if (!addFilesToArchive(writer, files)) {
        writer.close();
        FileUtils::deleteFile(tempArchive);
        return false;
    }

    // Finalize the new archive
    if (!writer.finalize()) {
        formatter.printError("Failed to finalize archive");
        writer.close();
        FileUtils::deleteFile(tempArchive);
        return false;
    }
    writer.close();

    // Replace original archive with temporary one
    if (!FileUtils::deleteFile(archiveName)) {
        formatter.printError("Cannot remove original archive: " + archiveName);
        FileUtils::deleteFile(tempArchive);
        return false;
    }

    if (!FileUtils::moveFile(tempArchive, archiveName, FileUtils::OverwriteMode::OVERWRITE)) {
        formatter.printError("Cannot move temporary archive to: " + archiveName);
        FileUtils::deleteFile(tempArchive);
        return false;
    }

    if (verbose_) {
        formatter.printInfo("Archive updated successfully");
    }

    return true;
}

bool AppendCommand::addFilesToArchive(ZipWriter& writer, const std::vector<std::string>& files) {
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

bool AppendCommand::addFileOrDirectory(ZipWriter& writer, const std::string& path, const std::string& basePath) {
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

std::string AppendCommand::getArchivePath(const std::string& filePath, const std::string& basePath) {
    if (basePath.empty()) {
        return FileUtils::getFileName(filePath);
    }
    
    std::string fileName = FileUtils::getFileName(filePath);
    return basePath + "/" + fileName;
}

bool AppendCommand::copyExistingEntries(ZipReader& reader, ZipWriter& writer) {
    OutputFormatter formatter(verbose_, quiet_);
    
    // Read all entries from the existing archive
    auto entries = reader.readEntries();
    
    if (verbose_) {
        formatter.printInfo("Copying " + std::to_string(entries.size()) + " existing entries");
    }

    // Copy each entry to the new archive
    for (const auto& entry : entries) {
        if (verbose_) {
            formatter.printInfo("Copying entry: " + entry->getName());
        }

        // Read entry data
        if (!reader.readEntryData(entry)) {
            formatter.printError("Failed to read data for entry: " + entry->getName());
            return false;
        }

        // Add entry to new archive
        if (!writer.addEntry(entry)) {
            formatter.printError("Failed to copy entry: " + entry->getName());
            return false;
        }
    }

    return true;
}
