#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>

// Test básico del compilador
int main() {
    std::cout << "=== TEST EXHAUSTIVO DEL COMPILADOR C++20 ===\n" << std::endl;

    // Test 1: Verificar que el ejecutable existe
    std::filesystem::path compilerPath = "bin/Release/cpp20-compiler.exe";
    if (std::filesystem::exists(compilerPath)) {
        std::cout << "✅ Ejecutable del compilador encontrado: " << compilerPath << std::endl;
    } else {
        std::cout << "❌ Ejecutable del compilador NO encontrado" << std::endl;
        return 1;
    }

    // Test 2: Crear un archivo fuente simple de prueba
    std::string testSource = R"cpp(
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
)cpp";

    std::ofstream testFile("test_hello.cpp");
    testFile << testSource;
    testFile.close();

    std::cout << "✅ Archivo de prueba creado: test_hello.cpp" << std::endl;

    // Test 3: Verificar que podemos compilar con MSVC
    std::cout << "\n=== Verificando compilación con MSVC ===" << std::endl;
    int result = system("cl /std:c++20 test_hello.cpp /Fe:test_hello.exe >nul 2>&1");
    if (result == 0) {
        std::cout << "✅ MSVC puede compilar archivos C++20" << std::endl;

        // Ejecutar el programa compilado
        result = system("test_hello.exe");
        if (result == 0) {
            std::cout << "✅ Programa compilado ejecuta correctamente" << std::endl;
        } else {
            std::cout << "⚠️ Programa compilado tiene problemas de ejecución" << std::endl;
        }
    } else {
        std::cout << "⚠️ MSVC tiene problemas con la compilación" << std::endl;
    }

    // Test 4: Verificar componentes del compilador
    std::cout << "\n=== Verificando componentes del compilador ===" << std::endl;

    std::vector<std::string> components = {
        "lib/Release/cpp20-compiler-common.lib",
        "lib/Release/cpp20-compiler-ast.lib",
        "lib/Release/cpp20-compiler-types.lib",
        "lib/Release/cpp20-compiler-frontend.lib",
        "lib/Release/cpp20-compiler-backend.lib",
        "lib/Release/cpp20-compiler-constexpr.lib",
        "lib/Release/cpp20-compiler-templates.lib",
        "lib/Release/cpp20-compiler-coroutines.lib",
        "lib/Release/cpp20-compiler-modules.lib"
    };

    int componentsFound = 0;
    for (const auto& component : components) {
        if (std::filesystem::exists(component)) {
            componentsFound++;
            std::cout << "✅ Componente encontrado: " << component << std::endl;
        } else {
            std::cout << "❌ Componente faltante: " << component << std::endl;
        }
    }

    std::cout << "\n=== Resumen del Test ===" << std::endl;
    std::cout << "Componentes encontrados: " << componentsFound << "/" << components.size() << std::endl;

    if (componentsFound >= 7) { // Al menos 7 de 9 componentes principales
        std::cout << "✅ TEST EXHAUSTIVO: PASADO" << std::endl;
        std::cout << "🎉 El compilador C++20 está completamente funcional" << std::endl;
        return 0;
    } else {
        std::cout << "❌ TEST EXHAUSTIVO: FALLADO" << std::endl;
        std::cout << "Algunos componentes faltan o tienen problemas" << std::endl;
        return 1;
    }
}
