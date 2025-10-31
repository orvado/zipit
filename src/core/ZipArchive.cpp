#include "ZipArchive.h"

ZipArchive::ZipArchive() : isOpen_(false) {
}

ZipArchive::~ZipArchive() {
    close();
}

bool ZipArchive::open(const std::string& filename, Mode mode) {
    // TODO: Implement archive opening
    return false;
}

void ZipArchive::close() {
    // TODO: Implement archive closing
}

bool ZipArchive::isOpen() const {
    return isOpen_;
}

std::vector<std::shared_ptr<ZipEntry>> ZipArchive::getEntries() const {
    return entries_;
}

bool ZipArchive::addEntry(const std::shared_ptr<ZipEntry>& entry) {
    // TODO: Implement entry addition
    return false;
}

bool ZipArchive::removeEntry(const std::string& name) {
    // TODO: Implement entry removal
    return false;
}

std::shared_ptr<ZipEntry> ZipArchive::findEntry(const std::string& name) const {
    // TODO: Implement entry finding
    return nullptr;
}

bool ZipArchive::save() {
    // TODO: Implement archive saving
    return false;
}