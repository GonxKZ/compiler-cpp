# =============================================================================
# Ejemplos del Compilador C++20
# =============================================================================

# Ejemplo básico hello world
add_executable(hello-world hello.cpp)

# Ejemplo de generación de objeto COFF
add_executable(hello-coff hello_coff.cpp)

# Ejemplo de prueba del sistema de unwind (Capa 2)
add_executable(test-unwind test_unwind.cpp)

# Ejemplo de prueba del sistema de name mangling (Capa 3)
add_executable(test-mangling test_mangling.cpp)

# Ejemplo de prueba del sistema de templates (Capa 5)
add_executable(test-templates test_templates.cpp)

# Ejemplo de prueba del sistema constexpr (Capa 6)
add_executable(test-constexpr test_constexpr.cpp)

# Enlazar con las librerías necesarias
target_link_libraries(hello-coff
    PRIVATE
        cpp20-compiler::backend
        cpp20-compiler::common
)

target_link_libraries(test-unwind
    PRIVATE
        cpp20-compiler::backend
        cpp20-compiler::common
)

target_link_libraries(test-mangling
    PRIVATE
        cpp20-compiler::backend
        cpp20-compiler::common
)

target_link_libraries(test-templates
    PRIVATE
        cpp20-compiler::backend
        cpp20-compiler::common
)

target_link_libraries(test-constexpr
    PRIVATE
        cpp20-compiler::constexpr
        cpp20-compiler::backend
        cpp20-compiler::common
)

# Ejemplo del sistema de módulos C++20 (Capa 7)
if(CPP20_COMPILER_ENABLE_MODULES)
    # Nota: Los módulos requieren soporte especial del compilador
    # Este ejemplo demuestra la estructura pero puede requerir
    # configuración adicional para compilación real

    add_executable(modules-demo modules_main.cpp)

    target_link_libraries(modules-demo
        PRIVATE
            cpp20-compiler::modules
            cpp20-compiler::backend
            cpp20-compiler::common
    )

    # Configurar para módulos si el compilador lo soporta
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(modules-demo PRIVATE /experimental:module)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(modules-demo PRIVATE -fmodules)
    endif()
endif()

# Ejemplo del sistema de corroutinas C++20 (Capa 8)
if(CPP20_COMPILER_ENABLE_COROUTINES)
    # Ejemplo de ping-pong con corroutinas
    add_executable(coro-pingpong coro_pingpong.cpp)

    target_link_libraries(coro-pingpong
        PRIVATE
            cpp20-compiler::coroutines
            cpp20-compiler::backend
            cpp20-compiler::common
    )

    # Configurar para corroutinas si el compilador lo soporta
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(coro-pingpong PRIVATE /await:strict)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(coro-pingpong PRIVATE -fcoroutines-ts)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(coro-pingpong PRIVATE -fcoroutines)
    endif()
endif()

# Configurar ejemplos según las capas implementadas
