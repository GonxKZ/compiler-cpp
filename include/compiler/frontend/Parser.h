/**
 * @file Parser.h
 * @brief Parser para C++20 con análisis sintáctico
 */

#pragma once

#include <compiler/frontend/lexer/Token.h>
#include <compiler/ast/ASTNode.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <vector>
#include <memory>

namespace cpp20::compiler::frontend {

/**
 * @brief Configuración del parser
 */
struct ParserConfig {
    bool enableTentativeParsing = true;   // Parsing tentativo para ambigüedades
    bool enableSemanticAnalysis = true;   // Análisis semántico durante parsing
    bool enableErrorRecovery = true;      // Recuperación de errores
    size_t maxLookahead = 3;              // Máximo lookahead para decisiones
};

/**
 * @brief Parser para C++20
 *
 * Implementa un parser descendente recursivo con:
 * - Tabla de precedencias para expresiones
 * - Parsing tentativo para ambigüedades
 * - Recuperación de errores
 * - Análisis semántico integrado
 */
class Parser {
public:
    /**
     * @brief Constructor
     */
    Parser(const std::vector<lexer::Token>& tokens,
           diagnostics::DiagnosticEngine& diagEngine,
           const ParserConfig& config = ParserConfig());

    /**
     * @brief Destructor
     */
    ~Parser();

    /**
     * @brief Parsear tokens en AST
     */
    std::unique_ptr<ast::TranslationUnit> parse();

    /**
     * @brief Verificar si el parsing fue exitoso
     */
    bool isSuccessful() const { return success_; }

    /**
     * @brief Obtener estadísticas del parsing
     */
    struct ParserStats {
        size_t tokensConsumed = 0;
        size_t nodesCreated = 0;
        size_t errorsReported = 0;
        size_t tentativeParses = 0;
        size_t errorRecoveries = 0;
    };
    ParserStats getStats() const { return stats_; }

private:
    std::vector<lexer::Token> tokens_;    // Tokens de entrada
    diagnostics::DiagnosticEngine& diagEngine_; // Motor de diagnósticos
    ParserConfig config_;                 // Configuración
    ParserStats stats_;                   // Estadísticas

    size_t currentTokenIndex_ = 0;        // Índice del token actual
    bool success_ = true;                 // Si el parsing fue exitoso

    /**
     * @brief Obtener token actual
     */
    const lexer::Token& currentToken() const;

    /**
     * @brief Obtener token siguiente
     */
    const lexer::Token& peekToken(size_t offset = 1) const;

    /**
     * @brief Consumir token actual
     */
    const lexer::Token& consumeToken();

    /**
     * @brief Verificar tipo de token actual
     */
    bool checkToken(lexer::TokenType type) const;

    /**
     * @brief Verificar y consumir token
     */
    bool matchToken(lexer::TokenType type);

    /**
     * @brief Verificar si estamos al final
     */
    bool isAtEnd() const;

    /**
     * @brief Reportar error de parsing
     */
    void reportError(const std::string& message, const diagnostics::SourceLocation& location);

    /**
     * @brief Intentar recuperación de error
     */
    void recoverFromError();

    // === PARSING DE UNIDADES DE TRADUCCIÓN ===

    /**
     * @brief Parsear unidad de traducción
     */
    std::unique_ptr<ast::TranslationUnit> parseTranslationUnit();

    /**
     * @brief Parsear declaración externa
     */
    std::unique_ptr<ast::ASTNode> parseExternalDeclaration();

    // === PARSING DE DECLARACIONES ===

    /**
     * @brief Parsear declaración
     */
    std::unique_ptr<ast::ASTNode> parseDeclaration();

    /**
     * @brief Parsear declaración de función
     */
    std::unique_ptr<ast::ASTNode> parseFunctionDeclaration();

    /**
     * @brief Parsear declaración de variable
     */
    std::unique_ptr<ast::ASTNode> parseVariableDeclaration();

    /**
     * @brief Parsear especificadores de tipo
     */
    std::vector<std::string> parseTypeSpecifiers();

    /**
     * @brief Parsear declarador
     */
    std::unique_ptr<ast::ASTNode> parseDeclarator();

    /**
     * @brief Parsear lista de parámetros
     */
    std::vector<std::unique_ptr<ast::ASTNode>> parseParameterList();

    // === PARSING DE EXPRESIONES ===

    /**
     * @brief Parsear expresión
     */
    std::unique_ptr<ast::ASTNode> parseExpression();

    /**
     * @brief Parsear expresión de asignación
     */
    std::unique_ptr<ast::ASTNode> parseAssignmentExpression();

    /**
     * @brief Parsear expresión condicional
     */
    std::unique_ptr<ast::ASTNode> parseConditionalExpression();

    /**
     * @brief Parsear expresión lógica OR
     */
    std::unique_ptr<ast::ASTNode> parseLogicalOrExpression();

    /**
     * @brief Parsear expresión lógica AND
     */
    std::unique_ptr<ast::ASTNode> parseLogicalAndExpression();

    /**
     * @brief Parsear expresión OR bit a bit
     */
    std::unique_ptr<ast::ASTNode> parseBitwiseOrExpression();

    /**
     * @brief Parsear expresión XOR bit a bit
     */
    std::unique_ptr<ast::ASTNode> parseBitwiseXorExpression();

    /**
     * @brief Parsear expresión AND bit a bit
     */
    std::unique_ptr<ast::ASTNode> parseBitwiseAndExpression();

    /**
     * @brief Parsear expresión de igualdad
     */
    std::unique_ptr<ast::ASTNode> parseEqualityExpression();

    /**
     * @brief Parsear expresión relacional
     */
    std::unique_ptr<ast::ASTNode> parseRelationalExpression();

    /**
     * @brief Parsear expresión de desplazamiento
     */
    std::unique_ptr<ast::ASTNode> parseShiftExpression();

    /**
     * @brief Parsear expresión aditiva
     */
    std::unique_ptr<ast::ASTNode> parseAdditiveExpression();

    /**
     * @brief Parsear expresión multiplicativa
     */
    std::unique_ptr<ast::ASTNode> parseMultiplicativeExpression();

    /**
     * @brief Parsear expresión unaria
     */
    std::unique_ptr<ast::ASTNode> parseUnaryExpression();

    /**
     * @brief Parsear expresión primaria
     */
    std::unique_ptr<ast::ASTNode> parsePrimaryExpression();

    // === PARSING DE SENTENCIAS ===

    /**
     * @brief Parsear sentencia
     */
    std::unique_ptr<ast::ASTNode> parseStatement();

    /**
     * @brief Parsear bloque de sentencias
     */
    std::unique_ptr<ast::ASTNode> parseCompoundStatement();

    /**
     * @brief Parsear sentencia if
     */
    std::unique_ptr<ast::ASTNode> parseIfStatement();

    /**
     * @brief Parsear sentencia while
     */
    std::unique_ptr<ast::ASTNode> parseWhileStatement();

    /**
     * @brief Parsear sentencia for
     */
    std::unique_ptr<ast::ASTNode> parseForStatement();

    /**
     * @brief Parsear sentencia return
     */
    std::unique_ptr<ast::ASTNode> parseReturnStatement();

    /**
     * @brief Parsear sentencia expression
     */
    std::unique_ptr<ast::ASTNode> parseExpressionStatement();

    // === UTILIDADES DE PARSING ===

    /**
     * @brief Parsing tentativo (para ambigüedades)
     */
    template<typename Func>
    std::unique_ptr<ast::ASTNode> tentativeParse(Func parserFunc);

    /**
     * @brief Verificar precedencia de operadores
     */
    int getOperatorPrecedence(lexer::TokenType type) const;

    /**
     * @brief Verificar asociatividad de operadores
     */
    bool isRightAssociative(lexer::TokenType type) const;

    /**
     * @brief Crear nodo AST básico
     */
    template<typename T, typename... Args>
    std::unique_ptr<T> createASTNode(Args&&... args);

    /**
     * @brief Crear ubicación actual
     */
    diagnostics::SourceLocation currentLocation() const;
};

/**
 * @brief Utilidades para el parsing
 */
class ParserUtils {
public:
    /**
     * @brief Verificar si un token puede iniciar una declaración
     */
    static bool canStartDeclaration(const lexer::Token& token);

    /**
     * @brief Verificar si un token puede iniciar una expresión
     */
    static bool canStartExpression(const lexer::Token& token);

    /**
     * @brief Verificar si un token puede iniciar una sentencia
     */
    static bool canStartStatement(const lexer::Token& token);

    /**
     * @brief Obtener palabras clave de tipos
     */
    static bool isTypeKeyword(const std::string& word);

    /**
     * @brief Verificar si es operador de asignación
     */
    static bool isAssignmentOperator(lexer::TokenType type);

    /**
     * @brief Verificar si es operador binario
     */
    static bool isBinaryOperator(lexer::TokenType type);

    /**
     * @brief Verificar si es operador unario
     */
    static bool isUnaryOperator(lexer::TokenType type);
};

} // namespace cpp20::compiler::frontend
