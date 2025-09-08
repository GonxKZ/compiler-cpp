# =============================================================================
# Front-end del Compilador C++20
# =============================================================================

# Lexer
set(LEXER_SOURCES
    lexer/Lexer.cpp
    lexer/Token.cpp
    lexer/KeywordTable.cpp
)

set(LEXER_HEADERS
    lexer/Lexer.h
    lexer/Token.h
    lexer/KeywordTable.h
)

# Parser
set(PARSER_SOURCES
    parser/Parser.cpp
    parser/ParseExpression.cpp
    parser/ParseStatement.cpp
    parser/ParseDeclaration.cpp
)

set(PARSER_HEADERS
    parser/Parser.h
    parser/ParseExpression.h
    parser/ParseStatement.h
    parser/ParseDeclaration.h
)

# Preprocesador
set(PREPROCESSOR_SOURCES
    preprocessor/Preprocessor.cpp
    preprocessor/Macro.cpp
    preprocessor/Directive.cpp
)

set(PREPROCESSOR_HEADERS
    preprocessor/Preprocessor.h
    preprocessor/Macro.h
    preprocessor/Directive.h
)

# Análisis Semántico
set(SEMANTIC_SOURCES
    semantic/SemanticAnalyzer.cpp
    semantic/TypeChecker.cpp
    semantic/NameResolver.cpp
)

set(SEMANTIC_HEADERS
    semantic/SemanticAnalyzer.h
    semantic/TypeChecker.h
    semantic/NameResolver.h
)

# Combinar todos los sources y headers
set(FRONTEND_SOURCES
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${PREPROCESSOR_SOURCES}
    ${SEMANTIC_SOURCES}
)

set(FRONTEND_HEADERS
    ${LEXER_HEADERS}
    ${PARSER_HEADERS}
    ${PREPROCESSOR_HEADERS}
    ${SEMANTIC_HEADERS}
)

# Crear librería del front-end
add_library(cpp20-compiler-frontend STATIC
    ${FRONTEND_SOURCES}
    ${FRONTEND_HEADERS}
)

# Dependencias
target_link_libraries(cpp20-compiler-frontend
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::types
        cpp20-compiler::symbols
        cpp20-compiler::ast
)

# Configuración
target_include_directories(cpp20-compiler-frontend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias
add_library(cpp20-compiler::frontend ALIAS cpp20-compiler-frontend)
