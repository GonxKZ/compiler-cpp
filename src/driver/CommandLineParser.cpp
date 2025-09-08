#include <compiler/driver/CommandLineParser.h>
#include <iostream>
#include <algorithm>

namespace cpp20::compiler {

CommandLineParser::CommandLineParser() {
}

bool CommandLineParser::parse(int argc, char* argv[], CompilerOptions& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Verificar si es un archivo fuente
        if (isSourceFile(arg)) {
            options.inputFiles.push_back(arg);
            continue;
        }

        // Verificar si es un flag
        if (arg[0] == '-') {
            if (!parseArgument(arg, options)) {
                std::cerr << "Error: argumento desconocido '" << arg << "'" << std::endl;
                return false;
            }
        } else {
            // Asumir que es un archivo fuente
            options.inputFiles.push_back(arg);
        }
    }

    return true;
}

bool CommandLineParser::parseArgument(const std::string& arg, CompilerOptions& options) {
    // Flags simples (sin valor)
    if (arg == "-h" || arg == "--help") {
        options.showHelp = true;
        return true;
    }

    if (arg == "--version") {
        options.showVersion = true;
        return true;
    }

    if (arg == "-c") {
        options.compileOnly = true;
        return true;
    }

    if (arg == "-S") {
        options.assembleOnly = true;
        return true;
    }

    if (arg == "-E") {
        options.preprocessOnly = true;
        return true;
    }

    if (arg == "-v" || arg == "--verbose") {
        options.verbose = true;
        return true;
    }

    if (arg == "-g") {
        options.debugInfo = true;
        return true;
    }

    if (arg == "-pedantic") {
        options.pedantic = true;
        return true;
    }

    // Opciones con valores
    if (arg.substr(0, 2) == "-O") {
        if (arg.size() == 2) {
            options.optimizationLevel = 1;
        } else {
            try {
                options.optimizationLevel = std::stoi(arg.substr(2));
            } catch (...) {
                options.optimizationLevel = 1;
            }
        }
        return true;
    }

    if (arg.substr(0, 2) == "-I") {
        if (arg.size() > 2) {
            options.includePaths.push_back(arg.substr(2));
        }
        return true;
    }

    if (arg.substr(0, 2) == "-D") {
        if (arg.size() > 2) {
            options.defines.push_back(arg.substr(2));
        }
        return true;
    }

    if (arg.substr(0, 2) == "-l") {
        if (arg.size() > 2) {
            options.libraries.push_back(arg.substr(2));
        }
        return true;
    }

    if (arg.substr(0, 2) == "-L") {
        if (arg.size() > 2) {
            options.libraryPaths.push_back(arg.substr(2));
        }
        return true;
    }

    if (arg.substr(0, 2) == "-o") {
        // Este caso se maneja especialmente porque necesita el siguiente argumento
        return true;
    }

    if (arg.substr(0, 5) == "-std=") {
        options.standard = arg.substr(5);
        return true;
    }

    return false;
}

void CommandLineParser::showHelp() const {
    std::cout << "Compilador C++20" << std::endl;
    std::cout << std::endl;
    std::cout << "Uso: cpp20-compiler [opciones] archivos..." << std::endl;
    std::cout << std::endl;
    std::cout << "Opciones principales:" << std::endl;
    std::cout << "  -h, --help           Mostrar esta ayuda" << std::endl;
    std::cout << "  --version            Mostrar version" << std::endl;
    std::cout << "  -c                   Compilar a objeto, no linkear" << std::endl;
    std::cout << "  -S                   Generar codigo ensamblador" << std::endl;
    std::cout << "  -E                   Solo preprocesamiento" << std::endl;
    std::cout << "  -o <archivo>         Archivo de salida" << std::endl;
    std::cout << "  -v, --verbose        Salida detallada" << std::endl;
    std::cout << std::endl;
    std::cout << "Opciones de optimizacion:" << std::endl;
    std::cout << "  -O0                  Sin optimizaciones" << std::endl;
    std::cout << "  -O1, -O2, -O3        Nivel de optimizacion" << std::endl;
    std::cout << std::endl;
    std::cout << "Opciones de busqueda:" << std::endl;
    std::cout << "  -I<directorio>       Agregar directorio de include" << std::endl;
    std::cout << "  -L<directorio>       Agregar directorio de libreria" << std::endl;
    std::cout << "  -l<lib>             Linkear con libreria" << std::endl;
    std::cout << std::endl;
    std::cout << "Opciones del lenguaje:" << std::endl;
    std::cout << "  -std=<version>       Version del estandar (default: c++20)" << std::endl;
    std::cout << "  -pedantic            Modo estricto" << std::endl;
    std::cout << std::endl;
}

void CommandLineParser::showVersion() const {
    std::cout << "Compilador C++20 version 1.0.0" << std::endl;
    std::cout << "Basado en LLVM y compatible con Windows x64" << std::endl;
}

bool CommandLineParser::isSourceFile(const std::string& filename) const {
    // Extensiones comunes de archivos fuente C++
    static const std::vector<std::string> extensions = {
        ".cpp", ".cxx", ".cc", ".c++", ".C",
        ".hpp", ".hxx", ".hh", ".h++", ".H",
        ".c", ".h"
    };

    for (const auto& ext : extensions) {
        if (filename.size() > ext.size() &&
            filename.substr(filename.size() - ext.size()) == ext) {
            return true;
        }
    }

    return false;
}

} // namespace cpp20::compiler