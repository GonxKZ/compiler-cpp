/**
 * @file TemplateCache.h
 * @brief Sistema de caché para instanciaciones de plantillas
 */

#pragma once

#include <compiler/types/Type.h>
#include <compiler/ast/ASTNode.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <filesystem>

namespace cpp20::compiler {

/**
 * @brief Clave de caché para instanciaciones de plantillas
 */
struct TemplateInstantiationKey {
    std::string templateName;           // Nombre de la plantilla
    std::vector<Type> argumentTypes;    // Tipos de los argumentos
    std::string sourceLocation;         // Ubicación en el código fuente
    std::string compilationContext;     // Contexto de compilación (flags, etc.)

    bool operator==(const TemplateInstantiationKey& other) const {
        return templateName == other.templateName &&
               argumentTypes == other.argumentTypes &&
               sourceLocation == other.sourceLocation &&
               compilationContext == other.compilationContext;
    }
};

/**
 * @brief Valor de caché para instanciaciones de plantillas
 */
struct TemplateInstantiationValue {
    std::unique_ptr<ast::ASTNode> instantiatedAST;  // AST instanciado
    std::chrono::system_clock::time_point timestamp; // Timestamp de creación
    size_t memorySize;                              // Tamaño en memoria
    std::vector<std::string> dependencies;          // Dependencias de esta instanciación
    bool isValid;                                   // Si la instanciación es válida

    TemplateInstantiationValue()
        : timestamp(std::chrono::system_clock::now()),
          memorySize(0), isValid(true) {}
};

/**
 * @brief Hash function para TemplateInstantiationKey
 */
struct TemplateInstantiationKeyHash {
    size_t operator()(const TemplateInstantiationKey& key) const {
        size_t hash = std::hash<std::string>()(key.templateName);

        for (const auto& type : key.argumentTypes) {
            hash ^= std::hash<std::string>()(type.toString());
        }

        hash ^= std::hash<std::string>()(key.sourceLocation);
        hash ^= std::hash<std::string>()(key.compilationContext);

        return hash;
    }
};

/**
 * @brief Estadísticas del caché de plantillas
 */
struct TemplateCacheStats {
    size_t totalInstantiations;         // Total de instanciaciones
    size_t cacheHits;                   // Aciertos de caché
    size_t cacheMisses;                 // Fallos de caché
    size_t memoryUsed;                  // Memoria usada
    size_t maxMemory;                   // Memoria máxima permitida
    double hitRate;                     // Tasa de aciertos (%)

    TemplateCacheStats()
        : totalInstantiations(0), cacheHits(0), cacheMisses(0),
          memoryUsed(0), maxMemory(100 * 1024 * 1024), hitRate(0.0) {}

    void updateHitRate() {
        if (totalInstantiations > 0) {
            hitRate = (static_cast<double>(cacheHits) / totalInstantiations) * 100.0;
        }
    }
};

/**
 * @brief Caché para instanciaciones de plantillas
 */
class TemplateInstantiationCache {
public:
    /**
     * @brief Constructor
     */
    TemplateInstantiationCache(size_t maxMemory = 100 * 1024 * 1024); // 100MB por defecto

    /**
     * @brief Destructor
     */
    ~TemplateInstantiationCache();

    /**
     * @brief Busca una instanciación en el caché
     */
    const TemplateInstantiationValue* lookup(const TemplateInstantiationKey& key) const;

    /**
     * @brief Almacena una instanciación en el caché
     */
    void store(const TemplateInstantiationKey& key,
               std::unique_ptr<ast::ASTNode> instantiatedAST,
               const std::vector<std::string>& dependencies = {});

    /**
     * @brief Verifica si una clave existe en el caché
     */
    bool contains(const TemplateInstantiationKey& key) const;

    /**
     * @brief Invalida una entrada del caché
     */
    void invalidate(const TemplateInstantiationKey& key);

    /**
     * @brief Limpia el caché completo
     */
    void clear();

    /**
     * @brief Obtiene estadísticas del caché
     */
    const TemplateCacheStats& getStats() const { return stats_; }

    /**
     * @brief Establece límite de memoria
     */
    void setMaxMemory(size_t maxMemory);

    /**
     * @brief Realiza limpieza automática basada en LRU
     */
    void performLRUCleanup();

    /**
     * @brief Serializa el caché a disco
     */
    bool serializeToFile(const std::filesystem::path& filePath) const;

    /**
     * @brief Deserializa el caché desde disco
     */
    bool deserializeFromFile(const std::filesystem::path& filePath);

    /**
     * @brief Habilita/deshabilita el caché
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Verifica si el caché está habilitado
     */
    bool isEnabled() const { return enabled_; }

private:
    bool enabled_;
    size_t maxMemory_;
    mutable std::mutex mutex_;

    std::unordered_map<TemplateInstantiationKey,
                       std::unique_ptr<TemplateInstantiationValue>,
                       TemplateInstantiationKeyHash> cache_;

    // Para LRU eviction
    std::unordered_map<TemplateInstantiationKey,
                       std::chrono::system_clock::time_point,
                       TemplateInstantiationKeyHash> accessTimes_;

    mutable TemplateCacheStats stats_;

    /**
     * @brief Actualiza estadísticas
     */
    void updateStats();

    /**
     * @brief Calcula tamaño de memoria de una entrada
     */
    size_t calculateEntrySize(const TemplateInstantiationValue& value) const;

    /**
     * @brief Encuentra la entrada más antigua para eviction
     */
    TemplateInstantiationKey findLRUEntry() const;

    /**
     * @brief Verifica si se necesita limpieza
     */
    bool needsCleanup() const;
};

/**
 * @brief Clave de caché para evaluaciones constexpr
 */
struct ConstexprEvaluationKey {
    std::string expression;             // Expresión a evaluar
    std::string context;                // Contexto (función, template, etc.)
    std::unordered_map<std::string, Type> parameters; // Parámetros y sus tipos
    std::string compilationFlags;       // Flags de compilación que afectan la evaluación

    bool operator==(const ConstexprEvaluationKey& other) const {
        return expression == other.expression &&
               context == other.context &&
               parameters == other.parameters &&
               compilationFlags == other.compilationFlags;
    }
};

/**
 * @brief Valor de caché para evaluaciones constexpr
 */
struct ConstexprEvaluationValue {
    std::string result;                 // Resultado de la evaluación
    bool isConstant;                    // Si el resultado es constante
    std::chrono::system_clock::time_point timestamp;
    size_t evaluationSteps;            // Número de pasos de evaluación
    bool evaluationSucceeded;          // Si la evaluación fue exitosa
    std::string errorMessage;          // Mensaje de error si falló

    ConstexprEvaluationValue()
        : isConstant(false), timestamp(std::chrono::system_clock::now()),
          evaluationSteps(0), evaluationSucceeded(false) {}
};

/**
 * @brief Hash function para ConstexprEvaluationKey
 */
struct ConstexprEvaluationKeyHash {
    size_t operator()(const ConstexprEvaluationKey& key) const {
        size_t hash = std::hash<std::string>()(key.expression);
        hash ^= std::hash<std::string>()(key.context);
        hash ^= std::hash<std::string>()(key.compilationFlags);

        for (const auto& [name, type] : key.parameters) {
            hash ^= std::hash<std::string>()(name);
            hash ^= std::hash<std::string>()(type.toString());
        }

        return hash;
    }
};

/**
 * @brief Estadísticas del caché constexpr
 */
struct ConstexprCacheStats {
    size_t totalEvaluations;
    size_t cacheHits;
    size_t cacheMisses;
    size_t failedEvaluations;
    double hitRate;

    ConstexprCacheStats()
        : totalEvaluations(0), cacheHits(0), cacheMisses(0),
          failedEvaluations(0), hitRate(0.0) {}

    void updateHitRate() {
        if (totalEvaluations > 0) {
            hitRate = (static_cast<double>(cacheHits) / totalEvaluations) * 100.0;
        }
    }
};

/**
 * @brief Caché para evaluaciones constexpr
 */
class ConstexprEvaluationCache {
public:
    /**
     * @brief Constructor
     */
    ConstexprEvaluationCache(size_t maxEntries = 10000);

    /**
     * @brief Destructor
     */
    ~ConstexprEvaluationCache();

    /**
     * @brief Busca una evaluación en el caché
     */
    const ConstexprEvaluationValue* lookup(const ConstexprEvaluationKey& key) const;

    /**
     * @brief Almacena una evaluación en el caché
     */
    void store(const ConstexprEvaluationKey& key,
               const std::string& result,
               bool isConstant,
               size_t evaluationSteps,
               bool evaluationSucceeded,
               const std::string& errorMessage = "");

    /**
     * @brief Verifica si una clave existe en el caché
     */
    bool contains(const ConstexprEvaluationKey& key) const;

    /**
     * @brief Invalida una entrada del caché
     */
    void invalidate(const ConstexprEvaluationKey& key);

    /**
     * @brief Limpia el caché completo
     */
    void clear();

    /**
     * @brief Obtiene estadísticas del caché
     */
    const ConstexprCacheStats& getStats() const { return stats_; }

    /**
     * @brief Establece límite de entradas
     */
    void setMaxEntries(size_t maxEntries);

    /**
     * @brief Serializa el caché a disco
     */
    bool serializeToFile(const std::filesystem::path& filePath) const;

    /**
     * @brief Deserializa el caché desde disco
     */
    bool deserializeFromFile(const std::filesystem::path& filePath);

    /**
     * @brief Habilita/deshabilita el caché
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Verifica si el caché está habilitado
     */
    bool isEnabled() const { return enabled_; }

private:
    bool enabled_;
    size_t maxEntries_;
    mutable std::mutex mutex_;

    std::unordered_map<ConstexprEvaluationKey,
                       std::unique_ptr<ConstexprEvaluationValue>,
                       ConstexprEvaluationKeyHash> cache_;

    mutable ConstexprCacheStats stats_;

    /**
     * @brief Actualiza estadísticas
     */
    void updateStats();

    /**
     * @brief Realiza limpieza por LRU si es necesario
     */
    void performLRUCleanup();
};

/**
 * @brief Sistema de caché unificado para templates y constexpr
 */
class UnifiedCache {
public:
    /**
     * @brief Constructor
     */
    UnifiedCache(size_t templateCacheMemory = 100 * 1024 * 1024,
                 size_t constexprCacheEntries = 10000);

    /**
     * @brief Destructor
     */
    ~UnifiedCache();

    /**
     * @brief Obtiene el caché de plantillas
     */
    TemplateInstantiationCache& getTemplateCache() { return templateCache_; }

    /**
     * @brief Obtiene el caché constexpr
     */
    ConstexprEvaluationCache& getConstexprCache() { return constexprCache_; }

    /**
     * @brief Carga cachés desde archivos
     */
    bool loadFromFiles(const std::filesystem::path& templateCacheFile,
                      const std::filesystem::path& constexprCacheFile);

    /**
     * @brief Guarda cachés a archivos
     */
    bool saveToFiles(const std::filesystem::path& templateCacheFile,
                    const std::filesystem::path& constexprCacheFile);

    /**
     * @brief Limpia todos los cachés
     */
    void clearAll();

    /**
     * @brief Obtiene estadísticas unificadas
     */
    std::string getUnifiedStats() const;

    /**
     * @brief Habilita/deshabilita todos los cachés
     */
    void setEnabled(bool enabled);

private:
    TemplateInstantiationCache templateCache_;
    ConstexprEvaluationCache constexprCache_;
};

} // namespace cpp20::compiler
