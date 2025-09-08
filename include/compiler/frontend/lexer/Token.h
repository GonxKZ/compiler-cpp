/**
 * @file Token.h
 * @brief Definición de tokens para el lexer C++20
 */

#pragma once

#include <string>
#include <memory>
#include <compiler/common/diagnostics/SourceLocation.h>

namespace cpp20::compiler::frontend::lexer {

/**
 * @brief Tipos de tokens para C++20
 */
enum class TokenType {
    // === TOKENS ESPECIALES ===
    END_OF_FILE,           // Fin de archivo
    INVALID,              // Token inválido

    // === LITERALES ===
    // Literales enteros
    INTEGER_LITERAL,      // 123, 0xFF, 077, 0b1010
    // Literales flotantes
    FLOAT_LITERAL,        // 1.23, 1.23f, 1.23L
    // Literales de caracter
    CHAR_LITERAL,         // 'a', L'b', u'c', U'd'
    // Literales de string
    STRING_LITERAL,       // "hello", L"world", u8"text", U"unicode"
    // Literales booleanos
    TRUE_LITERAL,         // true
    FALSE_LITERAL,        // false
    // Literales null pointer
    NULLPTR_LITERAL,      // nullptr

    // === IDENTIFICADORES Y PALABRAS CLAVE ===
    IDENTIFIER,           // nombre, _private, __system

    // Palabras clave C++20
    // Tipos fundamentales
    VOID, INT, CHAR, SHORT, LONG, FLOAT, DOUBLE, BOOL,
    // Calificadores
    CONST, VOLATILE, CONSTEVAL, CONSTEXPR, CONSTINIT, MUTABLE,
    // Especificadores de clase de almacenamiento
    STATIC, EXTERN, INLINE, THREAD_LOCAL,
    // Control de acceso
    PUBLIC, PRIVATE, PROTECTED,
    // Estructuras de control
    IF, ELSE, WHILE, FOR, DO, SWITCH, CASE, DEFAULT,
    BREAK, CONTINUE, RETURN, GOTO,
    // Estructuras de datos
    STRUCT, CLASS, UNION, ENUM,
    // Funciones y métodos
    VIRTUAL, OVERRIDE, FINAL, NOEXCEPT,
    // Excepciones
    TRY, CATCH, THROW,
    // Templates y conceptos
    TEMPLATE, TYPENAME, CONCEPT, REQUIRES,
    // Espacios de nombres
    NAMESPACE, USING,
    // Operadores y conversión
    OPERATOR, EXPLICIT,
    // Misceláneo
    SIZEOF, ALIGNOF, ALIGNAS, TYPEID, DECLTYPE, AUTO,
    // C++20 específico
    CO_AWAIT, CO_RETURN, CO_YIELD,
    MODULE, IMPORT, EXPORT,

    // === OPERADORES Y PUNTUACIÓN ===
    // Operadores aritméticos
    PLUS, MINUS, STAR, SLASH, PERCENT,      // +, -, *, /, %
    INCREMENT, DECREMENT,                    // ++, --

    // Operadores de comparación
    EQUAL, NOT_EQUAL,                        // ==, !=
    LESS, GREATER, LESS_EQUAL, GREATER_EQUAL, // <, >, <=, >=
    SPACESHIP,                               // <=> (C++20)

    // Operadores lógicos
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,    // &&, ||, !

    // Operadores bit a bit
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT,       // &, |, ^, ~
    LEFT_SHIFT, RIGHT_SHIFT,                 // <<, >>

    // Operadores de asignación
    ASSIGN,                                  // =
    PLUS_ASSIGN, MINUS_ASSIGN,               // +=, -=
    MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,      // *=, /=, %=
    AND_ASSIGN, OR_ASSIGN, XOR_ASSIGN,       // &=, |=, ^=
    LEFT_SHIFT_ASSIGN, RIGHT_SHIFT_ASSIGN,   // <<=, >>=

    // Operadores misceláneos
    ARROW, ARROW_STAR,                       // ->, ->*
    DOT, DOT_STAR,                           // ., .*
    QUESTION, COLON,                         // ?, :
    SEMICOLON, COMMA,                        // ;, ,

    // Paréntesis y brackets
    LEFT_PAREN, RIGHT_PAREN,                 // (, )
    LEFT_BRACKET, RIGHT_BRACKET,             // [, ]
    LEFT_BRACE, RIGHT_BRACE,                 // {, }

    // Preprocesador
    HASH, HASH_HASH,                         // #, ##

    // Otros
    ELLIPSIS,                                // ...
    SCOPE_RESOLUTION,                        // ::

    // === TOKENS ESPECIALES C++20 ===
    // Literales de usuario
    USER_DEFINED_INTEGER_LITERAL,
    USER_DEFINED_FLOAT_LITERAL,
    USER_DEFINED_CHAR_LITERAL,
    USER_DEFINED_STRING_LITERAL,

    // Operadores de comparación C++20
    THREE_WAY_COMPARISON,                    // <=>

    // Módulos C++20
    MODULE_KEYWORD, IMPORT_KEYWORD, EXPORT_KEYWORD,

    // Corrutinas C++20
    CO_AWAIT_KEYWORD, CO_RETURN_KEYWORD, CO_YIELD_KEYWORD,

    // Conceptos C++20
    CONCEPT_KEYWORD, REQUIRES_KEYWORD
};

/**
 * @brief Representa un token léxico en el código fuente
 *
 * Un token es la unidad básica de análisis léxico que contiene
 * información sobre un elemento sintáctico identificado en el código.
 */
class Token {
public:
    /**
     * @brief Constructor principal
     * @param type Tipo del token (keyword, identifier, literal, etc.)
     * @param lexeme Texto original del token en el código fuente
     * @param location Ubicación del token en el archivo fuente
     * @param value Valor semántico adicional (opcional)
     */
    Token(TokenType type, const std::string& lexeme,
          const diagnostics::SourceLocation& location,
          const std::string& value = "");

    /**
     * @brief Destructor virtual para permitir herencia
     */
    virtual ~Token();

    /**
     * @brief Constructor de copia generado por defecto
     * Necesario debido al destructor virtual personalizado
     */
    Token(const Token&) = default;

    /**
     * @brief Obtener tipo del token
     */
    TokenType getType() const { return type_; }

    /**
     * @brief Obtener lexema del token
     */
    const std::string& getLexeme() const { return lexeme_; }

    /**
     * @brief Obtener ubicación del token
     */
    const diagnostics::SourceLocation& getLocation() const { return location_; }

    /**
     * @brief Obtener valor semántico del token
     */
    const std::string& getValue() const { return value_; }

    /**
     * @brief Verificar si es un token de palabra clave
     */
    bool isKeyword() const;

    /**
     * @brief Verificar si es un token de operador
     */
    bool isOperator() const;

    /**
     * @brief Verificar si es un token de literal
     */
    bool isLiteral() const;

    /**
     * @brief Verificar si es un token de puntuación
     */
    bool isPunctuation() const;

    /**
     * @brief Obtener representación string del token
     */
    std::string toString() const;

    /**
     * @brief Verificar si el token es válido
     */
    bool isValid() const { return type_ != TokenType::INVALID; }

private:
    TokenType type_;                    // Tipo del token
    std::string lexeme_;               // Texto original del token
    diagnostics::SourceLocation location_; // Ubicación en el código fuente
    std::string value_;                // Valor semántico (para literales)
};

/**
 * @brief Utilidades para trabajar con tokens
 */
class TokenUtils {
public:
    /**
     * @brief Convertir TokenType a string
     */
    static std::string tokenTypeToString(TokenType type);

    /**
     * @brief Verificar si un identificador es una palabra clave
     */
    static TokenType getKeywordType(const std::string& identifier);

    /**
     * @brief Obtener precedencia de operador
     */
    static int getOperatorPrecedence(TokenType type);

    /**
     * @brief Verificar si es operador unario
     */
    static bool isUnaryOperator(TokenType type);

    /**
     * @brief Verificar si es operador binario
     */
    static bool isBinaryOperator(TokenType type);

    /**
     * @brief Verificar si es operador de asignación
     */
    static bool isAssignmentOperator(TokenType type);
};

} // namespace cpp20::compiler::frontend::lexer