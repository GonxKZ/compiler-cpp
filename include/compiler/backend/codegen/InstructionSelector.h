/**
 * @file InstructionSelector.h
 * @brief Selector de instrucciones para conversión IR -> x86-64
 */

#pragma once

#include <compiler/ir/IR.h>
#include <compiler/backend/abi/ABIContract.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace cpp20::compiler::backend {

/**
 * @brief Instrucciones x86-64
 */
enum class X86Opcode {
    // Movimiento de datos
    MOV, MOVZX, MOVSX, LEA,

    // Operaciones aritméticas
    ADD, SUB, IMUL, IDIV, INC, DEC, NEG,

    // Operaciones lógicas
    AND, OR, XOR, NOT, SHL, SHR, SAR,

    // Comparaciones y saltos
    CMP, TEST, JMP, JE, JNE, JL, JLE, JG, JGE,
    JB, JBE, JA, JAE, JS, JNS, JC, JNC,

    // Llamadas y retorno
    CALL, RET, LEAVE, ENTER,

    // Operaciones de pila
    PUSH, POP,

    // Operaciones de punto flotante (SSE/AVX)
    MOVSS, MOVSD, ADDSS, ADDSD, SUBSS, SUBSD,
    MULSS, MULSD, DIVSS, DIVSD, COMISS, COMISD,

    // Operaciones SIMD
    MOVAPS, MOVUPS, ADDPS, ADDPD,

    // Instrucciones de control
    NOP, HLT,

    // Prefijos
    LOCK, REP, REPZ, REPNZ
};

/**
 * @brief Registros x86-64
 */
enum class X86Register {
    // Registros de propósito general de 64 bits
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15,

    // Registros de 32 bits
    EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP,
    R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,

    // Registros de 16 bits
    AX, BX, CX, DX, SI, DI, BP, SP,
    R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,

    // Registros de 8 bits
    AL, BL, CL, DL, SIL, DIL, BPL, SPL,
    R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,

    // Registros de punto flotante
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,

    // Registros SIMD
    YMM0, YMM1, YMM2, YMM3, YMM4, YMM5, YMM6, YMM7,
    ZMM0, ZMM1, ZMM2, ZMM3, ZMM4, ZMM5, ZMM6, ZMM7,

    // Registros de segmento
    CS, DS, SS, ES, FS, GS,

    // Registro de flags
    RFLAGS
};

/**
 * @brief Modos de direccionamiento x86-64
 */
enum class AddressingMode {
    Register,           // Registro directo
    Immediate,          // Valor inmediato
    MemoryDirect,       // [address]
    MemoryIndirect,     // [reg]
    MemoryBaseDisp,     // [reg + displacement]
    MemoryBaseIndex,    // [reg + index*scale]
    MemoryBaseIndexDisp // [reg + index*scale + displacement]
};

/**
 * @brief Operando de instrucción x86-64
 */
struct X86Operand {
    AddressingMode mode;
    X86Register reg = X86Register::RAX;
    int64_t immediate = 0;
    int32_t displacement = 0;
    X86Register baseReg = X86Register::RAX;
    X86Register indexReg = X86Register::RAX;
    uint8_t scale = 1;  // 1, 2, 4, 8

    X86Operand(AddressingMode m = AddressingMode::Register) : mode(m) {}
};

/**
 * @brief Instrucción x86-64
 */
struct X86Instruction {
    X86Opcode opcode;
    std::vector<X86Operand> operands;
    std::string comment;

    X86Instruction(X86Opcode op = X86Opcode::NOP) : opcode(op) {}
};

/**
 * @brief Información de mapeo registro virtual -> registro físico
 */
struct RegisterMapping {
    int virtualReg;
    X86Register physicalReg;
    bool isSpilled = false;
    int spillSlot = -1;
};

/**
 * @brief Selector de instrucciones para x86-64
 */
class InstructionSelector {
public:
    /**
     * @brief Constructor
     */
    InstructionSelector(const abi::ABIContract& abiContract);

    /**
     * @brief Destructor
     */
    ~InstructionSelector();

    /**
     * @brief Selecciona instrucciones para una función IR
     */
    std::vector<X86Instruction> selectInstructions(
        const ir::IRFunction& function,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para una instrucción IR
     */
    std::vector<X86Instruction> selectInstruction(
        const ir::IRInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Genera prólogo de función
     */
    std::vector<X86Instruction> generateFunctionPrologue(
        const ir::IRFunction& function,
        size_t stackSize);

    /**
     * @brief Genera epílogo de función
     */
    std::vector<X86Instruction> generateFunctionEpilogue(
        const ir::IRFunction& function);

    /**
     * @brief Convierte instrucciones a texto ensamblador
     */
    std::string instructionsToAssembly(const std::vector<X86Instruction>& instructions);

private:
    const abi::ABIContract& abiContract_;

    /**
     * @brief Selecciona instrucciones para operación binaria
     */
    std::vector<X86Instruction> selectBinaryOperation(
        const ir::BinaryInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para operación unaria
     */
    std::vector<X86Instruction> selectUnaryOperation(
        const ir::UnaryInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para load
     */
    std::vector<X86Instruction> selectLoad(
        const ir::LoadInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para store
     */
    std::vector<X86Instruction> selectStore(
        const ir::StoreInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para branch
     */
    std::vector<X86Instruction> selectBranch(
        const ir::BranchInstruction& instruction);

    /**
     * @brief Selecciona instrucciones para return
     */
    std::vector<X86Instruction> selectReturn(
        const ir::ReturnInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Selecciona instrucciones para call
     */
    std::vector<X86Instruction> selectCall(
        const ir::CallInstruction& instruction,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Convierte operando IR a operando x86
     */
    X86Operand convertOperand(
        const std::shared_ptr<ir::IROperand>& operand,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Obtiene registro físico para registro virtual
     */
    X86Register getPhysicalRegister(
        int virtualReg,
        const std::unordered_map<int, RegisterMapping>& registerMap);

    /**
     * @brief Crea operando inmediato
     */
    X86Operand createImmediateOperand(int64_t value);

    /**
     * @brief Crea operando registro
     */
    X86Operand createRegisterOperand(X86Register reg);

    /**
     * @brief Crea operando memoria
     */
    X86Operand createMemoryOperand(X86Register base, int32_t displacement = 0);

    /**
     * @brief Obtiene nombre de registro como string
     */
    std::string registerToString(X86Register reg) const;

    /**
     * @brief Obtiene nombre de opcode como string
     */
    std::string opcodeToString(X86Opcode opcode) const;

    /**
     * @brief Genera instrucciones de movimiento
     */
    std::vector<X86Instruction> generateMove(X86Operand dst, X86Operand src);

    /**
     * @brief Optimiza secuencia de instrucciones
     */
    void optimizeInstructionSequence(std::vector<X86Instruction>& instructions);
};

} // namespace cpp20::compiler::backend
