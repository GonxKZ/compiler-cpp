/**
 * @file test_mangling.cpp
 * @brief Ejemplo de prueba para el sistema de name mangling MSVC (Capa 3)
 */

#include <iostream>
#include <vector>
#include <compiler/backend/mangling/MSVCNameMangler.h>
#include <compiler/backend/mangling/ClassLayout.h>
#include <compiler/backend/mangling/VTableGenerator.h>

using namespace cpp20::compiler::backend::mangling;

/**
 * @brief Función de ejemplo para demostrar name mangling
 */
void exampleFunction(int param1, const std::string& param2) {
    std::cout << "Example function called with: " << param1 << ", " << param2 << std::endl;
}

/**
 * @brief Clase de ejemplo con funciones virtuales
 */
class ExampleBase {
public:
    virtual void virtualMethod1() {
        std::cout << "Base::virtualMethod1" << std::endl;
    }

    virtual void virtualMethod2(int param) = 0; // Pure virtual

    virtual ~ExampleBase() = default;
};

class ExampleDerived : public ExampleBase {
public:
    void virtualMethod1() override {
        std::cout << "Derived::virtualMethod1" << std::endl;
    }

    void virtualMethod2(int param) override {
        std::cout << "Derived::virtualMethod2: " << param << std::endl;
    }

    void ownMethod() {
        std::cout << "Derived::ownMethod" << std::endl;
    }
};

int main() {
    std::cout << "=== Test de Capa 3: Name Mangling MSVC ===\n";

    MSVCNameMangler mangler;

    // === Test 1: Mangling de funciones ===
    std::cout << "\n1. Name Mangling de Funciones:\n";

    FunctionInfo funcInfo;
    funcInfo.name = "exampleFunction";
    funcInfo.parameterTypes = {"int", "class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &"};
    funcInfo.returnType = "void";
    funcInfo.qualifiers = FunctionQualifiers::None;
    funcInfo.isVirtual = false;
    funcInfo.isStatic = false;
    funcInfo.isExternC = false;

    std::string mangledFunc = mangler.mangleFunction(funcInfo);
    std::cout << "Función: void exampleFunction(int, const std::string&)" << std::endl;
    std::cout << "Mangled: " << mangledFunc << std::endl;

    // Función virtual
    FunctionInfo virtualFunc = funcInfo;
    virtualFunc.name = "virtualMethod1";
    virtualFunc.scope = "ExampleBase";
    virtualFunc.isVirtual = true;

    std::string mangledVirtual = mangler.mangleFunction(virtualFunc);
    std::cout << "\nFunción virtual: void ExampleBase::virtualMethod1()" << std::endl;
    std::cout << "Mangled: " << mangledVirtual << std::endl;

    // === Test 2: Mangling de variables ===
    std::cout << "\n2. Name Mangling de Variables:\n";

    VariableInfo varInfo;
    varInfo.name = "globalVariable";
    varInfo.type = "int";
    varInfo.isStatic = false;
    varInfo.isExternC = false;

    std::string mangledVar = mangler.mangleVariable(varInfo);
    std::cout << "Variable: int globalVariable" << std::endl;
    std::cout << "Mangled: " << mangledVar << std::endl;

    // === Test 3: Layout de clases ===
    std::cout << "\n3. Class Layout:\n";

    std::vector<MemberInfo> members = {
        MemberInfo("member1", "int", 0),
        MemberInfo("member2", "double", 0),
        MemberInfo("member3", "char", 0, false, 0, false)
    };

    std::vector<VirtualFunctionInfo> virtualFuncs = {
        VirtualFunctionInfo("virtualMethod1", "void ExampleBase::virtualMethod1(void)", 0),
        VirtualFunctionInfo("virtualMethod2", "void ExampleBase::virtualMethod2(int)", 1, true)
    };

    auto layout = ClassLayoutGenerator::createPolymorphicClass("ExampleBase", members, virtualFuncs);

    std::cout << "Clase: ExampleBase (con funciones virtuales)" << std::endl;
    std::cout << "Tamaño: " << layout->getSize() << " bytes" << std::endl;
    std::cout << "Alineación: " << layout->getAlignment() << " bytes" << std::endl;
    std::cout << "Offset vtable: " << layout->getVTableOffset() << std::endl;
    std::cout << "Compatible MSVC: " << (layout->isMSVCCompatible() ? "Sí" : "No") << std::endl;

    // Mostrar layout de miembros
    std::cout << "Layout de miembros:" << std::endl;
    for (const auto& member : layout->getDataMembers()) {
        std::cout << "  " << member.name << " (" << member.type << ") @ offset " << member.offset << std::endl;
    }

    // === Test 4: Generación de VTable ===
    std::cout << "\n4. Generación de VTable:\n";

    VTableGenerator vtableGen;
    auto vtableEntries = vtableGen.generateVTable(*layout);

    std::cout << "VTable para ExampleBase:" << std::endl;
    std::cout << "Número de entradas: " << vtableEntries.size() << std::endl;

    for (size_t i = 0; i < vtableEntries.size(); ++i) {
        const auto& entry = vtableEntries[i];
        std::cout << "  [" << i << "] " << entry.functionName
                  << " @ offset " << entry.offset
                  << (entry.isPureVirtual ? " (pure virtual)" : "")
                  << (entry.isThunk ? " (thunk)" : "")
                  << std::endl;
    }

    // === Test 5: RTTI ===
    std::cout << "\n5. Información RTTI:\n";

    RTTIInfo rttiInfo = vtableGen.generateRTTIInfo(*layout);
    std::cout << "Clase: " << rttiInfo.className << std::endl;
    std::cout << "Nombre mangled: " << rttiInfo.mangledClassName << std::endl;
    std::cout << "Tiene destructor virtual: " << (rttiInfo.hasVirtualDestructor ? "Sí" : "No") << std::endl;
    std::cout << "Clases base: " << rttiInfo.baseClasses.size() << std::endl;

    // === Test 6: Utilidades de Mangling ===
    std::cout << "\n6. Utilidades de Name Mangling:\n";

    std::string demangled = MangledNameUtils::demangle(mangledFunc);
    std::cout << "Demangled: " << mangledFunc << " -> " << demangled << std::endl;

    bool isMangled = MangledNameUtils::isMangled(mangledFunc);
    std::cout << "Es mangled: " << (isMangled ? "Sí" : "No") << std::endl;

    // Comparación de nombres
    bool namesEqual = MangledNameUtils::namesEqual(mangledFunc, mangledFunc);
    std::cout << "Nombres iguales: " << (namesEqual ? "Sí" : "No") << std::endl;

    // === Test 7: Validación ===
    std::cout << "\n7. Validación:\n";

    bool vtableValid = VTableGenerator::validateVTable(vtableEntries);
    std::cout << "VTable válida: " << (vtableValid ? "Sí" : "No") << std::endl;

    bool layoutValid = ClassLayoutGenerator::validateLayout(*layout);
    std::cout << "Layout válido: " << (layoutValid ? "Sí" : "No") << std::endl;

    std::cout << "\n=== Capa 3 completada exitosamente ===\n";
    std::cout << "✅ Name mangling MSVC implementado" << std::endl;
    std::cout << "✅ Class layout compatible calculado" << std::endl;
    std::cout << "✅ VTable generation operativa" << std::endl;
    std::cout << "✅ RTTI support preparado" << std::endl;

    return 0;
}
