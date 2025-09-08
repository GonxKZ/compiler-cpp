#pragma once

#include <compiler/types/Type.h>
#include <string>
#include <vector>
#include <memory>

namespace cpp20::compiler::symbols {

/**
 * @brief Tipos de símbolos
 */
enum class SymbolKind {
    Variable,
    Function,
    Type,
    Namespace
};

/**
 * @brief Clase base para todos los símbolos
 */
class Symbol {
public:
    Symbol(SymbolKind kind, std::string name, const types::Type* type);
    virtual ~Symbol();

    SymbolKind kind() const { return kind_; }
    const std::string& name() const { return name_; }
    const types::Type* type() const { return type_; }

    virtual std::string toString() const;

private:
    SymbolKind kind_;
    std::string name_;
    const types::Type* type_;
};

/**
 * @brief Símbolo de variable
 */
class VariableSymbol : public Symbol {
public:
    VariableSymbol(std::string name, const types::Type* type,
                  bool isConst = false, bool isStatic = false);

    bool isConst() const { return isConst_; }
    bool isStatic() const { return isStatic_; }

    std::string toString() const override;

private:
    bool isConst_;
    bool isStatic_;
};

/**
 * @brief Símbolo de función
 */
class FunctionSymbol : public Symbol {
public:
    FunctionSymbol(std::string name, const types::Type* returnType,
                  std::vector<const types::Type*> paramTypes,
                  bool isStatic = false);

    const std::vector<const types::Type*>& paramTypes() const { return paramTypes_; }
    bool isStatic() const { return isStatic_; }

    std::string toString() const override;

private:
    std::vector<const types::Type*> paramTypes_;
    bool isStatic_;
};

} // namespace cpp20::compiler::symbols
