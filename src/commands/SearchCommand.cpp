#include "SearchCommand.h"
#include "../core/ZipReader.h"
#include "../utils/FileUtils.h"
#include <iostream>
#include <regex>
#include <algorithm>

SearchCommand::SearchCommand(bool verbose, bool quiet, bool caseSensitive)
    : verbose_(verbose), quiet_(quiet), caseSensitive_(caseSensitive) {
}

SearchCommand::~SearchCommand() {
}

int SearchCommand::execute(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Error: Search command requires archive name and pattern" << std::endl;
        std::cerr << "Usage: zipit search <archive> <pattern> [archives...]" << std::endl;
        return 1;
    }

    std::string pattern = args[1];
    std::vector<std::string> archives;
    
    // If more than 2 args, treat args[2+] as additional archives
    if (args.size() > 2) {
        archives.push_back(args[0]);  // First archive
        for (size_t i = 2; i < args.size(); ++i) {
            archives.push_back(args[i]);
        }
    } else {
        archives.push_back(args[0]);  // Only one archive
    }
    
    OutputFormatter formatter(verbose_, quiet_);
    size_t totalMatches = 0;
    size_t searchedArchives = 0;

    for (const auto& archive : archives) {
        if (!FileUtils::exists(archive)) {
            formatter.printError("Archive not found: " + archive);
            continue;
        }
        
        if (searchInArchive(archive, pattern)) {
            searchedArchives++;
        }
    }

    if (!quiet_) {
        formatter.printSuccess("Search completed in " + std::to_string(searchedArchives) + " archives");
    }

    return 0;
}

bool SearchCommand::searchInArchive(const std::string& archiveName, const std::string& pattern) {
    ZipReader reader;
    if (!reader.open(archiveName)) {
        if (!quiet_) {
            std::cerr << "Cannot open archive: " << archiveName << std::endl;
        }
        return false;
    }

    auto entries = reader.readEntries();
    if (entries.empty()) {
        if (verbose_) {
            std::cout << "Archive is empty: " << archiveName << std::endl;
        }
        reader.close();
        return true;
    }

    size_t matchCount = 0;
    for (const auto& entry : entries) {
        if (matchesPattern(entry->getName(), pattern)) {
            matchCount++;
            printMatch(archiveName, entry, pattern);
        }
    }

    if (verbose_ && matchCount == 0) {
        std::cout << "No matches found in: " << archiveName << std::endl;
    }

    reader.close();
    return true;
}

bool SearchCommand::matchesPattern(const std::string& text, const std::string& pattern) const {
    try {
        std::string regexPattern = convertToRegex(pattern);
        std::regex::flag_type flags = std::regex::ECMAScript;
        if (!caseSensitive_) {
            flags |= std::regex::icase;
        }
        
        std::regex regex(regexPattern, flags);
        return std::regex_search(text, regex);
    } catch (const std::regex_error& e) {
        // If regex compilation fails, fall back to simple substring search
        if (caseSensitive_) {
            return text.find(pattern) != std::string::npos;
        } else {
            std::string lowerText = text;
            std::string lowerPattern = pattern;
            std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
            return lowerText.find(lowerPattern) != std::string::npos;
        }
    }
}

std::string SearchCommand::convertToRegex(const std::string& pattern) const {
    std::string regex;
    regex.reserve(pattern.length() * 2);
    
    for (char c : pattern) {
        switch (c) {
            case '*':
                regex += ".*";
                break;
            case '?':
                regex += ".";
                break;
            case '.':
            case '^':
            case '$':
            case '+':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '\\':
            case '|':
                regex += "\\";
                regex += c;
                break;
            default:
                regex += c;
                break;
        }
    }
    
    return regex;
}

void SearchCommand::printMatch(const std::string& archiveName, const std::shared_ptr<ZipEntry>& entry, const std::string& pattern) const {
    if (quiet_) {
        std::cout << archiveName << ":" << entry->getName() << std::endl;
    } else {
        std::cout << "[" << archiveName << "] " << entry->getName();
        if (entry->isDirectory()) {
            std::cout << " (directory)";
        } else {
            std::cout << " (" << entry->getFormattedSize() << ")";
        }
        std::cout << std::endl;
    }
}