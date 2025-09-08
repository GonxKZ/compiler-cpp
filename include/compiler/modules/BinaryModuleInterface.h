/**
 * @file BinaryModuleInterface.h
 * @brief Formato BMI (Binary Module Interface) para módulos C++20
 */

#pragma once

#include <compiler/ast/ASTNode.h>
#include <compiler/types/Type.h>
#include <compiler/symbols/Symbol.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <string>

namespace cpp20::compiler::modules {

/**
 * @brief Versión del formato BMI
 */
enum class BMIFormatVersion {
    Version1_0 = 1,  // C++20 inicial
    Version1_1 = 2,  // Con mejoras de rendimiento
    Version2_0 = 3   // C++23+
};

/**
 * @brief Tipos de entidades exportadas en un módulo
 */
enum class ExportedEntityType {
    Function,
    Variable,
    Type,
    Template,
    Namespace,
    Concept,
    Module      // Submódulo
};

/**
 * @brief Información de una entidad exportada
 */
struct ExportedEntity {
    std::string name;                    // Nombre de la entidad
    std::string mangledName;             // Nombre mangled (si aplica)
    ExportedEntityType type;             // Tipo de entidad
    std::string moduleName;              // Nombre del módulo que la define
    std::string sourceLocation;          // Ubicación en el código fuente
    std::vector<std::string> dependencies; // Dependencias de esta entidad

    // Información específica del tipo
    union {
        struct {
            std::string returnType;
            std::vector<std::string> paramTypes;
            bool isConstexpr;
            bool isNoexcept;
        } function;

        struct {
            std::string varType;
            bool isConst;
            bool isThreadLocal;
        } variable;

        struct {
            bool isClass;
            bool isEnum;
            bool isUnion;
            std::vector<std::string> baseClasses;
        } typeInfo;

        struct {
            std::vector<std::string> templateParams;
            bool isConcept;
        } templateInfo;
    } details;

    ExportedEntity(const std::string& n, ExportedEntityType t, const std::string& mod)
        : name(n), type(t), moduleName(mod) {}

    // Métodos de conveniencia
    bool isFunction() const { return type == ExportedEntityType::Function; }
    bool isVariable() const { return type == ExportedEntityType::Variable; }
    bool isType() const { return type == ExportedEntityType::Type; }
    bool isTemplate() const { return type == ExportedEntityType::Template; }
};

/**
 * @brief Información de import de un módulo
 */
struct ModuleImport {
    std::string moduleName;              // Nombre del módulo importado
    std::string partitionName;           // Nombre de la partición (opcional)
    bool isInterfaceImport;             // Import de interfaz vs implementación
    std::vector<std::string> importedEntities; // Entidades específicas importadas

    ModuleImport(const std::string& mod, const std::string& partition = "",
                bool interface = true)
        : moduleName(mod), partitionName(partition), isInterfaceImport(interface) {}
};

/**
 * @brief Información de requerimiento de un módulo
 */
struct ModuleRequirement {
    std::string requiredModule;          // Módulo requerido
    std::string minimumVersion;          // Versión mínima requerida
    bool isOptional;                     // Si es opcional

    ModuleRequirement(const std::string& mod, const std::string& version = "",
                     bool optional = false)
        : requiredModule(mod), minimumVersion(version), isOptional(optional) {}
};

/**
 * @brief Metadatos del BMI
 */
struct BMIMetadata {
    BMIFormatVersion formatVersion;      // Versión del formato
    std::string moduleName;              // Nombre del módulo
    std::string buildTimestamp;          // Timestamp de compilación
    std::string compilerVersion;         // Versión del compilador
    std::string targetTriple;            // Triple de destino
    std::string sourceHash;              // Hash del código fuente
    size_t entityCount;                  // Número de entidades exportadas

    // Estadísticas
    size_t totalSize;                    // Tamaño total del BMI en bytes
    size_t symbolTableSize;              // Tamaño de la tabla de símbolos
    size_t astSize;                      // Tamaño del AST serializado

    BMIMetadata(const std::string& modName = "")
        : formatVersion(BMIFormatVersion::Version1_0), moduleName(modName),
          entityCount(0), totalSize(0), symbolTableSize(0), astSize(0) {}
};

/**
 * @brief BMI (Binary Module Interface) completo
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
    ~BinaryModuleInterface();

    /**
     * @brief Añade una entidad exportada
     */
    void addExportedEntity(std::unique_ptr<ExportedEntity> entity);

    /**
     * @brief Añade un import de módulo
     */
    void addModuleImport(const ModuleImport& import);

    /**
     * @brief Añade un requerimiento de módulo
     */
    void addModuleRequirement(const ModuleRequirement& requirement);

    /**
     * @brief Establece el AST del módulo
     */
    void setModuleAST(std::unique_ptr<ast::ASTNode> ast);

    /**
     * @brief Serializa el BMI a un archivo
     */
    bool serializeToFile(const std::filesystem::path& filePath);

    /**
     * @brief Deserializa un BMI desde un archivo
     */
    static std::unique_ptr<BinaryModuleInterface> deserializeFromFile(
        const std::filesystem::path& filePath);

    /**
     * @brief Verifica si el BMI es válido
     */
    bool isValid() const;

    /**
     * @brief Obtiene metadatos del BMI
     */
    const BMIMetadata& getMetadata() const { return metadata_; }

    /**
     * @brief Obtiene entidades exportadas
     */
    const std::vector<std::unique_ptr<ExportedEntity>>& getExportedEntities() const {
        return exportedEntities_;
    }

    /**
     * @brief Busca una entidad exportada por nombre
     */
    const ExportedEntity* findEntity(const std::string& name) const;

    /**
     * @brief Verifica si una entidad está exportada
     */
    bool isEntityExported(const std::string& name) const;

    /**
     * @brief Obtiene imports del módulo
     */
    const std::vector<ModuleImport>& getModuleImports() const { return moduleImports_; }

    /**
     * @brief Obtiene requerimientos del módulo
     */
    const std::vector<ModuleRequirement>& getModuleRequirements() const {
        return moduleRequirements_;
    }

    /**
     * @brief Obtiene el AST del módulo
     */
    const ast::ASTNode* getModuleAST() const { return moduleAST_.get(); }

    /**
     * @brief Calcula hash del BMI para comparación
     */
    std::string calculateHash() const;

    /**
     * @brief Verifica compatibilidad con otro BMI
     */
    bool isCompatibleWith(const BinaryModuleInterface& other) const;

private:
    BMIMetadata metadata_;
    std::vector<std::unique_ptr<ExportedEntity>> exportedEntities_;
    std::vector<ModuleImport> moduleImports_;
    std::vector<ModuleRequirement> moduleRequirements_;
    std::unique_ptr<ast::ASTNode> moduleAST_;

    // Índices para búsqueda rápida
    std::unordered_map<std::string, size_t> entityIndex_;

    /**
     * @brief Actualiza metadatos
     */
    void updateMetadata();

    /**
     * @brief Construye índices de búsqueda
     */
    void buildIndices();

    /**
     * @brief Serializa metadatos
     */
    void serializeMetadata(std::ostream& stream) const;

    /**
     * @brief Deserializa metadatos
     */
    void deserializeMetadata(std::istream& stream);

    /**
     * @brief Serializa entidades exportadas
     */
    void serializeEntities(std::ostream& stream) const;

    /**
     * @brief Deserializa entidades exportadas
     */
    void deserializeEntities(std::istream& stream);

    /**
     * @brief Serializa AST del módulo
     */
    void serializeAST(std::ostream& stream) const;

    /**
     * @brief Deserializa AST del módulo
     */
    void deserializeAST(std::istream& stream);

    /**
     * @brief Calcula tamaño total del BMI
     */
    size_t calculateTotalSize() const;
};

/**
 * @brief Generador de BMI a partir de AST
 */
class BMIGenerator {
public:
    /**
     * @brief Constructor
     */
    BMIGenerator();

    /**
     * @brief Genera BMI a partir de un módulo
     */
    std::unique_ptr<BinaryModuleInterface> generateBMI(
        const ast::ASTNode* moduleDeclaration,
        const std::string& moduleName);

    /**
     * @brief Extrae entidades exportadas del AST
     */
    std::vector<std::unique_ptr<ExportedEntity>> extractExportedEntities(
        const ast::ASTNode* moduleDecl,
        const std::string& moduleName);

    /**
     * @brief Extrae imports de módulo del AST
     */
    std::vector<ModuleImport> extractModuleImports(const ast::ASTNode* moduleDecl);

    /**
     * @brief Verifica si una declaración es exportada
     */
    bool isExportedDeclaration(const ast::ASTNode* decl) const;

private:
    /**
     * @brief Procesa una declaración exportada
     */
    std::unique_ptr<ExportedEntity> processExportedDeclaration(
        const ast::ASTNode* decl,
        const std::string& moduleName);

    /**
     * @brief Extrae información de tipo de una declaración
     */
    std::string extractTypeInfo(const ast::ASTNode* typeNode) const;

    /**
     * @brief Genera nombre mangled para una entidad
     */
    std::string generateMangledName(const ExportedEntity& entity) const;
};

/**
 * @brief Validador de BMI
 */
class BMIValidator {
public:
    /**
     * @brief Valida estructura de un BMI
     */
    static bool validateBMI(const BinaryModuleInterface& bmi);

    /**
     * @brief Valida consistencia de entidades exportadas
     */
    static bool validateExportedEntities(const BinaryModuleInterface& bmi);

    /**
     * @brief Valida dependencias entre módulos
     */
    static bool validateModuleDependencies(const BinaryModuleInterface& bmi);

    /**
     * @brief Genera reporte de validación
     */
    static std::string generateValidationReport(const BinaryModuleInterface& bmi);
};

} // namespace cpp20::compiler::modules
