/**
 * @file UnwindEmitter.h
 * @brief Emisor de secciones .xdata y .pdata para Windows x64
 */

#pragma once

#include <compiler/backend/unwind/UnwindTypes.h>
#include <compiler/backend/coff/COFFTypes.h>
#include <vector>
#include <memory>

namespace cpp20::compiler::backend::unwind {

/**
 * @brief Emisor de información de unwind para Windows x64
 *
 * Esta clase genera las secciones .pdata y .xdata necesarias para
 * el stack unwinding en Windows x64.
 */
class UnwindEmitter {
public:
    /**
     * @brief Constructor
     */
    UnwindEmitter();

    /**
     * @brief Destructor
     */
    ~UnwindEmitter();

    /**
     * @brief Añade información de unwind para una función
     * @param functionRVA RVA de inicio de función
     * @param functionSize Tamaño de la función en bytes
     * @param prologueBytes Bytes del prólogo de la función
     * @param stackSize Tamaño total del stack frame
     * @param frameReg Registro de frame pointer (0 = RSP)
     * @param hasExceptionHandler Si la función tiene exception handler
     */
    void addFunctionUnwind(
        uint32_t functionRVA,
        uint32_t functionSize,
        const std::vector<uint8_t>& prologueBytes,
        uint32_t stackSize = 0,
        uint8_t frameReg = 0,
        bool hasExceptionHandler = false);

    /**
     * @brief Genera la sección .pdata (Runtime Functions)
     * @return Datos de la sección .pdata
     */
    std::vector<uint8_t> generatePdataSection();

    /**
     * @brief Genera la sección .xdata (Unwind Info)
     * @return Datos de la sección .xdata
     */
    std::vector<uint8_t> generateXdataSection();

    /**
     * @brief Obtiene el RVA base para la sección .xdata
     */
    uint32_t getXdataBaseRVA() const { return xdataBaseRVA_; }

    /**
     * @brief Establece el RVA base para la sección .xdata
     */
    void setXdataBaseRVA(uint32_t rva) { xdataBaseRVA_ = rva; }

    /**
     * @brief Obtiene el tamaño total de .pdata
     */
    uint32_t getPdataSize() const;

    /**
     * @brief Obtiene el tamaño total de .xdata
     */
    uint32_t getXdataSize() const;

    /**
     * @brief Valida toda la información de unwind generada
     * @return true si todo es válido
     */
    bool validateAll() const;

private:
    /**
     * @brief Información completa de unwind para una función
     */
    struct FunctionUnwindInfo {
        RuntimeFunction runtimeFunction;
        UnwindInfo unwindInfo;
        std::vector<UnwindCode> unwindCodes;
        std::vector<uint8_t> prologueBytes;
        uint32_t stackSize;
        bool hasExceptionHandler;

        FunctionUnwindInfo(uint32_t beginRVA, uint32_t endRVA,
                          const UnwindInfo& info,
                          const std::vector<UnwindCode>& codes,
                          const std::vector<uint8_t>& prologue,
                          uint32_t stackSz, bool hasEH)
            : runtimeFunction(beginRVA, endRVA, 0) // unwindInfoRVA set later
            , unwindInfo(info)
            , unwindCodes(codes)
            , prologueBytes(prologue)
            , stackSize(stackSz)
            , hasExceptionHandler(hasEH) {}
    };

    /**
     * @brief Genera datos binarios para UNWIND_INFO
     */
    std::vector<uint8_t> generateUnwindInfoData(const FunctionUnwindInfo& info);

    /**
     * @brief Calcula RVA para UNWIND_INFO de una función
     */
    uint32_t calculateUnwindInfoRVA(size_t functionIndex) const;

    /**
     * @brief Valida información de unwind para una función
     */
    bool validateFunctionUnwind(const FunctionUnwindInfo& info) const;

    // Miembros privados
    std::vector<std::unique_ptr<FunctionUnwindInfo>> functions_;
    uint32_t xdataBaseRVA_;
    uint32_t currentXdataOffset_;
};

} // namespace cpp20::compiler::backend::unwind
