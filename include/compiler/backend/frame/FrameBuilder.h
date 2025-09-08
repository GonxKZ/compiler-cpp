#pragma once

#include <compiler/backend/frame/FrameLayout.h>
#include <vector>
#include <utility>

// Definición local de ParameterInfo para evitar dependencias circulares
namespace cpp20::compiler::backend {

struct ParameterInfo {
    enum class Kind {
        Integer,        // Enteros y punteros
        Float,          // Float/double
        Vector,         // __m128, __m256
        Aggregate       // Structs, unions, clases
    };

    Kind kind;
    size_t size;        // Tamaño en bytes
    size_t alignment;   // Alineación requerida
    bool isSigned;      // Para enteros
    bool inRegister;    // Si se pasa en registro
    int registerIndex;  // Índice del registro (-1 si stack)

    ParameterInfo(Kind k = Kind::Integer, size_t sz = 0, size_t align = 0,
                  bool reg = false, int regIdx = -1, bool sign = false)
        : kind(k), size(sz), alignment(align), isSigned(sign),
          inRegister(reg), registerIndex(regIdx) {}
};

namespace abi {
    // Forward declaration
    class ABIContract;
}

/**
 * @brief Constructor de frames de pila para x86_64
 *
 * Responsable de:
 * - Calcular layouts de stack frames
 * - Asignar posiciones de spills
 * - Gestionar convenciones de llamada
 * - Validar frames generados
 */
class FrameBuilder {
public:
    FrameBuilder();

    /**
     * @brief Construye el layout de un frame
     * @param params Información de parámetros
     * @param localSize Tamaño de variables locales
     * @param spillSize Tamaño de spills
     * @return Layout del frame
     */
    FrameLayout buildFrameLayout(
        const std::vector<ParameterInfo>& params,
        size_t localSize,
        size_t spillSize);

    /**
     * @brief Clasifica parámetros según el ABI
     * @param paramSizes Vector de (size, alignment) para cada parámetro
     * @return Información clasificada de parámetros
     */
    std::vector<ParameterInfo> classifyParameters(
        const std::vector<std::pair<size_t, size_t>>& paramSizes);

    /**
     * @brief Calcula el tamaño necesario para spills
     * @param calleeSavedUsed Vector indicando qué callee-saved se usan
     * @return Tamaño total para spills
     */
    size_t calculateSpillSize(const std::vector<bool>& calleeSavedUsed);

    /**
     * @brief Valida un layout de frame
     * @param layout Layout a validar
     * @return true si es válido
     */
    bool validateFrame(const FrameLayout& layout);
};

} // namespace cpp20::compiler::backend
