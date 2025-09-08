#pragma once

#include <compiler/backend/frame/FrameLayout.h>
#include <compiler/backend/abi/ABIContract.h>
#include <vector>
#include <utility>

namespace cpp20::compiler::backend {

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
        const std::vector<ABIContract::ParameterInfo>& params,
        size_t localSize,
        size_t spillSize);

    /**
     * @brief Clasifica parámetros según el ABI
     * @param paramSizes Vector de (size, alignment) para cada parámetro
     * @return Información clasificada de parámetros
     */
    std::vector<ABIContract::ParameterInfo> classifyParameters(
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
