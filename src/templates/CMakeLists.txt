# =============================================================================
# Sistema de Templates del Compilador C++20
# =============================================================================

# Fuentes del sistema de templates
set(TEMPLATES_SOURCES
    # TemplateSystem.cpp ya está en semantic/
)

set(TEMPLATES_HEADERS
    # TemplateSystem.h ya está en semantic/
)

# Crear librería templates
add_library(cpp20-compiler-templates ${TEMPLATES_SOURCES} ${TEMPLATES_HEADERS})

# Configurar propiedades de la librería
target_include_directories(cpp20-compiler-templates
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enlazar con dependencias
target_link_libraries(cpp20-compiler-templates
    PUBLIC
        cpp20-compiler::common
        cpp20-compiler::ast
        cpp20-compiler::types
    PRIVATE
        cpp20-compiler::diagnostics
)

# Configurar opciones de compilación
target_compile_features(cpp20-compiler-templates PUBLIC cxx_std_20)
target_compile_options(cpp20-compiler-templates PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# Alias
add_library(cpp20-compiler::templates ALIAS cpp20-compiler-templates)
