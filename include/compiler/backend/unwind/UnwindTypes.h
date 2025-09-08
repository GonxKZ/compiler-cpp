/**
 * @file UnwindTypes.h
 * @brief Estructuras para desenrollado de pila Windows x64
 */

#pragma once

#include <cstdint>
#include <vector>

namespace cpp20::compiler::backend::unwind {

// ============================================================================
// Estructuras de UNWIND_INFO (Microsoft PE/COFF Specification)
// ============================================================================

/**
 * @brief Versiones de formato UNWIND_INFO
 */
enum class UnwindVersion : uint8_t {
    Version1 = 1,  // Windows 8.1+
    Version2 = 2   // Windows 10+
};

/**
 * @brief Flags para UNWIND_INFO
 */
enum class UnwindFlags : uint8_t {
    None = 0x00,
    EHHandler = 0x01,      // Función tiene exception handler
    UHHandler = 0x02,      // Función tiene termination handler
    FHandler = 0x03,       // Función tiene ambos handlers
    ChainInfo = 0x04       // Esta es una entrada chained
};

/**
 * @brief Tipos de operaciones UNWIND_CODE
 */
enum class UnwindOpCode : uint8_t {
    // Operaciones de prologue
    PUSH_NONVOL = 0,       // Push nonvolatile register
    ALLOC_LARGE = 1,       // Allocate large-sized area
    ALLOC_SMALL = 2,       // Allocate small-sized area
    SET_FPREG = 3,         // Set frame pointer register
    SAVE_NONVOL = 4,       // Save nonvolatile register
    SAVE_NONVOL_FAR = 5,   // Save nonvolatile register far
    SAVE_XMM128 = 6,       // Save XMM128 register
    SAVE_XMM128_FAR = 7,   // Save XMM128 register far
    PUSH_MACHFRAME = 8,    // Push machine frame

    // Operaciones de epilogue (solo en chained unwind)
    // (mismos códigos que prologue pero con flag epilogue)
};

/**
 * @brief Información de frame para SET_FPREG
 */
struct FrameInfo {
    uint8_t offset;     // Offset from RSP (scaled by 16)
    uint8_t reg;        // Register number (0-15)
};

/**
 * @brief Entrada UNWIND_CODE
 */
struct UnwindCode {
    uint8_t codeOffset;     // Offset from start of prologue
    uint8_t unwindOp : 4;   // UnwindOpCode
    uint8_t opInfo : 4;     // Operation info (register, etc.)

    // Constructor
    UnwindCode(uint8_t offset, UnwindOpCode op, uint8_t info = 0)
        : codeOffset(offset), unwindOp(static_cast<uint8_t>(op)), opInfo(info) {}
};

/**
 * @brief Estructura UNWIND_INFO principal
 */
struct UnwindInfo {
    uint8_t version : 3;           // UnwindVersion
    uint8_t flags : 5;             // UnwindFlags
    uint8_t sizeOfProlog;          // Size of prologue in bytes
    uint8_t countOfCodes;          // Count of unwind codes
    uint8_t frameRegister : 4;     // Frame pointer register
    uint8_t frameOffset : 4;       // Offset of frame pointer

    // Variable-sized arrays (stored after this structure)
    // UnwindCode unwindCodes[countOfCodes];
    // uint32_t exceptionHandlerRVA;  // If EHHandler flag set
    // uint32_t exceptionDataRVA;     // If EHHandler flag set

    // Constructor
    UnwindInfo(UnwindVersion ver = UnwindVersion::Version1,
               UnwindFlags fl = UnwindFlags::None,
               uint8_t prologSize = 0,
               uint8_t codeCount = 0,
               uint8_t frameReg = 0,
               uint8_t frameOff = 0)
        : version(static_cast<uint8_t>(ver))
        , flags(static_cast<uint8_t>(fl))
        , sizeOfProlog(prologSize)
        , countOfCodes(codeCount)
        , frameRegister(frameReg)
        , frameOffset(frameOff) {}
};

/**
 * @brief Información extendida para funciones con handlers
 */
struct ExceptionHandlerInfo {
    uint32_t exceptionHandlerRVA;   // RVA of exception handler
    uint32_t exceptionDataRVA;      // RVA of exception data (language-specific)
};

/**
 * @brief Información de función para .pdata
 */
struct RuntimeFunction {
    uint32_t beginAddress;     // RVA of function start
    uint32_t endAddress;       // RVA of function end
    uint32_t unwindInfoRVA;    // RVA of UNWIND_INFO

    // Constructor
    RuntimeFunction(uint32_t begin = 0, uint32_t end = 0, uint32_t unwindRVA = 0)
        : beginAddress(begin), endAddress(end), unwindInfoRVA(unwindRVA) {}
};

// ============================================================================
// Clases de alto nivel para gestión de unwind
// ============================================================================

/**
 * @brief Generador de códigos UNWIND_CODE a partir de prólogos
 */
class UnwindCodeGenerator {
public:
    /**
     * @brief Genera códigos unwind para un prólogo dado
     * @param prologueBytes Bytes del prólogo
     * @param stackSize Tamaño total de stack frame
     * @param frameReg Registro de frame pointer (o 0 si no hay)
     * @return Vector de UnwindCode
     */
    static std::vector<UnwindCode> generateFromPrologue(
        const std::vector<uint8_t>& prologueBytes,
        uint32_t stackSize,
        uint8_t frameReg = 0);

    /**
     * @brief Genera códigos para PUSH_NONVOL
     */
    static UnwindCode generatePushNonvol(uint8_t offset, uint8_t reg);

    /**
     * @brief Genera códigos para ALLOC_SMALL/ALLOC_LARGE
     */
    static std::vector<UnwindCode> generateAlloc(uint8_t offset, uint32_t size);

    /**
     * @brief Genera códigos para SAVE_NONVOL
     */
    static UnwindCode generateSaveNonvol(uint8_t offset, uint8_t reg, uint32_t saveOffset);

    /**
     * @brief Genera códigos para SET_FPREG
     */
    static UnwindCode generateSetFpreg(uint8_t offset, uint8_t reg, uint8_t frameOffset);
};

/**
 * @brief Generador de información UNWIND_INFO
 */
class UnwindInfoGenerator {
public:
    /**
     * @brief Crea UNWIND_INFO para una función
     * @param codes Códigos unwind generados
     * @param prologSize Tamaño del prólogo
     * @param hasExceptionHandler Si tiene exception handler
     * @param frameReg Registro de frame pointer
     * @param frameOffset Offset del frame pointer
     * @return Estructura UnwindInfo
     */
    static UnwindInfo generateUnwindInfo(
        const std::vector<UnwindCode>& codes,
        uint8_t prologSize,
        bool hasExceptionHandler = false,
        uint8_t frameReg = 0,
        uint8_t frameOffset = 0);

    /**
     * @brief Calcula el tamaño total de UNWIND_INFO en bytes
     */
    static uint32_t calculateUnwindInfoSize(const UnwindInfo& info);
};

/**
 * @brief Validador de información de unwind
 */
class UnwindValidator {
public:
    /**
     * @brief Valida que los códigos unwind sean consistentes
     * @param codes Vector de códigos unwind
     * @param prologSize Tamaño del prólogo
     * @return true si es válido
     */
    static bool validateUnwindCodes(
        const std::vector<UnwindCode>& codes,
        uint8_t prologSize);

    /**
     * @brief Valida estructura UNWIND_INFO completa
     */
    static bool validateUnwindInfo(const UnwindInfo& info);

    /**
     * @brief Verifica que el unwind sea consistente con el prólogo
     */
    static bool validateAgainstPrologue(
        const std::vector<UnwindCode>& codes,
        const std::vector<uint8_t>& prologueBytes);
};

} // namespace cpp20::compiler::backend::unwind
