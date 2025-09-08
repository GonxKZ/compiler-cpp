/**
 * @file UnwindCodeGenerator.cpp
 * @brief Implementación del generador de códigos UNWIND_CODE
 */

#include <compiler/backend/unwind/UnwindTypes.h>
#include <algorithm>
#include <cassert>

namespace cpp20::compiler::backend::unwind {

// ============================================================================
// UnwindCodeGenerator - Implementación
// ============================================================================

std::vector<UnwindCode> UnwindCodeGenerator::generateFromPrologue(
    const std::vector<uint8_t>& prologueBytes,
    uint32_t stackSize,
    uint8_t frameReg) {

    std::vector<UnwindCode> codes;
    size_t offset = 0;

    // Analizar prólogo byte por byte
    while (offset < prologueBytes.size()) {
        uint8_t byte = prologueBytes[offset];

        // PUSH reg (0x50 + reg)
        if (byte >= 0x50 && byte <= 0x57) {
            uint8_t reg = byte - 0x50;
            codes.push_back(generatePushNonvol(static_cast<uint8_t>(offset), reg));
            offset += 1;
        }
        // SUB RSP, imm8 (0x83 0xEC imm8)
        else if (byte == 0x83 && offset + 2 < prologueBytes.size() &&
                 prologueBytes[offset + 1] == 0xEC) {
            uint8_t size = prologueBytes[offset + 2];
            auto allocCodes = generateAlloc(static_cast<uint8_t>(offset), size);
            codes.insert(codes.end(), allocCodes.begin(), allocCodes.end());
            offset += 3;
        }
        // SUB RSP, imm32 (0x81 0xEC imm32)
        else if (byte == 0x81 && offset + 5 < prologueBytes.size() &&
                 prologueBytes[offset + 1] == 0xEC) {
            uint32_t size = *reinterpret_cast<const uint32_t*>(&prologueBytes[offset + 2]);
            auto allocCodes = generateAlloc(static_cast<uint8_t>(offset), size);
            codes.insert(codes.end(), allocCodes.begin(), allocCodes.end());
            offset += 6;
        }
        // MOV [RSP + offset], reg (0x89 reg 0x44 0x24 offset)
        else if (byte == 0x89 && offset + 3 < prologueBytes.size() &&
                 prologueBytes[offset + 2] == 0x44 && prologueBytes[offset + 3] == 0x24) {
            uint8_t reg = prologueBytes[offset + 1] >> 3; // Extract reg from ModR/M
            uint8_t saveOffset = prologueBytes[offset + 4];
            codes.push_back(generateSaveNonvol(static_cast<uint8_t>(offset), reg, saveOffset));
            offset += 5;
        }
        // MOV reg, RSP (0x89 0xE0 + reg*8)
        else if (byte == 0x89 && offset + 1 < prologueBytes.size()) {
            uint8_t modrm = prologueBytes[offset + 1];
            if ((modrm & 0xF8) == 0xE0) { // MOV reg, RSP
                uint8_t reg = (modrm >> 3) & 0x07;
                codes.push_back(generateSetFpreg(static_cast<uint8_t>(offset), reg, 0));
                offset += 2;
            } else {
                offset += 1; // Skip unknown byte
            }
        }
        else {
            offset += 1; // Skip unknown byte
        }
    }

    return codes;
}

UnwindCode UnwindCodeGenerator::generatePushNonvol(uint8_t offset, uint8_t reg) {
    return UnwindCode(offset, UnwindOpCode::PUSH_NONVOL, reg);
}

std::vector<UnwindCode> UnwindCodeGenerator::generateAlloc(uint8_t offset, uint32_t size) {
    std::vector<UnwindCode> codes;

    if (size == 0) {
        return codes;
    }

    if (size <= 128) {
        // ALLOC_SMALL: size in 8-byte units (1-16)
        uint8_t scaledSize = (size + 7) / 8; // Round up to 8-byte boundary
        if (scaledSize > 16) scaledSize = 16;
        codes.push_back(UnwindCode(offset, UnwindOpCode::ALLOC_SMALL, scaledSize - 1));
    } else if (size <= 512*1024) {
        // ALLOC_LARGE: size in 8-byte units
        uint16_t scaledSize = (size + 7) / 8;
        codes.push_back(UnwindCode(offset, UnwindOpCode::ALLOC_LARGE, 0)); // 16-bit size follows
        // Note: In real implementation, we'd store scaledSize in the next UNWIND_CODE slot
    } else {
        // Very large allocation - not typical
        codes.push_back(UnwindCode(offset, UnwindOpCode::ALLOC_LARGE, 1)); // 32-bit size
    }

    return codes;
}

UnwindCode UnwindCodeGenerator::generateSaveNonvol(uint8_t offset, uint8_t reg, uint32_t saveOffset) {
    if (saveOffset <= 0xFF) {
        return UnwindCode(offset, UnwindOpCode::SAVE_NONVOL, reg);
    } else {
        return UnwindCode(offset, UnwindOpCode::SAVE_NONVOL_FAR, reg);
    }
}

UnwindCode UnwindCodeGenerator::generateSetFpreg(uint8_t offset, uint8_t reg, uint8_t frameOffset) {
    return UnwindCode(offset, UnwindOpCode::SET_FPREG, reg);
}

// ============================================================================
// UnwindInfoGenerator - Implementación
// ============================================================================

UnwindInfo UnwindInfoGenerator::generateUnwindInfo(
    const std::vector<UnwindCode>& codes,
    uint8_t prologSize,
    bool hasExceptionHandler,
    uint8_t frameReg,
    uint8_t frameOffset) {

    UnwindFlags flags = UnwindFlags::None;
    if (hasExceptionHandler) {
        flags = UnwindFlags::EHHandler;
    }

    return UnwindInfo(UnwindVersion::Version1, flags, prologSize,
                     static_cast<uint8_t>(codes.size()), frameReg, frameOffset);
}

uint32_t UnwindInfoGenerator::calculateUnwindInfoSize(const UnwindInfo& info) {
    uint32_t size = sizeof(UnwindInfo);

    // Add size of unwind codes (each is 2 bytes)
    size += info.countOfCodes * sizeof(UnwindCode);

    // Add size of exception handler info if present
    if ((info.flags & static_cast<uint8_t>(UnwindFlags::EHHandler)) ||
        (info.flags & static_cast<uint8_t>(UnwindFlags::UHHandler))) {
        size += sizeof(ExceptionHandlerInfo);
    }

    // Add size for chained unwind info if present
    if (info.flags & static_cast<uint8_t>(UnwindFlags::ChainInfo)) {
        size += sizeof(RuntimeFunction);
    }

    return size;
}

// ============================================================================
// UnwindValidator - Implementación
// ============================================================================

bool UnwindValidator::validateUnwindCodes(
    const std::vector<UnwindCode>& codes,
    uint8_t prologSize) {

    if (codes.empty()) {
        return prologSize == 0; // Valid if no prologue
    }

    // Check that codes are in ascending offset order
    for (size_t i = 1; i < codes.size(); ++i) {
        if (codes[i].codeOffset < codes[i-1].codeOffset) {
            return false;
        }
    }

    // Check that all offsets are within prologue
    for (const auto& code : codes) {
        if (code.codeOffset >= prologSize) {
            return false;
        }
    }

    // Check for valid operation codes
    for (const auto& code : codes) {
        UnwindOpCode op = static_cast<UnwindOpCode>(code.unwindOp);
        switch (op) {
            case UnwindOpCode::PUSH_NONVOL:
                if (code.opInfo > 15) return false; // Invalid register
                break;
            case UnwindOpCode::ALLOC_SMALL:
                if (code.opInfo > 15) return false; // Invalid size
                break;
            case UnwindOpCode::ALLOC_LARGE:
            case UnwindOpCode::SAVE_NONVOL:
            case UnwindOpCode::SAVE_NONVOL_FAR:
            case UnwindOpCode::SAVE_XMM128:
            case UnwindOpCode::SAVE_XMM128_FAR:
            case UnwindOpCode::SET_FPREG:
            case UnwindOpCode::PUSH_MACHFRAME:
                // These are valid, additional validation could be added
                break;
            default:
                return false; // Unknown operation
        }
    }

    return true;
}

bool UnwindValidator::validateUnwindInfo(const UnwindInfo& info) {
    // Check version
    if (info.version != 1 && info.version != 2) {
        return false;
    }

    // Check frame register validity
    if (info.frameRegister != 0 && (info.frameRegister < 3 || info.frameRegister > 6)) {
        // Valid frame registers: RBX(3), RBP(4), RDI(5), RSI(6)
        return false;
    }

    // Check frame offset
    if (info.frameOffset > 15) {
        return false; // Must be 0-15 (scaled by 16)
    }

    return true;
}

bool UnwindValidator::validateAgainstPrologue(
    const std::vector<UnwindCode>& codes,
    const std::vector<uint8_t>& prologueBytes) {

    // This is a simplified validation
    // In a real implementation, this would disassemble the prologue
    // and verify that unwind codes match the actual instructions

    for (const auto& code : codes) {
        if (code.codeOffset >= prologueBytes.size()) {
            return false; // Code offset beyond prologue
        }
    }

    return true;
}

} // namespace cpp20::compiler::backend::unwind
