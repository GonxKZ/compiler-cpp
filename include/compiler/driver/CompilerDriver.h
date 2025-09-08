#pragma once

#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <compiler/common/diagnostics/SourceManager.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace cpp20::compiler {

/**
 * @brief Opciones de configuración del compilador
 */
struct CompilerOptions {
    // Fases de compilación
    bool preprocessOnly = false;        // -E: solo preprocesamiento
    bool compileOnly = false;           // -c: compilar a objeto, no linkear
    bool assembleOnly = false;          // -S: generar ensamblador
    bool linkOnly = false;              // Solo linking

    // Salida
    std::filesystem::path outputFile;   // -o: archivo de salida
    bool verbose = false;               // -v: verbose output
    std::string outputFormat;           // Formato de salida (obj, exe, dll)

    // Optimización
    int optimizationLevel = 0;          // -O0, -O1, -O2, -O3
    bool debugInfo = false;             // -g: incluir información de debug
    bool lto = false;                   // -flto: link-time optimization

    // Lenguaje
    std::string standard = "c++20";     // -std=c++20
    bool pedantic = false;              // -pedantic: estrictamente conforme
    bool msExtensions = false;          // -fms-extensions: extensiones MSVC
    bool gnuExtensions = false;         // -fgnu-extensions: extensiones GNU

    // Warnings y diagnósticos
    bool warningsAsErrors = false;      // -Werror
    int warningLevel = 1;               // -W1, -W2, -W3, -W4
    std::vector<std::string> disabledWarnings;  // -Wno-*
    std::vector<std::string> enabledWarnings;   // -W*

    // Preprocesador
    std::vector<std::string> includePaths;     // -I: directorios de include
    std::vector<std::string> defines;          // -D: definiciones de macro
    std::vector<std::string> undefines;        // -U: undefinir macros

    // Linking
    std::vector<std::string> libraryPaths;     // -L: directorios de librerías
    std::vector<std::string> libraries;        // -l: librerías a linkear
    std::string linkerScript;           // -T: script de linker

    // Características específicas de C++20
    bool enableModules = true;          // -fmodules-ts
    bool enableCoroutines = true;       // -fcoroutines
    bool enableConcepts = true;         // -fconcepts

    // Plataforma específica
    std::string targetTriple = "x86_64-pc-windows-msvc";  // Triple LLVM
    std::string abi = "msvc";             // ABI a usar

    // Avanzado
    size_t maxErrors = 100;             // Máximo número de errores
    bool timing = false;                // -ftime-report: reportar tiempos
    std::string saveTemps;              // -save-temps: guardar archivos temporales

    // Ayuda y versión
    bool showHelp = false;              // -h, --help: mostrar ayuda
    bool showVersion = false;           // -v, --version: mostrar versión

    // Archivos de entrada
    std::vector<std::string> inputFiles; // Archivos fuente de entrada
};

/**
 * @brief Resultado de una compilación
 */
struct CompilationResult {
    bool success = false;
    int exitCode = 0;
    std::string errorMessage;
    std::vector<std::string> outputFiles;
    double compilationTime = 0.0;  // en segundos
};

/**
 * @brief Driver principal del compilador
 *
 * El CompilerDriver es responsable de:
 * - Parsear argumentos de línea de comandos
 * - Configurar el entorno de compilación
 * - Coordinar las fases de compilación
 * - Gestionar el flujo de trabajo completo
 * - Reportar resultados y estadísticas
 */
class CompilerDriver {
public:
    CompilerDriver();
    ~CompilerDriver();

    /**
     * @brief Ejecuta el compilador con los argumentos dados
     * @param argc Número de argumentos
     * @param argv Array de argumentos
     * @return Código de salida (0 = éxito)
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Compila un conjunto de archivos
     * @param inputFiles Archivos de entrada
     * @param options Opciones de compilación
     * @return Resultado de la compilación
     */
    CompilationResult compile(
        const std::vector<std::filesystem::path>& inputFiles,
        const CompilerOptions& options
    );

private:
    // Componentes principales
    std::shared_ptr<diagnostics::SourceManager> sourceManager_;
    std::shared_ptr<diagnostics::DiagnosticEngine> diagnosticEngine_;

    // Métodos internos
    CompilerOptions parseCommandLine(int argc, char* argv[]);
    bool validateOptions(const CompilerOptions& options);
    void setupEnvironment(const CompilerOptions& options);
    void printVersion() const;
    void printHelp() const;
    void printDiagnostics() const;

    // Fases de compilación
    bool runPreprocessing(const std::vector<std::filesystem::path>& inputs,
                         const CompilerOptions& options);
    bool runCompilation(const std::vector<std::filesystem::path>& inputs,
                       const CompilerOptions& options);
    bool runAssembly(const std::vector<std::filesystem::path>& inputs,
                    const CompilerOptions& options);
    bool runLinking(const std::vector<std::filesystem::path>& inputs,
                   const CompilerOptions& options);

    // Utilidades
    std::vector<std::filesystem::path> collectInputFiles(int argc, char* argv[]);
    std::filesystem::path determineOutputFile(
        const std::vector<std::filesystem::path>& inputs,
        const CompilerOptions& options
    );
    void cleanupTempFiles(const CompilerOptions& options);
    void reportTiming(double totalTime, const CompilerOptions& options) const;

};

} // namespace cpp20::compiler
