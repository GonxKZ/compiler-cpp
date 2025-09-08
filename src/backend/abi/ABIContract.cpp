/**
 * @file ABIContract.cpp
 * @brief Implementación del contrato binario ABI para x86_64-pc-windows-msvc
 */

#include <compiler/backend/abi/ABIContract.h>
#include <cassert>
#include <algorithm>

namespace cpp20::compiler::backend::abi {

// ========================================================================
// FUNCIONES DE VALIDACIÓN DEL ABI
// ========================================================================

bool ABIContract::validateFrameLayout(const FrameLayout& layout) {
    // Verificar tamaño total
    if (layout.totalSize > MAX_FRAME_SIZE) {
        return false;
    }

    // Verificar alineación del stack
    if (!isStackAligned(layout.totalSize)) {
        return false;
    }

    // Verificar que el espacio de sombra esté presente
    if (layout.shadowSpaceSize != SHADOW_SPACE_SIZE) {
        return false;
    }

    // Verificar offsets razonables
    if (layout.returnAddressOffset >= layout.totalSize ||
        layout.savedRbpOffset >= layout.totalSize) {
        return false;
    }

    // Verificar que el área de parámetros tenga sentido
    if (layout.parameterAreaSize > 0 &&
        layout.firstParameterOffset >= layout.totalSize) {
        return false;
    }

    return true;
}

ABIContract::ParameterInfo ABIContract::classifyParameter(
    size_t size, size_t alignment, bool isFloat, bool isSigned) {

    ParameterInfo info(ParameterInfo::Kind::Integer, size, alignment, false, -1, isSigned);

    if (isFloat) {
        // Parámetros flotantes
        if (size <= 8) {  // float, double
            info.kind = ParameterInfo::Kind::Float;
        } else if (size == 16) {  // __m128
            info.kind = ParameterInfo::Kind::Vector;
        }
    } else {
        // Parámetros enteros/punteros
        if (size <= 8) {
            info.kind = ParameterInfo::Kind::Integer;
        } else if (size > 8 && size <= 16) {
            info.kind = ParameterInfo::Kind::Aggregate;
        } else {
            // Tipos grandes pasan por referencia (indirect)
            info.kind = ParameterInfo::Kind::Aggregate;
        }
    }

    return info;
}

ABIContract::ReturnInfo ABIContract::classifyReturn(
    size_t size, size_t alignment, bool isFloat, bool isAggregate) {

    ReturnInfo info;

    if (size == 0) {
        info.kind = ReturnInfo::Kind::Void;
        return info;
    }

    if (isAggregate) {
        // Structs/unions/clases pasan por referencia
        info.kind = ReturnInfo::Kind::Aggregate;
        info.isIndirect = true;
    } else if (isFloat) {
        if (size <= 8) {
            info.kind = ReturnInfo::Kind::Float;
        } else if (size == 16) {
            info.kind = ReturnInfo::Kind::Vector;
        }
    } else {
        // Enteros y punteros
        if (size <= 8) {
            info.kind = ReturnInfo::Kind::Integer;
        } else {
            info.kind = ReturnInfo::Kind::Aggregate;
            info.isIndirect = true;
        }
    }

    info.size = size;
    return info;
}

size_t ABIContract::calculateStackSize(
    const std::vector<ParameterInfo>& params,
    size_t localSize,
    size_t spillSize) {

    // Contar parámetros que van en stack
    size_t stackArgsSize = 0;
    for (const auto& param : params) {
        if (!param.inRegister) {
            // Alinear cada parámetro según su requerimiento
            size_t alignedSize = (param.size + param.alignment - 1) & ~(param.alignment - 1);
            stackArgsSize += alignedSize;
        }
    }

    // Calcular tamaño total del frame
    size_t totalSize = 0;

    // Espacio para dirección de retorno (8 bytes)
    totalSize += 8;

    // Espacio para RBP guardado (8 bytes)
    totalSize += 8;

    // Área de spills de registros
    totalSize += spillSize;

    // Área de variables locales
    totalSize += localSize;

    // Área de parámetros en stack
    totalSize += stackArgsSize;

    // Espacio de sombra (32 bytes)
    totalSize += SHADOW_SPACE_SIZE;

    // Alinear a 16 bytes
    totalSize = (totalSize + STACK_ALIGNMENT - 1) & ~(STACK_ALIGNMENT - 1);

    return totalSize;
}

std::string ABIContract::getIntegerArgRegister(int index) {
    switch (static_cast<ArgRegister>(index)) {
        case ArgRegister::RCX: return "rcx";
        case ArgRegister::RDX: return "rdx";
        case ArgRegister::R8:  return "r8";
        case ArgRegister::R9:  return "r9";
        default: return "";  // Stack
    }
}

std::string ABIContract::getFloatArgRegister(int index) {
    switch (static_cast<FloatArgRegister>(index)) {
        case FloatArgRegister::XMM0: return "xmm0";
        case FloatArgRegister::XMM1: return "xmm1";
        case FloatArgRegister::XMM2: return "xmm2";
        case FloatArgRegister::XMM3: return "xmm3";
        default: return "";  // Stack
    }
}

std::string ABIContract::getValidationErrorString(ValidationError error) {
    switch (error) {
        case ValidationError::OK:
            return "OK";
        case ValidationError::INVALID_FRAME_SIZE:
            return "Tamaño de frame inválido";
        case ValidationError::UNALIGNED_STACK:
            return "Stack no alineado correctamente";
        case ValidationError::INVALID_PARAMETER_CLASS:
            return "Clase de parámetro inválida";
        case ValidationError::TOO_MANY_REG_ARGS:
            return "Demasiados argumentos en registros";
        case ValidationError::INVALID_RETURN_CLASS:
            return "Clase de retorno inválida";
        default:
            return "Error de validación desconocido";
    }
}

} // namespace cpp20::compiler::backend::abi
