#pragma once

#include <string>
#include <vector>

namespace cpp20::compiler {

/**
 * @brief Opciones de configuración del compilador
 */
struct CompilerOptions {
    // Archivos
    std::string outputFile;
    std::vector<std::string> inputFiles;

    // Modos
    bool compileOnly = false;
    bool preprocessOnly = false;
    bool assembleOnly = false;
    bool showHelp = false;
    bool showVersion = false;

    // Opciones generales
    bool verbose = false;
    int optimizationLevel = 0;
    bool debugInfo = false;
    std::string standard = "c++20";

    // Rutas
    std::vector<std::string> includePaths;
};

/**
 * @brief Parser de línea de comandos
 */
class CommandLineParser {
public:
    CommandLineParser();

    /**
     * @brief Parsea los argumentos de línea de comandos
     * @param argc Número de argumentos
     * @param argv Array de argumentos
     * @param options Estructura donde se guardan las opciones
     * @return true si el parsing fue exitoso
     */
    bool parse(int argc, char* argv[], CompilerOptions& options);

    /**
     * @brief Muestra la ayuda
     */
    void showHelp() const;

    /**
     * @brief Muestra la versión
     */
    void showVersion() const;

    /**
     * @brief Obtiene los archivos de entrada
     */
    const std::vector<std::string>& getInputFiles() const;

private:
    std::string programName_;
    std::vector<std::string> inputFiles_;
};

} // namespace cpp20::compiler
