# =============================================================================
# Sistema de Tipos del Compilador C++20
# =============================================================================

set(TYPES_SOURCES
    Type.cpp
    BasicType.cpp
    PointerType.cpp
    ReferenceType.cpp
    ArrayType.cpp
    FunctionType.cpp
    ClassType.cpp
    EnumType.cpp
    TypeSystem.cpp
)

set(TYPES_HEADERS
    Type.h
    BasicType.h
    PointerType.h
    ReferenceType.h
    ArrayType.h
    FunctionType.h
    ClassType.h
    EnumType.h
    TypeSystem.h
)

# Crear librería de tipos
add_library(cpp20-compiler-types STATIC
    ${TYPES_SOURCES}
    ${TYPES_HEADERS}
)

# Dependencias
target_link_libraries(cpp20-compiler-types
    PRIVATE
        cpp20-compiler::common
)

# Configuración
target_include_directories(cpp20-compiler-types
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Alias
add_library(cpp20-compiler::types ALIAS cpp20-compiler-types)
