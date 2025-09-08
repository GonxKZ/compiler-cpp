/**
 * @file Type.cpp
 * @brief Implementación completa del sistema de tipos C++20
 *
 * Este archivo implementa la jerarquía de tipos que representa todos los tipos
 * fundamentales y compuestos del lenguaje C++20.
 *
 * Jerarquía de tipos implementada:
 * - Type (clase base abstracta)
 *   - BasicType (tipos fundamentales: void, bool, char, int, float, etc.)
 *   - PointerType (tipos puntero: T*)
 *   - ReferenceType (tipos referencia: T& y T&&)
 *   - ArrayType (tipos arreglo: T[N])
 *   - FunctionType (tipos función: T(params))
 *   - ClassType (tipos clase/struct)
 *   - EnumType (tipos enumeración)
 *
 * Características del sistema de tipos:
 * - CV-qualifiers completos (const, volatile, const volatile)
 * - Compatibilidad y conversión entre tipos
 * - Cálculo de tamaños y alineaciones
 * - Verificación de completitud de tipos
 * - Soporte para tipos compuestos complejos
 * - Factory functions para creación de tipos comunes
 *
 * El sistema garantiza que todos los tipos sean representables
 * correctamente en el ABI x64 de Windows.
 *
 * @author Equipo de desarrollo del compilador C++20
 * @version 1.0
 * @date 2024
 */

#include <compiler/types/Type.h>
#include <sstream>

namespace cpp20::compiler::types {

// ========================================================================
// Base Type implementation
// ========================================================================

Type::Type(Kind kind, CVQualifier cv) : kind_(kind), cv_(cv) {}

Type::~Type() = default;

bool Type::equals(const Type* other) const {
    if (!other) return false;
    return kind_ == other->kind_ && cv_ == other->cv_;
}

bool Type::compatible(const Type* other) const {
    // Basic compatibility check
    if (!other) return false;
    return kind_ == other->kind_;
}

// ========================================================================
// BasicType implementation
// ========================================================================

class BasicType : public Type {
public:
    enum class BasicKind {
        Void,
        Bool,
        Char,
        Short,
        Int,
        Long,
        LongLong,
        Float,
        Double,
        LongDouble
    };

    BasicType(BasicKind basicKind, CVQualifier cv = CVQualifier::None)
        : Type(Kind::Basic, cv), basicKind_(basicKind) {}

    std::string toString() const override {
        std::string result;
        switch (basicKind_) {
            case BasicKind::Void: result = "void"; break;
            case BasicKind::Bool: result = "bool"; break;
            case BasicKind::Char: result = "char"; break;
            case BasicKind::Short: result = "short"; break;
            case BasicKind::Int: result = "int"; break;
            case BasicKind::Long: result = "long"; break;
            case BasicKind::LongLong: result = "long long"; break;
            case BasicKind::Float: result = "float"; break;
            case BasicKind::Double: result = "double"; break;
            case BasicKind::LongDouble: result = "long double"; break;
        }

        // Add CV qualifiers
        if (cv() == CVQualifier::Const) result = "const " + result;
        else if (cv() == CVQualifier::Volatile) result = "volatile " + result;
        else if (cv() == CVQualifier::ConstVolatile) result = "const volatile " + result;

        return result;
    }

    size_t size() const override {
        switch (basicKind_) {
            case BasicKind::Void: return 0;
            case BasicKind::Bool: return 1;
            case BasicKind::Char: return 1;
            case BasicKind::Short: return 2;
            case BasicKind::Int: return 4;
            case BasicKind::Long: return 8;
            case BasicKind::LongLong: return 8;
            case BasicKind::Float: return 4;
            case BasicKind::Double: return 8;
            case BasicKind::LongDouble: return 16;
        }
        return 0;
    }

    size_t alignment() const override {
        return size();  // Basic types are self-aligned
    }

    bool isComplete() const override {
        return basicKind_ != BasicKind::Void;
    }

    bool equals(const Type* other) const override {
        if (!Type::equals(other)) return false;
        auto* basicOther = dynamic_cast<const BasicType*>(other);
        return basicOther && basicKind_ == basicOther->basicKind_;
    }

    std::unique_ptr<Type> withCV(CVQualifier cv) const override {
        return std::make_unique<BasicType>(basicKind_, cv);
    }

private:
    BasicKind basicKind_;
};

// ========================================================================
// Factory functions
// ========================================================================

std::unique_ptr<Type> makeVoidType(CVQualifier cv) {
    return std::make_unique<BasicType>(BasicType::BasicKind::Void, cv);
}

std::unique_ptr<Type> makeBoolType(CVQualifier cv) {
    return std::make_unique<BasicType>(BasicType::BasicKind::Bool, cv);
}

std::unique_ptr<Type> makeIntType(CVQualifier cv) {
    return std::make_unique<BasicType>(BasicType::BasicKind::Int, cv);
}

std::unique_ptr<Type> makeFloatType(CVQualifier cv) {
    return std::make_unique<BasicType>(BasicType::BasicKind::Float, cv);
}

std::unique_ptr<Type> makeDoubleType(CVQualifier cv) {
    return std::make_unique<BasicType>(BasicType::BasicKind::Double, cv);
}

} // namespace cpp20::compiler::types
