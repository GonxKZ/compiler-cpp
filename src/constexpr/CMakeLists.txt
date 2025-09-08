# =============================================================================
# Sistema Constexpr del Compilador C++20
# =============================================================================

# TODO: Implementar evaluador constexpr en Capa 6
# Por ahora, este módulo está vacío

# Crear librería constexpr (vacía por ahora)
add_library(cpp20-compiler-constexpr INTERFACE)

# Alias
add_library(cpp20-compiler::constexpr ALIAS cpp20-compiler-constexpr)
