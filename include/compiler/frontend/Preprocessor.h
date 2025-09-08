/**
 * @file Preprocessor.h
 * @brief Preprocesador C++20
 */

#pragma once

#include <compiler/frontend/lexer/Token.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace cpp20::compiler::frontend {

/**
 * @brief Definición de macro
 */
struct MacroDefinition {
    std::string name;                    // Nombre de la macro
    std::vector<std::string> parameters; // Parámetros (vacío para macros sin parámetros)
    std::vector<lexer::Token> body;      // Cuerpo de la macro
    bool isFunctionLike;                 // Si es una macro de función
    bool isVariadic;                     // Si tiene parámetros variádicos

    MacroDefinition(const std::string& n, const std::vector<lexer::Token>& b,
                   bool funcLike = false, bool variadic = false)
        : name(n), body(b), isFunctionLike(funcLike), isVariadic(variadic) {}
};

/**
 * @brief Estado de inclusión de archivos
 */
struct IncludeState {
    std::string filename;                // Nombre del archivo
    bool isSystemInclude;               // Si es una inclusión de sistema
    size_t includeDepth;                // Profundidad de inclusión

    IncludeState(const std::string& fname, bool system = false, size_t depth = 0)
        : filename(fname), isSystemInclude(system), includeDepth(depth) {}
};

/**
 * @brief Configuración del preprocesador
 */
struct PreprocessorConfig {
    bool enableWarnings = true;         // Habilitar advertencias
    bool keepComments = false;          // Mantener comentarios
    size_t maxIncludeDepth = 100;       // Profundidad máxima de inclusión
    std::vector<std::string> includePaths; // Rutas de inclusión
    std::vector<std::string> systemIncludePaths; // Rutas de inclusión de sistema
};

/**
 * @brief Preprocesador C++20
 *
 * Maneja directivas de preprocesador como:
 * - #include
 * - #define / #undef
 * - #ifdef / #ifndef / #else / #endif
 * - #if / #elif / #endif
 * - #pragma
 * - #line / #error / #warning
 */
class Preprocessor {
public:
    /**
     * @brief Constructor
     */
    Preprocessor(diagnostics::DiagnosticEngine& diagEngine,
                const PreprocessorConfig& config = PreprocessorConfig());

    /**
     * @brief Destructor
     */
    ~Preprocessor();

    /**
     * @brief Procesar tokens de entrada
     */
    std::vector<lexer::Token> process(const std::vector<lexer::Token>& inputTokens);

    /**
     * @brief Definir una macro predefinida
     */
    void defineMacro(const std::string& name, const std::string& value);

    /**
     * @brief Definir una macro con cuerpo de tokens
     */
    void defineMacro(const MacroDefinition& macro);

    /**
     * @brief Undefinir una macro
     */
    void undefineMacro(const std::string& name);

    /**
     * @brief Verificar si una macro está definida
     */
    bool isMacroDefined(const std::string& name) const;

    /**
     * @brief Obtener definición de macro
     */
    const MacroDefinition* getMacro(const std::string& name) const;

    /**
     * @brief Añadir ruta de inclusión
     */
    void addIncludePath(const std::string& path, bool system = false);

    /**
     * @brief Obtener estadísticas del preprocesamiento
     */
    struct PreprocessorStats {
        size_t macrosDefined = 0;
        size_t macrosUndefined = 0;
        size_t conditionalsProcessed = 0;
        size_t includesProcessed = 0;
        size_t tokensProcessed = 0;
        size_t tokensGenerated = 0;
    };
    PreprocessorStats getStats() const { return stats_; }

private:
    diagnostics::DiagnosticEngine& diagEngine_; // Motor de diagnósticos
    PreprocessorConfig config_;                 // Configuración
    PreprocessorStats stats_;                   // Estadísticas

    // Estado del preprocesamiento
    std::unordered_map<std::string, MacroDefinition> macros_; // Macros definidas
    std::vector<IncludeState> includeStack_;     // Pila de inclusiones
    std::vector<bool> conditionalStack_;         // Pila de condicionales
    std::unordered_map<std::string, std::string> predefinedMacros_; // Macros predefinidas

    // Control de flujo
    size_t currentTokenIndex_ = 0;              // Índice del token actual
    std::vector<lexer::Token> inputTokens_;     // Tokens de entrada
    std::vector<lexer::Token> outputTokens_;    // Tokens de salida

    /**
     * @brief Procesar token actual
     */
    void processToken();

    /**
     * @brief Procesar directiva de preprocesador
     */
    void processDirective();

    /**
     * @brief Procesar #include
     */
    void processInclude();

    /**
     * @brief Procesar #define
     */
    void processDefine();

    /**
     * @brief Procesar #undef
     */
    void processUndef();

    /**
     * @brief Procesar #ifdef / #ifndef
     */
    void processIfdef(bool checkDefined);

    /**
     * @brief Procesar #if
     */
    void processIf();

    /**
     * @brief Procesar #else / #elif
     */
    void processElseOrElif();

    /**
     * @brief Procesar #endif
     */
    void processEndif();

    /**
     * @brief Procesar #pragma
     */
    void processPragma();

    /**
     * @brief Procesar #line
     */
    void processLine();

    /**
     * @brief Procesar #error / #warning
     */
    void processDiagnostic(bool isError);

    /**
     * @brief Expandir macro
     */
    std::vector<lexer::Token> expandMacro(const MacroDefinition& macro,
                                         const std::vector<std::vector<lexer::Token>>& arguments = {});

    /**
     * @brief Evaluar expresión condicional
     */
    bool evaluateConditionalExpression(const std::vector<lexer::Token>& expression);

    /**
     * @brief Obtener tokens hasta fin de línea
     */
    std::vector<lexer::Token> getTokensUntilEndOfLine();

    /**
     * @brief Obtener token actual
     */
    const lexer::Token& currentToken() const;

    /**
     * @brief Avanzar al siguiente token
     */
    void advanceToken();

    /**
     * @brief Verificar si estamos al final
     */
    bool isAtEnd() const;

    /**
     * @brief Saltar tokens hasta fin de línea
     */
    void skipToEndOfLine();

    /**
     * @brief Reportar error de preprocesamiento
     */
    void reportError(const std::string& message, const diagnostics::SourceLocation& location);

    /**
     * @brief Reportar advertencia de preprocesamiento
     */
    void reportWarning(const std::string& message, const diagnostics::SourceLocation& location);

    /**
     * @brief Inicializar macros predefinidas
     */
    void initializePredefinedMacros();

    /**
     * @brief Verificar si estamos en una sección condicional activa
     */
    bool isInActiveConditionalSection() const;

    /**
     * @brief Verificar si estamos saltando tokens (debido a #if/#ifdef false)
     */
    bool isSkippingTokens() const;
};

/**
 * @brief Utilidades para el preprocesamiento
 */
class PreprocessorUtils {
public:
    /**
     * @brief Verificar si un token es el inicio de una directiva
     */
    static bool isDirectiveStart(const lexer::Token& token);

    /**
     * @brief Extraer nombre de directiva
     */
    static std::string extractDirectiveName(const lexer::Token& token);

    /**
     * @brief Verificar si una línea contiene solo espacios en blanco
     */
    static bool isBlankLine(const std::vector<lexer::Token>& line);

    /**
     * @brief Unir tokens en una cadena
     */
    static std::string tokensToString(const std::vector<lexer::Token>& tokens);

    /**
     * @brief Parsear parámetros de macro de función
     */
    static std::vector<std::string> parseMacroParameters(const std::vector<lexer::Token>& tokens);

    /**
     * @brief Reemplazar parámetros en cuerpo de macro
     */
    static std::vector<lexer::Token> substituteParameters(
        const std::vector<lexer::Token>& body,
        const std::vector<std::string>& parameters,
        const std::vector<std::vector<lexer::Token>>& arguments);
};

} // namespace cpp20::compiler::frontend
