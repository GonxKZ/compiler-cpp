#pragma once

#include <compiler/driver/CompilerDriver.h>
#include <string>
#include <vector>

namespace cpp20::compiler {

/**
 * @brief Parser de línea de comandos para el compilador
 */
class CommandLineParser {
public:
    CommandLineParser();
    ~CommandLineParser() = default;

    /**
     * @brief Parsea los argumentos de línea de comandos
     * @param argc Número de argumentos
     * @param argv Array de argumentos
     * @param options Opciones donde almacenar el resultado
     * @return true si el parseo fue exitoso, false en caso de error
     */
    bool parse(int argc, char* argv[], CompilerOptions& options);

    /**
     * @brief Muestra la ayuda del compilador
     */
    void showHelp() const;

    /**
     * @brief Muestra la versión del compilador
     */
    void showVersion() const;

private:
    /**
     * @brief Parsea un argumento individual
     * @param arg Argumento a parsear
     * @param options Opciones donde almacenar
     * @return true si se procesó correctamente
     */
    bool parseArgument(const std::string& arg, CompilerOptions& options);

    /**
     * @brief Parsea un flag (argumento que no toma valor)
     * @param flag Flag a parsear
     * @param options Opciones donde almacenar
     * @return true si se reconoció el flag
     */
    bool parseFlag(const std::string& flag, CompilerOptions& options);

    /**
     * @brief Parsea una opción con valor
     * @param option Opción a parsear
     * @param value Valor de la opción
     * @param options Opciones donde almacenar
     * @return true si se procesó correctamente
     */
    bool parseOption(const std::string& option, const std::string& value, CompilerOptions& options);

    /**
     * @brief Verifica si un string es un archivo fuente válido
     * @param filename Nombre del archivo
     * @return true si es un archivo fuente válido
     */
    bool isSourceFile(const std::string& filename) const;
};

} // namespace cpp20::compiler