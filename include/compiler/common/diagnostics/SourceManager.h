#pragma once

#include "SourceLocation.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Información sobre un archivo fuente
 */
struct SourceFile {
    uint32_t id;                           // ID único del archivo
    std::filesystem::path path;           // Ruta completa del archivo
    std::string content;                  // Contenido completo del archivo
    std::vector<uint32_t> lineOffsets;    // Offsets de inicio de cada línea
    std::string displayName;              // Nombre para mostrar (puede ser relativo)
    bool isPreprocessed = false;          // Si el contenido ya está preprocesado

    SourceFile(uint32_t id, std::filesystem::path path, std::string content);

    // Utilidades
    uint32_t lineCount() const { return static_cast<uint32_t>(lineOffsets.size()); }
    SourceLocation locationForOffset(uint32_t offset) const;
    uint32_t offsetForLocation(const SourceLocation& location) const;
    std::string getLine(uint32_t lineNumber) const;
    std::string getText(SourceRange range) const;
};

/**
 * @brief Administrador de archivos fuente
 *
 * El SourceManager es responsable de:
 * - Cargar y cachear archivos fuente
 * - Mantener el mapeo entre IDs de archivo y contenido
 * - Proporcionar servicios de localización de código
 * - Gestionar memoria de archivos fuente
 * - Soportar archivos virtuales (generados por el compilador)
 */
class SourceManager {
public:
    SourceManager();
    ~SourceManager();

    /**
     * @brief Carga un archivo fuente
     * @param path Ruta del archivo
     * @param displayName Nombre para mostrar (opcional)
     * @return ID del archivo o 0 si falló
     */
    uint32_t loadFile(const std::filesystem::path& path,
                     const std::string& displayName = "");

    /**
     * @brief Crea un archivo fuente virtual
     * @param content Contenido del archivo
     * @param displayName Nombre para mostrar
     * @return ID del archivo
     */
    uint32_t createVirtualFile(std::string content,
                              const std::string& displayName);

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

    // Estadísticas
    size_t fileCount() const { return files_.size(); }
    size_t totalSize() const;

    // Gestión de memoria
    void clearCache();
    void preloadFiles(const std::vector<std::filesystem::path>& paths);

    // Utilidades
    std::string getDisplayName(uint32_t fileId) const;
    bool isValidFileId(uint32_t fileId) const;
    bool isValidLocation(const SourceLocation& location) const;

private:
    std::vector<std::unique_ptr<SourceFile>> files_;
    std::unordered_map<std::filesystem::path, uint32_t> pathToId_;
    uint32_t nextFileId_ = 1;  // 0 es inválido

    // Métodos internos
    uint32_t assignFileId();
    std::vector<uint32_t> computeLineOffsets(const std::string& content) const;
    bool loadFileContent(const std::filesystem::path& path, std::string& content) const;
    std::string readFileToString(const std::filesystem::path& path) const;
};

} // namespace cpp20::compiler::diagnostics
