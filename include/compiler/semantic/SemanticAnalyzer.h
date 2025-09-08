/**
 * @file SemanticAnalyzer.h
 * @brief Analizador semántico completo para C++20
 */

#pragma once

#include <compiler/ast/ASTNode.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <compiler/common/diagnostics/SourceManager.h>
#include <compiler/symbols/Symbol.h>
#include <compiler/types/Type.h>
#include <compiler/semantic/TemplateSystem.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <stack>
#include <string>

namespace cpp20::compiler::semantic {

/**
 * @brief Modos de búsqueda de nombres
 */
enum class LookupMode {
    Ordinary,        // Búsqueda ordinaria
    Template,        // Búsqueda en contexto de template
    ADL,             // Argument Dependent Lookup
    Qualified        // Búsqueda calificada (::)
};

/**
 * @brief Resultado de búsqueda de nombres
 */
struct LookupResult {
    std::vector<const symbols::Symbol*> symbols;
    bool isAmbiguous = false;
    std::string errorMessage;

    bool found() const { return !symbols.empty() && !isAmbiguous; }
    bool empty() const { return symbols.empty(); }
    const symbols::Symbol* getUniqueSymbol() const {
        return found() && symbols.size() == 1 ? symbols[0] : nullptr;
    }
};

/**
 * @brief Información de conversión implícita
 */
struct ConversionInfo {
    types::ValueCategory sourceCategory;
    types::ValueCategory targetCategory;
    std::vector<std::string> conversionSteps;
    int rank = 0; // Ranking para resolución de sobrecarga

    bool isValid() const { return !conversionSteps.empty(); }
};

/**
 * @brief Entrada de tabla de símbolos
 */
struct SymbolTableEntry {
    std::unique_ptr<symbols::Symbol> symbol;
    uint32_t scopeLevel;
    bool isVisible = true;

    SymbolTableEntry(std::unique_ptr<symbols::Symbol> sym, uint32_t level)
        : symbol(std::move(sym)), scopeLevel(level) {}
};

/**
 * @brief Tabla de símbolos con scopes
 */
class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;

    /**
     * @brief Entrar en nuevo scope
     */
    void enterScope();

    /**
     * @brief Salir del scope actual
     */
    void exitScope();

    /**
     * @brief Añadir símbolo al scope actual
     */
    bool addSymbol(std::unique_ptr<symbols::Symbol> symbol);

    /**
     * @brief Buscar símbolo por nombre
     */
    LookupResult lookup(const std::string& name, LookupMode mode = LookupMode::Ordinary) const;

    /**
     * @brief Buscar símbolo en scope específico
     */
    LookupResult lookupInScope(const std::string& name, uint32_t scopeLevel) const;

    /**
     * @brief Obtener nivel de scope actual
     */
    uint32_t currentScopeLevel() const { return currentScope_; }

    /**
     * @brief Limpiar tabla de símbolos
     */
    void clear();

    /**
     * @brief Obtener estadísticas
     */
    struct Stats {
        size_t totalSymbols = 0;
        size_t scopes = 0;
        size_t maxDepth = 0;
    };
    Stats getStats() const;

private:
    std::unordered_map<std::string, std::vector<SymbolTableEntry>> symbolMap_;
    std::stack<uint32_t> scopeStack_;
    uint32_t currentScope_ = 0;
    uint32_t nextScopeId_ = 1;
};

/**
 * @brief Analizador de expresiones
 */
class ExpressionAnalyzer {
public:
    ExpressionAnalyzer(diagnostics::DiagnosticEngine& diagEngine,
                      SymbolTable& symbolTable,
                      TemplateSystem& templateSystem);

    /**
     * @brief Analizar expresión
     */
    std::unique_ptr<types::Type> analyzeExpression(const ast::ASTNode* expr);

    /**
     * @brief Verificar compatibilidad de tipos
     */
    bool checkTypeCompatibility(const types::Type* source, const types::Type* target);

    /**
     * @brief Encontrar conversiones implícitas
     */
    ConversionInfo findImplicitConversion(const types::Type* source, const types::Type* target);

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    SymbolTable& symbolTable_;
    TemplateSystem& templateSystem_;
};

/**
 * @brief Resolvedor de sobrecargas
 */
class OverloadResolver {
public:
    struct OverloadCandidate {
        const symbols::FunctionSymbol* function;
        std::vector<ConversionInfo> conversions;
        int viabilityRank = 0;
        bool isViable = true;
        std::string errorMessage;

        bool operator<(const OverloadCandidate& other) const {
            return viabilityRank < other.viabilityRank;
        }
    };

    OverloadResolver(diagnostics::DiagnosticEngine& diagEngine,
                    SymbolTable& symbolTable,
                    ExpressionAnalyzer& exprAnalyzer);

    /**
     * @brief Resolver sobrecarga de función
     */
    const symbols::FunctionSymbol* resolveOverload(
        const std::string& functionName,
        const std::vector<const types::Type*>& argumentTypes);

    /**
     * @brief Encontrar mejores candidatos viables
     */
    std::vector<OverloadCandidate> findViableCandidates(
        const std::vector<const symbols::FunctionSymbol*>& candidates,
        const std::vector<const types::Type*>& argumentTypes);

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    SymbolTable& symbolTable_;
    ExpressionAnalyzer& exprAnalyzer_;
};

/**
 * @brief Analizador semántico principal para C++20
 *
 * Implementa el análisis semántico completo incluyendo:
 * - Name lookup en dos fases para templates
 * - Reglas de conversión implícitas
 * - Resolución de sobrecarga
 * - ADL (Argument Dependent Lookup)
 * - Sistema de templates y constraints
 * - Análisis de expresiones
 */
class SemanticAnalyzer {
public:
    /**
     * @brief Constructor
     */
    SemanticAnalyzer(diagnostics::DiagnosticEngine& diagEngine,
                    diagnostics::SourceManager& sourceManager);

    /**
     * @brief Destructor
     */
    ~SemanticAnalyzer();

    /**
     * @brief Analizar unidad de traducción completa
     */
    bool analyzeTranslationUnit(const ast::ASTNode* root);

    /**
     * @brief Analizar declaración
     */
    bool analyzeDeclaration(const ast::ASTNode* decl);

    /**
     * @brief Analizar función
     */
    bool analyzeFunction(const ast::ASTNode* func);

    /**
     * @brief Analizar clase/struct
     */
    bool analyzeClass(const ast::ASTNode* classDecl);

    /**
     * @brief Analizar expresión
     */
    std::unique_ptr<types::Type> analyzeExpression(const ast::ASTNode* expr);

    /**
     * @brief Buscar nombre con modo específico
     */
    LookupResult lookupName(const std::string& name, LookupMode mode = LookupMode::Ordinary);

    /**
     * @brief Resolver sobrecarga de función
     */
    const symbols::FunctionSymbol* resolveFunctionOverload(
        const std::string& functionName,
        const std::vector<const types::Type*>& argumentTypes);

    /**
     * @brief Verificar conversión implícita
     */
    bool checkImplicitConversion(const types::Type* from, const types::Type* to);

    /**
     * @brief Entrar en scope de template
     */
    void enterTemplateScope();

    /**
     * @brief Salir de scope de template
     */
    void exitTemplateScope();

    /**
     * @brief Verificar si estamos en contexto de template
     */
    bool isInTemplateContext() const;

    /**
     * @brief Obtener estadísticas del análisis
     */
    struct AnalysisStats {
        size_t declarationsAnalyzed = 0;
        size_t expressionsAnalyzed = 0;
        size_t functionsAnalyzed = 0;
        size_t classesAnalyzed = 0;
        size_t overloadResolutions = 0;
        size_t templateInstantiations = 0;
        size_t errorsFound = 0;
        size_t warningsGenerated = 0;
    };
    AnalysisStats getStats() const;

    /**
     * @brief Limpiar estado del analizador
     */
    void clear();

private:
    diagnostics::DiagnosticEngine& diagEngine_;
    diagnostics::SourceManager& sourceManager_;

    // Componentes principales
    SymbolTable symbolTable_;
    ExpressionAnalyzer expressionAnalyzer_;
    OverloadResolver overloadResolver_;
    TemplateSystem templateSystem_;

    // Estado
    std::stack<bool> templateContextStack_;
    AnalysisStats stats_;

    // Métodos internos
    bool analyzeStatement(const ast::ASTNode* stmt);
    bool analyzeVariableDeclaration(const ast::ASTNode* varDecl);
    bool analyzeTypeSpecifier(const ast::ASTNode* typeSpec);
    std::unique_ptr<types::Type> deduceType(const ast::ASTNode* node);
    bool performTwoPhaseNameLookup(const ast::ASTNode* node);
    bool checkAccessControl(const symbols::Symbol* symbol, const ast::ASTNode* usage);
    void reportSemanticError(const std::string& message, const SourceLocation& location);
    void reportSemanticWarning(const std::string& message, const SourceLocation& location);
};

} // namespace cpp20::compiler::semantic
