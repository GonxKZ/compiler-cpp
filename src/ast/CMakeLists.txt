# =============================================================================
# Abstract Syntax Tree (AST) del Compilador C++20
# =============================================================================

set(AST_SOURCES
    ASTNode.cpp
    Expression.cpp
    Statement.cpp
    Declaration.cpp
    TypeNode.cpp
    Literal.cpp
    ASTVisitor.cpp
    ASTDumper.cpp
)

set(AST_HEADERS
    ASTNode.h
    Expression.h
    Statement.h
    Declaration.h
    TypeNode.h
    Literal.h
    ASTVisitor.h
    ASTDumper.h
)

# Crear librería AST
add_library(cpp20-compiler-ast STATIC
    ${AST_SOURCES}
    ${AST_HEADERS}
)

# Dependencias
target_link_libraries(cpp20-compiler-ast
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::types
        cpp20-compiler::symbols
)

# Configuración
target_include_directories(cpp20-compiler-ast
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias
add_library(cpp20-compiler::ast ALIAS cpp20-compiler-ast)
