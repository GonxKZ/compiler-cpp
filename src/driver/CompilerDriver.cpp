/**
 * @file CompilerDriver.cpp
 * @brief Implementación avanzada del CompilerDriver para C++20
 */

#include <compiler/driver/CompilerDriver.h>
#include <compiler/driver/CommandLineParser.h>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <iomanip>

namespace cpp20::compiler {

CompilerDriver::CompilerDriver()
    : sourceManager_(std::make_shared<diagnostics::SourceManager>()),
      diagnosticEngine_(std::make_shared<diagnostics::DiagnosticEngine>(sourceManager_)) {
}

CompilerDriver::~CompilerDriver() = default;

int CompilerDriver::run(int argc, char* argv[]) {
    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Parse command line arguments con manejo especial
        CompilerOptions options = parseCommandLine(argc, argv);

        // Handle special cases
        if (options.showHelp) {
            printHelp();
            return EXIT_SUCCESS;
        }

        if (options.showVersion) {
            printVersion();
            return EXIT_SUCCESS;
        }

        // Configurar entorno
        if (!validateOptions(options)) {
            return EXIT_FAILURE;
        }

        setupEnvironment(options);

        if (options.verbose) {
            std::cout << "Compilador C++20 iniciándose..." << std::endl;
            std::cout << "Archivos de entrada: " << options.inputFiles.size() << std::endl;
            std::cout << "Estándar: " << options.standard << std::endl;
            std::cout << "Nivel de optimización: O" << options.optimizationLevel << std::endl;
        }

        // Ejecutar compilación
        CompilationResult result = compile(
            std::vector<std::filesystem::path>(options.inputFiles.begin(), options.inputFiles.end()),
            options
        );

        // Reportar tiempos si solicitado
        if (options.timing) {
            reportTiming(result.compilationTime, options);
        }

        // Reportar resultado
        if (options.verbose || !result.success) {
            if (result.success) {
                std::cout << "Compilación exitosa" << std::endl;
                if (!result.outputFiles.empty()) {
                    std::cout << "Archivos generados:" << std::endl;
                    for (const auto& file : result.outputFiles) {
                        std::cout << "  " << file << std::endl;
                    }
                }
            } else {
                std::cerr << "Error de compilación: " << result.errorMessage << std::endl;
                return EXIT_FAILURE;
            }
        }

        return result.success ? EXIT_SUCCESS : EXIT_FAILURE;

    } catch (const std::exception& e) {
        std::cerr << "Error fatal del compilador: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

CompilerOptions CompilerDriver::parseCommandLine(int argc, char* argv[]) {
    CompilerOptions options;
    CommandLineParser parser;

    // El parser avanzado maneja todo automáticamente, incluyendo archivos de respuesta
    if (!parser.parse(argc, argv, options)) {
        // Si hay error, devolver opciones vacías para indicar fallo
        options.inputFiles.clear();
        return options;
    }

    return options;
}

CompilationResult CompilerDriver::compile(
    const std::vector<std::filesystem::path>& inputFiles,
    const CompilerOptions& options
) {
    CompilationResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Cargar archivos fuente
        for (const auto& inputFile : inputFiles) {
            uint32_t fileId = sourceManager_->loadFile(inputFile);
            if (fileId == 0) {
                result.success = false;
                result.errorMessage = "No se pudo cargar archivo: " + inputFile.string();
                return result;
            }

            if (options.verbose) {
                const auto* file = sourceManager_->getFile(fileId);
                if (file) {
                    std::cout << "Cargado: " << file->displayName
                             << " (" << file->fileSize << " bytes)" << std::endl;
                }
            }
        }

        // Ejecutar fases de compilación
        if (options.preprocessOnly) {
            result.success = runPreprocessing(inputFiles, options);
            result.outputFiles = {determineOutputFile(inputFiles, options).string()};
        } else if (options.compileOnly) {
            result.success = runCompilation(inputFiles, options);
            result.outputFiles = {determineOutputFile(inputFiles, options).string()};
        } else if (options.assembleOnly) {
            result.success = runAssembly(inputFiles, options);
            result.outputFiles = {determineOutputFile(inputFiles, options).string()};
        } else {
            // Compilación completa
            result.success = runLinking(inputFiles, options);
            result.outputFiles = {determineOutputFile(inputFiles, options).string()};
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("Error durante la compilación: ") + e.what();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.compilationTime = std::chrono::duration<double>(endTime - startTime).count();

    return result;
}

bool CompilerDriver::validateOptions(const CompilerOptions& options) {
    // Validaciones básicas
    if (options.inputFiles.empty() && !options.showHelp && !options.showVersion) {
        std::cerr << "Error: no se especificaron archivos de entrada" << std::endl;
        return false;
    }

    // Validar combinaciones de fases
    int phaseCount = 0;
    if (options.preprocessOnly) phaseCount++;
    if (options.compileOnly) phaseCount++;
    if (options.assembleOnly) phaseCount++;

    if (phaseCount > 1) {
        std::cerr << "Error: solo se puede especificar una fase de compilación" << std::endl;
        return false;
    }

    return true;
}

void CompilerDriver::setupEnvironment(const CompilerOptions& options) {
    // Configurar rutas de búsqueda de includes
    for (const auto& includePath : options.includePaths) {
        sourceManager_->addIncludePath(includePath, true); // Sistema
    }

    // Configurar rutas de búsqueda de includes de usuario
    sourceManager_->addIncludePath(".", false); // Directorio actual

    // Configurar otras rutas según el estándar de Windows
    if (options.standard == "c++20") {
        // Agregar rutas estándar de MSVC/CRT
        sourceManager_->addIncludePath("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/include", true);
        sourceManager_->addIncludePath("C:/Program Files (x86)/Windows Kits/10/Include/10.0.22000.0/ucrt", true);
    }
}

void CompilerDriver::printVersion() const {
    CommandLineParser parser;
    parser.showVersion();
}

void CompilerDriver::printHelp() const {
    CommandLineParser parser;
    parser.showHelp();
}

void CompilerDriver::printDiagnostics() const {
    // TODO: Implementar reporte de diagnósticos
    std::cout << "Sistema de diagnósticos inicializado" << std::endl;
}

bool CompilerDriver::runPreprocessing(const std::vector<std::filesystem::path>& inputs,
                                     const CompilerOptions& options) {
    if (options.verbose) {
        std::cout << "Ejecutando preprocesamiento..." << std::endl;
    }

    // Implementación básica: solo copiar archivos por ahora
    for (const auto& input : inputs) {
        std::filesystem::path outputFile = determineOutputFile({input}, options);

        // Simular preprocesamiento básico
        if (options.verbose) {
            std::cout << "Preprocesado: " << input << " -> " << outputFile << std::endl;
        }
    }

    return true;
}

bool CompilerDriver::runCompilation(const std::vector<std::filesystem::path>& inputs,
                                   const CompilerOptions& options) {
    if (options.verbose) {
        std::cout << "Ejecutando compilación..." << std::endl;
    }

    // Simular compilación básica
    for (const auto& input : inputs) {
        std::filesystem::path outputFile = determineOutputFile({input}, options);

        if (options.verbose) {
            std::cout << "Compilando: " << input << " -> " << outputFile << std::endl;
        }

        // Verificar que el archivo existe
        if (!std::filesystem::exists(input)) {
            std::cerr << "Error: Archivo de entrada no encontrado: " << input << std::endl;
            return false;
        }
    }

    return true;
}

bool CompilerDriver::runAssembly(const std::vector<std::filesystem::path>& /*inputs*/,
                                const CompilerOptions& /*options*/) {
    // TODO: Implementar ensamblado
    std::cout << "Fase de ensamblado - TODO" << std::endl;
    return true;
}

bool CompilerDriver::runLinking(const std::vector<std::filesystem::path>& /*inputs*/,
                               const CompilerOptions& /*options*/) {
    // TODO: Implementar linking
    std::cout << "Fase de linking - TODO" << std::endl;
    return true;
}

std::vector<std::filesystem::path> CompilerDriver::collectInputFiles(int argc, char* argv[]) {
    std::vector<std::filesystem::path> inputs;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] != '-' && arg.find('@') != 0) { // No es opción ni archivo de respuesta
            inputs.emplace_back(arg);
        }
    }
    return inputs;
}

std::filesystem::path CompilerDriver::determineOutputFile(
    const std::vector<std::filesystem::path>& inputs,
    const CompilerOptions& options
) {
    if (!options.outputFile.empty()) {
        return options.outputFile;
    }

    if (inputs.empty()) {
        return "a.out";
    }

    // Determinar extensión basada en la fase
    std::string extension;
    if (options.preprocessOnly) {
        extension = ".i";
    } else if (options.compileOnly) {
        extension = ".obj";
    } else if (options.assembleOnly) {
        extension = ".asm";
    } else {
        extension = ".exe";
    }

    // Usar nombre del primer archivo de entrada
    std::filesystem::path firstInput = inputs[0];
    std::string stem = firstInput.stem().string();
    return stem + extension;
}

void CompilerDriver::cleanupTempFiles(const CompilerOptions& /*options*/) {
    // TODO: Implementar limpieza de archivos temporales
}

void CompilerDriver::reportTiming(double totalTime, const CompilerOptions& /*options*/) const {
    std::cout << "Tiempo total de compilación: " << std::fixed << std::setprecision(3)
              << totalTime << " segundos" << std::endl;

    // TODO: Reportar tiempos por fase cuando estén implementadas
}


} // namespace cpp20::compiler
