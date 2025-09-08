/**
 * @file hello_coff.cpp
 * @brief Generador de objeto COFF simple para "Hello World"
 *
 * Este ejemplo demuestra cómo usar el COFF writer para crear un objeto
 * COFF válido que puede ser enlazado con link.exe para producir un
 * ejecutable funcional.
 */

#include <compiler/backend/coff/COFFWriter.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <iostream>
#include <fstream>

using namespace cpp20::compiler::backend::coff;

/**
 * @brief Crea un objeto COFF simple con función "hello"
 *
 * Genera código máquina x86_64 que:
 * 1. Llama a printf con "Hello, World!\n"
 * 2. Retorna 0
 */
COFFObject createHelloWorldCOFF() {
    COFFObject object = createBasicCOFFObject();

    // Código máquina x86_64 para hello world
    // Este es un ejemplo simplificado - en la práctica usaríamos
    // código real generado por nuestro compilador
    std::vector<uint8_t> helloCode = {
        0x48, 0x83, 0xEC, 0x28,           // sub rsp, 40 (shadow space + alignment)
        0x48, 0x8D, 0x0D, 0x0A, 0x00, 0x00, 0x00,  // lea rcx, [rip+10] (string address)
        0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,        // call [rip+0] (printf - relocation needed)
        0x31, 0xC0,                          // xor eax, eax (return 0)
        0x48, 0x83, 0xC4, 0x28,             // add rsp, 40
        0xC3                                 // ret
    };

    // String "Hello, World!\n"
    std::vector<uint8_t> helloString = {
        'H', 'e', 'l', 'l', 'o', ',', ' ',
        'W', 'o', 'r', 'l', 'd', '!', '\n', 0
    };

    // Configurar secciones
    object.sections[0].data = helloCode;        // .text
    object.sections[0].characteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

    object.sections[2].data = helloString;      // .rdata
    object.sections[2].characteristics |= IMAGE_SCN_MEM_READ;

    // Agregar símbolos
    COFFSymbol mainSymbol("_main", IMAGE_SYM_CLASS_EXTERNAL);
    mainSymbol.sectionNumber = 1;  // .text section
    mainSymbol.type = 0x20;        // Function
    object.addSymbol(std::move(mainSymbol));

    COFFSymbol printfSymbol("_printf", IMAGE_SYM_CLASS_EXTERNAL);
    printfSymbol.sectionNumber = 0;  // Undefined (external)
    printfSymbol.type = 0x20;        // Function
    object.addSymbol(std::move(printfSymbol));

    // Agregar relocation para la llamada a printf
    IMAGE_RELOCATION printfReloc = {};
    printfReloc.VirtualAddress = 9;        // Offset en .text donde está la call
    printfReloc.SymbolTableIndex = 1;     // Índice del símbolo printf
    printfReloc.Type = IMAGE_REL_AMD64_REL32;  // Rel32 relocation
    object.sections[0].relocations.push_back(printfReloc);

    return object;
}

/**
 * @brief Función principal del ejemplo
 */
int main(int argc, char* argv[]) {
    std::cout << "Generando objeto COFF 'Hello World'..." << std::endl;

    // Crear objeto COFF
    COFFObject helloObj = createHelloWorldCOFF();

    // Escribir a archivo
    std::string outputFile = "hello.obj";
    if (writeCOFFObject(helloObj, outputFile)) {
        std::cout << "✅ Objeto COFF generado: " << outputFile << std::endl;

        // Mostrar información del objeto
        std::cout << "\nInformación del objeto COFF:" << std::endl;
        std::cout << "- Secciones: " << helloObj.header.NumberOfSections << std::endl;
        std::cout << "- Símbolos: " << helloObj.header.NumberOfSymbols << std::endl;
        std::cout << "- Relocations: " << helloObj.sections[0].relocations.size() << std::endl;

        // Hacer dump del objeto
        std::cout << "\nDump del objeto COFF:" << std::endl;
        COFFDumper dumper;
        dumper.dumpFile(outputFile, std::cout);

        std::cout << "\n💡 Para enlazar con link.exe:" << std::endl;
        std::cout << "   link.exe hello.obj /OUT:hello.exe /SUBSYSTEM:CONSOLE kernel32.lib ucrt.lib" << std::endl;

        return 0;
    } else {
        std::cerr << "❌ Error al generar objeto COFF" << std::endl;
        return 1;
    }
}
