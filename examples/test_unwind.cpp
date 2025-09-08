/**
 * @file test_unwind.cpp
 * @brief Ejemplo de prueba para el sistema de unwind de Capa 2
 */

#include <iostream>
#include <vector>
#include <compiler/backend/unwind/UnwindEmitter.h>
#include <compiler/backend/unwind/ExceptionMapper.h>

using namespace cpp20::compiler::backend::unwind;

/**
 * @brief Función de ejemplo con prólogo típico
 * Esta función simula un prólogo típico de Windows x64
 */
extern "C" void exampleFunction() {
    // Este código sería generado por el compilador
    // PUSH RBP          ; 0x55
    // MOV RBP, RSP      ; 0x48 0x89 0xE5
    // SUB RSP, 32       ; 0x48 0x83 0xEC 0x20
    // ... function body ...
    // ADD RSP, 32       ; 0x48 0x83 0xC4 0x20
    // POP RBP           ; 0x5D
    // RET               ; 0xC3
}

int main() {
    std::cout << "=== Test de Capa 2: Sistema de Unwind ===\n";

    // Crear emisor de unwind
    UnwindEmitter emitter;

    // Simular prólogo de función típica de Windows x64
    // PUSH RBP (0x55) + MOV RBP, RSP (0x48 0x89 0xE5) + SUB RSP, 32 (0x48 0x83 0xEC 0x20)
    std::vector<uint8_t> prologueBytes = {
        0x55,                           // PUSH RBP
        0x48, 0x89, 0xE5,              // MOV RBP, RSP
        0x48, 0x83, 0xEC, 0x20         // SUB RSP, 32
    };

    // Añadir información de unwind para una función
    emitter.addFunctionUnwind(
        0x1000,        // RVA de inicio de función
        0x50,          // Tamaño de función
        prologueBytes, // Bytes del prólogo
        32,            // Tamaño del stack frame
        5,             // RBP como frame register
        false          // No tiene exception handler
    );

    // Establecer RVA base para .xdata
    emitter.setXdataBaseRVA(0x2000);

    // Generar secciones .pdata y .xdata
    auto pdataSection = emitter.generatePdataSection();
    auto xdataSection = emitter.generateXdataSection();

    std::cout << "Sección .pdata generada: " << pdataSection.size() << " bytes\n";
    std::cout << "Sección .xdata generada: " << xdataSection.size() << " bytes\n";

    // Validar información de unwind
    bool isValid = emitter.validateAll();
    std::cout << "Información de unwind válida: " << (isValid ? "Sí" : "No") << "\n";

    // === Test del mapeo de excepciones ===

    std::cout << "\n=== Test de Mapeo de Excepciones ===\n";

    ExceptionMapper exceptionMapper;

    // Añadir una región try/catch
    TryCatchRegion tryCatchRegion(
        0x1020,    // try start RVA
        0x1030,    // try end RVA
        0x1040,    // catch start RVA
        0x1050,    // catch end RVA
        0x3000     // exception type RVA
    );
    exceptionMapper.addTryCatchRegion(tryCatchRegion);

    // Añadir un sitio de throw
    ThrowSite throwSite(
        0x1025,    // throw RVA
        0x3000     // exception type RVA
    );
    exceptionMapper.addThrowSite(throwSite);

    std::cout << "Regiones try/catch: " << exceptionMapper.getTryCatchRegionCount() << "\n";
    std::cout << "Sitios de throw: " << exceptionMapper.getThrowSiteCount() << "\n";
    std::cout << "Tiene excepciones: " << (exceptionMapper.hasExceptions() ? "Sí" : "No") << "\n";

    // Generar información de exception handler
    uint32_t handlerRVA = exceptionMapper.generateExceptionHandler();
    std::cout << "Exception handler RVA: 0x" << std::hex << handlerRVA << std::dec << "\n";

    // Generar datos de excepción
    auto exceptionData = exceptionMapper.generateExceptionData();
    std::cout << "Datos de excepción generados: " << exceptionData.size() << " bytes\n";

    std::cout << "\n=== Capa 2 completada exitosamente ===\n";

    return 0;
}
