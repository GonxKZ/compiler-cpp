/**
 * @file ExceptionMapper.cpp
 * @brief Implementación del mapeo de excepciones C++ a Windows EH
 */

#include <compiler/backend/unwind/ExceptionMapper.h>
#include <algorithm>

namespace cpp20::compiler::backend::unwind {

// ============================================================================
// ExceptionMapper - Implementación
// ============================================================================

ExceptionMapper::ExceptionMapper()
    : windowsHandler_(nullptr) {
}

ExceptionMapper::~ExceptionMapper() = default;

void ExceptionMapper::addTryCatchRegion(const TryCatchRegion& region) {
    tryCatchRegions_.push_back(region);
}

void ExceptionMapper::addThrowSite(const ThrowSite& throwSite) {
    throwSites_.push_back(throwSite);
}

uint32_t ExceptionMapper::generateExceptionHandler() {
    if (!hasExceptions()) {
        return 0; // No exceptions, no handler needed
    }

    // Generate handler RVA (placeholder - in real implementation this would be
    // the RVA of the generated exception handler function)
    uint32_t handlerRVA = 0x1000; // Placeholder RVA

    // Generate exception data RVA
    uint32_t exceptionDataRVA = handlerRVA + 0x100; // Placeholder offset

    // Create Windows exception handler structure
    windowsHandler_ = std::make_unique<WindowsExceptionHandler>(
        handlerRVA, exceptionDataRVA);

    // Copy regions and throws
    windowsHandler_->regions = tryCatchRegions_;
    windowsHandler_->throws = throwSites_;

    return handlerRVA;
}

std::vector<uint8_t> ExceptionMapper::generateExceptionData() {
    if (!windowsHandler_) {
        return {}; // No handler generated
    }

    return generateWindowsExceptionData();
}

std::vector<uint8_t> ExceptionMapper::generateWindowsExceptionData() {
    std::vector<uint8_t> data;

    // Windows Exception Data Format (simplified)
    // This is a placeholder implementation. In a real compiler, this would
    // generate the proper exception data structures for Windows EH.

    // Number of try/catch regions
    uint32_t regionCount = static_cast<uint32_t>(tryCatchRegions_.size());
    const uint8_t* countBytes = reinterpret_cast<const uint8_t*>(&regionCount);
    data.insert(data.end(), countBytes, countBytes + 4);

    // Try/catch region data
    for (const auto& region : tryCatchRegions_) {
        // Try start RVA
        const uint8_t* tryStartBytes = reinterpret_cast<const uint8_t*>(&region.tryStart);
        data.insert(data.end(), tryStartBytes, tryStartBytes + 4);

        // Try end RVA
        const uint8_t* tryEndBytes = reinterpret_cast<const uint8_t*>(&region.tryEnd);
        data.insert(data.end(), tryEndBytes, tryEndBytes + 4);

        // Catch start RVA
        const uint8_t* catchStartBytes = reinterpret_cast<const uint8_t*>(&region.catchStart);
        data.insert(data.end(), catchStartBytes, catchStartBytes + 4);

        // Catch end RVA
        const uint8_t* catchEndBytes = reinterpret_cast<const uint8_t*>(&region.catchEnd);
        data.insert(data.end(), catchEndBytes, catchEndBytes + 4);

        // Exception type RVA
        const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&region.exceptionTypeRVA);
        data.insert(data.end(), typeBytes, typeBytes + 4);
    }

    // Number of throw sites
    uint32_t throwCount = static_cast<uint32_t>(throwSites_.size());
    const uint8_t* throwCountBytes = reinterpret_cast<const uint8_t*>(&throwCount);
    data.insert(data.end(), throwCountBytes, throwCountBytes + 4);

    // Throw site data
    for (const auto& throwSite : throwSites_) {
        // Throw RVA
        const uint8_t* throwBytes = reinterpret_cast<const uint8_t*>(&throwSite.throwRVA);
        data.insert(data.end(), throwBytes, throwBytes + 4);

        // Exception type RVA
        const uint8_t* typeBytes = reinterpret_cast<const uint8_t*>(&throwSite.exceptionTypeRVA);
        data.insert(data.end(), typeBytes, typeBytes + 4);
    }

    return data;
}

} // namespace cpp20::compiler::backend::unwind
