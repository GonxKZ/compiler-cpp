/**
 * @file ABIContract.cpp
 * @brief Implementación completa del contrato binario para x86_64-pc-windows-msvc
 */

#include <compiler/backend/abi/ABIContract.h>
#include <algorithm>

namespace cpp20::compiler::backend::abi {

// ============================================================================
// IMPLEMENTACIÓN DE FUNCIONES ESTÁTICAS
// ============================================================================

bool ABIContract::validateFrameLayout(const FrameLayout& layout) {
    // Validar tamaño total
    if (layout.totalSize > MAX_FRAME_SIZE) {
        return false;
    }

    // Validar alineación
    if (!isStackAligned(layout.totalSize)) {
        return false;
    }

    // Validar que el área de parámetros tenga el tamaño correcto
    if (layout.parameterAreaSize < SHADOW_SPACE_SIZE) {
        return false;
    }

    // Validar offsets
    if (layout.returnAddressOffset != 0) {
        return false;
    }

    return true;
}

ABIContract::ParameterInfo ABIContract::classifyParameter(
    size_t size, size_t alignment, bool isFloat, bool isSigned) {

    ParameterInfo info(ParameterInfo::Kind::Integer, size, alignment, false, -1, isSigned);

    // Parámetros flotantes
    if (isFloat) {
        info.kind = ParameterInfo::Kind::Float;
        return info;
    }

    // Parámetros agregados (structs, etc.)
    if (size > 8 || alignment > 8) {
        info.kind = ParameterInfo::Kind::Aggregate;
        return info;
    }

    // Parámetros vectoriales
    if (size == 16 && alignment == 16) { // __m128
        info.kind = ParameterInfo::Kind::Vector;
        return info;
    }

    // Parámetros enteros normales
    info.kind = ParameterInfo::Kind::Integer;
    return info;
}

ABIContract::ReturnInfo ABIContract::classifyReturn(
    size_t size, size_t alignment, bool isFloat, bool isAggregate) {

    ReturnInfo info;

    // Void
    if (size == 0) {
        info.kind = ReturnInfo::Kind::Void;
        return info;
    }

    // Retorno por referencia para agregados grandes
    if (isAggregate && (size > 8 || alignment > 8)) {
        info.kind = ReturnInfo::Kind::Aggregate;
        info.isIndirect = true;
        return info;
    }

    // Retorno flotante
    if (isFloat) {
        info.kind = ReturnInfo::Kind::Float;
        return info;
    }

    // Retorno entero
    info.kind = ReturnInfo::Kind::Integer;
    return info;
}

size_t ABIContract::calculateStackSize(
    const std::vector<ParameterInfo>& params,
    size_t localSize,
    size_t spillSize) {

    size_t stackSize = SHADOW_SPACE_SIZE; // Espacio de sombra mínimo

    // Calcular espacio para parámetros pasados en stack
    size_t paramStackSize = 0;
    int regCount = 0;

    for (const auto& param : params) {
        if (param.inRegister) {
            regCount++;
            if (regCount > MAX_INTEGER_ARGS_IN_REGS) {
                // Este parámetro va al stack
                paramStackSize += alignOffset(param.size, GENERAL_ALIGNMENT);
            }
        } else {
            paramStackSize += alignOffset(param.size, param.alignment);
        }
    }

    // Alinear el área de parámetros
    paramStackSize = alignOffset(paramStackSize, STACK_ALIGNMENT);

    // Calcular total
    stackSize += paramStackSize;
    stackSize += alignOffset(localSize, GENERAL_ALIGNMENT);
    stackSize += alignOffset(spillSize, GENERAL_ALIGNMENT);

    // Alinear el total
    return alignOffset(stackSize, STACK_ALIGNMENT);
}

std::string ABIContract::getIntegerArgRegister(int index) {
    static const char* registers[] = {"rcx", "rdx", "r8", "r9"};
    if (index >= 0 && index < 4) {
        return registers[index];
    }
    return "";
}

std::string ABIContract::getFloatArgRegister(int index) {
    static const char* registers[] = {"xmm0", "xmm1", "xmm2", "xmm3"};
    if (index >= 0 && index < 4) {
        return registers[index];
    }
    return "";
}

std::string ABIContract::getValidationErrorString(ValidationError error) {
    switch (error) {
        case ValidationError::OK:
            return "Sin errores";
        case ValidationError::INVALID_FRAME_SIZE:
            return "Tamaño de frame inválido";
        case ValidationError::UNALIGNED_STACK:
            return "Stack no alineado";
        case ValidationError::INVALID_PARAMETER_CLASS:
            return "Clase de parámetro inválida";
        case ValidationError::TOO_MANY_REG_ARGS:
            return "Demasiados argumentos en registros";
        case ValidationError::INVALID_RETURN_CLASS:
            return "Clase de retorno inválida";
        default:
            return "Error desconocido";
    }
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

size_t ABIContract::alignOffset(size_t offset, size_t alignment) {
    if (alignment == 0) return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

} // namespace cpp20::compiler::backend::abi