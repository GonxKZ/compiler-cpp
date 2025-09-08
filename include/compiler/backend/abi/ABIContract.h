#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace cpp20::compiler::backend::abi {

/**
 * @brief Especificación completa del contrato binario para x86_64-pc-windows-msvc
 *
 * Este documento define todas las invariantes binarias que deben cumplirse
 * para generar código compatible con el ABI de Microsoft para Windows x64.
 *
 * Referencia: https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention
 */
class ABIContract {
public:
    // ========================================================================
    // CONSTANTES DEL ABI
    // ========================================================================

    // Registros de paso de argumentos (orden específico)
    static constexpr int MAX_INTEGER_ARGS_IN_REGS = 4;
    static constexpr int MAX_FLOAT_ARGS_IN_REGS = 4;

    enum class ArgRegister {
        RCX = 0,    // Primer argumento entero
        RDX = 1,    // Segundo argumento entero
        R8  = 2,    // Tercer argumento entero
        R9  = 3     // Cuarto argumento entero
    };

    enum class FloatArgRegister {
        XMM0 = 0,   // Primer argumento flotante
        XMM1 = 1,   // Segundo argumento flotante
        XMM2 = 2,   // Tercer argumento flotante
        XMM3 = 3    // Cuarto argumento flotante
    };

    // Registros que deben preservarse (callee-saved)
    static constexpr uint16_t CALLEE_SAVED_REGS =
        (1 << 3)  | // RBX
        (1 << 6)  | // RSI
        (1 << 7)  | // RDI
        (1 << 12) | // R12
        (1 << 13) | // R13
        (1 << 14) | // R14
        (1 << 15) | // R15
        (1 << 5)  | // RBP
        (1 << 4);   // RSP (implícitamente)

    // Shadow space (espacio de sombra) - 32 bytes
    static constexpr size_t SHADOW_SPACE_SIZE = 32;

    // Alineaciones requeridas
    static constexpr size_t STACK_ALIGNMENT = 16;
    static constexpr size_t GENERAL_ALIGNMENT = 8;

    // ========================================================================
    // ESTRUCTURAS DE DATOS DEL ABI
    // ========================================================================

    /**
     * @brief Información de un parámetro para paso de argumentos
     */
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

        ParameterInfo(Kind k, size_t sz, size_t align, bool reg = false, int regIdx = -1, bool sign = false)
            : kind(k), size(sz), alignment(align), isSigned(sign),
              inRegister(reg), registerIndex(regIdx) {}
    };

    /**
     * @brief Información de retorno de función
     */
    struct ReturnInfo {
        enum class Kind {
            Void,
            Integer,        // RAX
            Float,          // XMM0
            Vector,         // XMM0/YMM0
            Aggregate       // Struct/union/class (paso por referencia)
        };

        Kind kind;
        size_t size;
        bool isIndirect;    // Retorno por referencia
        ParameterInfo::Kind underlyingKind;

        ReturnInfo(Kind k = Kind::Void, size_t sz = 0, bool indirect = false)
            : kind(k), size(sz), isIndirect(indirect) {}
    };

    /**
     * @brief Información de layout de stack frame
     */
    struct FrameLayout {
        size_t totalSize;           // Tamaño total del frame
        size_t parameterAreaSize;   // Área de parámetros
        size_t localAreaSize;       // Área de variables locales
        size_t spillAreaSize;       // Área de spills de registros
        size_t shadowSpaceSize;     // Espacio de sombra (32 bytes)

        // Offsets desde RSP
        size_t returnAddressOffset; // Offset de return address
        size_t savedRbpOffset;      // Offset de RBP guardado
        size_t firstParameterOffset;// Offset del primer parámetro

        FrameLayout()
            : totalSize(0), parameterAreaSize(0), localAreaSize(0),
              spillAreaSize(0), shadowSpaceSize(SHADOW_SPACE_SIZE),
              returnAddressOffset(0), savedRbpOffset(0), firstParameterOffset(0) {}
    };

    // ========================================================================
    // FUNCIONES DE VALIDACIÓN DEL ABI
    // ========================================================================

    /**
     * @brief Valida que un layout de frame cumpla con el ABI
     */
    static bool validateFrameLayout(const FrameLayout& layout);

    /**
     * @brief Determina cómo pasar un parámetro según el ABI
     */
    static ParameterInfo classifyParameter(
        size_t size, size_t alignment, bool isFloat, bool isSigned = false);

    /**
     * @brief Determina cómo retornar un valor según el ABI
     */
    static ReturnInfo classifyReturn(
        size_t size, size_t alignment, bool isFloat, bool isAggregate);

    /**
     * @brief Calcula el tamaño de stack necesario para una función
     */
    static size_t calculateStackSize(
        const std::vector<ParameterInfo>& params,
        size_t localSize,
        size_t spillSize);

    /**
     * @brief Verifica alineación de stack
     */
    static bool isStackAligned(size_t offset) {
        return (offset % STACK_ALIGNMENT) == 0;
    }

    /**
     * @brief Obtiene el registro para un argumento entero
     */
    static std::string getIntegerArgRegister(int index);

    /**
     * @brief Obtiene el registro para un argumento flotante
     */
    static std::string getFloatArgRegister(int index);

    /**
     * @brief Verifica si un registro debe preservarse
     */
    static bool isCalleeSavedRegister(int registerIndex) {
        return (CALLEE_SAVED_REGS & (1 << registerIndex)) != 0;
    }

    /**
     * @brief Alinea un offset al siguiente límite
     */
    static size_t alignOffset(size_t offset, size_t alignment);

    // ========================================================================
    // CONSTANTES DE VALIDACIÓN
    // ========================================================================

    // Límites del ABI
    static constexpr size_t MAX_STACK_ARGS = 4;  // Máximo args en registros
    static constexpr size_t MAX_FRAME_SIZE = 0x7FFFFFFF;  // 2GB límite teórico

    // Códigos de error para validación
    enum class ValidationError {
        OK = 0,
        INVALID_FRAME_SIZE,
        UNALIGNED_STACK,
        INVALID_PARAMETER_CLASS,
        TOO_MANY_REG_ARGS,
        INVALID_RETURN_CLASS
    };

    /**
     * @brief Obtiene descripción de un error de validación
     */
    static std::string getValidationErrorString(ValidationError error);
};

} // namespace cpp20::compiler::backend::abi
