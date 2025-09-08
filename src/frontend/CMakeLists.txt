# =============================================================================
# Front-end del Compilador C++20
# =============================================================================

# Lexer
set(LEXER_SOURCES
    lexer/Lexer.cpp
)

set(LEXER_HEADERS
    lexer/Lexer.h
    lexer/Token.h
)

# Combinar todos los sources y headers
set(FRONTEND_SOURCES
    ${LEXER_SOURCES}
)

set(FRONTEND_HEADERS
    ${LEXER_HEADERS}
)

# Crear librería del front-end
add_library(cpp20-compiler-frontend STATIC
    ${FRONTEND_SOURCES}
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
