/**
 * @file PeepholeOptimizer.cpp
 * @brief Implementación del optimizador de mirilla
 */

#include <compiler/backend/optimization/PeepholeOptimizer.h>
#include <algorithm>

namespace cpp20::compiler::backend {

// ============================================================================
// PeepholeOptimizer - Implementación
// ============================================================================

PeepholeOptimizer::PeepholeOptimizer() {
    initializeStandardPatterns();
}

PeepholeOptimizer::~PeepholeOptimizer() = default;

std::vector<X86Instruction> PeepholeOptimizer::optimize(const std::vector<X86Instruction>& instructions) {
    stats_.instructionsProcessed = instructions.size();

    auto optimized = instructions;

    // Aplicar múltiples pasadas de optimización
    bool changed = true;
    int maxPasses = 3; // Límite para evitar bucles infinitos
    int pass = 0;

    while (changed && pass < maxPasses) {
        changed = false;
        size_t originalSize = optimized.size();

        // Aplicar optimizaciones de mirilla
        optimized = applyPeepholeOptimizations(optimized);

        // Eliminar NOPs
        optimized = removeNops(optimized);

        // Fusionar instrucciones
        optimized = fuseInstructions(optimized);

        // Optimizar uso de registros
        optimized = optimizeRegisterUsage(optimized);

        // Verificar si hubo cambios
        if (optimized.size() != originalSize) {
            changed = true;
        }

        pass++;
    }

    stats_.instructionsRemoved = stats_.instructionsProcessed - optimized.size();

    return optimized;
}

void PeepholeOptimizer::addPattern(const PeepholePattern& pattern) {
    patterns_.push_back(pattern);
}

void PeepholeOptimizer::clearPatterns() {
    patterns_.clear();
    initializeStandardPatterns();
}

void PeepholeOptimizer::resetStats() {
    stats_ = OptimizationStats{};
}

void PeepholeOptimizer::initializeStandardPatterns() {
    // Patrón 1: MOV reg, reg -> eliminar (mov redundante)
    patterns_.emplace_back(
        std::vector<X86Opcode>{X86Opcode::MOV},
        std::vector<X86Opcode>{}, // Eliminar
        "Redundant MOV elimination",
        [this](const std::vector<X86Instruction>& window) {
            if (window.size() >= 1) {
                const auto& inst = window[0];
                return InstructionAnalysis::isIneffectiveMove(inst);
            }
            return false;
        }
    );

    // Patrón 2: ADD reg, 0 -> eliminar
    patterns_.emplace_back(
        std::vector<X86Opcode>{X86Opcode::ADD},
        std::vector<X86Opcode>{},
        "ADD by zero elimination",
        [](const std::vector<X86Instruction>& window) {
            if (window.size() >= 1) {
                const auto& inst = window[0];
                return inst.opcode == X86Opcode::ADD &&
                       !inst.operands.empty() &&
                       inst.operands.back().mode == AddressingMode::Immediate &&
                       inst.operands.back().immediate == 0;
            }
            return false;
        }
    );

    // Patrón 3: SUB reg, 0 -> eliminar
    patterns_.emplace_back(
        std::vector<X86Opcode>{X86Opcode::SUB},
        std::vector<X86Opcode>{},
        "SUB by zero elimination",
        [](const std::vector<X86Instruction>& window) {
            if (window.size() >= 1) {
                const auto& inst = window[0];
                return inst.opcode == X86Opcode::SUB &&
                       !inst.operands.empty() &&
                       inst.operands.back().mode == AddressingMode::Immediate &&
                       inst.operands.back().immediate == 0;
            }
            return false;
        }
    );

    // Patrón 4: MOV reg1, reg2; MOV reg2, reg1 -> XCHG reg1, reg2
    patterns_.emplace_back(
        std::vector<X86Opcode>{X86Opcode::MOV, X86Opcode::MOV},
        std::vector<X86Opcode>{X86Opcode::MOV}, // Simplificar a un MOV
        "MOV pair optimization",
        [this](const std::vector<X86Instruction>& window) {
            if (window.size() >= 2) {
                const auto& inst1 = window[0];
                const auto& inst2 = window[1];
                return inst1.opcode == X86Opcode::MOV && inst2.opcode == X86Opcode::MOV &&
                       operandsEqual(inst1.operands[0], inst2.operands[1]) &&
                       operandsEqual(inst1.operands[1], inst2.operands[0]);
            }
            return false;
        }
    );

    // Patrón 5: CMP reg, 0; JE/JNE -> TEST reg, reg; JE/JNE
    patterns_.emplace_back(
        std::vector<X86Opcode>{X86Opcode::CMP},
        std::vector<X86Opcode>{X86Opcode::TEST},
        "CMP to TEST optimization",
        [](const std::vector<X86Instruction>& window) {
            if (window.size() >= 1) {
                const auto& inst = window[0];
                return inst.opcode == X86Opcode::CMP &&
                       inst.operands.size() >= 2 &&
                       inst.operands[1].mode == AddressingMode::Immediate &&
                       inst.operands[1].immediate == 0;
            }
            return false;
        }
    );
}

std::vector<X86Instruction> PeepholeOptimizer::applyPeepholeOptimizations(
    const std::vector<X86Instruction>& instructions,
    size_t windowSize) {

    std::vector<X86Instruction> result;
    size_t i = 0;

    while (i < instructions.size()) {
        bool matched = false;

        // Probar cada patrón
        for (const auto& pattern : patterns_) {
            if (pattern.pattern.size() > instructions.size() - i) {
                continue; // No hay suficientes instrucciones
            }

            // Extraer ventana
            std::vector<X86Instruction> window(
                instructions.begin() + i,
                instructions.begin() + i + pattern.pattern.size()
            );

            // Verificar coincidencia
            if (matchesPattern(window, pattern)) {
                // Aplicar reemplazo
                auto replacement = applyPattern(window, pattern);
                result.insert(result.end(), replacement.begin(), replacement.end());

                // Actualizar estadísticas
                stats_.optimizationsApplied++;
                stats_.patternUsage[pattern.description]++;

                i += pattern.pattern.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            // No hubo coincidencia, copiar instrucción tal cual
            result.push_back(instructions[i]);
            i++;
        }
    }

    return result;
}

bool PeepholeOptimizer::matchesPattern(const std::vector<X86Instruction>& window,
                                      const PeepholePattern& pattern) const {

    // Verificar opcodes
    if (window.size() != pattern.pattern.size()) {
        return false;
    }

    for (size_t i = 0; i < window.size(); ++i) {
        if (window[i].opcode != pattern.pattern[i]) {
            return false;
        }
    }

    // Verificar condición adicional
    if (pattern.condition && !pattern.condition(window)) {
        return false;
    }

    return true;
}

std::vector<X86Instruction> PeepholeOptimizer::applyPattern(
    const std::vector<X86Instruction>& window,
    const PeepholePattern& pattern) const {

    std::vector<X86Instruction> result;

    // Si el reemplazo está vacío, no añadir nada (eliminación)
    if (pattern.replacement.empty()) {
        return result;
    }

    // Para esta implementación simplificada, crear instrucciones básicas
    // En un compilador real, esto sería más sofisticado
    for (auto opcode : pattern.replacement) {
        X86Instruction inst(opcode);
        // Copiar operandos de la primera instrucción de la ventana (simplificado)
        if (!window.empty()) {
            inst.operands = window[0].operands;
            inst.comment = pattern.description;
        }
        result.push_back(inst);
    }

    return result;
}

std::vector<X86Instruction> PeepholeOptimizer::removeNops(const std::vector<X86Instruction>& instructions) {
    std::vector<X86Instruction> result;

    for (const auto& inst : instructions) {
        if (!isEffectiveNop(inst)) {
            result.push_back(inst);
        } else {
            stats_.instructionsRemoved++;
        }
    }

    return result;
}

std::vector<X86Instruction> PeepholeOptimizer::fuseInstructions(const std::vector<X86Instruction>& instructions) {
    std::vector<X86Instruction> result;

    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& inst = instructions[i];

        // Intentar fusionar con la siguiente instrucción
        if (i + 1 < instructions.size()) {
            const auto& nextInst = instructions[i + 1];

            if (InstructionAnalysis::canFuseInstructions(inst, nextInst)) {
                // Fusionar instrucciones (simplificado)
                X86Instruction fusedInst = inst; // Mantener la primera
                fusedInst.comment = "Fused instruction";
                result.push_back(fusedInst);
                i++; // Saltar la siguiente
                stats_.optimizationsApplied++;
                continue;
            }
        }

        result.push_back(inst);
    }

    return result;
}

std::vector<X86Instruction> PeepholeOptimizer::optimizeRegisterUsage(const std::vector<X86Instruction>& instructions) {
    // Optimizaciones de uso de registros (simplificado)
    // En un compilador real, esto incluiría análisis de interferencia, etc.

    std::vector<X86Instruction> result = instructions;

    // Optimización simple: eliminar asignaciones temporales no utilizadas
    std::unordered_map<X86Register, size_t> lastUse;

    // Primera pasada: registrar último uso de cada registro
    for (size_t i = 0; i < result.size(); ++i) {
        const auto& inst = result[i];
        auto usedRegs = InstructionAnalysis::getUsedRegisters(inst);

        for (auto reg : usedRegs) {
            lastUse[reg] = i;
        }
    }

    // Segunda pasada: eliminar instrucciones que definen registros no utilizados
    std::vector<X86Instruction> optimized;
    for (size_t i = 0; i < result.size(); ++i) {
        const auto& inst = result[i];
        auto definedRegs = InstructionAnalysis::getDefinedRegisters(inst);

        bool allDead = true;
        for (auto reg : definedRegs) {
            auto it = lastUse.find(reg);
            if (it == lastUse.end() || it->second > i) {
                // Registro se usa después o no se encontró
                allDead = false;
                break;
            }
        }

        if (!allDead || InstructionAnalysis::hasSideEffects(inst)) {
            optimized.push_back(inst);
        } else {
            stats_.instructionsRemoved++;
        }
    }

    return optimized;
}

bool PeepholeOptimizer::operandsEqual(const X86Operand& a, const X86Operand& b) const {
    if (a.mode != b.mode) return false;

    switch (a.mode) {
        case AddressingMode::Register:
            return a.reg == b.reg;
        case AddressingMode::Immediate:
            return a.immediate == b.immediate;
        case AddressingMode::MemoryDirect:
            return a.immediate == b.immediate;
        default:
            return false;
    }
}

bool PeepholeOptimizer::isEffectiveNop(const X86Instruction& inst) const {
    // Verificar si es un NOP efectivo
    if (inst.opcode == X86Opcode::NOP) {
        return true;
    }

    // Verificar movs redundantes
    return InstructionAnalysis::isIneffectiveMove(inst);
}

// ============================================================================
// InstructionAnalysis - Implementación
// ============================================================================

bool InstructionAnalysis::modifiesRegister(const X86Instruction& inst, X86Register reg) {
    auto defined = getDefinedRegisters(inst);
    return std::find(defined.begin(), defined.end(), reg) != defined.end();
}

bool InstructionAnalysis::readsRegister(const X86Instruction& inst, X86Register reg) {
    auto used = getUsedRegisters(inst);
    return std::find(used.begin(), used.end(), reg) != used.end();
}

bool InstructionAnalysis::isIneffectiveMove(const X86Instruction& inst) {
    if (inst.opcode != X86Opcode::MOV) {
        return false;
    }

    if (inst.operands.size() < 2) {
        return false;
    }

    // MOV reg, reg (mismo registro)
    if (inst.operands[0].mode == AddressingMode::Register &&
        inst.operands[1].mode == AddressingMode::Register &&
        inst.operands[0].reg == inst.operands[1].reg) {
        return true;
    }

    // MOV reg, inmediato 0 cuando el registro ya es 0 (simplificado)
    // En un compilador real, esto requeriría análisis de flujo de datos

    return false;
}

bool InstructionAnalysis::canFuseInstructions(const X86Instruction& a, const X86Instruction& b) {
    // Verificar si dos instrucciones pueden fusionarse
    // Esto es muy simplificado - en un compilador real sería mucho más complejo

    // No fusionar instrucciones con efectos secundarios
    if (hasSideEffects(a) || hasSideEffects(b)) {
        return false;
    }

    // Fusionar operaciones aritméticas consecutivas (muy simplificado)
    if ((a.opcode == X86Opcode::ADD || a.opcode == X86Opcode::SUB) &&
        (b.opcode == X86Opcode::ADD || b.opcode == X86Opcode::SUB)) {
        return true;
    }

    return false;
}

int InstructionAnalysis::calculateInstructionCost(const X86Instruction& inst) {
    // Costos aproximados de instrucciones (en ciclos de reloj)
    switch (inst.opcode) {
        case X86Opcode::MOV:
            return 1;
        case X86Opcode::ADD:
        case X86Opcode::SUB:
            return 1;
        case X86Opcode::IMUL:
        case X86Opcode::IDIV:
            return 3;
        case X86Opcode::CMP:
        case X86Opcode::TEST:
            return 1;
        case X86Opcode::JMP:
            return 1;
        case X86Opcode::CALL:
            return 2;
        case X86Opcode::RET:
            return 1;
        default:
            return 2; // Costo por defecto
    }
}

bool InstructionAnalysis::hasSideEffects(const X86Instruction& inst) {
    // Instrucciones que pueden tener efectos secundarios
    switch (inst.opcode) {
        case X86Opcode::CALL:
        case X86Opcode::RET:
        case X86Opcode::PUSH:
        case X86Opcode::POP:
        case X86Opcode::JMP:
        case X86Opcode::JE:
        case X86Opcode::JNE:
        case X86Opcode::JL:
        case X86Opcode::JLE:
        case X86Opcode::JG:
        case X86Opcode::JGE:
            return true;
        default:
            return false;
    }
}

std::vector<X86Register> InstructionAnalysis::getDefinedRegisters(const X86Instruction& inst) {
    std::vector<X86Register> defined;

    // Analizar operandos de destino
    if (!inst.operands.empty()) {
        const auto& firstOp = inst.operands[0];
        if (firstOp.mode == AddressingMode::Register) {
            defined.push_back(firstOp.reg);
        }
    }

    return defined;
}

std::vector<X86Register> InstructionAnalysis::getUsedRegisters(const X86Instruction& inst) {
    std::vector<X86Register> used;

    // Analizar operandos fuente
    for (size_t i = 1; i < inst.operands.size(); ++i) {
        const auto& op = inst.operands[i];
        if (op.mode == AddressingMode::Register) {
            used.push_back(op.reg);
        } else if (op.mode == AddressingMode::MemoryIndirect ||
                   op.mode == AddressingMode::MemoryBaseDisp ||
                   op.mode == AddressingMode::MemoryBaseIndex ||
                   op.mode == AddressingMode::MemoryBaseIndexDisp) {
            // Registros usados en direccionamiento indirecto
            if (op.reg != X86Register::RAX) { // Placeholder
                used.push_back(op.reg);
            }
            if (op.baseReg != X86Register::RAX) {
                used.push_back(op.baseReg);
            }
            if (op.indexReg != X86Register::RAX) {
                used.push_back(op.indexReg);
            }
        }
    }

    return used;
}

} // namespace cpp20::compiler::backend
