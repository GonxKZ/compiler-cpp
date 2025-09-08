/**
 * @file test_parser.cpp
 * @brief Tests unitarios para el parser C++20
 */

#include <gtest/gtest.h>
#include <compiler/frontend/Parser.h>
#include <compiler/frontend/lexer/Lexer.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>

using namespace cpp20::compiler::frontend;
using namespace cpp20::compiler::diagnostics;

// Clase auxiliar para crear tokens de prueba
class ParserTestHelper {
public:
    static std::vector<lexer::Token> createTokens(const std::vector<std::tuple<lexer::TokenType, std::string>>& specs) {
        std::vector<lexer::Token> tokens;
        for (const auto& spec : specs) {
            tokens.emplace_back(std::get<0>(spec), std::get<1>(spec),
                              SourceLocation("", 1, 1));
        }
        return tokens;
    }

    static std::vector<lexer::Token> tokenizeSource(const std::string& source) {
        DiagnosticEngine diagEngine;
        lexer::Lexer lexer(source, diagEngine);
        return lexer.tokenize();
    }
};

// Test para inicialización del parser
TEST(ParserTest, BasicInitialization) {
    DiagnosticEngine diagEngine;
    std::vector<lexer::Token> tokens = {
        lexer::Token(lexer::TokenType::END_OF_FILE, "", SourceLocation("", 1, 1))
    };

    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    EXPECT_TRUE(ast != nullptr);

    auto stats = parser.getStats();
    EXPECT_EQ(stats.tokensConsumed, 1);
    EXPECT_EQ(stats.errorsReported, 0);
}

// Test para parsing de expresiones primarias
TEST(ParserTest, PrimaryExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "variable"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    auto stats = parser.getStats();
    EXPECT_EQ(stats.nodesCreated, 1); // TranslationUnit + Identifier
}

// Test para parsing de expresiones aritméticas
TEST(ParserTest, ArithmeticExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    auto stats = parser.getStats();
    EXPECT_GT(stats.nodesCreated, 1);
}

// Test para parsing de expresiones con paréntesis
TEST(ParserTest, ParenthesizedExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "y"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::STAR, "*"},
        {lexer::TokenType::IDENTIFIER, "z"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones condicionales
TEST(ParserTest, ConditionalExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "condition"},
        {lexer::TokenType::QUESTION, "?"},
        {lexer::TokenType::IDENTIFIER, "true_expr"},
        {lexer::TokenType::COLON, ":"},
        {lexer::TokenType::IDENTIFIER, "false_expr"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de literales
TEST(ParserTest, LiteralParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INTEGER_LITERAL, "42"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::FLOAT_LITERAL, "3.14"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::STRING_LITERAL, "\"hello\""},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones de asignación
TEST(ParserTest, AssignmentExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::ASSIGN, "="},
        {lexer::TokenType::IDENTIFIER, "y"},
        {lexer::TokenType::PLUS_ASSIGN, "+="},
        {lexer::TokenType::INTEGER_LITERAL, "10"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones lógicas
TEST(ParserTest, LogicalExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::LOGICAL_AND, "&&"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::LOGICAL_OR, "||"},
        {lexer::TokenType::IDENTIFIER, "c"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones de comparación
TEST(ParserTest, ComparisonExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::LESS, "<"},
        {lexer::TokenType::IDENTIFIER, "y"},
        {lexer::TokenType::EQUAL, "=="},
        {lexer::TokenType::IDENTIFIER, "z"},
        {lexer::TokenType::NOT_EQUAL, "!="},
        {lexer::TokenType::INTEGER_LITERAL, "0"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones unarias
TEST(ParserTest, UnaryExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::LOGICAL_NOT, "!"},
        {lexer::TokenType::IDENTIFIER, "condition"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::MINUS, "-"},
        {lexer::TokenType::IDENTIFIER, "value"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones con precedencia
TEST(ParserTest, PrecedenceExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::STAR, "*"},
        {lexer::TokenType::IDENTIFIER, "c"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    // a + (b * c) - la multiplicación tiene mayor precedencia
    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de declaraciones simples
TEST(ParserTest, SimpleDeclarationParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
    auto stats = parser.getStats();
    EXPECT_GT(stats.nodesCreated, 1);
}

// Test para parsing de declaraciones con inicialización
TEST(ParserTest, DeclarationWithInitializationParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::ASSIGN, "="},
        {lexer::TokenType::INTEGER_LITERAL, "42"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de declaraciones múltiples
TEST(ParserTest, MultipleDeclarationParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::COMMA, ","},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::ASSIGN, "="},
        {lexer::TokenType::INTEGER_LITERAL, "10"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de declaraciones de función
TEST(ParserTest, FunctionDeclarationParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "func"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "param"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de sentencias if
TEST(ParserTest, IfStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IF, "if"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "condition"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::IDENTIFIER, "stmt"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de sentencias if-else
TEST(ParserTest, IfElseStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IF, "if"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "condition"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::IDENTIFIER, "stmt1"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::ELSE, "else"},
        {lexer::TokenType::IDENTIFIER, "stmt2"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de sentencias while
TEST(ParserTest, WhileStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::WHILE, "while"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "condition"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::IDENTIFIER, "body"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de sentencias return
TEST(ParserTest, ReturnStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::RETURN, "return"},
        {lexer::TokenType::IDENTIFIER, "value"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de sentencias return sin expresión
TEST(ParserTest, ReturnVoidStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::RETURN, "return"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de bloques de sentencias
TEST(ParserTest, CompoundStatementParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::LEFT_BRACE, "{"},
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::RETURN, "return"},
        {lexer::TokenType::IDENTIFIER, "x"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::RIGHT_BRACE, "}"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing con errores de sintaxis
TEST(ParserTest, SyntaxErrorRecovery) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "x"},
        // Falta punto y coma
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    // Debería reportar error pero continuar
    EXPECT_FALSE(parser.isSuccessful());
    auto stats = parser.getStats();
    EXPECT_EQ(stats.errorsReported, 1);
}

// Test para parsing de expresiones complejas
TEST(ParserTest, ComplexExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::STAR, "*"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "c"},
        {lexer::TokenType::MINUS, "-"},
        {lexer::TokenType::IDENTIFIER, "d"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones con múltiples operadores
TEST(ParserTest, MultipleOperatorsExpressionParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::STAR, "*"},
        {lexer::TokenType::IDENTIFIER, "c"},
        {lexer::TokenType::MINUS, "-"},
        {lexer::TokenType::IDENTIFIER, "d"},
        {lexer::TokenType::SLASH, "/"},
        {lexer::TokenType::IDENTIFIER, "e"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    // a + (b * c) - (d / e)
    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de llamadas a función
TEST(ParserTest, FunctionCallParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "func"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "arg1"},
        {lexer::TokenType::COMMA, ","},
        {lexer::TokenType::IDENTIFIER, "arg2"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de llamadas a función sin argumentos
TEST(ParserTest, FunctionCallNoArgsParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "func"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones con operador spaceship (C++20)
TEST(ParserTest, SpaceshipOperatorParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::SPACESHIP, "<=>"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para parsing de expresiones con corutinas C++20
TEST(ParserTest, CoroutineParsing) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::CO_AWAIT, "co_await"},
        {lexer::TokenType::IDENTIFIER, "async_operation"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para estadísticas del parser
TEST(ParserTest, ParserStatistics) {
    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::INT, "int"},
        {lexer::TokenType::IDENTIFIER, "main"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::LEFT_BRACE, "{"},
        {lexer::TokenType::RETURN, "return"},
        {lexer::TokenType::INTEGER_LITERAL, "0"},
        {lexer::TokenType::SEMICOLON, ";"},
        {lexer::TokenType::RIGHT_BRACE, "}"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser parser(tokens, diagEngine);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());

    auto stats = parser.getStats();
    EXPECT_EQ(stats.tokensConsumed, 10);
    EXPECT_EQ(stats.errorsReported, 0);
    EXPECT_GT(stats.nodesCreated, 1);
}

// Test para configuración del parser
TEST(ParserTest, ParserConfiguration) {
    DiagnosticEngine diagEngine;

    ParserConfig config;
    config.enableTentativeParsing = true;
    config.enableSemanticAnalysis = false;
    config.enableErrorRecovery = true;
    config.maxLookahead = 5;

    auto tokens = ParserTestHelper::createTokens({
        {lexer::TokenType::END_OF_FILE, ""}
    });

    Parser parser(tokens, diagEngine, config);
    auto ast = parser.parse();

    EXPECT_TRUE(parser.isSuccessful());
}

// Test para utilidades del parser
TEST(ParserUtilsTest, DeclarationDetection) {
    using namespace cpp20::compiler::frontend;

    // Test para detectar declaraciones
    lexer::Token intToken(lexer::TokenType::INT, "int", SourceLocation("", 1, 1));
    EXPECT_TRUE(ParserUtils::canStartDeclaration(intToken));

    lexer::Token identToken(lexer::TokenType::IDENTIFIER, "variable", SourceLocation("", 1, 1));
    EXPECT_TRUE(ParserUtils::canStartExpression(identToken));
    EXPECT_FALSE(ParserUtils::canStartDeclaration(identToken));

    // Test para palabras clave de tipos
    EXPECT_TRUE(ParserUtils::isTypeKeyword("int"));
    EXPECT_TRUE(ParserUtils::isTypeKeyword("void"));
    EXPECT_TRUE(ParserUtils::isTypeKeyword("double"));
    EXPECT_FALSE(ParserUtils::isTypeKeyword("return"));
}

// Test para operadores
TEST(ParserUtilsTest, OperatorClassification) {
    using namespace cpp20::compiler::frontend;

    // Operadores de asignación
    EXPECT_TRUE(ParserUtils::isAssignmentOperator(lexer::TokenType::ASSIGN));
    EXPECT_TRUE(ParserUtils::isAssignmentOperator(lexer::TokenType::PLUS_ASSIGN));
    EXPECT_FALSE(ParserUtils::isAssignmentOperator(lexer::TokenType::PLUS));

    // Operadores binarios
    EXPECT_TRUE(ParserUtils::isBinaryOperator(lexer::TokenType::PLUS));
    EXPECT_TRUE(ParserUtils::isBinaryOperator(lexer::TokenType::MINUS));
    EXPECT_TRUE(ParserUtils::isBinaryOperator(lexer::TokenType::STAR));
    EXPECT_FALSE(ParserUtils::isBinaryOperator(lexer::TokenType::SEMICOLON));

    // Operadores unarios
    EXPECT_TRUE(ParserUtils::isUnaryOperator(lexer::TokenType::PLUS));
    EXPECT_TRUE(ParserUtils::isUnaryOperator(lexer::TokenType::MINUS));
    EXPECT_TRUE(ParserUtils::isUnaryOperator(lexer::TokenType::LOGICAL_NOT));
    EXPECT_FALSE(ParserUtils::isUnaryOperator(lexer::TokenType::ASSIGN));
}

// Test para casos extremos
TEST(ParserTest, EdgeCases) {
    // Test con solo EOF
    auto emptyTokens = ParserTestHelper::createTokens({
        {lexer::TokenType::END_OF_FILE, ""}
    });

    DiagnosticEngine diagEngine;
    Parser emptyParser(emptyTokens, diagEngine);
    auto emptyAst = emptyParser.parse();

    EXPECT_TRUE(emptyParser.isSuccessful());

    // Test con expresión muy compleja
    auto complexTokens = ParserTestHelper::createTokens({
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "a"},
        {lexer::TokenType::PLUS, "+"},
        {lexer::TokenType::IDENTIFIER, "b"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::QUESTION, "?"},
        {lexer::TokenType::LEFT_PAREN, "("},
        {lexer::TokenType::IDENTIFIER, "c"},
        {lexer::TokenType::STAR, "*"},
        {lexer::TokenType::IDENTIFIER, "d"},
        {lexer::TokenType::RIGHT_PAREN, ")"},
        {lexer::TokenType::COLON, ":"},
        {lexer::TokenType::IDENTIFIER, "e"},
        {lexer::TokenType::END_OF_FILE, ""}
    });

    Parser complexParser(complexTokens, diagEngine);
    auto complexAst = complexParser.parse();

    EXPECT_TRUE(complexParser.isSuccessful());
}
