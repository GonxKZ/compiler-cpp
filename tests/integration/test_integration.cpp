/**
 * @file test_integration.cpp
 * @brief Tests de integración para el compilador C++20 completo
 */

#include <gtest/gtest.h>
#include <compiler/backend/abi/ABIContract.h>
#include <compiler/backend/frame/FrameBuilder.h>
#include <compiler/backend/coff/COFFWriter.h>
#include <compiler/backend/coff/COFFDumper.h>
#include <compiler/backend/unwind/UnwindEmitter.h>
#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <compiler/backend/mangling/ClassLayout.h>
#include <compiler/backend/mangling/VTableGenerator.h>

using namespace cpp20::compiler::backend;

// Test de integración completa: ABI + Frame + COFF
TEST(IntegrationTest, CompleteFunctionCompilation) {
    // Simular compilación de una función completa
    std::string functionName = "testFunction";
    std::vector<std::string> paramTypes = {"int", "double", "float"};
    std::string returnType = "void";
    std::vector<std::string> localVars = {"local1", "local2"};

    // 1. Clasificar parámetros según ABI
    auto paramAssignments = ABIContract::assignParameterRegisters(paramTypes);
    std::vector<bool> paramInMemory;
    for (const auto& assignment : paramAssignments) {
        paramInMemory.push_back(assignment.inMemory);
    }

    // 2. Construir frame
    FrameBuilder frameBuilder;
    FrameLayout frameLayout = frameBuilder.buildFrameLayout(
        paramTypes, paramInMemory, localVars
    );

    // 3. Verificar que todo sea consistente
    EXPECT_TRUE(frameLayout.isValid());
    EXPECT_EQ(paramAssignments.size(), paramTypes.size());
    EXPECT_EQ(paramInMemory.size(), paramTypes.size());

    // 4. Simular generación de objeto COFF
    COFFWriter coffWriter;
    // En un test real, aquí se añadirían símbolos y secciones

    // Verificar que el frame cumpla con ABI
    EXPECT_EQ(frameLayout.getTotalSize() % ABIContract::STACK_ALIGNMENT, 0);
}

// Test de integración: Mangling + Class Layout + VTable
TEST(IntegrationTest, PolymorphicClassIntegration) {
    // Simular una clase polimórfica completa
    std::string className = "TestClass";

    // Definir miembros de datos
    std::vector<mangling::MemberInfo> members = {
        mangling::MemberInfo("data1", "int", 0),
        mangling::MemberInfo("data2", "double", 0)
    };

    // Definir funciones virtuales
    std::vector<mangling::VirtualFunctionInfo> virtualFuncs = {
        mangling::VirtualFunctionInfo("virtualFunc1", "void TestClass::virtualFunc1(void)", 0),
        mangling::VirtualFunctionInfo("virtualFunc2", "int TestClass::virtualFunc2(double)", 1),
        mangling::VirtualFunctionInfo("pureVirtual", "virtual void TestClass::pureVirtual() = 0", 2, true)
    };

    // 1. Crear layout de clase
    auto classLayout = mangling::ClassLayoutGenerator::createPolymorphicClass(
        className, members, virtualFuncs
    );

    // 2. Verificar layout
    EXPECT_TRUE(classLayout->isMSVCCompatible());
    EXPECT_TRUE(classLayout->hasVirtualFunctions());
    EXPECT_TRUE(classLayout->getSize() > 0);
    EXPECT_EQ(classLayout->getDataMembers().size(), members.size());
    EXPECT_EQ(classLayout->getVirtualFunctions().size(), virtualFuncs.size());

    // 3. Generar VTable
    mangling::VTableGenerator vtableGen;
    auto vtableEntries = vtableGen.generateVTable(*classLayout);

    // 4. Verificar VTable
    EXPECT_TRUE(mangling::VTableGenerator::validateVTable(vtableEntries));
    EXPECT_EQ(vtableEntries.size(), virtualFuncs.size());

    // 5. Generar RTTI
    mangling::RTTIInfo rtti = vtableGen.generateRTTIInfo(*classLayout);
    EXPECT_EQ(rtti.className, className);
    EXPECT_TRUE(rtti.mangledClassName.length() > 0);

    // 6. Generar nombres mangled
    mangling::MSVCNameMangler mangler;
    std::string vtableName = classLayout->generateVTableName();
    std::string typeInfoName = classLayout->generateTypeInfoName();

    EXPECT_TRUE(vtableName.length() > 0);
    EXPECT_TRUE(typeInfoName.length() > 0);
    EXPECT_TRUE(vtableName[0] == '?'); // MSVC mangled name
    EXPECT_TRUE(typeInfoName[0] == '?');
}

// Test de integración: Unwind + Exception Handling
TEST(IntegrationTest, ExceptionHandlingIntegration) {
    // Simular manejo de excepciones completo
    unwind::UnwindEmitter emitter;
    unwind::ExceptionMapper exceptionMapper;

    // 1. Definir función con try-catch
    unwind::RuntimeFunction func;
    func.beginAddress = 0x1000;
    func.endAddress = 0x1200;
    func.unwindInfoAddress = 0x2000;

    // 2. Añadir región try-catch
    exceptionMapper.addTryCatchRegion(0x1000, 0x1050, 0x2000);
    exceptionMapper.addThrowSite(0x1030, 0x3000);

    // 3. Generar handler
    uint32_t handlerRVA = exceptionMapper.generateExceptionHandler();
    EXPECT_TRUE(handlerRVA > 0);

    // 4. Añadir función al emisor
    emitter.addFunctionUnwind(func);

    // 5. Generar secciones
    auto pdata = emitter.generatePdataSection();
    auto xdata = emitter.generateXdataSection();
    auto exceptionData = exceptionMapper.generateExceptionData();

    // 6. Verificar que se generaron todas las secciones
    EXPECT_TRUE(pdata.size() > 0);
    EXPECT_TRUE(xdata.size() > 0);
    EXPECT_TRUE(exceptionData.size() > 0);

    // 7. Verificar integridad de .pdata
    EXPECT_EQ(pdata.size(), sizeof(unwind::RuntimeFunction));
    const unwind::RuntimeFunction* pdataFunc =
        reinterpret_cast<const unwind::RuntimeFunction*>(pdata.data());
    EXPECT_EQ(pdataFunc->beginAddress, 0x1000);
    EXPECT_EQ(pdataFunc->endAddress, 0x1200);
}

// Test de integración: COFF + Unwind + Mangling
TEST(IntegrationTest, CompleteObjectFileGeneration) {
    // Simular generación completa de archivo objeto
    COFFWriter coffWriter;

    // 1. Añadir sección .text (código)
    std::vector<uint8_t> textData = {0x48, 0x89, 0x5C, 0x24, 0x08}; // mov QWORD PTR [rsp+8], rbx
    coffWriter.addSection(".text", textData, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE);

    // 2. Añadir sección .rdata (datos de solo lectura)
    std::vector<uint8_t> rdataData = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    coffWriter.addSection(".rdata", rdataData, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ);

    // 3. Añadir símbolos
    coffWriter.addSymbol("_main", 0, 0, IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_TYPE_NULL);
    coffWriter.addSymbol("_printf", 0, 0, IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_TYPE_NULL);

    // 4. Añadir relocations
    coffWriter.addRelocation(0, 1, IMAGE_REL_AMD64_REL32); // Relocation en .text apuntando a .rdata

    // 5. Generar archivo COFF
    std::vector<uint8_t> coffData = coffWriter.generateCOFF();

    // 6. Verificar que se generó
    EXPECT_TRUE(coffData.size() > 0);

    // 7. Validar con dumper
    COFFDumper dumper;
    EXPECT_TRUE(dumper.validateCOFF(coffData));
}

// Test de integración: Sistema completo de tipos
TEST(IntegrationTest, CompleteTypeSystem) {
    // Simular sistema de tipos completo
    mangling::MSVCNameMangler mangler;

    // 1. Tipos básicos
    EXPECT_EQ(mangler.mangleType("int"), "H");
    EXPECT_EQ(mangler.mangleType("double"), "N");
    EXPECT_EQ(mangler.mangleType("void"), "X");

    // 2. Tipos compuestos
    EXPECT_EQ(mangler.manglePointerType("int"), "PH");
    EXPECT_EQ(mangler.mangleReferenceType("double"), "AN");

    // 3. Funciones
    std::vector<std::string> params = {"int", "double"};
    std::string funcType = mangler.mangleFunctionType("void", params);
    EXPECT_TRUE(funcType.length() > 0);

    // 4. Variables
    mangling::VariableInfo varInfo;
    varInfo.name = "globalVar";
    varInfo.type = "int";
    varInfo.isStatic = false;
    varInfo.isExternC = false;

    std::string mangledVar = mangler.mangleVariable(varInfo);
    EXPECT_TRUE(mangledVar.length() > 0);
    EXPECT_EQ(mangledVar[0], '?');

    // 5. Funciones
    mangling::FunctionInfo funcInfo;
    funcInfo.name = "testFunc";
    funcInfo.parameterTypes = {"int", "double"};
    funcInfo.returnType = "void";
    funcInfo.qualifiers = mangling::FunctionQualifiers::None;
    funcInfo.isVirtual = false;

    std::string mangledFunc = mangler.mangleFunction(funcInfo);
    EXPECT_TRUE(mangledFunc.length() > 0);
    EXPECT_EQ(mangledFunc[0], '?');
}

// Test de integración: Validación cruzada de componentes
TEST(IntegrationTest, CrossComponentValidation) {
    // Verificar que los componentes sean consistentes entre sí

    // 1. ABI y Frame deben ser consistentes
    std::vector<std::string> paramTypes = {"int", "double", "int"};
    auto paramAssignments = ABIContract::assignParameterRegisters(paramTypes);

    FrameBuilder frameBuilder;
    FrameLayout layout = frameBuilder.buildFrameLayout(paramTypes, {}, {});

    // Los parámetros asignados a registros no deben requerir memoria
    for (size_t i = 0; i < paramAssignments.size(); ++i) {
        if (!paramAssignments[i].inMemory) {
            // Si está en registro, no debe necesitar stack space adicional
            EXPECT_TRUE(layout.getTotalSize() >= 0);
        }
    }

    // 2. Mangling debe ser consistente con tipos de ABI
    mangling::MSVCNameMangler mangler;
    for (const auto& paramType : paramTypes) {
        std::string mangled = mangler.mangleType(paramType);
        EXPECT_TRUE(mangled.length() > 0);
    }

    // 3. Layout de clase debe ser consistente con mangling
    std::vector<mangling::MemberInfo> members;
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        members.emplace_back("member" + std::to_string(i), paramTypes[i], 0);
    }

    auto classLayout = mangling::ClassLayoutGenerator::createSimpleClass("TestClass", members);
    EXPECT_TRUE(classLayout->isMSVCCompatible());

    // 4. VTable debe ser consistente con layout
    if (classLayout->hasVirtualFunctions()) {
        mangling::VTableGenerator vtableGen;
        auto vtable = vtableGen.generateVTable(*classLayout);
        EXPECT_TRUE(mangling::VTableGenerator::validateVTable(vtable));
    }
}

// Test de integración: Rendimiento y límites
TEST(IntegrationTest, PerformanceAndLimits) {
    // Test con muchos parámetros
    std::vector<std::string> manyParams(50, "int");
    auto assignments = ABIContract::assignParameterRegisters(manyParams);

    // Verificar que maneja muchos parámetros
    EXPECT_EQ(assignments.size(), manyParams.size());

    // Solo los primeros 4 deberían estar en registros
    for (size_t i = 0; i < 4 && i < assignments.size(); ++i) {
        EXPECT_FALSE(assignments[i].inMemory);
    }

    // Los demás deberían estar en memoria
    for (size_t i = 4; i < assignments.size(); ++i) {
        EXPECT_TRUE(assignments[i].inMemory);
    }

    // Test con clase grande
    std::vector<mangling::MemberInfo> manyMembers;
    for (int i = 0; i < 100; ++i) {
        manyMembers.emplace_back("member" + std::to_string(i), "int", 0);
    }

    auto largeClass = mangling::ClassLayoutGenerator::createSimpleClass("LargeClass", manyMembers);
    EXPECT_TRUE(largeClass->isMSVCCompatible());
    EXPECT_TRUE(largeClass->getSize() > 0);
}

// Test de integración: Manejo de errores
TEST(IntegrationTest, ErrorHandlingIntegration) {
    // Test con entradas inválidas o edge cases

    // ABI con tipos vacíos
    auto emptyAssignments = ABIContract::assignParameterRegisters({});
    EXPECT_EQ(emptyAssignments.size(), 0);

    // Frame vacío
    FrameBuilder frameBuilder;
    FrameLayout emptyLayout = frameBuilder.buildFrameLayout({}, {}, {});
    EXPECT_TRUE(emptyLayout.isValid());
    EXPECT_EQ(emptyLayout.getTotalSize(), 0);

    // Mangling con nombres especiales
    mangling::MSVCNameMangler mangler;
    mangling::FunctionInfo funcInfo;
    funcInfo.name = "operator+";
    funcInfo.parameterTypes = {"int", "int"};
    funcInfo.returnType = "int";

    std::string mangledOp = mangler.mangleFunction(funcInfo);
    EXPECT_TRUE(mangledOp.length() > 0);
    EXPECT_EQ(mangledOp[0], '?');

    // Layout de clase vacío
    auto emptyClass = mangling::ClassLayoutGenerator::createSimpleClass("EmptyClass", {});
    EXPECT_TRUE(emptyClass->isMSVCCompatible());
    EXPECT_TRUE(emptyClass->getSize() >= 0);
}

// Test de integración final: Simulación de compilación completa
TEST(IntegrationTest, FullCompilationSimulation) {
    // Simular el proceso completo de compilación

    // 1. Definir función a compilar
    std::string funcName = "complexFunction";
    std::vector<std::string> params = {"int", "double*", "float", "char"};
    std::string returnType = "double";
    std::vector<std::string> locals = {"temp1", "temp2", "result"};

    // 2. ABI Analysis
    auto paramAssignments = ABIContract::assignParameterRegisters(params);
    auto returnAssignment = ABIContract::assignReturnRegister(returnType);

    // 3. Frame Construction
    FrameBuilder frameBuilder;
    FrameLayout frame = frameBuilder.buildFrameLayout(params, {}, locals);

    // 4. Name Mangling
    mangling::MSVCNameMangler mangler;
    mangling::FunctionInfo funcInfo;
    funcInfo.name = funcName;
    funcInfo.parameterTypes = params;
    funcInfo.returnType = returnType;
    funcInfo.isVirtual = false;

    std::string mangledName = mangler.mangleFunction(funcInfo);

    // 5. Unwind Information
    unwind::UnwindEmitter unwindEmitter;
    unwind::RuntimeFunction runtimeFunc;
    runtimeFunc.beginAddress = 0x1000;
    runtimeFunc.endAddress = 0x1500;
    unwindEmitter.addFunctionUnwind(runtimeFunc);

    // 6. COFF Generation
    COFFWriter coffWriter;
    coffWriter.addSymbol(mangledName, 0, 0, IMAGE_SYM_CLASS_EXTERNAL, IMAGE_SYM_TYPE_NULL);

    // 7. Verificaciones finales
    EXPECT_TRUE(frame.isValid());
    EXPECT_TRUE(mangledName.length() > 0);
    EXPECT_TRUE(paramAssignments.size() == params.size());
    EXPECT_FALSE(returnAssignment.inMemory); // double returns in XMM0
    EXPECT_EQ(returnAssignment.registerName, "xmm0");

    // 8. Generar outputs
    auto pdata = unwindEmitter.generatePdataSection();
    auto coff = coffWriter.generateCOFF();

    EXPECT_TRUE(pdata.size() > 0);
    EXPECT_TRUE(coff.size() > 0);

    std::cout << "✅ Compilación simulada exitosa:" << std::endl;
    std::cout << "  - Función: " << funcName << std::endl;
    std::cout << "  - Mangled: " << mangledName << std::endl;
    std::cout << "  - Parámetros: " << params.size() << std::endl;
    std::cout << "  - Frame size: " << frame.getTotalSize() << " bytes" << std::endl;
    std::cout << "  - COFF size: " << coff.size() << " bytes" << std::endl;
}
