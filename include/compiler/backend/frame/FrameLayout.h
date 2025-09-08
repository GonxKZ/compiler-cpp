#pragma once

#include <cstddef>

namespace cpp20::compiler::backend {

/**
 * @brief Información del layout de un stack frame
 */
struct FrameLayout {
    // Tamaños de áreas
    size_t parameterAreaSize = 0;   // Área de parámetros en stack
    size_t localAreaSize = 0;       // Área de variables locales
    size_t spillAreaSize = 0;       // Área de spills de registros

    // Offsets desde RSP
    size_t returnAddressOffset = 0; // Offset de return address
    size_t savedRbpOffset = 0;      // Offset de RBP guardado
    size_t firstParameterOffset = 0;// Offset del primer parámetro

    size_t totalSize() const;
    bool isValid() const;
};

} // namespace cpp20::compiler::backend
