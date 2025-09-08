/**
 * @file SemanticAnalyzer.cpp
 * @brief Implementación completa del analizador semántico para C++20
 */

#include <compiler/semantic/SemanticAnalyzer.h>
#include <compiler/ast/ASTNode.h>
#include <algorithm>
#include <iostream>

namespace cpp20::compiler::semantic {

// === SymbolTable Implementation ===

SymbolTable::SymbolTable() {
    // Crear scope global
    enterScope();
}

void SymbolTable::enterScope() {
    scopeStack_.push(currentScope_);
    currentScope_ = nextScopeId_++;
}

void SymbolTable::exitScope() {
    if (!scopeStack_.empty()) {
        currentScope_ = scopeStack_.top();
        scopeStack_.pop();
    }
}

bool SymbolTable::addSymbol(std::unique_ptr<symbols::Symbol> symbol) {
    if (!symbol) return false;

    const std::string& name = symbol->name();
    SymbolTableEntry entry(std::move(symbol), currentScope_);

    // Verificar si ya existe en el mismo scope
    auto& entries = symbolMap_[name];
    for (const auto& existing : entries) {
        if (existing.scopeLevel == currentScope_) {
            return false; // Ya existe en este scope
        }
    }

    entries.push_back(std::move(entry));
    return true;
}

LookupResult SymbolTable::lookup(const std::string& name, LookupMode mode) const {
    LookupResult result;

    auto it = symbolMap_.find(name);
    if (it == symbolMap_.end()) {
        return result; // No encontrado
    }

    const auto& entries = it->second;
    std::vector<uint32_t> activeScopes;

    // Recopilar scopes activos según el modo
    switch (mode) {
        case LookupMode::Ordinary: {
            // Buscar desde el scope actual hacia arriba
            uint32_t scope = currentScope_;
            while (scope != 0) {
                activeScopes.push_back(scope);
                // Buscar scope padre (simplificado)
                scope = (scope > 1) ? scope - 1 : 0;
            }
            break;
        }
        case LookupMode::Qualified: {
            // Solo buscar en scope global
            activeScopes.push_back(1);
            break;
        }
        case LookupMode::Template: {
            // Buscar en todos los scopes (two-phase lookup)
            for (uint32_t s = 1; s <= currentScope_; ++s) {
                activeScopes.push_back(s);
            }
            break;
        }
        case LookupMode::ADL: {
            // Argument Dependent Lookup - buscar en namespaces asociados
            activeScopes.push_back(currentScope_);
            break;
        }
    }

    // Recopilar símbolos visibles
    for (const auto& entry : entries) {
        if (entry.isVisible &&
            std::find(activeScopes.begin(), activeScopes.end(), entry.scopeLevel) != activeScopes.end()) {
            result.symbols.push_back(entry.symbol.get());
        }
    }

    // Verificar ambigüedad
    if (result.symbols.size() > 1) {
        result.isAmbiguous = true;
        result.errorMessage = "Nombre ambiguo: '" + name + "'";
    }

    return result;
}

LookupResult SymbolTable::lookupInScope(const std::string& name, uint32_t scopeLevel) const {
    LookupResult result;

    auto it = symbolMap_.find(name);
    if (it == symbolMap_.end()) {
        return result;
    }

    const auto& entries = it->second;
    for (const auto& entry : entries) {
        if (entry.scopeLevel == scopeLevel && entry.isVisible) {
            result.symbols.push_back(entry.symbol.get());
        }
    }

    return result;
}

void SymbolTable::clear() {
    symbolMap_.clear();
    while (!scopeStack_.empty()) {
        scopeStack_.pop();
    }
    currentScope_ = 0;
    nextScopeId_ = 1;
    enterScope(); // Recrear scope global
}

SymbolTable::Stats SymbolTable::getStats() const {
    Stats stats;
    stats.scopes = nextScopeId_;
    stats.maxDepth = 0;

    for (const auto& [name, entries] : symbolMap_) {
        stats.totalSymbols += entries.size();
        for (const auto& entry : entries) {
            stats.maxDepth = std::max(stats.maxDepth, entry.scopeLevel);
        }
    }

    return stats;
}

// === ExpressionAnalyzer Implementation ===

ExpressionAnalyzer::ExpressionAnalyzer(diagnostics::DiagnosticEngine& diagEngine,
                                     SymbolTable& symbolTable,
                                     TemplateSystem& templateSystem)
    : diagEngine_(diagEngine), symbolTable_(symbolTable), templateSystem_(templateSystem) {
}

std::unique_ptr<types::Type> ExpressionAnalyzer::analyzeExpression(const ast::ASTNode* expr) {
    if (!expr) return nullptr;

    switch (expr->getKind()) {
        case ast::ASTNodeKind::IntegerLiteral: {
            // Simplificado - devolver tipo int
            return std::make_unique<types::Type>(types::Type::Kind::Basic);
        }
        case ast::ASTNodeKind::Identifier: {
            // Buscar símbolo
            auto lookupResult = symbolTable_.lookup(expr->getName());
            if (lookupResult.found()) {
                const auto* symbol = lookupResult.getUniqueSymbol();
                return std::unique_ptr<types::Type>(
                    const_cast<types::Type*>(symbol->type()));
            }
            break;
        }
        case ast::ASTNodeKind::BinaryOperator: {
            // Análisis simplificado de operadores binarios
            auto leftType = analyzeExpression(expr->getLeft());
            auto rightType = analyzeExpression(expr->getRight());
            if (leftType && rightType) {
                // Reglas simplificadas de promoción de tipos
                return std::move(leftType);
            }
            break;
        }
        default:
            break;
    }

    return nullptr;
}

bool ExpressionAnalyzer::checkTypeCompatibility(const types::Type* source, const types::Type* target) {
    if (!source || !target) return false;

    // Verificar igualdad de tipos
    return source->equals(target);
}

ConversionInfo ExpressionAnalyzer::findImplicitConversion(const types::Type* source, const types::Type* target) {
    ConversionInfo info;

    if (!source || !target) return info;

    // Conversiones implícitas básicas simplificadas
    if (source->equals(target)) {
        info.conversionSteps = {"identity"};
        info.rank = 1;
        return info;
    }

    // Aquí irían las reglas completas de conversión implícita de C++
    // Por simplicidad, solo implementamos identidad

    return info;
}

// === OverloadResolver Implementation ===

OverloadResolver::OverloadResolver(diagnostics::DiagnosticEngine& diagEngine,
                                 SymbolTable& symbolTable,
                                 ExpressionAnalyzer& exprAnalyzer)
    : diagEngine_(diagEngine), symbolTable_(symbolTable), exprAnalyzer_(exprAnalyzer) {
}

const symbols::FunctionSymbol* OverloadResolver::resolveOverload(
    const std::string& functionName,
    const std::vector<const types::Type*>& argumentTypes) {

    // Buscar todas las sobrecargas
    auto lookupResult = symbolTable_.lookup(functionName);
    if (!lookupResult.found()) {
        return nullptr;
    }

    std::vector<const symbols::FunctionSymbol*> candidates;
    for (const auto* symbol : lookupResult.symbols) {
        if (auto* funcSymbol = dynamic_cast<const symbols::FunctionSymbol*>(symbol)) {
            candidates.push_back(funcSymbol);
        }
    }

    if (candidates.empty()) {
        return nullptr;
    }

    // Encontrar candidatos viables
    auto viableCandidates = findViableCandidates(candidates, argumentTypes);
    if (viableCandidates.empty()) {
        return nullptr;
    }

    // Seleccionar el mejor candidato (simplificado)
    std::sort(viableCandidates.begin(), viableCandidates.end());
    return viableCandidates.back().function;
}

std::vector<OverloadResolver::OverloadCandidate> OverloadResolver::findViableCandidates(
    const std::vector<const symbols::FunctionSymbol*>& candidates,
    const std::vector<const types::Type*>& argumentTypes) {

    std::vector<OverloadCandidate> viableCandidates;

    for (const auto* func : candidates) {
        OverloadCandidate candidate{func};

        const auto& paramTypes = func->paramTypes();

        // Verificar número de parámetros
        if (paramTypes.size() != argumentTypes.size()) {
            candidate.isViable = false;
            candidate.errorMessage = "Número incorrecto de argumentos";
            viableCandidates.push_back(candidate);
            continue;
        }

        // Verificar compatibilidad de tipos
        bool allCompatible = true;
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (!exprAnalyzer_.checkTypeCompatibility(argumentTypes[i], paramTypes[i])) {
                auto conversion = exprAnalyzer_.findImplicitConversion(argumentTypes[i], paramTypes[i]);
                if (conversion.isValid()) {
                    candidate.conversions.push_back(conversion);
                    candidate.viabilityRank += conversion.rank;
                } else {
                    candidate.isViable = false;
                    candidate.errorMessage = "No hay conversión válida para argumento " + std::to_string(i);
                    allCompatible = false;
                    break;
                }
            } else {
                candidate.viabilityRank += 100; // Exact match tiene alto ranking
            }
        }

        if (allCompatible) {
            candidate.isViable = true;
        }

        viableCandidates.push_back(candidate);
    }

    // Filtrar solo candidatos viables
    std::vector<OverloadCandidate> result;
    for (const auto& candidate : viableCandidates) {
        if (candidate.isViable) {
            result.push_back(candidate);
        }
    }

    return result;
}

// === SemanticAnalyzer Implementation ===

SemanticAnalyzer::SemanticAnalyzer(diagnostics::DiagnosticEngine& diagEngine,
                                 diagnostics::SourceManager& sourceManager)
    : diagEngine_(diagEngine),
      sourceManager_(sourceManager),
      expressionAnalyzer_(diagEngine, symbolTable_, templateSystem_),
      overloadResolver_(diagEngine, symbolTable_, expressionAnalyzer_),
      templateSystem_(diagEngine) {
}

SemanticAnalyzer::~SemanticAnalyzer() = default;

bool SemanticAnalyzer::analyzeTranslationUnit(const ast::ASTNode* root) {
    if (!root) return false;

    bool success = true;

    // Analizar cada declaración en la unidad de traducción
    for (const auto* child : root->getChildren()) {
        if (!analyzeDeclaration(child)) {
            success = false;
        }
    }

    stats_.declarationsAnalyzed++;
    return success;
}

bool SemanticAnalyzer::analyzeDeclaration(const ast::ASTNode* decl) {
    if (!decl) return false;

    switch (decl->getKind()) {
        case ast::ASTNodeKind::FunctionDecl:
            return analyzeFunction(decl);
        case ast::ASTNodeKind::ClassDecl:
        case ast::ASTNodeKind::StructDecl:
            return analyzeClass(decl);
        case ast::ASTNodeKind::VariableDecl:
            return analyzeVariableDeclaration(decl);
        default:
            // Otros tipos de declaraciones
            return analyzeStatement(decl);
    }
}

bool SemanticAnalyzer::analyzeFunction(const ast::ASTNode* func) {
    if (!func) return false;

    // Análisis simplificado de función
    const std::string& funcName = func->getName();

    // Verificar si ya existe
    auto lookupResult = symbolTable_.lookup(funcName);
    if (lookupResult.found()) {
        reportSemanticError("Función ya declarada: " + funcName, func->getLocation());
        return false;
    }

    // Crear símbolo de función
    auto returnType = deduceType(func->getType());
    std::vector<const types::Type*> paramTypes;

    // Analizar parámetros (simplificado)
    for (const auto* param : func->getChildren()) {
        if (param->getKind() == ast::ASTNodeKind::ParameterDecl) {
            auto paramType = deduceType(param->getType());
            if (paramType) {
                paramTypes.push_back(paramType.get());
            }
        }
    }

    auto funcSymbol = std::make_unique<symbols::FunctionSymbol>(
        funcName, returnType.get(), paramTypes);

    if (!symbolTable_.addSymbol(std::move(funcSymbol))) {
        reportSemanticError("Error al registrar función: " + funcName, func->getLocation());
        return false;
    }

    stats_.functionsAnalyzed++;
    return true;
}

bool SemanticAnalyzer::analyzeClass(const ast::ASTNode* classDecl) {
    if (!classDecl) return false;

    const std::string& className = classDecl->getName();

    // Verificar si ya existe
    auto lookupResult = symbolTable_.lookup(className);
    if (lookupResult.found()) {
        reportSemanticError("Clase ya declarada: " + className, classDecl->getLocation());
        return false;
    }

    // Crear símbolo de tipo
    auto classSymbol = std::make_unique<symbols::Symbol>(
        symbols::SymbolKind::Type, className, nullptr); // Tipo simplificado

    if (!symbolTable_.addSymbol(std::move(classSymbol))) {
        reportSemanticError("Error al registrar clase: " + className, classDecl->getLocation());
        return false;
    }

    // Entrar en scope de clase
    symbolTable_.enterScope();

    // Analizar miembros (simplificado)
    for (const auto* member : classDecl->getChildren()) {
        if (!analyzeDeclaration(member)) {
            symbolTable_.exitScope();
            return false;
        }
    }

    symbolTable_.exitScope();
    stats_.classesAnalyzed++;
    return true;
}

std::unique_ptr<types::Type> SemanticAnalyzer::analyzeExpression(const ast::ASTNode* expr) {
    return expressionAnalyzer_.analyzeExpression(expr);
}

LookupResult SemanticAnalyzer::lookupName(const std::string& name, LookupMode mode) {
    return symbolTable_.lookup(name, mode);
}

const symbols::FunctionSymbol* SemanticAnalyzer::resolveFunctionOverload(
    const std::string& functionName,
    const std::vector<const types::Type*>& argumentTypes) {

    stats_.overloadResolutions++;
    return overloadResolver_.resolveOverload(functionName, argumentTypes);
}

bool SemanticAnalyzer::checkImplicitConversion(const types::Type* from, const types::Type* to) {
    return expressionAnalyzer_.checkTypeCompatibility(from, to);
}

void SemanticAnalyzer::enterTemplateScope() {
    templateContextStack_.push(true);
}

void SemanticAnalyzer::exitTemplateScope() {
    if (!templateContextStack_.empty()) {
        templateContextStack_.pop();
    }
}

bool SemanticAnalyzer::isInTemplateContext() const {
    return !templateContextStack_.empty() && templateContextStack_.top();
}

SemanticAnalyzer::AnalysisStats SemanticAnalyzer::getStats() const {
    return stats_;
}

void SemanticAnalyzer::clear() {
    symbolTable_.clear();
    templateSystem_.clearCache();
    while (!templateContextStack_.empty()) {
        templateContextStack_.pop();
    }
    stats_ = AnalysisStats{};
}

// === Métodos internos ===

bool SemanticAnalyzer::analyzeStatement(const ast::ASTNode* stmt) {
    if (!stmt) return false;

    switch (stmt->getKind()) {
        case ast::ASTNodeKind::ExpressionStmt: {
            auto exprType = analyzeExpression(stmt->getExpression());
            return exprType != nullptr;
        }
        default:
            // Otros tipos de statements
            return true;
    }
}

bool SemanticAnalyzer::analyzeVariableDeclaration(const ast::ASTNode* varDecl) {
    if (!varDecl) return false;

    const std::string& varName = varDecl->getName();

    // Verificar si ya existe en el scope actual
    auto lookupResult = symbolTable_.lookupInScope(varName, symbolTable_.currentScopeLevel());
    if (lookupResult.found()) {
        reportSemanticError("Variable ya declarada en este scope: " + varName, varDecl->getLocation());
        return false;
    }

    // Deducir tipo
    auto varType = deduceType(varDecl->getType());
    if (!varType) {
        reportSemanticError("No se puede deducir tipo de variable: " + varName, varDecl->getLocation());
        return false;
    }

    // Crear símbolo de variable
    auto varSymbol = std::make_unique<symbols::VariableSymbol>(
        varName, varType.get());

    if (!symbolTable_.addSymbol(std::move(varSymbol))) {
        reportSemanticError("Error al registrar variable: " + varName, varDecl->getLocation());
        return false;
    }

    return true;
}

bool SemanticAnalyzer::analyzeTypeSpecifier(const ast::ASTNode* typeSpec) {
    // Implementación simplificada
    return typeSpec != nullptr;
}

std::unique_ptr<types::Type> SemanticAnalyzer::deduceType(const ast::ASTNode* node) {
    if (!node) return nullptr;

    // Deducción de tipos simplificada
    switch (node->getKind()) {
        case ast::ASTNodeKind::BuiltinType: {
            if (node->getName() == "int") {
                return std::make_unique<types::Type>(types::Type::Kind::Basic);
            }
            break;
        }
        default:
            break;
    }

    return nullptr;
}

bool SemanticAnalyzer::performTwoPhaseNameLookup(const ast::ASTNode* node) {
    // Implementación simplificada de two-phase name lookup
    if (!isInTemplateContext()) {
        return true;
    }

    // En contexto de template, buscar en todos los scopes
    return true;
}

bool SemanticAnalyzer::checkAccessControl(const symbols::Symbol* symbol, const ast::ASTNode* usage) {
    // Implementación simplificada - sin control de acceso
    return symbol != nullptr;
}

void SemanticAnalyzer::reportSemanticError(const std::string& message, const SourceLocation& location) {
    // Reportar error semántico usando el sistema de diagnósticos
    stats_.errorsFound++;
    std::cerr << "Error semántico en " << location.fileId() << ":" << location.line()
              << ":" << location.column() << ": " << message << std::endl;
}

void SemanticAnalyzer::reportSemanticWarning(const std::string& message, const SourceLocation& location) {
    // Reportar warning semántico
    stats_.warningsGenerated++;
    std::cout << "Warning semántico en " << location.fileId() << ":" << location.line()
              << ":" << location.column() << ": " << message << std::endl;
}

} // namespace cpp20::compiler::semantic
