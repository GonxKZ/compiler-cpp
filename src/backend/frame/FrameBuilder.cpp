/**
 * @file FrameBuilder.cpp
 * @brief Constructor de frames de pila para x86_64
 */

#include <compiler/backend/frame/FrameBuilder.h>
#include <compiler/backend/abi/ABIContract.h>
#include <algorithm>

namespace cpp20::compiler::backend {

// ========================================================================
// FrameLayout implementation
// ========================================================================

FrameLayout::FrameLayout() = default;

size_t FrameLayout::totalSize() const {
    return parameterAreaSize + localAreaSize + spillAreaSize +
           ABIContract::SHADOW_SPACE_SIZE + 16; // return addr + saved RBP
}

bool FrameLayout::isValid() const {
    return ABIContract::validateFrameLayout(*this);
}

// ========================================================================
// FrameBuilder implementation
// ========================================================================

FrameBuilder::FrameBuilder() = default;

FrameLayout FrameBuilder::buildFrameLayout(
    const std::vector<ABIContract::ParameterInfo>& params,
    size_t localSize,
    size_t spillSize) {

    FrameLayout layout;

    // Calcular tamaño de parámetros en stack
    size_t paramStackSize = 0;
    for (const auto& param : params) {
        if (!param.inRegister) {
            size_t alignedSize = (param.size + param.alignment - 1) & ~(param.alignment - 1);
            paramStackSize += alignedSize;
        }
    }

    // Asignar áreas
    layout.parameterAreaSize = paramStackSize;
    layout.localAreaSize = localSize;
    layout.spillAreaSize = spillSize;

    // Calcular offsets (desde RSP)
    layout.returnAddressOffset = 8;
    layout.savedRbpOffset = 0;

    if (paramStackSize > 0) {
        layout.firstParameterOffset = layout.returnAddressOffset + 8 + spillSize + localSize;
    }

    return layout;
}

std::vector<ABIContract::ParameterInfo> FrameBuilder::classifyParameters(
    const std::vector<std::pair<size_t, size_t>>& paramSizes) {

    std::vector<ABIContract::ParameterInfo> params;

    for (size_t i = 0; i < paramSizes.size(); ++i) {
        const auto& [size, alignment] = paramSizes[i];

        auto paramInfo = ABIContract::classifyParameter(size, alignment, false, false);

        // Asignar registro si está disponible
        if (paramInfo.kind == ABIContract::ParameterInfo::Kind::Integer && i < 4) {
            paramInfo.inRegister = true;
            paramInfo.registerIndex = static_cast<int>(i);
        }

        params.push_back(paramInfo);
    }

    return params;
}

size_t FrameBuilder::calculateSpillSize(const std::vector<bool>& calleeSavedUsed) {
    size_t spillSize = 0;

    // Callee-saved registers that might need spilling
    std::vector<int> calleeSaved = {3, 6, 7, 12, 13, 14, 15}; // RBX, RSI, RDI, R12-R15

    for (size_t i = 0; i < calleeSaved.size() && i < calleeSavedUsed.size(); ++i) {
        if (calleeSavedUsed[i]) {
            spillSize += 8; // 8 bytes per register
        }
    }

    return spillSize;
}

bool FrameBuilder::validateFrame(const FrameLayout& layout) {
    return layout.isValid();
}

} // namespace cpp20::compiler::backend
