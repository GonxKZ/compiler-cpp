/**
 * @file RegisterAllocator.h
 * @brief Asignador de registros con algoritmo greedy y manejo de spills
 */

#pragma once

#include <compiler/ir/IR.h>
#include <compiler/backend/abi/ABIContract.h>
#include <compiler/backend/codegen/InstructionSelector.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>

namespace cpp20::compiler::backend {

/**
 * @brief Información de intervalo de vida de un registro virtual
 */
struct LiveInterval {
    int virtualReg;
    int startPoint;     // Punto de definición
    int endPoint;       // Último uso
    bool isSpilled = false;
    int spillSlot = -1;

    LiveInterval(int reg, int start, int end)
        : virtualReg(reg), startPoint(start), endPoint(end) {}

    bool overlaps(const LiveInterval& other) const {
        return !(endPoint < other.startPoint || other.endPoint < startPoint);
    }

    int length() const { return endPoint - startPoint + 1; }
};

/**
 * @brief Información de asignación de registro físico
 */
struct PhysicalRegisterInfo {
    X86Register reg;
    bool isAvailable = true;
    int assignedVirtualReg = -1;
    int lastUse = -1;

    PhysicalRegisterInfo(X86Register r) : reg(r) {}
};

/**
 * @brief Estado del asignador de registros
 */
struct AllocationState {
    std::unordered_map<int, X86Register> virtualToPhysical;
    std::unordered_map<X86Register, int> physicalToVirtual;
    std::vector<int> spilledRegisters;
    int nextSpillSlot = 0;
    size_t maxSpillSlots = 0;
};

/**
 * @brief Asignador de registros lineal (greedy) con manejo de spills
 */
class RegisterAllocator {
public:
    /**
     * @brief Constructor
     */
    RegisterAllocator(const abi::ABIContract& abiContract);

    /**
     * @brief Destructor
     */
    ~RegisterAllocator();

    /**
     * @brief Asigna registros para una función
     */
    AllocationState allocateRegisters(const ir::IRFunction& function);

    /**
     * @brief Libera recursos del asignador
     */
    void clear();

    /**
     * @brief Obtiene estadísticas de asignación
     */
    struct AllocationStats {
        size_t totalVirtualRegisters = 0;
        size_t registersAssigned = 0;
        size_t registersSpilled = 0;
        size_t spillSlotsUsed = 0;
        size_t maxLiveRegisters = 0;
    };
    AllocationStats getStats() const { return stats_; }

private:
    const abi::ABIContract& abiContract_;
    AllocationStats stats_;

    // Registros disponibles para asignación
    std::vector<X86Register> availableRegisters_;
    std::unordered_map<X86Register, PhysicalRegisterInfo> registerInfo_;

    /**
     * @brief Inicializa la lista de registros disponibles
     */
    void initializeAvailableRegisters();

    /**
     * @brief Calcula intervalos de vida para todos los registros virtuales
     */
    std::vector<LiveInterval> computeLiveIntervals(const ir::IRFunction& function);

    /**
     * @brief Encuentra el registro físico más adecuado para asignar
     */
    X86Register findBestPhysicalRegister(
        const LiveInterval& interval,
        const std::vector<LiveInterval>& activeIntervals);

    /**
     * @brief Maneja el spill de un registro
     */
    void spillRegister(int virtualReg, AllocationState& state);

    /**
     * @brief Restaura un registro desde memoria
     */
    void restoreRegister(int virtualReg, X86Register physicalReg, AllocationState& state);

    /**
     * @brief Verifica si un registro físico está disponible en un intervalo
     */
    bool isRegisterAvailable(
        X86Register reg,
        const LiveInterval& interval,
        const std::vector<LiveInterval>& activeIntervals) const;

    /**
     * @brief Obtiene el registro con el último uso más lejano
     */
    X86Register getFurthestUseRegister(
        const std::vector<LiveInterval>& activeIntervals) const;

    /**
     * @brief Actualiza estadísticas
     */
    void updateStats(const AllocationState& state);

    /**
     * @brief Algoritmo principal de asignación lineal
     */
    AllocationState linearScanAllocation(const std::vector<LiveInterval>& intervals);

    /**
     * @brief Ordena intervalos por punto de inicio
     */
    static bool compareIntervals(const LiveInterval& a, const LiveInterval& b) {
        return a.startPoint < b.startPoint;
    }

    /**
     * @brief Verifica si dos intervalos pueden coexistir
     */
    static bool canCoexist(const LiveInterval& a, const LiveInterval& b) {
        return !a.overlaps(b);
    }
};

/**
 * @brief Utilidades para trabajar con asignaciones de registros
 */
class RegisterAllocationUtils {
public:
    /**
     * @brief Genera código para guardar/restaurar registros spilled
     */
    static std::vector<X86Instruction> generateSpillCode(
        const AllocationState& state,
        bool isPrologue);

    /**
     * @brief Calcula el tamaño de stack necesario para spills
     */
    static size_t calculateSpillStackSize(const AllocationState& state);

    /**
     * @brief Valida que una asignación sea correcta
     */
    static bool validateAllocation(
        const ir::IRFunction& function,
        const AllocationState& state);

    /**
     * @brief Optimiza asignación eliminando movs redundantes
     */
    static void optimizeAllocation(AllocationState& state);

    /**
     * @brief Genera mapa de mapeo registro virtual -> físico
     */
    static std::unordered_map<int, RegisterMapping> createRegisterMapping(
        const AllocationState& state);
};

} // namespace cpp20::compiler::backend
