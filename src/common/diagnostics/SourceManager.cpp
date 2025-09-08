/**
 * @file SourceManager.cpp
 * @brief Implementación del SourceManager
 */

#include <compiler/common/diagnostics/SourceManager.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace cpp20::compiler::diagnostics {

// SourceFile implementation
SourceFile::SourceFile(uint32_t id, std::filesystem::path path, std::string content)
    : id(id), path(std::move(path)), content(std::move(content)) {
    // Compute line offsets
    lineOffsets.push_back(0); // First line starts at offset 0
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            lineOffsets.push_back(static_cast<uint32_t>(i + 1));
        }
    }
    displayName = this->path.filename().string();
}

SourceLocation SourceFile::locationForOffset(uint32_t offset) const {
    if (offset >= content.size()) {
        return SourceLocation::invalid();
    }

    // Find the line containing this offset using binary search
    auto it = std::upper_bound(lineOffsets.begin(), lineOffsets.end(), offset);
    size_t lineIndex = it - lineOffsets.begin() - 1;

    uint32_t line = static_cast<uint32_t>(lineIndex + 1);
    uint32_t column = offset - lineOffsets[lineIndex] + 1;

    return SourceLocation(line, column, offset, id);
}

uint32_t SourceFile::offsetForLocation(const SourceLocation& location) const {
    if (location.line() == 0 || location.line() > lineOffsets.size()) {
        return 0;
    }

    uint32_t lineStart = lineOffsets[location.line() - 1];
    return lineStart + location.column() - 1;
}

std::string SourceFile::getLine(uint32_t lineNumber) const {
    if (lineNumber == 0 || lineNumber > lineOffsets.size()) {
        return "";
    }

    uint32_t start = lineOffsets[lineNumber - 1];
    uint32_t end = (lineNumber < lineOffsets.size()) ?
                   lineOffsets[lineNumber] : static_cast<uint32_t>(content.size());

    // Remove trailing newline/carriage return
    while (end > start && (content[end - 1] == '\n' || content[end - 1] == '\r')) {
        --end;
    }

    return content.substr(start, end - start);
}

std::string SourceFile::getText(SourceRange range) const {
    uint32_t startOffset = offsetForLocation(range.start());
    uint32_t endOffset = offsetForLocation(range.end());

    if (startOffset >= endOffset || endOffset > content.size()) {
        return "";
    }

    return content.substr(startOffset, endOffset - startOffset);
}

// SourceManager implementation
SourceManager::SourceManager() = default;
SourceManager::~SourceManager() = default;

uint32_t SourceManager::loadFile(const std::filesystem::path& path,
                                const std::string& displayName) {
    // Check if already loaded
    auto it = pathToId_.find(path);
    if (it != pathToId_.end()) {
        return it->second;
    }

    // Load file content
    std::string content;
    if (!loadFileContent(path, content)) {
        return 0; // Invalid file ID
    }

    // Create source file
    uint32_t fileId = assignFileId();
    auto sourceFile = std::make_unique<SourceFile>(fileId, path, std::move(content));
    if (!displayName.empty()) {
        sourceFile->displayName = displayName;
    }

    files_.push_back(std::move(sourceFile));
    pathToId_[path] = fileId;

    return fileId;
}

uint32_t SourceManager::createVirtualFile(std::string content,
                                         const std::string& displayName) {
    uint32_t fileId = assignFileId();
    auto sourceFile = std::make_unique<SourceFile>(
        fileId,
        std::filesystem::path("<virtual>/" + displayName),
        std::move(content)
    );
    sourceFile->displayName = displayName;
    sourceFile->isPreprocessed = true;

    files_.push_back(std::move(sourceFile));
    return fileId;
}

const SourceFile* SourceManager::getFile(uint32_t fileId) const {
    if (fileId == 0 || fileId > files_.size()) {
        return nullptr;
    }
    return files_[fileId - 1].get();
}

const SourceFile* SourceManager::getFileForLocation(const SourceLocation& location) const {
    return getFile(location.fileId());
}

SourceLocation SourceManager::getLocation(uint32_t fileId, uint32_t offset) const {
    const SourceFile* file = getFile(fileId);
    if (!file) {
        return SourceLocation::invalid();
    }
    return file->locationForOffset(offset);
}

uint32_t SourceManager::getOffset(const SourceLocation& location) const {
    const SourceFile* file = getFileForLocation(location);
    if (!file) {
        return 0;
    }
    return file->offsetForLocation(location);
}

std::string SourceManager::getText(const SourceRange& range) const {
    const SourceFile* file = getFileForLocation(range.start());
    if (!file) {
        return "";
    }
    return file->getText(range);
}

std::string SourceManager::getLine(const SourceLocation& location) const {
    const SourceFile* file = getFileForLocation(location);
    if (!file) {
        return "";
    }
    return file->getLine(location.line());
}

std::string SourceManager::getContextLines(const SourceLocation& location,
                                          int beforeLines,
                                          int afterLines) const {
    // TODO: Implement context lines display
    return getLine(location);
}

size_t SourceManager::totalSize() const {
    size_t total = 0;
    for (const auto& file : files_) {
        total += file->content.size();
    }
    return total;
}

void SourceManager::clearCache() {
    files_.clear();
    pathToId_.clear();
    nextFileId_ = 1;
}

void SourceManager::preloadFiles(const std::vector<std::filesystem::path>& paths) {
    for (const auto& path : paths) {
        loadFile(path);
    }
}

std::string SourceManager::getDisplayName(uint32_t fileId) const {
    const SourceFile* file = getFile(fileId);
    return file ? file->displayName : "<unknown>";
}

bool SourceManager::isValidFileId(uint32_t fileId) const {
    return fileId > 0 && fileId <= files_.size();
}

bool SourceManager::isValidLocation(const SourceLocation& location) const {
    return isValidFileId(location.fileId());
}

uint32_t SourceManager::assignFileId() {
    return nextFileId_++;
}


bool SourceManager::loadFileContent(const std::filesystem::path& path,
                                   std::string& content) const {
    try {
        content = readFileToString(path);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string SourceManager::readFileToString(const std::filesystem::path& path) const {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content(static_cast<size_t>(size), '\0');
    if (!file.read(content.data(), size)) {
        throw std::runtime_error("Cannot read file: " + path.string());
    }

    return content;
}

} // namespace cpp20::compiler::diagnostics
