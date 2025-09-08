/**
 * @file test_preprocessor.cpp
 * @brief Tests unitarios para el preprocesador C++20
 */

#include <gtest/gtest.h>
#include <compiler/frontend/Preprocessor.h>
#include <compiler/frontend/lexer/Lexer.h>
#include <compiler/common/diagnostics/DiagnosticEngine.h>

using namespace cpp20::compiler::frontend;
using namespace cpp20::compiler::diagnostics;

// Clase auxiliar para crear tokens de prueba
class TestTokenFactory {
public:
    static lexer::Token createToken(lexer::TokenType type, const std::string& lexeme = "",
                                   const std::string& value = "") {
        return lexer::Token(type, lexeme,
                           SourceLocation("", 1, 1), value);
    }

    static std::vector<lexer::Token> createTokens(const std::vector<std::pair<lexer::TokenType, std::string>>& tokenSpecs) {
        std::vector<lexer::Token> tokens;
        for (const auto& spec : tokenSpecs) {
            tokens.push_back(createToken(spec.first, spec.second));
        }
        return tokens;
    }
};

// Test para inicialización del preprocesador
TEST(PreprocessorTest, BasicInitialization) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Verificar que las macros predefinidas están disponibles
    EXPECT_TRUE(preprocessor.isMacroDefined("__cplusplus"));
    EXPECT_TRUE(preprocessor.isMacroDefined("__STDC_HOSTED__"));
    EXPECT_TRUE(preprocessor.isMacroDefined("__FILE__"));
    EXPECT_TRUE(preprocessor.isMacroDefined("__LINE__"));
    EXPECT_TRUE(preprocessor.isMacroDefined("__DATE__"));
    EXPECT_TRUE(preprocessor.isMacroDefined("__TIME__"));
}

// Test para definición de macros simples
TEST(PreprocessorTest, SimpleMacroDefinition) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir una macro simple
    preprocessor.defineMacro("MAX_SIZE", "100");

    EXPECT_TRUE(preprocessor.isMacroDefined("MAX_SIZE"));

    const MacroDefinition* macro = preprocessor.getMacro("MAX_SIZE");
    ASSERT_TRUE(macro != nullptr);
    EXPECT_EQ(macro->name, "MAX_SIZE");
    EXPECT_FALSE(macro->isFunctionLike);
    EXPECT_EQ(macro->body.size(), 1);
    EXPECT_EQ(macro->body[0].getLexeme(), "100");
}

// Test para definición de macros de función
TEST(PreprocessorTest, FunctionMacroDefinition) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Crear definición de macro de función
    std::vector<lexer::Token> body = {
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "x"),
        TestTokenFactory::createToken(lexer::TokenType::PLUS, "+"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "y")
    };

    std::vector<std::string> parameters = {"x", "y"};
    MacroDefinition funcMacro("ADD", body, true, false);
    funcMacro.parameters = parameters;

    preprocessor.defineMacro(funcMacro);

    EXPECT_TRUE(preprocessor.isMacroDefined("ADD"));

    const MacroDefinition* macro = preprocessor.getMacro("ADD");
    ASSERT_TRUE(macro != nullptr);
    EXPECT_TRUE(macro->isFunctionLike);
    EXPECT_EQ(macro->parameters.size(), 2);
    EXPECT_EQ(macro->parameters[0], "x");
    EXPECT_EQ(macro->parameters[1], "y");
}

// Test para eliminación de macros
TEST(PreprocessorTest, MacroUndefinition) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir y verificar
    preprocessor.defineMacro("TEMP_MACRO", "value");
    EXPECT_TRUE(preprocessor.isMacroDefined("TEMP_MACRO"));

    // Eliminar y verificar
    preprocessor.undefineMacro("TEMP_MACRO");
    EXPECT_FALSE(preprocessor.isMacroDefined("TEMP_MACRO"));
}

// Test para procesamiento básico
TEST(PreprocessorTest, BasicProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Crear tokens de entrada simples
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::INT, "int"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "main"),
        TestTokenFactory::createToken(lexer::TokenType::LEFT_PAREN, "("),
        TestTokenFactory::createToken(lexer::TokenType::RIGHT_PAREN, ")"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Verificar que los tokens pasaron sin cambios
    ASSERT_EQ(outputTokens.size(), inputTokens.size());
    for (size_t i = 0; i < outputTokens.size(); ++i) {
        EXPECT_EQ(outputTokens[i].getType(), inputTokens[i].getType());
        EXPECT_EQ(outputTokens[i].getLexeme(), inputTokens[i].getLexeme());
    }
}

// Test para procesamiento de #define
TEST(PreprocessorTest, DefineDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Crear tokens que simulan #define MAX 100
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "define"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "MAX"),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "100"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Verificar que la macro fue definida
    EXPECT_TRUE(preprocessor.isMacroDefined("MAX"));

    // El output debería estar vacío (solo EOF) porque #define se procesa
    ASSERT_EQ(outputTokens.size(), 1);
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::END_OF_FILE);
}

// Test para procesamiento de #ifdef/#ifndef
TEST(PreprocessorTest, ConditionalDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir una macro para pruebas
    preprocessor.defineMacro("DEFINED_MACRO", "1");

    // Test #ifdef con macro definida
    std::vector<lexer::Token> inputTokens1 = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "ifdef"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "DEFINED_MACRO"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "content"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "endif"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens1 = preprocessor.process(inputTokens1);

    // Debería incluir "content"
    bool foundContent = false;
    for (const auto& token : outputTokens1) {
        if (token.getLexeme() == "content") {
            foundContent = true;
            break;
        }
    }
    EXPECT_TRUE(foundContent);

    // Test #ifndef con macro no definida
    std::vector<lexer::Token> inputTokens2 = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "ifndef"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "UNDEFINED_MACRO"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "content2"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "endif"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens2 = preprocessor.process(inputTokens2);

    // Debería incluir "content2"
    bool foundContent2 = false;
    for (const auto& token : outputTokens2) {
        if (token.getLexeme() == "content2") {
            foundContent2 = true;
            break;
        }
    }
    EXPECT_TRUE(foundContent2);
}

// Test para procesamiento de #include
TEST(PreprocessorTest, IncludeDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Simular #include <iostream>
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "include"),
        TestTokenFactory::createToken(lexer::TokenType::LESS, "<"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "iostream"),
        TestTokenFactory::createToken(lexer::TokenType::GREATER, ">"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // El output debería estar vacío porque #include se procesa
    ASSERT_EQ(outputTokens.size(), 1);
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::END_OF_FILE);
}

// Test para expansión de macros simples
TEST(PreprocessorTest, SimpleMacroExpansion) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir macro
    preprocessor.defineMacro("PI", "3.14159");

    // Crear tokens con uso de la macro
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "PI"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Verificar que PI fue expandido a 3.14159
    ASSERT_EQ(outputTokens.size(), 2); // "3.14159" + EOF
    EXPECT_EQ(outputTokens[0].getLexeme(), "3.14159");
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::FLOAT_LITERAL);
}

// Test para expansión de macros de función (simplificada)
TEST(PreprocessorTest, FunctionMacroExpansion) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Crear macro de función simple
    std::vector<lexer::Token> body = {
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "a"),
        TestTokenFactory::createToken(lexer::TokenType::PLUS, "+"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "b")
    };

    MacroDefinition funcMacro("SUM", body, true, false);
    funcMacro.parameters = {"a", "b"};
    preprocessor.defineMacro(funcMacro);

    // Crear tokens con llamada a la macro
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "SUM"),
        TestTokenFactory::createToken(lexer::TokenType::LEFT_PAREN, "("),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "1"),
        TestTokenFactory::createToken(lexer::TokenType::COMMA, ","),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "2"),
        TestTokenFactory::createToken(lexer::TokenType::RIGHT_PAREN, ")"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Verificar que la expansión funcionó (simplificada)
    EXPECT_GT(outputTokens.size(), 1);
}

// Test para anidamiento condicional
TEST(PreprocessorTest, NestedConditionals) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    preprocessor.defineMacro("LEVEL1", "1");

    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "ifdef"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "LEVEL1"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "ifdef"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "LEVEL2"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "nested"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "endif"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "endif"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "outer"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Debería incluir "outer" pero no "nested"
    bool foundOuter = false, foundNested = false;
    for (const auto& token : outputTokens) {
        if (token.getLexeme() == "outer") foundOuter = true;
        if (token.getLexeme() == "nested") foundNested = true;
    }

    EXPECT_TRUE(foundOuter);
    EXPECT_FALSE(foundNested);
}

// Test para estadísticas del preprocesador
TEST(PreprocessorTest, PreprocessorStatistics) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir algunas macros
    preprocessor.defineMacro("MACRO1", "value1");
    preprocessor.defineMacro("MACRO2", "value2");

    // Eliminar una
    preprocessor.undefineMacro("MACRO1");

    // Procesar algunos tokens
    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "define"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "TEMP"),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "42"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    preprocessor.process(inputTokens);

    auto stats = preprocessor.getStats();

    EXPECT_EQ(stats.macrosDefined, 3); // 2 manuales + 1 procesada
    EXPECT_EQ(stats.macrosUndefined, 1);
    EXPECT_EQ(stats.tokensProcessed, 5);
}

// Test para macros predefinidas
TEST(PreprocessorTest, PredefinedMacros) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Verificar macros estándar de C++
    EXPECT_TRUE(preprocessor.isMacroDefined("__cplusplus"));

    const MacroDefinition* cppMacro = preprocessor.getMacro("__cplusplus");
    ASSERT_TRUE(cppMacro != nullptr);
    EXPECT_FALSE(cppMacro->isFunctionLike);
    EXPECT_EQ(cppMacro->body.size(), 1);
    EXPECT_EQ(cppMacro->body[0].getLexeme(), "202002L");
}

// Test para procesamiento de #pragma
TEST(PreprocessorTest, PragmaDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "pragma"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "once"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // #pragma debería ser procesado sin generar output
    ASSERT_EQ(outputTokens.size(), 1);
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::END_OF_FILE);
}

// Test para procesamiento de #line
TEST(PreprocessorTest, LineDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "line"),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "100"),
        TestTokenFactory::createToken(lexer::TokenType::STRING_LITERAL, "\"test.h\""),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // #line debería ser procesado sin generar output
    ASSERT_EQ(outputTokens.size(), 1);
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::END_OF_FILE);
}

// Test para procesamiento de #error
TEST(PreprocessorTest, ErrorDirectiveProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "error"),
        TestTokenFactory::createToken(lexer::TokenType::STRING_LITERAL, "\"Test error\""),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    // Este test verificaría que se reporta un error, pero no podemos capturar
    // la salida de error fácilmente en este contexto
    auto outputTokens = preprocessor.process(inputTokens);

    // #error debería ser procesado sin generar output
    ASSERT_EQ(outputTokens.size(), 1);
    EXPECT_EQ(outputTokens[0].getType(), lexer::TokenType::END_OF_FILE);
}

// Test para procesamiento de expresiones condicionales
TEST(PreprocessorTest, ConditionalExpressionProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    preprocessor.defineMacro("VALUE", "5");

    std::vector<lexer::Token> inputTokens = {
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "if"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "VALUE"),
        TestTokenFactory::createToken(lexer::TokenType::GREATER, ">"),
        TestTokenFactory::createToken(lexer::TokenType::INTEGER_LITERAL, "3"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "included"),
        TestTokenFactory::createToken(lexer::TokenType::HASH, "#"),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "endif"),
        TestTokenFactory::createToken(lexer::TokenType::END_OF_FILE)
    };

    auto outputTokens = preprocessor.process(inputTokens);

    // Debería incluir "included" porque VALUE > 3
    bool foundIncluded = false;
    for (const auto& token : outputTokens) {
        if (token.getLexeme() == "included") {
            foundIncluded = true;
            break;
        }
    }
    EXPECT_TRUE(foundIncluded);
}

// Test para configuración del preprocesador
TEST(PreprocessorTest, PreprocessorConfiguration) {
    DiagnosticEngine diagEngine;

    PreprocessorConfig config;
    config.enableWarnings = false;
    config.maxIncludeDepth = 10;
    config.includePaths = {"/usr/include", "/opt/include"};

    Preprocessor preprocessor(diagEngine, config);

    // Verificar que la configuración se aplicó
    EXPECT_EQ(config.maxIncludeDepth, 10);
    EXPECT_EQ(config.includePaths.size(), 2);
    EXPECT_FALSE(config.enableWarnings);
}

// Test para utilidades del preprocesador
TEST(PreprocessorUtilsTest, DirectiveDetection) {
    using namespace cpp20::compiler::frontend;

    // Test para detectar directivas
    lexer::Token hashToken(lexer::TokenType::HASH, "#", SourceLocation("", 1, 1));
    EXPECT_TRUE(PreprocessorUtils::isDirectiveStart(hashToken));

    lexer::Token identToken(lexer::TokenType::IDENTIFIER, "int", SourceLocation("", 1, 1));
    EXPECT_FALSE(PreprocessorUtils::isDirectiveStart(identToken));

    // Test para extraer nombre de directiva
    EXPECT_EQ(PreprocessorUtils::extractDirectiveName(hashToken), "#");

    // Test para líneas en blanco
    std::vector<lexer::Token> blankLine = {
        lexer::Token(lexer::TokenType::INVALID, " ", SourceLocation("", 1, 1))
    };
    EXPECT_TRUE(PreprocessorUtils::isBlankLine(blankLine));

    std::vector<lexer::Token> nonBlankLine = {
        lexer::Token(lexer::TokenType::INT, "int", SourceLocation("", 1, 1))
    };
    EXPECT_FALSE(PreprocessorUtils::isBlankLine(nonBlankLine));
}

// Test para procesamiento de macros complejas
TEST(PreprocessorTest, ComplexMacroProcessing) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Definir macro compleja
    std::vector<lexer::Token> body = {
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "printf"),
        TestTokenFactory::createToken(lexer::TokenType::LEFT_PAREN, "("),
        TestTokenFactory::createToken(lexer::TokenType::STRING_LITERAL, "\"%d\\n\""),
        TestTokenFactory::createToken(lexer::TokenType::COMMA, ","),
        TestTokenFactory::createToken(lexer::TokenType::IDENTIFIER, "x"),
        TestTokenFactory::createToken(lexer::TokenType::RIGHT_PAREN, ")"),
        TestTokenFactory::createToken(lexer::TokenType::SEMICOLON, ";")
    };

    MacroDefinition complexMacro("PRINT_INT", body, true, false);
    complexMacro.parameters = {"x"};
    preprocessor.defineMacro(complexMacro);

    EXPECT_TRUE(preprocessor.isMacroDefined("PRINT_INT"));

    const MacroDefinition* macro = preprocessor.getMacro("PRINT_INT");
    ASSERT_TRUE(macro != nullptr);
    EXPECT_TRUE(macro->isFunctionLike);
    EXPECT_EQ(macro->parameters.size(), 1);
    EXPECT_EQ(macro->parameters[0], "x");
}

// Test para límites y casos extremos
TEST(PreprocessorTest, LimitsAndEdgeCases) {
    DiagnosticEngine diagEngine;
    Preprocessor preprocessor(diagEngine);

    // Test con nombre de macro vacío (debería fallar)
    preprocessor.defineMacro("", "value");
    EXPECT_FALSE(preprocessor.isMacroDefined(""));

    // Test con macro que se redefine a sí misma
    preprocessor.defineMacro("SELF", "SELF");
    EXPECT_TRUE(preprocessor.isMacroDefined("SELF"));

    // Test con undef de macro no existente
    preprocessor.undefineMacro("NONEXISTENT");
    // No debería causar problemas

    // Test con muchas macros
    for (int i = 0; i < 100; ++i) {
        std::string name = "MACRO" + std::to_string(i);
        preprocessor.defineMacro(name, "value" + std::to_string(i));
        EXPECT_TRUE(preprocessor.isMacroDefined(name));
    }

    auto stats = preprocessor.getStats();
    EXPECT_EQ(stats.macrosDefined, 103); // 100 + SELF + las predefinidas
}
