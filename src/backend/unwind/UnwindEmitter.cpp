/**
 * @file UnwindEmitter.cpp
 * @brief Implementación del emisor de unwind para Windows x64
 */

#include <compiler/backend/unwind/UnwindEmitter.h>
#include <algorithm>
#include <cassert>

namespace cpp20::compiler::backend::unwind {

// ============================================================================
// UnwindEmitter - Implementación
// ============================================================================

UnwindEmitter::UnwindEmitter()
    : xdataBaseRVA_(0), currentXdataOffset_(0) {
}

UnwindEmitter::~UnwindEmitter() = default;

void UnwindEmitter::addFunctionUnwind(
    uint32_t functionRVA,
    uint32_t functionSize,
    const std::vector<uint8_t>& prologueBytes,
    uint32_t stackSize,
    uint8_t frameReg,
    bool hasExceptionHandler) {

    // Generar códigos unwind del prólogo
    auto unwindCodes = UnwindCodeGenerator::generateFromPrologue(
        prologueBytes, stackSize, frameReg);

    // Generar UNWIND_INFO
    uint8_t prologSize = static_cast<uint8_t>(prologueBytes.size());
    auto unwindInfo = UnwindInfoGenerator::generateUnwindInfo(
        unwindCodes, prologSize, hasExceptionHandler, frameReg, 0);

    // Crear entrada completa
    auto functionInfo = std::make_unique<FunctionUnwindInfo>(
        functionRVA,
        functionRVA + functionSize,
        unwindInfo,
        unwindCodes,
        prologueBytes,
        stackSize,
        hasExceptionHandler);

    functions_.push_back(std::move(functionInfo));
}

std::vector<uint8_t> UnwindEmitter::generatePdataSection() {
    std::vector<uint8_t> pdata;

    for (size_t i = 0; i < functions_.size(); ++i) {
        const auto& func = functions_[i];

        // Calcular RVA de UNWIND_INFO
        uint32_t unwindInfoRVA = calculateUnwindInfoRVA(i);

        // Actualizar el RuntimeFunction con el RVA correcto
        RuntimeFunction rf(func->runtimeFunction.beginAddress,
                          func->runtimeFunction.endAddress,
                          unwindInfoRVA);

        // Serializar RuntimeFunction (12 bytes en little-endian)
        const uint8_t* rfBytes = reinterpret_cast<const uint8_t*>(&rf);
        pdata.insert(pdata.end(), rfBytes, rfBytes + sizeof(RuntimeFunction));
    }

    return pdata;
}

std::vector<uint8_t> UnwindEmitter::generateXdataSection() {
    std::vector<uint8_t> xdata;
    currentXdataOffset_ = 0;

    for (const auto& func : functions_) {
        auto unwindData = generateUnwindInfoData(*func);
        xdata.insert(xdata.end(), unwindData.begin(), unwindData.end());
    }

    return xdata;
}

uint32_t UnwindEmitter::getPdataSize() const {
    return static_cast<uint32_t>(functions_.size() * sizeof(RuntimeFunction));
}

uint32_t UnwindEmitter::getXdataSize() const {
    uint32_t totalSize = 0;
    for (const auto& func : functions_) {
        totalSize += UnwindInfoGenerator::calculateUnwindInfoSize(func->unwindInfo);
    }
    return totalSize;
}

bool UnwindEmitter::validateAll() const {
    for (const auto& func : functions_) {
        if (!validateFunctionUnwind(*func)) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> UnwindEmitter::generateUnwindInfoData(const FunctionUnwindInfo& info) {
    std::vector<uint8_t> data;

    // Serializar UNWIND_INFO header
    data.push_back(info.unwindInfo.version | (info.unwindInfo.flags << 3));
    data.push_back(info.unwindInfo.sizeOfProlog);
    data.push_back(info.unwindInfo.countOfCodes);
    data.push_back(info.unwindInfo.frameRegister | (info.unwindInfo.frameOffset << 4));

    // Serializar UNWIND_CODEs
    for (const auto& code : info.unwindCodes) {
        data.push_back(code.codeOffset);
        data.push_back(code.unwindOp | (code.opInfo << 4));
    }

    // Si hay exception handler, añadir información adicional
    if (info.hasExceptionHandler) {
        // Exception handler RVA (placeholder - 4 bytes)
        uint32_t ehRVA = 0;
        const uint8_t* ehBytes = reinterpret_cast<const uint8_t*>(&ehRVA);
        data.insert(data.end(), ehBytes, ehBytes + 4);

        // Exception data RVA (placeholder - 4 bytes)
        uint32_t edRVA = 0;
        const uint8_t* edBytes = reinterpret_cast<const uint8_t*>(&edRVA);
        data.insert(data.end(), edBytes, edBytes + 4);
    }

    // Actualizar offset para siguiente función
    currentXdataOffset_ += static_cast<uint32_t>(data.size());

    return data;
}

uint32_t UnwindEmitter::calculateUnwindInfoRVA(size_t functionIndex) const {
    if (xdataBaseRVA_ == 0) {
        return 0; // No base RVA set
    }

    uint32_t offset = 0;

    // Calcular offset sumando tamaños de funciones anteriores
    for (size_t i = 0; i < functionIndex; ++i) {
        offset += UnwindInfoGenerator::calculateUnwindInfoSize(functions_[i]->unwindInfo);
    }

    return xdataBaseRVA_ + offset;
}

bool UnwindEmitter::validateFunctionUnwind(const FunctionUnwindInfo& info) const {
    // Validar códigos unwind
    if (!UnwindValidator::validateUnwindCodes(info.unwindCodes, info.unwindInfo.sizeOfProlog)) {
        return false;
    }

    // Validar UNWIND_INFO
    if (!UnwindValidator::validateUnwindInfo(info.unwindInfo)) {
        return false;
    }

    // Validar contra prólogo
    if (!UnwindValidator::validateAgainstPrologue(info.unwindCodes, info.prologueBytes)) {
        return false;
    }

    // Validar direcciones
    if (info.runtimeFunction.beginAddress >= info.runtimeFunction.endAddress) {
        return false;
    }

    return true;
}

} // namespace cpp20::compiler::backend::unwind
