/**
 * @file ModuleSystem.h
 * @brief Sistema completo de módulos C++20 con BMI
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>

namespace cpp20::compiler::modules {

// ============================================================================
// Forward declarations
// ============================================================================

class ModuleInterface;
class ModuleImplementation;
class BinaryModuleInterface;
class ModuleDependencyScanner;
class ModuleCache;
class ModuleLoader;

// ============================================================================
// Enums y tipos básicos
// ============================================================================

/**
 * @brief Tipo de módulo
 */
enum class ModuleType {
    Interface,      // module interface (export module)
    Implementation, // module implementation
    Partition,      // module partition
    Global          // global module fragment
};

/**
 * @brief Estado de un módulo
 */
enum class ModuleState {
    Discovered,     // Descubierto pero no procesado
    Scanning,       // Escaneando dependencias
    InterfacesReady,// Interfaces listas
    Compiling,      // Compilando
    Ready,          // Listo para usar
    Error           // Error en procesamiento
};

/**
 * @brief Tipo de entidad exportada
 */
enum class ExportType {
    Type,           // Tipo (class, struct, enum)
    Function,       // Función
    Variable,       // Variable
    Template,       // Template
    Namespace,      // Namespace
    Concept         // Concept (C++20)
};

// ============================================================================
// Estructuras de datos
// ============================================================================

/**
 * @brief Información de una entidad exportada
 */
struct ExportedEntity {
    std::string name;
    std::string qualifiedName;
    ExportType type;
    std::string sourceLocation;
    bool isInline = false;
    bool isConstexpr = false;

    ExportedEntity(const std::string& n, const std::string& qn, ExportType t,
                  const std::string& loc = "")
        : name(n), qualifiedName(qn), type(t), sourceLocation(loc) {}
};

/**
 * @brief Información de dependencia entre módulos
 */
struct ModuleDependency {
    std::string moduleName;
    bool isInterface = true;  // true para import, false para header unit
    std::string sourceLocation;

    ModuleDependency(const std::string& name, bool iface, const std::string& loc = "")
        : moduleName(name), isInterface(iface), sourceLocation(loc) {}
};

/**
 * @brief Hash de opciones de compilación
 */
struct CompilationOptionsHash {
    size_t preprocessorHash = 0;
    size_t compilerFlagsHash = 0;
    size_t systemIncludesHash = 0;

    size_t combined() const {
        return preprocessorHash ^ (compilerFlagsHash << 1) ^ (systemIncludesHash << 2);
    }
};

// ============================================================================
// Binary Module Interface (BMI)
// ============================================================================

/**
 * @brief Formato compacto de BMI
 */
class BinaryModuleInterface {
public:
    /**
     * @brief Constructor
     */
    BinaryModuleInterface(const std::string& moduleName);

    /**
     * @brief Destructor
     */
    ~BinaryModuleInterface() = default;

    /**
     * @brief Serializar BMI a bytes
     */
    std::vector<uint8_t> serialize() const;

    /**
     * @brief Deserializar BMI desde bytes
     */
    static std::unique_ptr<BinaryModuleInterface> deserialize(const std::vector<uint8_t>& data);

    /**
     * @brief Verificar si BMI es válido
     */
    bool isValid() const;

    /**
     * @brief Obtener nombre del módulo
     */
    const std::string& getModuleName() const { return moduleName_; }

    /**
     * @brief Agregar entidad exportada
     */
    void addExportedEntity(const ExportedEntity& entity);

    /**
     * @brief Obtener entidades exportadas
     */
    const std::vector<ExportedEntity>& getExportedEntities() const;

    /**
     * @brief Agregar dependencia
     */
    void addDependency(const ModuleDependency& dep);

    /**
     * @brief Obtener dependencias
     */
    const std::vector<ModuleDependency>& getDependencies() const;

    /**
     * @brief Establecer hash de opciones
     */
    void setCompilationOptionsHash(const CompilationOptionsHash& hash);

    /**
     * @brief Obtener hash de opciones
     */
    const CompilationOptionsHash& getCompilationOptionsHash() const;

private:
    std::string moduleName_;
    std::vector<ExportedEntity> exportedEntities_;
    std::vector<ModuleDependency> dependencies_;
    CompilationOptionsHash optionsHash_;
    uint32_t version_ = 1;
    uint64_t timestamp_ = 0;
};

// ============================================================================
// Module Interface
// ============================================================================

/**
 * @brief Representa una interfaz de módulo (export module)
 */
class ModuleInterface {
public:
    /**
     * @brief Constructor
     */
    ModuleInterface(const std::string& moduleName, const std::filesystem::path& sourcePath);

    /**
     * @brief Destructor
     */
    ~ModuleInterface() = default;

    /**
     * @brief Obtener nombre del módulo
     */
    const std::string& getModuleName() const { return moduleName_; }

    /**
     * @brief Obtener ruta del archivo fuente
     */
    const std::filesystem::path& getSourcePath() const { return sourcePath_; }

    /**
     * @brief Agregar partición
     */
    void addPartition(const std::string& partitionName);

    /**
     * @brief Obtener particiones
     */
    const std::vector<std::string>& getPartitions() const;

    /**
     * @brief Establecer BMI
     */
    void setBMI(std::unique_ptr<BinaryModuleInterface> bmi);

    /**
     * @brief Obtener BMI
     */
    const BinaryModuleInterface* getBMI() const;

    /**
     * @brief Verificar si está listo
     */
    bool isReady() const;

private:
    std::string moduleName_;
    std::filesystem::path sourcePath_;
    std::vector<std::string> partitions_;
    std::unique_ptr<BinaryModuleInterface> bmi_;
};

// ============================================================================
// Module Implementation
// ============================================================================

/**
 * @brief Representa una implementación de módulo
 */
class ModuleImplementation {
public:
    /**
     * @brief Constructor
     */
    ModuleImplementation(const std::string& moduleName, const std::filesystem::path& sourcePath);

    /**
     * @brief Destructor
     */
    ~ModuleImplementation() = default;

    /**
     * @brief Obtener nombre del módulo
     */
    const std::string& getModuleName() const { return moduleName_; }

    /**
     * @brief Obtener ruta del archivo fuente
     */
    const std::filesystem::path& getSourcePath() const { return sourcePath_; }

    /**
     * @brief Agregar dependencia
     */
    void addDependency(const std::string& moduleName);

    /**
     * @brief Obtener dependencias
     */
    const std::vector<std::string>& getDependencies() const;

private:
    std::string moduleName_;
    std::filesystem::path sourcePath_;
    std::vector<std::string> dependencies_;
};

// ============================================================================
// Module Dependency Scanner
// ============================================================================

/**
 * @brief Escáner de dependencias de módulos
 */
class ModuleDependencyScanner {
public:
    /**
     * @brief Constructor
     */
    ModuleDependencyScanner();

    /**
     * @brief Destructor
     */
    ~ModuleDependencyScanner() = default;

    /**
     * @brief Escanear archivo fuente para dependencias
     */
    std::vector<ModuleDependency> scanFile(const std::filesystem::path& filePath);

    /**
     * @brief Verificar si archivo contiene módulo
     */
    bool containsModuleDeclaration(const std::filesystem::path& filePath);

    /**
     * @brief Extraer nombre del módulo desde declaración
     */
    std::string extractModuleName(const std::string& line);

    /**
     * @brief Extraer import desde línea
     */
    std::string extractImportName(const std::string& line);

    /**
     * @brief Verificar si es header unit
     */
    bool isHeaderUnit(const std::string& importName);

private:
    // Cache de archivos escaneados
    std::unordered_map<std::string, std::vector<ModuleDependency>> scanCache_;
};

// ============================================================================
// Module Cache
// ============================================================================

/**
 * @brief Sistema de cache para BMI
 */
class ModuleCache {
public:
    /**
     * @brief Constructor
     */
    ModuleCache(const std::filesystem::path& cacheDir);

    /**
     * @brief Destructor
     */
    ~ModuleCache() = default;

    /**
     * @brief Almacenar BMI en cache
     */
    bool store(const std::string& moduleName, const BinaryModuleInterface& bmi);

    /**
     * @brief Recuperar BMI desde cache
     */
    std::unique_ptr<BinaryModuleInterface> retrieve(const std::string& moduleName);

    /**
     * @brief Verificar si BMI está en cache y es válido
     */
    bool isValid(const std::string& moduleName, const CompilationOptionsHash& currentHash);

    /**
     * @brief Invalidar entrada de cache
     */
    void invalidate(const std::string& moduleName);

    /**
     * @brief Limpiar cache completo
     */
    void clear();

    /**
     * @brief Obtener estadísticas de cache
     */
    struct CacheStats {
        size_t totalEntries = 0;
        size_t hits = 0;
        size_t misses = 0;
        size_t invalidations = 0;
    };
    CacheStats getStats() const;

private:
    std::filesystem::path cacheDir_;
    CacheStats stats_;

    std::string generateCacheKey(const std::string& moduleName) const;
    std::filesystem::path getCacheFilePath(const std::string& key) const;
};

// ============================================================================
// Module Loader
// ============================================================================

/**
 * @brief Carga y administra módulos
 */
class ModuleLoader {
public:
    /**
     * @brief Constructor
     */
    ModuleLoader(std::shared_ptr<ModuleCache> cache);

    /**
     * @brief Destructor
     */
    ~ModuleLoader() = default;

    /**
     * @brief Cargar módulo
     */
    std::unique_ptr<ModuleInterface> loadModule(const std::string& moduleName,
                                               const std::filesystem::path& sourcePath);

    /**
     * @brief Verificar si módulo está cargado
     */
    bool isModuleLoaded(const std::string& moduleName) const;

    /**
     * @brief Obtener módulo cargado
     */
    const ModuleInterface* getModule(const std::string& moduleName) const;

    /**
     * @brief Descargar módulo
     */
    void unloadModule(const std::string& moduleName);

private:
    std::shared_ptr<ModuleCache> cache_;
    std::unordered_map<std::string, std::unique_ptr<ModuleInterface>> loadedModules_;
};

// ============================================================================
// Module System (Clase principal)
// ============================================================================

/**
 * @brief Sistema completo de módulos C++20
 */
class ModuleSystem {
public:
    /**
     * @brief Constructor
     */
    ModuleSystem(const std::filesystem::path& cacheDir = "module_cache");

    /**
     * @brief Destructor
     */
    ~ModuleSystem() = default;

    /**
     * @brief Inicializar sistema
     */
    bool initialize();

    /**
     * @brief Procesar archivo fuente
     */
    bool processSourceFile(const std::filesystem::path& sourcePath);

    /**
     * @brief Compilar módulo
     */
    bool compileModule(const std::string& moduleName);

    /**
     * @brief Obtener dependencias de módulo
     */
    std::vector<std::string> getModuleDependencies(const std::string& moduleName);

    /**
     * @brief Verificar si módulo existe
     */
    bool moduleExists(const std::string& moduleName) const;

    /**
     * @brief Obtener estadísticas del sistema
     */
    struct SystemStats {
        size_t totalModules = 0;
        size_t interfacesCompiled = 0;
        size_t implementationsCompiled = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
    };
    SystemStats getStats() const;

    /**
     * @brief Limpiar cache
     */
    void clearCache();

private:
    std::shared_ptr<ModuleDependencyScanner> scanner_;
    std::shared_ptr<ModuleCache> cache_;
    std::shared_ptr<ModuleLoader> loader_;

    std::unordered_map<std::string, std::unique_ptr<ModuleInterface>> interfaces_;
    std::unordered_map<std::string, std::unique_ptr<ModuleImplementation>> implementations_;

    SystemStats stats_;

    bool processModuleDeclaration(const std::filesystem::path& filePath,
                                const std::string& moduleName);
    bool processImportDeclaration(const std::filesystem::path& filePath,
                                const std::string& importName);
    std::vector<std::string> computeCompilationOrder(const std::string& targetModule);
};

} // namespace cpp20::compiler::modules
