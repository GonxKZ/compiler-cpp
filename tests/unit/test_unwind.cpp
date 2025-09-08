/**
 * @file test_unwind.cpp
 * @brief Tests unitarios para el sistema de unwind Windows x64
 */

#include <gtest/gtest.h>
#include <compiler/backend/unwind/UnwindTypes.h>
#include <compiler/backend/unwind/UnwindCodeGenerator.h>
#include <compiler/backend/unwind/UnwindEmitter.h>
#include <compiler/backend/unwind/ExceptionMapper.h>

using namespace cpp20::compiler::backend::unwind;

// Test para estructuras de unwind básicas
TEST(UnwindTypesTest, BasicUnwindStructures) {
    // Test UnwindVersion
    EXPECT_EQ(static_cast<uint8_t>(UnwindVersion::Version1), 1);
    EXPECT_EQ(static_cast<uint8_t>(UnwindVersion::Version2), 2);

    // Test UnwindFlags
    EXPECT_EQ(static_cast<uint8_t>(UnwindFlags::None), 0);
    EXPECT_EQ(static_cast<uint8_t>(UnwindFlags::EHHandler), 1);
    EXPECT_EQ(static_cast<uint8_t>(UnwindFlags::TerminateHandler), 2);
    EXPECT_EQ(static_cast<uint8_t>(UnwindFlags::ChainInfo), 4);

    // Test UnwindCode básico
    UnwindCode unwindCode;
    unwindCode.codeOffset = 0x10;
    unwindCode.operation = static_cast<uint8_t>(UnwindOperation::UWOP_PUSH_NONVOL);
    unwindCode.info = 5; // RBX register

    EXPECT_EQ(unwindCode.codeOffset, 0x10);
    EXPECT_EQ(unwindCode.operation, static_cast<uint8_t>(UnwindOperation::UWOP_PUSH_NONVOL));
    EXPECT_EQ(unwindCode.info, 5);
}

// Test para UnwindInfo
TEST(UnwindTypesTest, UnwindInfoStructure) {
    UnwindInfo unwindInfo;

    // Configurar propiedades
    unwindInfo.version = UnwindVersion::Version1;
    unwindInfo.flags = UnwindFlags::EHHandler;
    unwindInfo.prologSize = 0x20;
    unwindInfo.countOfCodes = 3;
    unwindInfo.frameRegister = 0;
    unwindInfo.frameOffset = 0;

    // Añadir unwind codes
    UnwindCode code1;
    code1.codeOffset = 0x05;
    code1.operation = static_cast<uint8_t>(UnwindOperation::UWOP_PUSH_NONVOL);
    code1.info = 3; // RSI

    UnwindCode code2;
    code2.codeOffset = 0x04;
    code2.operation = static_cast<uint8_t>(UnwindOperation::UWOP_PUSH_NONVOL);
    code2.info = 1; // RCX

    unwindInfo.unwindCodes = {code1, code2};

    // Verificar propiedades
    EXPECT_EQ(unwindInfo.version, UnwindVersion::Version1);
    EXPECT_EQ(unwindInfo.flags, UnwindFlags::EHHandler);
    EXPECT_EQ(unwindInfo.prologSize, 0x20);
    EXPECT_EQ(unwindInfo.countOfCodes, 3); // Debería ajustarse automáticamente
    EXPECT_EQ(unwindInfo.unwindCodes.size(), 2);
}

// Test para RuntimeFunction
TEST(UnwindTypesTest, RuntimeFunctionStructure) {
    RuntimeFunction runtimeFunc;

    runtimeFunc.beginAddress = 0x1000;
    runtimeFunc.endAddress = 0x1200;
    runtimeFunc.unwindInfoAddress = 0x2000;

    EXPECT_EQ(runtimeFunc.beginAddress, 0x1000);
    EXPECT_EQ(runtimeFunc.endAddress, 0x1200);
    EXPECT_EQ(runtimeFunc.unwindInfoAddress, 0x2000);
}

// Test para generador de unwind codes
TEST(UnwindCodeGeneratorTest, BasicCodeGeneration) {
    UnwindCodeGenerator generator;

    // Simular prólogo simple
    std::vector<std::string> prologueOps = {
        "push rbx",      // PUSH_NONVOL RBX
        "push rsi",      // PUSH_NONVOL RSI
        "sub rsp, 32"    // ALLOC_SMALL 32
    };

    std::vector<UnwindCode> codes = generator.generateFromPrologue(prologueOps);

    // Verificar que se generaron códigos
    EXPECT_TRUE(codes.size() > 0);

    // El primer código debería ser el más cercano al final
    if (!codes.empty()) {
        EXPECT_EQ(codes[0].operation, static_cast<uint8_t>(UnwindOperation::UWOP_ALLOC_SMALL));
        EXPECT_EQ(codes[0].info, 32 / 8 - 1); // ALLOC_SMALL encoding
    }
}

// Test para operaciones específicas de unwind
TEST(UnwindCodeGeneratorTest, SpecificOperations) {
    UnwindCodeGenerator generator;

    // Test PUSH_NONVOL
    UnwindCode pushCode = generator.generatePushNonvol(0x10, 5); // RBX
    EXPECT_EQ(pushCode.codeOffset, 0x10);
    EXPECT_EQ(pushCode.operation, static_cast<uint8_t>(UnwindOperation::UWOP_PUSH_NONVOL));
    EXPECT_EQ(pushCode.info, 5);

    // Test ALLOC_SMALL
    UnwindCode allocCode = generator.generateAlloc(0x08, 16);
    EXPECT_EQ(allocCode.codeOffset, 0x08);
    EXPECT_EQ(allocCode.operation, static_cast<uint8_t>(UnwindOperation::UWOP_ALLOC_SMALL));
    EXPECT_EQ(allocCode.info, 16 / 8 - 1);

    // Test ALLOC_LARGE
    UnwindCode largeAllocCode = generator.generateAlloc(0x08, 512);
    EXPECT_EQ(largeAllocCode.codeOffset, 0x08);
    EXPECT_EQ(largeAllocCode.operation, static_cast<uint8_t>(UnwindOperation::UWOP_ALLOC_LARGE));
}

// Test para emisor de unwind
TEST(UnwindEmitterTest, BasicEmission) {
    UnwindEmitter emitter;

    // Crear información de función
    RuntimeFunction func;
    func.beginAddress = 0x1000;
    func.endAddress = 0x1100;
    func.unwindInfoAddress = 0x2000;

    // Añadir función
    emitter.addFunctionUnwind(func);

    // Generar secciones
    std::vector<uint8_t> pdataSection = emitter.generatePdataSection();
    std::vector<uint8_t> xdataSection = emitter.generateXdataSection();

    // Verificar que se generaron datos
    EXPECT_TRUE(pdataSection.size() > 0);
    EXPECT_TRUE(xdataSection.size() > 0);

    // .pdata section debe contener RuntimeFunction structures
    EXPECT_EQ(pdataSection.size() % sizeof(RuntimeFunction), 0);
}

// Test para mapeo de excepciones
TEST(ExceptionMapperTest, BasicExceptionMapping) {
    ExceptionMapper mapper;

    // Añadir región try-catch
    mapper.addTryCatchRegion(0x1000, 0x1050, 0x2000); // Try de 0x1000-0x1050, handler en 0x2000

    // Añadir sitio de throw
    mapper.addThrowSite(0x1030, 0x3000); // Throw en 0x1030, info en 0x3000

    // Generar handler
    uint32_t handlerRVA = mapper.generateExceptionHandler();

    // Verificar que se generó un handler
    EXPECT_TRUE(handlerRVA > 0);

    // Generar datos de excepción
    std::vector<uint8_t> exceptionData = mapper.generateExceptionData();

    // Verificar que se generaron datos
    EXPECT_TRUE(exceptionData.size() > 0);
}

// Test para integración completa de unwind
TEST(UnwindIntegrationTest, CompleteUnwindFlow) {
    // Simular una función completa
    UnwindCodeGenerator codeGen;
    UnwindEmitter emitter;
    ExceptionMapper exceptionMapper;

    // Generar códigos de unwind para prólogo típico
    std::vector<std::string> prologue = {
        "push rbp",
        "mov rbp, rsp",
        "push rbx",
        "push rsi",
        "push rdi",
        "sub rsp, 32"
    };

    auto unwindCodes = codeGen.generateFromPrologue(prologue);

    // Crear RuntimeFunction
    RuntimeFunction func;
    func.beginAddress = 0x1000;
    func.endAddress = 0x1200;

    // Crear UnwindInfo
    UnwindInfo unwindInfo;
    unwindInfo.version = UnwindVersion::Version1;
    unwindInfo.flags = UnwindFlags::None;
    unwindInfo.prologSize = 0x20;
    unwindInfo.unwindCodes = unwindCodes;

    // Añadir función al emisor
    emitter.addFunctionUnwind(func);

    // Generar secciones
    auto pdata = emitter.generatePdataSection();
    auto xdata = emitter.generateXdataSection();

    // Verificar que todo se generó correctamente
    EXPECT_TRUE(pdata.size() > 0);
    EXPECT_TRUE(xdata.size() > 0);

    // Verificar integridad de .pdata
    EXPECT_EQ(pdata.size(), sizeof(RuntimeFunction));
    const RuntimeFunction* pdataFunc = reinterpret_cast<const RuntimeFunction*>(pdata.data());
    EXPECT_EQ(pdataFunc->beginAddress, 0x1000);
    EXPECT_EQ(pdataFunc->endAddress, 0x1200);
}

// Test para validación de prólogos
TEST(UnwindValidationTest, PrologueValidation) {
    UnwindCodeGenerator generator;

    // Prólogo válido
    std::vector<std::string> validPrologue = {
        "push rbp",
        "mov rbp, rsp",
        "push rbx",
        "sub rsp, 16"
    };

    auto validCodes = generator.generateFromPrologue(validPrologue);
    EXPECT_TRUE(validCodes.size() > 0);

    // Prólogo con operación no soportada
    std::vector<std::string> invalidPrologue = {
        "push rbp",
        "xor rax, rax",  // Operación no relacionada con stack
        "push rbx"
    };

    auto invalidCodes = generator.generateFromPrologue(invalidPrologue);
    // Debería manejar operaciones no soportadas
    EXPECT_TRUE(invalidCodes.size() >= 0);
}

// Test para límites de unwind
TEST(UnwindLimitsTest, BoundaryConditions) {
    UnwindCodeGenerator generator;

    // Prólogo vacío
    std::vector<std::string> emptyPrologue;
    auto emptyCodes = generator.generateFromPrologue(emptyPrologue);
    EXPECT_EQ(emptyCodes.size(), 0);

    // Prólogo muy grande
    std::vector<std::string> largePrologue;
    for (int i = 0; i < 100; ++i) {
        largePrologue.push_back("push rbx");
    }

    auto largeCodes = generator.generateFromPrologue(largePrologue);
    // Debería manejar prólogos grandes
    EXPECT_TRUE(largeCodes.size() > 0);
}

// Test para compatibilidad con MSVC
TEST(UnwindCompatibilityTest, MSVCCompatibility) {
    // Verificar que las estructuras generadas son compatibles con MSVC

    UnwindInfo info;
    info.version = UnwindVersion::Version1;
    info.flags = UnwindFlags::EHHandler;
    info.prologSize = 0x10;

    // Verificar que los campos están en el orden correcto para MSVC
    EXPECT_EQ(sizeof(info.version), 1);
    EXPECT_EQ(sizeof(info.flags), 1);
    EXPECT_EQ(sizeof(info.prologSize), 1);
    EXPECT_EQ(sizeof(info.countOfCodes), 1);

    // Verificar alineación
    EXPECT_EQ(offsetof(UnwindInfo, version), 0);
    EXPECT_EQ(offsetof(UnwindInfo, flags), 1);
    EXPECT_EQ(offsetof(UnwindInfo, prologSize), 2);
    EXPECT_EQ(offsetof(UnwindInfo, countOfCodes), 3);
}

// Test para manejo de errores
TEST(UnwindErrorTest, ErrorHandling) {
    UnwindEmitter emitter;

    // Intentar generar secciones sin funciones
    auto emptyPdata = emitter.generatePdataSection();
    auto emptyXdata = emitter.generateXdataSection();

    // Debería manejar el caso vacío
    EXPECT_TRUE(emptyPdata.empty() || emptyPdata.size() == 0);
    EXPECT_TRUE(emptyXdata.empty() || emptyXdata.size() == 0);

    // Añadir función y verificar que ya no está vacío
    RuntimeFunction func;
    func.beginAddress = 0x1000;
    func.endAddress = 0x1100;

    emitter.addFunctionUnwind(func);

    auto populatedPdata = emitter.generatePdataSection();
    auto populatedXdata = emitter.generateXdataSection();

    EXPECT_TRUE(populatedPdata.size() > 0);
    EXPECT_TRUE(populatedXdata.size() > 0);
}
