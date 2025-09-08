# =============================================================================
# Sistema Constexpr del Compilador C++20
# =============================================================================

# Fuentes del sistema constexpr
set(CONSTEXPR_SOURCES
    ConstexprEvaluator.cpp
)

set(CONSTEXPR_HEADERS
    ../../include/compiler/constexpr/ConstexprEvaluator.h
)

# Crear librería constexpr
add_library(cpp20-compiler-constexpr ${CONSTEXPR_SOURCES} ${CONSTEXPR_HEADERS})

# Configurar propiedades de la librería
target_include_directories(cpp20-compiler-constexpr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enlazar con dependencias
target_link_libraries(cpp20-compiler-constexpr
    PUBLIC
        cpp20-compiler::common
        cpp20-compiler::ast
        cpp20-compiler::types
)

# Configurar opciones de compilación
target_compile_features(cpp20-compiler-constexpr PUBLIC cxx_std_20)
target_compile_options(cpp20-compiler-constexpr PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# Alias
add_library(cpp20-compiler::constexpr ALIAS cpp20-compiler-constexpr)
