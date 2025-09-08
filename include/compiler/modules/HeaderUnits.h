/**
 * @file HeaderUnits.h
 * @brief Sistema de Header Units para C++20
 */

#pragma once

#include <compiler/modules/BinaryModuleInterface.h>
#include <compiler/ast/ASTNode.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <filesystem>
#include <chrono>

namespace cpp20::compiler::modules {

/**
 * @brief Información de un header unit
 */
struct HeaderUnit {
    std::filesystem::path headerPath;           // Ruta del header
    std::string headerName;                     // Nombre lógico del header
    std::string contentHash;                    // Hash del contenido
    std::chrono::system_clock::time_point lastModified; // Última modificación
    std::unique_ptr<BinaryModuleInterface> bmi; // BMI compilado
    std::vector<std::string> dependencies;      // Headers que importa
    bool isCompiled;                            // Si está compilado
    bool needsRebuild;                          // Si necesita recompilación

    HeaderUnit(const std::filesystem::path& path = "",
               const std::string& name = "")
        : headerPath(path), headerName(name), isCompiled(false), needsRebuild(true) {}
};

/**
 * @brief Información de dependencia de header
 */
struct HeaderDependency {
    std::string fromHeader;                     // Header que importa
    std::string toHeader;                       // Header importado
    DependencyType type;                        // Tipo de dependencia

    HeaderDependency(const std::string& from = "",
                    const std::string& to = "",
                    DependencyType t = DependencyType::Direct)
        : fromHeader(from), toHeader(to), type(t) {}
};

/**
 * @brief Tipo de dependencia
 */
enum class DependencyType {
    Direct,         // #include directo
    Indirect,       // #include indirecto a través de otros headers
    System,         // Header del sistema
    Module          // Dependencia de módulo
};

/**
 * @brief Estado de compilación de header units
 */
enum class CompilationState {
    NotCompiled,    // No compilado
    Compiling,      // En proceso de compilación
    Compiled,       // Compilado exitosamente
    Failed,         // Falló la compilación
    Outdated        // Necesita recompilación
};

/**
 * @brief Compilador de header units
 */
class HeaderUnitCompiler {
public:
    /**
     * @brief Constructor
     */
    HeaderUnitCompiler();

    /**
     * @brief Destructor
     */
    ~HeaderUnitCompiler();

    /**
     * @brief Compila un header en una unidad
     */
    std::unique_ptr<BinaryModuleInterface> compileHeaderUnit(
        const std::filesystem::path& headerPath,
        const std::vector<std::filesystem::path>& includePaths = {});

    /**
     * @brief Verifica si un header puede compilarse como unidad
     */
    bool canCompileAsHeaderUnit(const std::filesystem::path& headerPath) const;

    /**
     * @brief Obtiene dependencias de un header
     */
    std::vector<std::string> getHeaderDependencies(
        const std::filesystem::path& headerPath,
        const std::vector<std::filesystem::path>& includePaths = {});

    /**
     * @brief Preprocesa un header para análisis de dependencias
     */
    std::string preprocessHeader(
        const std::filesystem::path& headerPath,
        const std::vector<std::filesystem::path>& includePaths = {});

    /**
     * @brief Establece opciones de compilación
     */
    void setCompilationOptions(const std::vector<std::string>& options);

    /**
     * @brief Obtiene opciones de compilación actuales
     */
    const std::vector<std::string>& getCompilationOptions() const { return compilationOptions_; }

private:
    std::vector<std::string> compilationOptions_;

    /**
     * @brief Analiza directivas de preprocesador
     */
    std::vector<std::string> analyzePreprocessorDirectives(const std::string& content);

    /**
     * @brief Resuelve rutas de include
     */
    std::filesystem::path resolveIncludePath(
        const std::string& includePath,
        const std::vector<std::filesystem::path>& includePaths) const;

    /**
     * @brief Verifica si un header es del sistema
     */
    bool isSystemHeader(const std::filesystem::path& headerPath) const;

    /**
     * @brief Calcula hash del contenido de un header
     */
    std::string calculateContentHash(const std::filesystem::path& headerPath) const;
};

/**
 * @brief Caché de header units
 */
class HeaderUnitCache {
public:
    /**
     * @brief Constructor
     */
    HeaderUnitCache(const std::filesystem::path& cacheDirectory = "");

    /**
     * @brief Destructor
     */
    ~HeaderUnitCache();

    /**
     * @brief Busca un header unit en el caché
     */
    std::shared_ptr<HeaderUnit> lookup(const std::filesystem::path& headerPath);

    /**
     * @brief Almacena un header unit en el caché
     */
    void store(std::shared_ptr<HeaderUnit> headerUnit);

    /**
     * @brief Verifica si un header unit está en caché y es válido
     */
    bool isCached(const std::filesystem::path& headerPath);

    /**
     * @brief Invalida un header unit del caché
     */
    void invalidate(const std::filesystem::path& headerPath);

    /**
     * @brief Limpia el caché completo
     */
    void clear();

    /**
     * @brief Obtiene estadísticas del caché
     */
    std::unordered_map<std::string, size_t> getCacheStatistics() const;

    /**
     * @brief Establece directorio de caché
     */
    void setCacheDirectory(const std::filesystem::path& cacheDir);

    /**
     * @brief Obtiene directorio de caché
     */
    const std::filesystem::path& getCacheDirectory() const { return cacheDirectory_; }

    /**
     * @brief Serializa el caché a disco
     */
    bool serializeToDisk() const;

    /**
     * @brief Deserializa el caché desde disco
     */
    bool deserializeFromDisk();

    /**
     * @brief Verifica integridad del caché
     */
    bool verifyCacheIntegrity() const;

    /**
     * @brief Lista todos los header units en caché
     */
    std::vector<std::filesystem::path> listCachedHeaders() const;

private:
    std::filesystem::path cacheDirectory_;
    std::unordered_map<std::string, std::shared_ptr<HeaderUnit>> cache_;
    mutable std::mutex mutex_;

    // Estadísticas
    size_t totalHits_;
    size_t totalMisses_;
    size_t totalInvalidations_;

    /**
     * @brief Genera clave de caché para un header
     */
    std::string generateCacheKey(const std::filesystem::path& headerPath) const;

    /**
     * @brief Genera nombre de archivo para un header unit
     */
    std::filesystem::path generateCacheFileName(const std::string& cacheKey) const;

    /**
     * @brief Verifica si un header unit en caché es válido
     */
    bool isCacheEntryValid(const std::shared_ptr<HeaderUnit>& headerUnit) const;

    /**
     * @brief Actualiza estadísticas
     */
    void updateStatistics(bool isHit);

    /**
     * @brief Limpia entradas inválidas del caché
     */
    void cleanupInvalidEntries();

    /**
     * @brief Calcula tamaño total del caché
     */
    size_t calculateCacheSize() const;
};

/**
 * @brief Gestor de dependencias de header units
 */
class HeaderDependencyManager {
public:
    /**
     * @brief Constructor
     */
    HeaderDependencyManager();

    /**
     * @brief Destructor
     */
    ~HeaderDependencyManager();

    /**
     * @brief Añade una dependencia
     */
    void addDependency(const HeaderDependency& dependency);

    /**
     * @brief Obtiene dependencias de un header
     */
    std::vector<HeaderDependency> getDependencies(const std::string& headerName) const;

    /**
     * @brief Obtiene headers que dependen de un header dado
     */
    std::vector<std::string> getDependents(const std::string& headerName) const;

    /**
     * @brief Calcula orden de compilación topológica
     */
    std::vector<std::string> calculateCompilationOrder(
        const std::vector<std::string>& headers) const;

    /**
     * @brief Detecta ciclos en dependencias
     */
    std::vector<std::vector<std::string>> detectCycles() const;

    /**
     * @brief Verifica si hay dependencias circulares
     */
    bool hasCircularDependencies() const;

    /**
     * @brief Obtiene grafo de dependencias
     */
    std::unordered_map<std::string, std::vector<std::string>> getDependencyGraph() const;

    /**
     * @brief Limpia todas las dependencias
     */
    void clear();

    /**
     * @brief Serializa dependencias a archivo
     */
    bool serializeToFile(const std::filesystem::path& filePath) const;

    /**
     * @brief Deserializa dependencias desde archivo
     */
    bool deserializeFromFile(const std::filesystem::path& filePath);

private:
    std::unordered_map<std::string, std::vector<HeaderDependency>> dependencies_;
    mutable std::mutex mutex_;

    /**
     * @brief Algoritmo de ordenamiento topológico (Kahn)
     */
    std::vector<std::string> topologicalSort(
        const std::unordered_map<std::string, std::vector<std::string>>& graph) const;

    /**
     * @brief Detecta ciclos usando DFS
     */
    void detectCyclesDFS(const std::string& node,
                        std::unordered_set<std::string>& visited,
                        std::unordered_set<std::string>& recursionStack,
                        std::vector<std::string>& currentPath,
                        std::vector<std::vector<std::string>>& cycles) const;

    /**
     * @brief Construye grafo de dependencias
     */
    std::unordered_map<std::string, std::vector<std::string>> buildDependencyGraph() const;
};

/**
 * @brief Coordinador de compilación de header units
 */
class HeaderUnitCoordinator {
public:
    /**
     * @brief Constructor
     */
    HeaderUnitCoordinator(std::shared_ptr<HeaderUnitCache> cache = nullptr,
                         std::shared_ptr<HeaderDependencyManager> depManager = nullptr);

    /**
     * @brief Destructor
     */
    ~HeaderUnitCoordinator();

    /**
     * @brief Compila múltiples header units en orden correcto
     */
    std::vector<std::shared_ptr<HeaderUnit>> compileHeaderUnits(
        const std::vector<std::filesystem::path>& headerPaths,
        const std::vector<std::filesystem::path>& includePaths = {});

    /**
     * @brief Compila header units con dependencias
     */
    std::vector<std::shared_ptr<HeaderUnit>> compileWithDependencies(
        const std::vector<std::filesystem::path>& headerPaths,
        const std::vector<std::filesystem::path>& includePaths = {});

    /**
     * @brief Verifica si todos los headers pueden compilarse
     */
    bool canCompileAll(const std::vector<std::filesystem::path>& headerPaths) const;

    /**
     * @brief Obtiene headers que necesitan recompilación
     */
    std::vector<std::filesystem::path> getOutdatedHeaders(
        const std::vector<std::filesystem::path>& headerPaths) const;

    /**
     * @brief Fuerza recompilación de headers
     */
    void forceRebuild(const std::vector<std::filesystem::path>& headerPaths);

    /**
     * @brief Obtiene estadísticas de compilación
     */
    std::unordered_map<std::string, size_t> getCompilationStatistics() const;

    /**
     * @brief Establece número máximo de hilos para compilación paralela
     */
    void setMaxParallelJobs(size_t maxJobs);

    /**
     * @brief Obtiene número máximo de hilos
     */
    size_t getMaxParallelJobs() const { return maxParallelJobs_; }

private:
    std::shared_ptr<HeaderUnitCache> cache_;
    std::shared_ptr<HeaderDependencyManager> dependencyManager_;
    std::unique_ptr<HeaderUnitCompiler> compiler_;
    size_t maxParallelJobs_;

    // Estadísticas
    size_t totalCompiled_;
    size_t totalFromCache_;
    size_t totalFailed_;

    /**
     * @brief Compila un header unit individual
     */
    std::shared_ptr<HeaderUnit> compileSingleHeaderUnit(
        const std::filesystem::path& headerPath,
        const std::vector<std::filesystem::path>& includePaths);

    /**
     * @brief Verifica si un header unit necesita recompilación
     */
    bool needsRebuild(const std::filesystem::path& headerPath) const;

    /**
     * @brief Actualiza dependencias después de compilación
     */
    void updateDependencies(const std::shared_ptr<HeaderUnit>& headerUnit);

    /**
     * @brief Compila headers en paralelo
     */
    std::vector<std::shared_ptr<HeaderUnit>> compileInParallel(
        const std::vector<std::filesystem::path>& headerPaths,
        const std::vector<std::filesystem::path>& includePaths);

    /**
     * @brief Verifica integridad de dependencias
     */
    bool verifyDependencies(const std::vector<std::shared_ptr<HeaderUnit>>& headerUnits) const;

    /**
     * @brief Actualiza estadísticas
     */
    void updateStatistics(bool fromCache, bool success);
};

/**
 * @brief Utilidades para trabajar con header units
 */
class HeaderUnitUtils {
public:
    /**
     * @brief Verifica si un archivo es un header C/C++
     */
    static bool isHeaderFile(const std::filesystem::path& filePath);

    /**
     * @brief Obtiene nombre lógico de un header
     */
    static std::string getHeaderName(const std::filesystem::path& filePath);

    /**
     * @brief Normaliza ruta de header
     */
    static std::string normalizeHeaderPath(const std::filesystem::path& filePath);

    /**
     * @brief Extrae includes de un archivo fuente
     */
    static std::vector<std::string> extractIncludes(const std::string& content);

    /**
     * @brief Verifica si un include es del sistema
     */
    static bool isSystemInclude(const std::string& includeLine);

    /**
     * @brief Convierte include relativo a absoluto
     */
    static std::filesystem::path resolveIncludePath(
        const std::string& includePath,
        const std::vector<std::filesystem::path>& includeDirs);

    /**
     * @brief Calcula hash de contenido de archivo
     */
    static std::string calculateFileHash(const std::filesystem::path& filePath);

    /**
     * @brief Compara dos archivos por contenido
     */
    static bool compareFiles(const std::filesystem::path& file1,
                           const std::filesystem::path& file2);

    /**
     * @brief Obtiene tiempo de modificación de archivo
     */
    static std::chrono::system_clock::time_point getFileModificationTime(
        const std::filesystem::path& filePath);

    /**
     * @brief Verifica si un archivo existe y es legible
     */
    static bool isValidHeaderFile(const std::filesystem::path& filePath);

    /**
     * @brief Crea directorio si no existe
     */
    static bool ensureDirectoryExists(const std::filesystem::path& directory);

    /**
     * @brief Lista archivos header en un directorio
     */
    static std::vector<std::filesystem::path> listHeaderFiles(
        const std::filesystem::path& directory);
};

} // namespace cpp20::compiler::modules
