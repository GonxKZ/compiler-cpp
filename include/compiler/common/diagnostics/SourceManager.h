#pragma once

#include "SourceLocation.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <optional>
#include <chrono>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Información de codificación de un archivo
 */
enum class Encoding {
    UTF8,
    UTF16_LE,
    UTF16_BE,
    UTF32_LE,
    UTF32_BE,
    LATIN1,
    ASCII,
    UNKNOWN
};

/**
 * @brief Información sobre un archivo fuente con soporte completo para C++20
 */
struct SourceFile {
    uint32_t id;                           // ID único del archivo
    std::filesystem::path path;           // Ruta completa del archivo
    std::string rawContent;               // Contenido raw sin procesar
    std::string normalizedContent;        // Contenido normalizado (UTF-8, \n)
    std::vector<uint32_t> lineOffsets;    // Offsets de inicio de cada línea
    std::string displayName;              // Nombre para mostrar (puede ser relativo)
    Encoding encoding = Encoding::UNKNOWN; // Codificación detectada
    bool isPreprocessed = false;          // Si el contenido ya está preprocesado
    bool isHeaderUnit = false;           // Si es una unidad de encabezado
    std::filesystem::file_time_type lastModified; // Última modificación
    size_t fileSize = 0;                 // Tamaño del archivo en bytes

    // Mapeos para preprocesador y módulos
    std::unordered_map<uint32_t, SourceLocation> offsetToOriginalLocation; // Para #line
    std::unordered_map<uint32_t, std::string> macroExpansions; // Expansiones de macros

    SourceFile(uint32_t id, std::filesystem::path path, std::string rawContent,
               Encoding encoding = Encoding::UTF8);

    // Utilidades
    uint32_t lineCount() const { return static_cast<uint32_t>(lineOffsets.size()); }
    SourceLocation locationForOffset(uint32_t offset) const;
    uint32_t offsetForLocation(const SourceLocation& location) const;
    std::string getLine(uint32_t lineNumber) const;
    std::string getText(SourceRange range) const;
    std::string getNormalizedContent() const { return normalizedContent; }

    // Soporte para mapeos de preprocesador
    void addMacroExpansion(uint32_t offset, const std::string& expansion);
    std::optional<std::string> getMacroExpansion(uint32_t offset) const;
    SourceLocation mapToOriginalLocation(uint32_t offset) const;
};

/**
 * @brief Entrada de caché para archivos de encabezado
 */
struct IncludeCacheEntry {
    std::filesystem::path resolvedPath;
    std::string contentHash;
    std::filesystem::file_time_type lastModified;
    uint32_t fileId = 0;
    bool isValid = true;
};

/**
 * @brief Sistema de búsqueda de includes con caché
 */
class IncludeSearchPath {
public:
    IncludeSearchPath() = default;

    void addSystemPath(const std::filesystem::path& path);
    void addUserPath(const std::filesystem::path& path);
    void clearPaths();

    std::optional<std::filesystem::path> findInclude(
        const std::string& includeName,
        bool isSystemInclude) const;

private:
    std::vector<std::filesystem::path> systemPaths_;
    std::vector<std::filesystem::path> userPaths_;
};

/**
 * @brief Administrador avanzado de archivos fuente para C++20
 *
 * El SourceManager es responsable de:
 * - Cargar y cachear archivos fuente con soporte completo para C++20
 * - Sistema avanzado de búsqueda de includes con caché
 * - Control de codificaciones y normalización de finales de línea
 * - Preservación de mapeos para preprocesador y módulos
 * - Soporte para header units y BMI
 * - Integración con sistema de módulos
 */
class SourceManager {
public:
    SourceManager();
    ~SourceManager();

    // === CARGA DE ARCHIVOS ===

    /**
     * @brief Carga un archivo fuente con todas las optimizaciones
     * @param path Ruta del archivo
     * @param displayName Nombre para mostrar (opcional)
     * @param isHeaderUnit Si es una unidad de encabezado
     * @return ID del archivo o 0 si falló
     */
    uint32_t loadFile(const std::filesystem::path& path,
                     const std::string& displayName = "",
                     bool isHeaderUnit = false);

    /**
     * @brief Crea un archivo fuente virtual
     * @param content Contenido del archivo
     * @param displayName Nombre para mostrar
     * @return ID del archivo
     */
    uint32_t createVirtualFile(std::string content,
                              const std::string& displayName);

    /**
     * @brief Precarga archivos de encabezado comunes
     * @param paths Lista de rutas de encabezados
     */
    void preloadHeaders(const std::vector<std::filesystem::path>& paths);

    // === GESTIÓN DE INCLUDES ===

    /**
     * @brief Busca y carga un archivo de include
     * @param includeName Nombre del include (con o sin <> o "")
     * @param currentFileId ID del archivo que hace el include
     * @param isSystemInclude Si es un include de sistema
     * @return ID del archivo incluido o 0 si no encontrado
     */
    uint32_t findAndLoadInclude(const std::string& includeName,
                               uint32_t currentFileId,
                               bool isSystemInclude);

    /**
     * @brief Configura rutas de búsqueda de includes
     * @param searchPath Sistema de búsqueda configurado
     */
    void setIncludeSearchPath(const IncludeSearchPath& searchPath);

    /**
     * @brief Añade una ruta de búsqueda de includes
     * @param path Ruta a añadir
     * @param isSystemPath Si es una ruta de sistema
     */
    void addIncludePath(const std::filesystem::path& path, bool isSystemPath = false);

    // === ACCESO A ARCHIVOS ===

    /**
     * @brief Obtiene información de un archivo
     * @param fileId ID del archivo
     * @return Puntero al archivo o nullptr si no existe
     */
    const SourceFile* getFile(uint32_t fileId) const;

    /**
     * @brief Obtiene información de un archivo por ubicación
     * @param location Ubicación en el código
     * @return Puntero al archivo o nullptr si no existe
     */
    const SourceFile* getFileForLocation(const SourceLocation& location) const;

    // === CONVERSIONES DE UBICACIÓN ===

    /**
     * @brief Convierte offset absoluto a SourceLocation
     * @param fileId ID del archivo
     * @param offset Offset absoluto en el archivo
     * @return SourceLocation correspondiente
     */
    SourceLocation getLocation(uint32_t fileId, uint32_t offset) const;

    /**
     * @brief Convierte SourceLocation a offset absoluto
     * @param location Ubicación en el código
     * @return Offset absoluto o 0 si inválido
     */
    uint32_t getOffset(const SourceLocation& location) const;

    /**
     * @brief Obtiene ubicación original mapeada por preprocesador
     * @param location Ubicación actual
     * @return Ubicación original o la misma si no hay mapeo
     */
    SourceLocation getOriginalLocation(const SourceLocation& location) const;

    // === ACCESO A TEXTO ===

    /**
     * @brief Obtiene texto de un rango
     * @param range Rango de código
     * @return Texto del rango o string vacío si inválido
     */
    std::string getText(const SourceRange& range) const;

    /**
     * @brief Obtiene línea completa que contiene una ubicación
     * @param location Ubicación en el código
     * @return Texto de la línea o string vacío si inválido
     */
    std::string getLine(const SourceLocation& location) const;

    /**
     * @brief Obtiene múltiples líneas alrededor de una ubicación
     * @param location Ubicación central
     * @param beforeLines Número de líneas antes
     * @param afterLines Número de líneas después
     * @return Texto de las líneas con numeración
     */
    std::string getContextLines(const SourceLocation& location,
                               int beforeLines = 1,
                               int afterLines = 1) const;

    // === SOPORTE PARA PREPROCESADOR ===

    /**
     * @brief Registra expansión de macro
     * @param location Ubicación de la expansión
     * @param expansion Texto de la expansión
     */
    void registerMacroExpansion(const SourceLocation& location, const std::string& expansion);

    /**
     * @brief Registra mapeo de línea (#line directive)
     * @param currentLocation Ubicación actual
     * @param originalLocation Ubicación original
     */
    void registerLineMapping(const SourceLocation& currentLocation,
                           const SourceLocation& originalLocation);

    // === GESTIÓN DE HEADER UNITS ===

    /**
     * @brief Marca un archivo como header unit
     * @param fileId ID del archivo
     * @param moduleName Nombre del módulo
     */
    void markAsHeaderUnit(uint32_t fileId, const std::string& moduleName);

    /**
     * @brief Verifica si un archivo es header unit
     * @param fileId ID del archivo
     * @return true si es header unit
     */
    bool isHeaderUnit(uint32_t fileId) const;

    // === ESTADÍSTICAS Y GESTIÓN ===

    // Estadísticas
    size_t fileCount() const { return files_.size(); }
    size_t totalSize() const;
    size_t cacheHitCount() const { return cacheHits_; }
    size_t cacheMissCount() const { return cacheMisses_; }

    // Gestión de memoria
    void clearCache();
    void clearIncludeCache();
    void preloadFiles(const std::vector<std::filesystem::path>& paths);

    // Utilidades
    std::string getDisplayName(uint32_t fileId) const;
    bool isValidFileId(uint32_t fileId) const;
    bool isValidLocation(const SourceLocation& location) const;
    Encoding detectEncoding(const std::string& content) const;
    std::string normalizeLineEndings(const std::string& content) const;

private:
    std::vector<std::unique_ptr<SourceFile>> files_;
    std::unordered_map<std::filesystem::path, uint32_t> pathToId_;
    std::unordered_map<std::string, IncludeCacheEntry> includeCache_;
    IncludeSearchPath includeSearchPath_;
    uint32_t nextFileId_ = 1;  // 0 es inválido

    // Estadísticas de caché
    size_t cacheHits_ = 0;
    size_t cacheMisses_ = 0;

    // Métodos internos
    uint32_t assignFileId();
    std::vector<uint32_t> computeLineOffsets(const std::string& content) const;
    bool loadFileContent(const std::filesystem::path& path, std::string& content,
                        Encoding& encoding, std::filesystem::file_time_type& lastModified) const;
    std::string readFileToString(const std::filesystem::path& path) const;
    std::string decodeContent(const std::string& rawContent, Encoding encoding) const;
    std::string computeContentHash(const std::string& content) const;
    bool isCacheValid(const std::filesystem::path& path, const IncludeCacheEntry& entry) const;
    void updateIncludeCache(const std::string& includeName, const std::filesystem::path& resolvedPath,
                           uint32_t fileId);
};

} // namespace cpp20::compiler::diagnostics
