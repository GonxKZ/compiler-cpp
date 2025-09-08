/**
 * @file test_mangling.cpp
 * @brief Tests unitarios para el sistema de name mangling MSVC
 */

#include <gtest/gtest.h>
#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <compiler/backend/mangling/ClassLayout.h>
#include <compiler/backend/mangling/VTableGenerator.h>

using namespace cpp20::compiler::backend::mangling;

// Test para mangling de funciones básicas
TEST(ManglingTest, BasicFunctionMangling) {
    MSVCNameMangler mangler;

    FunctionInfo funcInfo;
    funcInfo.name = "simpleFunction";
    funcInfo.parameterTypes = {"int", "double"};
    funcInfo.returnType = "void";
    funcInfo.qualifiers = FunctionQualifiers::None;
    funcInfo.isVirtual = false;
    funcInfo.isStatic = false;
    funcInfo.isExternC = false;

    std::string mangled = mangler.mangleFunction(funcInfo);

    // Verificar que el nombre mangled sigue el formato MSVC
    EXPECT_TRUE(mangled.length() > 0);
    EXPECT_EQ(mangled[0], '?'); // MSVC functions start with '?'

    // Debe contener el nombre de la función
    EXPECT_NE(mangled.find("simpleFunction"), std::string::npos);
}

// Test para mangling de funciones con calificadores
TEST(ManglingTest, QualifiedFunctionMangling) {
    MSVCNameMangler mangler;

    FunctionInfo funcInfo;
    funcInfo.name = "qualifiedFunction";
    funcInfo.parameterTypes = {"int"};
    funcInfo.returnType = "void";
    funcInfo.qualifiers = FunctionQualifiers::Const;
    funcInfo.isVirtual = false;
    funcInfo.isStatic = false;
    funcInfo.isExternC = false;

    std::string mangled = mangler.mangleFunction(funcInfo);

    EXPECT_TRUE(mangled.length() > 0);
    EXPECT_EQ(mangled[0], '?');
    EXPECT_NE(mangled.find("qualifiedFunction"), std::string::npos);
    // Const qualifier should be encoded
    EXPECT_NE(mangled.find('B'), std::string::npos);
}

// Test para mangling de funciones virtuales
TEST(ManglingTest, VirtualFunctionMangling) {
    MSVCNameMangler mangler;

    FunctionInfo funcInfo;
    funcInfo.name = "virtualFunction";
    funcInfo.scope = "MyClass";
    funcInfo.parameterTypes = {};
    funcInfo.returnType = "void";
    funcInfo.qualifiers = FunctionQualifiers::None;
    funcInfo.isVirtual = true;
    funcInfo.isStatic = false;
    funcInfo.isExternC = false;

    std::string mangled = mangler.mangleFunction(funcInfo);

    EXPECT_TRUE(mangled.length() > 0);
    EXPECT_EQ(mangled[0], '?');
    EXPECT_NE(mangled.find("virtualFunction"), std::string::npos);
    EXPECT_NE(mangled.find("MyClass"), std::string::npos);
}

// Test para mangling de variables
TEST(ManglingTest, VariableMangling) {
    MSVCNameMangler mangler;

    VariableInfo varInfo;
    varInfo.name = "globalVariable";
    varInfo.type = "int";
    varInfo.isStatic = false;
    varInfo.isExternC = false;

    std::string mangled = mangler.mangleVariable(varInfo);

    EXPECT_TRUE(mangled.length() > 0);
    EXPECT_EQ(mangled[0], '?');
    EXPECT_NE(mangled.find("globalVariable"), std::string::npos);
}

// Test para mangling de clases
TEST(ManglingTest, ClassMangling) {
    MSVCNameMangler mangler;

    ClassInfo classInfo;
    classInfo.name = "MyClass";
    classInfo.scope = "";
    classInfo.isStruct = false;
    classInfo.hasVirtualFunctions = false;
    classInfo.templateArgs = 0;

    std::string mangled = mangler.mangleClass(classInfo);

    EXPECT_TRUE(mangled.length() > 0);
    EXPECT_EQ(mangled[0], '?');
    EXPECT_NE(mangled.find("MyClass"), std::string::npos);
}

// Test para tipos básicos
TEST(ManglingTest, BasicTypeMangling) {
    MSVCNameMangler mangler;

    // Test various basic types
    EXPECT_EQ(mangler.mangleType("void"), "X");
    EXPECT_EQ(mangler.mangleType("int"), "H");
    EXPECT_EQ(mangler.mangleType("double"), "N");
    EXPECT_EQ(mangler.mangleType("char"), "D");
    EXPECT_EQ(mangler.mangleType("float"), "M");
}

// Test para tipos puntero
TEST(ManglingTest, PointerTypeMangling) {
    MSVCNameMangler mangler;

    EXPECT_EQ(mangler.manglePointerType("int"), "PH");  // Pointer to int
    EXPECT_EQ(mangler.manglePointerType("void"), "PX"); // Pointer to void
}

// Test para tipos referencia
TEST(ManglingTest, ReferenceTypeMangling) {
    MSVCNameMangler mangler;

    EXPECT_EQ(mangler.mangleReferenceType("int"), "AH");  // Reference to int
    EXPECT_EQ(mangler.mangleReferenceType("double"), "AN"); // Reference to double
}

// Test para tipos array
TEST(ManglingTest, ArrayTypeMangling) {
    MSVCNameMangler mangler;

    EXPECT_EQ(mangler.mangleArrayType("int", 10), "Y0AH");  // Array of 10 ints
    EXPECT_EQ(mangler.mangleArrayType("char", 0), "QAD");   // Array of unknown size chars
}

// Test para tipos función
TEST(ManglingTest, FunctionTypeMangling) {
    MSVCNameMangler mangler;

    std::vector<std::string> params = {"int", "double"};
    std::string funcType = mangler.mangleFunctionType("void", params);

    EXPECT_TRUE(funcType.length() > 0);
    EXPECT_NE(funcType.find("X"), std::string::npos); // void return
    EXPECT_NE(funcType.find("H"), std::string::npos); // int parameter
    EXPECT_NE(funcType.find("N"), std::string::npos); // double parameter
}

// Test para layout de clases simples
TEST(ClassLayoutTest, SimpleClassLayout) {
    std::vector<MemberInfo> members = {
        MemberInfo("member1", "int", 0),
        MemberInfo("member2", "double", 0),
        MemberInfo("member3", "char", 0)
    };

    auto layout = ClassLayoutGenerator::createSimpleClass("SimpleClass", members);

    // Verificar propiedades básicas
    EXPECT_TRUE(layout->getSize() > 0);
    EXPECT_TRUE(layout->getAlignment() > 0);
    EXPECT_FALSE(layout->hasVirtualFunctions());

    // Verificar que el layout sea válido
    EXPECT_TRUE(layout->isMSVCCompatible());
}

// Test para layout de clases con funciones virtuales
TEST(ClassLayoutTest, PolymorphicClassLayout) {
    std::vector<MemberInfo> members = {
        MemberInfo("data", "int", 0)
    };

    std::vector<VirtualFunctionInfo> virtualFuncs = {
        VirtualFunctionInfo("virtualFunc", "void SimpleClass::virtualFunc(void)", 0)
    };

    auto layout = ClassLayoutGenerator::createPolymorphicClass("PolymorphicClass", members, virtualFuncs);

    // Verificar propiedades
    EXPECT_TRUE(layout->getSize() > 0);
    EXPECT_TRUE(layout->getAlignment() > 0);
    EXPECT_TRUE(layout->hasVirtualFunctions());

    // Debe tener vtable
    EXPECT_TRUE(layout->getVTableOffset() >= 0);

    // Verificar que el layout sea válido
    EXPECT_TRUE(layout->isMSVCCompatible());
}

// Test para layout de clases con herencia
TEST(ClassLayoutTest, InheritedClassLayout) {
    std::vector<InheritanceInfo> bases = {
        InheritanceInfo("BaseClass", 0, false, true)
    };

    std::vector<MemberInfo> members = {
        MemberInfo("derivedData", "int", 0)
    };

    auto layout = ClassLayoutGenerator::createInheritedClass("DerivedClass", bases, members);

    // Verificar propiedades
    EXPECT_TRUE(layout->getSize() > 0);
    EXPECT_TRUE(layout->getAlignment() > 0);

    // Debe tener información de herencia
    EXPECT_EQ(layout->getInheritance().size(), 1);

    // Verificar que el layout sea válido
    EXPECT_TRUE(layout->isMSVCCompatible());
}

// Test para generador de VTable
TEST(VTableGeneratorTest, VTableGeneration) {
    // Crear layout de clase con funciones virtuales
    std::vector<MemberInfo> members = {
        MemberInfo("data", "int", 0)
    };

    std::vector<VirtualFunctionInfo> virtualFuncs = {
        VirtualFunctionInfo("func1", "void TestClass::func1(void)", 0),
        VirtualFunctionInfo("func2", "void TestClass::func2(int)", 1),
        VirtualFunctionInfo("func3", "int TestClass::func3(void)", 2, true) // Pure virtual
    };

    auto layout = ClassLayoutGenerator::createPolymorphicClass("TestClass", members, virtualFuncs);
    VTableGenerator vtableGen;

    // Generar VTable
    auto vtableEntries = vtableGen.generateVTable(*layout);

    // Verificar que se generaron entradas
    EXPECT_EQ(vtableEntries.size(), virtualFuncs.size());

    // Verificar offsets consecutivos
    for (size_t i = 0; i < vtableEntries.size(); ++i) {
        EXPECT_EQ(vtableEntries[i].offset, i * sizeof(void*));
    }

    // Verificar que la última función es pura virtual
    EXPECT_TRUE(vtableEntries.back().isPureVirtual);

    // Verificar validación de VTable
    EXPECT_TRUE(VTableGenerator::validateVTable(vtableEntries));
}

// Test para generador de RTTI
TEST(VTableGeneratorTest, RTTIGeneration) {
    // Crear layout de clase con funciones virtuales
    std::vector<MemberInfo> members = {
        MemberInfo("data", "int", 0)
    };

    std::vector<VirtualFunctionInfo> virtualFuncs = {
        VirtualFunctionInfo("virtualFunc", "void TestClass::virtualFunc(void)", 0)
    };

    auto layout = ClassLayoutGenerator::createPolymorphicClass("TestClass", members, virtualFuncs);
    VTableGenerator vtableGen;

    // Generar RTTI
    RTTIInfo rtti = vtableGen.generateRTTIInfo(*layout);

    // Verificar propiedades RTTI
    EXPECT_EQ(rtti.className, "TestClass");
    EXPECT_TRUE(rtti.mangledClassName.length() > 0);
    EXPECT_EQ(rtti.baseClasses.size(), 0); // No inheritance in this test
    EXPECT_FALSE(rtti.hasVirtualDestructor);
}

// Test para compatibilidad de layouts
TEST(ClassLayoutTest, LayoutCompatibility) {
    // Crear dos layouts idénticos
    std::vector<MemberInfo> members1 = {
        MemberInfo("member1", "int", 0),
        MemberInfo("member2", "double", 0)
    };

    std::vector<MemberInfo> members2 = {
        MemberInfo("member1", "int", 0),
        MemberInfo("member2", "double", 0)
    };

    auto layout1 = ClassLayoutGenerator::createSimpleClass("TestClass1", members1);
    auto layout2 = ClassLayoutGenerator::createSimpleClass("TestClass2", members2);

    // Verificar compatibilidad
    EXPECT_TRUE(ClassLayoutGenerator::layoutsCompatible(*layout1, *layout2));
}

// Test para utilidades de mangling
TEST(ManglingUtilsTest, NameValidation) {
    // Test mangled names
    EXPECT_TRUE(MangledNameUtils::isMangled("?testFunction@@YAXXZ"));
    EXPECT_FALSE(MangledNameUtils::isMangled("normalFunction"));
    EXPECT_FALSE(MangledNameUtils::isMangled(""));

    // Test name comparison
    std::string name1 = "?testFunction@@YAXXZ";
    std::string name2 = "?testFunction@@YAXXZ";
    std::string name3 = "?otherFunction@@YAXXZ";

    EXPECT_TRUE(MangledNameUtils::namesEqual(name1, name2));
    EXPECT_FALSE(MangledNameUtils::namesEqual(name1, name3));

    // Test demangling (simplified)
    std::string demangled = MangledNameUtils::demangle(name1);
    EXPECT_TRUE(demangled.length() > 0);
}

// Test para herencia múltiple en VTable
TEST(VTableGeneratorTest, MultipleInheritance) {
    // Crear layout con herencia múltiple
    std::vector<InheritanceInfo> bases = {
        InheritanceInfo("Base1", 0, false, true),
        InheritanceInfo("Base2", 8, false, false)
    };

    std::vector<MemberInfo> members = {
        MemberInfo("derivedData", "int", 16)
    };

    auto layout = ClassLayoutGenerator::createInheritedClass("MultipleDerived", bases, members);
    VTableGenerator vtableGen;

    // Verificar que maneja herencia múltiple correctamente
    EXPECT_EQ(layout->getInheritance().size(), 2);

    // El layout debería ser válido
    EXPECT_TRUE(layout->isMSVCCompatible());
}
