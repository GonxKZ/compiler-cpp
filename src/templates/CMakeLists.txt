# =============================================================================
# Sistema de Templates del Compilador C++20
# =============================================================================

# TODO: Implementar motor de templates en Capa 5
# Por ahora, este módulo está vacío

# Crear librería templates (vacía por ahora)
add_library(cpp20-compiler-templates INTERFACE)

# Alias
add_library(cpp20-compiler::templates ALIAS cpp20-compiler-templates)
