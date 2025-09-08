# =============================================================================
# Sistema de Módulos C++20
# =============================================================================

# TODO: Implementar sistema de módulos en Capa 7
# Por ahora, este módulo está vacío

# Crear librería modules (vacía por ahora)
add_library(cpp20-compiler-modules INTERFACE)

# Alias
add_library(cpp20-compiler::modules ALIAS cpp20-compiler-modules)
