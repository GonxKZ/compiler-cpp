/**
 * @file RegisterAllocator.cpp
 * @brief Implementación del asignador de registros lineal (greedy)
 */

#include <compiler/backend/codegen/RegisterAllocator.h>
#include <algorithm>
#include <iostream>

namespace cpp20::compiler::backend {

// ============================================================================
// RegisterAllocator - Implementación
// ============================================================================

RegisterAllocator::RegisterAllocator(const abi::ABIContract& abiContract)
    : abiContract_(abiContract) {
    initializeAvailableRegisters();
}

RegisterAllocator::~RegisterAllocator() = default;

AllocationState RegisterAllocator::allocateRegisters(const ir::IRFunction& function) {
    // Calcular intervalos de vida
    auto intervals = computeLiveIntervals(function);

    // Aplicar algoritmo de asignación lineal
    auto state = linearScanAllocation(intervals);

    // Actualizar estadísticas
    updateStats(state);

    return state;
}

void RegisterAllocator::clear() {
    for (auto& [reg, info] : registerInfo_) {
        info.isAvailable = true;
        info.assignedVirtualReg = -1;
        info.lastUse = -1;
    }
    stats_ = AllocationStats{};
}

void RegisterAllocator::initializeAvailableRegisters() {
    // Registros de propósito general disponibles para asignación
    // (excluyendo los reservado por el ABI)
    availableRegisters_ = {
        X86Register::RAX, X86Register::RBX, X86Register::RCX, X86Register::RDX,
        X86Register::RSI, X86Register::RDI,
        X86Register::R8, X86Register::R9, X86Register::R10, X86Register::R11,
        X86Register::R12, X86Register::R13, X86Register::R14, X86Register::R15
    };

    // Inicializar información de registros
    for (auto reg : availableRegisters_) {
        registerInfo_[reg] = PhysicalRegisterInfo(reg);
    }
}

std::vector<LiveInterval> RegisterAllocator::computeLiveIntervals(const ir::IRFunction& function) {
    std::vector<LiveInterval> intervals;
    std::unordered_map<int, int> firstDef, lastUse;

    int instructionIndex = 0;

    // Recorrer todas las instrucciones para calcular intervalos
    for (const auto& block : function.getBasicBlocks()) {
        for (const auto& inst : block->getInstructions()) {
            // Procesar operandos de entrada
            for (const auto& operand : inst->getOperands()) {
                if (auto regOp = std::dynamic_pointer_cast<ir::Register>(operand)) {
                    int regId = regOp->getId();
                    lastUse[regId] = instructionIndex;
                    if (firstDef.find(regId) == firstDef.end()) {
                        firstDef[regId] = instructionIndex;
                    }
                }
            }

            // Procesar resultado
            if (auto result = inst->getResult()) {
                int regId = result->getId();
                firstDef[regId] = instructionIndex;
                lastUse[regId] = instructionIndex;
            }

            instructionIndex++;
        }
    }

    // Crear intervalos
    for (const auto& [regId, defPoint] : firstDef) {
        int usePoint = lastUse[regId];
        intervals.emplace_back(regId, defPoint, usePoint);
    }

    // Ordenar por punto de inicio
    std::sort(intervals.begin(), intervals.end(), compareIntervals);

    return intervals;
}

X86Register RegisterAllocator::findBestPhysicalRegister(
    const LiveInterval& interval,
    const std::vector<LiveInterval>& activeIntervals) {

    // Buscar registro disponible que no tenga conflictos
    for (auto reg : availableRegisters_) {
        if (isRegisterAvailable(reg, interval, activeIntervals)) {
            return reg;
        }
    }

    // Si no hay disponible, elegir el que libere primero
    return getFurthestUseRegister(activeIntervals);
}

void RegisterAllocator::spillRegister(int virtualReg, AllocationState& state) {
    // Asignar un slot de spill
    int spillSlot = state.nextSpillSlot++;
    state.spilledRegisters.push_back(virtualReg);

    // Actualizar estadísticas
    if (static_cast<size_t>(spillSlot + 1) > state.maxSpillSlots) {
        state.maxSpillSlots = spillSlot + 1;
    }
}

void RegisterAllocator::restoreRegister(int virtualReg, X86Register physicalReg, AllocationState& state) {
    // Remover de la lista de spilled
    auto it = std::find(state.spilledRegisters.begin(), state.spilledRegisters.end(), virtualReg);
    if (it != state.spilledRegisters.end()) {
        state.spilledRegisters.erase(it);
    }
}

bool RegisterAllocator::isRegisterAvailable(
    X86Register reg,
    const LiveInterval& interval,
    const std::vector<LiveInterval>& activeIntervals) const {

    auto it = registerInfo_.find(reg);
    if (it == registerInfo_.end()) {
        return false;
    }

    // Verificar que no haya conflictos con intervalos activos
    for (const auto& active : activeIntervals) {
        if (interval.overlaps(active)) {
            auto activeIt = state.virtualToPhysical.find(active.virtualReg);
            if (activeIt != state.virtualToPhysical.end() && activeIt->second == reg) {
                return false;
            }
        }
    }

    return true;
}

X86Register RegisterAllocator::getFurthestUseRegister(
    const std::vector<LiveInterval>& activeIntervals) const {

    X86Register bestReg = X86Register::RAX;
    int furthestUse = -1;

    for (auto reg : availableRegisters_) {
        auto it = registerInfo_.find(reg);
        if (it != registerInfo_.end()) {
            int lastUse = it->second.lastUse;
            if (lastUse > furthestUse) {
                furthestUse = lastUse;
                bestReg = reg;
            }
        }
    }

    return bestReg;
}

void RegisterAllocator::updateStats(const AllocationState& state) {
    stats_.totalVirtualRegisters = state.virtualToPhysical.size() + state.spilledRegisters.size();
    stats_.registersAssigned = state.virtualToPhysical.size();
    stats_.registersSpilled = state.spilledRegisters.size();
    stats_.spillSlotsUsed = state.maxSpillSlots;
}

AllocationState RegisterAllocator::linearScanAllocation(const std::vector<LiveInterval>& intervals) {
    AllocationState state;
    std::vector<LiveInterval> activeIntervals;

    for (const auto& interval : intervals) {
        // Expulsar intervalos que han terminado
        activeIntervals.erase(
            std::remove_if(activeIntervals.begin(), activeIntervals.end(),
                [&interval](const LiveInterval& active) {
                    return active.endPoint < interval.startPoint;
                }),
            activeIntervals.end()
        );

        // Intentar asignar registro
        X86Register assignedReg = findBestPhysicalRegister(interval, activeIntervals);

        // Verificar si necesitamos spill
        auto regIt = registerInfo_.find(assignedReg);
        if (regIt != registerInfo_.end() && regIt->second.assignedVirtualReg != -1) {
            // Spill del registro actual
            int spilledReg = regIt->second.assignedVirtualReg;
            spillRegister(spilledReg, state);

            // Remover de active
            activeIntervals.erase(
                std::remove_if(activeIntervals.begin(), activeIntervals.end(),
                    [spilledReg](const LiveInterval& active) {
                        return active.virtualReg == spilledReg;
                    }),
                activeIntervals.end()
            );
        }

        // Asignar el registro
        state.virtualToPhysical[interval.virtualReg] = assignedReg;
        registerInfo_[assignedReg].assignedVirtualReg = interval.virtualReg;
        registerInfo_[assignedReg].lastUse = interval.endPoint;

        // Añadir a intervalos activos
        activeIntervals.push_back(interval);
    }

    return state;
}

// ============================================================================
// RegisterAllocationUtils - Implementación
// ============================================================================

std::vector<X86Instruction> RegisterAllocationUtils::generateSpillCode(
    const AllocationState& state,
    bool isPrologue) {

    std::vector<X86Instruction> spillCode;

    if (isPrologue) {
        // Código para guardar registros spilled al inicio de la función
        for (int virtualReg : state.spilledRegisters) {
            auto it = state.virtualToPhysical.find(virtualReg);
            if (it != state.virtualToPhysical.end()) {
                X86Instruction movInst(X86Opcode::MOV);
                // Movimiento a slot de spill (simplificado)
                spillCode.push_back(movInst);
            }
        }
    } else {
        // Código para restaurar registros spilled al final de la función
        for (auto it = state.spilledRegisters.rbegin(); it != state.spilledRegisters.rend(); ++it) {
            int virtualReg = *it;
            auto physIt = state.virtualToPhysical.find(virtualReg);
            if (physIt != state.virtualToPhysical.end()) {
                X86Instruction movInst(X86Opcode::MOV);
                // Movimiento desde slot de spill (simplificado)
                spillCode.push_back(movInst);
            }
        }
    }

    return spillCode;
}

size_t RegisterAllocationUtils::calculateSpillStackSize(const AllocationState& state) {
    // Cada registro spilled necesita 8 bytes en x64
    return state.spilledRegisters.size() * 8;
}

bool RegisterAllocationUtils::validateAllocation(
    const ir::IRFunction& function,
    const AllocationState& state) {

    // Verificar que todos los registros virtuales estén asignados o spilled
    std::unordered_set<int> assignedOrSpilled;

    for (const auto& [virt, phys] : state.virtualToPhysical) {
        assignedOrSpilled.insert(virt);
    }

    for (int spilled : state.spilledRegisters) {
        assignedOrSpilled.insert(spilled);
    }

    // Verificar que no haya conflictos en los registros físicos asignados
    std::unordered_map<X86Register, int> physicalAssignments;

    for (const auto& [virt, phys] : state.virtualToPhysical) {
        auto it = physicalAssignments.find(phys);
        if (it != physicalAssignments.end()) {
            // Conflicto: mismo registro físico asignado a múltiples virtuales
            return false;
        }
        physicalAssignments[phys] = virt;
    }

    return true;
}

void RegisterAllocationUtils::optimizeAllocation(AllocationState& state) {
    // Optimizaciones simples:
    // - Eliminar asignaciones redundantes
    // - Reutilizar registros cuando sea posible

    // Implementación simplificada
    std::unordered_map<X86Register, std::vector<int>> regsByPhys;

    for (const auto& [virt, phys] : state.virtualToPhysical) {
        regsByPhys[phys].push_back(virt);
    }

    // Consolidar asignaciones
    for (auto& [phys, virtRegs] : regsByPhys) {
        if (virtRegs.size() > 1) {
            // Si múltiples virtuales usan el mismo físico,
            // mantener solo el último
            for (size_t i = 0; i < virtRegs.size() - 1; ++i) {
                state.virtualToPhysical.erase(virtRegs[i]);
            }
        }
    }
}

std::unordered_map<int, RegisterMapping> RegisterAllocationUtils::createRegisterMapping(
    const AllocationState& state) {

    std::unordered_map<int, RegisterMapping> mapping;

    for (const auto& [virt, phys] : state.virtualToPhysical) {
        RegisterMapping regMap;
        regMap.virtualReg = virt;
        regMap.physicalReg = phys;

        // Verificar si está spilled
        auto it = std::find(state.spilledRegisters.begin(),
                           state.spilledRegisters.end(), virt);
        if (it != state.spilledRegisters.end()) {
            regMap.isSpilled = true;
            regMap.spillSlot = std::distance(state.spilledRegisters.begin(), it);
        }

        mapping[virt] = regMap;
    }

    return mapping;
}

} // namespace cpp20::compiler::backend
