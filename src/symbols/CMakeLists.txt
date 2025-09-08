# =============================================================================
# Sistema de Símbolos del Compilador C++20
# =============================================================================

set(SYMBOLS_SOURCES
    Symbol.cpp
    SymbolTable.cpp
    Scope.cpp
    Lookup.cpp
    NameLookup.cpp
)

set(SYMBOLS_HEADERS
    Symbol.h
    SymbolTable.h
    Scope.h
    Lookup.h
    NameLookup.h
)

# Crear librería de símbolos
add_library(cpp20-compiler-symbols STATIC
    ${SYMBOLS_SOURCES}
    ${SYMBOLS_HEADERS}
)

# Dependencias
target_link_libraries(cpp20-compiler-symbols
    PRIVATE
        cpp20-compiler::common
        cpp20-compiler::types
)

# Configuración
target_include_directories(cpp20-compiler-symbols
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias
add_library(cpp20-compiler::symbols ALIAS cpp20-compiler-symbols)
