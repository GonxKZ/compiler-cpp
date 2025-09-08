/**
 * @file Lexer.h
 * @brief Lexer para C++20 que implementa las fases de traducción
 */

#pragma once

#include <compiler/frontend/lexer/Token.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <string>
#include <vector>
#include <memory>

namespace cpp20::compiler::frontend::lexer {

/**
 * @brief Configuración del lexer
 */
struct LexerConfig {
    bool enableUnicodeSupport = true;     // Soporte para UCNs
    bool enableRawStrings = true;         // Soporte para raw strings
    bool enableUserDefinedLiterals = true; // Literales definidos por usuario
    bool enableModules = true;            // Soporte para módulos C++20
    bool enableCoroutines = true;         // Soporte para corrutinas C++20
    bool enableConcepts = true;           // Soporte para conceptos C++20
    bool preserveComments = false;        // Preservar comentarios en tokens
};

/**
 * @brief Estado del lexer durante el análisis
 */
struct LexerState {
    size_t position = 0;                  // Posición actual en el código fuente
    size_t line = 1;                      // Línea actual
    size_t column = 1;                    // Columna actual
    bool inRawString = false;             // Si estamos en un raw string
    std::string rawStringDelimiter;       // Delimitador del raw string
    bool inBlockComment = false;          // Si estamos en comentario de bloque
};

/**
 * @brief Lexer para C++20 que implementa las fases de traducción
 *
 * El lexer sigue las fases de traducción especificadas en [lex.phases]:
 * 1. Codificación física de caracteres
 * 2. Eliminación de caracteres de control
 * 3. Concatenación de líneas
 * 4. Reemplazo de trígrafos
 * 5. Eliminación de espacios en blanco y comentarios
 * 6. Tokenización
 */
class Lexer {
public:
    /**
     * @brief Constructor
     */
    Lexer(const std::string& source, diagnostics::DiagnosticEngine& diagEngine,
          const LexerConfig& config = LexerConfig());

    /**
     * @brief Destructor
     */
    ~Lexer();

    /**
     * @brief Ejecutar todas las fases de traducción y obtener tokens
     */
    std::vector<Token> tokenize();

    /**
     * @brief Obtener siguiente token sin consumir
     */
    Token peekNextToken();

    /**
     * @brief Obtener y consumir siguiente token
     */
    Token getNextToken();

    /**
     * @brief Verificar si hay más tokens disponibles
     */
    bool hasMoreTokens() const;

    /**
     * @brief Reiniciar el lexer
     */
    void reset();

    /**
     * @brief Obtener estadísticas del análisis léxico
     */
    struct LexerStats {
        size_t totalCharacters = 0;
        size_t totalLines = 0;
        size_t totalTokens = 0;
        size_t commentLines = 0;
        size_t errorCount = 0;
    };
    LexerStats getStats() const { return stats_; }

private:
    std::string source_;                  // Código fuente original
    diagnostics::DiagnosticEngine& diagEngine_; // Motor de diagnósticos
    LexerConfig config_;                  // Configuración del lexer
    LexerState state_;                    // Estado actual del lexer
    LexerStats stats_;                    // Estadísticas del análisis

    std::vector<Token> tokens_;           // Tokens generados
    size_t currentTokenIndex_ = 0;        // Índice del token actual

    // === FASES DE TRADUCCIÓN ===

    /**
     * @brief Fase 1: Codificación física de caracteres
     * (Asumimos que ya está en UTF-8)
     */
    void phase1_PhysicalSource();

    /**
     * @brief Fase 2: Eliminación de caracteres de control
     */
    void phase2_RemoveControlChars();

    /**
     * @brief Fase 3: Concatenación de líneas
     */
    void phase3_LineConcatenation();

    /**
     * @brief Fase 4: Reemplazo de trígrafos
     */
    void phase4_TrigraphReplacement();

    /**
     * @brief Fase 5: Eliminación de espacios en blanco y comentarios
     */
    void phase5_RemoveWhitespaceAndComments();

    /**
     * @brief Fase 6: Tokenización
     */
    void phase6_Tokenization();

    // === FUNCIONES DE TOKENIZACIÓN ===

    /**
     * @brief Obtener siguiente caracter sin consumir
     */
    char peekChar() const;

    /**
     * @brief Obtener y consumir siguiente caracter
     */
    char getChar();

    /**
     * @brief Retroceder un caracter
     */
    void ungetChar();

    /**
     * @brief Verificar si hemos llegado al final
     */
    bool isAtEnd() const;

    /**
     * @brief Crear ubicación actual
     */
    diagnostics::SourceLocation currentLocation() const;

    /**
     * @brief Avanzar posición y actualizar línea/columna
     */
    void advancePosition(char c);

    /**
     * @brief Tokenizar identificadores y palabras clave
     */
    Token tokenizeIdentifier();

    /**
     * @brief Tokenizar literales numéricos
     */
    Token tokenizeNumber();

    /**
     * @brief Tokenizar literales de caracter
     */
    Token tokenizeCharacterLiteral();

    /**
     * @brief Tokenizar literales de string
     */
    Token tokenizeStringLiteral();

    /**
     * @brief Tokenizar operadores y puntuación
     */
    Token tokenizeOperatorOrPunctuation();

    /**
     * @brief Manejar secuencias de escape
     */
    std::string handleEscapeSequence();

    /**
     * @brief Verificar si es dígito
     */
    bool isDigit(char c) const;

    /**
     * @brief Verificar si es letra o guión bajo
     */
    bool isAlpha(char c) const;

    /**
     * @brief Verificar si es alfanumérico
     */
    bool isAlnum(char c) const;

    /**
     * @brief Verificar si es espacio en blanco
     */
    bool isWhitespace(char c) const;

    /**
     * @brief Reportar error léxico
     */
    void reportError(const std::string& message, const diagnostics::SourceLocation& location);

    /**
     * @brief Crear token
     */
    Token createToken(TokenType type, const std::string& lexeme = "",
                     const std::string& value = "");

    // === MANEJO DE LITERALES ===

    /**
     * @brief Procesar sufijos de literales enteros
     */
    std::pair<std::string, TokenType> processIntegerSuffix(const std::string& literal);

    /**
     * @brief Procesar sufijos de literales flotantes
     */
    std::pair<std::string, TokenType> processFloatSuffix(const std::string& literal);

    /**
     * @brief Verificar si es literal definido por usuario
     */
    bool isUserDefinedLiteral(const std::string& literal) const;

    // === MANEJO DE STRINGS RAW ===

    /**
     * @brief Procesar raw string literal
     */
    Token processRawString();

    /**
     * @brief Extraer delimitador de raw string
     */
    std::string extractRawStringDelimiter();

    // === MANEJO DE UCNs ===

    /**
     * @brief Procesar Universal Character Names
     */
    std::string processUCN(const std::string& ucn);

    // === MANEJO DE COMENTARIOS ===

    /**
     * @brief Saltar comentario de línea
     */
    void skipLineComment();

    /**
     * @brief Saltar comentario de bloque
     */
    void skipBlockComment();
};

/**
 * @brief Utilidades para el análisis léxico
 */
class LexerUtils {
public:
    /**
     * @brief Verificar si un caracter es válido en identificadores
     */
    static bool isValidIdentifierChar(char c, bool firstChar = false);

    /**
     * @brief Verificar si una secuencia es un dígito válido
     */
    static bool isValidDigit(char c, int base = 10);

    /**
     * @brief Convertir dígito a valor numérico
     */
    static int digitValue(char c);

    /**
     * @brief Verificar si es palabra clave C++20
     */
    static bool isCpp20Keyword(const std::string& word);

    /**
     * @brief Normalizar nueva línea
     */
    static std::string normalizeNewlines(const std::string& source);
};

} // namespace cpp20::compiler::frontend::lexer