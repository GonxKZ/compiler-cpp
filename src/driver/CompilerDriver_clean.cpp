/**
 * @file CompilerDriver.cpp
 * @brief Implementación simplificada del CompilerDriver
 */

#include <compiler/driver/CompilerDriver.h>
#include <iostream>

namespace cpp20::compiler {

CompilerDriver::CompilerDriver()
    : sourceManager_(std::make_shared<diagnostics::SourceManager>()),
      diagnosticEngine_(std::make_shared<diagnostics::DiagnosticEngine>(sourceManager_)) {
}

CompilerDriver::~CompilerDriver() = default;

int CompilerDriver::run(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        CompilerOptions options;
        CommandLineParser parser;
        if (!parser.parse(argc, argv, options)) {
            return EXIT_FAILURE;
        }

        // Handle special cases
        if (options.showHelp) {
            parser.showHelp();
            return EXIT_SUCCESS;
        }

        if (options.showVersion) {
            parser.showVersion();
            return EXIT_SUCCESS;
        }

        if (options.verbose) {
            std::cout << "C++20 Compiler starting..." << std::endl;
        }

        // Mostrar archivos de entrada
        if (!options.inputFiles.empty()) {
            std::cout << "Input files:" << std::endl;
            for (const auto& file : options.inputFiles) {
                std::cout << "  " << file << std::endl;
            }
        } else {
            std::cerr << "No input files specified" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Compilation phases:" << std::endl;
        if (options.preprocessOnly) {
            std::cout << "  Preprocessing only" << std::endl;
        } else if (options.compileOnly) {
            std::cout << "  Compile only" << std::endl;
        } else if (options.assembleOnly) {
            std::cout << "  Assemble only" << std::endl;
        } else {
            std::cout << "  Full compilation and linking" << std::endl;
        }

        std::cout << "Output: " << (options.outputFile.empty() ? "default" : options.outputFile) << std::endl;
        std::cout << "Standard: " << options.standard << std::endl;

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "Compiler error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

} // namespace cpp20::compiler
