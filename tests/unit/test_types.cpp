/**
 * @file test_types.cpp
 * @brief Tests unitarios para el sistema de tipos
 */

#include <gtest/gtest.h>
#include <compiler/types/Type.h>
#include <compiler/types/BasicType.h>
#include <compiler/types/PointerType.h>
#include <compiler/types/ReferenceType.h>
#include <compiler/types/ArrayType.h>
#include <compiler/types/FunctionType.h>

using namespace cpp20::compiler::types;

// Test para tipos básicos
TEST(TypeSystemTest, BasicTypeTest) {
    // Crear tipos básicos
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType constIntType(Type::Kind::Int, CVQualifier::Const);
    BasicType volatileDoubleType(Type::Kind::Double, CVQualifier::Volatile);

    // Verificar propiedades
    EXPECT_EQ(intType.getKind(), Type::Kind::Int);
    EXPECT_EQ(intType.getCVQualifier(), CVQualifier::None);
    EXPECT_FALSE(intType.isConst());
    EXPECT_FALSE(intType.isVolatile());

    EXPECT_EQ(constIntType.getKind(), Type::Kind::Int);
    EXPECT_EQ(constIntType.getCVQualifier(), CVQualifier::Const);
    EXPECT_TRUE(constIntType.isConst());
    EXPECT_FALSE(constIntType.isVolatile());

    EXPECT_EQ(volatileDoubleType.getKind(), Type::Kind::Double);
    EXPECT_EQ(volatileDoubleType.getCVQualifier(), CVQualifier::Volatile);
    EXPECT_FALSE(volatileDoubleType.isConst());
    EXPECT_TRUE(volatileDoubleType.isVolatile());
}

// Test para tipos puntero
TEST(TypeSystemTest, PointerTypeTest) {
    // Crear tipos básicos para apuntar
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType charType(Type::Kind::Char, CVQualifier::None);

    // Crear punteros
    PointerType intPtr(&intType, CVQualifier::None);
    PointerType constCharPtr(&charType, CVQualifier::Const);

    // Verificar propiedades
    EXPECT_EQ(intPtr.getKind(), Type::Kind::Pointer);
    EXPECT_EQ(intPtr.getPointeeType(), &intType);
    EXPECT_EQ(intPtr.getCVQualifier(), CVQualifier::None);

    EXPECT_EQ(constCharPtr.getKind(), Type::Kind::Pointer);
    EXPECT_EQ(constCharPtr.getPointeeType(), &charType);
    EXPECT_EQ(constCharPtr.getCVQualifier(), CVQualifier::Const);
}

// Test para tipos referencia
TEST(TypeSystemTest, ReferenceTypeTest) {
    // Crear tipos básicos para referenciar
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType doubleType(Type::Kind::Double, CVQualifier::None);

    // Crear referencias
    ReferenceType intRef(&intType);
    ReferenceType doubleRef(&doubleType);

    // Verificar propiedades
    EXPECT_EQ(intRef.getKind(), Type::Kind::LValueReference);
    EXPECT_EQ(intRef.getReferencedType(), &intType);

    EXPECT_EQ(doubleRef.getKind(), Type::Kind::LValueReference);
    EXPECT_EQ(doubleRef.getReferencedType(), &doubleType);
}

// Test para tipos array
TEST(TypeSystemTest, ArrayTypeTest) {
    // Crear tipos básicos para arrays
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType charType(Type::Kind::Char, CVQualifier::None);

    // Crear arrays
    ArrayType intArray(&intType, 10);
    ArrayType charArray(&charType, 0); // Tamaño desconocido

    // Verificar propiedades
    EXPECT_EQ(intArray.getKind(), Type::Kind::Array);
    EXPECT_EQ(intArray.getElementType(), &intType);
    EXPECT_EQ(intArray.getSize(), 10);

    EXPECT_EQ(charArray.getKind(), Type::Kind::Array);
    EXPECT_EQ(charArray.getElementType(), &charType);
    EXPECT_EQ(charArray.getSize(), 0);
}

// Test para tipos función
TEST(TypeSystemTest, FunctionTypeTest) {
    // Crear tipos para parámetros y retorno
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType voidType(Type::Kind::Void, CVQualifier::None);
    BasicType doubleType(Type::Kind::Double, CVQualifier::None);

    // Crear lista de parámetros
    std::vector<const Type*> params = {&intType, &doubleType};

    // Crear función
    FunctionType funcType(&voidType, params, CVQualifier::None);

    // Verificar propiedades
    EXPECT_EQ(funcType.getKind(), Type::Kind::Function);
    EXPECT_EQ(funcType.getReturnType(), &voidType);
    EXPECT_EQ(funcType.getParameterTypes().size(), 2);
    EXPECT_EQ(funcType.getParameterTypes()[0], &intType);
    EXPECT_EQ(funcType.getParameterTypes()[1], &doubleType);
    EXPECT_EQ(funcType.getCVQualifier(), CVQualifier::None);
}

// Test para compatibilidad de tipos
TEST(TypeSystemTest, TypeCompatibilityTest) {
    BasicType intType1(Type::Kind::Int, CVQualifier::None);
    BasicType intType2(Type::Kind::Int, CVQualifier::None);
    BasicType constIntType(Type::Kind::Int, CVQualifier::Const);
    BasicType doubleType(Type::Kind::Double, CVQualifier::None);

    // Tipos idénticos deben ser compatibles
    EXPECT_TRUE(intType1.isCompatible(&intType2));
    EXPECT_TRUE(intType2.isCompatible(&intType1));

    // int y const int no son completamente compatibles (pero pueden convertirse)
    EXPECT_FALSE(intType1.isCompatible(&constIntType));

    // int y double no son compatibles
    EXPECT_FALSE(intType1.isCompatible(&doubleType));
}

// Test para conversiones de tipos
TEST(TypeSystemTest, TypeConversionTest) {
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType doubleType(Type::Kind::Double, CVQualifier::None);
    BasicType charType(Type::Kind::Char, CVQualifier::None);

    // Verificar conversiones implícitas permitidas
    EXPECT_TRUE(intType.canConvertTo(&doubleType)); // int -> double OK
    EXPECT_TRUE(charType.canConvertTo(&intType));   // char -> int OK

    // Verificar conversiones que requieren cast
    EXPECT_FALSE(doubleType.canConvertTo(&intType)); // double -> int requiere cast
    EXPECT_FALSE(intType.canConvertTo(&charType));   // int -> char requiere cast
}

// Test para tamaños y alineaciones
TEST(TypeSystemTest, TypeSizeTest) {
    BasicType charType(Type::Kind::Char, CVQualifier::None);
    BasicType shortType(Type::Kind::Short, CVQualifier::None);
    BasicType intType(Type::Kind::Int, CVQualifier::None);
    BasicType longType(Type::Kind::Long, CVQualifier::None);
    BasicType floatType(Type::Kind::Float, CVQualifier::None);
    BasicType doubleType(Type::Kind::Double, CVQualifier::None);
    BasicType voidType(Type::Kind::Void, CVQualifier::None);

    // Verificar tamaños típicos (dependen de la plataforma)
    EXPECT_GE(charType.getSize(), 1);
    EXPECT_GE(shortType.getSize(), 2);
    EXPECT_GE(intType.getSize(), 2);
    EXPECT_GE(longType.getSize(), 4);
    EXPECT_GE(floatType.getSize(), 4);
    EXPECT_GE(doubleType.getSize(), 8);

    // void no tiene tamaño definido
    EXPECT_EQ(voidType.getSize(), 0);

    // Verificar alineaciones
    EXPECT_GE(charType.getAlignment(), 1);
    EXPECT_GE(shortType.getAlignment(), 1);
    EXPECT_GE(intType.getAlignment(), 1);
    EXPECT_GE(longType.getAlignment(), 1);
    EXPECT_GE(floatType.getAlignment(), 1);
    EXPECT_GE(doubleType.getAlignment(), 1);

    // Alineación nunca mayor que tamaño (excepto para void)
    EXPECT_LE(charType.getAlignment(), charType.getSize());
    EXPECT_LE(shortType.getAlignment(), shortType.getSize());
    EXPECT_LE(intType.getAlignment(), intType.getSize());
    EXPECT_LE(longType.getAlignment(), longType.getSize());
    EXPECT_LE(floatType.getAlignment(), floatType.getSize());
    EXPECT_LE(doubleType.getAlignment(), doubleType.getSize());
}
