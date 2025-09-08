#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace cpp20::compiler::common::utils {

/**
 * @brief Parser de argumentos de línea de comandos
 */
class CommandLineParser {
public:
    enum class OptionType {
        String,     // Requiere valor string
        Integer,    // Requiere valor entero
        Flag        // Bandera booleana
    };

    struct Option {
        std::string name;
        std::string description;
        OptionType type;
        bool required;
        std::string defaultValue;
    };

    /**
     * @brief Constructor
     * @param programName Nombre del programa
     */
    explicit CommandLineParser(const std::string& programName = "");

    /**
     * @brief Agrega una opción
     * @param name Nombre de la opción (sin guiones)
     * @param description Descripción
     * @param type Tipo de opción
     * @param required Si es requerida
     */
    void addOption(const std::string& name, const std::string& description,
                  OptionType type = OptionType::String, bool required = false);

    /**
     * @brief Agrega una bandera
     * @param name Nombre de la bandera
     * @param description Descripción
     */
    void addFlag(const std::string& name, const std::string& description);

    /**
     * @brief Parsea los argumentos
     * @param argc Número de argumentos
     * @param argv Array de argumentos
     * @return true si el parsing fue exitoso
     */
    bool parse(int argc, char* argv[]);

    /**
     * @brief Verifica si una opción fue proporcionada
     * @param name Nombre de la opción
     * @return true si la opción está presente
     */
    bool hasOption(const std::string& name) const;

    /**
     * @brief Obtiene el valor de una opción
     * @param name Nombre de la opción
     * @param defaultValue Valor por defecto
     * @return Valor de la opción o defaultValue
     */
    std::string getOptionValue(const std::string& name,
                              const std::string& defaultValue = "") const;

    /**
     * @brief Obtiene los argumentos posicionales
     * @return Vector de argumentos posicionales
     */
    std::vector<std::string> getPositionalArgs() const;

    /**
     * @brief Imprime la ayuda
     */
    void printHelp() const;

private:
    std::string programName_;
    std::unordered_map<std::string, Option> options_;
    std::unordered_map<std::string, std::string> parsedOptions_;
    std::vector<std::string> positionalArgs_;

    bool parseLongOption(const std::string& arg, int argc, char* argv[], int& index);
    bool parseShortOption(const std::string& arg, int argc, char* argv[], int& index);
};

} // namespace cpp20::compiler::common::utils
