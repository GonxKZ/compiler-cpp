/**
 * @file TemplateCache.cpp
 * @brief Implementación del sistema de caché para templates y constexpr
 */

#include <compiler/common/TemplateCache.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>

namespace cpp20::compiler {

// ============================================================================
// TemplateInstantiationKey - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// TemplateInstantiationCache - Implementación
// ============================================================================

TemplateInstantiationCache::TemplateInstantiationCache(size_t maxMemory)
    : enabled_(true), maxMemory_(maxMemory) {
}

TemplateInstantiationCache::~TemplateInstantiationCache() = default;

const TemplateInstantiationValue* TemplateInstantiationCache::lookup(
    const TemplateInstantiationKey& key) const {

    if (!enabled_) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // Actualizar tiempo de acceso para LRU
        accessTimes_[key] = std::chrono::system_clock::now();

        stats_.cacheHits++;
        stats_.totalInstantiations++;
        stats_.updateHitRate();

        return it->second.get();
    }

    stats_.cacheMisses++;
    stats_.totalInstantiations++;
    stats_.updateHitRate();

    return nullptr;
}

void TemplateInstantiationCache::store(
    const TemplateInstantiationKey& key,
    std::unique_ptr<ast::ASTNode> instantiatedAST,
    const std::vector<std::string>& dependencies) {

    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Verificar si necesitamos limpieza
    if (needsCleanup()) {
        performLRUCleanup();
    }

    auto value = std::make_unique<TemplateInstantiationValue>();
    value->instantiatedAST = std::move(instantiatedAST);
    value->dependencies = dependencies;
    value->memorySize = calculateEntrySize(*value);

    // Verificar límites de memoria
    if (stats_.memoryUsed + value->memorySize > maxMemory_) {
        // No almacenar si excede el límite
        return;
    }

    cache_[key] = std::move(value);
    accessTimes_[key] = std::chrono::system_clock::now();

    updateStats();
}

bool TemplateInstantiationCache::contains(const TemplateInstantiationKey& key) const {
    if (!enabled_) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
}

void TemplateInstantiationCache::invalidate(const TemplateInstantiationKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        stats_.memoryUsed -= it->second->memorySize;
        cache_.erase(it);
        accessTimes_.erase(key);
    }

    updateStats();
}

void TemplateInstantiationCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    cache_.clear();
    accessTimes_.clear();
    stats_ = TemplateCacheStats();
}

void TemplateInstantiationCache::setMaxMemory(size_t maxMemory) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxMemory_ = maxMemory;
    stats_.maxMemory = maxMemory;

    // Realizar limpieza si es necesario
    if (needsCleanup()) {
        performLRUCleanup();
    }
}

void TemplateInstantiationCache::performLRUCleanup() {
    while (needsCleanup() && !cache_.empty()) {
        auto lruKey = findLRUEntry();
        invalidate(lruKey);
    }
}

bool TemplateInstantiationCache::serializeToFile(const std::filesystem::path& filePath) const {
    if (!enabled_) return false;

    try {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;

        std::lock_guard<std::mutex> lock(mutex_);

        // Serializar estadísticas
        file.write(reinterpret_cast<const char*>(&stats_), sizeof(stats_));

        // Serializar número de entradas
        size_t entryCount = cache_.size();
        file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        // Serializar cada entrada
        for (const auto& [key, value] : cache_) {
            // Serializar clave
            size_t nameLen = key.templateName.size();
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(key.templateName.data(), nameLen);

            size_t argCount = key.argumentTypes.size();
            file.write(reinterpret_cast<const char*>(&argCount), sizeof(argCount));
            for (const auto& type : key.argumentTypes) {
                std::string typeStr = type.toString();
                size_t typeLen = typeStr.size();
                file.write(reinterpret_cast<const char*>(&typeLen), sizeof(typeLen));
                file.write(typeStr.data(), typeLen);
            }

            size_t locLen = key.sourceLocation.size();
            file.write(reinterpret_cast<const char*>(&locLen), sizeof(locLen));
            file.write(key.sourceLocation.data(), locLen);

            size_t ctxLen = key.compilationContext.size();
            file.write(reinterpret_cast<const char*>(&ctxLen), sizeof(ctxLen));
            file.write(key.compilationContext.data(), ctxLen);

            // Serializar valor (simplificado)
            auto timestamp = value->timestamp.time_since_epoch().count();
            file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
            file.write(reinterpret_cast<const char*>(&value->memorySize), sizeof(value->memorySize));
            file.write(reinterpret_cast<const char*>(&value->isValid), sizeof(value->isValid));

            size_t depCount = value->dependencies.size();
            file.write(reinterpret_cast<const char*>(&depCount), sizeof(depCount));
            for (const auto& dep : value->dependencies) {
                size_t depLen = dep.size();
                file.write(reinterpret_cast<const char*>(&depLen), sizeof(depLen));
                file.write(dep.data(), depLen);
            }
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool TemplateInstantiationCache::deserializeFromFile(const std::filesystem::path& filePath) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;

        std::lock_guard<std::mutex> lock(mutex_);

        // Limpiar caché actual
        clear();

        // Deserializar estadísticas
        file.read(reinterpret_cast<char*>(&stats_), sizeof(stats_));

        // Deserializar número de entradas
        size_t entryCount;
        file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

        // Deserializar cada entrada
        for (size_t i = 0; i < entryCount; ++i) {
            TemplateInstantiationKey key;

            // Deserializar clave
            size_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            key.templateName.resize(nameLen);
            file.read(key.templateName.data(), nameLen);

            size_t argCount;
            file.read(reinterpret_cast<char*>(&argCount), sizeof(argCount));
            key.argumentTypes.reserve(argCount);
            for (size_t j = 0; j < argCount; ++j) {
                size_t typeLen;
                file.read(reinterpret_cast<char*>(&typeLen), sizeof(typeLen));
                std::string typeStr(typeLen, '\0');
                file.read(typeStr.data(), typeLen);
                // En un compilador real, aquí se convertiría el string a Type
                key.argumentTypes.emplace_back(Type::fromString(typeStr));
            }

            size_t locLen;
            file.read(reinterpret_cast<char*>(&locLen), sizeof(locLen));
            key.sourceLocation.resize(locLen);
            file.read(key.sourceLocation.data(), locLen);

            size_t ctxLen;
            file.read(reinterpret_cast<char*>(&ctxLen), sizeof(ctxLen));
            key.compilationContext.resize(ctxLen);
            file.read(key.compilationContext.data(), ctxLen);

            // Deserializar valor
            auto value = std::make_unique<TemplateInstantiationValue>();

            long long timestamp;
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
            value->timestamp = std::chrono::system_clock::time_point(
                std::chrono::system_clock::duration(timestamp));

            file.read(reinterpret_cast<char*>(&value->memorySize), sizeof(value->memorySize));
            file.read(reinterpret_cast<char*>(&value->isValid), sizeof(value->isValid));

            size_t depCount;
            file.read(reinterpret_cast<char*>(&depCount), sizeof(depCount));
            value->dependencies.reserve(depCount);
            for (size_t j = 0; j < depCount; ++j) {
                size_t depLen;
                file.read(reinterpret_cast<char*>(&depLen), sizeof(depLen));
                std::string dep(depLen, '\0');
                file.read(dep.data(), depLen);
                value->dependencies.push_back(dep);
            }

            // Nota: El AST no se serializa en esta implementación simplificada
            value->instantiatedAST = nullptr;

            cache_[key] = std::move(value);
        }

        file.close();
        updateStats();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

void TemplateInstantiationCache::updateStats() {
    stats_.memoryUsed = 0;
    for (const auto& [key, value] : cache_) {
        stats_.memoryUsed += value->memorySize;
    }
}

size_t TemplateInstantiationCache::calculateEntrySize(const TemplateInstantiationValue& value) const {
    size_t size = sizeof(TemplateInstantiationValue);

    // Estimar tamaño del AST (simplificado)
    if (value.instantiatedAST) {
        size += 1024; // Estimación aproximada
    }

    // Tamaño de dependencias
    for (const auto& dep : value.dependencies) {
        size += dep.size();
    }

    return size;
}

TemplateInstantiationKey TemplateInstantiationCache::findLRUEntry() const {
    auto oldestTime = std::chrono::system_clock::now();
    TemplateInstantiationKey oldestKey;

    for (const auto& [key, time] : accessTimes_) {
        if (time < oldestTime) {
            oldestTime = time;
            oldestKey = key;
        }
    }

    return oldestKey;
}

bool TemplateInstantiationCache::needsCleanup() const {
    return stats_.memoryUsed > maxMemory_;
}

// ============================================================================
// ConstexprEvaluationCache - Implementación
// ============================================================================

ConstexprEvaluationCache::ConstexprEvaluationCache(size_t maxEntries)
    : enabled_(true), maxEntries_(maxEntries) {
}

ConstexprEvaluationCache::~ConstexprEvaluationCache() = default;

const ConstexprEvaluationValue* ConstexprEvaluationCache::lookup(
    const ConstexprEvaluationKey& key) const {

    if (!enabled_) return nullptr;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        stats_.cacheHits++;
        stats_.totalEvaluations++;
        stats_.updateHitRate();
        return it->second.get();
    }

    stats_.cacheMisses++;
    stats_.totalEvaluations++;
    stats_.updateHitRate();

    return nullptr;
}

void ConstexprEvaluationCache::store(
    const ConstexprEvaluationKey& key,
    const std::string& result,
    bool isConstant,
    size_t evaluationSteps,
    bool evaluationSucceeded,
    const std::string& errorMessage) {

    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Verificar si necesitamos limpieza
    if (cache_.size() >= maxEntries_) {
        performLRUCleanup();
    }

    auto value = std::make_unique<ConstexprEvaluationValue>();
    value->result = result;
    value->isConstant = isConstant;
    value->evaluationSteps = evaluationSteps;
    value->evaluationSucceeded = evaluationSucceeded;
    value->errorMessage = errorMessage;

    if (!evaluationSucceeded) {
        stats_.failedEvaluations++;
    }

    cache_[key] = std::move(value);
    updateStats();
}

bool ConstexprEvaluationCache::contains(const ConstexprEvaluationKey& key) const {
    if (!enabled_) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
}

void ConstexprEvaluationCache::invalidate(const ConstexprEvaluationKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.erase(key);
    updateStats();
}

void ConstexprEvaluationCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    stats_ = ConstexprCacheStats();
}

void ConstexprEvaluationCache::setMaxEntries(size_t maxEntries) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxEntries_ = maxEntries;

    if (cache_.size() > maxEntries_) {
        performLRUCleanup();
    }
}

bool ConstexprEvaluationCache::serializeToFile(const std::filesystem::path& filePath) const {
    if (!enabled_) return false;

    try {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;

        std::lock_guard<std::mutex> lock(mutex_);

        // Serializar estadísticas
        file.write(reinterpret_cast<const char*>(&stats_), sizeof(stats_));

        // Serializar número de entradas
        size_t entryCount = cache_.size();
        file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        // Serializar cada entrada
        for (const auto& [key, value] : cache_) {
            // Serializar clave
            size_t exprLen = key.expression.size();
            file.write(reinterpret_cast<const char*>(&exprLen), sizeof(exprLen));
            file.write(key.expression.data(), exprLen);

            size_t ctxLen = key.context.size();
            file.write(reinterpret_cast<const char*>(&ctxLen), sizeof(ctxLen));
            file.write(key.context.data(), ctxLen);

            size_t paramCount = key.parameters.size();
            file.write(reinterpret_cast<const char*>(&paramCount), sizeof(paramCount));
            for (const auto& [name, type] : key.parameters) {
                size_t nameLen = name.size();
                file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
                file.write(name.data(), nameLen);

                std::string typeStr = type.toString();
                size_t typeLen = typeStr.size();
                file.write(reinterpret_cast<const char*>(&typeLen), sizeof(typeLen));
                file.write(typeStr.data(), typeLen);
            }

            size_t flagsLen = key.compilationFlags.size();
            file.write(reinterpret_cast<const char*>(&flagsLen), sizeof(flagsLen));
            file.write(key.compilationFlags.data(), flagsLen);

            // Serializar valor
            size_t resultLen = value->result.size();
            file.write(reinterpret_cast<const char*>(&resultLen), sizeof(resultLen));
            file.write(value->result.data(), resultLen);

            file.write(reinterpret_cast<const char*>(&value->isConstant), sizeof(value->isConstant));

            auto timestamp = value->timestamp.time_since_epoch().count();
            file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

            file.write(reinterpret_cast<const char*>(&value->evaluationSteps), sizeof(value->evaluationSteps));
            file.write(reinterpret_cast<const char*>(&value->evaluationSucceeded), sizeof(value->evaluationSucceeded));

            size_t errorLen = value->errorMessage.size();
            file.write(reinterpret_cast<const char*>(&errorLen), sizeof(errorLen));
            file.write(value->errorMessage.data(), errorLen);
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool ConstexprEvaluationCache::deserializeFromFile(const std::filesystem::path& filePath) {
    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;

        std::lock_guard<std::mutex> lock(mutex_);

        // Limpiar caché actual
        clear();

        // Deserializar estadísticas
        file.read(reinterpret_cast<char*>(&stats_), sizeof(stats_));

        // Deserializar número de entradas
        size_t entryCount;
        file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

        // Deserializar cada entrada
        for (size_t i = 0; i < entryCount; ++i) {
            ConstexprEvaluationKey key;

            // Deserializar clave
            size_t exprLen;
            file.read(reinterpret_cast<char*>(&exprLen), sizeof(exprLen));
            key.expression.resize(exprLen);
            file.read(key.expression.data(), exprLen);

            size_t ctxLen;
            file.read(reinterpret_cast<char*>(&ctxLen), sizeof(ctxLen));
            key.context.resize(ctxLen);
            file.read(key.context.data(), ctxLen);

            size_t paramCount;
            file.read(reinterpret_cast<char*>(&paramCount), sizeof(paramCount));
            for (size_t j = 0; j < paramCount; ++j) {
                size_t nameLen;
                file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
                std::string name(nameLen, '\0');
                file.read(name.data(), nameLen);

                size_t typeLen;
                file.read(reinterpret_cast<char*>(&typeLen), sizeof(typeLen));
                std::string typeStr(typeLen, '\0');
                file.read(typeStr.data(), typeLen);

                key.parameters[name] = Type::fromString(typeStr);
            }

            size_t flagsLen;
            file.read(reinterpret_cast<char*>(&flagsLen), sizeof(flagsLen));
            key.compilationFlags.resize(flagsLen);
            file.read(key.compilationFlags.data(), flagsLen);

            // Deserializar valor
            auto value = std::make_unique<ConstexprEvaluationValue>();

            size_t resultLen;
            file.read(reinterpret_cast<char*>(&resultLen), sizeof(resultLen));
            value->result.resize(resultLen);
            file.read(value->result.data(), resultLen);

            file.read(reinterpret_cast<char*>(&value->isConstant), sizeof(value->isConstant));

            long long timestamp;
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
            value->timestamp = std::chrono::system_clock::time_point(
                std::chrono::system_clock::duration(timestamp));

            file.read(reinterpret_cast<char*>(&value->evaluationSteps), sizeof(value->evaluationSteps));
            file.read(reinterpret_cast<char*>(&value->evaluationSucceeded), sizeof(value->evaluationSucceeded));

            size_t errorLen;
            file.read(reinterpret_cast<char*>(&errorLen), sizeof(errorLen));
            value->errorMessage.resize(errorLen);
            file.read(value->errorMessage.data(), errorLen);

            cache_[key] = std::move(value);
        }

        file.close();
        updateStats();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

void ConstexprEvaluationCache::updateStats() {
    // Las estadísticas se actualizan en tiempo real en lookup() y store()
}

void ConstexprEvaluationCache::performLRUCleanup() {
    // En una implementación real, mantendríamos un timestamp de acceso
    // Por simplicidad, eliminamos entradas antiguas hasta llegar al límite

    while (cache_.size() > maxEntries_ && !cache_.empty()) {
        // Eliminar la primera entrada (simplificado)
        cache_.erase(cache_.begin());
    }
}

// ============================================================================
// UnifiedCache - Implementación
// ============================================================================

UnifiedCache::UnifiedCache(size_t templateCacheMemory, size_t constexprCacheEntries)
    : templateCache_(templateCacheMemory), constexprCache_(constexprCacheEntries) {
}

UnifiedCache::~UnifiedCache() = default;

bool UnifiedCache::loadFromFiles(const std::filesystem::path& templateCacheFile,
                                const std::filesystem::path& constexprCacheFile) {
    bool templateLoaded = templateCache_.deserializeFromFile(templateCacheFile);
    bool constexprLoaded = constexprCache_.deserializeFromFile(constexprCacheFile);

    return templateLoaded && constexprLoaded;
}

bool UnifiedCache::saveToFiles(const std::filesystem::path& templateCacheFile,
                              const std::filesystem::path& constexprCacheFile) {
    bool templateSaved = templateCache_.serializeToFile(templateCacheFile);
    bool constexprSaved = constexprCache_.serializeToFile(constexprCacheFile);

    return templateSaved && constexprSaved;
}

void UnifiedCache::clearAll() {
    templateCache_.clear();
    constexprCache_.clear();
}

std::string UnifiedCache::getUnifiedStats() const {
    std::stringstream ss;

    ss << "=== Unified Cache Statistics ===\n\n";

    ss << "Template Cache:\n";
    const auto& templateStats = templateCache_.getStats();
    ss << "  Total instantiations: " << templateStats.totalInstantiations << "\n";
    ss << "  Cache hits: " << templateStats.cacheHits << "\n";
    ss << "  Cache misses: " << templateStats.cacheMisses << "\n";
    ss << "  Hit rate: " << std::fixed << std::setprecision(1) << templateStats.hitRate << "%\n";
    ss << "  Memory used: " << templateStats.memoryUsed << " bytes\n";
    ss << "  Max memory: " << templateStats.maxMemory << " bytes\n\n";

    ss << "Constexpr Cache:\n";
    const auto& constexprStats = constexprCache_.getStats();
    ss << "  Total evaluations: " << constexprStats.totalEvaluations << "\n";
    ss << "  Cache hits: " << constexprStats.cacheHits << "\n";
    ss << "  Cache misses: " << constexprStats.cacheMisses << "\n";
    ss << "  Failed evaluations: " << constexprStats.failedEvaluations << "\n";
    ss << "  Hit rate: " << std::fixed << std::setprecision(1) << constexprStats.hitRate << "%\n";

    return ss.str();
}

void UnifiedCache::setEnabled(bool enabled) {
    templateCache_.setEnabled(enabled);
    constexprCache_.setEnabled(enabled);
}

} // namespace cpp20::compiler
