#pragma once

#include "SourceLocation.h"
#include <string>
#include <vector>
#include <memory>
#include <format>

namespace cpp20::compiler::diagnostics {

/**
 * @brief Nivel de severidad de un diagnóstico
 */
enum class DiagnosticLevel {
    Note,       // Información adicional, no es un error
    Warning,    // Advertencia, no impide compilación
    Error,      // Error que impide compilación
    Fatal       // Error fatal que detiene el compilador
};

/**
 * @brief Categoría del diagnóstico para organización y filtrado
 */
enum class DiagnosticCategory {
    Lexical,        // Errores del lexer/preprocesador
    Syntactic,      // Errores de sintaxis
    Semantic,       // Errores semánticos
    Template,       // Errores relacionados con plantillas
    Constexpr,      // Errores de evaluación constexpr
    Link,          // Errores de linking
    Optimization,   // Advertencias de optimización
    Deprecated,     // Uso de features deprecated
    Performance,    // Sugerencias de performance
    Portability     // Problemas de portabilidad
};

/**
 * @brief Código específico del diagnóstico
 *
 * Cada código identifica un tipo específico de problema que puede
 * ocurrir durante la compilación.
 */
enum class DiagnosticCode {
    // Errores léxicos (1000-1999)
    ERR_LEX_INVALID_CHARACTER = 1000,
    ERR_LEX_UNTERMINATED_STRING = 1001,
    ERR_LEX_INVALID_NUMBER = 1002,
    ERR_LEX_UNTERMINATED_COMMENT = 1003,

    // Errores sintácticos (2000-2999)
    ERR_SYN_EXPECTED_TOKEN = 2000,
    ERR_SYN_UNEXPECTED_TOKEN = 2001,
    ERR_SYN_MISSING_SEMICOLON = 2002,
    ERR_SYN_INVALID_DECLARATION = 2003,

    // Errores semánticos (3000-3999)
    ERR_SEM_UNDEFINED_SYMBOL = 3000,
    ERR_SEM_TYPE_MISMATCH = 3001,
    ERR_SEM_INVALID_CONVERSION = 3002,
    ERR_SEM_REDEFINITION = 3003,
    ERR_SEM_INVALID_OPERATION = 3004,

    // Errores de plantillas (4000-4999)
    ERR_TPL_INVALID_ARGUMENTS = 4000,
    ERR_TPL_AMBIGUOUS_SPECIALIZATION = 4001,
    ERR_TPL_RECURSION_DEPTH = 4002,
    ERR_TPL_INVALID_CONSTRAINT = 4003,

    // Errores constexpr (5000-5999)
    ERR_CONSTEXPR_NOT_CONSTANT = 5000,
    ERR_CONSTEXPR_INVALID_OPERATION = 5001,
    ERR_CONSTEXPR_RECURSION = 5002,

    // Advertencias (6000-6999)
    WARN_UNUSED_VARIABLE = 6000,
    WARN_IMPLICIT_CONVERSION = 6001,
    WARN_UNREACHABLE_CODE = 6002,
    WARN_PERFORMANCE = 6003,

    // Notas informativas (7000-7999)
    NOTE_PREVIOUS_DEFINITION = 7000,
    NOTE_CANDIDATE_FUNCTION = 7001,
    NOTE_TYPE_CONVERSION = 7002
};

/**
 * @brief Argumento de formato para diagnósticos
 *
 * Los diagnósticos pueden incluir argumentos que se formatean
 * en el mensaje final.
 */
class DiagnosticArgument {
public:
    enum class Type {
        String,
        Integer,
        Unsigned,
        Location,
        Range,
        Type,
        Symbol
    };

    DiagnosticArgument(std::string value) : type_(Type::String), stringValue_(std::move(value)) {}
    DiagnosticArgument(int64_t value) : type_(Type::Integer), intValue_(value) {}
    DiagnosticArgument(uint64_t value) : type_(Type::Unsigned), uintValue_(value) {}
    DiagnosticArgument(SourceLocation loc) : type_(Type::Location), locationValue_(loc) {}
    DiagnosticArgument(SourceRange range) : type_(Type::Range), rangeValue_(range) {}

    Type type() const { return type_; }

    const std::string& asString() const { return stringValue_; }
    int64_t asInteger() const { return intValue_; }
    uint64_t asUnsigned() const { return uintValue_; }
    const SourceLocation& asLocation() const { return locationValue_; }
    const SourceRange& asRange() const { return rangeValue_; }

private:
    Type type_;
    std::string stringValue_;
    int64_t intValue_ = 0;
    uint64_t uintValue_ = 0;
    SourceLocation locationValue_;
    SourceRange rangeValue_;
};

/**
 * @brief Representa un diagnóstico completo del compilador
 *
 * Un diagnóstico contiene toda la información necesaria para reportar
 * un problema o información al usuario: ubicación, severidad, código,
 * mensaje, y argumentos para formateo.
 */
class Diagnostic {
public:
    Diagnostic(
        DiagnosticLevel level,
        DiagnosticCode code,
        SourceLocation location,
        std::string message
    );

    // Getters
    DiagnosticLevel level() const { return level_; }
    DiagnosticCode code() const { return code_; }
    const SourceLocation& location() const { return location_; }
    const std::string& message() const { return message_; }
    const std::vector<DiagnosticArgument>& arguments() const { return arguments_; }

    // Agregar argumentos
    void addArgument(DiagnosticArgument arg) {
        arguments_.push_back(std::move(arg));
    }

    // Utilidades
    bool isError() const {
        return level_ == DiagnosticLevel::Error || level_ == DiagnosticLevel::Fatal;
    }

    bool isWarning() const {
        return level_ == DiagnosticLevel::Warning;
    }

    bool isNote() const {
        return level_ == DiagnosticLevel::Note;
    }

    std::string formatMessage() const;
    DiagnosticCategory category() const;

    // Factory methods para diagnósticos comunes
    static Diagnostic error(DiagnosticCode code, SourceLocation loc, std::string msg);
    static Diagnostic warning(DiagnosticCode code, SourceLocation loc, std::string msg);
    static Diagnostic note(DiagnosticCode code, SourceLocation loc, std::string msg);
    static Diagnostic fatal(DiagnosticCode code, SourceLocation loc, std::string msg);

private:
    DiagnosticLevel level_;
    DiagnosticCode code_;
    SourceLocation location_;
    std::string message_;
    std::vector<DiagnosticArgument> arguments_;
};

/**
 * @brief Información de corrección sugerida para un diagnóstico
 */
class FixItHint {
public:
    enum class Action {
        Insert,    // Insertar texto
        Remove,    // Remover texto
        Replace    // Reemplazar texto
    };

    FixItHint(Action action, SourceRange range, std::string text = "")
        : action_(action), range_(range), text_(std::move(text)) {}

    Action action() const { return action_; }
    const SourceRange& range() const { return range_; }
    const std::string& text() const { return text_; }

private:
    Action action_;
    SourceRange range_;
    std::string text_;
};

} // namespace cpp20::compiler::diagnostics
