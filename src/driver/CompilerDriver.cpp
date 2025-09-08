/**
 * @file CompilerDriver.cpp
 * @brief Implementación del CompilerDriver
 */

#include <compiler/driver/CompilerDriver.h>
#include <iostream>
#include <chrono>

namespace cpp20::compiler {

CompilerDriver::CompilerDriver()
    : sourceManager_(std::make_shared<diagnostics::SourceManager>()),
      diagnosticEngine_(std::make_shared<diagnostics::DiagnosticEngine>(sourceManager_)) {
}

CompilerDriver::~CompilerDriver() = default;

int CompilerDriver::run(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        auto options = parseCommandLine(argc, argv);

        // Handle special cases
        if (options.verbose) {
            printVersion();
        }

        // Collect input files
        auto inputFiles = collectInputFiles(argc, argv);
        if (inputFiles.empty()) {
            std::cerr << "No input files specified" << std::endl;
            return EXIT_FAILURE;
        }

        // Compile
        auto result = compile(inputFiles, options);

        // Report results
        if (options.verbose) {
            reportTiming(result.compilationTime, options);
        }

        return result.success ? EXIT_SUCCESS : EXIT_FAILURE;

    } catch (const std::exception& e) {
        std::cerr << "Compiler error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

CompilationResult CompilerDriver::compile(
    const std::vector<std::filesystem::path>& inputFiles,
    const CompilerOptions& options
) {
    CompilationResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Validate options
        if (!validateOptions(options)) {
            result.errorMessage = "Invalid compiler options";
            return result;
        }

        // Setup environment
        setupEnvironment(options);

        // Determine output file
        auto outputFile = determineOutputFile(inputFiles, options);

        // Run compilation phases
        bool success = true;

        if (options.preprocessOnly) {
            success = runPreprocessing(inputFiles, options);
        } else if (options.compileOnly) {
            success = runCompilation(inputFiles, options);
        } else if (options.assembleOnly) {
            success = runAssembly(inputFiles, options);
        } else {
            // Full compilation and linking
            success = runCompilation(inputFiles, options);
            if (success) {
                success = runLinking(inputFiles, options);
            }
        }

        result.success = success;
        result.outputFiles = {outputFile.string()};

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.compilationTime = std::chrono::duration<double>(endTime - startTime).count();

    return result;
}

CompilerOptions CompilerDriver::parseCommandLine(int argc, char* argv[]) {
    CompilerOptions options;

    // TODO: Implement proper command line parsing
    // For now, return default options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-o" && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (arg == "-c") {
            options.compileOnly = true;
        } else if (arg == "-E") {
            options.preprocessOnly = true;
        } else if (arg == "-S") {
            options.assembleOnly = true;
        } else if (arg.starts_with("-O")) {
            // Parse optimization level
            if (arg.size() > 2) {
                options.optimizationLevel = std::stoi(arg.substr(2));
            }
        } else if (arg == "-g") {
            options.debugInfo = true;
        } else if (arg == "-std=c++20") {
            options.standard = "c++20";
        }
    }

    return options;
}

bool CompilerDriver::validateOptions(const CompilerOptions& options) {
    // Basic validation
    if (options.optimizationLevel < 0 || options.optimizationLevel > 3) {
        return false;
    }

    if (options.standard != "c++20" && options.standard != "c++17") {
        return false;
    }

    return true;
}

void CompilerDriver::setupEnvironment(const CompilerOptions& options) {
    // TODO: Setup include paths, library paths, etc.
    (void)options; // Suppress unused parameter warning
}

void CompilerDriver::printVersion() const {
    std::cout << "C++20 Compiler v0.1.0" << std::endl;
    std::cout << "Target: x86_64-pc-windows-msvc" << std::endl;
    std::cout << "Built with LLVM integration" << std::endl;
}

void CompilerDriver::printHelp() const {
    std::cout << "Usage: cpp20-compiler [options] file..." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -c              Compile only, do not link" << std::endl;
    std::cout << "  -E              Preprocess only" << std::endl;
    std::cout << "  -S              Assemble only" << std::endl;
    std::cout << "  -o <file>       Output file" << std::endl;
    std::cout << "  -v, --verbose   Verbose output" << std::endl;
    std::cout << "  -g              Generate debug info" << std::endl;
    std::cout << "  -O<level>       Optimization level (0-3)" << std::endl;
    std::cout << "  --help          Show this help" << std::endl;
}

void CompilerDriver::printDiagnostics() const {
    // TODO: Print diagnostic summary
}

std::vector<std::filesystem::path> CompilerDriver::collectInputFiles(int argc, char* argv[]) {
    std::vector<std::filesystem::path> files;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Skip options
        if (arg.starts_with("-")) {
            if (arg == "-o" && i + 1 < argc) {
                ++i; // Skip output file argument
            }
            continue;
        }

        // Add as input file
        files.emplace_back(arg);
    }

    return files;
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

    auto input = inputs[0];
    if (options.compileOnly) {
        return input.stem().string() + ".obj";
    } else if (options.assembleOnly) {
        return input.stem().string() + ".asm";
    } else {
        return input.stem().string() + ".exe";
    }
}

void CompilerDriver::cleanupTempFiles(const CompilerOptions& options) {
    // TODO: Cleanup temporary files
    (void)options;
}

void CompilerDriver::reportTiming(double totalTime, const CompilerOptions& options) const {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Compilation time: " << totalTime << " seconds" << std::endl;
    (void)options; // Suppress unused parameter warning
}

// Placeholder implementations for compilation phases
bool CompilerDriver::runPreprocessing(const std::vector<std::filesystem::path>& inputs,
                                     const CompilerOptions& options) {
    std::cout << "Preprocessing " << inputs.size() << " files..." << std::endl;
    // TODO: Implement preprocessing
    (void)options;
    return true;
}

bool CompilerDriver::runCompilation(const std::vector<std::filesystem::path>& inputs,
                                   const CompilerOptions& options) {
    std::cout << "Compiling " << inputs.size() << " files..." << std::endl;

    for (const auto& input : inputs) {
        std::cout << "  " << input.string() << std::endl;
        // TODO: Implement actual compilation
    }

    (void)options;
    return true;
}

bool CompilerDriver::runAssembly(const std::vector<std::filesystem::path>& inputs,
                                const CompilerOptions& options) {
    std::cout << "Assembling " << inputs.size() << " files..." << std::endl;
    // TODO: Implement assembly generation
    (void)inputs;
    (void)options;
    return true;
}

bool CompilerDriver::runLinking(const std::vector<std::filesystem::path>& inputs,
                               const CompilerOptions& options) {
    std::cout << "Linking " << inputs.size() << " files..." << std::endl;
    // TODO: Implement linking
    (void)inputs;
    (void)options;
    return true;
}

} // namespace cpp20::compiler
