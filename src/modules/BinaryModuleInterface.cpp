/**
 * @file BinaryModuleInterface.cpp
 * @brief Implementación del formato BMI para módulos C++20
 */

#include <compiler/modules/BinaryModuleInterface.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace cpp20::compiler::modules {

// ============================================================================
// ExportedEntity - Implementación
// ============================================================================

// Constructor ya definido en header

// ============================================================================
// BinaryModuleInterface - Implementación
// ============================================================================

BinaryModuleInterface::BinaryModuleInterface(const std::string& moduleName) {
    metadata_.moduleName = moduleName;
    metadata_.formatVersion = BMIFormatVersion::Version1_0;
    metadata_.buildTimestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    metadata_.compilerVersion = "cpp20-compiler 1.0";
    metadata_.targetTriple = "x86_64-pc-windows-msvc";
}

BinaryModuleInterface::~BinaryModuleInterface() = default;

void BinaryModuleInterface::addExportedEntity(std::unique_ptr<ExportedEntity> entity) {
    exportedEntities_.push_back(std::move(entity));
    updateMetadata();
    buildIndices();
}

void BinaryModuleInterface::addModuleImport(const ModuleImport& import) {
    moduleImports_.push_back(import);
    updateMetadata();
}

void BinaryModuleInterface::addModuleRequirement(const ModuleRequirement& requirement) {
    moduleRequirements_.push_back(requirement);
    updateMetadata();
}

void BinaryModuleInterface::setModuleAST(std::unique_ptr<ast::ASTNode> ast) {
    moduleAST_ = std::move(ast);
    updateMetadata();
}

bool BinaryModuleInterface::serializeToFile(const std::filesystem::path& filePath) {
    try {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Serializar metadatos
        serializeMetadata(file);

        // Serializar entidades exportadas
        serializeEntities(file);

        // Serializar imports
        size_t importCount = moduleImports_.size();
        file.write(reinterpret_cast<const char*>(&importCount), sizeof(importCount));
        for (const auto& import : moduleImports_) {
            size_t nameLen = import.moduleName.size();
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(import.moduleName.data(), nameLen);

            size_t partitionLen = import.partitionName.size();
            file.write(reinterpret_cast<const char*>(&partitionLen), sizeof(partitionLen));
            file.write(import.partitionName.data(), partitionLen);

            file.write(reinterpret_cast<const char*>(&import.isInterfaceImport), sizeof(import.isInterfaceImport));

            size_t entityCount = import.importedEntities.size();
            file.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));
            for (const auto& entity : import.importedEntities) {
                size_t entityLen = entity.size();
                file.write(reinterpret_cast<const char*>(&entityLen), sizeof(entityLen));
                file.write(entity.data(), entityLen);
            }
        }

        // Serializar requerimientos
        size_t reqCount = moduleRequirements_.size();
        file.write(reinterpret_cast<const char*>(&reqCount), sizeof(reqCount));
        for (const auto& req : moduleRequirements_) {
            size_t modLen = req.requiredModule.size();
            file.write(reinterpret_cast<const char*>(&modLen), sizeof(modLen));
            file.write(req.requiredModule.data(), modLen);

            size_t verLen = req.minimumVersion.size();
            file.write(reinterpret_cast<const char*>(&verLen), sizeof(verLen));
            file.write(req.minimumVersion.data(), verLen);

            file.write(reinterpret_cast<const char*>(&req.isOptional), sizeof(req.isOptional));
        }

        // Serializar AST (simplificado - en un compilador real sería más complejo)
        bool hasAST = (moduleAST_ != nullptr);
        file.write(reinterpret_cast<const char*>(&hasAST), sizeof(hasAST));
        if (hasAST) {
            // Placeholder: serialización simplificada del AST
            std::string astData = "AST_PLACEHOLDER";
            size_t astLen = astData.size();
            file.write(reinterpret_cast<const char*>(&astLen), sizeof(astLen));
            file.write(astData.data(), astLen);
        }

        file.close();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error serializando BMI: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<BinaryModuleInterface> BinaryModuleInterface::deserializeFromFile(
    const std::filesystem::path& filePath) {

    try {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }

        auto bmi = std::make_unique<BinaryModuleInterface>("");

        // Deserializar metadatos
        bmi->deserializeMetadata(file);

        // Deserializar entidades
        bmi->deserializeEntities(file);

        // Deserializar imports
        size_t importCount;
        file.read(reinterpret_cast<char*>(&importCount), sizeof(importCount));
        for (size_t i = 0; i < importCount; ++i) {
            size_t nameLen;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            std::string moduleName(nameLen, '\0');
            file.read(moduleName.data(), nameLen);

            size_t partitionLen;
            file.read(reinterpret_cast<char*>(&partitionLen), sizeof(partitionLen));
            std::string partitionName(partitionLen, '\0');
            file.read(partitionName.data(), partitionLen);

            bool isInterfaceImport;
            file.read(reinterpret_cast<char*>(&isInterfaceImport), sizeof(isInterfaceImport));

            ModuleImport import(moduleName, partitionName, isInterfaceImport);

            size_t entityCount;
            file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));
            for (size_t j = 0; j < entityCount; ++j) {
                size_t entityLen;
                file.read(reinterpret_cast<char*>(&entityLen), sizeof(entityLen));
                std::string entity(entityLen, '\0');
                file.read(entity.data(), entityLen);
                import.importedEntities.push_back(entity);
            }

            bmi->addModuleImport(import);
        }

        // Deserializar requerimientos
        size_t reqCount;
        file.read(reinterpret_cast<char*>(&reqCount), sizeof(reqCount));
        for (size_t i = 0; i < reqCount; ++i) {
            size_t modLen;
            file.read(reinterpret_cast<char*>(&modLen), sizeof(modLen));
            std::string requiredModule(modLen, '\0');
            file.read(requiredModule.data(), modLen);

            size_t verLen;
            file.read(reinterpret_cast<char*>(&verLen), sizeof(verLen));
            std::string minimumVersion(verLen, '\0');
            file.read(minimumVersion.data(), verLen);

            bool isOptional;
            file.read(reinterpret_cast<char*>(&isOptional), sizeof(isOptional));

            bmi->addModuleRequirement(ModuleRequirement(requiredModule, minimumVersion, isOptional));
        }

        // Deserializar AST
        bmi->deserializeAST(file);

        file.close();
        bmi->buildIndices();

        return bmi;

    } catch (const std::exception& e) {
        std::cerr << "Error deserializando BMI: " << e.what() << std::endl;
        return nullptr;
    }
}

bool BinaryModuleInterface::isValid() const {
    return BMIValidator::validateBMI(*this);
}

const ExportedEntity* BinaryModuleInterface::findEntity(const std::string& name) const {
    auto it = entityIndex_.find(name);
    if (it != entityIndex_.end()) {
        return exportedEntities_[it->second].get();
    }
    return nullptr;
}

bool BinaryModuleInterface::isEntityExported(const std::string& name) const {
    return findEntity(name) != nullptr;
}

std::string BinaryModuleInterface::calculateHash() const {
    std::stringstream ss;

    // Incluir información clave en el hash
    ss << metadata_.moduleName << "|"
       << metadata_.sourceHash << "|"
       << metadata_.entityCount << "|";

    for (const auto& entity : exportedEntities_) {
        ss << entity->name << ":" << static_cast<int>(entity->type) << "|";
    }

    // Calcular hash simple (en un compilador real se usaría SHA256)
    std::hash<std::string> hasher;
    return std::to_string(hasher(ss.str()));
}

bool BinaryModuleInterface::isCompatibleWith(const BinaryModuleInterface& other) const {
    // Verificar versión del formato
    if (metadata_.formatVersion != other.metadata_.formatVersion) {
        return false;
    }

    // Verificar que todas las entidades exportadas estén presentes
    for (const auto& entity : exportedEntities_) {
        const auto* otherEntity = other.findEntity(entity->name);
        if (!otherEntity || otherEntity->type != entity->type) {
            return false;
        }
    }

    return true;
}

void BinaryModuleInterface::updateMetadata() {
    metadata_.entityCount = exportedEntities_.size();
    metadata_.totalSize = calculateTotalSize();

    // Calcular hash del código fuente (simplificado)
    std::stringstream ss;
    for (const auto& entity : exportedEntities_) {
        ss << entity->name;
    }
    metadata_.sourceHash = std::to_string(std::hash<std::string>()(ss.str()));
}

void BinaryModuleInterface::buildIndices() {
    entityIndex_.clear();
    for (size_t i = 0; i < exportedEntities_.size(); ++i) {
        entityIndex_[exportedEntities_[i]->name] = i;
    }
}

void BinaryModuleInterface::serializeMetadata(std::ostream& stream) const {
    // Versión del formato
    int version = static_cast<int>(metadata_.formatVersion);
    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Nombre del módulo
    size_t nameLen = metadata_.moduleName.size();
    stream.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    stream.write(metadata_.moduleName.data(), nameLen);

    // Timestamp
    size_t tsLen = metadata_.buildTimestamp.size();
    stream.write(reinterpret_cast<const char*>(&tsLen), sizeof(tsLen));
    stream.write(metadata_.buildTimestamp.data(), tsLen);

    // Versión del compilador
    size_t compLen = metadata_.compilerVersion.size();
    stream.write(reinterpret_cast<const char*>(&compLen), sizeof(compLen));
    stream.write(metadata_.compilerVersion.data(), compLen);

    // Triple de destino
    size_t tripleLen = metadata_.targetTriple.size();
    stream.write(reinterpret_cast<const char*>(&tripleLen), sizeof(tripleLen));
    stream.write(metadata_.targetTriple.data(), tripleLen);

    // Hash fuente
    size_t hashLen = metadata_.sourceHash.size();
    stream.write(reinterpret_cast<const char*>(&hashLen), sizeof(hashLen));
    stream.write(metadata_.sourceHash.data(), hashLen);

    // Conteos
    stream.write(reinterpret_cast<const char*>(&metadata_.entityCount), sizeof(metadata_.entityCount));
    stream.write(reinterpret_cast<const char*>(&metadata_.totalSize), sizeof(metadata_.totalSize));
    stream.write(reinterpret_cast<const char*>(&metadata_.symbolTableSize), sizeof(metadata_.symbolTableSize));
    stream.write(reinterpret_cast<const char*>(&metadata_.astSize), sizeof(metadata_.astSize));
}

void BinaryModuleInterface::deserializeMetadata(std::istream& stream) {
    // Versión del formato
    int version;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    metadata_.formatVersion = static_cast<BMIFormatVersion>(version);

    // Nombre del módulo
    size_t nameLen;
    stream.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    metadata_.moduleName.resize(nameLen);
    stream.read(metadata_.moduleName.data(), nameLen);

    // Timestamp
    size_t tsLen;
    stream.read(reinterpret_cast<char*>(&tsLen), sizeof(tsLen));
    metadata_.buildTimestamp.resize(tsLen);
    stream.read(metadata_.buildTimestamp.data(), tsLen);

    // Versión del compilador
    size_t compLen;
    stream.read(reinterpret_cast<char*>(&compLen), sizeof(compLen));
    metadata_.compilerVersion.resize(compLen);
    stream.read(metadata_.compilerVersion.data(), compLen);

    // Triple de destino
    size_t tripleLen;
    stream.read(reinterpret_cast<char*>(&tripleLen), sizeof(tripleLen));
    metadata_.targetTriple.resize(tripleLen);
    stream.read(metadata_.targetTriple.data(), tripleLen);

    // Hash fuente
    size_t hashLen;
    stream.read(reinterpret_cast<char*>(&hashLen), sizeof(hashLen));
    metadata_.sourceHash.resize(hashLen);
    stream.read(metadata_.sourceHash.data(), hashLen);

    // Conteos
    stream.read(reinterpret_cast<char*>(&metadata_.entityCount), sizeof(metadata_.entityCount));
    stream.read(reinterpret_cast<char*>(&metadata_.totalSize), sizeof(metadata_.totalSize));
    stream.read(reinterpret_cast<char*>(&metadata_.symbolTableSize), sizeof(metadata_.symbolTableSize));
    stream.read(reinterpret_cast<char*>(&metadata_.astSize), sizeof(metadata_.astSize));
}

void BinaryModuleInterface::serializeEntities(std::ostream& stream) const {
    size_t entityCount = exportedEntities_.size();
    stream.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));

    for (const auto& entity : exportedEntities_) {
        // Información básica
        size_t nameLen = entity->name.size();
        stream.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        stream.write(entity->name.data(), nameLen);

        size_t mangledLen = entity->mangledName.size();
        stream.write(reinterpret_cast<const char*>(&mangledLen), sizeof(mangledLen));
        stream.write(entity->mangledName.data(), mangledLen);

        int type = static_cast<int>(entity->type);
        stream.write(reinterpret_cast<const char*>(&type), sizeof(type));

        size_t modLen = entity->moduleName.size();
        stream.write(reinterpret_cast<const char*>(&modLen), sizeof(modLen));
        stream.write(entity->moduleName.data(), modLen);

        size_t locLen = entity->sourceLocation.size();
        stream.write(reinterpret_cast<const char*>(&locLen), sizeof(locLen));
        stream.write(entity->sourceLocation.data(), locLen);

        // Dependencias
        size_t depCount = entity->dependencies.size();
        stream.write(reinterpret_cast<const char*>(&depCount), sizeof(depCount));
        for (const auto& dep : entity->dependencies) {
            size_t depLen = dep.size();
            stream.write(reinterpret_cast<const char*>(&depLen), sizeof(depLen));
            stream.write(dep.data(), depLen);
        }

        // Detalles específicos del tipo (simplificado)
        // En un compilador real, esto sería más detallado
    }
}

void BinaryModuleInterface::deserializeEntities(std::istream& stream) {
    size_t entityCount;
    stream.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));

    for (size_t i = 0; i < entityCount; ++i) {
        // Información básica
        size_t nameLen;
        stream.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        stream.read(name.data(), nameLen);

        size_t mangledLen;
        stream.read(reinterpret_cast<char*>(&mangledLen), sizeof(mangledLen));
        std::string mangledName(mangledLen, '\0');
        stream.read(mangledName.data(), mangledLen);

        int typeInt;
        stream.read(reinterpret_cast<char*>(&typeInt), sizeof(typeInt));
        ExportedEntityType type = static_cast<ExportedEntityType>(typeInt);

        size_t modLen;
        stream.read(reinterpret_cast<char*>(&modLen), sizeof(modLen));
        std::string moduleName(modLen, '\0');
        stream.read(moduleName.data(), modLen);

        size_t locLen;
        stream.read(reinterpret_cast<char*>(&locLen), sizeof(locLen));
        std::string sourceLocation(locLen, '\0');
        stream.read(sourceLocation.data(), locLen);

        // Crear entidad
        auto entity = std::make_unique<ExportedEntity>(name, type, moduleName);
        entity->mangledName = mangledName;
        entity->sourceLocation = sourceLocation;

        // Dependencias
        size_t depCount;
        stream.read(reinterpret_cast<char*>(&depCount), sizeof(depCount));
        for (size_t j = 0; j < depCount; ++j) {
            size_t depLen;
            stream.read(reinterpret_cast<char*>(&depLen), sizeof(depLen));
            std::string dep(depLen, '\0');
            stream.read(dep.data(), depLen);
            entity->dependencies.push_back(dep);
        }

        // Detalles específicos (simplificado)
        addExportedEntity(std::move(entity));
    }
}

void BinaryModuleInterface::serializeAST(std::ostream& stream) const {
    // Serialización simplificada del AST
    // En un compilador real, esto sería mucho más complejo
    bool hasAST = (moduleAST_ != nullptr);
    stream.write(reinterpret_cast<const char*>(&hasAST), sizeof(hasAST));

    if (hasAST) {
        std::string astData = "AST_SERIALIZED_DATA";
        size_t astLen = astData.size();
        stream.write(reinterpret_cast<const char*>(&astLen), sizeof(astLen));
        stream.write(astData.data(), astLen);
    }
}

void BinaryModuleInterface::deserializeAST(std::istream& stream) {
    bool hasAST;
    stream.read(reinterpret_cast<char*>(&hasAST), sizeof(hasAST));

    if (hasAST) {
        size_t astLen;
        stream.read(reinterpret_cast<char*>(&astLen), sizeof(astLen));
        std::string astData(astLen, '\0');
        stream.read(astData.data(), astLen);

        // En un compilador real, aquí se deserializaría el AST completo
        // Por simplicidad, creamos un placeholder
        moduleAST_ = nullptr; // Placeholder
    }
}

size_t BinaryModuleInterface::calculateTotalSize() const {
    size_t size = sizeof(BMIMetadata);

    // Tamaño de entidades
    for (const auto& entity : exportedEntities_) {
        size += entity->name.size() + entity->mangledName.size() +
                entity->moduleName.size() + entity->sourceLocation.size();

        for (const auto& dep : entity->dependencies) {
            size += dep.size();
        }
    }

    // Tamaño de imports
    for (const auto& import : moduleImports_) {
        size += import.moduleName.size() + import.partitionName.size();
        for (const auto& entity : import.importedEntities) {
            size += entity.size();
        }
    }

    // Tamaño de requerimientos
    for (const auto& req : moduleRequirements_) {
        size += req.requiredModule.size() + req.minimumVersion.size();
    }

    return size;
}

// ============================================================================
// BMIGenerator - Implementación
// ============================================================================

BMIGenerator::BMIGenerator() = default;

std::unique_ptr<BinaryModuleInterface> BMIGenerator::generateBMI(
    const ast::ASTNode* moduleDeclaration,
    const std::string& moduleName) {

    auto bmi = std::make_unique<BinaryModuleInterface>(moduleName);

    // Extraer entidades exportadas
    auto entities = extractExportedEntities(moduleDeclaration, moduleName);
    for (auto& entity : entities) {
        bmi->addExportedEntity(std::move(entity));
    }

    // Extraer imports
    auto imports = extractModuleImports(moduleDeclaration);
    for (const auto& import : imports) {
        bmi->addModuleImport(import);
    }

    // Establecer AST
    // En un compilador real, aquí se copiaría el AST completo
    bmi->setModuleAST(nullptr); // Placeholder

    return bmi;
}

std::vector<std::unique_ptr<ExportedEntity>> BMIGenerator::extractExportedEntities(
    const ast::ASTNode* moduleDecl,
    const std::string& moduleName) {

    std::vector<std::unique_ptr<ExportedEntity>> entities;

    // En un compilador real, aquí se recorrería el AST para encontrar
    // todas las declaraciones exportadas

    // Placeholder: crear algunas entidades de ejemplo
    if (moduleName == "std") {
        auto cout = std::make_unique<ExportedEntity>("cout", ExportedEntityType::Variable, moduleName);
        cout->details.variable.varType = "std::ostream";
        cout->details.variable.isConst = false;
        entities.push_back(std::move(cout));

        auto endl = std::make_unique<ExportedEntity>("endl", ExportedEntityType::Function, moduleName);
        endl->details.function.returnType = "std::ostream&";
        endl->details.function.paramTypes = {"std::ostream&"};
        entities.push_back(std::move(endl));
    }

    return entities;
}

std::vector<ModuleImport> BMIGenerator::extractModuleImports(const ast::ASTNode* moduleDecl) {
    std::vector<ModuleImport> imports;

    // En un compilador real, aquí se analizarían las directivas import
    // Placeholder: importar algunos módulos estándar
    imports.emplace_back("std.core");
    imports.emplace_back("std.io");

    return imports;
}

bool BMIGenerator::isExportedDeclaration(const ast::ASTNode* decl) const {
    // En un compilador real, aquí se verificaría si la declaración
    // tiene el especificador 'export'
    return true; // Placeholder
}

std::unique_ptr<ExportedEntity> BMIGenerator::processExportedDeclaration(
    const ast::ASTNode* decl,
    const std::string& moduleName) {

    // En un compilador real, aquí se analizaría el AST de la declaración
    // y se crearía la entidad apropiada

    // Placeholder
    return std::make_unique<ExportedEntity>("placeholder", ExportedEntityType::Function, moduleName);
}

std::string BMIGenerator::extractTypeInfo(const ast::ASTNode* typeNode) const {
    // En un compilador real, aquí se extraería información de tipo del AST
    return "unknown_type"; // Placeholder
}

std::string BMIGenerator::generateMangledName(const ExportedEntity& entity) const {
    // En un compilador real, aquí se usaría el sistema de name mangling
    return "?" + entity.name + "@@" + entity.moduleName; // Placeholder MSVC-style
}

// ============================================================================
// BMIValidator - Implementación
// ============================================================================

bool BMIValidator::validateBMI(const BinaryModuleInterface& bmi) {
    return validateExportedEntities(bmi) && validateModuleDependencies(bmi);
}

bool BMIValidator::validateExportedEntities(const BinaryModuleInterface& bmi) {
    const auto& entities = bmi.getExportedEntities();

    // Verificar que no hay nombres duplicados
    std::unordered_set<std::string> names;
    for (const auto& entity : entities) {
        if (names.count(entity->name)) {
            return false; // Nombre duplicado
        }
        names.insert(entity->name);
    }

    // Verificar que todas las entidades tienen nombres válidos
    for (const auto& entity : entities) {
        if (entity->name.empty()) {
            return false;
        }
    }

    return true;
}

bool BMIValidator::validateModuleDependencies(const BinaryModuleInterface& bmi) {
    // Verificar que no hay dependencias circulares (simplificado)
    // En un compilador real, esto sería más complejo

    const auto& imports = bmi.getModuleImports();
    std::unordered_set<std::string> importedModules;

    for (const auto& import : imports) {
        if (importedModules.count(import.moduleName)) {
            continue; // Ya importado
        }
        importedModules.insert(import.moduleName);
    }

    return true;
}

std::string BMIValidator::generateValidationReport(const BinaryModuleInterface& bmi) {
    std::stringstream report;

    report << "BMI Validation Report for module: " << bmi.getMetadata().moduleName << "\n";
    report << "Format Version: " << static_cast<int>(bmi.getMetadata().formatVersion) << "\n";
    report << "Entity Count: " << bmi.getMetadata().entityCount << "\n";
    report << "Total Size: " << bmi.getMetadata().totalSize << " bytes\n";

    if (validateBMI(bmi)) {
        report << "Status: VALID\n";
    } else {
        report << "Status: INVALID\n";

        if (!validateExportedEntities(bmi)) {
            report << "Issue: Invalid exported entities\n";
        }
        if (!validateModuleDependencies(bmi)) {
            report << "Issue: Invalid module dependencies\n";
        }
    }

    return report.str();
}

} // namespace cpp20::compiler::modules
