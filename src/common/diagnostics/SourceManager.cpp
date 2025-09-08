/**
 * @file SourceManager.cpp
 * @brief Implementación avanzada del SourceManager para C++20
 */

#include <compiler/common/diagnostics/SourceManager.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <optional>
#include <filesystem>

namespace cpp20::compiler::diagnostics {

// === SourceFile Implementation ===

SourceFile::SourceFile(uint32_t id, std::filesystem::path path, std::string rawContent,
                       Encoding encoding)
    : id(id), path(std::move(path)), rawContent(std::move(rawContent)), encoding(encoding) {
    // Normalizar finales de línea (implementación inline)
    std::string result;
    result.reserve(rawContent.size());

    for (size_t i = 0; i < rawContent.size(); ++i) {
        if (rawContent[i] == '\r') {
            // Convertir \r\n a \n, o \r solo a \n
            if (i + 1 < rawContent.size() && rawContent[i + 1] == '\n') {
                ++i; // Saltar el \n después de \r
            }
            result += '\n';
        } else {
            result += rawContent[i];
        }
    }

    normalizedContent = std::move(result);

    // Calcular offsets de línea en contenido normalizado
    lineOffsets = computeLineOffsets(normalizedContent);

    // Inicializar display name
    displayName = this->path.filename().string();

    // Calcular tamaño
    fileSize = this->rawContent.size();

    // Timestamp de modificación
    try {
        lastModified = std::filesystem::last_write_time(this->path);
    } catch (...) {
        lastModified = std::filesystem::file_time_type::clock::now();
    }
}

std::vector<uint32_t> SourceFile::computeLineOffsets(const std::string& content) const {
    std::vector<uint32_t> offsets;
    offsets.push_back(0); // Primera línea comienza en offset 0

    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            offsets.push_back(static_cast<uint32_t>(i + 1));
        }
    }

    return offsets;
}

SourceLocation SourceFile::locationForOffset(uint32_t offset) const {
    if (offset >= normalizedContent.size()) {
        return SourceLocation::invalid();
    }

    // Búsqueda binaria para encontrar la línea que contiene este offset
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
                   lineOffsets[lineNumber] : static_cast<uint32_t>(normalizedContent.size());

    // Remover caracteres de nueva línea al final
    while (end > start && (normalizedContent[end - 1] == '\n' || normalizedContent[end - 1] == '\r')) {
        --end;
    }

    return normalizedContent.substr(start, end - start);
}

std::string SourceFile::getText(SourceRange range) const {
    uint32_t startOffset = offsetForLocation(range.start());
    uint32_t endOffset = offsetForLocation(range.end());

    if (startOffset >= endOffset || endOffset > normalizedContent.size()) {
        return "";
    }

    return normalizedContent.substr(startOffset, endOffset - startOffset);
}

void SourceFile::addMacroExpansion(uint32_t offset, const std::string& expansion) {
    macroExpansions[offset] = expansion;
}

std::optional<std::string> SourceFile::getMacroExpansion(uint32_t offset) const {
    auto it = macroExpansions.find(offset);
    if (it != macroExpansions.end()) {
        return it->second;
    }
    return std::nullopt;
}

SourceLocation SourceFile::mapToOriginalLocation(uint32_t offset) const {
    auto it = offsetToOriginalLocation.find(offset);
    if (it != offsetToOriginalLocation.end()) {
        return it->second;
    }
    return locationForOffset(offset);
}

// === IncludeSearchPath Implementation ===

void IncludeSearchPath::addSystemPath(const std::filesystem::path& path) {
    systemPaths_.push_back(path);
}

void IncludeSearchPath::addUserPath(const std::filesystem::path& path) {
    userPaths_.push_back(path);
}

void IncludeSearchPath::clearPaths() {
    systemPaths_.clear();
    userPaths_.clear();
}

std::optional<std::filesystem::path> IncludeSearchPath::findInclude(
    const std::string& includeName,
    bool isSystemInclude) const {

    const auto& searchPaths = isSystemInclude ? systemPaths_ : userPaths_;

    for (const auto& basePath : searchPaths) {
        std::filesystem::path fullPath = basePath / includeName;
        if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
            return fullPath;
        }
    }

    // Si es include de usuario y no se encontró, intentar en rutas de sistema como fallback
    if (!isSystemInclude) {
        for (const auto& basePath : systemPaths_) {
            std::filesystem::path fullPath = basePath / includeName;
            if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
                return fullPath;
            }
        }
    }

    return std::nullopt;
}

// === SourceManager Implementation ===

SourceManager::SourceManager() = default;
SourceManager::~SourceManager() = default;

// === CARGA DE ARCHIVOS ===

uint32_t SourceManager::loadFile(const std::filesystem::path& path,
                                const std::string& displayName,
                                bool isHeaderUnit) {
    // Verificar si ya está cargado
    auto it = pathToId_.find(path);
    if (it != pathToId_.end()) {
        return it->second;
    }

    // Cargar contenido del archivo con detección de encoding
    std::string rawContent;
    Encoding encoding;
    std::filesystem::file_time_type lastModified;

    if (!loadFileContent(path, rawContent, encoding, lastModified)) {
        return 0; // ID de archivo inválido
    }

    // Crear archivo fuente
    uint32_t fileId = assignFileId();
    auto sourceFile = std::make_unique<SourceFile>(fileId, path, std::move(rawContent), encoding);
    sourceFile->lastModified = lastModified;
    sourceFile->isHeaderUnit = isHeaderUnit;

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
        std::move(content),
        Encoding::UTF8
    );
    sourceFile->displayName = displayName;
    sourceFile->isPreprocessed = true;

    files_.push_back(std::move(sourceFile));
    return fileId;
}

void SourceManager::preloadHeaders(const std::vector<std::filesystem::path>& paths) {
    for (const auto& path : paths) {
        loadFile(path, "", true);
    }
}

// === GESTIÓN DE INCLUDES ===

uint32_t SourceManager::findAndLoadInclude(const std::string& includeName,
                                          uint32_t currentFileId,
                                          bool isSystemInclude) {
    // Verificar caché primero
    auto cacheIt = includeCache_.find(includeName);
    if (cacheIt != includeCache_.end()) {
        const auto& entry = cacheIt->second;
        if (entry.isValid && isCacheValid(entry.resolvedPath, entry)) {
            cacheHits_++;
            return entry.fileId;
        }
    }

    cacheMisses_++;

    // Buscar el archivo usando las rutas configuradas
    auto resolvedPath = includeSearchPath_.findInclude(includeName, isSystemInclude);
    if (!resolvedPath) {
        return 0; // No encontrado
    }

    // Cargar el archivo encontrado
    uint32_t fileId = loadFile(*resolvedPath, includeName, false);
    if (fileId != 0) {
        updateIncludeCache(includeName, *resolvedPath, fileId);
    }

    return fileId;
}

void SourceManager::setIncludeSearchPath(const IncludeSearchPath& searchPath) {
    includeSearchPath_ = searchPath;
}

void SourceManager::addIncludePath(const std::filesystem::path& path, bool isSystemPath) {
    if (isSystemPath) {
        includeSearchPath_.addSystemPath(path);
    } else {
        includeSearchPath_.addUserPath(path);
    }
}

// === ACCESO A ARCHIVOS ===

const SourceFile* SourceManager::getFile(uint32_t fileId) const {
    if (fileId == 0 || fileId > files_.size()) {
        return nullptr;
    }
    return files_[fileId - 1].get();
}

const SourceFile* SourceManager::getFileForLocation(const SourceLocation& location) const {
    return getFile(location.fileId());
}

// === CONVERSIONES DE UBICACIÓN ===

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

SourceLocation SourceManager::getOriginalLocation(const SourceLocation& location) const {
    const SourceFile* file = getFileForLocation(location);
    if (!file) {
        return location;
    }

    uint32_t offset = file->offsetForLocation(location);
    return file->mapToOriginalLocation(offset);
}

// === ACCESO A TEXTO ===

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
    const SourceFile* file = getFileForLocation(location);
    if (!file) {
        return "";
    }

    uint32_t startLine = std::max(1u, location.line() - static_cast<uint32_t>(beforeLines));
    uint32_t endLine = std::min(file->lineCount(), location.line() + static_cast<uint32_t>(afterLines));

    std::stringstream result;
    for (uint32_t line = startLine; line <= endLine; ++line) {
        std::string lineText = file->getLine(line);
        result << std::setw(6) << line << " | " << lineText;
        if (line < endLine) {
            result << "\n";
        }
    }

    return result.str();
}

// === SOPORTE PARA PREPROCESADOR ===

void SourceManager::registerMacroExpansion(const SourceLocation& location, const std::string& expansion) {
    SourceFile* file = const_cast<SourceFile*>(getFileForLocation(location));
    if (file) {
        uint32_t offset = file->offsetForLocation(location);
        file->addMacroExpansion(offset, expansion);
    }
}

void SourceManager::registerLineMapping(const SourceLocation& currentLocation,
                                      const SourceLocation& originalLocation) {
    SourceFile* file = const_cast<SourceFile*>(getFileForLocation(currentLocation));
    if (file) {
        uint32_t offset = file->offsetForLocation(currentLocation);
        file->offsetToOriginalLocation[offset] = originalLocation;
    }
}

// === GESTIÓN DE HEADER UNITS ===

void SourceManager::markAsHeaderUnit(uint32_t fileId, const std::string& /*moduleName*/) {
    SourceFile* file = const_cast<SourceFile*>(getFile(fileId));
    if (file) {
        file->isHeaderUnit = true;
    }
}

bool SourceManager::isHeaderUnit(uint32_t fileId) const {
    const SourceFile* file = getFile(fileId);
    return file ? file->isHeaderUnit : false;
}

// === ESTADÍSTICAS Y GESTIÓN ===

size_t SourceManager::totalSize() const {
    size_t total = 0;
    for (const auto& file : files_) {
        total += file->fileSize;
    }
    return total;
}

void SourceManager::clearCache() {
    files_.clear();
    pathToId_.clear();
    includeCache_.clear();
    nextFileId_ = 1;
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

void SourceManager::clearIncludeCache() {
    includeCache_.clear();
}

void SourceManager::preloadFiles(const std::vector<std::filesystem::path>& paths) {
    for (const auto& path : paths) {
        loadFile(path);
    }
}

// === UTILIDADES ===

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

Encoding SourceManager::detectEncoding(const std::string& content) const {
    if (content.size() >= 3) {
        // UTF-8 BOM
        if (content[0] == '\xEF' && content[1] == '\xBB' && content[2] == '\xBF') {
            return Encoding::UTF8;
        }
        // UTF-16 BE BOM
        if (content[0] == '\xFE' && content[1] == '\xFF') {
            return Encoding::UTF16_BE;
        }
        // UTF-16 LE BOM
        if (content[0] == '\xFF' && content[1] == '\xFE') {
            return Encoding::UTF16_LE;
        }
    }

    if (content.size() >= 4) {
        // UTF-32 BE BOM
        if (content[0] == '\x00' && content[1] == '\x00' && content[2] == '\xFE' && content[3] == '\xFF') {
            return Encoding::UTF32_BE;
        }
        // UTF-32 LE BOM
        if (content[0] == '\xFF' && content[1] == '\xFE' && content[2] == '\x00' && content[3] == '\x00') {
            return Encoding::UTF32_LE;
        }
    }

    // Asumir ASCII/UTF-8 por defecto
    return Encoding::UTF8;
}

std::string SourceManager::normalizeLineEndings(const std::string& content) const {
    std::string result;
    result.reserve(content.size());

    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\r') {
            // Convertir \r\n a \n, o \r solo a \n
            if (i + 1 < content.size() && content[i + 1] == '\n') {
                ++i; // Saltar el \n después de \r
            }
            result += '\n';
        } else {
            result += content[i];
        }
    }

    return result;
}

// === MÉTODOS INTERNOS ===

uint32_t SourceManager::assignFileId() {
    return nextFileId_++;
}

std::vector<uint32_t> SourceManager::computeLineOffsets(const std::string& content) const {
    std::vector<uint32_t> offsets;
    offsets.push_back(0);

    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            offsets.push_back(static_cast<uint32_t>(i + 1));
        }
    }

    return offsets;
}

bool SourceManager::loadFileContent(const std::filesystem::path& path,
                                   std::string& content,
                                   Encoding& encoding,
                                   std::filesystem::file_time_type& lastModified) const {
    try {
        // Leer archivo en modo binario
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        content.resize(static_cast<size_t>(size));
        if (!file.read(content.data(), size)) {
            return false;
        }

        // Detectar encoding
        encoding = detectEncoding(content);

        // Decodificar si es necesario
        if (encoding != Encoding::UTF8 && encoding != Encoding::ASCII) {
            content = decodeContent(content, encoding);
            encoding = Encoding::UTF8; // Después de decodificar es UTF-8
        }

        // Obtener timestamp de modificación
        lastModified = std::filesystem::last_write_time(path);

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

std::string SourceManager::decodeContent(const std::string& rawContent, Encoding encoding) const {
    // Implementación simplificada - en un compilador real necesitaríamos
    // una biblioteca de codificación completa como ICU
    switch (encoding) {
        case Encoding::UTF16_LE:
        case Encoding::UTF16_BE:
            // TODO: Implementar conversión UTF-16 a UTF-8
            return rawContent;
        case Encoding::UTF32_LE:
        case Encoding::UTF32_BE:
            // TODO: Implementar conversión UTF-32 a UTF-8
            return rawContent;
        case Encoding::LATIN1:
            // TODO: Implementar conversión Latin1 a UTF-8
            return rawContent;
        default:
            return rawContent;
    }
}

std::string SourceManager::computeContentHash(const std::string& content) const {
    // Implementación simple de hash - en producción usaríamos un hash criptográfico
    size_t hash = 0;
    for (char c : content) {
        hash = hash * 31 + static_cast<size_t>(static_cast<unsigned char>(c));
    }
    return std::to_string(hash);
}

bool SourceManager::isCacheValid(const std::filesystem::path& path, const IncludeCacheEntry& entry) const {
    try {
        auto currentTime = std::filesystem::last_write_time(path);
        return currentTime <= entry.lastModified;
    } catch (...) {
        return false;
    }
}

void SourceManager::updateIncludeCache(const std::string& includeName,
                                      const std::filesystem::path& resolvedPath,
                                      uint32_t fileId) {
    IncludeCacheEntry entry;
    entry.resolvedPath = resolvedPath;
    entry.fileId = fileId;
    entry.isValid = true;

    try {
        entry.lastModified = std::filesystem::last_write_time(resolvedPath);
        const SourceFile* file = getFile(fileId);
        if (file) {
            entry.contentHash = computeContentHash(file->normalizedContent);
        }
    } catch (...) {
        entry.isValid = false;
    }

    includeCache_[includeName] = entry;
}

} // namespace cpp20::compiler::diagnostics
