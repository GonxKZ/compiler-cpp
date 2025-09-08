/**
 * @file HeaderUnits.cpp
 * @brief Implementación del sistema de Header Units para C++20
 */

#include <compiler/modules/HeaderUnits.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <thread>
#include <future>

namespace cpp20::compiler::modules {

// ============================================================================
// HeaderUnitCompiler - Implementación
// ============================================================================

HeaderUnitCompiler::HeaderUnitCompiler() = default;

HeaderUnitCompiler::~HeaderUnitCompiler() = default;

std::unique_ptr<BinaryModuleInterface> HeaderUnitCompiler::compileHeaderUnit(
    const std::filesystem::path& headerPath,
    const std::vector<std::filesystem::path>& includePaths) {

    if (!std::filesystem::exists(headerPath)) {
        return nullptr;
    }

    // Verificar que sea un header válido
    if (!canCompileAsHeaderUnit(headerPath)) {
        return nullptr;
    }

    // Leer contenido del header
    std::ifstream file(headerPath);
    if (!file.is_open()) {
        return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Crear BMI para el header unit
    auto bmi = std::make_unique<BinaryModuleInterface>(headerPath.filename().string());

    // En un compilador real, aquí se haría el análisis sintáctico y semántico
    // del header y se generaría el BMI correspondiente

    // Por simplicidad, creamos un BMI básico
    BMIGenerator generator;
    // Nota: necesitaríamos acceso al AST del header

    return bmi;
}

bool HeaderUnitCompiler::canCompileAsHeaderUnit(const std::filesystem::path& headerPath) const {
    if (!std::filesystem::exists(headerPath)) {
        return false;
    }

    // Verificar extensión
    std::string ext = headerPath.extension().string();
    if (ext != ".h" && ext != ".hpp" && ext != ".hxx" && ext != ".h++") {
        return false;
    }

    // Verificar que sea un archivo regular
    return std::filesystem::is_regular_file(headerPath);
}

std::vector<std::string> HeaderUnitCompiler::getHeaderDependencies(
    const std::filesystem::path& headerPath,
    const std::vector<std::filesystem::path>& includePaths) {

    std::vector<std::string> dependencies;

    if (!std::filesystem::exists(headerPath)) {
        return dependencies;
    }

    // Leer contenido del header
    std::ifstream file(headerPath);
    if (!file.is_open()) {
        return dependencies;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Analizar directivas de preprocesador
    return analyzePreprocessorDirectives(content);
}

std::string HeaderUnitCompiler::preprocessHeader(
    const std::filesystem::path& headerPath,
    const std::vector<std::filesystem::path>& includePaths) {

    // En un compilador real, aquí se haría el preprocesamiento completo
    // Por simplicidad, devolvemos el contenido sin procesar

    if (!std::filesystem::exists(headerPath)) {
        return "";
    }

    std::ifstream file(headerPath);
    if (!file.is_open()) {
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    return content;
}

void HeaderUnitCompiler::setCompilationOptions(const std::vector<std::string>& options) {
    compilationOptions_ = options;
}

std::vector<std::string> HeaderUnitCompiler::analyzePreprocessorDirectives(const std::string& content) {
    std::vector<std::string> includes;
    std::istringstream iss(content);
    std::string line;

    std::regex includeRegex(R"(^\s*#\s*include\s+["<]([^">]+)[">])");

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, includeRegex)) {
            includes.push_back(match[1].str());
        }
    }

    return includes;
}

std::filesystem::path HeaderUnitCompiler::resolveIncludePath(
    const std::string& includePath,
    const std::vector<std::filesystem::path>& includePaths) const {

    // Primero buscar como archivo relativo
    std::filesystem::path relativePath = std::filesystem::current_path() / includePath;
    if (std::filesystem::exists(relativePath)) {
        return relativePath;
    }

    // Buscar en directorios de include
    for (const auto& includeDir : includePaths) {
        std::filesystem::path fullPath = includeDir / includePath;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    return {}; // No encontrado
}

bool HeaderUnitCompiler::isSystemHeader(const std::filesystem::path& headerPath) const {
    // Headers del sistema suelen estar en directorios como /usr/include, C:\Program Files, etc.
    std::string pathStr = headerPath.string();

#ifdef _WIN32
    return pathStr.find("Program Files") != std::string::npos ||
           pathStr.find("Windows Kits") != std::string::npos;
#else
    return pathStr.find("/usr/include") != std::string::npos ||
           pathStr.find("/usr/local/include") != std::string::npos;
#endif
}

std::string HeaderUnitCompiler::calculateContentHash(const std::filesystem::path& headerPath) const {
    if (!std::filesystem::exists(headerPath)) {
        return "";
    }

    std::ifstream file(headerPath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    // Calcular hash simple (en un compilador real se usaría SHA256)
    std::hash<std::string> hasher;
    return std::to_string(hasher(content));
}

// ============================================================================
// HeaderUnitCache - Implementación
// ============================================================================

HeaderUnitCache::HeaderUnitCache(const std::filesystem::path& cacheDirectory)
    : cacheDirectory_(cacheDirectory), totalHits_(0), totalMisses_(0), totalInvalidations_(0) {

    if (cacheDirectory_.empty()) {
        // Usar directorio temporal por defecto
        cacheDirectory_ = std::filesystem::temp_directory_path() / "cpp20_header_cache";
    }

    // Crear directorio si no existe
    std::filesystem::create_directories(cacheDirectory_);

    // Intentar cargar caché desde disco
    deserializeFromDisk();
}

HeaderUnitCache::~HeaderUnitCache() {
    // Guardar caché al salir
    serializeToDisk();
}

std::shared_ptr<HeaderUnit> HeaderUnitCache::lookup(const std::filesystem::path& headerPath) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string cacheKey = generateCacheKey(headerPath);
    auto it = cache_.find(cacheKey);

    if (it != cache_.end()) {
        auto& headerUnit = it->second;

        // Verificar si es válido
        if (isCacheEntryValid(headerUnit)) {
            updateStatistics(true);
            return headerUnit;
        } else {
            // Entrada inválida, eliminar
            cache_.erase(it);
            updateStatistics(false);
        }
    }

    updateStatistics(false);
    return nullptr;
}

void HeaderUnitCache::store(std::shared_ptr<HeaderUnit> headerUnit) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!headerUnit) return;

    std::string cacheKey = generateCacheKey(headerUnit->headerPath);
    cache_[cacheKey] = headerUnit;

    // Intentar serializar inmediatamente
    serializeToDisk();
}

bool HeaderUnitCache::isCached(const std::filesystem::path& headerPath) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string cacheKey = generateCacheKey(headerPath);
    auto it = cache_.find(cacheKey);

    return it != cache_.end() && isCacheEntryValid(it->second);
}

void HeaderUnitCache::invalidate(const std::filesystem::path& headerPath) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string cacheKey = generateCacheKey(headerPath);
    auto it = cache_.find(cacheKey);

    if (it != cache_.end()) {
        cache_.erase(it);
        totalInvalidations_++;
    }
}

void HeaderUnitCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    cache_.clear();
    totalHits_ = 0;
    totalMisses_ = 0;
    totalInvalidations_ = 0;
}

std::unordered_map<std::string, size_t> HeaderUnitCache::getCacheStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return {
        {"total_entries", cache_.size()},
        {"total_hits", totalHits_},
        {"total_misses", totalMisses_},
        {"total_invalidations", totalInvalidations_},
        {"hit_rate", totalHits_ + totalMisses_ > 0 ?
                    (totalHits_ * 100) / (totalHits_ + totalMisses_) : 0},
        {"cache_size_bytes", calculateCacheSize()}
    };
}

void HeaderUnitCache::setCacheDirectory(const std::filesystem::path& cacheDir) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Guardar caché actual
    serializeToDisk();

    cacheDirectory_ = cacheDir;
    std::filesystem::create_directories(cacheDirectory_);

    // Limpiar caché en memoria
    cache_.clear();

    // Cargar caché desde nuevo directorio
    deserializeFromDisk();
}

bool HeaderUnitCache::serializeToDisk() const {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::filesystem::path cacheFile = cacheDirectory_ / "header_cache.dat";
        std::ofstream file(cacheFile, std::ios::binary);

        if (!file.is_open()) return false;

        // Serializar estadísticas
        file.write(reinterpret_cast<const char*>(&totalHits_), sizeof(totalHits_));
        file.write(reinterpret_cast<const char*>(&totalMisses_), sizeof(totalMisses_));
        file.write(reinterpret_cast<const char*>(&totalInvalidations_), sizeof(totalInvalidations_));

        // Serializar número de entradas
        size_t entryCount = cache_.size();
        file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));

        // Serializar cada entrada
        for (const auto& [key, headerUnit] : cache_) {
            // Serializar clave
            size_t keyLen = key.size();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
            file.write(key.data(), keyLen);

            // Serializar información del header unit
            size_t pathLen = headerUnit->headerPath.string().size();
            file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
            file.write(headerUnit->headerPath.string().data(), pathLen);

            size_t nameLen = headerUnit->headerName.size();
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(headerUnit->headerName.data(), nameLen);

            size_t hashLen = headerUnit->contentHash.size();
            file.write(reinterpret_cast<const char*>(&hashLen), sizeof(hashLen));
            file.write(headerUnit->contentHash.data(), hashLen);

            auto timestamp = headerUnit->lastModified.time_since_epoch().count();
            file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));

            size_t depCount = headerUnit->dependencies.size();
            file.write(reinterpret_cast<const char*>(&depCount), sizeof(depCount));
            for (const auto& dep : headerUnit->dependencies) {
                size_t depLen = dep.size();
                file.write(reinterpret_cast<const char*>(&depLen), sizeof(depLen));
                file.write(dep.data(), depLen);
            }

            file.write(reinterpret_cast<const char*>(&headerUnit->isCompiled), sizeof(headerUnit->isCompiled));
            file.write(reinterpret_cast<const char*>(&headerUnit->needsRebuild), sizeof(headerUnit->needsRebuild));
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool HeaderUnitCache::deserializeFromDisk() {
    try {
        std::filesystem::path cacheFile = cacheDirectory_ / "header_cache.dat";
        if (!std::filesystem::exists(cacheFile)) {
            return true; // No hay caché previo
        }

        std::ifstream file(cacheFile, std::ios::binary);
        if (!file.is_open()) return false;

        // Limpiar caché actual
        cache_.clear();

        // Deserializar estadísticas
        file.read(reinterpret_cast<char*>(&totalHits_), sizeof(totalHits_));
        file.read(reinterpret_cast<char*>(&totalMisses_), sizeof(totalMisses_));
        file.read(reinterpret_cast<char*>(&totalInvalidations_), sizeof(totalInvalidations_));

        // Deserializar número de entradas
        size_t entryCount;
        file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

        // Deserializar cada entrada
        for (size_t i = 0; i < entryCount; ++i) {
            // Deserializar clave
            size_t keyLen;
            file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
            std::string key(keyLen, '\0');
            file.read(key.data(), keyLen);

            // Crear header unit
            auto headerUnit = std::make_shared<HeaderUnit>();

            // Deserializar información
            size_t pathLen;
            file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
            std::string pathStr(pathLen, '\0');
            file.read(pathStr.data(), pathLen);
            headerUnit->headerPath = pathStr;

            size_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            headerUnit->headerName.resize(nameLen);
            file.read(headerUnit->headerName.data(), nameLen);

            size_t hashLen;
            file.read(reinterpret_cast<char*>(&hashLen), sizeof(hashLen));
            headerUnit->contentHash.resize(hashLen);
            file.read(headerUnit->contentHash.data(), hashLen);

            long long timestamp;
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
            headerUnit->lastModified = std::chrono::system_clock::time_point(
                std::chrono::system_clock::duration(timestamp));

            size_t depCount;
            file.read(reinterpret_cast<char*>(&depCount), sizeof(depCount));
            headerUnit->dependencies.reserve(depCount);
            for (size_t j = 0; j < depCount; ++j) {
                size_t depLen;
                file.read(reinterpret_cast<char*>(&depLen), sizeof(depLen));
                std::string dep(depLen, '\0');
                file.read(dep.data(), depLen);
                headerUnit->dependencies.push_back(dep);
            }

            file.read(reinterpret_cast<char*>(&headerUnit->isCompiled), sizeof(headerUnit->isCompiled));
            file.read(reinterpret_cast<char*>(&headerUnit->needsRebuild), sizeof(headerUnit->needsRebuild));

            cache_[key] = headerUnit;
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool HeaderUnitCache::verifyCacheIntegrity() const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [key, headerUnit] : cache_) {
        if (!isCacheEntryValid(headerUnit)) {
            return false;
        }
    }

    return true;
}

std::vector<std::filesystem::path> HeaderUnitCache::listCachedHeaders() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::filesystem::path> headers;
    headers.reserve(cache_.size());

    for (const auto& [key, headerUnit] : cache_) {
        headers.push_back(headerUnit->headerPath);
    }

    return headers;
}

std::string HeaderUnitCache::generateCacheKey(const std::filesystem::path& headerPath) const {
    return headerPath.string();
}

std::filesystem::path HeaderUnitCache::generateCacheFileName(const std::string& cacheKey) const {
    // Generar nombre de archivo basado en hash de la clave
    std::hash<std::string> hasher;
    std::string hashStr = std::to_string(hasher(cacheKey));
    return cacheDirectory_ / (hashStr + ".bmi");
}

bool HeaderUnitCache::isCacheEntryValid(const std::shared_ptr<HeaderUnit>& headerUnit) const {
    // Verificar que el archivo header aún existe
    if (!std::filesystem::exists(headerUnit->headerPath)) {
        return false;
    }

    // Verificar que no ha cambiado
    auto currentTime = std::filesystem::last_write_time(headerUnit->headerPath);
    auto cachedTime = headerUnit->lastModified;

    if (currentTime != cachedTime) {
        return false;
    }

    // Verificar hash del contenido
    std::string currentHash = HeaderUnitCompiler().calculateContentHash(headerUnit->headerPath);
    if (currentHash != headerUnit->contentHash) {
        return false;
    }

    return headerUnit->isCompiled;
}

void HeaderUnitCache::updateStatistics(bool isHit) {
    if (isHit) {
        totalHits_++;
    } else {
        totalMisses_++;
    }
}

void HeaderUnitCache::cleanupInvalidEntries() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = cache_.begin(); it != cache_.end(); ) {
        if (!isCacheEntryValid(it->second)) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t HeaderUnitCache::calculateCacheSize() const {
    size_t totalSize = 0;

    for (const auto& [key, headerUnit] : cache_) {
        totalSize += sizeof(HeaderUnit);
        totalSize += headerUnit->headerPath.string().size();
        totalSize += headerUnit->headerName.size();
        totalSize += headerUnit->contentHash.size();

        for (const auto& dep : headerUnit->dependencies) {
            totalSize += dep.size();
        }

        // Estimar tamaño del BMI
        if (headerUnit->bmi) {
            totalSize += 1024; // Estimación aproximada
        }
    }

    return totalSize;
}

// ============================================================================
// HeaderDependencyManager - Implementación
// ============================================================================

HeaderDependencyManager::HeaderDependencyManager() = default;

HeaderDependencyManager::~HeaderDependencyManager() = default;

void HeaderDependencyManager::addDependency(const HeaderDependency& dependency) {
    std::lock_guard<std::mutex> lock(mutex_);
    dependencies_[dependency.fromHeader].push_back(dependency);
}

std::vector<HeaderDependency> HeaderDependencyManager::getDependencies(const std::string& headerName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = dependencies_.find(headerName);
    if (it != dependencies_.end()) {
        return it->second;
    }

    return {};
}

std::vector<std::string> HeaderDependencyManager::getDependents(const std::string& headerName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> dependents;

    for (const auto& [from, deps] : dependencies_) {
        for (const auto& dep : deps) {
            if (dep.toHeader == headerName) {
                dependents.push_back(from);
                break;
            }
        }
    }

    return dependents;
}

std::vector<std::string> HeaderDependencyManager::calculateCompilationOrder(
    const std::vector<std::string>& headers) const {

    std::lock_guard<std::mutex> lock(mutex_);

    // Construir grafo de dependencias
    auto graph = buildDependencyGraph();

    // Filtrar solo los headers especificados
    std::unordered_set<std::string> targetHeaders(headers.begin(), headers.end());

    // Aplicar ordenamiento topológico
    return topologicalSort(graph);
}

std::vector<std::vector<std::string>> HeaderDependencyManager::detectCycles() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;

    auto graph = buildDependencyGraph();

    for (const auto& [node, _] : graph) {
        std::vector<std::string> currentPath;
        if (visited.find(node) == visited.end()) {
            detectCyclesDFS(node, visited, recursionStack, currentPath, cycles);
        }
    }

    return cycles;
}

bool HeaderDependencyManager::hasCircularDependencies() const {
    return !detectCycles().empty();
}

std::unordered_map<std::string, std::vector<std::string>> HeaderDependencyManager::getDependencyGraph() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buildDependencyGraph();
}

void HeaderDependencyManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    dependencies_.clear();
}

bool HeaderDependencyManager::serializeToFile(const std::filesystem::path& filePath) const {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;

        for (const auto& [from, deps] : dependencies_) {
            for (const auto& dep : deps) {
                file << from << " -> " << dep.toHeader << " (" << static_cast<int>(dep.type) << ")\n";
            }
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

bool HeaderDependencyManager::deserializeFromFile(const std::filesystem::path& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        clear();

        std::string line;
        std::regex depRegex(R"((\S+)\s+->\s+(\S+)\s+\((\d+)\))");

        while (std::getline(file, line)) {
            std::smatch match;
            if (std::regex_search(line, match, depRegex)) {
                std::string from = match[1].str();
                std::string to = match[2].str();
                int typeInt = std::stoi(match[3].str());

                HeaderDependency dep(from, to, static_cast<DependencyType>(typeInt));
                addDependency(dep);
            }
        }

        file.close();
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

std::vector<std::string> HeaderDependencyManager::topologicalSort(
    const std::unordered_map<std::string, std::vector<std::string>>& graph) const {

    std::vector<std::string> result;
    std::unordered_map<std::string, int> inDegree;
    std::queue<std::string> zeroInDegree;

    // Calcular grados de entrada
    for (const auto& [node, neighbors] : graph) {
        if (inDegree.find(node) == inDegree.end()) {
            inDegree[node] = 0;
        }

        for (const auto& neighbor : neighbors) {
            inDegree[neighbor]++;
        }
    }

    // Encontrar nodos con grado de entrada 0
    for (const auto& [node, degree] : inDegree) {
        if (degree == 0) {
            zeroInDegree.push(node);
        }
    }

    // Algoritmo de Kahn
    while (!zeroInDegree.empty()) {
        std::string current = zeroInDegree.front();
        zeroInDegree.pop();
        result.push_back(current);

        auto it = graph.find(current);
        if (it != graph.end()) {
            for (const auto& neighbor : it->second) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    zeroInDegree.push(neighbor);
                }
            }
        }
    }

    return result;
}

void HeaderDependencyManager::detectCyclesDFS(const std::string& node,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recursionStack,
    std::vector<std::string>& currentPath,
    std::vector<std::vector<std::string>>& cycles) const {

    visited.insert(node);
    recursionStack.insert(node);
    currentPath.push_back(node);

    auto graph = buildDependencyGraph();
    auto it = graph.find(node);

    if (it != graph.end()) {
        for (const auto& neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                detectCyclesDFS(neighbor, visited, recursionStack, currentPath, cycles);
            } else if (recursionStack.find(neighbor) != recursionStack.end()) {
                // Encontrado ciclo
                auto cycleStart = std::find(currentPath.begin(), currentPath.end(), neighbor);
                if (cycleStart != currentPath.end()) {
                    std::vector<std::string> cycle(cycleStart, currentPath.end());
                    cycle.push_back(neighbor);
                    cycles.push_back(cycle);
                }
            }
        }
    }

    recursionStack.erase(node);
    currentPath.pop_back();
}

std::unordered_map<std::string, std::vector<std::string>> HeaderDependencyManager::buildDependencyGraph() const {
    std::unordered_map<std::string, std::vector<std::string>> graph;

    for (const auto& [from, deps] : dependencies_) {
        for (const auto& dep : deps) {
            graph[from].push_back(dep.toHeader);
        }
    }

    return graph;
}

// ============================================================================
// HeaderUnitCoordinator - Implementación
// ============================================================================

HeaderUnitCoordinator::HeaderUnitCoordinator(std::shared_ptr<HeaderUnitCache> cache,
                                           std::shared_ptr<HeaderDependencyManager> depManager)
    : cache_(cache), dependencyManager_(depManager), maxParallelJobs_(std::thread::hardware_concurrency()),
      totalCompiled_(0), totalFromCache_(0), totalFailed_(0) {

    if (!cache_) {
        cache_ = std::make_shared<HeaderUnitCache>();
    }

    if (!dependencyManager_) {
        dependencyManager_ = std::make_shared<HeaderDependencyManager>();
    }

    compiler_ = std::make_unique<HeaderUnitCompiler>();
}

HeaderUnitCoordinator::~HeaderUnitCoordinator() = default;

std::vector<std::shared_ptr<HeaderUnit>> HeaderUnitCoordinator::compileHeaderUnits(
    const std::vector<std::filesystem::path>& headerPaths,
    const std::vector<std::filesystem::path>& includePaths) {

    // Verificar que todos los headers sean válidos
    if (!canCompileAll(headerPaths)) {
        return {};
    }

    // Calcular orden de compilación
    std::vector<std::string> headerNames;
    for (const auto& path : headerPaths) {
        headerNames.push_back(HeaderUnitUtils::getHeaderName(path));
    }

    auto compilationOrder = dependencyManager_->calculateCompilationOrder(headerNames);

    // Reordenar headers según dependencias
    std::vector<std::filesystem::path> orderedHeaders;
    std::unordered_map<std::string, std::filesystem::path> nameToPath;

    for (const auto& path : headerPaths) {
        nameToPath[HeaderUnitUtils::getHeaderName(path)] = path;
    }

    for (const auto& name : compilationOrder) {
        auto it = nameToPath.find(name);
        if (it != nameToPath.end()) {
            orderedHeaders.push_back(it->second);
        }
    }

    // Compilar en paralelo
    return compileInParallel(orderedHeaders, includePaths);
}

std::vector<std::shared_ptr<HeaderUnit>> HeaderUnitCoordinator::compileWithDependencies(
    const std::vector<std::filesystem::path>& headerPaths,
    const std::vector<std::filesystem::path>& includePaths) {

    // Primero analizar dependencias
    for (const auto& headerPath : headerPaths) {
        std::string headerName = HeaderUnitUtils::getHeaderName(headerPath);
        auto dependencies = compiler_->getHeaderDependencies(headerPath, includePaths);

        for (const auto& dep : dependencies) {
            HeaderDependency headerDep(headerName, dep, DependencyType::Direct);
            dependencyManager_->addDependency(headerDep);
        }
    }

    // Verificar dependencias circulares
    if (dependencyManager_->hasCircularDependencies()) {
        // En un compilador real, aquí se reportaría un error
        std::cerr << "Warning: Circular dependencies detected in header units" << std::endl;
    }

    // Compilar considerando dependencias
    return compileHeaderUnits(headerPaths, includePaths);
}

bool HeaderUnitCoordinator::canCompileAll(const std::vector<std::filesystem::path>& headerPaths) const {
    for (const auto& path : headerPaths) {
        if (!HeaderUnitUtils::isValidHeaderFile(path)) {
            return false;
        }
    }
    return true;
}

std::vector<std::filesystem::path> HeaderUnitCoordinator::getOutdatedHeaders(
    const std::vector<std::filesystem::path>& headerPaths) const {

    std::vector<std::filesystem::path> outdated;

    for (const auto& path : headerPaths) {
        if (needsRebuild(path)) {
            outdated.push_back(path);
        }
    }

    return outdated;
}

void HeaderUnitCoordinator::forceRebuild(const std::vector<std::filesystem::path>& headerPaths) {
    for (const auto& path : headerPaths) {
        cache_->invalidate(path);
    }
}

std::unordered_map<std::string, size_t> HeaderUnitCoordinator::getCompilationStatistics() const {
    return {
        {"total_compiled", totalCompiled_},
        {"total_from_cache", totalFromCache_},
        {"total_failed", totalFailed_},
        {"cache_hit_rate", totalCompiled_ + totalFromCache_ > 0 ?
                          (totalFromCache_ * 100) / (totalCompiled_ + totalFromCache_) : 0}
    };
}

void HeaderUnitCoordinator::setMaxParallelJobs(size_t maxJobs) {
    maxParallelJobs_ = maxJobs > 0 ? maxJobs : 1;
}

std::shared_ptr<HeaderUnit> HeaderUnitCoordinator::compileSingleHeaderUnit(
    const std::filesystem::path& headerPath,
    const std::vector<std::filesystem::path>& includePaths) {

    // Verificar caché primero
    auto cachedUnit = cache_->lookup(headerPath);
    if (cachedUnit && !needsRebuild(headerPath)) {
        updateStatistics(true, true);
        return cachedUnit;
    }

    // Compilar header unit
    auto bmi = compiler_->compileHeaderUnit(headerPath, includePaths);
    if (!bmi) {
        updateStatistics(false, false);
        return nullptr;
    }

    // Crear header unit
    auto headerUnit = std::make_shared<HeaderUnit>(headerPath, HeaderUnitUtils::getHeaderName(headerPath));
    headerUnit->contentHash = compiler_->calculateContentHash(headerPath);
    headerUnit->lastModified = HeaderUnitUtils::getFileModificationTime(headerPath);
    headerUnit->bmi = std::move(bmi);
    headerUnit->isCompiled = true;
    headerUnit->needsRebuild = false;

    // Obtener dependencias
    headerUnit->dependencies = compiler_->getHeaderDependencies(headerPath, includePaths);

    // Actualizar dependencias
    updateDependencies(headerUnit);

    // Almacenar en caché
    cache_->store(headerUnit);

    updateStatistics(false, true);
    return headerUnit;
}

bool HeaderUnitCoordinator::needsRebuild(const std::filesystem::path& headerPath) const {
    // Verificar si existe en caché
    if (!cache_->isCached(headerPath)) {
        return true;
    }

    // Verificar si el archivo ha cambiado
    auto cachedUnit = cache_->lookup(headerPath);
    if (!cachedUnit) {
        return true;
    }

    auto currentTime = HeaderUnitUtils::getFileModificationTime(headerPath);
    if (currentTime != cachedUnit->lastModified) {
        return true;
    }

    // Verificar hash del contenido
    std::string currentHash = compiler_->calculateContentHash(headerPath);
    if (currentHash != cachedUnit->contentHash) {
        return true;
    }

    return false;
}

void HeaderUnitCoordinator::updateDependencies(const std::shared_ptr<HeaderUnit>& headerUnit) {
    for (const auto& dep : headerUnit->dependencies) {
        HeaderDependency headerDep(headerUnit->headerName, dep, DependencyType::Direct);
        dependencyManager_->addDependency(headerDep);
    }
}

std::vector<std::shared_ptr<HeaderUnit>> HeaderUnitCoordinator::compileInParallel(
    const std::vector<std::filesystem::path>& headerPaths,
    const std::vector<std::filesystem::path>& includePaths) {

    std::vector<std::shared_ptr<HeaderUnit>> results;
    results.reserve(headerPaths.size());

    // Para simplificar, compilamos secuencialmente
    // En un compilador real, aquí se usaría std::async para paralelización

    for (const auto& path : headerPaths) {
        auto result = compileSingleHeaderUnit(path, includePaths);
        if (result) {
            results.push_back(result);
        }
    }

    return results;
}

bool HeaderUnitCoordinator::verifyDependencies(const std::vector<std::shared_ptr<HeaderUnit>>& headerUnits) const {
    // Verificar que todas las dependencias estén satisfechas
    for (const auto& unit : headerUnits) {
        for (const auto& dep : unit->dependencies) {
            bool found = false;
            for (const auto& otherUnit : headerUnits) {
                if (otherUnit->headerName == dep) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Dependencia no encontrada - podría ser externa
                // En un compilador real, aquí se verificaría más estrictamente
            }
        }
    }

    return true;
}

void HeaderUnitCoordinator::updateStatistics(bool fromCache, bool success) {
    if (fromCache && success) {
        totalFromCache_++;
    } else if (!fromCache && success) {
        totalCompiled_++;
    } else {
        totalFailed_++;
    }
}

// ============================================================================
// HeaderUnitUtils - Implementación
// ============================================================================

bool HeaderUnitUtils::isHeaderFile(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        return false;
    }

    std::string ext = filePath.extension().string();
    return ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".h++" ||
           ext == ".hh" || ext == ".hcc";
}

std::string HeaderUnitUtils::getHeaderName(const std::filesystem::path& filePath) {
    return filePath.filename().string();
}

std::string HeaderUnitUtils::normalizeHeaderPath(const std::filesystem::path& filePath) {
    return std::filesystem::weakly_canonical(filePath).string();
}

std::vector<std::string> HeaderUnitUtils::extractIncludes(const std::string& content) {
    std::vector<std::string> includes;
    std::istringstream iss(content);
    std::string line;

    std::regex includeRegex(R"(^\s*#\s*include\s+["<]([^">]+)[">])");

    while (std::getline(iss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, includeRegex)) {
            includes.push_back(match[1].str());
        }
    }

    return includes;
}

bool HeaderUnitUtils::isSystemInclude(const std::string& includeLine) {
    // Los includes con < > son del sistema
    return includeLine.find('<') != std::string::npos;
}

std::filesystem::path HeaderUnitUtils::resolveIncludePath(
    const std::string& includePath,
    const std::vector<std::filesystem::path>& includeDirs) {

    // Buscar en directorios de include
    for (const auto& includeDir : includeDirs) {
        std::filesystem::path fullPath = includeDir / includePath;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    return {};
}

std::string HeaderUnitUtils::calculateFileHash(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return "";
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    std::hash<std::string> hasher;
    return std::to_string(hasher(content));
}

bool HeaderUnitUtils::compareFiles(const std::filesystem::path& file1,
                                 const std::filesystem::path& file2) {
    std::string hash1 = calculateFileHash(file1);
    std::string hash2 = calculateFileHash(file2);

    return !hash1.empty() && !hash2.empty() && hash1 == hash2;
}

std::chrono::system_clock::time_point HeaderUnitUtils::getFileModificationTime(
    const std::filesystem::path& filePath) {

    if (!std::filesystem::exists(filePath)) {
        return std::chrono::system_clock::time_point::min();
    }

    auto fileTime = std::filesystem::last_write_time(filePath);
    return std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
}

bool HeaderUnitUtils::isValidHeaderFile(const std::filesystem::path& filePath) {
    return std::filesystem::exists(filePath) &&
           std::filesystem::is_regular_file(filePath) &&
           isHeaderFile(filePath);
}

bool HeaderUnitUtils::ensureDirectoryExists(const std::filesystem::path& directory) {
    return std::filesystem::create_directories(directory);
}

std::vector<std::filesystem::path> HeaderUnitUtils::listHeaderFiles(
    const std::filesystem::path& directory) {

    std::vector<std::filesystem::path> headers;

    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        return headers;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && isHeaderFile(entry.path())) {
            headers.push_back(entry.path());
        }
    }

    return headers;
}

} // namespace cpp20::compiler::modules
