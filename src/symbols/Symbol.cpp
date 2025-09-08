/**
 * @file Symbol.cpp
 * @brief Implementación básica del sistema de símbolos
 */

#include <compiler/symbols/Symbol.h>

namespace cpp20::compiler::symbols {

// ========================================================================
// Symbol implementation
// ========================================================================

Symbol::Symbol(SymbolKind kind, std::string name, const types::Type* type)
    : kind_(kind), name_(std::move(name)), type_(type) {}

Symbol::~Symbol() = default;

std::string Symbol::toString() const {
    std::string result;
    switch (kind_) {
        case SymbolKind::Variable: result = "variable"; break;
        case SymbolKind::Function: result = "function"; break;
        case SymbolKind::Type: result = "type"; break;
        case SymbolKind::Namespace: result = "namespace"; break;
    }
    result += " " + name_;
    return result;
}

// ========================================================================
// VariableSymbol implementation
// ========================================================================

VariableSymbol::VariableSymbol(std::string name, const types::Type* type,
                             bool isConst, bool isStatic)
    : Symbol(SymbolKind::Variable, std::move(name), type),
      isConst_(isConst), isStatic_(isStatic) {}

std::string VariableSymbol::toString() const {
    std::string result = Symbol::toString();
    if (isConst_) result = "const " + result;
    if (isStatic_) result = "static " + result;
    return result;
}

// ========================================================================
// FunctionSymbol implementation
// ========================================================================

FunctionSymbol::FunctionSymbol(std::string name, const types::Type* returnType,
                             std::vector<const types::Type*> paramTypes,
                             bool isStatic)
    : Symbol(SymbolKind::Function, std::move(name), returnType),
      paramTypes_(std::move(paramTypes)), isStatic_(isStatic) {}

std::string FunctionSymbol::toString() const {
    std::string result = Symbol::toString() + "(";
    for (size_t i = 0; i < paramTypes_.size(); ++i) {
        if (i > 0) result += ", ";
        result += paramTypes_[i]->toString();
    }
    result += ")";
    return result;
}

} // namespace cpp20::compiler::symbols
