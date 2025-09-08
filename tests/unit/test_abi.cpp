/**
 * @file test_abi.cpp
 * @brief Tests unitarios para el sistema ABI x64 de Microsoft
 */

#include <gtest/gtest.h>
#include <compiler/backend/abi/ABIContract.h>
#include <compiler/backend/frame/FrameBuilder.h>

using namespace cpp20::compiler::backend;

// Test para contrato ABI básico
TEST(ABIContractTest, BasicABIProperties) {
    // Verificar constantes de ABI
    EXPECT_EQ(ABIContract::SHADOW_SPACE_SIZE, 32);
    EXPECT_EQ(ABIContract::STACK_ALIGNMENT, 16);
    EXPECT_EQ(ABIContract::RED_ZONE_SIZE, 0); // No red zone en Windows

    // Verificar registros volátiles
    EXPECT_TRUE(ABIContract::isVolatileRegister("rax"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("rcx"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("rdx"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("r8"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("r9"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("r10"));
    EXPECT_TRUE(ABIContract::isVolatileRegister("r11"));

    // Verificar registros no volátiles
    EXPECT_FALSE(ABIContract::isVolatileRegister("rbx"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("rsi"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("rdi"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("rbp"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("rsp"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("r12"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("r13"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("r14"));
    EXPECT_FALSE(ABIContract::isVolatileRegister("r15"));
}

// Test para clasificación de parámetros
TEST(ABIContractTest, ParameterClassification) {
    // Tipos enteros
    EXPECT_EQ(ABIContract::classifyParameterType("char"), ParameterClass::Integer);
    EXPECT_EQ(ABIContract::classifyParameterType("short"), ParameterClass::Integer);
    EXPECT_EQ(ABIContract::classifyParameterType("int"), ParameterClass::Integer);
    EXPECT_EQ(ABIContract::classifyParameterType("long"), ParameterClass::Integer);
    EXPECT_EQ(ABIContract::classifyParameterType("long long"), ParameterClass::Integer);

    // Tipos flotantes
    EXPECT_EQ(ABIContract::classifyParameterType("float"), ParameterClass::SSE);
    EXPECT_EQ(ABIContract::classifyParameterType("double"), ParameterClass::SSE);

    // Punteros
    EXPECT_EQ(ABIContract::classifyParameterType("void*"), ParameterClass::Integer);
    EXPECT_EQ(ABIContract::classifyParameterType("int*"), ParameterClass::Integer);

    // Tipos compuestos (simplificado)
    EXPECT_EQ(ABIContract::classifyParameterType("struct"), ParameterClass::Memory);
    EXPECT_EQ(ABIContract::classifyParameterType("union"), ParameterClass::Memory);
}

// Test para asignación de registros de parámetros
TEST(ABIContractTest, ParameterRegisterAssignment) {
    std::vector<std::string> paramTypes = {"int", "double", "int", "float", "int"};

    auto assignments = ABIContract::assignParameterRegisters(paramTypes);

    // Verificar asignaciones
    ASSERT_EQ(assignments.size(), paramTypes.size());

    // Primer parámetro entero -> RCX
    EXPECT_EQ(assignments[0].registerName, "rcx");
    EXPECT_FALSE(assignments[0].inMemory);

    // Segundo parámetro flotante -> XMM0
    EXPECT_EQ(assignments[1].registerName, "xmm0");
    EXPECT_FALSE(assignments[1].inMemory);

    // Tercer parámetro entero -> R8
    EXPECT_EQ(assignments[2].registerName, "r8");
    EXPECT_FALSE(assignments[2].inMemory);

    // Cuarto parámetro flotante -> XMM1
    EXPECT_EQ(assignments[3].registerName, "xmm1");
    EXPECT_FALSE(assignments[3].inMemory);

    // Quinto parámetro entero -> en stack (ya no hay registros disponibles)
    EXPECT_TRUE(assignments[4].inMemory);
}

// Test para construcción de frames
TEST(FrameBuilderTest, BasicFrameConstruction) {
    FrameBuilder builder;

    // Configurar función simple
    std::vector<std::string> paramTypes = {"int", "double"};
    std::vector<bool> paramInMemory = {false, false};
    std::vector<std::string> localVars = {"local1", "local2"};

    FrameLayout layout = builder.buildFrameLayout(paramTypes, paramInMemory, localVars);

    // Verificar propiedades del frame
    EXPECT_TRUE(layout.getTotalSize() > 0);
    EXPECT_EQ(layout.getTotalSize() % 16, 0); // Debe estar alineado a 16 bytes

    // Verificar offsets de locales
    EXPECT_TRUE(layout.getLocalOffsets().size() > 0);

    // Verificar que sea válido
    EXPECT_TRUE(layout.isValid());
}

// Test para preservación de registros no volátiles
TEST(FrameBuilderTest, NonVolatilePreservation) {
    FrameBuilder builder;

    std::vector<std::string> usedNonVolatiles = {"rbx", "rsi", "rdi"};

    FrameLayout layout = builder.buildFrameLayout({}, {}, {}, usedNonVolatiles);

    // Verificar que se reservó espacio para registros no volátiles
    EXPECT_TRUE(layout.getTotalSize() >= 3 * 8); // 3 registros * 8 bytes cada uno

    // Verificar que sea válido
    EXPECT_TRUE(layout.isValid());
}

// Test para shadow space
TEST(FrameBuilderTest, ShadowSpaceHandling) {
    FrameBuilder builder;

    std::vector<std::string> paramTypes = {"int", "int", "int", "int", "int"};
    std::vector<bool> paramInMemory = {false, false, false, false, true}; // Último en memoria

    FrameLayout layout = builder.buildFrameLayout(paramTypes, paramInMemory, {});

    // Verificar que se incluyó shadow space
    EXPECT_TRUE(layout.getTotalSize() >= ABIContract::SHADOW_SPACE_SIZE);

    // Verificar que sea válido
    EXPECT_TRUE(layout.isValid());
}

// Test para tipos de retorno
TEST(ABIContractTest, ReturnValueClassification) {
    // Tipos de retorno enteros
    EXPECT_EQ(ABIContract::classifyReturnType("char"), ReturnClass::Integer);
    EXPECT_EQ(ABIContract::classifyReturnType("int"), ReturnClass::Integer);
    EXPECT_EQ(ABIContract::classifyReturnType("long long"), ReturnClass::Integer);

    // Tipos de retorno flotantes
    EXPECT_EQ(ABIContract::classifyReturnType("float"), ReturnClass::SSE);
    EXPECT_EQ(ABIContract::classifyReturnType("double"), ReturnClass::SSE);

    // Tipos de retorno void
    EXPECT_EQ(ABIContract::classifyReturnType("void"), ReturnClass::Void);

    // Tipos de retorno grandes
    EXPECT_EQ(ABIContract::classifyReturnType("struct"), ReturnClass::Memory);
}

// Test para asignación de registros de retorno
TEST(ABIContractTest, ReturnRegisterAssignment) {
    // Retorno entero
    auto intReturn = ABIContract::assignReturnRegister("int");
    EXPECT_EQ(intReturn.registerName, "rax");
    EXPECT_FALSE(intReturn.inMemory);

    // Retorno flotante
    auto floatReturn = ABIContract::assignReturnRegister("float");
    EXPECT_EQ(floatReturn.registerName, "xmm0");
    EXPECT_FALSE(floatReturn.inMemory);

    // Retorno void
    auto voidReturn = ABIContract::assignReturnRegister("void");
    EXPECT_EQ(voidReturn.registerName, "");
    EXPECT_FALSE(voidReturn.inMemory);

    // Retorno grande (struct)
    auto structReturn = ABIContract::assignReturnRegister("struct");
    EXPECT_TRUE(structReturn.inMemory); // Se pasa por memoria
}

// Test para validación de frames
TEST(FrameBuilderTest, FrameValidation) {
    FrameBuilder builder;

    // Frame válido
    FrameLayout validLayout = builder.buildFrameLayout(
        {"int", "double"}, {false, false}, {"local1", "local2"}
    );
    EXPECT_TRUE(validLayout.isValid());

    // Verificar propiedades de validación
    EXPECT_TRUE(validLayout.getTotalSize() > 0);
    EXPECT_EQ(validLayout.getTotalSize() % ABIContract::STACK_ALIGNMENT, 0);
    EXPECT_TRUE(validLayout.getLocalOffsets().size() > 0);
}

// Test para límites de parámetros
TEST(ABIContractTest, ParameterLimits) {
    // Función con muchos parámetros
    std::vector<std::string> manyParams(20, "int");
    auto assignments = ABIContract::assignParameterRegisters(manyParams);

    // Verificar que los primeros 4 parámetros estén en registros
    for (size_t i = 0; i < 4 && i < assignments.size(); ++i) {
        EXPECT_FALSE(assignments[i].inMemory);
    }

    // Verificar que los parámetros restantes estén en memoria
    for (size_t i = 4; i < assignments.size(); ++i) {
        EXPECT_TRUE(assignments[i].inMemory);
    }
}

// Test para tipos compuestos
TEST(ABIContractTest, CompositeTypes) {
    // Struct pequeño (pasa por registros)
    std::string smallStruct = "struct { int x; int y; }";
    EXPECT_EQ(ABIContract::classifyParameterType(smallStruct), ParameterClass::Integer);

    // Struct grande (pasa por memoria)
    std::string largeStruct = "struct { int arr[10]; double d; }";
    EXPECT_EQ(ABIContract::classifyParameterType(largeStruct), ParameterClass::Memory);

    // Union
    std::string unionType = "union { int i; double d; }";
    EXPECT_EQ(ABIContract::classifyParameterType(unionType), ParameterClass::Memory);
}

// Test para alineación de stack
TEST(FrameBuilderTest, StackAlignment) {
    FrameBuilder builder;

    // Crear frame con diferentes tipos de variables locales
    std::vector<std::string> localVars = {"char", "int", "double", "long long"};

    FrameLayout layout = builder.buildFrameLayout({}, {}, localVars);

    // Verificar alineación general
    EXPECT_EQ(layout.getTotalSize() % ABIContract::STACK_ALIGNMENT, 0);

    // Verificar que el tamaño sea suficiente para todas las variables
    size_t expectedMinSize = 1 + 4 + 8 + 8; // Tamaños de las variables
    EXPECT_TRUE(layout.getTotalSize() >= expectedMinSize);

    // Verificar que sea válido
    EXPECT_TRUE(layout.isValid());
}

// Test para convención de llamadas
TEST(ABIContractTest, CallingConvention) {
    // Verificar que la convención es correcta para Windows x64

    // Stack debe crecer hacia abajo
    EXPECT_TRUE(ABIContract::STACK_GROWS_DOWN);

    // Shadow space debe ser 32 bytes
    EXPECT_EQ(ABIContract::SHADOW_SPACE_SIZE, 32);

    // No hay red zone en Windows
    EXPECT_EQ(ABIContract::RED_ZONE_SIZE, 0);

    // Stack alignment es 16 bytes
    EXPECT_EQ(ABIContract::STACK_ALIGNMENT, 16);
}

// Test para integración completa ABI
TEST(ABIIntegrationTest, CompleteFunctionABI) {
    FrameBuilder builder;

    // Función compleja: int func(double d, int i, float f, char c, long long ll)
    std::vector<std::string> paramTypes = {"double", "int", "float", "char", "long long"};
    std::vector<std::string> localVars = {"local_int", "local_double", "local_ptr"};
    std::vector<std::string> usedNonVolatiles = {"rbx", "rsi"};

    auto paramAssignments = ABIContract::assignParameterRegisters(paramTypes);
    FrameLayout layout = builder.buildFrameLayout(
        paramTypes, {}, localVars, usedNonVolatiles
    );

    // Verificar asignaciones de parámetros
    ASSERT_EQ(paramAssignments.size(), 5);

    // double -> XMM0
    EXPECT_EQ(paramAssignments[0].registerName, "xmm0");
    // int -> RCX
    EXPECT_EQ(paramAssignments[1].registerName, "rcx");
    // float -> XMM1
    EXPECT_EQ(paramAssignments[2].registerName, "xmm1");
    // char -> R8
    EXPECT_EQ(paramAssignments[3].registerName, "r8");
    // long long -> R9
    EXPECT_EQ(paramAssignments[4].registerName, "r9");

    // Verificar frame
    EXPECT_TRUE(layout.isValid());
    EXPECT_TRUE(layout.getTotalSize() > 0);
    EXPECT_EQ(layout.getTotalSize() % 16, 0); // Alineado

    // Verificar que hay espacio para locales y registros no volátiles
    EXPECT_TRUE(layout.getLocalOffsets().size() >= 3); // 3 variables locales
    EXPECT_TRUE(layout.getNonVolatileOffsets().size() >= 2); // 2 registros no volátiles
}

// Test para manejo de errores ABI
TEST(ABIErrorTest, ErrorHandling) {
    FrameBuilder builder;

    // Parámetros vacíos
    FrameLayout emptyLayout = builder.buildFrameLayout({}, {}, {});
    EXPECT_TRUE(emptyLayout.isValid());
    EXPECT_EQ(emptyLayout.getTotalSize(), 0); // Frame vacío

    // Solo shadow space
    FrameLayout shadowLayout = builder.buildFrameLayout(
        std::vector<std::string>(4, "int"), // 4 parámetros enteros
        std::vector<bool>(4, false),
        {}
    );
    EXPECT_TRUE(shadowLayout.isValid());
    EXPECT_TRUE(shadowLayout.getTotalSize() >= ABIContract::SHADOW_SPACE_SIZE);
}
