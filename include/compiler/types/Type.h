#pragma once

#include <memory>
#include <string>
#include <vector>

namespace cpp20::compiler::types {

/**
 * @brief Categorías de valor de C++
 */
enum class ValueCategory {
    LValue,     // l-value
    XValue,     // x-value (expiring value)
    PRValue     // pr-value (pure r-value)
};

/**
 * @brief Calificadores de tipo CV
 */
enum class CVQualifier {
    None = 0,
    Const = 1,
    Volatile = 2,
    ConstVolatile = Const | Volatile
};

/**
 * @brief Clase base para todos los tipos del sistema de tipos
 */
class Type {
public:
    enum class Kind {
        Basic,
        Pointer,
        Reference,
        Array,
        Function,
        Class,
        Enum,
        Void,
        Nullptr,
        Auto,
        Decltype
    };

    Type(Kind kind, CVQualifier cv = CVQualifier::None)
        : kind_(kind), cv_(cv) {}
    virtual ~Type() = default;

    // Getters
    Kind kind() const { return kind_; }
    CVQualifier cv() const { return cv_; }
    bool isConst() const { return static_cast<int>(cv_) & static_cast<int>(CVQualifier::Const); }
    bool isVolatile() const { return static_cast<int>(cv_) & static_cast<int>(CVQualifier::Volatile); }

    // Utilidades
    virtual std::string toString() const = 0;
    virtual size_t size() const = 0;
    virtual size_t alignment() const = 0;
    virtual bool isComplete() const = 0;

    // Comparación de tipos
    virtual bool equals(const Type* other) const = 0;
    bool compatible(const Type* other) const;

    // Modificadores
    virtual std::unique_ptr<Type> withCV(CVQualifier cv) const = 0;

private:
    Kind kind_;
    CVQualifier cv_;
};

/**
 * @brief Información de un parámetro de función
 */
struct ParameterInfo {
    std::string name;
    std::unique_ptr<Type> type;
    bool hasDefaultValue = false;
};

/**
 * @brief Información de una función
 */
struct FunctionInfo {
    std::unique_ptr<Type> returnType;
    std::vector<ParameterInfo> parameters;
    bool isVariadic = false;
    bool isNoexcept = false;
    CVQualifier cv = CVQualifier::None;
    bool isRefQualified = false;
};

} // namespace cpp20::compiler::types
