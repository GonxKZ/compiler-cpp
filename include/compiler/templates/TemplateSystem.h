/**
 * @file TemplateSystem.h
 * @brief Sistema de templates C++20 con constraint solver
 */

#pragma once

#include <compiler/ast/ASTNode.h>
#include <compiler/ast/TemplateAST.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace cpp20::compiler::semantic {

/**
 * @brief Estado de evaluación de constraint
 */
enum class ConstraintSatisfaction {
    Satisfied,      // Constraint satisfecha
    NotSatisfied,   // Constraint no satisfecha
    Error           // Error durante evaluación
};

/**
 * @brief Resultado de evaluación de constraint
 */
struct ConstraintEvaluationResult {
    ConstraintSatisfaction satisfaction;
    std::string errorMessage;
    std::vector<std::string> diagnosticNotes;

    ConstraintEvaluationResult(ConstraintSatisfaction sat = ConstraintSatisfaction::Satisfied)
        : satisfaction(sat) {}
};

/**
 * @brief Información de template
 */
struct TemplateInfo {
    std::string name;
    std::unique_ptr<ast::TemplateParameterList> parameters;
    std::unique_ptr<ast::ASTNode> definition;
    std::unordered_map<std::string, std::unique_ptr<ast::ASTNode>> specializations;
    bool isConcept = false;

    TemplateInfo(const std::string& n, std::unique_ptr<ast::TemplateParameterList> params,
                std::unique_ptr<ast::ASTNode> def)
        : name(n), parameters(std::move(params)), definition(std::move(def)) {}
};

/**
 * @brief Instancia de template
 */
struct TemplateInstance {
    std::string templateName;
    std::vector<std::string> arguments;
    std::unique_ptr<ast::ASTNode> instantiatedCode;
    bool isValid = true;
    std::string errorMessage;

    TemplateInstance(const std::string& name, const std::vector<std::string>& args)
        : templateName(name), arguments(args) {}
};

/**
 * @brief Constraint Solver para concepts C++20
 */
class ConstraintSolver {
public:
    /**
     * @brief Constructor
     */
    ConstraintSolver(diagnostics::DiagnosticEngine& diagEngine);

    /**
     * @brief Evaluar constraint expression
     */
    ConstraintEvaluationResult evaluateConstraint(const ast::ConstraintExpression* constraint,
                                                const std::unordered_map<std::string, std::string>& bindings);

    /**
     * @brief Verificar si un tipo satisface un concept
     */
    ConstraintEvaluationResult checkConceptSatisfaction(const std::string& conceptName,
                                                       const std::string& typeName,
                                                       const std::unordered_map<std::string, std::string>& bindings);

    /**
     * @brief Verificar subsumption entre constraints
     */
    bool checkSubsumption(const ast::ConstraintExpression* derived,
                         const ast::ConstraintExpression* base);

private:
    diagnostics::DiagnosticEngine& diagEngine_;

    /**
     * @brief Evaluar constraint atómico
     */
    ConstraintEvaluationResult evaluateAtomicConstraint(const ast::ASTNode* atomic,
                                                       const std::unordered_map<std::string, std::string>& bindings);

    /**
     * @brief Evaluar conjunction (&&)
     */
    ConstraintEvaluationResult evaluateConjunction(const ast::ConstraintExpression* constraint,
                                                  const std::unordered_map<std::string, std::string>& bindings);

    /**
     * @brief Evaluar disjunction (||)
     */
    ConstraintEvaluationResult evaluateDisjunction(const ast::ConstraintExpression* constraint,
                                                 const std::unordered_map<std::string, std::string>& bindings);
};

/**
 * @brief Template Instantiation Engine
 */
class TemplateInstantiationEngine {
public:
    /**
     * @brief Constructor
     */
    TemplateInstantiationEngine(diagnostics::DiagnosticEngine& diagEngine,
                              ConstraintSolver& constraintSolver);

    /**
     * @brief Registrar template
     */
    void registerTemplate(std::unique_ptr<TemplateInfo> templateInfo);

    /**
     * @brief Instanciar template
     */
    std::unique_ptr<TemplateInstance> instantiateTemplate(const std::string& templateName,
                                                        const std::vector<std::string>& arguments);

    /**
     * @brief Verificar si template puede ser instanciado
     */
    bool canInstantiateTemplate(const std::string& templateName,
                               const std::vector<std::string>& arguments,
                               std::string& errorMessage);

    /**
     * @brief Obtener información de template
     */
    const TemplateInfo* getTemplateInfo(const std::string& templateName) const;

    /**
     * @brief Limpiar cache de instancias
     */
    void clearCache();

    /**
     * @brief Obtener estadísticas
     */
    struct InstantiationStats {
        size_t templatesRegistered = 0;
        size_t instancesCreated = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        size_t errors = 0;
    };
    InstantiationStats getStats() const { return stats_; }

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    ConstraintSolver& constraintSolver_;
    InstantiationStats stats_;

    std::unordered_map<std::string, std::unique_ptr<TemplateInfo>> templates_;
    std::unordered_map<std::string, std::unique_ptr<TemplateInstance>> instanceCache_;

    /**
     * @brief Generar clave de cache
     */
    std::string generateCacheKey(const std::string& templateName,
                                const std::vector<std::string>& arguments);

    /**
     * @brief Sustituir parámetros en AST
     */
    std::unique_ptr<ast::ASTNode> substituteParameters(
        const ast::ASTNode* templateAST,
        const std::unordered_map<std::string, std::string>& parameterMap);

    /**
     * @brief Verificar argumentos template
     */
    bool validateTemplateArguments(const TemplateInfo* templateInfo,
                                 const std::vector<std::string>& arguments,
                                 std::string& errorMessage);

    /**
     * @brief Verificar constraints
     */
    bool checkConstraints(const TemplateInfo* templateInfo,
                         const std::vector<std::string>& arguments,
                         std::string& errorMessage);
};

/**
 * @brief SFINAE Handler para fallos en template deduction
 */
class SFINAEHandler {
public:
    /**
     * @brief Constructor
     */
    SFINAEHandler(diagnostics::DiagnosticEngine& diagEngine);

    /**
     * @brief Registrar fallo SFINAE
     */
    void registerSFINAEFailure(const std::string& templateName,
                              const std::vector<std::string>& arguments,
                              const std::string& errorMessage);

    /**
     * @brief Verificar si es fallo SFINAE
     */
    bool isSFINAEFailure(const std::string& templateName,
                        const std::vector<std::string>& arguments) const;

    /**
     * @brief Obtener mensaje de error SFINAE
     */
    std::string getSFINAEErrorMessage(const std::string& templateName,
                                    const std::vector<std::string>& arguments) const;

    /**
     * @brief Limpiar registros SFINAE
     */
    void clear();

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    std::unordered_map<std::string, std::string> sfinaeErrors_;
};

/**
 * @brief Sistema de Templates C++20
 */
class TemplateSystem {
public:
    /**
     * @brief Constructor
     */
    TemplateSystem(diagnostics::DiagnosticEngine& diagEngine);

    /**
     * @brief Destructor
     */
    ~TemplateSystem();

    /**
     * @brief Registrar template
     */
    void registerTemplate(std::unique_ptr<TemplateInfo> templateInfo);

    /**
     * @brief Registrar concept
     */
    void registerConcept(std::unique_ptr<TemplateInfo> conceptInfo);

    /**
     * @brief Instanciar template
     */
    std::unique_ptr<TemplateInstance> instantiateTemplate(const std::string& templateName,
                                                        const std::vector<std::string>& arguments);

    /**
     * @brief Verificar concept satisfaction
     */
    ConstraintEvaluationResult checkConceptSatisfaction(const std::string& conceptName,
                                                       const std::string& typeName);

    /**
     * @brief Resolver sobrecarga con templates
     */
    std::vector<std::unique_ptr<TemplateInstance>> resolveOverload(
        const std::string& functionName,
        const std::vector<std::string>& argumentTypes);

    /**
     * @brief Limpiar cache
     */
    void clearCache();

    /**
     * @brief Obtener estadísticas
     */
    struct TemplateStats {
        size_t templatesRegistered = 0;
        size_t conceptsRegistered = 0;
        size_t instancesCreated = 0;
        size_t sfinaeFailures = 0;
        size_t constraintChecks = 0;
    };
    TemplateStats getStats() const;

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    std::unique_ptr<ConstraintSolver> constraintSolver_;
    std::unique_ptr<TemplateInstantiationEngine> instantiationEngine_;
    std::unique_ptr<SFINAEHandler> sfinaeHandler_;

    TemplateStats stats_;
};

} // namespace cpp20::compiler::semantic
