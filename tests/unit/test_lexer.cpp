/**
 * @file test_lexer.cpp
 * @brief Tests unitarios para el lexer C++20
 */

#include <gtest/gtest.h>
#include <compiler/frontend/lexer/Lexer.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>

using namespace cpp20::compiler::frontend::lexer;
using namespace cpp20::compiler::diagnostics;

// Test para inicialización del lexer
TEST(LexerTest, BasicInitialization) {
    DiagnosticEngine diagEngine;
    LexerConfig config;
    Lexer lexer("", diagEngine, config);

    EXPECT_FALSE(lexer.hasMoreTokens());

    auto tokens = lexer.tokenize();
    EXPECT_EQ(tokens.size(), 1); // Solo EOF
    EXPECT_EQ(tokens[0].getType(), TokenType::END_OF_FILE);
}

// Test para tokenización básica
TEST(LexerTest, BasicTokenization) {
    DiagnosticEngine diagEngine;
    std::string source = "int main() { return 0; }";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que se generaron tokens
    EXPECT_GT(tokens.size(), 1);

    // Verificar algunos tokens específicos
    EXPECT_EQ(tokens[0].getType(), TokenType::INT);
    EXPECT_EQ(tokens[1].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].getType(), TokenType::LEFT_PAREN);
    EXPECT_EQ(tokens[3].getType(), TokenType::RIGHT_PAREN);
    EXPECT_EQ(tokens[4].getType(), TokenType::LEFT_BRACE);
}

// Test para identificadores
TEST(LexerTest, Identifiers) {
    DiagnosticEngine diagEngine;
    std::string source = "variable _private __system myVar123";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 4);

    EXPECT_EQ(tokens[0].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].getLexeme(), "variable");

    EXPECT_EQ(tokens[1].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].getLexeme(), "_private");

    EXPECT_EQ(tokens[2].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].getLexeme(), "__system");

    EXPECT_EQ(tokens[3].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].getLexeme(), "myVar123");
}

// Test para palabras clave
TEST(LexerTest, Keywords) {
    DiagnosticEngine diagEngine;
    std::string source = "int void char if else while for return";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 7);

    EXPECT_EQ(tokens[0].getType(), TokenType::INT);
    EXPECT_EQ(tokens[1].getType(), TokenType::VOID);
    EXPECT_EQ(tokens[2].getType(), TokenType::CHAR);
    EXPECT_EQ(tokens[3].getType(), TokenType::IF);
    EXPECT_EQ(tokens[4].getType(), TokenType::ELSE);
    EXPECT_EQ(tokens[5].getType(), TokenType::WHILE);
    EXPECT_EQ(tokens[6].getType(), TokenType::FOR);
    EXPECT_EQ(tokens[7].getType(), TokenType::RETURN);
}

// Test para literales enteros
TEST(LexerTest, IntegerLiterals) {
    DiagnosticEngine diagEngine;
    std::string source = "42 0xFF 077 0b1010 123ULL";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 5);

    EXPECT_EQ(tokens[0].getType(), TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[0].getLexeme(), "42");

    EXPECT_EQ(tokens[1].getType(), TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[1].getLexeme(), "0xFF");

    EXPECT_EQ(tokens[2].getType(), TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[2].getLexeme(), "077");

    EXPECT_EQ(tokens[3].getType(), TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[3].getLexeme(), "0b1010");

    EXPECT_EQ(tokens[4].getType(), TokenType::INTEGER_LITERAL);
    EXPECT_EQ(tokens[4].getLexeme(), "123ULL");
}

// Test para literales flotantes
TEST(LexerTest, FloatLiterals) {
    DiagnosticEngine diagEngine;
    std::string source = "3.14 2.5f 1.23L 4.56e-2";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 4);

    EXPECT_EQ(tokens[0].getType(), TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[0].getLexeme(), "3.14");

    EXPECT_EQ(tokens[1].getType(), TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[1].getLexeme(), "2.5f");

    EXPECT_EQ(tokens[2].getType(), TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[2].getLexeme(), "1.23L");

    EXPECT_EQ(tokens[3].getType(), TokenType::FLOAT_LITERAL);
    EXPECT_EQ(tokens[3].getLexeme(), "4.56e-2");
}

// Test para literales de caracter
TEST(LexerTest, CharacterLiterals) {
    DiagnosticEngine diagEngine;
    std::string source = "'a' L'b' u'c' U'd' '\\n' '\\x41'";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 6);

    EXPECT_EQ(tokens[0].getType(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[1].getType(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[2].getType(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[3].getType(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[4].getType(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[5].getType(), TokenType::CHAR_LITERAL);
}

// Test para literales de string
TEST(LexerTest, StringLiterals) {
    DiagnosticEngine diagEngine;
    std::string source = "\"hello\" L\"world\" u8\"text\" U\"unicode\"";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 4);

    EXPECT_EQ(tokens[0].getType(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[1].getType(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].getType(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[3].getType(), TokenType::STRING_LITERAL);
}

// Test para operadores
TEST(LexerTest, Operators) {
    DiagnosticEngine diagEngine;
    std::string source = "+ - * / % ++ -- == != < > <= >= && || ! & | ^ ~ << >> = += -= *= /= %= &= |= ^= <<= >>=";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que se reconocieron los operadores
    EXPECT_GT(tokens.size(), 20);

    // Verificar algunos operadores específicos
    bool foundPlus = false, foundMinus = false, foundEqual = false;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::PLUS) foundPlus = true;
        if (token.getType() == TokenType::MINUS) foundMinus = true;
        if (token.getType() == TokenType::ASSIGN) foundEqual = true;
    }

    EXPECT_TRUE(foundPlus);
    EXPECT_TRUE(foundMinus);
    EXPECT_TRUE(foundEqual);
}

// Test para puntuación
TEST(LexerTest, Punctuation) {
    DiagnosticEngine diagEngine;
    std::string source = "();[]{},.;:?";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 10);

    EXPECT_EQ(tokens[0].getType(), TokenType::LEFT_PAREN);
    EXPECT_EQ(tokens[1].getType(), TokenType::RIGHT_PAREN);
    EXPECT_EQ(tokens[2].getType(), TokenType::SEMICOLON);
    EXPECT_EQ(tokens[3].getType(), TokenType::LEFT_BRACKET);
    EXPECT_EQ(tokens[4].getType(), TokenType::RIGHT_BRACKET);
    EXPECT_EQ(tokens[5].getType(), TokenType::LEFT_BRACE);
    EXPECT_EQ(tokens[6].getType(), TokenType::RIGHT_BRACE);
    EXPECT_EQ(tokens[7].getType(), TokenType::COMMA);
    EXPECT_EQ(tokens[8].getType(), TokenType::DOT);
    EXPECT_EQ(tokens[9].getType(), TokenType::SEMICOLON);
}

// Test para operadores de comparación C++20
TEST(LexerTest, SpaceshipOperator) {
    DiagnosticEngine diagEngine;
    std::string source = "a <=> b";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 3);

    EXPECT_EQ(tokens[0].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].getType(), TokenType::SPACESHIP);
    EXPECT_EQ(tokens[2].getType(), TokenType::IDENTIFIER);
}

// Test para palabras clave C++20
TEST(LexerTest, Cpp20Keywords) {
    DiagnosticEngine diagEngine;
    std::string source = "co_await co_return co_yield module import export concept requires";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 8);

    EXPECT_EQ(tokens[0].getType(), TokenType::CO_AWAIT);
    EXPECT_EQ(tokens[1].getType(), TokenType::CO_RETURN);
    EXPECT_EQ(tokens[2].getType(), TokenType::CO_YIELD);
    EXPECT_EQ(tokens[3].getType(), TokenType::MODULE);
    EXPECT_EQ(tokens[4].getType(), TokenType::IMPORT);
    EXPECT_EQ(tokens[5].getType(), TokenType::EXPORT);
    EXPECT_EQ(tokens[6].getType(), TokenType::CONCEPT);
    EXPECT_EQ(tokens[7].getType(), TokenType::REQUIRES);
}

// Test para literales booleanos y nullptr
TEST(LexerTest, BooleanAndNullptrLiterals) {
    DiagnosticEngine diagEngine;
    std::string source = "true false nullptr";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 3);

    EXPECT_EQ(tokens[0].getType(), TokenType::TRUE_LITERAL);
    EXPECT_EQ(tokens[1].getType(), TokenType::FALSE_LITERAL);
    EXPECT_EQ(tokens[2].getType(), TokenType::NULLPTR_LITERAL);
}

// Test para secuencias de escape
TEST(LexerTest, EscapeSequences) {
    DiagnosticEngine diagEngine;
    std::string source = "'\\n' '\\t' '\\'' '\\\"' '\\\\' '\\x41' '\\u0041' '\\U00000041'";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que se reconocieron los literales de caracter con escape
    ASSERT_GE(tokens.size(), 8);

    for (const auto& token : tokens) {
        EXPECT_EQ(token.getType(), TokenType::CHAR_LITERAL);
    }
}

// Test para concatenación de líneas
TEST(LexerTest, LineConcatenation) {
    DiagnosticEngine diagEngine;
    std::string source = "int main() { \\\n    return 0; \\\n}";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que la concatenación funcionó
    EXPECT_GT(tokens.size(), 1);

    // Debería encontrar 'int', 'main', '(', ')', '{', 'return', '0', ';', '}'
    bool foundInt = false, foundMain = false, foundReturn = false;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::INT) foundInt = true;
        if (token.getLexeme() == "main") foundMain = true;
        if (token.getType() == TokenType::RETURN) foundReturn = true;
    }

    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundMain);
    EXPECT_TRUE(foundReturn);
}

// Test para eliminación de espacios en blanco y comentarios
TEST(LexerTest, WhitespaceAndComments) {
    DiagnosticEngine diagEngine;
    std::string source = "int   main()  // comentario\n{  /* otro\n   comentario */  return    0;  }";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que los espacios en blanco y comentarios fueron eliminados
    EXPECT_GT(tokens.size(), 1);

    // Verificar secuencia correcta de tokens
    EXPECT_EQ(tokens[0].getType(), TokenType::INT);
    EXPECT_EQ(tokens[1].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].getLexeme(), "main");
}

// Test para estadísticas del lexer
TEST(LexerTest, LexerStatistics) {
    DiagnosticEngine diagEngine;
    std::string source = "/* comentario */\nint main() {\n    return 0;\n}\n// otro comentario";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();
    auto stats = lexer.getStats();

    EXPECT_GT(stats.totalCharacters, 0);
    EXPECT_GT(stats.totalLines, 1);
    EXPECT_GT(stats.totalTokens, 1);
    EXPECT_EQ(stats.commentLines, 2); // Dos líneas de comentario
    EXPECT_EQ(stats.errorCount, 0);   // Sin errores
}

// Test para manejo de errores
TEST(LexerTest, ErrorHandling) {
    DiagnosticEngine diagEngine;
    std::string source = "int main() { return @; }"; // @ es un carácter inválido
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();
    auto stats = lexer.getStats();

    // Debería haber un error reportado
    EXPECT_EQ(stats.errorCount, 1);
    EXPECT_GT(tokens.size(), 1); // Debería continuar procesando después del error
}

// Test para string vacío
TEST(LexerTest, EmptyString) {
    DiagnosticEngine diagEngine;
    std::string source = "";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].getType(), TokenType::END_OF_FILE);
}

// Test para solo espacios en blanco
TEST(LexerTest, OnlyWhitespace) {
    DiagnosticEngine diagEngine;
    std::string source = "   \n\t  \n  ";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].getType(), TokenType::END_OF_FILE);
}

// Test para números con sufijos complejos
TEST(LexerTest, ComplexNumberSuffixes) {
    DiagnosticEngine diagEngine;
    std::string source = "123u 456l 789ul 101112LL 131415ull 161718.5f 192021.0L";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 7);

    // Todos deberían ser literales válidos
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_TRUE(tokens[i].getType() == TokenType::INTEGER_LITERAL ||
                   tokens[i].getType() == TokenType::FLOAT_LITERAL);
    }
}

// Test para identificadores con caracteres Unicode (simulado)
TEST(LexerTest, UnicodeIdentifiers) {
    DiagnosticEngine diagEngine;
    std::string source = "variable_normal _con_guion _doble_guion";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 3);

    EXPECT_EQ(tokens[0].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].getType(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].getType(), TokenType::IDENTIFIER);
}

// Test para operadores compuestos
TEST(LexerTest, CompoundOperators) {
    DiagnosticEngine diagEngine;
    std::string source = "a+=b a-=c a*=d a/=e a%=f a&=g a|=h a^=i a<<=j a>>=k a==b a!=c a<=d a>=e a&&f a||g a++ a-- ++a --a";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    // Verificar que se reconocieron correctamente los operadores compuestos
    EXPECT_GT(tokens.size(), 20);

    // Contar operadores de asignación compuestos
    int compoundAssignments = 0;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::PLUS_ASSIGN ||
            token.getType() == TokenType::MINUS_ASSIGN ||
            token.getType() == TokenType::MUL_ASSIGN ||
            token.getType() == TokenType::DIV_ASSIGN ||
            token.getType() == TokenType::MOD_ASSIGN ||
            token.getType() == TokenType::AND_ASSIGN ||
            token.getType() == TokenType::OR_ASSIGN ||
            token.getType() == TokenType::XOR_ASSIGN ||
            token.getType() == TokenType::LEFT_SHIFT_ASSIGN ||
            token.getType() == TokenType::RIGHT_SHIFT_ASSIGN) {
            compoundAssignments++;
        }
    }

    EXPECT_EQ(compoundAssignments, 10);
}

// Test para ámbito (::)
TEST(LexerTest, ScopeResolution) {
    DiagnosticEngine diagEngine;
    std::string source = "std::cout ::global ns::func";
    Lexer lexer(source, diagEngine);

    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 7);

    // Verificar que se reconocieron los operadores ::
    bool foundScope = false;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::SCOPE_RESOLUTION) {
            foundScope = true;
            break;
        }
    }

    EXPECT_TRUE(foundScope);
}

// Test para configuración del lexer
TEST(LexerTest, LexerConfiguration) {
    DiagnosticEngine diagEngine;

    // Configuración por defecto
    LexerConfig config;
    EXPECT_TRUE(config.enableUnicodeSupport);
    EXPECT_TRUE(config.enableRawStrings);
    EXPECT_TRUE(config.enableUserDefinedLiterals);
    EXPECT_TRUE(config.enableModules);
    EXPECT_TRUE(config.enableCoroutines);
    EXPECT_TRUE(config.enableConcepts);
    EXPECT_FALSE(config.preserveComments);

    // Lexer con configuración personalizada
    config.preserveComments = true;
    Lexer lexer("", diagEngine, config);

    auto tokens = lexer.tokenize();
    EXPECT_EQ(tokens.size(), 1); // Solo EOF
}

// Test para procesamiento de fases del lexer
TEST(LexerTest, LexerPhasesProcessing) {
    DiagnosticEngine diagEngine;
    std::string source = "/* comment */ int main() { return 0; } // line comment";
    Lexer lexer(source, diagEngine);

    // Ejecutar tokenización completa (todas las fases)
    auto tokens = lexer.tokenize();

    // Verificar que los comentarios fueron eliminados
    bool foundComment = false;
    for (const auto& token : tokens) {
        if (token.getType() == TokenType::INVALID) {
            foundComment = true;
            break;
        }
    }

    EXPECT_FALSE(foundComment); // No debería haber tokens de comentario

    // Verificar estadísticas
    auto stats = lexer.getStats();
    EXPECT_EQ(stats.commentLines, 2); // Dos líneas de comentario
    EXPECT_EQ(stats.errorCount, 0);
}
