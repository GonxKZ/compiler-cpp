/**
 * @file PeepholeOptimizer.h
 * @brief Optimizador de mirilla (peephole) para instrucciones x86-64
 */

#pragma once

#include <compiler/backend/codegen/InstructionSelector.h>
#include <vector>
#include <unordered_map>
#include <functional>

namespace cpp20::compiler::backend {

/**
 * @brief Patrón de optimización de mirilla
 */
struct PeepholePattern {
    std::vector<X86Opcode> pattern;        // Secuencia de opcodes a buscar
    std::vector<X86Opcode> replacement;    // Secuencia de reemplazo
    std::string description;               // Descripción de la optimización
    std::function<bool(const std::vector<X86Instruction>&)> condition; // Condición adicional

    PeepholePattern(const std::vector<X86Opcode>& p,
                   const std::vector<X86Opcode>& r,
                   const std::string& desc = "",
                   std::function<bool(const std::vector<X86Instruction>&)> cond = nullptr)
        : pattern(p), replacement(r), description(desc), condition(cond) {}
};

/**
 * @brief Optimizador de mirilla para instrucciones x86-64
 */
class PeepholeOptimizer {
public:
    /**
     * @brief Constructor
     */
    PeepholeOptimizer();

    /**
     * @brief Destructor
     */
    ~PeepholeOptimizer();

    /**
     * @brief Optimiza una secuencia de instrucciones
     */
    std::vector<X86Instruction> optimize(const std::vector<X86Instruction>& instructions);

    /**
     * @brief Añade un patrón de optimización personalizado
     */
    void addPattern(const PeepholePattern& pattern);

    /**
     * @brief Elimina patrones de optimización
     */
    void clearPatterns();

    /**
     * @brief Obtiene estadísticas de optimización
     */
    struct OptimizationStats {
        size_t instructionsProcessed = 0;
        size_t optimizationsApplied = 0;
        size_t instructionsRemoved = 0;
        size_t instructionsAdded = 0;
        std::unordered_map<std::string, size_t> patternUsage;
    };
    OptimizationStats getStats() const { return stats_; }

    /**
     * @brief Reinicia estadísticas
     */
    void resetStats();

private:
    std::vector<PeepholePattern> patterns_;
    OptimizationStats stats_;

    /**
     * @brief Inicializa patrones de optimización estándar
     */
    void initializeStandardPatterns();

    /**
     * @brief Aplica optimizaciones en una ventana deslizante
     */
    std::vector<X86Instruction> applyPeepholeOptimizations(
        const std::vector<X86Instruction>& instructions,
        size_t windowSize = 3);

    /**
     * @brief Verifica si una secuencia coincide con un patrón
     */
    bool matchesPattern(const std::vector<X86Instruction>& window,
                       const PeepholePattern& pattern) const;

    /**
     * @brief Aplica un patrón de reemplazo
     */
    std::vector<X86Instruction> applyPattern(
        const std::vector<X86Instruction>& window,
        const PeepholePattern& pattern) const;

    /**
     * @brief Optimizaciones específicas
     */
    std::vector<X86Instruction> optimizeRedundantMoves(
        const std::vector<X86Instruction>& instructions);

    std::vector<X86Instruction> optimizeArithmetic(
        const std::vector<X86Instruction>& instructions);

    std::vector<X86Instruction> optimizeBranches(
        const std::vector<X86Instruction>& instructions);

    std::vector<X86Instruction> optimizeMemoryAccess(
        const std::vector<X86Instruction>& instructions);

    /**
     * @brief Verifica si dos operandos son equivalentes
     */
    bool operandsEqual(const X86Operand& a, const X86Operand& b) const;

    /**
     * @brief Verifica si una instrucción es un NOP efectivo
     */
    bool isEffectiveNop(const X86Instruction& inst) const;

    /**
     * @brief Elimina instrucciones NOP
     */
    std::vector<X86Instruction> removeNops(const std::vector<X86Instruction>& instructions);

    /**
     * @brief Fusiona instrucciones consecutivas cuando sea posible
     */
    std::vector<X86Instruction> fuseInstructions(const std::vector<X86Instruction>& instructions);

    /**
     * @brief Optimiza uso de registros
     */
    std::vector<X86Instruction> optimizeRegisterUsage(const std::vector<X86Instruction>& instructions);
};

/**
 * @brief Utilidades para análisis de instrucciones
 */
class InstructionAnalysis {
public:
    /**
     * @brief Verifica si una instrucción modifica un registro
     */
    static bool modifiesRegister(const X86Instruction& inst, X86Register reg);

    /**
     * @brief Verifica si una instrucción lee un registro
     */
    static bool readsRegister(const X86Instruction& inst, X86Register reg);

    /**
     * @brief Verifica si una instrucción es mov inefectivo
     */
    static bool isIneffectiveMove(const X86Instruction& inst);

    /**
     * @brief Verifica si dos instrucciones pueden fusionarse
     */
    static bool canFuseInstructions(const X86Instruction& a, const X86Instruction& b);

    /**
     * @brief Calcula el costo de una instrucción
     */
    static int calculateInstructionCost(const X86Instruction& inst);

    /**
     * @brief Verifica si una instrucción tiene efectos secundarios
     */
    static bool hasSideEffects(const X86Instruction& inst);

    /**
     * @brief Obtiene registros definidos por una instrucción
     */
    static std::vector<X86Register> getDefinedRegisters(const X86Instruction& inst);

    /**
     * @brief Obtiene registros usados por una instrucción
     */
    static std::vector<X86Register> getUsedRegisters(const X86Instruction& inst);
};

} // namespace cpp20::compiler::backend
