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

# Configurar ejemplos según las capas implementadas
