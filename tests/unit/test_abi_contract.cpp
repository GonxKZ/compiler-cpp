/**
 * @file test_abi_contract.cpp
 * @brief Tests para validar el contrato ABI x86_64-pc-windows-msvc
 */

#include <compiler/backend/abi/ABIContract.h>
#include <gtest/gtest.h>

using namespace cpp20::compiler::backend::abi;

class ABIContractTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configuración inicial si es necesaria
    }

    void TearDown() override {
        // Limpieza si es necesaria
    }
};

// ========================================================================
// Tests para clasificación de parámetros
// ========================================================================

TEST_F(ABIContractTest, ClassifyIntegerParameter) {
    // Test parámetros enteros básicos
    auto param = ABIContract::classifyParameter(4, 4, false, true);  // int32_t
    EXPECT_EQ(param.kind, ABIContract::ParameterInfo::Kind::Integer);
    EXPECT_EQ(param.size, 4u);
    EXPECT_EQ(param.alignment, 4u);
    EXPECT_TRUE(param.isSigned);
}

TEST_F(ABIContractTest, ClassifyPointerParameter) {
    // Test punteros (64-bit en x64)
    auto param = ABIContract::classifyParameter(8, 8, false, false);  // void*
    EXPECT_EQ(param.kind, ABIContract::ParameterInfo::Kind::Integer);
    EXPECT_EQ(param.size, 8u);
    EXPECT_EQ(param.alignment, 8u);
}

TEST_F(ABIContractTest, ClassifyFloatParameter) {
    // Test parámetros flotantes
    auto param = ABIContract::classifyParameter(8, 8, true, false);  // double
    EXPECT_EQ(param.kind, ABIContract::ParameterInfo::Kind::Float);
    EXPECT_EQ(param.size, 8u);
    EXPECT_EQ(param.alignment, 8u);
}

TEST_F(ABIContractTest, ClassifyVectorParameter) {
    // Test parámetros vectoriales
    auto param = ABIContract::classifyParameter(16, 16, true, false);  // __m128
    EXPECT_EQ(param.kind, ABIContract::ParameterInfo::Kind::Vector);
    EXPECT_EQ(param.size, 16u);
    EXPECT_EQ(param.alignment, 16u);
}

TEST_F(ABIContractTest, ClassifyLargeAggregate) {
    // Test agregados grandes (deben pasar por referencia)
    auto param = ABIContract::classifyParameter(24, 8, false, false);  // struct de 24 bytes
    EXPECT_EQ(param.kind, ABIContract::ParameterInfo::Kind::Aggregate);
    EXPECT_EQ(param.size, 24u);
}

// ========================================================================
// Tests para clasificación de retornos
// ========================================================================

TEST_F(ABIContractTest, ClassifyVoidReturn) {
    auto ret = ABIContract::classifyReturn(0, 0, false, false);
    EXPECT_EQ(ret.kind, ABIContract::ReturnInfo::Kind::Void);
    EXPECT_EQ(ret.size, 0u);
    EXPECT_FALSE(ret.isIndirect);
}

TEST_F(ABIContractTest, ClassifyIntegerReturn) {
    auto ret = ABIContract::classifyReturn(4, 4, false, false);  // int32_t
    EXPECT_EQ(ret.kind, ABIContract::ReturnInfo::Kind::Integer);
    EXPECT_EQ(ret.size, 4u);
    EXPECT_FALSE(ret.isIndirect);
}

TEST_F(ABIContractTest, ClassifyFloatReturn) {
    auto ret = ABIContract::classifyReturn(8, 8, true, false);  // double
    EXPECT_EQ(ret.kind, ABIContract::ReturnInfo::Kind::Float);
    EXPECT_EQ(ret.size, 8u);
    EXPECT_FALSE(ret.isIndirect);
}

TEST_F(ABIContractTest, ClassifyAggregateReturn) {
    // Structs pasan por referencia
    auto ret = ABIContract::classifyReturn(16, 8, false, true);  // struct de 16 bytes
    EXPECT_EQ(ret.kind, ABIContract::ReturnInfo::Kind::Aggregate);
    EXPECT_EQ(ret.size, 16u);
    EXPECT_TRUE(ret.isIndirect);
}

// ========================================================================
// Tests para registros de argumentos
// ========================================================================

TEST_F(ABIContractTest, IntegerArgRegisters) {
    EXPECT_EQ(ABIContract::getIntegerArgRegister(0), "rcx");
    EXPECT_EQ(ABIContract::getIntegerArgRegister(1), "rdx");
    EXPECT_EQ(ABIContract::getIntegerArgRegister(2), "r8");
    EXPECT_EQ(ABIContract::getIntegerArgRegister(3), "r9");
    EXPECT_EQ(ABIContract::getIntegerArgRegister(4), "");  // Stack
}

TEST_F(ABIContractTest, FloatArgRegisters) {
    EXPECT_EQ(ABIContract::getFloatArgRegister(0), "xmm0");
    EXPECT_EQ(ABIContract::getFloatArgRegister(1), "xmm1");
    EXPECT_EQ(ABIContract::getFloatArgRegister(2), "xmm2");
    EXPECT_EQ(ABIContract::getFloatArgRegister(3), "xmm3");
    EXPECT_EQ(ABIContract::getFloatArgRegister(4), "");  // Stack
}

// ========================================================================
// Tests para callee-saved registers
// ========================================================================

TEST_F(ABIContractTest, CalleeSavedRegisters) {
    // Registros que deben preservarse: RBX, RSI, RDI, RBP, R12-R15
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(3));   // RBX
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(5));   // RBP
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(6));   // RSI
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(7));   // RDI
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(12));  // R12
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(13));  // R13
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(14));  // R14
    EXPECT_TRUE(ABIContract::isCalleeSavedRegister(15));  // R15

    // Registros volátiles
    EXPECT_FALSE(ABIContract::isCalleeSavedRegister(0));  // RAX
    EXPECT_FALSE(ABIContract::isCalleeSavedRegister(1));  // RCX
    EXPECT_FALSE(ABIContract::isCalleeSavedRegister(2));  // RDX
    EXPECT_FALSE(ABIContract::isCalleeSavedRegister(8));  // R8
}

// ========================================================================
// Tests para cálculo de tamaño de stack
// ========================================================================

TEST_F(ABIContractTest, CalculateStackSize) {
    std::vector<ABIContract::ParameterInfo> params;

    // Función sin parámetros
    size_t size = ABIContract::calculateStackSize(params, 0, 0);
    EXPECT_EQ(size, 48u);  // 8 (return) + 8 (rbp) + 32 (shadow) = 48, aligned to 16 = 48

    // Función con parámetros en registros (no afectan stack)
    params.push_back(ABIContract::ParameterInfo(
        ABIContract::ParameterInfo::Kind::Integer, 4, 4, true, 0));  // En RCX

    size = ABIContract::calculateStackSize(params, 0, 0);
    EXPECT_EQ(size, 48u);  // Mismo tamaño

    // Función con parámetros en stack
    params.push_back(ABIContract::ParameterInfo(
        ABIContract::ParameterInfo::Kind::Integer, 4, 4, false, -1));  // En stack

    size = ABIContract::calculateStackSize(params, 0, 0);
    EXPECT_EQ(size, 64u);  // 48 + 16 (param aligned) = 64
}

// ========================================================================
// Tests para validación de frame layout
// ========================================================================

TEST_F(ABIContractTest, ValidateValidFrameLayout) {
    ABIContract::FrameLayout layout;
    layout.totalSize = 64;
    layout.shadowSpaceSize = 32;
    layout.returnAddressOffset = 8;
    layout.savedRbpOffset = 0;

    EXPECT_TRUE(ABIContract::validateFrameLayout(layout));
}

TEST_F(ABIContractTest, ValidateInvalidFrameSize) {
    ABIContract::FrameLayout layout;
    layout.totalSize = ABIContract::MAX_FRAME_SIZE + 1;

    EXPECT_FALSE(ABIContract::validateFrameLayout(layout));
}

TEST_F(ABIContractTest, ValidateUnalignedStack) {
    ABIContract::FrameLayout layout;
    layout.totalSize = 10;  // No alineado a 16 bytes
    layout.shadowSpaceSize = 32;

    EXPECT_FALSE(ABIContract::validateFrameLayout(layout));
}

TEST_F(ABIContractTest, ValidateInvalidShadowSpace) {
    ABIContract::FrameLayout layout;
    layout.totalSize = 64;
    layout.shadowSpaceSize = 16;  // Debe ser 32

    EXPECT_FALSE(ABIContract::validateFrameLayout(layout));
}

// ========================================================================
// Tests para alineación de stack
// ========================================================================

TEST_F(ABIContractTest, StackAlignment) {
    EXPECT_TRUE(ABIContract::isStackAligned(0));
    EXPECT_TRUE(ABIContract::isStackAligned(16));
    EXPECT_TRUE(ABIContract::isStackAligned(32));
    EXPECT_TRUE(ABIContract::isStackAligned(48));

    EXPECT_FALSE(ABIContract::isStackAligned(8));
    EXPECT_FALSE(ABIContract::isStackAligned(24));
    EXPECT_FALSE(ABIContract::isStackAligned(10));
}

// ========================================================================
// Tests para constantes del ABI
// ========================================================================

TEST_F(ABIContractTest, ABIConstants) {
    EXPECT_EQ(ABIContract::MAX_INTEGER_ARGS_IN_REGS, 4);
    EXPECT_EQ(ABIContract::MAX_FLOAT_ARGS_IN_REGS, 4);
    EXPECT_EQ(ABIContract::SHADOW_SPACE_SIZE, 32u);
    EXPECT_EQ(ABIContract::STACK_ALIGNMENT, 16u);
    EXPECT_EQ(ABIContract::GENERAL_ALIGNMENT, 8u);
}

// ========================================================================
// Tests para mensajes de error de validación
// ========================================================================

TEST_F(ABIContractTest, ValidationErrorMessages) {
    EXPECT_EQ(ABIContract::getValidationErrorString(
        ABIContract::ValidationError::OK), "OK");

    EXPECT_EQ(ABIContract::getValidationErrorString(
        ABIContract::ValidationError::INVALID_FRAME_SIZE),
        "Tamaño de frame inválido");

    EXPECT_EQ(ABIContract::getValidationErrorString(
        ABIContract::ValidationError::UNALIGNED_STACK),
        "Stack no alineado correctamente");
}
