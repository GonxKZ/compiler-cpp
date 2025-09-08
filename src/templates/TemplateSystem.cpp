/**
 * @file TemplateSystem.cpp
 * @brief Implementación del sistema de templates C++20
 */

#include <compiler/templates/TemplateSystem.h>
#include <algorithm>

namespace cpp20::compiler::semantic {

// ============================================================================
// ConstraintSolver - Implementación
// ============================================================================

ConstraintSolver::ConstraintSolver(diagnostics::DiagnosticEngine& diagEngine)
    : diagEngine_(diagEngine) {
}

ConstraintEvaluationResult ConstraintSolver::evaluateConstraint(
    const ast::ConstraintExpression* constraint,
    const std::unordered_map<std::string, std::string>& bindings) {

    if (!constraint) {
        return ConstraintEvaluationResult(ConstraintSatisfaction::Error);
    }

    switch (constraint->getConstraintType()) {
        case ast::ConstraintExpression::ConstraintType::Atomic:
            return evaluateAtomicConstraint(constraint->getLeft(), bindings);

        case ast::ConstraintExpression::ConstraintType::Conjunction:
        case ast::ConstraintExpression::ConstraintType::LogicalAnd:
            return evaluateConjunction(constraint, bindings);

        case ast::ConstraintExpression::ConstraintType::Disjunction:
        case ast::ConstraintExpression::ConstraintType::LogicalOr:
            return evaluateDisjunction(constraint, bindings);

        default:
            ConstraintEvaluationResult result(ConstraintSatisfaction::Error);
            result.errorMessage = "Tipo de constraint no soportado";
            return result;
    }
}

ConstraintEvaluationResult ConstraintSolver::checkConceptSatisfaction(
    const std::string& conceptName,
    const std::string& typeName,
    const std::unordered_map<std::string, std::string>& bindings) {

    // Implementación simplificada - en un compilador real esto sería mucho más complejo
    ConstraintEvaluationResult result;

    // Simular verificación de concepts comunes
    if (conceptName == "Integral" || conceptName == "std::integral") {
        // Verificar si es tipo integral
        if (typeName == "int" || typeName == "long" || typeName == "short" ||
            typeName == "char" || typeName.find("unsigned") != std::string::npos) {
            result.satisfaction = ConstraintSatisfaction::Satisfied;
        } else {
            result.satisfaction = ConstraintSatisfaction::NotSatisfied;
            result.errorMessage = "Tipo '" + typeName + "' no satisface concept 'Integral'";
        }
    } else if (conceptName == "FloatingPoint" || conceptName == "std::floating_point") {
        // Verificar si es tipo flotante
        if (typeName == "float" || typeName == "double" || typeName == "long double") {
            result.satisfaction = ConstraintSatisfaction::Satisfied;
        } else {
            result.satisfaction = ConstraintSatisfaction::NotSatisfied;
            result.errorMessage = "Tipo '" + typeName + "' no satisface concept 'FloatingPoint'";
        }
    } else {
        result.satisfaction = ConstraintSatisfaction::Error;
        result.errorMessage = "Concept '" + conceptName + "' no encontrado";
    }

    return result;
}

bool ConstraintSolver::checkSubsumption(const ast::ConstraintExpression* derived,
                                       const ast::ConstraintExpression* base) {
    // Implementación simplificada de subsumption
    if (!derived || !base) return false;

    // Para esta implementación simplificada, asumimos que constraints idénticos se subsumen
    if (derived->getConstraintType() == base->getConstraintType()) {
        return true;
    }

    return false;
}

ConstraintEvaluationResult ConstraintSolver::evaluateAtomicConstraint(
    const ast::ASTNode* atomic,
    const std::unordered_map<std::string, std::string>& bindings) {

    ConstraintEvaluationResult result;

    // Implementación simplificada para constraints atómicos
    if (atomic && atomic->getKind() == ast::ASTNodeKind::Identifier) {
        // Simular verificación de constraint atómico
        result.satisfaction = ConstraintSatisfaction::Satisfied;
    } else {
        result.satisfaction = ConstraintSatisfaction::NotSatisfied;
        result.errorMessage = "Constraint atómico no válido";
    }

    return result;
}

ConstraintEvaluationResult ConstraintSolver::evaluateConjunction(
    const ast::ConstraintExpression* constraint,
    const std::unordered_map<std::string, std::string>& bindings) {

    // Evaluar izquierda
    auto leftResult = evaluateConstraint(
        dynamic_cast<const ast::ConstraintExpression*>(constraint->getLeft()), bindings);

    if (leftResult.satisfaction != ConstraintSatisfaction::Satisfied) {
        return leftResult;
    }

    // Evaluar derecha
    auto rightResult = evaluateConstraint(
        dynamic_cast<const ast::ConstraintExpression*>(constraint->getRight()), bindings);

    return rightResult;
}

ConstraintEvaluationResult ConstraintSolver::evaluateDisjunction(
    const ast::ConstraintExpression* constraint,
    const std::unordered_map<std::string, std::string>& bindings) {

    // Evaluar izquierda
    auto leftResult = evaluateConstraint(
        dynamic_cast<const ast::ConstraintExpression*>(constraint->getLeft()), bindings);

    if (leftResult.satisfaction == ConstraintSatisfaction::Satisfied) {
        return leftResult;
    }

    // Evaluar derecha
    auto rightResult = evaluateConstraint(
        dynamic_cast<const ast::ConstraintExpression*>(constraint->getRight()), bindings);

    return rightResult;
}

// ============================================================================
// TemplateInstantiationEngine - Implementación
// ============================================================================

TemplateInstantiationEngine::TemplateInstantiationEngine(diagnostics::DiagnosticEngine& diagEngine,
                                                       ConstraintSolver& constraintSolver)
    : diagEngine_(diagEngine), constraintSolver_(constraintSolver) {
}

void TemplateInstantiationEngine::registerTemplate(std::unique_ptr<TemplateInfo> templateInfo) {
    if (templateInfo) {
        templates_[templateInfo->name] = std::move(templateInfo);
        ++stats_.templatesRegistered;
    }
}

std::unique_ptr<TemplateInstance> TemplateInstantiationEngine::instantiateTemplate(
    const std::string& templateName,
    const std::vector<std::string>& arguments) {

    auto instance = std::make_unique<TemplateInstance>(templateName, arguments);

    // Verificar si ya está en cache
    std::string cacheKey = generateCacheKey(templateName, arguments);
    auto it = instanceCache_.find(cacheKey);
    if (it != instanceCache_.end()) {
        ++stats_.cacheHits;
        return std::make_unique<TemplateInstance>(*it->second);
    }

    ++stats_.cacheMisses;

    // Obtener información del template
    const TemplateInfo* templateInfo = getTemplateInfo(templateName);
    if (!templateInfo) {
        instance->isValid = false;
        instance->errorMessage = "Template '" + templateName + "' no encontrado";
        ++stats_.errors;
        return instance;
    }

    // Validar argumentos
    std::string validationError;
    if (!validateTemplateArguments(templateInfo, arguments, validationError)) {
        instance->isValid = false;
        instance->errorMessage = validationError;
        ++stats_.errors;
        return instance;
    }

    // Verificar constraints
    std::string constraintError;
    if (!checkConstraints(templateInfo, arguments, constraintError)) {
        instance->isValid = false;
        instance->errorMessage = constraintError;
        ++stats_.errors;
        return instance;
    }

    // Crear mapeo de parámetros
    std::unordered_map<std::string, std::string> parameterMap;
    const auto& params = templateInfo->parameters->getParameters();
    for (size_t i = 0; i < params.size() && i < arguments.size(); ++i) {
        parameterMap[params[i]->getName()] = arguments[i];
    }

    // Sustituir parámetros en el AST
    instance->instantiatedCode = substituteParameters(templateInfo->definition.get(), parameterMap);

    // Cachear la instancia
    instanceCache_[cacheKey] = std::make_unique<TemplateInstance>(*instance);
    ++stats_.instancesCreated;

    return instance;
}

bool TemplateInstantiationEngine::canInstantiateTemplate(const std::string& templateName,
                                                       const std::vector<std::string>& arguments,
                                                       std::string& errorMessage) {
    const TemplateInfo* templateInfo = getTemplateInfo(templateName);
    if (!templateInfo) {
        errorMessage = "Template '" + templateName + "' no encontrado";
        return false;
    }

    if (!validateTemplateArguments(templateInfo, arguments, errorMessage)) {
        return false;
    }

    if (!checkConstraints(templateInfo, arguments, errorMessage)) {
        return false;
    }

    return true;
}

const TemplateInfo* TemplateInstantiationEngine::getTemplateInfo(const std::string& templateName) const {
    auto it = templates_.find(templateName);
    return it != templates_.end() ? it->second.get() : nullptr;
}

void TemplateInstantiationEngine::clearCache() {
    instanceCache_.clear();
}

std::string TemplateInstantiationEngine::generateCacheKey(const std::string& templateName,
                                                        const std::vector<std::string>& arguments) {
    std::string key = templateName + "<";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) key += ",";
        key += arguments[i];
    }
    key += ">";
    return key;
}

std::unique_ptr<ast::ASTNode> TemplateInstantiationEngine::substituteParameters(
    const ast::ASTNode* templateAST,
    const std::unordered_map<std::string, std::string>& parameterMap) {

    // Implementación simplificada - en un compilador real esto sería mucho más complejo
    // Aquí solo copiaríamos el AST y reemplazaríamos las referencias a parámetros

    if (!templateAST) return nullptr;

    // Para esta implementación, simplemente devolvemos una copia
    // En la práctica, necesitaríamos un visitor que recorra el AST
    // y reemplace las referencias a parámetros template

    return std::make_unique<ast::ASTNode>(ast::ASTNodeKind::CompoundStmt);
}

bool TemplateInstantiationEngine::validateTemplateArguments(
    const TemplateInfo* templateInfo,
    const std::vector<std::string>& arguments,
    std::string& errorMessage) {

    const auto& params = templateInfo->parameters->getParameters();

    if (params.size() != arguments.size()) {
        errorMessage = "Número incorrecto de argumentos template. Esperados: " +
                      std::to_string(params.size()) + ", obtenidos: " +
                      std::to_string(arguments.size());
        return false;
    }

    // Validación básica de tipos (simplificada)
    for (size_t i = 0; i < params.size(); ++i) {
        // Aquí iría validación más sofisticada de tipos de argumentos
        // vs tipos de parámetros
    }

    return true;
}

bool TemplateInstantiationEngine::checkConstraints(
    const TemplateInfo* templateInfo,
    const std::vector<std::string>& arguments,
    std::string& errorMessage) {

    // Implementación simplificada - verificar constraints básicos
    // En un compilador real, esto sería mucho más complejo

    // Crear bindings para evaluación de constraints
    std::unordered_map<std::string, std::string> bindings;
    const auto& params = templateInfo->parameters->getParameters();
    for (size_t i = 0; i < params.size() && i < arguments.size(); ++i) {
        bindings[params[i]->getName()] = arguments[i];
    }

    // Aquí iría la evaluación real de constraints
    // Para esta implementación simplificada, asumimos que pasan

    return true;
}

// ============================================================================
// SFINAEHandler - Implementación
// ============================================================================

SFINAEHandler::SFINAEHandler(diagnostics::DiagnosticEngine& diagEngine)
    : diagEngine_(diagEngine) {
}

void SFINAEHandler::registerSFINAEFailure(const std::string& templateName,
                                        const std::vector<std::string>& arguments,
                                        const std::string& errorMessage) {
    std::string key = templateName + "<";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) key += ",";
        key += arguments[i];
    }
    key += ">";

    sfinaeErrors_[key] = errorMessage;
}

bool SFINAEHandler::isSFINAEFailure(const std::string& templateName,
                                  const std::vector<std::string>& arguments) const {
    std::string key = templateName + "<";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) key += ",";
        key += arguments[i];
    }
    key += ">";

    return sfinaeErrors_.find(key) != sfinaeErrors_.end();
}

std::string SFINAEHandler::getSFINAEErrorMessage(const std::string& templateName,
                                               const std::vector<std::string>& arguments) const {
    std::string key = templateName + "<";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) key += ",";
        key += arguments[i];
    }
    key += ">";

    auto it = sfinaeErrors_.find(key);
    return it != sfinaeErrors_.end() ? it->second : "";
}

void SFINAEHandler::clear() {
    sfinaeErrors_.clear();
}

// ============================================================================
// TemplateSystem - Implementación
// ============================================================================

TemplateSystem::TemplateSystem(diagnostics::DiagnosticEngine& diagEngine)
    : diagEngine_(diagEngine),
      constraintSolver_(std::make_unique<ConstraintSolver>(diagEngine)),
      instantiationEngine_(std::make_unique<TemplateInstantiationEngine>(diagEngine, *constraintSolver_)),
      sfinaeHandler_(std::make_unique<SFINAEHandler>(diagEngine)) {
}

TemplateSystem::~TemplateSystem() = default;

void TemplateSystem::registerTemplate(std::unique_ptr<TemplateInfo> templateInfo) {
    if (templateInfo) {
        instantiationEngine_->registerTemplate(std::move(templateInfo));
        ++stats_.templatesRegistered;
    }
}

void TemplateSystem::registerConcept(std::unique_ptr<TemplateInfo> conceptInfo) {
    if (conceptInfo) {
        conceptInfo->isConcept = true;
        instantiationEngine_->registerTemplate(std::move(conceptInfo));
        ++stats_.conceptsRegistered;
    }
}

std::unique_ptr<TemplateInstance> TemplateSystem::instantiateTemplate(
    const std::string& templateName,
    const std::vector<std::string>& arguments) {

    auto instance = instantiationEngine_->instantiateTemplate(templateName, arguments);

    if (instance && instance->isValid) {
        ++stats_.instancesCreated;
    } else {
        ++stats_.sfinaeFailures;
    }

    return instance;
}

ConstraintEvaluationResult TemplateSystem::checkConceptSatisfaction(
    const std::string& conceptName,
    const std::string& typeName) {

    ++stats_.constraintChecks;
    std::unordered_map<std::string, std::string> bindings;
    return constraintSolver_->checkConceptSatisfaction(conceptName, typeName, bindings);
}

std::vector<std::unique_ptr<TemplateInstance>> TemplateSystem::resolveOverload(
    const std::string& functionName,
    const std::vector<std::string>& argumentTypes) {

    std::vector<std::unique_ptr<TemplateInstance>> candidates;

    // Implementación simplificada de resolución de sobrecarga
    // En un compilador real, esto buscaría todas las sobrecargas posibles
    // y aplicaría las reglas de resolución de sobrecarga de C++

    // Para esta implementación, simplemente intentamos instanciar
    // con los argumentos proporcionados
    auto instance = instantiateTemplate(functionName, argumentTypes);
    if (instance && instance->isValid) {
        candidates.push_back(std::move(instance));
    }

    return candidates;
}

void TemplateSystem::clearCache() {
    instantiationEngine_->clearCache();
    sfinaeHandler_->clear();
}

TemplateSystem::TemplateStats TemplateSystem::getStats() const {
    TemplateStats combinedStats = stats_;

    // Agregar estadísticas del instantiation engine
    auto instStats = instantiationEngine_->getStats();
    combinedStats.instancesCreated = instStats.instancesCreated;

    return combinedStats;
}

} // namespace cpp20::compiler::semantic
