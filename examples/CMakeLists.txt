# =============================================================================
# Ejemplos del Compilador C++20
# =============================================================================

# Ejemplo básico hello world
add_executable(hello-world hello.cpp)

# Ejemplo de generación de objeto COFF
add_executable(hello-coff hello_coff.cpp)

# Enlazar con las librerías necesarias
target_link_libraries(hello-coff
    PRIVATE
        cpp20-compiler::backend
        cpp20-compiler::common
)

# TODO: Configurar ejemplos más avanzados según las capas
