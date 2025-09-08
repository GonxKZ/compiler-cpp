# =============================================================================
# IR Intermedio del Compilador C++20
# =============================================================================

# TODO: Implementar IR intermedia cuando se integre LLVM
# Por ahora, este módulo está vacío ya que estamos en la fase "from scratch"

# Crear librería IR (vacía por ahora)
add_library(cpp20-compiler-ir INTERFACE)

# Alias
add_library(cpp20-compiler::ir ALIAS cpp20-compiler-ir)
