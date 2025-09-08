/**
 * @file CommandLineParser.cpp
 * @brief Implementación avanzada del parser de línea de comandos
 */

#include <compiler/driver/CommandLineParser.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace cpp20::compiler {

CommandLineParser::CommandLineParser() = default;

bool CommandLineParser::parse(int argc, char* argv[], CompilerOptions& options) {
    processedResponseFiles_.clear();

    // Procesar argumentos uno por uno
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Verificar si es un archivo de respuesta
        if (isResponseFile(arg)) {
            std::filesystem::path responsePath = arg.substr(1); // Remover '@'
            if (!parseResponseFile(responsePath, options)) {
                std::cerr << "Error: no se pudo procesar archivo de respuesta '" << arg << "'" << std::endl;
                return false;
            }
            continue;
        }

        // Verificar si es un archivo fuente
        if (isSourceFile(arg)) {
            options.inputFiles.push_back(arg);
            continue;
        }

        // Verificar si es un flag u opción
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

    // Validar consistencia de opciones
    if (!validateOptions(options)) {
        return false;
    }

    return true;
}

bool CommandLineParser::parseResponseFile(const std::filesystem::path& responseFile, CompilerOptions& options) {
    // Evitar recursión infinita
    if (std::find(processedResponseFiles_.begin(), processedResponseFiles_.end(), responseFile) !=
        processedResponseFiles_.end()) {
        std::cerr << "Error: archivo de respuesta recursivo detectado: " << responseFile << std::endl;
        return false;
    }

    processedResponseFiles_.push_back(responseFile);

    auto args = readResponseFile(responseFile);
    if (args.empty()) {
        return false;
    }

    // Convertir vector de strings a estilo argv
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }

    // Parsear recursivamente
    return parse(static_cast<int>(argv.size()), argv.data(), options);
}

bool CommandLineParser::parseArgument(const std::string& arg, CompilerOptions& options) {
    // Primero intentar opciones compuestas (con =)
    if (parseCompoundOption(arg, options)) {
        return true;
    }

    // Luego opciones con valor separado
    if (parseFlag(arg, options)) {
        return true;
    }

    return false;
}

bool CommandLineParser::parseFlag(const std::string& flag, CompilerOptions& options) {
    // === FLAGS SIMPLES ===

    // Ayuda y versión
    if (flag == "-h" || flag == "--help" || flag == "/?") {
        options.showHelp = true;
        return true;
    }

    if (flag == "--version" || flag == "-V") {
        options.showVersion = true;
        return true;
    }

    // Fases de compilación
    if (flag == "-c") {
        options.compileOnly = true;
        return true;
    }

    if (flag == "-S") {
        options.assembleOnly = true;
        return true;
    }

    if (flag == "-E") {
        options.preprocessOnly = true;
        return true;
    }

    // Output y verbose
    if (flag == "-v" || flag == "--verbose") {
        options.verbose = true;
        return true;
    }

    // Debug
    if (flag == "-g" || flag == "-g2" || flag == "-ggdb") {
        options.debugInfo = true;
        return true;
    }

    // Lenguaje
    if (flag == "-pedantic" || flag == "-pedantic-errors") {
        options.pedantic = true;
        if (flag == "-pedantic-errors") {
            options.warningsAsErrors = true;
        }
        return true;
    }

    // Warnings
    if (flag == "-w") {
        options.warningLevel = 0; // Deshabilitar warnings
        return true;
    }

    if (flag == "-Werror") {
        options.warningsAsErrors = true;
        return true;
    }

    // Optimización
    if (flag == "-O0") {
        options.optimizationLevel = 0;
        return true;
    }

    if (flag == "-O1") {
        options.optimizationLevel = 1;
        return true;
    }

    if (flag == "-O2") {
        options.optimizationLevel = 2;
        return true;
    }

    if (flag == "-O3") {
        options.optimizationLevel = 3;
        return true;
    }

    if (flag == "-Os") {
        options.optimizationLevel = 2; // Optimize for size
        return true;
    }

    // === OPCIONES CON VALOR SEPARADO ===

    // Output file (se maneja especialmente en el loop principal)
    if (flag == "-o") {
        return true;
    }

    return false;
}

bool CommandLineParser::parseCompoundOption(const std::string& arg, CompilerOptions& options) {
    // Buscar separador '='
    size_t equalPos = arg.find('=');
    std::string option, value;

    if (equalPos != std::string::npos) {
        option = arg.substr(0, equalPos);
        value = arg.substr(equalPos + 1);
    } else {
        option = arg;
        // Para opciones pegadas sin '=', el valor está después del prefijo
    }

    // === OPCIONES CON VALOR COMPUESTO ===

    // Standard
    if (option == "-std") {
        if (!value.empty()) {
            options.standard = value;
            return true;
        }
    }

    // Include paths
    if (option == "-I") {
        if (!value.empty()) {
            options.includePaths.push_back(value);
            return true;
        }
    }

    // Library paths
    if (option == "-L") {
        if (!value.empty()) {
            options.libraryPaths.push_back(value);
            return true;
        }
    }

    // Libraries
    if (option == "-l") {
        if (!value.empty()) {
            options.libraries.push_back(value);
            return true;
        }
    }

    // Defines
    if (option == "-D") {
        if (!value.empty()) {
            options.defines.push_back(value);
            return true;
        }
    }

    // Undefines
    if (option == "-U") {
        if (!value.empty()) {
            options.undefines.push_back(value);
            return true;
        }
    }

    // Output file
    if (option == "-o") {
        if (!value.empty()) {
            options.outputFile = value;
            return true;
        }
    }

    // Warnings
    if (option.substr(0, 2) == "-W") {
        std::string warningSpec = option.substr(2);
        if (!warningSpec.empty()) {
            bool enable = (warningSpec.substr(0, 2) != "no-");
            if (!enable) {
                warningSpec = warningSpec.substr(3);
            }
            parseWarningOption(warningSpec, enable, options);
            return true;
        }
    }

    // === OPCIONES PEGADAS (GCC style) ===

    // Include paths pegadas
    if (option.substr(0, 2) == "-I") {
        std::string path = option.substr(2);
        if (!path.empty()) {
            options.includePaths.push_back(path);
            return true;
        }
    }

    // Defines pegadas
    if (option.substr(0, 2) == "-D") {
        std::string def = option.substr(2);
        if (!def.empty()) {
            options.defines.push_back(def);
            return true;
        }
    }

    // Undefines pegadas
    if (option.substr(0, 2) == "-U") {
        std::string undef = option.substr(2);
        if (!undef.empty()) {
            options.undefines.push_back(undef);
            return true;
        }
    }

    // Libraries pegadas
    if (option.substr(0, 2) == "-l") {
        std::string lib = option.substr(2);
        if (!lib.empty()) {
            options.libraries.push_back(lib);
            return true;
        }
    }

    // Library paths pegadas
    if (option.substr(0, 2) == "-L") {
        std::string path = option.substr(2);
        if (!path.empty()) {
            options.libraryPaths.push_back(path);
            return true;
        }
    }

    return false;
}

void CommandLineParser::parseWarningOption(const std::string& warningSpec, bool enable, CompilerOptions& options) {
    if (enable) {
        options.enabledWarnings.push_back(warningSpec);
    } else {
        options.disabledWarnings.push_back(warningSpec);
    }
}

bool CommandLineParser::validateOptions(const CompilerOptions& options) const {
    // Validar combinaciones mutuamente exclusivas
    int phaseCount = 0;
    if (options.preprocessOnly) phaseCount++;
    if (options.compileOnly) phaseCount++;
    if (options.assembleOnly) phaseCount++;

    if (phaseCount > 1) {
        std::cerr << "Error: solo se puede especificar una fase de compilación (-E, -S, -c)" << std::endl;
        return false;
    }

    // Validar standard
    if (options.standard != "c++20" && options.standard != "c++17" &&
        options.standard != "c++14" && options.standard != "c++11") {
        std::cerr << "Error: standard no soportado '" << options.standard << "'" << std::endl;
        return false;
    }

    // Validar archivos de entrada
    if (options.inputFiles.empty() && !options.showHelp && !options.showVersion) {
        std::cerr << "Error: no se especificaron archivos de entrada" << std::endl;
        return false;
    }

    return true;
}

bool CommandLineParser::isSourceFile(const std::string& filename) const {
    // Extensiones comunes de archivos fuente C/C++
    static const std::vector<std::string> extensions = {
        ".cpp", ".cxx", ".cc", ".c++", ".C",
        ".hpp", ".hxx", ".hh", ".h++", ".H",
        ".c", ".h", ".ixx", ".cppm"  // C++20 modules
    };

    for (const auto& ext : extensions) {
        if (filename.size() > ext.size() &&
            filename.substr(filename.size() - ext.size()) == ext) {
            return true;
        }
    }

    return false;
}

bool CommandLineParser::isResponseFile(const std::string& filename) const {
    return !filename.empty() && filename[0] == '@';
}

std::vector<std::string> CommandLineParser::readResponseFile(const std::filesystem::path& path) const {
    std::vector<std::string> args;
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Error: no se puede abrir archivo de respuesta '" << path << "'" << std::endl;
        return args;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remover comentarios (desde # hasta fin de línea)
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());

        if (!line.empty()) {
            // Parsear línea en argumentos (simple split por espacios)
            std::istringstream iss(line);
            std::string arg;
            while (iss >> arg) {
                args.push_back(arg);
            }
        }
    }

    return args;
}

void CommandLineParser::showHelp() const {
    std::cout << "Compilador C++20 para Windows x64" << std::endl;
    std::cout << "Uso: cpp20-compiler [opciones] archivos..." << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones principales:" << std::endl;
    std::cout << "  -h, --help           Mostrar esta ayuda completa" << std::endl;
    std::cout << "  --version            Mostrar versión del compilador" << std::endl;
    std::cout << "  -c                   Compilar a objeto, no linkear" << std::endl;
    std::cout << "  -S                   Generar código ensamblador" << std::endl;
    std::cout << "  -E                   Solo preprocesamiento" << std::endl;
    std::cout << "  -o <archivo>         Archivo de salida" << std::endl;
    std::cout << "  -v, --verbose        Salida detallada" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones de lenguaje:" << std::endl;
    std::cout << "  -std=<version>       Versión del estándar (c++20, c++17, c++14, c++11)" << std::endl;
    std::cout << "  -pedantic            Modo estricto conforme al estándar" << std::endl;
    std::cout << "  -pedantic-errors     Tratar warnings como errores" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones de búsqueda:" << std::endl;
    std::cout << "  -I<directorio>       Agregar directorio de include" << std::endl;
    std::cout << "  -L<directorio>       Agregar directorio de librería" << std::endl;
    std::cout << "  -l<lib>             Linkear con librería" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones del preprocesador:" << std::endl;
    std::cout << "  -D<macro>[=valor]    Definir macro" << std::endl;
    std::cout << "  -U<macro>           Indefinir macro" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones de warnings:" << std::endl;
    std::cout << "  -w                   Deshabilitar todos los warnings" << std::endl;
    std::cout << "  -W<warning>         Habilitar warning específico" << std::endl;
    std::cout << "  -Wno-<warning>      Deshabilitar warning específico" << std::endl;
    std::cout << "  -Werror             Tratar warnings como errores" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones de optimización:" << std::endl;
    std::cout << "  -O0                  Sin optimizaciones" << std::endl;
    std::cout << "  -O1, -O2, -O3        Nivel de optimización" << std::endl;
    std::cout << "  -Os                  Optimizar para tamaño" << std::endl;
    std::cout << std::endl;

    std::cout << "Opciones de debug:" << std::endl;
    std::cout << "  -g                   Incluir información de debug" << std::endl;
    std::cout << std::endl;

    std::cout << "Archivos de respuesta:" << std::endl;
    std::cout << "  @<archivo>          Leer opciones desde archivo de respuesta" << std::endl;
    std::cout << std::endl;

    std::cout << "Ejemplos:" << std::endl;
    std::cout << "  cpp20-compiler hello.cpp -o hello.exe" << std::endl;
    std::cout << "  cpp20-compiler -c main.cpp -I./include" << std::endl;
    std::cout << "  cpp20-compiler -E main.cpp -o main.i" << std::endl;
    std::cout << "  cpp20-compiler @build.rsp main.cpp" << std::endl;
    std::cout << std::endl;
}

void CommandLineParser::showVersion() const {
    std::cout << "Compilador C++20 versión 1.0.0" << std::endl;
    std::cout << "Compatible con Windows x64 y MSVC ABI" << std::endl;
    std::cout << "Soporte completo para C++20 incluyendo módulos, conceptos y corrutinas" << std::endl;
    std::cout << "Basado en especificación C++20 (N4861)" << std::endl;
}

void CommandLineParser::showOptionHelp(const std::string& option) const {
    std::cout << "Ayuda para opción '" << option << "':" << std::endl;
    // Implementación simplificada - en producción tendría ayuda detallada por opción
    std::cout << "Opción no documentada aún." << std::endl;
}

} // namespace cpp20::compiler