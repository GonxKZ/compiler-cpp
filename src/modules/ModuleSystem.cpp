/**
 * @file ModuleSystem.cpp
 * @brief Implementación completa del sistema de módulos C++20
 */

#include <compiler/modules/ModuleSystem.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <compiler/common/utils/FileUtils.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace cpp20::compiler::modules {

// ============================================================================
// BinaryModuleInterface Implementation
// ============================================================================

BinaryModuleInterface::BinaryModuleInterface(const std::string& moduleName)
    : moduleName_(moduleName) {
    timestamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::vector<uint8_t> BinaryModuleInterface::serialize() const {
    std::vector<uint8_t> data;

    // Header
    // Version (4 bytes)
    uint32_t version = version_;
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&version),
                reinterpret_cast<uint8_t*>(&version) + sizeof(uint32_t));

    // Timestamp (8 bytes)
    uint64_t timestamp = timestamp_;
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&timestamp),
                reinterpret_cast<uint8_t*>(&timestamp) + sizeof(uint64_t));

    // Module name length (4 bytes)
    uint32_t nameLength = static_cast<uint32_t>(moduleName_.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&nameLength),
                reinterpret_cast<uint8_t*>(&nameLength) + sizeof(uint32_t));

    // Module name
    data.insert(data.end(), moduleName_.begin(), moduleName_.end());

    // Compilation options hash (24 bytes)
    size_t hash1 = optionsHash_.preprocessorHash;
    size_t hash2 = optionsHash_.compilerFlagsHash;
    size_t hash3 = optionsHash_.systemIncludesHash;

    data.insert(data.end(), reinterpret_cast<uint8_t*>(&hash1),
                reinterpret_cast<uint8_t*>(&hash1) + sizeof(size_t));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&hash2),
                reinterpret_cast<uint8_t*>(&hash2) + sizeof(size_t));
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&hash3),
                reinterpret_cast<uint8_t*>(&hash3) + sizeof(size_t));

    // Number of exported entities (4 bytes)
    uint32_t numEntities = static_cast<uint32_t>(exportedEntities_.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&numEntities),
                reinterpret_cast<uint8_t*>(&numEntities) + sizeof(uint32_t));

    // Exported entities
    for (const auto& entity : exportedEntities_) {
        // Entity type (4 bytes)
        uint32_t type = static_cast<uint32_t>(entity.type);
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&type),
                    reinterpret_cast<uint8_t*>(&type) + sizeof(uint32_t));

        // Name length (4 bytes)
        uint32_t nameLen = static_cast<uint32_t>(entity.name.size());
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&nameLen),
                    reinterpret_cast<uint8_t*>(&nameLen) + sizeof(uint32_t));

        // Name
        data.insert(data.end(), entity.name.begin(), entity.name.end());

        // Qualified name length (4 bytes)
        uint32_t qualNameLen = static_cast<uint32_t>(entity.qualifiedName.size());
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&qualNameLen),
                    reinterpret_cast<uint8_t*>(&qualNameLen) + sizeof(uint32_t));

        // Qualified name
        data.insert(data.end(), entity.qualifiedName.begin(), entity.qualifiedName.end());

        // Flags (1 byte)
        uint8_t flags = (entity.isInline ? 1 : 0) | (entity.isConstexpr ? 2 : 0);
        data.push_back(flags);
    }

    // Number of dependencies (4 bytes)
    uint32_t numDeps = static_cast<uint32_t>(dependencies_.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&numDeps),
                reinterpret_cast<uint8_t*>(&numDeps) + sizeof(uint32_t));

    // Dependencies
    for (const auto& dep : dependencies_) {
        // Is interface (1 byte)
        uint8_t isInterface = dep.isInterface ? 1 : 0;
        data.push_back(isInterface);

        // Module name length (4 bytes)
        uint32_t depNameLen = static_cast<uint32_t>(dep.moduleName.size());
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&depNameLen),
                    reinterpret_cast<uint8_t*>(&depNameLen) + sizeof(uint32_t));

        // Module name
        data.insert(data.end(), dep.moduleName.begin(), dep.moduleName.end());
    }

    return data;
}

std::unique_ptr<BinaryModuleInterface> BinaryModuleInterface::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 16) { // Minimum header size
        return nullptr;
    }

    size_t offset = 0;

    // Version
    if (offset + sizeof(uint32_t) > data.size()) return nullptr;
    uint32_t version;
    std::memcpy(&version, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (version != 1) {
        return nullptr; // Unsupported version
    }

    // Timestamp
    if (offset + sizeof(uint64_t) > data.size()) return nullptr;
    uint64_t timestamp;
    std::memcpy(&timestamp, &data[offset], sizeof(uint64_t));
    offset += sizeof(uint64_t);

    // Module name
    if (offset + sizeof(uint32_t) > data.size()) return nullptr;
    uint32_t nameLength;
    std::memcpy(&nameLength, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (offset + nameLength > data.size()) return nullptr;
    std::string moduleName(reinterpret_cast<const char*>(&data[offset]), nameLength);
    offset += nameLength;

    auto bmi = std::make_unique<BinaryModuleInterface>(moduleName);
    bmi->timestamp_ = timestamp;

    // Compilation options hash
    if (offset + 3 * sizeof(size_t) > data.size()) return nullptr;
    std::memcpy(&bmi->optionsHash_.preprocessorHash, &data[offset], sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(&bmi->optionsHash_.compilerFlagsHash, &data[offset], sizeof(size_t));
    offset += sizeof(size_t);
    std::memcpy(&bmi->optionsHash_.systemIncludesHash, &data[offset], sizeof(size_t));
    offset += sizeof(size_t);

    // Exported entities
    if (offset + sizeof(uint32_t) > data.size()) return nullptr;
    uint32_t numEntities;
    std::memcpy(&numEntities, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < numEntities; ++i) {
        if (offset + sizeof(uint32_t) > data.size()) return nullptr;
        uint32_t type;
        std::memcpy(&type, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + sizeof(uint32_t) > data.size()) return nullptr;
        uint32_t nameLen;
        std::memcpy(&nameLen, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + nameLen > data.size()) return nullptr;
        std::string name(reinterpret_cast<const char*>(&data[offset]), nameLen);
        offset += nameLen;

        if (offset + sizeof(uint32_t) > data.size()) return nullptr;
        uint32_t qualNameLen;
        std::memcpy(&qualNameLen, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + qualNameLen > data.size()) return nullptr;
        std::string qualName(reinterpret_cast<const char*>(&data[offset]), qualNameLen);
        offset += qualNameLen;

        if (offset + sizeof(uint8_t) > data.size()) return nullptr;
        uint8_t flags = data[offset];
        offset += sizeof(uint8_t);

        ExportedEntity entity(name, qualName, static_cast<ExportType>(type));
        entity.isInline = (flags & 1) != 0;
        entity.isConstexpr = (flags & 2) != 0;

        bmi->exportedEntities_.push_back(entity);
    }

    // Dependencies
    if (offset + sizeof(uint32_t) > data.size()) return nullptr;
    uint32_t numDeps;
    std::memcpy(&numDeps, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < numDeps; ++i) {
        if (offset + sizeof(uint8_t) > data.size()) return nullptr;
        uint8_t isInterface = data[offset];
        offset += sizeof(uint8_t);

        if (offset + sizeof(uint32_t) > data.size()) return nullptr;
        uint32_t depNameLen;
        std::memcpy(&depNameLen, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);

        if (offset + depNameLen > data.size()) return nullptr;
        std::string depName(reinterpret_cast<const char*>(&data[offset]), depNameLen);
        offset += depNameLen;

        ModuleDependency dep(depName, isInterface != 0);
        bmi->dependencies_.push_back(dep);
    }

    return bmi;
}

bool BinaryModuleInterface::isValid() const {
    return !moduleName_.empty() && !exportedEntities_.empty();
}

void BinaryModuleInterface::addExportedEntity(const ExportedEntity& entity) {
    exportedEntities_.push_back(entity);
}

const std::vector<ExportedEntity>& BinaryModuleInterface::getExportedEntities() const {
    return exportedEntities_;
}

void BinaryModuleInterface::addDependency(const ModuleDependency& dep) {
    dependencies_.push_back(dep);
}

const std::vector<ModuleDependency>& BinaryModuleInterface::getDependencies() const {
    return dependencies_;
}

void BinaryModuleInterface::setCompilationOptionsHash(const CompilationOptionsHash& hash) {
    optionsHash_ = hash;
}

const CompilationOptionsHash& BinaryModuleInterface::getCompilationOptionsHash() const {
    return optionsHash_;
}

// ============================================================================
// ModuleInterface Implementation
// ============================================================================

ModuleInterface::ModuleInterface(const std::string& moduleName, const std::filesystem::path& sourcePath)
    : moduleName_(moduleName), sourcePath_(sourcePath) {
}

void ModuleInterface::addPartition(const std::string& partitionName) {
    if (std::find(partitions_.begin(), partitions_.end(), partitionName) == partitions_.end()) {
        partitions_.push_back(partitionName);
    }
}

const std::vector<std::string>& ModuleInterface::getPartitions() const {
    return partitions_;
}

void ModuleInterface::setBMI(std::unique_ptr<BinaryModuleInterface> bmi) {
    bmi_ = std::move(bmi);
}

const BinaryModuleInterface* ModuleInterface::getBMI() const {
    return bmi_.get();
}

bool ModuleInterface::isReady() const {
    return bmi_ != nullptr && bmi_->isValid();
}

// ============================================================================
// ModuleImplementation Implementation
// ============================================================================

ModuleImplementation::ModuleImplementation(const std::string& moduleName, const std::filesystem::path& sourcePath)
    : moduleName_(moduleName), sourcePath_(sourcePath) {
}

void ModuleImplementation::addDependency(const std::string& moduleName) {
    if (std::find(dependencies_.begin(), dependencies_.end(), moduleName) == dependencies_.end()) {
        dependencies_.push_back(moduleName);
    }
}

const std::vector<std::string>& ModuleImplementation::getDependencies() const {
    return dependencies_;
}

// ============================================================================
// ModuleDependencyScanner Implementation
// ============================================================================

ModuleDependencyScanner::ModuleDependencyScanner() = default;

std::vector<ModuleDependency> ModuleDependencyScanner::scanFile(const std::filesystem::path& filePath) {
    std::string filePathStr = filePath.string();

    // Check cache first
    auto it = scanCache_.find(filePathStr);
    if (it != scanCache_.end()) {
        return it->second;
    }

    std::vector<ModuleDependency> dependencies;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return dependencies;
    }

    std::string line;
    size_t lineNumber = 1;

    while (std::getline(file, line)) {
        // Remove comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Check for module declarations
        if (line.find("export module") != std::string::npos) {
            size_t pos = line.find("export module");
            std::string moduleDecl = line.substr(pos + 13); // Skip "export module"
            // Module declaration found, but we don't add it as a dependency
            continue;
        }

        // Check for module declarations (non-export)
        if (line.find("module") != std::string::npos && line.find("export module") == std::string::npos) {
            size_t pos = line.find("module");
            std::string moduleDecl = line.substr(pos + 6); // Skip "module"
            // Module declaration found, but we don't add it as a dependency
            continue;
        }

        // Check for import declarations
        if (line.find("import") != std::string::npos) {
            size_t pos = line.find("import");
            std::string importDecl = line.substr(pos + 6); // Skip "import"

            // Handle different import forms
            if (importDecl.find("<") != std::string::npos && importDecl.find(">") != std::string::npos) {
                // Header unit: import <header>
                size_t start = importDecl.find("<");
                size_t end = importDecl.find(">", start);
                if (start != std::string::npos && end != std::string::npos) {
                    std::string headerName = importDecl.substr(start + 1, end - start - 1);
                    dependencies.emplace_back(headerName, false, filePathStr + ":" + std::to_string(lineNumber));
                }
            } else {
                // Module import: import module_name
                size_t semicolonPos = importDecl.find(";");
                if (semicolonPos != std::string::npos) {
                    importDecl = importDecl.substr(0, semicolonPos);
                }

                // Remove whitespace
                importDecl.erase(std::remove_if(importDecl.begin(), importDecl.end(), ::isspace),
                               importDecl.end());

                if (!importDecl.empty()) {
                    dependencies.emplace_back(importDecl, true, filePathStr + ":" + std::to_string(lineNumber));
                }
            }
        }

        lineNumber++;
    }

    // Cache the result
    scanCache_[filePathStr] = dependencies;

    return dependencies;
}

bool ModuleDependencyScanner::containsModuleDeclaration(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Check for module declarations
        if (line.find("export module") != std::string::npos || line.find("module") != std::string::npos) {
            return true;
        }
    }

    return false;
}

std::string ModuleDependencyScanner::extractModuleName(const std::string& line) {
    size_t modulePos = line.find("module");
    if (modulePos == std::string::npos) {
        return "";
    }

    std::string moduleDecl = line.substr(modulePos + 6); // Skip "module"
    size_t semicolonPos = moduleDecl.find(";");
    if (semicolonPos != std::string::npos) {
        moduleDecl = moduleDecl.substr(0, semicolonPos);
    }

    // Remove whitespace
    moduleDecl.erase(std::remove_if(moduleDecl.begin(), moduleDecl.end(), ::isspace),
                   moduleDecl.end());

    return moduleDecl;
}

std::string ModuleDependencyScanner::extractImportName(const std::string& line) {
    size_t importPos = line.find("import");
    if (importPos == std::string::npos) {
        return "";
    }

    std::string importDecl = line.substr(importPos + 6); // Skip "import"
    size_t semicolonPos = importDecl.find(";");
    if (semicolonPos != std::string::npos) {
        importDecl = importDecl.substr(0, semicolonPos);
    }

    // Remove whitespace
    importDecl.erase(std::remove_if(importDecl.begin(), importDecl.end(), ::isspace),
                   importDecl.end());

    return importDecl;
}

bool ModuleDependencyScanner::isHeaderUnit(const std::string& importName) {
    return importName.find("<") != std::string::npos && importName.find(">") != std::string::npos;
}

// ============================================================================
// ModuleCache Implementation
// ============================================================================

ModuleCache::ModuleCache(const std::filesystem::path& cacheDir)
    : cacheDir_(cacheDir) {
    // Create cache directory if it doesn't exist
    std::filesystem::create_directories(cacheDir_);
}

bool ModuleCache::store(const std::string& moduleName, const BinaryModuleInterface& bmi) {
    try {
        std::string key = generateCacheKey(moduleName);
        std::filesystem::path cacheFile = getCacheFilePath(key);

        std::vector<uint8_t> data = bmi.serialize();

        std::ofstream file(cacheFile, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();

        stats_.totalEntries++;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::unique_ptr<BinaryModuleInterface> ModuleCache::retrieve(const std::string& moduleName) {
    try {
        std::string key = generateCacheKey(moduleName);
        std::filesystem::path cacheFile = getCacheFilePath(key);

        if (!std::filesystem::exists(cacheFile)) {
            stats_.misses++;
            return nullptr;
        }

        std::ifstream file(cacheFile, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            stats_.misses++;
            return nullptr;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            stats_.misses++;
            return nullptr;
        }

        file.close();

        auto bmi = BinaryModuleInterface::deserialize(data);
        if (bmi && bmi->isValid()) {
            stats_.hits++;
            return bmi;
        } else {
            stats_.misses++;
            return nullptr;
        }
    } catch (const std::exception&) {
        stats_.misses++;
        return nullptr;
    }
}

bool ModuleCache::isValid(const std::string& moduleName, const CompilationOptionsHash& currentHash) {
    auto bmi = retrieve(moduleName);
    if (!bmi) {
        return false;
    }

    const auto& cachedHash = bmi->getCompilationOptionsHash();
    return cachedHash.combined() == currentHash.combined();
}

void ModuleCache::invalidate(const std::string& moduleName) {
    try {
        std::string key = generateCacheKey(moduleName);
        std::filesystem::path cacheFile = getCacheFilePath(key);

        if (std::filesystem::exists(cacheFile)) {
            std::filesystem::remove(cacheFile);
            stats_.invalidations++;
            stats_.totalEntries--;
        }
    } catch (const std::exception&) {
        // Ignore errors during invalidation
    }
}

void ModuleCache::clear() {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(cacheDir_)) {
            if (entry.is_regular_file()) {
                std::filesystem::remove(entry.path());
            }
        }
        stats_ = CacheStats{};
    } catch (const std::exception&) {
        // Ignore errors during clear
    }
}

ModuleCache::CacheStats ModuleCache::getStats() const {
    return stats_;
}

std::string ModuleCache::generateCacheKey(const std::string& moduleName) const {
    // Simple hash for cache key
    size_t hash = std::hash<std::string>{}(moduleName);
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::filesystem::path ModuleCache::getCacheFilePath(const std::string& key) const {
    return cacheDir_ / (key + ".bmi");
}

// ============================================================================
// ModuleLoader Implementation
// ============================================================================

ModuleLoader::ModuleLoader(std::shared_ptr<ModuleCache> cache)
    : cache_(cache) {
}

std::unique_ptr<ModuleInterface> ModuleLoader::loadModule(const std::string& moduleName,
                                                        const std::filesystem::path& sourcePath) {
    // Check if already loaded
    auto it = loadedModules_.find(moduleName);
    if (it != loadedModules_.end()) {
        return nullptr; // Already loaded
    }

    // Try to load from cache first
    auto bmi = cache_->retrieve(moduleName);
    if (bmi) {
        auto module = std::make_unique<ModuleInterface>(moduleName, sourcePath);
        module->setBMI(std::move(bmi));
        loadedModules_[moduleName] = std::move(module);
        return std::make_unique<ModuleInterface>(moduleName, sourcePath);
    }

    // Create new module interface
    auto module = std::make_unique<ModuleInterface>(moduleName, sourcePath);
    loadedModules_[moduleName] = std::move(module);

    return std::make_unique<ModuleInterface>(moduleName, sourcePath);
}

bool ModuleLoader::isModuleLoaded(const std::string& moduleName) const {
    return loadedModules_.find(moduleName) != loadedModules_.end();
}

const ModuleInterface* ModuleLoader::getModule(const std::string& moduleName) const {
    auto it = loadedModules_.find(moduleName);
    return it != loadedModules_.end() ? it->second.get() : nullptr;
}

void ModuleLoader::unloadModule(const std::string& moduleName) {
    loadedModules_.erase(moduleName);
}

// ============================================================================
// ModuleSystem Implementation
// ============================================================================

ModuleSystem::ModuleSystem(const std::filesystem::path& cacheDir)
    : scanner_(std::make_shared<ModuleDependencyScanner>()),
      cache_(std::make_shared<ModuleCache>(cacheDir)),
      loader_(std::make_shared<ModuleLoader>(cache_)) {
}

bool ModuleSystem::initialize() {
    // Clear any existing state
    interfaces_.clear();
    implementations_.clear();
    stats_ = SystemStats{};

    return true;
}

bool ModuleSystem::processSourceFile(const std::filesystem::path& sourcePath) {
    try {
        // Scan for dependencies
        auto dependencies = scanner_->scanFile(sourcePath);

        // Check if file contains module declaration
        if (scanner_->containsModuleDeclaration(sourcePath)) {
            // Read the file to extract module name
            std::ifstream file(sourcePath);
            if (!file.is_open()) {
                return false;
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line.find("export module") != std::string::npos) {
                    std::string moduleName = scanner_->extractModuleName(line);
                    if (!moduleName.empty()) {
                        return processModuleDeclaration(sourcePath, moduleName);
                    }
                } else if (line.find("module") != std::string::npos &&
                          line.find("export module") == std::string::npos) {
                    std::string moduleName = scanner_->extractModuleName(line);
                    if (!moduleName.empty()) {
                        return processModuleDeclaration(sourcePath, moduleName);
                    }
                }
            }
        }

        // Process imports
        for (const auto& dep : dependencies) {
            if (dep.isInterface) {
                processImportDeclaration(sourcePath, dep.moduleName);
            }
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool ModuleSystem::compileModule(const std::string& moduleName) {
    // Check if module exists
    auto it = interfaces_.find(moduleName);
    if (it == interfaces_.end()) {
        return false;
    }

    auto& module = it->second;

    // Check if already compiled
    if (module->isReady()) {
        return true;
    }

    // Get dependencies
    auto dependencies = getModuleDependencies(moduleName);

    // Compile dependencies first (topological order)
    for (const auto& dep : dependencies) {
        if (!compileModule(dep)) {
            return false;
        }
    }

    // Compile this module
    // In a real implementation, this would invoke the compiler
    // For now, we'll create a mock BMI
    auto bmi = std::make_unique<BinaryModuleInterface>(moduleName);

    // Add some mock exported entities
    bmi->addExportedEntity(ExportedEntity("MyClass", "MyModule::MyClass", ExportType::Type));
    bmi->addExportedEntity(ExportedEntity("myFunction", "MyModule::myFunction", ExportType::Function));

    // Add dependencies
    for (const auto& dep : dependencies) {
        bmi->addDependency(ModuleDependency(dep, true));
    }

    // Set compilation options hash (mock)
    CompilationOptionsHash hash;
    hash.preprocessorHash = std::hash<std::string>{}("#define DEBUG");
    hash.compilerFlagsHash = std::hash<std::string>{}("-O2 -std=c++20");
    hash.systemIncludesHash = std::hash<std::string>{}("/usr/include");
    bmi->setCompilationOptionsHash(hash);

    // Store in cache
    if (cache_->store(moduleName, *bmi)) {
        module->setBMI(std::move(bmi));
        stats_.interfacesCompiled++;
        return true;
    }

    return false;
}

std::vector<std::string> ModuleSystem::getModuleDependencies(const std::string& moduleName) {
    std::vector<std::string> dependencies;

    auto it = interfaces_.find(moduleName);
    if (it != interfaces_.end()) {
        const auto& module = it->second;
        if (module->getBMI()) {
            for (const auto& dep : module->getBMI()->getDependencies()) {
                if (dep.isInterface) {
                    dependencies.push_back(dep.moduleName);
                }
            }
        }
    }

    return dependencies;
}

bool ModuleSystem::moduleExists(const std::string& moduleName) const {
    return interfaces_.find(moduleName) != interfaces_.end();
}

ModuleSystem::SystemStats ModuleSystem::getStats() const {
    auto cacheStats = cache_->getStats();
    return {
        interfaces_.size() + implementations_.size(),
        stats_.interfacesCompiled,
        stats_.implementationsCompiled,
        cacheStats.hits,
        cacheStats.misses
    };
}

void ModuleSystem::clearCache() {
    cache_->clear();
}

bool ModuleSystem::processModuleDeclaration(const std::filesystem::path& filePath,
                                          const std::string& moduleName) {
    // Create module interface
    auto module = std::make_unique<ModuleInterface>(moduleName, filePath);
    interfaces_[moduleName] = std::move(module);

    stats_.totalModules++;
    return true;
}

bool ModuleSystem::processImportDeclaration(const std::filesystem::path& filePath,
                                          const std::string& importName) {
    // For now, just track the import
    // In a full implementation, this would load the imported module
    return true;
}

std::vector<std::string> ModuleSystem::computeCompilationOrder(const std::string& targetModule) {
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> visiting;

    std::function<bool(const std::string&)> visit = [&](const std::string& module) -> bool {
        if (visiting.count(module)) {
            // Circular dependency detected
            return false;
        }
        if (visited.count(module)) {
            return true;
        }

        visiting.insert(module);

        // Visit dependencies
        auto dependencies = getModuleDependencies(module);
        for (const auto& dep : dependencies) {
            if (!visit(dep)) {
                return false;
            }
        }

        visiting.erase(module);
        visited.insert(module);
        order.push_back(module);

        return true;
    };

    if (!visit(targetModule)) {
        return {}; // Circular dependency or error
    }

    return order;
}

} // namespace cpp20::compiler::modules
