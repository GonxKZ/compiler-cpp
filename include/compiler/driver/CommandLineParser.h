#pragma once

#include <compiler/driver/CompilerDriver.h>
#include <string>
#include <vector>
#include <filesystem>

namespace cpp20::compiler {

/**
 * @brief Parser avanzado de línea de comandos para el compilador C++20
 *
 * Soporta todas las opciones estándar de GCC/Clang/MSVC incluyendo:
 * - Opciones de fase de compilación (-E, -S, -c)
 * - Opciones de salida (-o, -v)
 * - Opciones de lenguaje (-std=c++20)
 * - Opciones de búsqueda (-I, -L, -l)
 * - Opciones de preprocesador (-D, -U)
 * - Opciones de warning (-W, -w)
 * - Archivos de respuesta (@file.rsp)
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
     * @brief Parsea un archivo de respuesta
     * @param responseFile Ruta al archivo de respuesta
     * @param options Opciones donde almacenar el resultado
     * @return true si el parseo fue exitoso, false en caso de error
     */
    bool parseResponseFile(const std::filesystem::path& responseFile, CompilerOptions& options);

    /**
     * @brief Muestra la ayuda completa del compilador
     */
    void showHelp() const;

    /**
     * @brief Muestra la versión del compilador
     */
    void showVersion() const;

    /**
     * @brief Muestra información detallada sobre una opción específica
     * @param option Nombre de la opción
     */
    void showOptionHelp(const std::string& option) const;

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
     * @brief Parsea una opción con valor separado
     * @param option Opción a parsear
     * @param value Valor de la opción
     * @param options Opciones donde almacenar
     * @return true si se procesó correctamente
     */
    bool parseOption(const std::string& option, const std::string& value, CompilerOptions& options);

    /**
     * @brief Parsea una opción con valor integrado (= o pegado)
     * @param arg Argumento completo (ej: -std=c++20)
     * @param options Opciones donde almacenar
     * @return true si se procesó correctamente
     */
    bool parseCompoundOption(const std::string& arg, CompilerOptions& options);

    /**
     * @brief Verifica si un string es un archivo fuente válido
     * @param filename Nombre del archivo
     * @return true si es un archivo fuente válido
     */
    bool isSourceFile(const std::string& filename) const;

    /**
     * @brief Verifica si un string es un archivo de respuesta
     * @param filename Nombre del archivo
     * @return true si es un archivo de respuesta
     */
    bool isResponseFile(const std::string& filename) const;

    /**
     * @brief Lee y parsea las líneas de un archivo de respuesta
     * @param path Ruta al archivo
     * @return Vector de argumentos parseados
     */
    std::vector<std::string> readResponseFile(const std::filesystem::path& path) const;

    /**
     * @brief Parsea opciones de warning
     * @param warningSpec Especificación del warning (ej: "no-unused-variable")
     * @param enable Si se debe habilitar o deshabilitar
     * @param options Opciones donde almacenar
     */
    void parseWarningOption(const std::string& warningSpec, bool enable, CompilerOptions& options);

    /**
     * @brief Valida la consistencia de las opciones
     * @param options Opciones a validar
     * @return true si las opciones son consistentes
     */
    bool validateOptions(const CompilerOptions& options) const;

    // Estado interno
    std::vector<std::filesystem::path> processedResponseFiles_; // Para evitar recursión
};

} // namespace cpp20::compiler