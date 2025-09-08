/**
 * @file CommandLineParser.cpp
 * @brief Parser de línea de comandos del driver
 */

#include <compiler/driver/CommandLineParser.h>
#include <iostream>

namespace cpp20::compiler {

// ========================================================================
// CompilerOptions implementation
// ========================================================================
// Constructor por defecto ya está definido inline en el header

// ========================================================================
// CommandLineParser implementation
// ========================================================================

CommandLineParser::CommandLineParser() {
    // Constructor vacío - implementación simplificada
}

bool CommandLineParser::parse(int argc, char* argv[], CompilerOptions& options) {
    if (argc < 1) return false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
            return true;
        }

        if (arg == "--version" || arg == "-v") {
            options.showVersion = true;
            return true;
        }

        if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-o" && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (arg == "-I" && i + 1 < argc) {
            options.includePaths.push_back(argv[++i]);
        } else if (arg == "-std" && i + 1 < argc) {
            options.standard = argv[++i];
        } else if (arg == "-c") {
            options.compileOnly = true;
        } else if (arg == "-E") {
            options.preprocessOnly = true;
        } else if (arg == "-S") {
            options.assembleOnly = true;
        } else if (arg == "-g") {
            options.debugInfo = true;
        } else if (arg.size() >= 2 && arg.substr(0, 2) == "-O") {
            std::string optLevel = arg.substr(2);
            if (!optLevel.empty()) {
                try {
                    options.optimizationLevel = std::stoi(optLevel);
                } catch (const std::exception&) {
                    options.optimizationLevel = 1; // Default
                }
            }
        } else if (!arg.empty() && arg[0] != '-') {
            // Archivo de entrada
            options.inputFiles.push_back(arg);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return false;
        }
    }

    if (options.inputFiles.empty() && !options.showHelp && !options.showVersion) {
        std::cerr << "No input files specified" << std::endl;
        return false;
    }

    return true;
}

void CommandLineParser::showHelp() const {
    std::cout << "Usage: cpp20-compiler [options] file...\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c              Compile only, do not link\n";
    std::cout << "  -E              Preprocess only\n";
    std::cout << "  -S              Assemble only\n";
    std::cout << "  -o <file>       Output file\n";
    std::cout << "  -I <dir>        Add include directory\n";
    std::cout << "  -std <version>  C++ standard version\n";
    std::cout << "  -g              Generate debug information\n";
    std::cout << "  -O<level>       Optimization level (0-3)\n";
    std::cout << "  -v, --verbose   Verbose output\n";
    std::cout << "  -h, --help      Show this help\n";
    std::cout << "  --version       Show version information\n";
    std::cout << "\n";
}

void CommandLineParser::showVersion() const {
    std::cout << "C++20 Compiler v0.1.0" << std::endl;
    std::cout << "Target: x86_64-pc-windows-msvc" << std::endl;
}

} // namespace cpp20::compiler
